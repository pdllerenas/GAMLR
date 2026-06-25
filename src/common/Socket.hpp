#pragma once

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <cerrno>
#include <iostream>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

class INetworkLink {
 public:
  virtual void Send(const std::vector<uint8_t>& data) = 0;
  virtual std::vector<uint8_t> Receive(size_t max_bytes) = 0;
  virtual ~INetworkLink() = default;
};

class BaseSocket {
 protected:
  int fd{-1};

 public:
  // base constructor handles creation
  BaseSocket() {
    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) {
      throw std::system_error(errno, std::system_category(),
                              "Error: socket creation");
    }
  }

  // delete copy semantics
  BaseSocket(const BaseSocket&) = delete;
  BaseSocket& operator=(const BaseSocket&) = delete;

  BaseSocket(BaseSocket&& other) noexcept : fd(std::exchange(other.fd, -1)) {}
  BaseSocket& operator=(BaseSocket&& other) noexcept {
    if (this != &other) {
      if (fd >= 0) close(fd);
      fd = std::exchange(other.fd, -1);
    }
    return *this;
  }

  // destructor must close socket
  virtual ~BaseSocket() {
    if (fd >= 0) {
      close(fd);
    }
  }

  [[nodiscard]] int get_fd() const noexcept { return fd; }
};

class UDPServer : public BaseSocket {
 public:
  explicit UDPServer(uint16_t port) : BaseSocket() {
    struct sockaddr_in local{};
    local.sin_family = AF_INET;
    local.sin_port = htons(port);
    local.sin_addr.s_addr = htonl(INADDR_ANY);

    // udp only binds to a port
    if (bind(fd, reinterpret_cast<struct sockaddr*>(&local), sizeof(local)) <
        0) {
      throw std::system_error(errno, std::system_category(), "Error: bind");
    }
  }

  // recvfrom wrapper
  std::vector<uint8_t> ReceiveFrom(size_t max_bytes,
                                   struct sockaddr_in& sender_addr) {
    std::vector<uint8_t> buffer(max_bytes);
    socklen_t sender_len = sizeof(sender_addr);

    ssize_t bytes =
        recvfrom(fd, buffer.data(), buffer.size(), 0,
                 reinterpret_cast<struct sockaddr*>(&sender_addr), &sender_len);

    if (bytes < 0) {
      throw std::system_error(errno, std::system_category(), "Error: recvfrom");
    }

    buffer.resize(bytes);
    return buffer;
  }

  // sendto wrapper
  void SendTo(const std::vector<uint8_t>& data,
              const struct sockaddr_in& dest_addr) {
    ssize_t sent = sendto(fd, data.data(), data.size(), 0,
                          reinterpret_cast<const struct sockaddr*>(&dest_addr),
                          sizeof(dest_addr));
    if (sent < 0) {
      throw std::system_error(errno, std::system_category(), "Error: sendto");
    }
  }
};

class UDPClient : public BaseSocket, public INetworkLink {
 public:
  explicit UDPClient(const std::string& ip, uint16_t port) : BaseSocket() {
    struct sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, ip.c_str(), &server_addr.sin_addr) <= 0) {
      throw std::invalid_argument("Error: Invalid IP address format");
    }

    if (connect(fd, reinterpret_cast<struct sockaddr*>(&server_addr),
                sizeof(server_addr)) < 0) {
      throw std::system_error(errno, std::system_category(), "Error: connect");
    }
  }

  void Send(const std::vector<uint8_t>& data) override {
    ssize_t sent = send(fd, data.data(), data.size(), 0);
    if (sent < 0) {
      throw std::system_error(errno, std::system_category(), "Error: send");
    }
  }

  std::vector<uint8_t> Receive(size_t max_bytes) override {
    std::vector<uint8_t> buffer(max_bytes);
    ssize_t bytes = recv(fd, buffer.data(), buffer.size(), 0);

    if (bytes < 0) {
      throw std::system_error(errno, std::system_category(), "Error: recv");
    }

    buffer.resize(bytes);
    return buffer;
  }
};