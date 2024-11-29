// utils.cpp
#include "utils.h"
#include <cstdio>
#include <cstdlib>
#include <cstdarg>

void panic(const char* message) {
	fprintf(stderr, "Panic: %s\n", message);
	exit(1);
}

void kprintf(const char* format, ...) {
	va_list args;
	va_start(args, format);
	vprintf(format, args);
	va_end(args);
}
