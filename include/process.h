// process.h
#ifndef PROCESS_H
#define PROCESS_H

#include "types.h"
#include "memory.h"

#define MAX_PROCESSES 64
#define PROCESS_STACK_SIZE 4096
#define PROCESS_NAME_LENGTH 32

struct Process {
    uint32_t pid;
    char name[PROCESS_NAME_LENGTH];
    process_state_t state;  // 使用types.h中定义的进程状态类型
    uint32_t priority;
    uint32_t time_quantum;
    
    // 内存相关
    page_directory_t* page_directory;  // 改用正确的页目录类型
    void* kernel_stack;
    uint32_t kernel_stack_size;
    
    // CPU状态
    uint32_t eip;
    uint32_t esp;
    uint32_t ebp;
    uint32_t cr3;
    
    // 链表指针
    Process* next;
};

// 进程管理函数
void process_table_init();
Process* create_process(const char* name);
void destroy_process(Process* process);
void switch_to_process(Process* process);
Process* get_next_process();

// 调度器函数
void scheduler_init();
void schedule();
void yield();

// 新增: 进程优先级修改函数
bool change_process_priority(uint32_t pid, int32_t new_priority);

#endif // PROCESS_H