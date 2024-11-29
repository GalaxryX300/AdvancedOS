// disk_manager.h - 磁盘管理头文件
#ifndef DISK_MANAGER_H
#define DISK_MANAGER_H

#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <fstream>
#include <ctime>

class DiskManager {
private:
    struct Partition {
        std::string name;
        size_t size;
        size_t used_space;
        std::string mount_point;
        bool is_mounted;
        std::string filesystem_type;
        
        Partition(std::string n, size_t s) 
            : name(n), size(s), used_space(0), is_mounted(false),
              filesystem_type("ext4") {}
    };
    
    struct BlockInfo {
        size_t block_number;
        bool is_free;
        size_t size;
    };

    struct FileEntry {
        std::string name;
        std::string type;
        size_t size;
        time_t modified_time;
        size_t start_block;
        size_t block_count;
    };
    
    std::string disk_file;
    size_t total_size;
    size_t block_size;
    std::vector<Partition> partitions;
    std::vector<BlockInfo> block_map;
    std::map<std::string, std::vector<FileEntry>> filesystem;
    std::mutex disk_mutex;
    
    // 磁盘文件操作
    bool write_to_disk(size_t offset, const void* data, size_t size);
    bool read_from_disk(size_t offset, void* buffer, size_t size);
    size_t allocate_blocks(size_t size);
    void free_blocks(size_t start_block, size_t count);
    
public:
    DiskManager(const std::string& disk_file, size_t size, size_t block_size = 4096);
    ~DiskManager();
    
    // 分区管理
    bool create_partition(const std::string& name, size_t size);
    bool delete_partition(const std::string& name);
    bool mount_partition(const std::string& name, const std::string& mount_point);
    bool unmount_partition(const std::string& name);
    bool format_partition(const std::string& name);
    
    // 文件系统操作
    struct FileInfo {
        std::string name;
        std::string type;
        size_t size;
        time_t modified_time;
        std::string path;
    };
    
    std::vector<FileInfo> list_files(const std::string& path = "");
    bool create_file(const std::string& filename, const std::string& content = "");
    bool create_directory(const std::string& dirname);
    bool delete_file(const std::string& filename);
    bool delete_directory(const std::string& dirname);
    bool write_file(const std::string& filename, const std::string& content);
    std::string read_file(const std::string& filename);
    
    // 空间管理
    size_t get_free_space() const;
    size_t get_used_space() const;
    size_t get_total_space() const;
    
    // 分区信息
    struct PartitionInfo {
        std::string name;
        size_t size;
        size_t used_space;
        std::string mount_point;
        bool is_mounted;
    };
    
    std::vector<PartitionInfo> list_partitions() const;
    PartitionInfo get_partition_info(const std::string& name) const;
};

#endif // DISK_MANAGER_H