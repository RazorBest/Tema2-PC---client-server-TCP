#pragma once

#include <cstdlib>
#include <queue>
#include <cstdint>
#include <memory>
#include "./utils.h"

#define MAX_PACKET_SIZE 65536

#define BUFFER_WRITTEN    0 
#define PACKET_NOT_READY  1
#define BUFFER_TOO_SMALL  2
#define UNBUFFERED_SOCKET 4
#define PACKET_TOO_BIG    8
#define BUFFER_READ       16
#define CONNECTION_CLOSED 32

struct pack_t {
    uint32_t size;
    std::shared_ptr<char[]> data;
};

class BufferedTcpSocket {
public:
    BufferedTcpSocket(int sockfd);
    size_t send(void *buf, uint32_t len, int flags);
    size_t recv(int flags, int *out);

    void reset();
    int read(char *buf, uint32_t size);
    /* Returns true if the next packet has been completely read */
    bool packet_ready();
    /* Writes the application packet if it's available and it's size
     * is smaller or equal than the given size 
     *
     * After the paket is copied to buf, size is updated with the new size
     * of the buffer */
    int write_packet(char *buf);
    
private:
    int sockfd;

    std::shared_ptr<char[]> current_buffer;
    uint32_t expected_size;
    uint32_t buffer_size;
    bool size_read;
    uint8_t read_size_index;
    std::queue<pack_t> packet_queue;

    int read_size(char *&buf, uint32_t &size);
    int read_current_packet(char *&buf, uint32_t &size);
};
