#ifndef mandelbrot_h
#define mandelbrot_h

#include <extramath.h>
#include <window.h>
#include <thread>
#include <atomic>
#include <semaphore>
#include <condition_variable>
#include <mutex>


//struct Window;
//enum EventType {
//    EVENT_POINTER,
//    EVENT_RESIZE,
//};
//
//enum PointerButtonCode {
//    BUTTON_LEFT = 272,
//    BUTTON_RIGHT = 273,
//    BUTTON_MIDDLE = 274,
//};
//
//enum PointerEventType {
//    POINTER_BUTTON_PRESSED,
//    POINTER_BUTTON_RELEASED,
//};
//
//struct Event {
//    EventType type;
//};
//
//struct PointerEvent : public Event {
////    EventType type; // EVENT_POINTER
//    PointerEventType p_type;
//    PointerButtonCode button;
//};
//
//struct ResizeEvent :public Event { 
//    EventType type;
//    Vec2<i32> new_size;
//};

//Window *openWindow(i32 width, i32 height, const char *title);
//void destroyWindow(Window *w);
//
//bool getWindowEvent(Window *w, Event **ev);
//void updateWindow(Window *w);
//bool windowShouldClose(Window *w);
//void windowSetCanvas(Window *w, Buffer *canvas);
//Vec2<double> getWindowPointerPosition(Window *w);
//Vec2<i32> getWindowSize(Window *w);


Color getPaletteColor(u32 i);

class FractalExplorer {
    static constexpr u8 N_THREAD = 16;
    static constexpr float ZOOM_FACTOR = 1.2;
public:
    FractalExplorer(Window *w);
    FractalExplorer(Window *w, Vec2<f64> julia_param);
    ~FractalExplorer();
    Buffer *getCanvas() const;
    void resizeCanvas(Vec2<i32> size);
    void pan(Vec2<f64> direction);
    void zoom(Vec2<f64> focus, f64 amount);
    void stopDrawing();
    inline Vec2<f64> screenToFractal(Vec2<f64> p) {
        p.x = p.x * (fractal_size.x / canvas->width) + offset.x;
        p.y = (canvas->height - p.y) * (fractal_size.y / canvas->height) + offset.y;
        return p;
    }
private:
    void initialize();
    void generateFullWorkUnits();
    struct WorkUnit { i32 min_x, max_x, min_y, max_y; };
    void startDrawing();
    void getWorkUnit();
    void doWorkUnit(WorkUnit w);
    Color (FractalExplorer::*computePixel)(i32, i32);
    Color computeMandlebrotPixel(i32 px, i32 py);
    Color computeJuliaPixel(i32 px, i32 py);
    Window *window;
    Buffer *canvas;
    u32 max_iterations = 1000;
    std::atomic<bool> stop_drawing = false;

    // {-0.835, -0.321}
    f64 zoom_level;
    Vec2<f64> c, offset, fractal_size;// = 1.15; //to see it all

    std::vector<WorkUnit> work_units;
    std::mutex work_unit_lock;
    std::vector<std::thread> threads;

    std::mutex finished_lock;
    i32 finished = 0;
    std::condition_variable all_stopped_condition;

    bool work_available = false;
    std::mutex work_available_lock;
    std::condition_variable waiting_condition;
    std::atomic<bool> alive = true;
    //std::mutex finished_lock;
};

bool writeBitmap(const char *filename, const Buffer buf);

#endif // mandelbrot_h
