#include "screen.h"
#include <cstdio>
#include <cstdarg>
#include <cstring>

static int cursor_x = 0;
static int cursor_y = 0;
static uint8_t current_color = COLOR_WHITE;

void init_screen() {
	clear_screen();
	update_cursor();
}

void clear_screen() {
	cursor_x = 0;
	cursor_y = 0;
	printf("\033[2J\033[H"); // 清屏并将光标移到左上角
}

void set_color(uint8_t foreground, uint8_t background) {
	current_color = (background << 4) | (foreground & 0x0F);
}

void putchar(char c) {
	if (c == '\n') {
		cursor_x = 0;
		cursor_y++;
		if (cursor_y >= VGA_HEIGHT) {
			scroll_screen();
		}
		printf("\n");
		return;
	}
	
	if (c == '\r') {
		cursor_x = 0;
		return;
	}
	
	if (c == '\b') {
		if (cursor_x > 0) {
			cursor_x--;
			printf("\b \b");
		}
		return;
	}
	
	printf("%c", c);
	cursor_x++;
	if (cursor_x >= VGA_WIDTH) {
		cursor_x = 0;
		cursor_y++;
		if (cursor_y >= VGA_HEIGHT) {
			scroll_screen();
		}
	}
}

void put_string(const char* str) {
	while (*str) {
		putchar(*str++);
	}
}

void put_hex(uint32_t num) {
	char buf[9];
	snprintf(buf, sizeof(buf), "%08x", num);
	put_string(buf);
}

void put_dec(uint32_t num) {
	char buf[11];
	snprintf(buf, sizeof(buf), "%u", num);
	put_string(buf);
}

void update_cursor() {
	printf("\033[%d;%dH", cursor_y + 1, cursor_x + 1);
}

void scroll_screen() {
	if (cursor_y >= VGA_HEIGHT) {
		cursor_y = VGA_HEIGHT - 1;
		printf("\033[S"); // 向上滚动一行
	}
}

void kprintf(const char* format, ...) {
	char buffer[1024];
	va_list args;
	va_start(args, format);
	vsnprintf(buffer, sizeof(buffer), format, args);
	va_end(args);
	put_string(buffer);
}
