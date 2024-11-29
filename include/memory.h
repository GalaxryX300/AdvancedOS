// memory.h
#ifndef MEMORY_H
#define MEMORY_H

#include <cstdint>
#include <cstddef>

#define PAGE_SIZE 4096
#define HEAP_START 0x00100000
#define HEAP_END   0x00900000

struct page_table_entry_t {
    uint32_t present    : 1;
    uint32_t rw         : 1;
    uint32_t user       : 1;
    uint32_t accessed   : 1;
    uint32_t dirty      : 1;
    uint32_t unused     : 7;
    uint32_t frame      : 20;
};

struct page_table_t {
    page_table_entry_t pages[1024];
};

struct page_directory_t {
    page_table_t* tables[1024];
    uint32_t physicalAddr;
};

struct block_header_t {
    size_t size;
    int is_free;
    block_header_t* next;
};

void init_paging();
void init_physical_memory();
void init_heap();
page_directory_t* create_page_directory();
void destroy_page_directory(page_directory_t* dir);
void map_page(void* virtual_addr, void* physical_addr, uint32_t flags);
void* kmalloc(size_t size);
void kfree(void* ptr);
void page_fault_handler(uint32_t error_code);
void switch_page_directory(page_directory_t* dir);
void* alloc_frame();
void free_frame(void* frame);

#endif // MEMORY_H