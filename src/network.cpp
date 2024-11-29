// network.cpp - 网络管理实现
#include "../include/network.h"
#include <unistd.h>
#include <cstring>
#include <iostream>

NetworkManager::NetworkManager() 
: server_socket(-1), running(false),
gen(rd()),
sent_dist(50.0, 20.0),   // 均值50KB，标准差20KB
recv_dist(30.0, 15.0),   // 均值30KB，标准差15KB
last_bytes_sent(0),
last_bytes_received(0) {
	// 添加一个模拟客户端
	auto client = std::make_shared<ClientInfo>(1, "192.168.1.100");
	clients[1] = client;
}

NetworkManager::~NetworkManager() {
    stop_server();
}

bool NetworkManager::start_server(int port) {
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if(server_socket < 0) {
        return false;
    }
    
    // 设置socket选项
    int opt = 1;
    if(setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, 
                  &opt, sizeof(opt)) < 0) {
        close(server_socket);
        return false;
    }
    
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);
    
    if(bind(server_socket, (struct sockaddr*)&server_addr, 
            sizeof(server_addr)) < 0) {
        close(server_socket);
        return false;
    }
    
    if(listen(server_socket, 5) < 0) {
        close(server_socket);
        return false;
    }
    
    running = true;
    server_thread = std::thread(&NetworkManager::server_loop, this);
    
    return true;
}

void NetworkManager::stop_server() {
    running = false;
    if(server_socket != -1) {
        close(server_socket);
        server_socket = -1;
    }
    
    if(server_thread.joinable()) {
        server_thread.join();
    }
    
    // 关闭所有客户端连接
    std::lock_guard<std::mutex> lock(network_mutex);
    for(auto& client : clients) {
        close(client.second->socket_fd);
    }
    clients.clear();
}

void NetworkManager::server_loop() {
    while(running) {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(server_socket, &read_fds);
        
        struct timeval timeout;
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        
        if(select(server_socket + 1, &read_fds, NULL, NULL, &timeout) > 0) {
            struct sockaddr_in client_addr;
            socklen_t client_len = sizeof(client_addr);
            
            int client_socket = accept(server_socket, 
                                     (struct sockaddr*)&client_addr,
                                     &client_len);
                                     
            if(client_socket >= 0) {
                std::lock_guard<std::mutex> lock(network_mutex);
                auto client_info = std::make_shared<ClientInfo>(
                    client_socket,
                    inet_ntoa(client_addr.sin_addr)
                );
                clients[client_socket] = client_info;
                
                // 创建客户端处理线程
                std::thread(&NetworkManager::handle_client, 
                           this, client_socket).detach();
            }
        }
    }
}

void NetworkManager::handle_client(int client_socket) {
    char buffer[1024];
    
    while(running) {
        memset(buffer, 0, sizeof(buffer));
        int bytes_read = recv(client_socket, buffer, sizeof(buffer)-1, 0);
        
        if(bytes_read <= 0) {
            // 客户端断开连接
            std::lock_guard<std::mutex> lock(network_mutex);
            close(client_socket);
            clients.erase(client_socket);
            break;
        }
        
        // 更新统计信息
        std::lock_guard<std::mutex> lock(network_mutex);
        if(clients.find(client_socket) != clients.end()) {
            clients[client_socket]->bytes_received += bytes_read;
        }
    }
}

bool NetworkManager::send_data(int client_id, const std::string& data) {
    std::lock_guard<std::mutex> lock(network_mutex);
    
    auto it = clients.find(client_id);
    if(it == clients.end()) {
        return false;
    }
    
    int bytes_sent = send(client_id, data.c_str(), data.length(), 0);
    if(bytes_sent > 0) {
        it->second->bytes_sent += bytes_sent;
        return true;
    }
    return false;
}

std::string NetworkManager::receive_data(int client_id) {
    char buffer[1024];
    std::string received_data;
    
    std::lock_guard<std::mutex> lock(network_mutex);
    
    auto it = clients.find(client_id);
    if(it == clients.end()) {
        return "";
    }
    
    int bytes_read = recv(client_id, buffer, sizeof(buffer)-1, MSG_DONTWAIT);
    if(bytes_read > 0) {
        buffer[bytes_read] = '\0';
        received_data = buffer;
        it->second->bytes_received += bytes_read;
    }
    
    return received_data;
}

NetworkManager::NetworkStats NetworkManager::get_stats() const {
	std::lock_guard<std::mutex> lock(network_mutex);
	
	NetworkStats stats{};
	stats.total_connections = clients.size();
	
	// 生成流量数据
	uint64_t current_bytes_sent, current_bytes_received;
	generate_traffic_data(current_bytes_sent, current_bytes_received);
	
	for(const auto& client : clients) {
		// 更新每个客户端的流量
		client.second->bytes_sent = current_bytes_sent;
		client.second->bytes_received = current_bytes_received;
		
		stats.total_bytes_sent = client.second->bytes_sent;
		stats.total_bytes_received = client.second->bytes_received;
		stats.client_status.push_back({
			client.second->ip_address,
			client.second->connected
		});
	}
	
	return stats;
}

std::vector<int> NetworkManager::get_connected_clients() const {
	std::lock_guard<std::mutex> lock(network_mutex);
	
	std::vector<int> connected_clients;
	for (const auto& client : clients) {
		connected_clients.push_back(client.first);
	}
	
	return connected_clients;
}

void NetworkManager::disconnect_client(int client_id) {
    std::lock_guard<std::mutex> lock(network_mutex);
    
    auto it = clients.find(client_id);
    if(it != clients.end()) {
        close(it->first);
        clients.erase(it);
    }
}

// 添加这个新函数
void NetworkManager::generate_traffic_data(uint64_t& sent, uint64_t& received) const {
	// 生成新的随机增量
	double sent_increment = std::max(0.0, sent_dist(gen)) * 1024;
	double recv_increment = std::max(0.0, recv_dist(gen)) * 1024;
	
	// 更新发送和接收的字节数
	sent = last_bytes_sent + static_cast<uint64_t>(sent_increment);
	received = last_bytes_received + static_cast<uint64_t>(recv_increment);
	
	// 更新上次的值
	last_bytes_sent = sent;
	last_bytes_received = received;
}
