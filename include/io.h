// include/io.h
#ifndef IO_H
#define IO_H

#include "types.h"

inline void outb(uint16_t port, uint8_t val) {
    asm volatile ("outb %0, %1" :: "a"(val), "Nd"(port));
}

inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

inline void io_wait() {
    outb(0x80, 0);
}

#endif // IO_H