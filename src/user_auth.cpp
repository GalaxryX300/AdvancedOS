#include "user_auth.h"
#include <openssl/evp.h>
#include <algorithm>
#include <stdexcept>
#include <iostream>

UserAuth::UserAuth() = default;
UserAuth::~UserAuth() = default;

std::string UserAuth::hash_password(const std::string& password) {
	EVP_MD_CTX* context = EVP_MD_CTX_new();
	unsigned char hash[EVP_MAX_MD_SIZE];
	unsigned int hash_length = 0;
	
	if (context == nullptr ||
		EVP_DigestInit_ex(context, EVP_sha256(), nullptr) != 1 ||
		EVP_DigestUpdate(context, password.c_str(), password.size()) != 1 ||
		EVP_DigestFinal_ex(context, hash, &hash_length) != 1) {
		EVP_MD_CTX_free(context);
		throw std::runtime_error("Error hashing password");
	}
	
	EVP_MD_CTX_free(context);
	return std::string(reinterpret_cast<char*>(hash), hash_length);
}

bool UserAuth::verify_password(const std::string& password, const std::string& hash) {
	return hash_password(password) == hash;
}

bool UserAuth::login(const std::string& username, const std::string& password) {
	std::lock_guard<std::mutex> lock(auth_mutex);
	
	auto it = users.find(username);
	if (it == users.end()) {
		return false;
	}
	
	if (verify_password(password, it->second.password_hash)) {
		current_user = username;
		it->second.last_login = time(nullptr);
		return true;
	}
	
	return false;
}

void UserAuth::logout() {
	std::lock_guard<std::mutex> lock(auth_mutex);
	current_user.clear();
}

bool UserAuth::is_authenticated() const {
	std::lock_guard<std::mutex> lock(auth_mutex);
	return !current_user.empty();
}

bool UserAuth::is_admin() const {
	std::lock_guard<std::mutex> lock(auth_mutex);
	
	auto it = users.find(current_user);
	if (it == users.end()) {
		return false;
	}
	
	return it->second.is_admin;
}

std::string UserAuth::get_current_user() const {
	std::lock_guard<std::mutex> lock(auth_mutex);
	return current_user;
}

bool UserAuth::has_permission(const std::string& permission) {
	std::lock_guard<std::mutex> lock(auth_mutex);
	
	auto it = users.find(current_user);
	if (it == users.end() || !is_authenticated()) {
		return false;
	}
	
	if (it->second.is_admin) {
		return true;
	}
	
	const auto& perms = it->second.permissions;
	return std::find(perms.begin(), perms.end(), permission) != perms.end();
}

bool UserAuth::add_permission(const std::string& username, const std::string& permission) {
	std::lock_guard<std::mutex> lock(auth_mutex);
	
	auto it = users.find(username);
	if (it == users.end()) {
		return false;
	}
	
	auto& perms = it->second.permissions;
	if (std::find(perms.begin(), perms.end(), permission) == perms.end()) {
		perms.push_back(permission);
		return true;
	}
	
	return false;
}

bool UserAuth::remove_permission(const std::string& username, const std::string& permission) {
	std::lock_guard<std::mutex> lock(auth_mutex);
	
	auto it = users.find(username);
	if (it == users.end()) {
		return false;
	}
	
	auto& perms = it->second.permissions;
	auto perm_it = std::find(perms.begin(), perms.end(), permission);
	if (perm_it != perms.end()) {
		perms.erase(perm_it);
		return true;
	}
	
	return false;
}

bool UserAuth::add_user(const std::string& username, const std::string& password) {
	std::lock_guard<std::mutex> lock(auth_mutex);
	
	if (users.find(username) != users.end()) {
		return false;
	}
	
	users.emplace(username, User(username, hash_password(password)));
	return true;
}

bool UserAuth::remove_user(const std::string& username) {
	std::lock_guard<std::mutex> lock(auth_mutex);
	
	auto it = users.find(username);
	if (it == users.end()) {
		return false;
	}
	
	users.erase(it);
	if (current_user == username) {
		current_user.clear();
	}
	
	return true;
}

bool UserAuth::change_password(const std::string& username, const std::string& old_password, const std::string& new_password) {
	std::lock_guard<std::mutex> lock(auth_mutex);
	
	auto it = users.find(username);
	if (it == users.end() || !verify_password(old_password, it->second.password_hash)) {
		return false;
	}
	
	it->second.password_hash = hash_password(new_password);
	return true;
}

UserAuth::UserInfo UserAuth::get_user_info(const std::string& username) const {
	std::lock_guard<std::mutex> lock(auth_mutex);
	
	auto it = users.find(username);
	if (it == users.end()) {
		throw std::runtime_error("User not found");
	}
	
	const auto& user = it->second;
	return {user.username, user.is_admin, user.last_login, user.creation_time};
}

std::vector<UserAuth::UserInfo> UserAuth::list_users() const {
	std::lock_guard<std::mutex> lock(auth_mutex);
	
	std::vector<UserInfo> user_list;
	for (const auto& [username, user] : users) {
		user_list.push_back({user.username, user.is_admin, user.last_login, user.creation_time});
	}
	
	return user_list;
}

