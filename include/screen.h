#ifndef SCREEN_H
#define SCREEN_H

#include "types.h"
#include <cstdarg>

#define VIDEO_MEMORY 0xB8000
#define VGA_WIDTH 80
#define VGA_HEIGHT 25

// 颜色定义
enum vga_color {
    COLOR_BLACK = 0,
    COLOR_BLUE = 1,
    COLOR_GREEN = 2,
    COLOR_CYAN = 3,
    COLOR_RED = 4,
    COLOR_MAGENTA = 5,
    COLOR_BROWN = 6,
    COLOR_LIGHT_GREY = 7,
    COLOR_DARK_GREY = 8,
    COLOR_LIGHT_BLUE = 9,
    COLOR_LIGHT_GREEN = 10,
    COLOR_LIGHT_CYAN = 11,
    COLOR_LIGHT_RED = 12,
    COLOR_LIGHT_MAGENTA = 13,
    COLOR_LIGHT_BROWN = 14,
    COLOR_WHITE = 15,
};

// 屏幕操作函数
void init_screen();
void clear_screen();
void set_color(uint8_t foreground, uint8_t background);
void putchar(char c);
void put_string(const char* str);
void put_hex(uint32_t num);
void put_dec(uint32_t num);
void update_cursor();
void scroll_screen();
void kprintf(const char* format, ...);

#endif // SCREEN_H