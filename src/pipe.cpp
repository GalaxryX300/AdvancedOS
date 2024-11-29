// pipe.cpp
#include "pipe.h"
#include <cstring>

Pipe::Pipe() : read_pos(0), write_pos(0), closed(false),
reader_pid(-1), writer_pid(-1) {
	memset(buffer, 0, PIPE_BUFFER_SIZE);
}

Pipe::~Pipe() {
	close();
}

ssize_t Pipe::read(void* buf, size_t count) {
	std::lock_guard<std::mutex> lock(pipe_mutex);
	
	if(closed) return 0;
	
	// 计算可读数据量
	size_t available = (write_pos >= read_pos) ?
	write_pos - read_pos :
	PIPE_BUFFER_SIZE - (read_pos - write_pos);
	
	if(available == 0) return 0;
	
	// 读取数据
	size_t to_read = std::min(count, available);
	size_t first_chunk = std::min(to_read, PIPE_BUFFER_SIZE - read_pos);
	size_t second_chunk = to_read - first_chunk;
	
	// 复制第一块
	memcpy(buf, buffer + read_pos, first_chunk);
	read_pos = (read_pos + first_chunk) % PIPE_BUFFER_SIZE;
	
	// 如果需要，复制第二块
	if(second_chunk > 0) {
		memcpy((char*)buf + first_chunk, buffer, second_chunk);
		read_pos = second_chunk;
	}
	
	return to_read;
}

ssize_t Pipe::write(const void* buf, size_t count) {
	std::lock_guard<std::mutex> lock(pipe_mutex);
	
	if(closed) return -1;
	
	// 计算可写空间
	size_t available = (read_pos > write_pos) ?
	read_pos - write_pos - 1 :
	PIPE_BUFFER_SIZE - (write_pos - read_pos) - 1;
	
	if(available == 0) return 0;
	
	// 写入数据
	size_t to_write = std::min(count, available);
	size_t first_chunk = std::min(to_write, PIPE_BUFFER_SIZE - write_pos);
	size_t second_chunk = to_write - first_chunk;
	
	// 写入第一块
	memcpy(buffer + write_pos, buf, first_chunk);
	write_pos = (write_pos + first_chunk) % PIPE_BUFFER_SIZE;
	
	// 如果需要，写入第二块
	if(second_chunk > 0) {
		memcpy(buffer, (const char*)buf + first_chunk, second_chunk);
		write_pos = second_chunk;
	}
	
	return to_write;
}

void Pipe::close() {
	std::lock_guard<std::mutex> lock(pipe_mutex);
	closed = true;
}

bool Pipe::is_closed() const {
	return closed;
}

// PipeManager实现
PipeManager::PipeManager() {}

PipeManager::~PipeManager() {
	for(auto pipe : pipes) {
		delete pipe;
	}
	pipes.clear();
}

Pipe* PipeManager::create_pipe() {
	std::lock_guard<std::mutex> lock(manager_mutex);
	
	Pipe* pipe = new Pipe();
	pipes.push_back(pipe);
	return pipe;
}

void PipeManager::destroy_pipe(Pipe* pipe) {
	std::lock_guard<std::mutex> lock(manager_mutex);
	
	auto it = std::find(pipes.begin(), pipes.end(), pipe);
	if(it != pipes.end()) {
		pipes.erase(it);
		delete pipe;
	}
}

void PipeManager::cleanup_process_pipes(pid_t pid) {
	std::lock_guard<std::mutex> lock(manager_mutex);
	
	for(auto it = pipes.begin(); it != pipes.end();) {
	Pipe* pipe = *it;
		if(pipe->get_reader() == pid || pipe->get_writer() == pid) {
			pipe->close();
			delete pipe;
			it = pipes.erase(it);
		} else {
			++it;
		}
}
}

// 新增管道命令处理功能
class PipelineCommand {
public:
	struct Command {
		std::string name;
		std::vector<std::string> args;
		Pipe* input_pipe;
		Pipe* output_pipe;
	};
	
	static std::vector<Command> parse_pipeline(const std::string& cmdline) {
		std::vector<Command> pipeline;
		std::stringstream ss(cmdline);
		std::string segment;
		bool in_quotes = false;
		char quote_char = 0;
		
		Command current_cmd;
		std::string current_arg;
		
		auto finish_arg = [&]() {
			if(!current_arg.empty()) {
				if(current_cmd.name.empty()) {
					current_cmd.name = current_arg;
				} else {
					current_cmd.args.push_back(current_arg);
				}
				current_arg.clear();
			}
		};
		
		auto finish_command = [&]() {
			finish_arg();
			if(!current_cmd.name.empty()) {
				pipeline.push_back(std::move(current_cmd));
				current_cmd = Command();
			}
		};
		
		char c;
		while(ss.get(c)) {
			if(c == '"' || c == '\'') {
				if(!in_quotes) {
					in_quotes = true;
					quote_char = c;
				} else if(c == quote_char) {
					in_quotes = false;
					quote_char = 0;
				} else {
					current_arg += c;
				}
			} else if(c == '|' && !in_quotes) {
				finish_command();
			} else if(std::isspace(c) && !in_quotes) {
				finish_arg();
			} else {
				current_arg += c;
			}
		}
		
		finish_command();
		return pipeline;
	}
	
	static void execute_pipeline(const std::vector<Command>& pipeline) {
		if(pipeline.empty()) return;
		
		std::vector<Pipe*> pipes;
		PipeManager& pipe_manager = PipeManager::get_instance();
		
		// 创建管道
		for(size_t i = 0; i < pipeline.size() - 1; ++i) {
			pipes.push_back(pipe_manager.create_pipe());
		}
		
		// 创建进程执行每个命令
		std::vector<pid_t> child_pids;
		for(size_t i = 0; i < pipeline.size(); ++i) {
			const Command& cmd = pipeline[i];
			
			pid_t pid = create_process([&]() {
				// 设置输入管道
				if(i > 0) {
					dup2(pipes[i-1]->get_read_fd(), STDIN_FILENO);
				}
				
				// 设置输出管道
				if(i < pipeline.size() - 1) {
					dup2(pipes[i]->get_write_fd(), STDOUT_FILENO);
				}
				
				// 关闭所有管道
				for(auto pipe : pipes) {
					pipe->close();
				}
				
				// 执行命令
				execute_command(cmd.name, cmd.args);
				exit(0);
			});
			
			if(pid > 0) {
				child_pids.push_back(pid);
			}
		}
		
		// 父进程关闭所有管道
		for(auto pipe : pipes) {
			pipe->close();
		}
		
		// 等待所有子进程结束
		for(pid_t pid : child_pids) {
			wait_process(pid);
		}
		
		// 清理管道
		for(auto pipe : pipes) {
			pipe_manager.destroy_pipe(pipe);
		}
	}
	
private:
	static void execute_command(const std::string& name, 
		const std::vector<std::string>& args) {
			// 查找并执行命令
			auto cmd_it = CLI::get_instance().find_command(name);
			if(cmd_it != nullptr) {
				cmd_it->handler(args);
			} else {
				kprintf("Command not found: %s\n", name.c_str());
			}
		}
};

// 在CLI类中添加管道支持
void CLI::execute_pipeline(const std::string& cmdline) {
	auto pipeline = PipelineCommand::parse_pipeline(cmdline);
	if(!pipeline.empty()) {
		PipelineCommand::execute_pipeline(pipeline);
	}
}
