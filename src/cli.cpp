// cli.cpp
#include "cli.h"
#include "screen.h"
#include "keyboard.h"
#include <algorithm>
#include <sstream>

CLI::CLI() : history_index(0), current_dir("/") {
	prompt = "$ ";
}

void CLI::init() {
	// 注册基本命令
	register_command({"help", "显示帮助信息", [this](const auto& args) {
		kprintf("可用命令:\n");
		for(const auto& cmd : commands) {
			kprintf("  %-10s - %s\n", cmd.first.c_str(), cmd.second.description.c_str());
		}
	}});
	
	register_command({"ls", "列出目录内容", [this](const auto& args) {
		std::string path = args.size() > 1 ? args[1] : current_dir;
		auto files = filesystem.list_directory(path);
		for(const auto& file : files) {
			kprintf("%s  ", file.c_str());
		}
		kprintf("\n");
	}});
	
	register_command({"cd", "切换目录", [this](const auto& args) {
		if(args.size() < 2) {
			kprintf("用法: cd <目录>\n");
			return;
		}
		std::string new_dir = args[1];
		if(filesystem.is_directory(new_dir)) {
			current_dir = new_dir;
		} else {
			kprintf("错误: 目录不存在\n");
		}
	}});
	
	register_command({"cat", "显示文件内容", [this](const auto& args) {
		if(args.size() < 2) {
			kprintf("用法: cat <文件名>\n");
			return;
		}
		char buffer[1024];
		if(filesystem.read_file(args[1], buffer, sizeof(buffer))) {
			kprintf("%s", buffer);
		} else {
			kprintf("错误: 无法读取文件\n");
		}
	}});
	
	register_command({"clear", "清屏", [this](const auto& args) {
		clear_screen();
	}});
	
	register_command({"mkdir", "创建目录", [this](const auto& args) {
		if(args.size() < 2) {
			kprintf("用法: mkdir <目录名>\n");
			return;
		}
		if(filesystem.create_directory(args[1])) {
			kprintf("目录已创建\n");
		} else {
			kprintf("错误: 无法创建目录\n");
		}
	}});
	
	register_command({"touch", "创建空文件", [this](const auto& args) {
		if(args.size() < 2) {
			kprintf("用法: touch <文件名>\n");
			return;
		}
		if(filesystem.create_file(args[1])) {
			kprintf("文件已创建\n");
		} else {
			kprintf("错误: 无法创建文件\n");
		}
	}});
	
	register_command({"rm", "删除文件或目录", [this](const auto& args) {
		if(args.size() < 2) {
			kprintf("用法: rm <文件名>\n");
			return;
		}
		if(filesystem.delete_file(args[1])) {
			kprintf("文件已删除\n");
		} else {
			kprintf("错误: 无法删除文件\n");
		}
	}});
	
	// 清屏并显示欢迎信息
	clear_screen();
	kprintf("Simple OS Command Line Interface\n");
	kprintf("输入 'help' 查看可用命令\n\n");
}

void CLI::run() {
	while(true) {
		// 显示提示符
		kprintf("%s%s", current_dir.c_str(), prompt.c_str());
		
		// 读取命令
		command_buffer.clear();
		bool running = true;
		while(running) {
			char c = keyboard_getchar();
			if(c == 0) continue;
			
			switch(c) {
			case '\n':
				kprintf("\n");
				if(!command_buffer.empty()) {
					add_to_history(command_buffer);
					execute_command(command_buffer);
				}
				running = false;
				break;
				
			case '\b':
				handle_backspace();
				break;
				
			case '\t':
				handle_tab_completion();
				break;
				
			default:
				if(command_buffer.length() < MAX_COMMAND_LENGTH) {
					command_buffer += c;
					putchar(c);
				}
				break;
			}
		}
	}
}

void CLI::register_command(const Command& cmd) {
	commands[cmd.name] = cmd;
}

void CLI::execute_command(const std::string& cmd_line) {
	auto args = parse_command(cmd_line);
	if(args.empty()) return;
	
	auto it = commands.find(args[0]);
	if(it != commands.end()) {
		it->second.handler(args);
	} else {
		kprintf("错误: 未知命令 '%s'\n", args[0].c_str());
	}
}

std::vector<std::string> CLI::parse_command(const std::string& cmd_line) {
	std::vector<std::string> args;
	std::stringstream ss(cmd_line);
	std::string arg;
	
	while(ss >> arg) {
		if(args.size() < MAX_ARGS) {
			args.push_back(arg);
		}
	}
	
	return args;
}

void CLI::handle_backspace() {
	if(!command_buffer.empty()) {
		command_buffer.pop_back();
		kprintf("\b \b");
	}
}

void CLI::handle_tab_completion() {
	auto suggestions = get_command_suggestions(command_buffer);
	if(suggestions.empty()) return;
	
	if(suggestions.size() == 1) {
		// 自动完成唯一匹配
		command_buffer = suggestions[0];
		kprintf("\r%s%s%s", 
			current_dir.c_str(), 
			prompt.c_str(), 
			command_buffer.c_str());
	} else {
		// 显示所有可能的补全
		kprintf("\n");
		for(const auto& sugg : suggestions) {
			kprintf("%s  ", sugg.c_str());
		}
		kprintf("\n%s%s%s", 
			current_dir.c_str(), 
			prompt.c_str(), 
			command_buffer.c_str());
	}
}

void CLI::add_to_history(const std::string& cmd) {
	if(command_history.size() >= MAX_COMMAND_HISTORY) {
		command_history.erase(command_history.begin());
	}
	command_history.push_back(cmd);
	history_index = command_history.size();
}

std::string CLI::get_previous_command() {
	if(history_index > 0) {
		history_index--;
		return command_history[history_index];
	}
	return command_buffer;
}

std::string CLI::get_next_command() {
	if(history_index < command_history.size() - 1) {
		history_index++;
		return command_history[history_index];
	}
	return "";
}

std::vector<std::string> CLI::get_command_suggestions(const std::string& prefix) {
	std::vector<std::string> suggestions;
	
	// 遍历所有已注册的命令，找到匹配前缀的命令
	for(const auto& cmd : commands) {
		if(cmd.first.substr(0, prefix.length()) == prefix) {
			suggestions.push_back(cmd.first);
		}
	}
	
	// 如果是空前缀，返回所有命令
	if(prefix.empty()) {
		for(const auto& cmd : commands) {
			suggestions.push_back(cmd.first);
		}
	}
	
	return suggestions;
}
