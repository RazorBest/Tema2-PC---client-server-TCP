#include "./BufferedTcpSocket.h"

#include <sys/socket.h>
#include <cstring>
#include <cstdlib>
#include <arpa/inet.h>

#define TCP_BUFFER_SIZE 4096

BufferedTcpSocket::BufferedTcpSocket(int sockfd) : sockfd(sockfd) {
    read_size_index = 0;
    size_read = false;
}

int BufferedTcpSocket::read_size(char *&buf, uint32_t &size) {
    size_t to_copy;

    if (sizeof(expected_size) - read_size_index > size) {
        to_copy = size;
    } else {
        to_copy = (sizeof(expected_size) - read_size_index);
    }

    memcpy((char*)(&expected_size) + read_size_index, buf, to_copy);
    read_size_index += to_copy;
    size -= to_copy;
    buf += to_copy;

    if (read_size_index == sizeof(expected_size)) {
        buffer_size = expected_size = ntohl(expected_size);
        size_read = true;

        if (buffer_size > MAX_PACKET_SIZE) {
            return PACKET_TOO_BIG;
        }

        current_buffer = std::unique_ptr<char[]>(new char[buffer_size]);
    }

    return PACKET_NOT_READY;
}

int BufferedTcpSocket::read_current_packet(char *&buf, uint32_t &size) {
    // First, read the 4 bytes of size
    if (!size_read) {
        int ret = read_size(buf, size);
        if (ret == PACKET_TOO_BIG) {
            return PACKET_TOO_BIG;
        }
        if (size == 0) {
            return PACKET_NOT_READY;
        }
    }

    size_t to_copy = size;
    if (to_copy > expected_size) {
        to_copy = expected_size;
    } 

    memcpy(current_buffer.get() + (buffer_size - expected_size), buf, to_copy);
    expected_size -= to_copy;
    size -= to_copy;
    buf += to_copy;

    if (expected_size == 0) {
        pack_t packet = {buffer_size, current_buffer};
        packet_queue.push(packet);

        size_read = false;
        read_size_index = 0;

        return BUFFER_WRITTEN;
    }

    return PACKET_NOT_READY;
}

size_t BufferedTcpSocket::send(void *buf, uint32_t len, int flags) {
    static char tcp_buffer[TCP_BUFFER_SIZE];
    uint32_t tcp_packet_size = htonl(len);
    size_t out;

    DIE(len + sizeof(uint32_t) > TCP_BUFFER_SIZE, "send_buffered");

    memcpy(tcp_buffer, &tcp_packet_size, sizeof(uint32_t)); 
    memcpy(tcp_buffer + sizeof(uint32_t), buf, len); 

    out = ::send(sockfd, tcp_buffer, sizeof(uint32_t) + len, flags);
    DIE(out < 0, "send");

    return out; 
}

size_t BufferedTcpSocket::recv(int flags, int *out) {
    static char buffer[TCP_BUFFER_SIZE];
    static ssize_t size;

    *out = 0;

    size = ::recv(sockfd, buffer, sizeof(buffer), flags);
    if (size < 0) {
        return size;
    }

    if (size == 0) {
        *out = CONNECTION_CLOSED;
        reset();
        return 0;
    }

    int ret = read(buffer, size); 
    if (ret == PACKET_TOO_BIG) {
        *out = PACKET_TOO_BIG;
        return 0;
    }

    return size;
}

void BufferedTcpSocket::reset() {
    if (size_read) {
        current_buffer.reset();
    }
    read_size_index = 0;
    size_read = false;
    while (!packet_queue.empty()) {
        packet_queue.pop();
    }
}

int BufferedTcpSocket::read(char *buf, uint32_t size) {
    while (size) {
        int ret = read_current_packet(buf, size); 
        if (ret == PACKET_TOO_BIG) {
            reset();

            return PACKET_TOO_BIG;
        }
    }
    return BUFFER_READ;
}

int BufferedTcpSocket::write_packet(char *buf) {
    if (!packet_ready()) {
        return 0;
    }
    uint32_t size;

    pack_t& packet = packet_queue.front();
    size = packet.size;
    memcpy(buf, packet.data.get(), size);
    packet_queue.pop();

    return size;
}

bool BufferedTcpSocket::packet_ready() {
    return !packet_queue.empty();
}
