// pipe.h
#ifndef PIPE_H
#define PIPE_H

#include "types.h"
#include <queue>
#include <mutex>

#define PIPE_BUFFER_SIZE 4096

class Pipe {
private:
    char buffer[PIPE_BUFFER_SIZE];
    size_t read_pos;
    size_t write_pos;
    bool closed;
    std::mutex pipe_mutex;
    
    // 读写进程ID
    pid_t reader_pid;
    pid_t writer_pid;

public:
    Pipe();
    ~Pipe();

    // 基本操作
    ssize_t read(void* buf, size_t count);
    ssize_t write(const void* buf, size_t count);
    void close();
    bool is_closed() const;

    // 进程关联
    void set_reader(pid_t pid) { reader_pid = pid; }
    void set_writer(pid_t pid) { writer_pid = pid; }
    pid_t get_reader() const { return reader_pid; }
    pid_t get_writer() const { return writer_pid; }
};

// 管道管理器
class PipeManager {
private:
    std::vector<Pipe*> pipes;
    std::mutex manager_mutex;

public:
    PipeManager();
    ~PipeManager();

    // 管道创建和管理
    Pipe* create_pipe();
    void destroy_pipe(Pipe* pipe);
    void cleanup_process_pipes(pid_t pid);
};

#endif // PIPE_H