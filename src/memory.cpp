// memory.cpp
#include "memory.h"
#include "utils.h"
#include <cstring>
#include <cstdlib>

// 模拟物理内存管理
static uint8_t* simulated_physical_memory = nullptr;
static size_t simulated_memory_size = 1024 * 1024 * 1024; // 1GB

// 模拟页目录和页表
static page_directory_t* current_directory = nullptr;
static block_header_t* heap_start = nullptr;

void init_paging() {
	// 分配模拟的物理内存
	simulated_physical_memory = static_cast<uint8_t*>(malloc(simulated_memory_size));
	if (!simulated_physical_memory) {
		panic("Failed to allocate simulated physical memory");
	}
	
	// 创建内核页目录
	page_directory_t* kernel_dir = create_page_directory();
	
	// 映射内核空间（前4MB）
	for(uint32_t i = 0; i < 1024; i++) {
		map_page(reinterpret_cast<void*>(i * PAGE_SIZE), reinterpret_cast<void*>(i * PAGE_SIZE), 0x03);
	}
	
	// 切换到新页目录
	switch_page_directory(kernel_dir);
	
	kprintf("Paging initialized (simulated)\n");
}

void init_physical_memory() {
	// 模拟标记内核使用的帧
	kprintf("Physical memory initialized (simulated)\n");
}

void init_heap() {
	// 模拟初始化内核堆
	heap_start = reinterpret_cast<block_header_t*>(malloc(HEAP_END - HEAP_START));
	if (!heap_start) {
		panic("Failed to allocate simulated heap");
	}
	heap_start->size = HEAP_END - HEAP_START - sizeof(block_header_t);
	heap_start->is_free = 1;
	heap_start->next = nullptr;
	
	kprintf("Heap initialized (simulated)\n");
}

page_directory_t* create_page_directory() {
	page_directory_t* dir = static_cast<page_directory_t*>(malloc(sizeof(page_directory_t)));
	if (dir) {
		memset(dir, 0, sizeof(page_directory_t));
	}
	return dir;
}

void destroy_page_directory(page_directory_t* dir) {
	if (dir) {
		// 释放页表
		for (int i = 0; i < 1024; i++) {
			if (dir->tables[i]) {
				free(dir->tables[i]);
			}
		}
		// 释放页目录
		free(dir);
	}
}

void map_page(void* virtual_addr, void* physical_addr, uint32_t flags) {
	kprintf("Mapping page: virtual 0x%p to physical 0x%p with flags 0x%x\n", virtual_addr, physical_addr, flags);
}

void* kmalloc(size_t size) {
	if(size == 0) return nullptr;
	
	// 对齐到8字节
	size = (size + 7) & ~7;
	
	block_header_t* current = heap_start;
	while(current) {
		if(current->is_free && current->size >= size) {
			// 足够大的块
			if(current->size >= size + sizeof(block_header_t) + 8) {
				// 分割块
				block_header_t* new_block = reinterpret_cast<block_header_t*>(
					reinterpret_cast<char*>(current) + sizeof(block_header_t) + size);
				new_block->size = current->size - size - sizeof(block_header_t);
				new_block->is_free = 1;
				new_block->next = current->next;
				
				current->size = size;
				current->next = new_block;
			}
			
			current->is_free = 0;
			return reinterpret_cast<void*>(reinterpret_cast<char*>(current) + sizeof(block_header_t));
		}
		current = current->next;
	}
	
	return nullptr;
}

void kfree(void* ptr) {
	if(!ptr) return;
	
	block_header_t* header = reinterpret_cast<block_header_t*>(
		reinterpret_cast<char*>(ptr) - sizeof(block_header_t));
	header->is_free = 1;
	
	// 合并相邻的空闲块
	block_header_t* current = heap_start;
	while(current && current->next) {
		if(current->is_free && current->next->is_free) {
			current->size += sizeof(block_header_t) + current->next->size;
			current->next = current->next->next;
		} else {
			current = current->next;
		}
	}
}

void page_fault_handler(uint32_t error_code) {
	uint32_t faulting_address = 0; // 模拟值
	
	kprintf("Page fault! ( ");
	if(error_code & 0x1) kprintf("present ");
	if(error_code & 0x2) kprintf("write ");
	if(error_code & 0x4) kprintf("user ");
	if(error_code & 0x8) kprintf("reserved ");
	kprintf(") at 0x%x\n", faulting_address);
	
	// 如果是内核页错误，停止系统
	if(!(error_code & 0x4)) {
		panic("Kernel page fault!");
	}
}

void switch_page_directory(page_directory_t* dir) {
	current_directory = dir;
	kprintf("Switched to new page directory (simulated)\n");
}

void* alloc_frame() {
	// 模拟分配一个物理帧
	return malloc(PAGE_SIZE);
}

void free_frame(void* frame) {
	// 模拟释放一个物理帧
	free(frame);
}
