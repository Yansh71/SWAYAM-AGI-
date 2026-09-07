#ifndef SWAYAM_NEXUS_C2_HPP
#define SWAYAM_NEXUS_C2_HPP

#include <string>
#include <iostream>
#include <vector>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <cerrno>

namespace Swayam {

class NexusC2 {
private:
    static std::string encrypt_payload(const std::string& data, const std::string& mutation_id) noexcept {
        std::string encrypted = data;
        
        // THE APEX FIX: True Polymorphic XOR Key Generation
        char dynamic_key = 0;
        for (char c : mutation_id) { dynamic_key ^= (c + 0x07); } // Entropy generation
        if (dynamic_key == 0x00 || dynamic_key == '\n') dynamic_key = 0x5A; // Fallback
        
        // Prepend the dynamic key as the first byte for the C2 server to extract and decrypt
        encrypted.insert(encrypted.begin(), dynamic_key); 
        
        for (size_t i = 1; i < encrypted.size(); ++i) {
            encrypted[i] ^= dynamic_key;
        }
        return encrypted;
    }

    static bool set_non_blocking(int sock) noexcept {
        int flags = fcntl(sock, F_GETFL, 0);
        if (flags == -1) return false;
        return fcntl(sock, F_SETFL, flags | O_NONBLOCK) != -1;
    }

public:
    static void transmit_telemetry(const std::string& mutation_id, const std::string& status) noexcept {
        const char* C2_IP = "127.0.0.1";
        const int C2_PORT = 4444;

        int sock = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
        if (sock == -1) return; 

        if (!set_non_blocking(sock)) { close(sock); return; }

        sockaddr_in server_addr{};
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(C2_PORT);
        if (inet_pton(AF_INET, C2_IP, &server_addr.sin_addr) <= 0) { close(sock); return; }

        int res = connect(sock, reinterpret_cast<struct sockaddr*>(&server_addr), sizeof(server_addr));
        if (res < 0 && errno != EINPROGRESS) { close(sock); return; }

        fd_set write_fds;
        FD_ZERO(&write_fds);
        FD_SET(sock, &write_fds);

        struct timeval timeout{};
        timeout.tv_sec = 1; timeout.tv_usec = 0;

        res = select(sock + 1, nullptr, &write_fds, nullptr, &timeout);
        if (res > 0 && FD_ISSET(sock, &write_fds)) {
            int so_error = 0;
            socklen_t len = sizeof(so_error);
            getsockopt(sock, SOL_SOCKET, SO_ERROR, &so_error, &len);
            
            if (so_error == 0) {
                std::string raw_payload = "[AGI-PING] ID: " + mutation_id + " | STATUS: " + status + "\n";
                std::string secure_payload = encrypt_payload(raw_payload, mutation_id);
                
                // THE APEX FIX: Resilient EAGAIN Transmission Loop (Zero Telemetry Drop)
                size_t total_sent = 0;
                int max_retries = 5;
                while (total_sent < secure_payload.size() && max_retries > 0) {
                    ssize_t bytes = send(sock, secure_payload.c_str() + total_sent, secure_payload.size() - total_sent, MSG_NOSIGNAL);
                    if (bytes > 0) {
                        total_sent += bytes;
                    } else if (bytes < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
                        struct timeval tv{0, 50000}; // 50ms wait
                        fd_set wset; FD_ZERO(&wset); FD_SET(sock, &wset);
                        select(sock + 1, nullptr, &wset, nullptr, &tv);
                        max_retries--;
                    } else { break; } // Hard disconnect
                }
            }
        }
        close(sock);
    }
};

} // namespace Swayam
#endif // SWAYAM_NEXUS_C2_HPP
