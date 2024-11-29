#ifndef USER_AUTH_H
#define USER_AUTH_H

#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <ctime>

class UserAuth {
public:
    struct User {
        std::string username;
        std::string password_hash;
        std::vector<std::string> permissions;
        bool is_admin;
        time_t last_login;
        time_t creation_time;

        // 默认构造函数
        User() = default;

        // 参数化构造函数
        User(std::string u, std::string p)
            : username(std::move(u)), password_hash(std::move(p)), is_admin(false) {
            creation_time = time(nullptr);
            last_login = 0;
        }
    };

    // 用户信息结构体
    struct UserInfo {
        std::string username;
        bool is_admin;
        time_t last_login;
        time_t creation_time;
    };

private:
    std::map<std::string, User> users;
    std::string current_user;
    mutable std::mutex auth_mutex;

public:
    UserAuth();
    ~UserAuth();

    bool login(const std::string& username, const std::string& password);
    void logout();
    bool is_authenticated() const;
    bool is_admin() const;
    std::string get_current_user() const;

    bool has_permission(const std::string& permission);
    bool add_permission(const std::string& username, const std::string& permission);
    bool remove_permission(const std::string& username, const std::string& permission);

    bool add_user(const std::string& username, const std::string& password);
    bool remove_user(const std::string& username);
    bool change_password(const std::string& username, const std::string& old_password, const std::string& new_password);

    std::string hash_password(const std::string& password);
    bool verify_password(const std::string& password, const std::string& hash);

    UserInfo get_user_info(const std::string& username) const;
    std::vector<UserInfo> list_users() const;
};

#endif // USER_AUTH_H
