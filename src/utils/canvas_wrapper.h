// src/utils/canvas_wrapper.h
#ifndef UTILS_CANVAS_WRAPPER_H
#define UTILS_CANVAS_WRAPPER_H

#include <cstdint>
#include <cstddef>

class Canvas {
public:
    Canvas(int16_t w, int16_t h);
    ~Canvas();

    Canvas(const Canvas&) = delete;
    Canvas& operator=(const Canvas&) = delete;

    int16_t width() const { return w_; }
    int16_t height() const { return h_; }

    void fill_screen(uint16_t color);
    void fill_circle(int16_t cx, int16_t cy, int16_t r, uint16_t color);
    void draw_circle(int16_t cx, int16_t cy, int16_t r, uint16_t color);
    void fill_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
    void draw_line(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);
    void draw_pixel(int16_t x, int16_t y, uint16_t color);
    uint16_t get_pixel(int16_t x, int16_t y) const;

    uint16_t* get_buffer();
    size_t buffer_size_bytes() const;

private:
    int16_t w_;
    int16_t h_;
    uint16_t* buffer_ = nullptr;
};

#endif // UTILS_CANVAS_WRAPPER_H
