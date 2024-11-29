// interrupt.h
#ifndef INTERRUPT_H
#define INTERRUPT_H

#include "types.h"

// IDT表项结构
struct idt_entry {
    uint16_t base_low;
    uint16_t selector;
    uint8_t  zero;
    uint8_t  flags;
    uint16_t base_high;
} __attribute__((packed));

// IDT指针结构
struct idt_ptr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

// 中断处理函数类型
typedef void (*interrupt_handler_t)(void);

// 函数声明
void init_idt();
void set_interrupt_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags);
void register_interrupt_handler(uint8_t num, interrupt_handler_t handler);

// 中断服务程序
extern "C" {
    void isr0();
    void isr1();
    // ...更多ISR声明...
    void isr31();

    // IRQ处理程序
    void irq0();
    void irq1();
    // ...更多IRQ声明...
    void irq15();
}

#endif // INTERRUPT_H
// interrupt.h
#ifndef INTERRUPT_H
#define INTERRUPT_H

#include "types.h"

// IDT表项结构
struct idt_entry {
    uint16_t base_low;
    uint16_t selector;
    uint8_t  zero;
    uint8_t  flags;
    uint16_t base_high;
} __attribute__((packed));

// IDT指针结构
struct idt_ptr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

// 中断处理函数类型
typedef void (*interrupt_handler_t)(void);

// 函数声明
void init_idt();
void set_interrupt_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags);
void register_interrupt_handler(uint8_t num, interrupt_handler_t handler);

// 中断服务程序
extern "C" {
    void isr0();
    void isr1();
    // ...更多ISR声明...
    void isr31();

    // IRQ处理程序
    void irq0();
    void irq1();
    // ...更多IRQ声明...
    void irq15();
}

#endif // INTERRUPT_H
