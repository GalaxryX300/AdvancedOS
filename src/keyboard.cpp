#include "keyboard.h"
#include "io.h"
#include <cstdio>

// 键盘缓冲区
static char keyboard_buffer[KEYBOARD_BUFFER_SIZE];
static int buffer_start = 0;
static int buffer_end = 0;

// 按键状态
bool shift_pressed = false;
bool ctrl_pressed = false;
bool alt_pressed = false;
bool caps_lock = false;
bool num_lock = false;
bool scroll_lock = false;

void init_keyboard() {
	// 初始化键盘
	buffer_start = 0;
	buffer_end = 0;
	shift_pressed = false;
	ctrl_pressed = false;
	alt_pressed = false;
	caps_lock = false;
	num_lock = false;
	scroll_lock = false;
}

void keyboard_handler() {
	char c = getchar();
	if (c) {
		int next = (buffer_end + 1) % KEYBOARD_BUFFER_SIZE;
		if (next != buffer_start) {
			keyboard_buffer[buffer_end] = c;
			buffer_end = next;
		}
	}
}

char keyboard_getchar() {
	if (buffer_start == buffer_end) {
		return getchar();  // 临时使用标准输入
	}
	
	char c = keyboard_buffer[buffer_start];
	buffer_start = (buffer_start + 1) % KEYBOARD_BUFFER_SIZE;
	return c;
}

void keyboard_wait() {
}

void keyboard_enable() {
}

void keyboard_disable() {
}
