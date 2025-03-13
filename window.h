#ifndef WINDOW_H
#define WINDOW_H

#include <cstdint>
#include <cstddef>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef size_t usize;
typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;
typedef ptrdiff_t isize;
typedef float f32;
typedef double f64;

struct Color { float r, g, b; };

inline constexpr Color getColor(u32 color) {
    return { (float)((color & 0xff0000) >> 16) / 255.0f, (float)((color & 0x00ff00) >> 8) / 255.0f, (float)((color & 0x0000ff) >> 0) / 255.0f };
}

inline constexpr u32 getColorHex(Color color) {
    return ( ((0xffu) << 24) | ((u32)(color.r * 255.0) << 16) | ((u32)(color.g * 255.0) << 8) | ((u32)(color.b * 255.0) << 0) );
}

static constexpr Color WHITE = {1.0, 1.0, 1.0};
static constexpr Color BLACK = {0.0, 0.0, 0.0};

enum Scancode { 
    KEYBOARD_A = 30, KEYBOARD_B = 48, KEYBOARD_C = 46, KEYBOARD_D = 32, KEYBOARD_E = 18, KEYBOARD_F = 33,
    KEYBOARD_G = 34, KEYBOARD_H = 35, KEYBOARD_I = 23, KEYBOARD_J = 36, KEYBOARD_K = 37, KEYBOARD_L = 38,
    KEYBOARD_M = 50, KEYBOARD_N = 49, KEYBOARD_O = 24, KEYBOARD_P = 25, KEYBOARD_Q = 16, KEYBOARD_R = 19,
    KEYBOARD_S = 31, KEYBOARD_T = 20, KEYBOARD_U = 22, KEYBOARD_V = 47, KEYBOARD_W = 17, KEYBOARD_X = 45,
    KEYBOARD_Y = 21, KEYBOARD_Z = 44, KEYBOARD_1 = 2, KEYBOARD_2 = 3, KEYBOARD_3 = 4, KEYBOARD_4 = 5,
    KEYBOARD_5 = 6, KEYBOARD_6 = 7, KEYBOARD_7 = 8, KEYBOARD_8 = 9, KEYBOARD_9 = 10, KEYBOARD_0 = 11,
    KEYBOARD_ENTER = 28, KEYBOARD_ESCAPE = 1, KEYBOARD_BACKSPACE = 14, KEYBOARD_TAB = 15, KEYBOARD_SPACE = 57,
    KEYBOARD_MINUS = 12, KEYBOARD_EQUALS = 13, KEYBOARD_LEFT_BRACKET = 26, KEYBOARD_RIGHT_BRACKET = 27,
    KEYBOARD_BACKSLASH = 43, KEYBOARD_SEMICOLON = 39, KEYBOARD_APOSTROPHE = 40, KEYBOARD_GRAVE = 41,
    KEYBOARD_COMMA = 51, KEYBOARD_PERIOD = 52, KEYBOARD_SLASH = 53, KEYBOARD_F1 = 59, KEYBOARD_F2 = 60,
    KEYBOARD_F3 = 61, KEYBOARD_F4 = 62, KEYBOARD_F5 = 63, KEYBOARD_F6 = 64, KEYBOARD_F7 = 65, KEYBOARD_F8 = 66,
    KEYBOARD_F9 = 67, KEYBOARD_F10 = 68, KEYBOARD_F11 = 87, KEYBOARD_F12 = 88, KEYBOARD_LEFT_CTRL = 29,
    KEYBOARD_LEFT_SHIFT = 42, KEYBOARD_LEFT_ALT = 56, KEYBOARD_LEFT_META = 125, KEYBOARD_RIGHT_CTRL = 97,
    KEYBOARD_RIGHT_SHIFT = 54, KEYBOARD_RIGHT_ALT = 100, KEYBOARD_RIGHT_META = 126, KEYBOARD_INSERT = 110,
    KEYBOARD_DELETE = 111, KEYBOARD_HOME = 102, KEYBOARD_END = 107, KEYBOARD_PAGE_UP = 104,
    KEYBOARD_PAGE_DOWN = 109, KEYBOARD_ARROW_UP = 103, KEYBOARD_ARROW_DOWN = 108, KEYBOARD_ARROW_LEFT = 105,
    KEYBOARD_ARROW_RIGHT = 106, NUMPAD_0 = 82, NUMPAD_1 = 79, NUMPAD_2 = 80, NUMPAD_3 = 81, NUMPAD_4 = 75,
    NUMPAD_5 = 76, NUMPAD_6 = 77, NUMPAD_7 = 71, NUMPAD_8 = 72, NUMPAD_9 = 73, NUMPAD_DECIMAL = 83,
    NUMPAD_ENTER = 96, NUMPAD_ADD = 78, NUMPAD_SUBTRACT = 74, NUMPAD_MULTIPLY = 55, NUMPAD_DIVIDE = 98,
    KEY_CAPS_LOCK = 58, KEY_NUM_LOCK = 69, KEY_SCROLL_LOCK = 70, MEDIA_PLAY_PAUSE = 164, MEDIA_STOP = 166,
    MEDIA_PREVIOUS_TRACK = 165, MEDIA_NEXT_TRACK = 163, MEDIA_VOLUME_UP = 115, MEDIA_VOLUME_DOWN = 114,
    MEDIA_MUTE = 113, KEY_POWER = 116, KEY_SLEEP = 142, KEY_WAKE = 143,
    MOUSE_BUTTON_LEFT = 0x110,           // BTN_LEFT
    MOUSE_BUTTON_RIGHT = 0x111,          // BTN_RIGHT
    MOUSE_BUTTON_MIDDLE = 0x112,         // BTN_MIDDLE
    MOUSE_BUTTON_SIDE_BUTTON = 0x113,    // BTN_SIDE (extra button 1, often "back")
    MOUSE_BUTTON_EXTRA_BUTTON = 0x114,   // BTN_EXTRA (extra button 2, often "forward")
    MOUSE_BUTTON_FORWARD = 0x115,        // BTN_FORWARD
    MOUSE_BUTTON_BACK = 0x116,           // BTN_BACK
    MOUSE_BUTTON_TASK = 0x117,           // BTN_TASK (rare, used in some mice with extra buttons)
    MAX_SCANCODES
};

struct Buffer { u32 *data;  i32 width;  i32 height; };
inline Buffer *initBuffer(i32 width, i32 height) {
    Buffer *b = new Buffer;
    b->data = new u32[width * height];
    b->width = width;
    b->height = height;
    return b;
}
inline void freeBuffer(Buffer *b) { delete[] b->data; }
inline constexpr u32 getBufferSize(const Buffer *buf) {
    return buf->height * buf->width * sizeof(u32);
}
inline void fillPixel(Buffer *buf, u32 x, u32 y, Color color) {
    buf->data[y * buf->width + x] = getColorHex(color);
}

void fillBuffer(Buffer *buf, Color color);
void zoomBufferInterpolate(Buffer *b, i32 focus_x, i32 focus_y, f32 zoom);
void blurBufferGaussian(Buffer *buf, u8 kernel_size, f32 sigma);
void blitBuffer(Buffer *dest, Buffer *src);
void fillBuffer(Buffer *buf, Color color);
void zoomBufferInterpolate(Buffer *b, i32 focus_x, i32 focus_y, f32 zoom);
void blurBufferGaussian(Buffer *buf, u8 kernel_size, f32 sigma);

// una window potrebbe avere modes: tipo opengl e scegli versione, vulkan, canvas
class Window {
public:
    Window(i32 width, i32 height, const char *title);
    ~Window();
    void setCanvas(Buffer *canvas);
    Vec2<i32> size();
    bool openedSuccesfully(); // the window opened 
    void update(); // call in your loop to update the window
    bool shouldClose(); 
    bool wasResized();

    bool buttonPressed(Scancode button);
    bool buttonHeld(Scancode button);
    Vec2<f64> mousePosition();
    Vec2<f64> mousePositionDelta();
    Vec2<f64> scrollVector();


private:
    void *handle;
    bool opened ;
};

#endif // WINDOW_H
