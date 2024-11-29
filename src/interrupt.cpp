// interrupt.cpp
#include "interrupt.h"
#include "io.h"

// IDT表
static struct idt_entry idt[256];
static struct idt_ptr idtp;

// 中断处理函数表
static interrupt_handler_t interrupt_handlers[256];

void init_idt() {
	// 设置IDT指针
	idtp.limit = (sizeof(struct idt_entry) * 256) - 1;
	idtp.base = (uint32_t)&idt;
	
	// 清空IDT
	memset(&idt, 0, sizeof(struct idt_entry) * 256);
	
	// 重新映射PIC
	outb(0x20, 0x11);
	outb(0xA0, 0x11);
	outb(0x21, 0x20);    // 主PIC的起始中断号为0x20
	outb(0xA1, 0x28);    // 从PIC的起始中断号为0x28
	outb(0x21, 0x04);
	outb(0xA1, 0x02);
	outb(0x21, 0x01);
	outb(0xA1, 0x01);
	outb(0x21, 0x0);
	outb(0xA1, 0x0);
	
	// 设置异常处理程序
	set_interrupt_gate(0, (uint32_t)isr0, 0x08, 0x8E);
	set_interrupt_gate(1, (uint32_t)isr1, 0x08, 0x8E);
	// ...设置更多异常处理程序...
	
	// 设置IRQ处理程序
	set_interrupt_gate(32, (uint32_t)irq0, 0x08, 0x8E);
	set_interrupt_gate(33, (uint32_t)irq1, 0x08, 0x8E);
	// ...设置更多IRQ处理程序...
	
	// 加载IDT
	asm volatile("lidt %0" : : "m" (idtp));
	
	// 启用中断
	asm volatile("sti");
}

void set_interrupt_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags) {
	idt[num].base_low = base & 0xFFFF;
	idt[num].base_high = (base >> 16) & 0xFFFF;
	idt[num].selector = sel;
	idt[num].zero = 0;
	idt[num].flags = flags;
}

void register_interrupt_handler(uint8_t num, interrupt_handler_t handler) {
	interrupt_handlers[num] = handler;
}

// 中断处理程序
extern "C" void interrupt_handler(uint32_t int_no) {
	if(interrupt_handlers[int_no]) {
		interrupt_handlers[int_no]();
	} else {
		kprintf("Unhandled interrupt: %d\n", int_no);
	}
	
	// 如果是IRQ，需要发送EOI
	if(int_no >= 32) {
		if(int_no >= 40) {
			outb(0xA0, 0x20);    // 发送EOI到从PIC
		}
		outb(0x20, 0x20);        // 发送EOI到主PIC
	}
}

// 实现一些基本的中断处理程序
void timer_handler() {
	static uint32_t tick = 0;
	tick++;
	if(tick % 100 == 0) {    // 每100个时钟周期
		schedule();           // 调用进程调度器
	}
}

void keyboard_handler() {
	uint8_t scancode = inb(0x60);    // 读取扫描码
	// 处理键盘输入...
}

// 注册基本的中断处理程序
void init_interrupt_handlers() {
	register_interrupt_handler(32, timer_handler);     // 时钟中断
	register_interrupt_handler(33, keyboard_handler);  // 键盘中断
	register_interrupt_handler(14, page_fault_handler);// 页错误
}
