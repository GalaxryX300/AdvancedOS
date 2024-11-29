#ifndef CLI_H
#define CLI_H

#include <string>
#include <vector>
#include <map>
#include <functional>
#include "filesystem.h"

// 命令行最大长度
#define MAX_COMMAND_LENGTH 256
#define MAX_ARGS 16
#define MAX_COMMAND_HISTORY 100

// 命令结构
struct Command {
    std::string name;
    std::string description;
    std::function<void(const std::vector<std::string>&)> handler;
};

class CLI {
public:
    CLI();
    void init();
    void run();

private:
    std::string current_dir;
    std::string prompt;
    std::string command_buffer;
    std::vector<std::string> command_history;
    std::map<std::string, Command> commands;
    int history_index;
    FileSystem filesystem;

    void register_command(const Command& cmd);
    void execute_command(const std::string& cmd_line);
    std::vector<std::string> parse_command(const std::string& cmd_line);
    void handle_backspace();
    void handle_tab_completion();
    void add_to_history(const std::string& cmd);
    std::string get_previous_command();
    std::string get_next_command();
    std::vector<std::string> get_command_suggestions(const std::string& prefix);
};

#endif // CLI_H