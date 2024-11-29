// src/ipc.cpp
#include <cstdlib>
#include "ipc.h"
#include <cstring>
#include <iostream>
#include <algorithm>  // 为 std::find
#include <unistd.h>  // 为 getpid()

// MessageQueue 实现
void MessageQueue::send(const std::string& message, pid_t receiver) {
	std::lock_guard<std::mutex> lock(queue_mutex);
	messages.push(Message(message, ::getpid(), receiver));
	cv.notify_one();
}

std::string MessageQueue::receive(pid_t receiver) {
	std::unique_lock<std::mutex> lock(queue_mutex);
	cv.wait(lock, [this, receiver]() {
		return !messages.empty() && messages.front().receiver == receiver;
	});
	
	std::string msg = messages.front().content;
	messages.pop();
	return msg;
}

size_t MessageQueue::size() const {
	std::lock_guard<std::mutex> lock(queue_mutex);
	return messages.size();
}

// SharedMemory 实现
SharedMemory::SharedMemory(size_t size) : memory_size(size) {
	memory = malloc(size);
	if (memory) {
		memset(memory, 0, size);
	}
}

SharedMemory::~SharedMemory() {
	if (memory) {
		free(memory);
		memory = nullptr;
	}
}

bool SharedMemory::attach(pid_t pid) {
	std::lock_guard<std::mutex> lock(shm_mutex);
	auto it = std::find(attached_processes.begin(), attached_processes.end(), pid);
	if (it == attached_processes.end()) {
		attached_processes.push_back(pid);
		return true;
	}
	return false;
}

bool SharedMemory::detach(pid_t pid) {
	std::lock_guard<std::mutex> lock(shm_mutex);
	auto it = std::find(attached_processes.begin(), attached_processes.end(), pid);
	if (it != attached_processes.end()) {
		attached_processes.erase(it);
		return true;
	}
	return false;
}

bool SharedMemory::is_attached(pid_t pid) const {
	std::lock_guard<std::mutex> lock(shm_mutex);
	return std::find(attached_processes.begin(), attached_processes.end(), pid) 
	!= attached_processes.end();
}

// IPCManager 实现
MessageQueue* IPCManager::create_message_queue(const std::string& name) {
	std::lock_guard<std::mutex> lock(ipc_mutex);
	auto it = message_queues.find(name);
	if (it != message_queues.end()) {
		return nullptr;
	}
	
	auto queue = new MessageQueue();
	message_queues[name] = queue;
	return queue;
}

SharedMemory* IPCManager::create_shared_memory(const std::string& name, size_t size) {
	std::lock_guard<std::mutex> lock(ipc_mutex);
	auto it = shared_memories.find(name);
	if (it != shared_memories.end()) {
		return nullptr;
	}
	
	auto shm = new SharedMemory(size);
	shared_memories[name] = shm;
	return shm;
}

void IPCManager::destroy_message_queue(const std::string& name) {
	std::lock_guard<std::mutex> lock(ipc_mutex);
	auto it = message_queues.find(name);
	if (it != message_queues.end()) {
		delete it->second;
		message_queues.erase(it);
	}
}

void IPCManager::destroy_shared_memory(const std::string& name) {
	std::lock_guard<std::mutex> lock(ipc_mutex);
	auto it = shared_memories.find(name);
	if (it != shared_memories.end()) {
		delete it->second;
		shared_memories.erase(it);
	}
}

MessageQueue* IPCManager::get_message_queue(const std::string& name) {
	std::lock_guard<std::mutex> lock(ipc_mutex);
	auto it = message_queues.find(name);
	return it != message_queues.end() ? it->second : nullptr;
}

SharedMemory* IPCManager::get_shared_memory(const std::string& name) {
	std::lock_guard<std::mutex> lock(ipc_mutex);
	auto it = shared_memories.find(name);
	return it != shared_memories.end() ? it->second : nullptr;
}
