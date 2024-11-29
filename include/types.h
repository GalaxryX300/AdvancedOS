// include/types.h
#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>
#include <stddef.h>
#include <time.h>

// 基本类型定义
typedef unsigned char  uint8;
typedef unsigned short uint16;
typedef unsigned int   uint32;
typedef unsigned long long uint64;

typedef signed char    int8;
typedef signed short   int16;
typedef signed int     int32;
typedef signed long long int64;

// 内存地址类型
typedef uint32 physical_addr_t;
typedef uint32 virtual_addr_t;

// 进程ID类型
typedef int32 pid_t;

// 错误码
typedef int32 error_t;

// 文件大小类型
typedef uint64 file_size_t;

// 时间类型
typedef uint64 system_time_t;

// 布尔类型
#ifndef __cplusplus
typedef enum { false = 0, true = 1 } bool;
#endif

// 内存页大小
#define PAGE_SIZE 4096
#define PAGE_MASK (~(PAGE_SIZE-1))
#define PAGE_SHIFT 12

// 权限标志
#define PERM_READ    0x04
#define PERM_WRITE   0x02
#define PERM_EXECUTE 0x01

// 文件系统常量
#define MAX_PATH_LENGTH 256
#define MAX_NAME_LENGTH 128
#define SECTOR_SIZE    512

// 系统常量
#define MAX_PROCESSES  64
#define MAX_THREADS    128
#define MAX_FILES      1024
#define MAX_SEMAPHORES 256

// 内存区域标志
#define MEMORY_KERNEL    0x01   // 内核内存
#define MEMORY_USER      0x02   // 用户内存
#define MEMORY_READABLE  0x04   // 可读
#define MEMORY_WRITABLE  0x08   // 可写
#define MEMORY_EXECUTABLE 0x10  // 可执行

// 错误码定义
#define ERROR_SUCCESS       0
#define ERROR_INVALID      -1
#define ERROR_NOT_FOUND    -2
#define ERROR_NO_MEMORY    -3
#define ERROR_IO           -4
#define ERROR_BUSY         -5
#define ERROR_EXIST        -6
#define ERROR_NOT_READY    -7
#define ERROR_PERMISSION   -8

// 系统状态
typedef enum {
    SYSTEM_RUNNING,
    SYSTEM_HALTED,
    SYSTEM_PANIC
} system_state_t;

// 进程状态
typedef enum {
    PROCESS_CREATED,
    PROCESS_READY,
    PROCESS_RUNNING,
    PROCESS_WAITING,
    PROCESS_TERMINATED
} process_state_t;

// 内存区域描述符
typedef struct {
    virtual_addr_t start_address;
    size_t size;
    uint32_t flags;
} memory_region_t;

// 硬盘扇区结构
typedef struct {
    uint16_t cylinder;
    uint8_t head;
    uint8_t sector;
} disk_sector_t;

#endif // TYPES_H