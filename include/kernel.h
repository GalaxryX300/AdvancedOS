// include/kernel.h
#ifndef KERNEL_H
#define KERNEL_H

#include <string>
#include <queue>
#include <vector>
#include <map>
#include "types.h"

void panic(const char* message);
void kprintf(const char* format, ...);

// 进程控制块
struct PCB {
    int pid;
    std::string name;
    int state;  // 0=就绪, 1=运行
    unsigned long memory_start;
    unsigned long memory_size;
    
    PCB(int p, const std::string& n) : 
        pid(p), name(n), state(0), 
        memory_start(0), memory_size(0) {}
};

// 内存块
struct MemoryBlock {
    unsigned long start_address;
    unsigned long size;
    bool is_free;
    
    MemoryBlock(unsigned long start, unsigned long s) : 
        start_address(start), size(s), is_free(true) {}
};

// 文件
struct File {
    std::string name;
    std::string content;
    size_t size;
    
    File(const std::string& n) : name(n), size(0) {}
};

class Kernel {
public:
    Kernel();
    ~Kernel();
    
    // 进程管理
    int create_process(std::string name);
    void terminate_process(int pid);
    void schedule();
    
    // 内存管理
    unsigned long allocate_memory(unsigned long size);
    void free_memory(unsigned long address);
    
    // 文件系统
    bool create_file(std::string name);
    bool write_file(std::string name, std::string content);
    std::string read_file(std::string name);
    bool delete_file(std::string name);
    
    // 设备管理
    void handle_keyboard_input(char input);
    void display_output(std::string output);
    
    // 命令行接口
    void execute_command(std::string command);
    void display_help();
    
private:
    int next_pid;
    PCB* current_process;
    std::vector<PCB*> process_table;
    std::queue<PCB*> ready_queue;
    std::vector<MemoryBlock*> memory_blocks;
    std::map<std::string, File*> file_system;
    unsigned long total_memory;
};

#endif // KERNEL_H