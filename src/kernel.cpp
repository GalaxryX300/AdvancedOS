// kernel.cpp - 基础内核实现
#include "kernel.h"
#include <string>
#include <iostream>
#include <queue>
#include <vector>
#include <map>
#include <algorithm>
#include <iterator>
#include <cstdio>
#include <cstdlib>

Kernel::Kernel() : next_pid(0), current_process(nullptr), total_memory(1024 * 1024) {
	// 初始化内存块 - 默认1MB内存
	memory_blocks.push_back(new MemoryBlock(0, total_memory));
}

Kernel::~Kernel() {
	// 清理资源
	for(auto p : process_table) delete p;
	for(auto m : memory_blocks) delete m;
	for(auto f : file_system) delete f.second;
}

// 进程管理实现
int Kernel::create_process(std::string name) {
	PCB* pcb = new PCB(next_pid++, name);
	process_table.push_back(pcb);
	ready_queue.push(pcb);
	
	// 为新进程分配内存
	pcb->memory_start = allocate_memory(1024); // 默认分配1KB
	pcb->memory_size = 1024;
	
	return pcb->pid;
}

void Kernel::terminate_process(int pid) {
	for(auto it = process_table.begin(); it != process_table.end(); ++it) {
		if((*it)->pid == pid) {
			// 释放进程占用的内存
			free_memory((*it)->memory_start);
			
			// 如果是当前运行的进程，需要调度下一个进程
			if(*it == current_process) {
				current_process = nullptr;
				schedule();
			}
			
			delete *it;
			process_table.erase(it);
			break;
		}
	}
}

void Kernel::schedule() {
	if(!ready_queue.empty()) {
		if(current_process) {
			current_process->state = 0; // 就绪状态
			ready_queue.push(current_process);
		}
		
		current_process = ready_queue.front();
		ready_queue.pop();
		current_process->state = 1; // 运行状态
	}
}

// 内存管理实现
unsigned long Kernel::allocate_memory(unsigned long size) {
	for(auto block : memory_blocks) {
		if(block->is_free && block->size >= size) {
			block->is_free = false;
			
			// 如果块太大，分割它
			if(block->size > size) {
				auto new_block = new MemoryBlock(
					block->start_address + size,
					block->size - size
					);
				block->size = size;
				memory_blocks.push_back(new_block);
			}
			
			return block->start_address;
		}
	}
	return -1; // 分配失败
}

void Kernel::free_memory(unsigned long address) {
	for(auto it = memory_blocks.begin(); it != memory_blocks.end(); ++it) {
		if((*it)->start_address == address) {
			(*it)->is_free = true;
			
			// 尝试合并相邻的空闲块
			auto next = std::next(it);
			if(next != memory_blocks.end() && (*next)->is_free) {
				(*it)->size += (*next)->size;
				auto temp = *next;
				memory_blocks.erase(next);
				delete temp;
			}
			
			if(it != memory_blocks.begin()) {
				auto prev = std::prev(it);
				if((*prev)->is_free) {
					(*prev)->size += (*it)->size;
					auto temp = *it;
					memory_blocks.erase(it);
					delete temp;
				}
			}
			break;
		}
	}
}

// 文件系统实现
bool Kernel::create_file(std::string name) {
	if(file_system.find(name) != file_system.end())
		return false;
	
	file_system[name] = new File(name);
	return true;
}

bool Kernel::write_file(std::string name, std::string content) {
	auto it = file_system.find(name);
	if(it == file_system.end()) return false;
	
	it->second->content = content;
	it->second->size = content.size();
	return true;
}

std::string Kernel::read_file(std::string name) {
	auto it = file_system.find(name);
	if(it == file_system.end()) return "";
	return it->second->content;
}

bool Kernel::delete_file(std::string name) {
	auto it = file_system.find(name);
	if(it == file_system.end()) return false;
	
	delete it->second;
	file_system.erase(it);
	return true;
}

// 设备管理实现
void Kernel::handle_keyboard_input(char input) {
	std::cout << "Keyboard input: " << input << std::endl;
}

void Kernel::display_output(std::string output) {
	std::cout << output << std::endl;
}

// 命令行接口实现
void Kernel::execute_command(std::string command) {
	if(command == "help") {
		display_help();
	}
	else if(command.substr(0, 7) == "create ") {
		std::string name = command.substr(7);
		if(create_file(name))
			std::cout << "File created: " << name << std::endl;
		else
			std::cout << "Failed to create file" << std::endl;
	}
	else if(command.substr(0, 6) == "write ") {
		size_t space_pos = command.find(" ", 6);
		if(space_pos != std::string::npos) {
			std::string name = command.substr(6, space_pos - 6);
			std::string content = command.substr(space_pos + 1);
			if(write_file(name, content))
				std::cout << "File written: " << name << std::endl;
			else
				std::cout << "Failed to write file" << std::endl;
		}
	}
	else if(command.substr(0, 5) == "read ") {
		std::string name = command.substr(5);
		std::string content = read_file(name);
		if(!content.empty())
			std::cout << "Content: " << content << std::endl;
		else
			std::cout << "File not found" << std::endl;
	}
	else if(command.substr(0, 7) == "delete ") {
		std::string name = command.substr(7);
		if(delete_file(name))
			std::cout << "File deleted: " << name << std::endl;
		else
			std::cout << "Failed to delete file" << std::endl;
	}
	else {
		std::cout << "Unknown command. Type 'help' for available commands." << std::endl;
	}
}

void Kernel::display_help() {
	std::cout << "Available commands:\n"
	<< "help - Display this help message\n"
	<< "create <filename> - Create a new file\n"
	<< "write <filename> <content> - Write to a file\n"
	<< "read <filename> - Read file content\n"
	<< "delete <filename> - Delete a file\n"
	<< "exit - Exit the system\n";
}

void panic(const char* message) {
	fprintf(stderr, "Panic: %s\n", message);
	exit(1);
}
