// disk_manager.cpp - 磁盘管理实现
#include "../include/disk_manager.h"
#include <algorithm>
#include <cstring>
#include <sys/stat.h>
#include <unistd.h>
#include <sstream>

DiskManager::DiskManager(const std::string& file, size_t size, size_t bs) 
: disk_file(file), total_size(size), block_size(bs) {
	
	// 打开或创建磁盘文件
	std::fstream disk(disk_file, std::ios::in | std::ios::out | std::ios::binary);
	if(!disk) {
		disk.open(disk_file, std::ios::out | std::ios::binary);
		// 初始化磁盘文件
		std::vector<char> empty(size, 0);
		disk.write(empty.data(), size);
	}
	disk.close();
	
	// 初始化块映射
	size_t total_blocks = total_size / block_size;
	block_map.resize(total_blocks);
	for(size_t i = 0; i < total_blocks; i++) {
		block_map[i] = {i, true, block_size};
	}
	
	// 创建根分区
	create_partition("/", size / 2);
	mount_partition("/", "/");
	
	create_partition("/", size / 2);
	mount_partition("/", "/");
	create_partition("/home", size / 4);
	mount_partition("/home", "/home");
	create_partition("/var", size / 8);
	mount_partition("/var", "/var");
	
	// 模拟使用空间
	for(auto& partition : partitions) {
		if(partition.name == "/") {
			partition.used_space = partition.size * 0.65; // 65% 使用率
		} else if(partition.name == "/home") {
			partition.used_space = partition.size * 0.45; // 45% 使用率
		} else if(partition.name == "/var") {
			partition.used_space = partition.size * 0.25; // 25% 使用率
		}
	}
}

DiskManager::~DiskManager() {
	// 确保所有分区都被卸载
	for(auto& partition : partitions) {
		if(partition.is_mounted) {
			unmount_partition(partition.name);
		}
	}
}

bool DiskManager::write_to_disk(size_t offset, const void* data, size_t size) {
	std::lock_guard<std::mutex> lock(disk_mutex);
	
	std::fstream disk(disk_file, std::ios::in | std::ios::out | std::ios::binary);
	if(!disk) {
		return false;
	}
	
	disk.seekp(offset);
	disk.write(static_cast<const char*>(data), size);
	return disk.good();
}

bool DiskManager::read_from_disk(size_t offset, void* buffer, size_t size) {
	std::lock_guard<std::mutex> lock(disk_mutex);
	
	std::fstream disk(disk_file, std::ios::in | std::ios::binary);
	if(!disk) {
		return false;
	}
	
	disk.seekg(offset);
	disk.read(static_cast<char*>(buffer), size);
	return disk.good();
}

size_t DiskManager::allocate_blocks(size_t size) {
	size_t blocks_needed = (size + block_size - 1) / block_size;
	size_t start_block = 0;
	size_t consecutive_blocks = 0;
	
	// 寻找连续的空闲块
	for(size_t i = 0; i < block_map.size(); i++) {
		if(block_map[i].is_free) {
			if(consecutive_blocks == 0) {
				start_block = i;
			}
			consecutive_blocks++;
			
			if(consecutive_blocks >= blocks_needed) {
				// 标记这些块为已使用
				for(size_t j = start_block; j < start_block + blocks_needed; j++) {
					block_map[j].is_free = false;
				}
				return start_block;
			}
		} else {
			consecutive_blocks = 0;
		}
	}
	
	return -1; // 分配失败
}

void DiskManager::free_blocks(size_t start_block, size_t count) {
	for(size_t i = start_block; i < start_block + count && i < block_map.size(); i++) {
		block_map[i].is_free = true;
	}
}

bool DiskManager::create_partition(const std::string& name, size_t size) {
	std::lock_guard<std::mutex> lock(disk_mutex);
	
	// 检查分区名是否已存在
	for(const auto& part : partitions) {
		if(part.name == name) {
			return false;
		}
	}
	
	// 检查是否有足够的空间
	size_t total_used = 0;
	for(const auto& part : partitions) {
		total_used += part.size;
	}
	
	if(total_used + size > total_size) {
		return false;
	}
	
	// 创建新分区
	partitions.push_back(Partition(name, size));
	return true;
}

bool DiskManager::delete_partition(const std::string& name) {
	std::lock_guard<std::mutex> lock(disk_mutex);
	
	for(auto it = partitions.begin(); it != partitions.end(); ++it) {
		if(it->name == name) {
			if(it->is_mounted) {
				return false; // 不能删除已挂载的分区
			}
			partitions.erase(it);
			return true;
		}
	}
	return false;
}

bool DiskManager::mount_partition(const std::string& name, 
	const std::string& mount_point) {
		std::lock_guard<std::mutex> lock(disk_mutex);
		
		// 检查挂载点是否已被使用
		for(const auto& part : partitions) {
			if(part.is_mounted && part.mount_point == mount_point) {
				return false;
			}
		}
		
		// 查找并挂载分区
		for(auto& part : partitions) {
			if(part.name == name) {
				if(part.is_mounted) {
					return false; // 已经被挂载
				}
				part.mount_point = mount_point;
				part.is_mounted = true;
				filesystem[mount_point] = std::vector<FileEntry>();
				return true;
			}
		}
		return false;
	}

bool DiskManager::unmount_partition(const std::string& name) {
	std::lock_guard<std::mutex> lock(disk_mutex);
	
	for(auto& part : partitions) {
		if(part.name == name && part.is_mounted) {
			filesystem.erase(part.mount_point);
			part.is_mounted = false;
			part.mount_point.clear();
			return true;
		}
	}
	return false;
}

bool DiskManager::format_partition(const std::string& name) {
	std::lock_guard<std::mutex> lock(disk_mutex);
	
	for(auto& part : partitions) {
		if(part.name == name) {
			if(!part.is_mounted) {
				return false; // 分区必须先挂载才能格式化
			}
			
			// 清除文件系统中该分区的所有文件
			filesystem[part.mount_point].clear();
			
			// 重置使用空间
			part.used_space = 0;
			
			// 释放所有块
			size_t start_block = (part.mount_point == "/") ? 0 : 
			allocate_blocks(part.size);
			size_t block_count = part.size / block_size;
			free_blocks(start_block, block_count);
			
			return true;
		}
	}
	return false;
}

std::vector<DiskManager::FileInfo> DiskManager::list_files(const std::string& path) {
	std::lock_guard<std::mutex> lock(disk_mutex);
	std::vector<FileInfo> files;
	
	// 查找对应目录的文件列表
	auto it = filesystem.find(path.empty() ? "/" : path);
	if(it != filesystem.end()) {
		for(const auto& entry : it->second) {
			FileInfo info;
			info.name = entry.name;
			info.type = entry.type;
			info.size = entry.size;
			info.modified_time = entry.modified_time;
			info.path = path;
			files.push_back(info);
		}
	}
	
	return files;
}

bool DiskManager::create_file(const std::string& filename, const std::string& content) {
	std::lock_guard<std::mutex> lock(disk_mutex);
	
	// 查找文件所在分区
	std::string mount_point = "/";
	for(const auto& part : partitions) {
		if(filename.find(part.mount_point) == 0 && 
			part.mount_point.length() > mount_point.length()) {
			mount_point = part.mount_point;
		}
	}
	
	// 创建文件条目
	FileEntry entry;
	entry.name = filename.substr(filename.find_last_of("/") + 1);
	entry.type = "file";
	entry.size = content.length();
	entry.modified_time = std::time(nullptr);
	
	// 分配空间
	entry.start_block = allocate_blocks(entry.size);
	if(entry.start_block == (size_t)-1) {
		return false;
	}
	entry.block_count = (entry.size + block_size - 1) / block_size;
	
	// 写入数据
	if(!write_to_disk(entry.start_block * block_size, 
		content.c_str(), content.length())) {
		free_blocks(entry.start_block, entry.block_count);
		return false;
	}
	
	// 添加文件条目
	filesystem[mount_point].push_back(entry);
	
	// 更新分区使用空间
	for(auto& part : partitions) {
		if(part.mount_point == mount_point) {
			part.used_space += entry.block_count * block_size;
			break;
		}
	}
	
	return true;
}

bool DiskManager::create_directory(const std::string& dirname) {
	std::lock_guard<std::mutex> lock(disk_mutex);
	
	// 判断目录是否已存在
	std::string mount_point = "/";
	for(const auto& part : partitions) {
		if(dirname.find(part.mount_point) == 0 && 
			part.mount_point.length() > mount_point.length()) {
			mount_point = part.mount_point;
		}
	}
	
	for(const auto& entry : filesystem[mount_point]) {
		if(entry.name == dirname) {
			return false;  // 同名文件或目录已存在
		}
	}
	
	// 创建目录条目
	FileEntry entry;
	entry.name = dirname.substr(dirname.find_last_of("/") + 1);
	entry.type = "directory";
	entry.size = 0;
	entry.modified_time = std::time(nullptr);
	entry.start_block = 0;  // 目录不占用数据块
	entry.block_count = 0;
	
	// 添加目录条目
	filesystem[mount_point].push_back(entry);
	
	// 为新目录创建文件列表
	std::string full_path = mount_point;
	if(full_path != "/") full_path += "/";
	full_path += entry.name;
	filesystem[full_path] = std::vector<FileEntry>();
	
	return true;
}

bool DiskManager::delete_file(const std::string& filename) {
	std::lock_guard<std::mutex> lock(disk_mutex);
	
	// 查找文件所在分区
	std::string mount_point = "/";
	for(const auto& part : partitions) {
		if(filename.find(part.mount_point) == 0 && 
			part.mount_point.length() > mount_point.length()) {
			mount_point = part.mount_point;
		}
	}
	
	// 查找并删除文件
	auto& files = filesystem[mount_point];
	for(auto it = files.begin(); it != files.end(); ++it) {
		if(it->name == filename.substr(filename.find_last_of("/") + 1) && 
			it->type == "file") {
			// 释放文件占用的块
			free_blocks(it->start_block, it->block_count);
			
			// 更新分区使用空间
			for(auto& part : partitions) {
				if(part.mount_point == mount_point) {
					part.used_space -= it->block_count * block_size;
					break;
				}
			}
			
			// 删除文件条目
			files.erase(it);
			return true;
		}
	}
	
	return false;
}

bool DiskManager::delete_directory(const std::string& dirname) {
	std::lock_guard<std::mutex> lock(disk_mutex);
	
	// 查找目录所在分区
	std::string mount_point = "/";
	for(const auto& part : partitions) {
		if(dirname.find(part.mount_point) == 0 && 
			part.mount_point.length() > mount_point.length()) {
			mount_point = part.mount_point;
		}
	}
	
	// 构建完整路径
	std::string full_path = mount_point;
	if(full_path != "/") full_path += "/";
	full_path += dirname.substr(dirname.find_last_of("/") + 1);
	
	// 检查目录是否为空
	if(!filesystem[full_path].empty()) {
		return false;  // 不能删除非空目录
	}
	
	// 删除目录条目
	auto& entries = filesystem[mount_point];
	for(auto it = entries.begin(); it != entries.end(); ++it) {
		if(it->name == dirname.substr(dirname.find_last_of("/") + 1) && 
			it->type == "directory") {
			entries.erase(it);
			// 删除目录的文件列表
			filesystem.erase(full_path);
			return true;
		}
	}
	
	return false;
}

bool DiskManager::write_file(const std::string& filename, const std::string& content) {
	// 如果文件已存在，先删除它
	delete_file(filename);
	// 创建新文件
	return create_file(filename, content);
}

std::string DiskManager::read_file(const std::string& filename) {
	std::lock_guard<std::mutex> lock(disk_mutex);
	
	// 查找文件所在分区
	std::string mount_point = "/";
	for(const auto& part : partitions) {
		if(filename.find(part.mount_point) == 0 && 
			part.mount_point.length() > mount_point.length()) {
			mount_point = part.mount_point;
		}
	}
	
	// 查找文件
	const auto& files = filesystem[mount_point];
	for(const auto& entry : files) {
		if(entry.name == filename.substr(filename.find_last_of("/") + 1) && 
			entry.type == "file") {
			// 读取文件内容
			std::vector<char> buffer(entry.size);
			if(read_from_disk(entry.start_block * block_size, 
				buffer.data(), entry.size)) {
				return std::string(buffer.begin(), buffer.end());
			}
			break;
		}
	}
	
	return "";
}

std::vector<DiskManager::PartitionInfo> DiskManager::list_partitions() const {
	std::vector<PartitionInfo> info_list;
	
	for(const auto& part : partitions) {
		PartitionInfo info;
		info.name = part.name;
		info.size = part.size;
		info.used_space = part.used_space;
		info.mount_point = part.mount_point;
		info.is_mounted = part.is_mounted;
		info_list.push_back(info);
	}
	
	return info_list;
}

DiskManager::PartitionInfo DiskManager::get_partition_info(const std::string& name) const {
	for(const auto& part : partitions) {
		if(part.name == name) {
			PartitionInfo info;
			info.name = part.name;
			info.size = part.size;
			info.used_space = part.used_space;
			info.mount_point = part.mount_point;
			info.is_mounted = part.is_mounted;
			return info;
		}
	}
	
	return PartitionInfo();
}

size_t DiskManager::get_free_space() const {
	size_t free_space = 0;
	for(const auto& block : block_map) {
		if(block.is_free) {
			free_space += block.size;
		}
	}
	return free_space;
}

size_t DiskManager::get_used_space() const {
	return total_size - get_free_space();
}

size_t DiskManager::get_total_space() const {
	return total_size;
}
