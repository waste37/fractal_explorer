#include <mandelbrot.h>
#include <window.h>
#include <iostream>

#include <algorithm>
#include <cstring>

// i thread vengono accesi
// viene generato lavoro e 

FractalExplorer::FractalExplorer(Window *w) {
    computePixel = &FractalExplorer::computeMandlebrotPixel;
    window = w;
    initialize();
}

FractalExplorer::FractalExplorer(Window *w, Vec2<f64> julia_param) {
    c = julia_param;
    computePixel = &FractalExplorer::computeJuliaPixel;
    window = w;
    initialize();
}

void FractalExplorer::initialize() {
    Vec2<i32> size = window->size();
    canvas = initBuffer(size.x, size.y); 
    fillBuffer(canvas, BLACK);

    if (size.x < size.y) {
        fractal_size.x = 4.0;
        fractal_size.y = 4.0 / size.x * size.y;
    } else {
        fractal_size.y = 4.0;
        fractal_size.x = 4.0 / size.y * size.x;
    }

    offset = {-2, -2};
    zoom_level = 1.0;

    Vec2<f64> center = {(double)canvas->width/2.0, (double)canvas->height/2.0};
    center = screenToFractal(center);

    window->setCanvas(canvas);
    threads.resize(N_THREAD);

    for (i32 i = 0; i < N_THREAD; ++i) {
        threads[i] = std::thread(&FractalExplorer::getWorkUnit, this);
    }

    generateFullWorkUnits();
    startDrawing();
}

FractalExplorer::~FractalExplorer() {
    if (finished > 0) stop_drawing = true;
    alive = false;
    work_available = true;
    waiting_condition.notify_all();
    for (auto &th : threads) { th.join(); }
    freeBuffer(canvas);
}

Buffer *FractalExplorer::getCanvas() const {
    return canvas;
}

void FractalExplorer::resizeCanvas(Vec2<i32> size) {
    stopDrawing();
    Buffer *newcanvas = initBuffer(size.x, size.y);

    for (i32 y = 0; y < size.y; ++y) {
        for (i32 x = 0; x < size.x; ++x) {
            if (x < canvas->width && y < canvas->height) {
                newcanvas->data[y * size.x + x] = canvas->data[y * canvas->width + x];
            } else {
                newcanvas->data[y * size.x + x] = 0;
            }
        }
    }

    window->setCanvas(newcanvas);
    freeBuffer(canvas);
    canvas = newcanvas;

    //Vec2<f64> new_fractal_size = screenToFractal({(f64)size.x, (f64)size.y});
    //Vec2<f64> delta_size = new_fractal_size - fractal_size;
    //fractal_size = fractal_size + delta_size;

    if (size.x < size.y) {
        fractal_size.x = 4.0 / zoom_level;
        fractal_size.y = (4.0 / size.x * size.y) / zoom_level;
    } else {
        fractal_size.y = (4.0) / zoom_level;
        fractal_size.x = (4.0 / size.y * size.x) / zoom_level;
    }
    generateFullWorkUnits();
    startDrawing();
}
void FractalExplorer::pan(Vec2<f64> delta) {
    if (length(delta) == 0) return;
    stopDrawing();

    Buffer *work = initBuffer(canvas->width, canvas->height);
    for (i32 y = 0; y < canvas->height; ++y) {
        for (i32 x = 0; x < canvas->width; ++x) {
            i32 canvas_index = y * canvas->width + x;
            i32 work_index = max(0, min(
                (i32)(y + floor(delta.y)) * canvas->width + (i32)(x + floor(delta.x)), 
                work->width * work->height
            ));
            work->data[work_index] = canvas->data[canvas_index];
        }
    }

    std::memcpy(canvas->data, work->data, sizeof(u32) * canvas->height * canvas->width);

    delta.x = delta.x * (fractal_size.x / canvas->width);
    delta.y = -1 * delta.y * (fractal_size.y / canvas->height);
    offset = offset - delta;

    generateFullWorkUnits();
    startDrawing();
}
void FractalExplorer::zoom(Vec2<f64> focus, f64 amount) {
    stopDrawing();
    Vec2<f64> focus_drawing = screenToFractal(focus);
    zoom_level *= amount;
    fractal_size /= amount;
    Vec2<f64> new_focus_drawing = screenToFractal(focus);
    offset = offset + (focus_drawing - new_focus_drawing);
    if (amount > 1.0) zoomBufferInterpolate(canvas, focus.x, focus.y, amount);
    // regenerate work units..
    generateFullWorkUnits();
    startDrawing();
}

void FractalExplorer::stopDrawing() {
    stop_drawing = true;
    work_unit_lock.lock();
    work_units.clear();
    work_unit_lock.unlock();
    work_available_lock.lock();
    work_available = false;
    work_available_lock.unlock();
    std::unique_lock<std::mutex> lock(finished_lock);
    all_stopped_condition.wait(lock, [&finished = finished] {  return finished <= 0; });
}

// notify threads that it is time to start drawing again
void FractalExplorer::startDrawing() {
    stop_drawing = false;
    std::unique_lock<std::mutex> lock(work_available_lock);
    work_available = true;
    waiting_condition.notify_all();
}

void FractalExplorer::generateFullWorkUnits() {
    i32 step = max(50, min(canvas->width, canvas->height) / 10);

    for (i32 y = 0; y < canvas->height; y += step) {
        for (i32 x = 0; x < canvas->width; x += step) {
            work_units.push_back({
                x, min(x + step, canvas->width), y, min(y + step, canvas->height)
            });
        }
    }
}

void FractalExplorer::doWorkUnit(WorkUnit w) {
    i32 draw_width = w.max_x - w.min_x; 
    i32 draw_height = w.max_y - w.min_y;
    Buffer *work_buffer = initBuffer(draw_width, draw_height);
    for (i32 y = 0; y < draw_height; ++y) {
        for (i32 x = 0; x < draw_width; ++x) {
            Color color = (*this.*computePixel)(x + w.min_x, y + w.min_y);
            fillPixel(work_buffer , x, y, color);
        }
    }

    for (i32 y = 0; y < draw_height; ++y) {
        if (stop_drawing) break;
        for (i32 x = 0; x < draw_width; ++x) {
            if (stop_drawing) break;
            u32 hex = work_buffer->data[y*draw_width + x];
            canvas->data[x+w.min_x + (y+w.min_y)*canvas->width] = hex;
        }
    }

    freeBuffer(work_buffer);
}

void FractalExplorer::getWorkUnit() {
    WorkUnit w;
    while (alive) {
        work_available_lock.lock();
        ++finished;
        work_available_lock.unlock();
        while (!stop_drawing) {
            work_unit_lock.lock();
            if (!work_units.empty()) {
                w = work_units.back();
                work_units.pop_back();
                work_unit_lock.unlock();
                doWorkUnit(w);
            } else {
                work_unit_lock.unlock();
                std::unique_lock<std::mutex> lock(work_available_lock);
                work_available = false;
                break;
            }
        }
        work_available_lock.lock();
        if (--finished <= 0) {
            finished_lock.lock();
            all_stopped_condition.notify_one();
            finished_lock.unlock();
        }
        work_available_lock.unlock();
        std::unique_lock<std::mutex> lock(work_available_lock);
        waiting_condition.wait(lock, [&work_available = work_available] { return work_available; });
    }
}

Color (FractalExplorer::*computePixel)(i32, i32);

Color FractalExplorer::computeMandlebrotPixel(i32 px, i32 py) {
    Vec2<f64> v0{(f64)px, (f64)py};
    v0 = screenToFractal(v0);
    Vec2<f64> v{0, 0};
    f64 x2 = 0, y2 = 0;
    f64 iteration = 0;

    while (x2 + y2 <= (1 << 16) && iteration < max_iterations) {
        v.y = (v.x + v.x) * v.y + v0.y;
        v.x = x2 - y2 + v0.x;
        x2 = sqr(v.x);
        y2 = sqr(v.y);
        iteration += 1;
    }

    Color c;
    if (iteration < max_iterations) {
        f64 log_zn = log(v.x*v.x + v.y*v.y) / 2;
        f64 nu = log(log_zn / log(2)) / log(2);
        iteration = iteration + 1 - nu;
        Color c2 = getPaletteColor(floor(iteration));
        Color c1 = getPaletteColor(floor(iteration) + 1);
        float frac = iteration - floor(iteration);
        c = {
            frac * c1.r + (1 - frac) * c2.r, 
            frac * c1.g + (1 - frac) * c2.g, 
            frac * c1.b + (1 - frac) * c2.b
        };
    } else c = {0, 0, 0};
    return c;
}

Color FractalExplorer::computeJuliaPixel(i32 px, i32 py) {
    Vec2<f64> z{(f64)px, (f64)py};
    z = screenToFractal(z);
    static constexpr double R = 100;
    f64 iteration = 0;

    while (z.x * z.x + z.y * z.y < R * R && iteration < max_iterations) {
        f64 xtemp = z.x * z.x - z.y * z.y;
        z.y = 2.0 * z.x * z.y + c.y; 
        z.x = xtemp + c.x; 
        iteration += 1;
    }

    Color color;
    if (iteration < max_iterations) {
        f64 log_zn = log(z.x*z.x + z.y*z.y) / 2;
        f64 nu = log(log_zn / log(2)) / log(2);
        iteration = iteration + 1 - nu;
        Color c2 = getPaletteColor(floor(iteration));
        Color c1 = getPaletteColor(floor(iteration) + 1);
        float frac = iteration - floor(iteration);
        color = {
            frac * c1.r + (1 - frac) * c2.r, 
            frac * c1.g + (1 - frac) * c2.g, 
            frac * c1.b + (1 - frac) * c2.b
        };
    } else { 
        color = {0, 0, 0};
    }
    return color;
}
