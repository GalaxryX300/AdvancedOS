// kernel_main.cpp
#include "kernel.h"
#include "interrupt.h"
#include "memory.h"
#include "process.h"

extern "C" void kernel_main() {
	// 初始化屏幕
	init_screen();
	kprintf("OS Kernel Starting...\n");
	
	// 初始化中断向量表
	init_idt();
	kprintf("Interrupt Descriptor Table initialized\n");
	
	// 初始化内存管理
	init_memory();
	kprintf("Memory management initialized\n");
	
	// 初始化基本进程管理
	init_processes();
	kprintf("Process management initialized\n");
	
	// 启用中断
	asm volatile("sti");
	kprintf("Interrupts enabled\n");
	
	// 创建初始进程
	create_initial_processes();
	
	// 进入进程调度循环
	while(1) {
		schedule_processes();
	}
}

void init_screen() {
	// 清理屏幕
	for(int i = 0; i < 80 * 25; i++) {
		((unsigned short*)0xB8000)[i] = 0x0720; // 空白字符，白色
	}
}

void init_memory() {
	// 初始化分页系统
	init_paging();
	
	// 初始化物理内存管理
	init_physical_memory();
	
	// 初始化堆内存管理
	init_heap();
}

void init_processes() {
	// 初始化进程表
	process_table_init();
	
	// 初始化调度器
	scheduler_init();
}

void create_initial_processes() {
	// 创建init进程
	Process* init = create_process("init");
	if(init) {
		init->state = PROCESS_READY;
		init->priority = 1;
	}
	
	// 创建idle进程
	Process* idle = create_process("idle");
	if(idle) {
		idle->state = PROCESS_READY;
		idle->priority = 0;
	}
}

void schedule_processes() {
	// 执行进程调度
	Process* next = get_next_process();
	if(next) {
		switch_to_process(next);
	}
}

// 中断处理相关
extern "C" void interrupt_handler(int interrupt_no, int error_code) {
	switch(interrupt_no) {
		case 0x20: // 时钟中断
		timer_tick();
		break;
		case 0x21: // 键盘中断
		handle_keyboard();
		break;
		case 0x0E: // 页错误
		handle_page_fault(error_code);
		break;
	default:
		if(interrupt_no < 32) {
			kprintf("Exception %d occurred!\n", interrupt_no);
			while(1); // 暂停系统
		}
		break;
	}
}
