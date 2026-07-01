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

/**
 * @brief Abstract interface for sending and receiving network packet data.
 */
class INetworkLink {
 public:
  /**
   * @brief Send raw packet data over the network.
   *
   * @param data Packet bytes to transmit.
   */
  virtual void Send(const std::vector<uint8_t>& data) = 0;

  /**
   * @brief Receive raw packet data from the network.
   *
   * @param max_bytes Maximum number of bytes to read.
   * @return Received packet bytes.
   */
  virtual std::vector<uint8_t> Receive(size_t max_bytes) = 0;

  virtual ~INetworkLink() = default;
};

/**
 * @brief Base class for low-level socket management.
 *
 * Handles socket creation, destructor cleanup, and move-only semantics.
 */
class BaseSocket {
 protected:
  int fd{-1};

 public:
  /**
   * @brief Construct a UDP socket file descriptor.
   *
   * @throws std::system_error If socket creation fails.
   */
  BaseSocket() {
    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) {
      throw std::system_error(errno, std::system_category(),
                              "Error: socket creation");
    }
  }

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

  /**
   * @brief Close the socket file descriptor when the object goes out of scope.
   */
  virtual ~BaseSocket() {
    if (fd >= 0) {
      close(fd);
    }
  }

  /**
   * @brief Get the underlying socket file descriptor.
   *
   * @return Socket file descriptor.
   */
  [[nodiscard]] int get_fd() const noexcept { return fd; }
};

/**
 * @brief UDP server socket used to receive packets from remote clients.
 */
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

  /**
   * @brief Receive data from a remote sender.
   *
   * @param max_bytes Maximum number of bytes to read.
   * @param sender_addr Output parameter filled with the sender address.
   * @return Received packet bytes.
   *
   * @throws std::system_error If recvfrom fails.
   */
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

  /**
   * @brief Send a packet to the specified destination address.
   *
   * @param data Packet bytes to send.
   * @param dest_addr Destination socket address.
   *
   * @throws std::system_error If sendto fails.
   */
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

/**
 * @brief UDP client socket implementing the generic network link interface.
 *
 * This class connects to a specific server address and provides simple
 * Send/Receive operations for packet-based communication.
 */
class UDPClient : public BaseSocket, public INetworkLink {
 public:
  /**
   * @brief Construct a UDP client and connect it to the remote server.
   *
   * @param ip Remote server IPv4 address.
   * @param port Remote server UDP port.
   *
   * @throws std::invalid_argument If the IP address format is invalid.
   * @throws std::system_error If socket connect fails.
   */
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

    struct timeval tv;
    tv.tv_sec = 5;
    tv.tv_usec = 0;
    if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
      std::cerr << "Warning: Failed to set client socket timeout.\n";
    }
  }

  /**
   * @brief Send packet data to the connected remote server.
   *
   * @param data Packet bytes to transmit.
   *
   * @throws std::system_error If send fails.
   */
  void Send(const std::vector<uint8_t>& data) override {
    ssize_t sent = send(fd, data.data(), data.size(), 0);
    if (sent < 0) {
      throw std::system_error(errno, std::system_category(), "Error: send");
    }
  }

  /**
   * @brief Receive a packet from the connected remote server.
   *
   * @param max_bytes Maximum number of bytes to read.
   * @return Received packet bytes.
   *
   * @throws std::system_error If recv fails.
   */
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