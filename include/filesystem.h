// include/filesystem.h
#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <string>
#include <vector>
#include <ctime>
#include "types.h"

#define BLOCK_SIZE 4096
#define MAX_FILENAME 256
#define MAX_BLOCKS_PER_FILE 1024

// 文件系统超级块
struct SuperBlock {
    uint32_t magic;          // 文件系统魔数
    uint32_t block_size;     // 块大小
    uint32_t total_blocks;   // 总块数
    uint32_t free_blocks;    // 空闲块数
    uint32_t first_data_block; // 第一个数据块的位置
};

// 文件条目
struct FileEntry {
    char name[MAX_FILENAME];
    size_t size;
    uint32_t blocks[MAX_BLOCKS_PER_FILE];
    uint32_t block_count;
    bool is_directory;
    uint32_t permissions;
    time_t created_time;
    time_t modified_time;
    time_t accessed_time;
};

class FileSystem {
public:
    FileSystem();
    ~FileSystem();
    
    // 文件系统操作
    bool mount(const std::string& device, const std::string& mount_point);
    bool unmount();
    bool format(uint32_t size);
    
    // 文件操作
    bool create_file(const std::string& path);
    bool write_file(const std::string& path, const void* data, size_t size);
    bool read_file(const std::string& path, void* buffer, size_t size);
    bool delete_file(const std::string& path);
    bool create_directory(const std::string& path);
    bool is_directory(const std::string& path);
    std::vector<std::string> list_directory(const std::string& path);

protected:
    // 内部辅助函数
    uint32_t allocate_block();
    void free_block(uint32_t block_number);
    FileEntry* find_file(const std::string& path);
    std::string get_filename(const std::string& path) {
        size_t pos = path.find_last_of("/");
        return (pos == std::string::npos) ? path : path.substr(pos + 1);
    }

private:
    SuperBlock superblock;
    std::vector<bool> block_bitmap;
    std::vector<FileEntry> file_table;
    std::string mount_point;
    bool mounted;
};

#endif // FILESYSTEM_H