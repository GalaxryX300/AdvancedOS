// filesystem.cpp
#include "filesystem.h"
#include "memory.h"
#include <cstring>
#include <algorithm>

FileSystem::FileSystem() : mounted(false) {
	superblock.magic = 0x4D465331;  // "MFS1"
	superblock.block_size = BLOCK_SIZE;
}

FileSystem::~FileSystem() {
	if(mounted) {
		unmount();
	}
}

bool FileSystem::mount(const std::string& device, const std::string& mount_point) {
	if(mounted) {
		return false;
	}
	
	// 读取超级块
	// TODO: 实际实现需要从设备读取
	
	// 初始化位图和文件表
	block_bitmap.resize(superblock.total_blocks, false);
	
	this->mount_point = mount_point;
	mounted = true;
	return true;
}

bool FileSystem::unmount() {
	if(!mounted) {
		return false;
	}
	
	// 保存文件系统状态
	// TODO: 实际实现需要写回设备
	
	mounted = false;
	return true;
}

bool FileSystem::format(uint32_t size) {
	if(mounted) {
		return false;
	}
	
	// 计算块数量
	superblock.total_blocks = size / BLOCK_SIZE;
	superblock.free_blocks = superblock.total_blocks - 1;  // 减去超级块
	superblock.first_data_block = 1;
	
	// 初始化位图
	block_bitmap.clear();
	block_bitmap.resize(superblock.total_blocks, true);
	block_bitmap[0] = false;  // 超级块已使用
	
	// 清空文件表
	file_table.clear();
	
	// 创建根目录
	FileEntry root;
	strcpy(root.name, "/");
	root.size = 0;
	root.block_count = 0;
	root.is_directory = true;
	root.permissions = 0755;
	root.created_time = root.modified_time = root.accessed_time = time(nullptr);
	file_table.push_back(root);
	
	return true;
}

uint32_t FileSystem::allocate_block() {
	for(uint32_t i = 0; i < block_bitmap.size(); i++) {
		if(block_bitmap[i]) {
			block_bitmap[i] = false;
			superblock.free_blocks--;
			return i;
		}
	}
	return (uint32_t)-1;
}

void FileSystem::free_block(uint32_t block_number) {
	if(block_number < block_bitmap.size() && !block_bitmap[block_number]) {
		block_bitmap[block_number] = true;
		superblock.free_blocks++;
	}
}

bool FileSystem::create_file(const std::string& path) {
	if(!mounted) return false;
	
	// 检查文件是否已存在
	if(find_file(path)) return false;
	
	// 创建新文件条目
	FileEntry file;
	std::string filename = get_filename(path);
	if(filename.length() >= MAX_FILENAME) return false;
	
	strcpy(file.name, filename.c_str());
	file.size = 0;
	file.block_count = 0;
	file.is_directory = false;
	file.permissions = 0644;
	file.created_time = file.modified_time = file.accessed_time = time(nullptr);
	
	// 添加到文件表
	file_table.push_back(file);
	return true;
}

bool FileSystem::write_file(const std::string& path, const void* data, size_t size) {
	if(!mounted) return false;
	
	FileEntry* file = find_file(path);
	if(!file || file->is_directory) return false;
	
	// 释放原有块
	for(uint32_t i = 0; i < file->block_count; i++) {
		free_block(file->blocks[i]);
	}
	
	// 分配新块
	uint32_t blocks_needed = (size + BLOCK_SIZE - 1) / BLOCK_SIZE;
	if(blocks_needed > MAX_BLOCKS_PER_FILE) return false;
	
	file->block_count = 0;
	for(uint32_t i = 0; i < blocks_needed; i++) {
		uint32_t new_block = allocate_block();
		if(new_block == (uint32_t)-1) {
			// 分配失败，回滚
			for(uint32_t j = 0; j < file->block_count; j++) {
				free_block(file->blocks[j]);
			}
			file->block_count = 0;
			file->size = 0;
			return false;
		}
		file->blocks[file->block_count++] = new_block;
	}
	
	// 写入数据
	// TODO: 实际实现需要写入设备
	
	file->size = size;
	file->modified_time = time(nullptr);
	return true;
}

bool FileSystem::read_file(const std::string& path, void* buffer, size_t size) {
	if(!mounted) return false;
	
	FileEntry* file = find_file(path);
	if(!file || file->is_directory) return false;
	
	// 读取数据
	// TODO: 实际实现需要从设备读取
	
	file->accessed_time = time(nullptr);
	return true;
}

FileEntry* FileSystem::find_file(const std::string& path) {
	if(path == "/" || path.empty()) return &file_table[0];  // 返回根目录
	
	std::string filename = get_filename(path);
	for(auto& entry : file_table) {
		if(strcmp(entry.name, filename.c_str()) == 0) {
			return &entry;
		}
	}
	return nullptr;
}

// 新增函数实现
bool FileSystem::delete_file(const std::string& path) {
	if(!mounted) return false;
	
	// 不允许删除根目录
	if(path == "/" || path.empty()) return false;
	
	// 查找文件
	auto it = std::find_if(file_table.begin(), file_table.end(),
		[&](const FileEntry& entry) {
			return strcmp(entry.name, get_filename(path).c_str()) == 0;
		});
	
	if(it == file_table.end()) return false;
	
	// 如果是非空目录，不允许删除
	if(it->is_directory) {
		for(const auto& entry : file_table) {
			if(strcmp(entry.name, it->name) != 0) {
				std::string full_path = std::string(it->name) + "/" + entry.name;
				if(find_file(full_path)) return false;
			}
		}
	}
	
	// 释放文件占用的所有块
	for(uint32_t i = 0; i < it->block_count; i++) {
		free_block(it->blocks[i]);
	}
	
	// 从文件表中移除
	file_table.erase(it);
	return true;
}

bool FileSystem::create_directory(const std::string& path) {
	if(!mounted) return false;
	
	// 检查目录是否已存在
	if(find_file(path)) return false;
	
	// 创建新目录条目
	FileEntry dir;
	std::string dirname = get_filename(path);
	if(dirname.length() >= MAX_FILENAME) return false;
	
	strcpy(dir.name, dirname.c_str());
	dir.size = 0;
	dir.block_count = 0;
	dir.is_directory = true;
	dir.permissions = 0755;
	dir.created_time = dir.modified_time = dir.accessed_time = time(nullptr);
	
	// 添加到文件表
	file_table.push_back(dir);
	return true;
}

bool FileSystem::is_directory(const std::string& path) {
	if(!mounted) return false;
	
	FileEntry* entry = find_file(path);
	return entry && entry->is_directory;
}

std::vector<std::string> FileSystem::list_directory(const std::string& path) {
	std::vector<std::string> files;
	if(!mounted) return files;
	
	// 检查路径是否是目录
	FileEntry* dir = find_file(path);
	if(!dir || !dir->is_directory) return files;
	
	// 收集目录下的所有文件和子目录
	for(const auto& entry : file_table) {
		// 先添加当前目录和父目录
		if(path == "/") {
			files.push_back(".");
			files.push_back("..");
		}
		
		// 在实际实现中，这里需要考虑路径的层次结构
		// 当前简化版本只列出直接子文件和子目录
		if(strcmp(entry.name, dir->name) != 0) {  // 不包含目录自身
			std::string file_name = entry.name;
			if(entry.is_directory) {
				file_name += "/";
			}
			files.push_back(file_name);
		}
	}
	
	return files;
}
