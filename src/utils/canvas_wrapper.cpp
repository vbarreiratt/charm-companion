// src/utils/canvas_wrapper.cpp
#include "utils/canvas_wrapper.h"

#if defined(ARDUINO)
#include <esp_heap_caps.h>
#endif

Canvas::Canvas(int16_t w, int16_t h) : w_(w), h_(h) {
#if defined(ARDUINO)
    buffer_ = static_cast<uint16_t*>(heap_caps_malloc(
        static_cast<size_t>(w_) * h_ * sizeof(uint16_t), MALLOC_CAP_SPIRAM));
#else
    buffer_ = new uint16_t[static_cast<size_t>(w_) * h_];
#endif
    fill_screen(0);
}

Canvas::~Canvas() {
#if defined(ARDUINO)
    if (buffer_) heap_caps_free(buffer_);
#else
    delete[] buffer_;
#endif
}

void Canvas::fill_screen(uint16_t color) {
    size_t count = static_cast<size_t>(w_) * h_;
    for (size_t i = 0; i < count; ++i) buffer_[i] = color;
}

void Canvas::draw_pixel(int16_t x, int16_t y, uint16_t color) {
    if (x < 0 || y < 0 || x >= w_ || y >= h_) return;
    buffer_[static_cast<size_t>(y) * w_ + x] = color;
}

uint16_t Canvas::get_pixel(int16_t x, int16_t y) const {
    if (x < 0 || y < 0 || x >= w_ || y >= h_) return 0;
    return buffer_[static_cast<size_t>(y) * w_ + x];
}

void Canvas::fill_circle(int16_t cx, int16_t cy, int16_t r, uint16_t color) {
    int32_t r2 = static_cast<int32_t>(r) * r;
    for (int16_t dy = -r; dy <= r; ++dy) {
        for (int16_t dx = -r; dx <= r; ++dx) {
            if (static_cast<int32_t>(dx) * dx + static_cast<int32_t>(dy) * dy <= r2) {
                draw_pixel(cx + dx, cy + dy, color);
            }
        }
    }
}

void Canvas::draw_circle(int16_t cx, int16_t cy, int16_t r, uint16_t color) {
    int16_t x = r;
    int16_t y = 0;
    int16_t err = 1 - r;
    while (x >= y) {
        draw_pixel(cx + x, cy + y, color);
        draw_pixel(cx + y, cy + x, color);
        draw_pixel(cx - y, cy + x, color);
        draw_pixel(cx - x, cy + y, color);
        draw_pixel(cx - x, cy - y, color);
        draw_pixel(cx - y, cy - x, color);
        draw_pixel(cx + y, cy - x, color);
        draw_pixel(cx + x, cy - y, color);
        y += 1;
        if (err < 0) {
            err += 2 * y + 1;
        } else {
            x -= 1;
            err += 2 * (y - x) + 1;
        }
    }
}

void Canvas::fill_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    for (int16_t j = y; j < y + h; ++j) {
        for (int16_t i = x; i < x + w; ++i) {
            draw_pixel(i, j, color);
        }
    }
}

void Canvas::draw_line(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color) {
    int16_t dx = (x1 > x0) ? (x1 - x0) : (x0 - x1);
    int16_t dy = (y1 > y0) ? (y0 - y1) : (y1 - y0);  // negative magnitude
    int16_t sx = (x0 < x1) ? 1 : -1;
    int16_t sy = (y0 < y1) ? 1 : -1;
    int16_t err = dx + dy;
    while (true) {
        draw_pixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        int16_t e2 = 2 * err;
        if (e2 >= dy) {
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx) {
            err += dx;
            y0 += sy;
        }
    }
}

uint16_t* Canvas::get_buffer() { return buffer_; }

size_t Canvas::buffer_size_bytes() const {
    return static_cast<size_t>(w_) * h_ * sizeof(uint16_t);
}
