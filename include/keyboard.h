#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "types.h"

// 键盘命令端口
#define KEYBOARD_CMD_PORT 0x64
#define KEYBOARD_DATA_PORT 0x60

// 键盘命令
#define KEYBOARD_CMD_LED 0xED
#define KEYBOARD_CMD_ENABLE 0xF4
#define KEYBOARD_CMD_RESET 0xFF

// 缓冲区大小
#define KEYBOARD_BUFFER_SIZE 128

// 按键状态标志
extern bool shift_pressed;
extern bool ctrl_pressed;
extern bool alt_pressed;
extern bool caps_lock;
extern bool num_lock;
extern bool scroll_lock;

// 键盘初始化和控制函数
void init_keyboard();
void keyboard_handler();
char keyboard_getchar();
void keyboard_wait();
void keyboard_enable();
void keyboard_disable();

#endif // KEYBOARD_H