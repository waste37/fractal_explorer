#include <mandelbrot.h>
#include <cstring>
#include <iostream>
#include <wayland-client.h>
#include <wayland-util.h>
#include <xdg-shell-client-protocol.h>

void blitBuffer(Buffer *dest, Buffer *src) {
    i32 max_x = min(dest->width, src->width);
    i32 max_y = min(dest->height, src->height);
    for (i32 y = 0; y < max_y; ++y) {
        for (i32 x = 0; x < max_x; ++x) {
            dest->data[(y * dest->width) + x] = src->data[(y * src->width) + x];
        }
    }
}

// posix
#include <sys/mman.h>
#include <sys/stat.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <unistd.h>

static void randname(char *buf) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    u64 r = ts.tv_nsec;
    while (*buf) {
        *buf++ = 'A'+(r&15)+(r&16)*2;
        r >>= 5;
    }
}

static i32 create_shm_file(void) {
    i32 retries = 100;
    do {
        char name[] = "/wl_shm-XXXXXX";
        randname(name + sizeof(name) - 7);
        --retries;
        i32 fd = shm_open(name, O_RDWR | O_CREAT | O_EXCL, 0600);
        if (fd >= 0) {
            shm_unlink(name);
            return fd;
        }
    } while (retries > 0 && errno == EEXIST);
    return -1;
}

static i32 allocate_shm_file(usize size) {
    i32 fd = create_shm_file();
    if (fd < 0)
        return -1;
    i32 ret;
    do {
        ret = ftruncate(fd, size);
    } while (ret < 0 && errno == EINTR);
    if (ret < 0) {
        close(fd);
        return -1;
    }
    return fd;
}

struct WindowHandle {
    Buffer *canvas;
    Vec2<i32> size;
    bool should_close;
    f64 delta_frame;
    f64 last_frame;
    struct {
        wl_display *display;
        wl_shm *shm;
        wl_compositor *compositor;
        wl_surface *surface;
        xdg_wm_base *wm_base;
        wl_seat *seat;
        wl_pointer *pointer;
    } wl;
    struct {
        xdg_surface *surface;
        u32 serial;
        bool ack_configure, was_resize;
    } todo;
    struct {
        i32 current;
        struct {
            wl_buffer *handle;
            i32 width, height;
            u32 *data;
            bool held;
        } buf[2];
    } fb;
    struct {
        Vec2<double> pointer;
        Vec2<double> pointer_delta;
        int buttons[MAX_SCANCODES];
    } input;

    struct {
        bool resized;
        Vec2<f64> axis;
    } frame_events;
};

static void wlRegistryHandleGlobal(void *data, wl_registry *reg, u32 name, const char *iface, u32 version) {
    WindowHandle *w = (WindowHandle*)data;
    if (std::strcmp(wl_compositor_interface.name, iface) == 0) {
        if (version < 6) {
            w->should_close = true;
            return;
        }
        w->wl.compositor = (wl_compositor*)wl_registry_bind(reg, name, &wl_compositor_interface, version);
    } else if (std::strcmp(wl_shm_interface.name, iface) == 0) {
        if (version < 1) {
            w->should_close = true;
            return;
        }
        w->wl.shm = (wl_shm*)wl_registry_bind(reg, name, &wl_shm_interface, version);
    } else if (std::strcmp(xdg_wm_base_interface.name, iface) == 0) {
        if (version < 6) {
            w->should_close = true;
            return;
        }
        w->wl.wm_base = (xdg_wm_base*)wl_registry_bind(reg, name, &xdg_wm_base_interface, version);
    } else if (std::strcmp(wl_seat_interface.name, iface) == 0) {
        if (version < 9) {
            w->should_close = true;
            return;
        }
        w->wl.seat = (wl_seat*)wl_registry_bind(reg, name, &wl_seat_interface, version);
    }
}

static void wlRegistryHandleGlobalRemove(void *data, wl_registry *reg, u32 name) { 
    // maybe we should die?? never happens though
}

static const struct wl_registry_listener wl_registry_listener = {
    .global = wlRegistryHandleGlobal,
    .global_remove = wlRegistryHandleGlobalRemove
};

static void xdgWmBaseHandlePing(void *data, struct xdg_wm_base *xdg_wm_base, u32 serial) {
    xdg_wm_base_pong(xdg_wm_base, serial);
}

static const struct xdg_wm_base_listener xdg_wm_base_listener = {
    .ping = xdgWmBaseHandlePing
};

static void wlBufferHandleRelease(void *data, wl_buffer *buffer);
static const struct wl_buffer_listener wl_buffer_listener = {
    .release = wlBufferHandleRelease,
};


bool allocateWindowBuffer(WindowHandle *w, i32 index) {
    usize buffer_size = w->size.x * 4 * w->size.y;
    i32 fd = allocate_shm_file(buffer_size);
    if (fd == -1) return false;
    u32 *data = (u32*)mmap(nullptr, buffer_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (data == MAP_FAILED) return false;
    wl_shm_pool *pool = wl_shm_create_pool(w->wl.shm, fd, buffer_size);
    wl_buffer *buffer = wl_shm_pool_create_buffer(pool, 0, w->size.x, w->size.y, 4 * w->size.x, WL_SHM_FORMAT_ARGB8888);
    if (!buffer) return false;
    for (i32 i = 0; i < w->size.x * w->size.y; ++i) { data[i] = 0xff000000; }
    wl_buffer_add_listener(buffer, &wl_buffer_listener, w);
    w->fb.buf[index].data = data;
    w->fb.buf[index].handle = buffer;
    w->fb.buf[index].width = w->size.x;
    w->fb.buf[index].height = w->size.y;
    w->fb.buf[index].held = false;
    wl_shm_pool_destroy(pool);
    close(fd);
    return true;
}

static void ackXdgSurfaceConfigure(WindowHandle *w, xdg_surface *surface, u32 serial) {
    wl_buffer *handle = w->fb.buf[w->fb.current].handle;
    if (w->canvas) {
        Buffer wrapper{w->fb.buf[w->fb.current].data, w->fb.buf[w->fb.current].width, w->fb.buf[w->fb.current].height };
        blitBuffer(&wrapper, w->canvas);
    }
    w->fb.buf[w->fb.current].held = true;
    w->fb.current = (w->fb.current + 1) % 2;
    if (w->fb.buf[w->fb.current].held) w->fb.current = -1;
    wl_surface_attach(w->wl.surface, handle, 0, 0);
    wl_surface_commit(w->wl.surface);
    xdg_surface_ack_configure(surface, serial);
    if (w->todo.was_resize) w->frame_events.resized = true;

    w->todo.was_resize = false;
}

static void wlBufferHandleRelease(void *data, wl_buffer *buffer) {
    WindowHandle *w = (WindowHandle*)data;
    i32 index = buffer == w->fb.buf[0].handle ? 0 : 1;
    w->fb.buf[index].held = false;
    // probably the right moment to resize it if it is to be resized
    if (w->fb.buf[index].width != w->size.x || w->fb.buf[index].height != w->size.y) {
        munmap(w->fb.buf[index].data, w->fb.buf[index].width * w->fb.buf[index].height * 4);
        wl_buffer_destroy(w->fb.buf[index].handle);
        allocateWindowBuffer(w, index);
    }
    if (w->fb.current == -1) w->fb.current = index;
    if (w->todo.ack_configure) {
        ackXdgSurfaceConfigure(w, w->todo.surface, w->todo.serial);
        w->todo.ack_configure = false;
    }
}

static void xdgSurfaceHandleConfigure(void *data, xdg_surface *surface, u32 serial) {
    // this is the final configure call, where we handle all accumulated state from the
    // toplevel configure calls
    WindowHandle *w = (WindowHandle*)data;
    for (u8 i = 0; i < 2; ++i) {
        if (!w->fb.buf[i].held && (w->fb.buf[i].width != w->size.x || w->fb.buf[i].height != w->size.y)) {
            munmap(w->fb.buf[i].data, w->fb.buf[i].width * w->fb.buf[i].height * 4);
            allocateWindowBuffer(w, i);
        }
    }
    if (w->fb.current == -1) {
        w->todo.ack_configure = true;
        w->todo.surface = surface;
        w->todo.serial = serial;
    } else { ackXdgSurfaceConfigure(w, surface, serial); }
}

static const struct xdg_surface_listener xdg_surface_listener = {
    .configure = xdgSurfaceHandleConfigure
};

static void xdgToplevelHandleConfigure(void *data, xdg_toplevel *toplevel, i32 width, i32 height, wl_array *states) {
    // receive the configure state to be resolved in xdgSurfaceHandleConfigure
    if (!width || !height) return;
    WindowHandle *w = (WindowHandle*)data;
    if (w->size.x != width || w->size.y != height) {
        w->size.x = width;
        w->size.y = height;
        w->todo.was_resize = true;
    }
}

static void xdgToplevelHandleClose(void *data, xdg_toplevel *toplevel) {
    ((WindowHandle*)data)->should_close = true;
}

static void xdgToplevelHandleConfigureBounds(void *data, xdg_toplevel *toplevel, i32 width, i32 height) { }
static void xdgToplevelHandleWmCapabilities(void *data, xdg_toplevel *toplevel, wl_array *capabilities) { }

static const struct xdg_toplevel_listener xdg_toplevel_listener = {
    .configure = xdgToplevelHandleConfigure,
    .close = xdgToplevelHandleClose,
    .configure_bounds = xdgToplevelHandleConfigureBounds,
    .wm_capabilities = xdgToplevelHandleWmCapabilities
};

static void wlPointerHandleEnter(void *data, wl_pointer *pointer, u32 serial, wl_surface *surface, wl_fixed_t x, wl_fixed_t y) {
    WindowHandle *w = (WindowHandle*)data;
    Vec2<f64> new_pos = {wl_fixed_to_double(x), wl_fixed_to_double(y)};
    w->input.pointer_delta = new_pos - w->input.pointer;
    w->input.pointer = new_pos;
}

static void wlPointerHandleLeave(void *data, wl_pointer *pointer, u32 serial, wl_surface *surface) { }
static void wlPointerHandleMotion(void *data, wl_pointer *pointer, u32 time, wl_fixed_t x, wl_fixed_t y) {
    WindowHandle *w = (WindowHandle*)data;
    Vec2<f64> new_pos = {wl_fixed_to_double(x), wl_fixed_to_double(y)};
    w->input.pointer_delta = new_pos - w->input.pointer;
    w->input.pointer = new_pos;
}

static void wlPointerHandleButton(void *data, wl_pointer *pointer, u32 serial, u32 time, u32 button, u32 state) {
    WindowHandle *w = (WindowHandle*)data;
    w->input.buttons[button] = state == WL_POINTER_BUTTON_STATE_PRESSED;
}

static void wlPointerHandleAxis(void *data, wl_pointer *pointer, u32 time, u32 axis, wl_fixed_t value) {
    //std::cout << "axis event: axis " << axis << " value: " << wl_fixed_to_double(value) << std::endl;
    WindowHandle *w = (WindowHandle*)data;
    if (axis == 0) {
        w->frame_events.axis.y = wl_fixed_to_double(value);
    } else {
        w->frame_events.axis.x = wl_fixed_to_double(value);
    }
}

static void wlPointerHandleAxisSource(void *data, wl_pointer *pointer, u32 axis_source) { }
static void wlPointerHandleAxisStop(void *data, wl_pointer *pointer, u32 time, u32 axis) { }
static void wlPointerHandleAxisDiscrete(void *data, wl_pointer *pointer, u32 axis, i32 discrete) { }
static void wlPointerHandleFrame(void *data, wl_pointer *pointer) {}
static void wlPointerHandleAxisValue120(void *data, wl_pointer *pointer, u32 axis, i32 value120) {}
static void wlPointerHandleAxisRelativeDirection(void *data, wl_pointer *pointer, u32 axis, u32 direction) {}

static const struct wl_pointer_listener wl_pointer_listener = {
    .enter = wlPointerHandleEnter,
    .leave = wlPointerHandleLeave,
    .motion = wlPointerHandleMotion,
    .button = wlPointerHandleButton,
    .axis = wlPointerHandleAxis,
    .frame = wlPointerHandleFrame,
    .axis_source = wlPointerHandleAxisSource,
    .axis_stop = wlPointerHandleAxisStop,
    .axis_discrete = wlPointerHandleAxisDiscrete,
	.axis_value120 = wlPointerHandleAxisValue120,
	.axis_relative_direction = wlPointerHandleAxisRelativeDirection,
};



static void  wlSeatHandleCapabilities(void *data, wl_seat *seat, u32 capabilities) {
    WindowHandle *w = (WindowHandle*)data;
    bool have_pointer = capabilities & WL_SEAT_CAPABILITY_POINTER;
    if (have_pointer && w->wl.pointer == nullptr) {
        w->wl.pointer = wl_seat_get_pointer(w->wl.seat);
        wl_pointer_add_listener(w->wl.pointer, &wl_pointer_listener, w);
    } else if (!have_pointer && w->wl.pointer != nullptr) {
        wl_pointer_release(w->wl.pointer);
        w->wl.pointer = nullptr;
    }
}

static void wlSeatHandleName(void *data, wl_seat *seat, const char *name) {
    // who cares...
}

static const struct wl_seat_listener wl_seat_listener = {
    .capabilities = wlSeatHandleCapabilities,
    .name = wlSeatHandleName
};

static void frameHandleDone(void *data, wl_callback *cb, u32 callback_data);
const struct wl_callback_listener frame_listener = {
    .done = frameHandleDone,
};

static void frameHandleDone(void *data, wl_callback *cb, u32 callback_data) {
    WindowHandle *w = (WindowHandle*)data;
    if (w->fb.current != -1) {
        if (w->canvas) {
            Buffer wrapper{
                w->fb.buf[w->fb.current].data, 
                w->fb.buf[w->fb.current].width, 
                w->fb.buf[w->fb.current].height 
            };
            blitBuffer(&wrapper, w->canvas);
        }

        wl_buffer *handle = w->fb.buf[w->fb.current].handle;
        wl_surface_attach(w->wl.surface, handle, 0, 0);
        wl_surface_damage_buffer(w->wl.surface, 0, 0, INT32_MAX, INT32_MAX);
        wl_surface_commit(w->wl.surface);
        w->fb.buf[w->fb.current].held = true;
        w->fb.current = (w->fb.current + 1) % 2;
        if (w->fb.buf[w->fb.current].held) w->fb.current = -1;
    }
    wl_callback_destroy(cb);
    cb = wl_surface_frame(w->wl.surface);
    wl_callback_add_listener(cb, &frame_listener, w);
}

void destroyWindowHandle(WindowHandle *w) {
    wl_shm_destroy(w->wl.shm);
    wl_display_disconnect(w->wl.display);
    delete w;
}

Window::Window(i32 width, i32 height, const char *title) {
    WindowHandle *w = new WindowHandle;
    w->size.x = width;
    w->size.y = height;
    w->should_close = false;
    w->todo.ack_configure = false;
    w->wl.compositor = nullptr;
    w->wl.shm = nullptr;
    w->wl.wm_base = nullptr;
    w->wl.seat = nullptr;
    w->wl.pointer = nullptr;
    w->canvas = nullptr;
    w->frame_events.resized = false;
    w->input.pointer = {0, 0};
    w->input.pointer_delta = {0, 0};

    for (i32 i = 0; i < MAX_SCANCODES; ++i) {
        w->input.buttons[i] = 0;
    }

    w->wl.display = wl_display_connect(nullptr);
    wl_registry *registry = wl_display_get_registry(w->wl.display);
    wl_registry_add_listener(registry, &wl_registry_listener, w);
    wl_display_roundtrip(w->wl.display);

    if (!w->wl.compositor || !w->wl.shm || !w->wl.seat || !w->wl.wm_base || w->should_close == true) {
        std::cerr << "error: required interface versions unmatched by your wayland compositor.\n";
        std::cerr << "       failed to open window...\n";
        wl_display_disconnect(w->wl.display);
        handle = nullptr;
    }

    xdg_wm_base_add_listener(w->wl.wm_base, &xdg_wm_base_listener, w);
    wl_seat_add_listener(w->wl.seat, &wl_seat_listener, w);
    w->wl.surface = wl_compositor_create_surface(w->wl.compositor);
    xdg_surface *surface = xdg_wm_base_get_xdg_surface(w->wl.wm_base, w->wl.surface);
    xdg_surface_add_listener(surface, &xdg_surface_listener, w);
    xdg_toplevel *toplevel = xdg_surface_get_toplevel(surface);
    xdg_toplevel_set_title(toplevel, title);
    xdg_toplevel_add_listener(toplevel, &xdg_toplevel_listener, w);

    if (!allocateWindowBuffer(w, 0)) {
        destroyWindowHandle(w);
        handle = nullptr;
    }
    if (!allocateWindowBuffer(w, 1)) {
        destroyWindowHandle(w);
        handle = nullptr;
    }

    w->fb.current = 0;
    wl_surface_commit(w->wl.surface);
    wl_callback *cb = wl_surface_frame(w->wl.surface);
    wl_callback_add_listener(cb, &frame_listener, w);
    handle = w;
}

Window::~Window() { destroyWindowHandle((WindowHandle *)handle); }

Vec2<i32> Window::size() {
    WindowHandle *w = (WindowHandle *)handle;
    return w->size;
}

bool Window::openedSuccesfully() {
    return handle != nullptr;
} 

void Window::update() {
    WindowHandle *w = (WindowHandle *)handle;
    w->frame_events.resized = false;
    w->frame_events.axis = {0, 0};
    w->input.pointer_delta.x = 0;
    w->input.pointer_delta.y = 0;
    for (i32 i = 0; i < MAX_SCANCODES; ++i) {
        if (w->input.buttons[i] == 1) {
            w->input.buttons[i] = 2;
        }
    }

    wl_display_dispatch(w->wl.display);
}

bool Window::shouldClose() {
    return ((WindowHandle *)handle)->should_close;
} 

bool Window::wasResized() {
    return ((WindowHandle *)handle)->frame_events.resized;
}

bool Window::buttonPressed(Scancode button) {
    return ((WindowHandle *)handle)->input.buttons[button] == 1;
}

bool Window::buttonHeld(Scancode button) {
    return ((WindowHandle *)handle)->input.buttons[button] >= 1;
}

Vec2<f64> Window::mousePosition() {
    return ((WindowHandle *)handle)->input.pointer;
}

Vec2<f64> Window::mousePositionDelta() {
    return ((WindowHandle *)handle)->input.pointer_delta;
}

Vec2<f64> Window::scrollVector() {
    return ((WindowHandle *)handle)->frame_events.axis;
}

void Window::setCanvas(Buffer *canvas) {
    ((WindowHandle *)handle)->canvas = canvas;
}
