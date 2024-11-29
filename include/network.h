// network.h - 网络管理头文件
#ifndef NETWORK_H
#define NETWORK_H

#include <string>
#include <vector>
#include <queue>
#include <mutex>
#include <thread>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <map>
#include <memory>
#include <random>  // 添加这个头文件

class NetworkManager {
private:
    struct ClientInfo {
        int socket_fd;
        std::string ip_address;
        uint64_t bytes_received;
        uint64_t bytes_sent;
        bool connected;
        
        ClientInfo(int fd, std::string ip) 
            : socket_fd(fd), ip_address(ip), 
              bytes_received(0), bytes_sent(0), connected(true) {}
    };

    int server_socket;
    std::map<int, std::shared_ptr<ClientInfo>> clients;
    std::thread server_thread;
    bool running;
    mutable std::mutex network_mutex;
    
    // 随机数生成相关成员
    mutable std::random_device rd;
    mutable std::mt19937 gen;
    mutable std::normal_distribution<double> sent_dist;
    mutable std::normal_distribution<double> recv_dist;
    mutable uint64_t last_bytes_sent;
    mutable uint64_t last_bytes_received;
    
    void handle_client(int client_socket);
    void server_loop();
    void generate_traffic_data(uint64_t& sent, uint64_t& received) const;
    
public:
    NetworkManager();
    ~NetworkManager();
    
    bool start_server(int port);
    void stop_server();
    bool send_data(int client_id, const std::string& data);
    std::string receive_data(int client_id);
    
    // 获取网络状态信息
    struct NetworkStats {
        int total_connections;
        uint64_t total_bytes_sent;
        uint64_t total_bytes_received;
        std::vector<std::pair<std::string, bool>> client_status;
    };
    
    NetworkStats get_stats() const;
    std::vector<int> get_connected_clients() const;
    void disconnect_client(int client_id);
};

#endif // NETWORK_H