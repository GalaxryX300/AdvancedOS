// include/ipc.h
#ifndef IPC_H
#define IPC_H

#include <string>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <map>
#include <vector>
#include "types.h"

class Message {
public:
    std::string content;
    pid_t sender;
    pid_t receiver;
    
    Message(const std::string& c, pid_t s, pid_t r)
        : content(c), sender(s), receiver(r) {}
};

class MessageQueue {
public:
    void send(const std::string& message, pid_t receiver);
    std::string receive(pid_t receiver);
    size_t size() const;

private:
    mutable std::mutex queue_mutex;
    std::condition_variable cv;
    std::queue<Message> messages;
};

class SharedMemory {
public:
    explicit SharedMemory(size_t size);
    ~SharedMemory();
    
    bool attach(pid_t pid);
    bool detach(pid_t pid);
    bool is_attached(pid_t pid) const;
    
    void* get_memory() { return memory; }
    size_t get_size() const { return memory_size; }

private:
    void* memory;
    size_t memory_size;
    mutable std::mutex shm_mutex;
    std::vector<pid_t> attached_processes;
};

class IPCManager {
public:
    MessageQueue* create_message_queue(const std::string& name);
    SharedMemory* create_shared_memory(const std::string& name, size_t size);
    void destroy_message_queue(const std::string& name);
    void destroy_shared_memory(const std::string& name);
    
    MessageQueue* get_message_queue(const std::string& name);
    SharedMemory* get_shared_memory(const std::string& name);

private:
    std::map<std::string, MessageQueue*> message_queues;
    std::map<std::string, SharedMemory*> shared_memories;
    std::mutex ipc_mutex;
};

#endif // IPC_H