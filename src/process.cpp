#include "process.h"
#include "memory.h"
#include <cstring>
#include <stdint.h>

static Process* process_table[MAX_PROCESSES];
static Process* current_process = nullptr;
static Process* ready_queue = nullptr;
static uint32_t next_pid = 0;

static inline unsigned long read_cr3(void) {
	unsigned long val;
	asm volatile("mov %%cr3, %%rax\n\t"
		"mov %%rax, %0"
		: "=r" (val)
		:
		: "%rax", "memory");
	return val;
}

static inline void write_cr3(unsigned long val) {
	asm volatile("mov %0, %%rax\n\t"
		"mov %%rax, %%cr3"
		:
		: "r" (val)
		: "%rax", "memory");
}

void process_table_init() {
	for(int i = 0; i < MAX_PROCESSES; i++) {
		process_table[i] = nullptr;
	}
}

Process* create_process(const char* name) {
	// 分配进程控制块
	Process* process = (Process*)kmalloc(sizeof(Process));
	if(!process) return nullptr;
	
	// 初始化进程信息  
	process->pid = next_pid++;
	strncpy(process->name, name, PROCESS_NAME_LENGTH-1);
	process->name[PROCESS_NAME_LENGTH-1] = '\0';
	process->state = PROCESS_CREATED;
	process->priority = 1;
	process->time_quantum = 100;
	
	// 分配内核栈
	process->kernel_stack_size = PROCESS_STACK_SIZE;
	process->kernel_stack = kmalloc(PROCESS_STACK_SIZE);
	if(!process->kernel_stack) {
		kfree(process);
		return nullptr;
	}
	
	// 设置栈指针
	process->esp = (uint32_t)((uintptr_t)process->kernel_stack + PROCESS_STACK_SIZE);
	process->ebp = process->esp;
	
	// 创建页目录
	process->page_directory = create_page_directory();
	if(!process->page_directory) {
		kfree(process->kernel_stack);
		kfree(process);
		return nullptr;
	}
	
	// 设置CR3值为页目录的物理地址
	process->cr3 = (uint32_t)((uintptr_t)process->page_directory & 0xFFFFFFFF); 
	
	// 添加到进程表
	for(int i = 0; i < MAX_PROCESSES; i++) {
		if(!process_table[i]) {
			process_table[i] = process;
			break;
		}
	}
	
	return process;
}

void destroy_process(Process* process) {
	if(!process) return;
	
	// 从进程表中移除
	for(int i = 0; i < MAX_PROCESSES; i++) {
		if(process_table[i] == process) {
			process_table[i] = nullptr;
			break;
		}
	}
	
	// 释放资源  
	if(process->kernel_stack) {
		kfree(process->kernel_stack);
	}
	if(process->page_directory) {
		destroy_page_directory(process->page_directory); 
	}
	
	kfree(process);
}

void switch_to_process(Process* next) {
	if(!next || next == current_process) return;
	
	Process* prev = current_process;
	current_process = next;
	
	// 保存当前进程状态
	if(prev) {
		asm volatile(
			"movl %%esp, %0\n"
			"movl %%ebp, %1"
			: "=m"(prev->esp), "=m"(prev->ebp)
			:
			: "memory"
			);
		prev->cr3 = read_cr3();  // 保存当前CR3值
	}
	
	// 加载新进程的页目录   
	write_cr3(next->cr3);
	
	// 恢复栈指针
	asm volatile(
		"movl %0, %%esp\n"
		"movl %1, %%ebp"
		:
		: "m"(next->esp), "m"(next->ebp)  
		: "memory"
		);
}

Process* get_next_process() {
	if(!ready_queue) return nullptr;
	
	Process* next = ready_queue;
	ready_queue = ready_queue->next;
	next->next = nullptr;
	return next;
}

void schedule() {
	Process* next = get_next_process();
	if(next) {
		switch_to_process(next);
	}
}

// 新增的进程优先级修改函数
bool change_process_priority(uint32_t pid, int32_t new_priority) {
	// 在进程表中查找指定PID的进程
	Process* target = nullptr;
	for(int i = 0; i < MAX_PROCESSES; i++) {
		if(process_table[i] && process_table[i]->pid == pid) {
			target = process_table[i];
			break;
		}
	}
	
	if(!target) {
		return false;  // 进程未找到
	}
	
	// 限制优先级范围(1-10，数字越大优先级越高)
	if(new_priority < 1) new_priority = 1;
	if(new_priority > 10) new_priority = 10;
	
	// 更新优先级
	target->priority = new_priority;
	
	return true;
}
