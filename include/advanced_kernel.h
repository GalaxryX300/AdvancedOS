// advanced_kernel.h - 扩展的内核头文件
#ifndef ADVANCED_KERNEL_H
#define ADVANCED_KERNEL_H

#include <iostream>
#include <vector>
#include <queue>
#include <map>
#include <string>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <memory>
#include <algorithm>

// 常量定义
const int MAX_PROCESS = 100;
const int MAX_PRIORITY = 10;
const int TIME_SLICE = 100; // ms
const int PAGE_SIZE = 4096; // 4KB
const int TOTAL_MEMORY = 1024 * 1024 * 1024; // 1GB

// 进程状态枚举
enum ProcessState {
    READY,
    RUNNING,
    BLOCKED,
    TERMINATED
};

// 页表项结构
struct PageTableEntry {
    unsigned long physical_address;
    bool present;
    bool dirty;
    bool accessed;
    
    PageTableEntry() : physical_address(0), present(false), dirty(false), accessed(false) {}
};

// 扩展的进程控制块
struct AdvancedPCB {
    int pid;
    std::string name;
    ProcessState state;
    int priority;
    std::vector<PageTableEntry> page_table;
    unsigned long virtual_memory_size;
    time_t creation_time;
    time_t cpu_time;
    int nice_value;
    
    // 进程间通信相关
    std::vector<int> open_pipes;
    std::map<std::string, void*> shared_memory;
    
    AdvancedPCB(int id, std::string n) 
        : pid(id), name(n), state(READY), priority(0), 
          virtual_memory_size(0), cpu_time(0), nice_value(0) {
        creation_time = time(nullptr);
    }
};

// 虚拟内存管理
class VirtualMemoryManager {
private:
    std::vector<bool> physical_memory_map;
    std::map<unsigned long, unsigned long> page_mapping;
    
public:
    VirtualMemoryManager();
    unsigned long allocate_page();
    void free_page(unsigned long physical_address);
    unsigned long get_physical_address(unsigned long virtual_address);
    void handle_page_fault(unsigned long virtual_address);
};

// 进程调度器 - 多级反馈队列
class Scheduler {
private:
    std::vector<std::queue<AdvancedPCB*>> priority_queues;
    AdvancedPCB* current_process;
    std::mutex scheduler_mutex;
    
public:
    Scheduler();
    void add_process(AdvancedPCB* pcb);
    AdvancedPCB* get_next_process();
    void update_priority(AdvancedPCB* pcb);
    void time_slice_expired();
};

// 高级内核类
class AdvancedKernel {
private:
    VirtualMemoryManager vmm;
    Scheduler scheduler;
    std::vector<AdvancedPCB*> all_processes;
    int next_pid;
    
    // 互斥锁和条件变量
    std::mutex kernel_mutex;
    std::condition_variable process_wait;
    
public:
    AdvancedKernel();
    ~AdvancedKernel();
    
    // 进程管理
    int create_process(std::string name, int priority = 0);
    void terminate_process(int pid);
    void yield_cpu();
    void sleep_process(int pid, unsigned long milliseconds);
    std::vector<AdvancedPCB*> get_process_list() const;
    bool change_process_priority(uint32_t pid, int32_t new_priority);  // 新增的进程优先级修改方法
    
    // 内存管理
    void* allocate_memory(size_t size);
    void free_memory(void* ptr);
    
    // 系统调度
    void schedule();
    void handle_timer_interrupt();
    
    // 命令行接口
    void execute_command(const std::string& command);
    
    // 获取系统信息
    struct SystemInfo {
        unsigned long total_memory;
        unsigned long used_memory;
        unsigned long free_memory;
        int total_processes;
        int running_processes;
    };
    
    SystemInfo get_system_info() const;
};

#endif // ADVANCED_KERNEL_H