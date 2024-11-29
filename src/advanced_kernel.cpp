// advanced_kernel.cpp - 扩展内核实现
#include "../include/advanced_kernel.h"
#include <sstream>

// VirtualMemoryManager实现
VirtualMemoryManager::VirtualMemoryManager() {
	physical_memory_map.resize(TOTAL_MEMORY / PAGE_SIZE, false);
}

unsigned long VirtualMemoryManager::allocate_page() {
	for(size_t i = 0; i < physical_memory_map.size(); i++) {
		if(!physical_memory_map[i]) {
			physical_memory_map[i] = true;
			return i * PAGE_SIZE;
		}
	}
	return -1; // 内存已满
}

void VirtualMemoryManager::free_page(unsigned long physical_address) {
	size_t page_index = physical_address / PAGE_SIZE;
	if(page_index < physical_memory_map.size()) {
		physical_memory_map[page_index] = false;
	}
}

unsigned long VirtualMemoryManager::get_physical_address(unsigned long virtual_address) {
	auto it = page_mapping.find(virtual_address);
	if(it != page_mapping.end()) {
		return it->second;
	}
	handle_page_fault(virtual_address);
	return page_mapping[virtual_address];
}

void VirtualMemoryManager::handle_page_fault(unsigned long virtual_address) {
	unsigned long physical_address = allocate_page();
	if(physical_address != -1) {
		page_mapping[virtual_address] = physical_address;
	}
}

// Scheduler实现
Scheduler::Scheduler() : current_process(nullptr) {
	priority_queues.resize(MAX_PRIORITY);
}

void Scheduler::add_process(AdvancedPCB* pcb) {
	std::lock_guard<std::mutex> lock(scheduler_mutex);
	priority_queues[pcb->priority].push(pcb);
}

AdvancedPCB* Scheduler::get_next_process() {
	std::lock_guard<std::mutex> lock(scheduler_mutex);
	
	// 从最高优先级队列开始查找
	for(int i = MAX_PRIORITY - 1; i >= 0; i--) {
		if(!priority_queues[i].empty()) {
			AdvancedPCB* next = priority_queues[i].front();
			priority_queues[i].pop();
			return next;
		}
	}
	return nullptr;
}

void Scheduler::update_priority(AdvancedPCB* pcb) {
	std::lock_guard<std::mutex> lock(scheduler_mutex);
	
	// 根据进程的nice值和CPU使用时间调整优先级
	int new_priority = pcb->priority;
	if(pcb->cpu_time > TIME_SLICE) {
		new_priority = std::max(0, new_priority - 1);
	}
	new_priority = std::max(0, std::min(MAX_PRIORITY - 1, 
		new_priority + pcb->nice_value));
	pcb->priority = new_priority;
}

// AdvancedKernel实现
AdvancedKernel::AdvancedKernel() : next_pid(0) {
	// 创建初始系统进程
	create_process("init", MAX_PRIORITY - 1);
}

AdvancedKernel::~AdvancedKernel() {
	for(auto pcb : all_processes) {
		delete pcb;
	}
}

int AdvancedKernel::create_process(std::string name, int priority) {
	std::lock_guard<std::mutex> lock(kernel_mutex);
	
	auto pcb = new AdvancedPCB(next_pid++, name);
	pcb->priority = std::min(priority, MAX_PRIORITY - 1);
	
	// 分配虚拟内存空间
	pcb->virtual_memory_size = PAGE_SIZE; // 初始分配一页
	pcb->page_table.push_back(PageTableEntry());
	
	all_processes.push_back(pcb);
	scheduler.add_process(pcb);
	
	return pcb->pid;
}

bool AdvancedKernel::change_process_priority(uint32_t pid, int32_t new_priority) {
	std::lock_guard<std::mutex> lock(kernel_mutex);
	
	for(auto pcb : all_processes) {
		if(pcb->pid == pid) {
			// 将优先级映射到合适的范围（0到MAX_PRIORITY-1）
			int mapped_priority = std::max(0, std::min(MAX_PRIORITY - 1, new_priority));
			pcb->priority = mapped_priority;
			
			// 更新进程在调度器中的优先级
			scheduler.update_priority(pcb);
			
			return true;
		}
	}
	return false;
}

void AdvancedKernel::terminate_process(int pid) {
	std::lock_guard<std::mutex> lock(kernel_mutex);
	
	for(auto it = all_processes.begin(); it != all_processes.end(); ++it) {
		if((*it)->pid == pid) {
			// 释放进程的所有页面
			for(auto& page : (*it)->page_table) {
				if(page.present) {
					vmm.free_page(page.physical_address);
				}
			}
			
			delete *it;
			all_processes.erase(it);
			break;
		}
	}
}

void AdvancedKernel::yield_cpu() {
	std::lock_guard<std::mutex> lock(kernel_mutex);
	schedule();
}

void AdvancedKernel::sleep_process(int pid, unsigned long milliseconds) {
	std::lock_guard<std::mutex> lock(kernel_mutex);
	
	for(auto pcb : all_processes) {
		if(pcb->pid == pid) {
			pcb->state = BLOCKED;
			
			// 创建睡眠线程
			std::thread([this, pid, milliseconds]() {
				std::this_thread::sleep_for(
					std::chrono::milliseconds(milliseconds));
				
				std::lock_guard<std::mutex> lock(kernel_mutex);
				for(auto pcb : all_processes) {
					if(pcb->pid == pid && pcb->state == BLOCKED) {
						pcb->state = READY;
						scheduler.add_process(pcb);
						break;
					}
				}
			}).detach();
			
			break;
		}
	}
}

void AdvancedKernel::schedule() {
	AdvancedPCB* next = scheduler.get_next_process();
	if(next) {
		if(next->state == READY) {
			next->state = RUNNING;
			scheduler.update_priority(next);
		}
	}
}

void* AdvancedKernel::allocate_memory(size_t size) {
	size_t pages_needed = (size + PAGE_SIZE - 1) / PAGE_SIZE;
	std::vector<unsigned long> allocated_pages;
	
	for(size_t i = 0; i < pages_needed; i++) {
		unsigned long physical_address = vmm.allocate_page();
		if(physical_address == -1) {
			// 分配失败，释放已分配的页面
			for(auto page : allocated_pages) {
				vmm.free_page(page);
			}
			return nullptr;
		}
		allocated_pages.push_back(physical_address);
	}
	
	// 成功分配所有页面
	return (void*)allocated_pages[0];
}

std::vector<AdvancedPCB*> AdvancedKernel::get_process_list() const {
	return all_processes;
}

void AdvancedKernel::handle_timer_interrupt() {
	std::lock_guard<std::mutex> lock(kernel_mutex);
	
	// 更新当前进程的CPU时间
	for(auto pcb : all_processes) {
		if(pcb->state == RUNNING) {
			pcb->cpu_time += TIME_SLICE;
			break;
		}
	}
	
	// 检查是否需要重新调度
	schedule();
}

void AdvancedKernel::execute_command(const std::string& command) {
	std::lock_guard<std::mutex> lock(kernel_mutex);
	
	// 解析命令
	std::istringstream iss(command);
	std::string cmd;
	iss >> cmd;
	
	if(cmd == "ps" || cmd == "processes") {
		std::cout << "Process List:\n";
		std::cout << "PID\tName\tState\tPriority\tCPU Time\n";
		for(const auto& pcb : all_processes) {
			std::cout << pcb->pid << "\t"
			<< pcb->name << "\t"
			<< static_cast<int>(pcb->state) << "\t"
			<< pcb->priority << "\t\t"
			<< pcb->cpu_time << "ms\n";
		}
	}
	else if(cmd == "kill") {
		int pid;
		if(iss >> pid) {
			terminate_process(pid);
			std::cout << "Process " << pid << " terminated.\n";
		}
	}
	else if(cmd == "nice") {
		int pid, value;
		if(iss >> pid >> value) {
			for(auto pcb : all_processes) {
				if(pcb->pid == pid) {
					pcb->nice_value = std::min(19, std::max(-20, value));
					scheduler.update_priority(pcb);
					std::cout << "Updated nice value for process " << pid << "\n";
					break;
				}
			}
		}
	}
	else if(cmd == "mem" || cmd == "memory") {
		SystemInfo info = get_system_info();
		std::cout << "Memory Information:\n"
		<< "Total: " << info.total_memory / 1024 << "KB\n"
		<< "Used: " << info.used_memory / 1024 << "KB\n"
		<< "Free: " << info.free_memory / 1024 << "KB\n";
	}
	else if(cmd == "help") {
		std::cout << "Available commands:\n"
		<< "ps, processes - List all processes\n"
		<< "kill <pid> - Terminate a process\n"
		<< "nice <pid> <value> - Adjust process priority\n"
		<< "mem, memory - Show memory information\n"
		<< "help - Show this help message\n";
	}
	else {
		std::cout << "Unknown command. Type 'help' for available commands.\n";
	}
}

AdvancedKernel::SystemInfo AdvancedKernel::get_system_info() const {
	SystemInfo info;
	info.total_memory = TOTAL_MEMORY;
	
	// 计算已使用的内存
	size_t used = 0;
	for(const auto& pcb : all_processes) {
		used += pcb->virtual_memory_size;
	}
	
	info.used_memory = used;
	info.free_memory = TOTAL_MEMORY - used;
	info.total_processes = all_processes.size();
	
	// 计算运行中的进程数
	info.running_processes = 0;
	for(const auto& pcb : all_processes) {
		if(pcb->state == RUNNING) {
			info.running_processes++;
		}
	}
	
	return info;
}
