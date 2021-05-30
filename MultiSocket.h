#pragma once

#include <cstdlib>
#include <unordered_map>
#include <memory>
#include <set>
#include <netinet/in.h>
#include "BufferedTcpSocket.h"

#define SOCK_NULL       0
#define SOCK_CLOSE      16
#define SOCK_TCP        32
#define SOCK_UDP        64
#define SOCK_CONNECT    256
#define FD_UNBUFFERED   512

struct recv_holder {
    int fd;
    int type;
};

class MultiSocket {
public:
    MultiSocket();
    MultiSocket(int port);
    int enable_udp();
    int connect(uint32_t ip, uint16_t port);
    int enable_tcp_connections();
    void add_fd(int fd);
    void select();
    int get_next_fd();

    bool is_tcp_connection();
    bool is_tcp();
    bool is_udp();
    bool has_closed();

    int accept();
    void close();
    void close(int sockfd);
    bool has_packet(int sockfd);
    size_t send(int sockfd, void *buf, uint32_t len, int flags);
    size_t recv(void *buf, uint32_t len, int flags, int *out);

private:
	struct sockaddr_in serv_addr;
    int sockfd_udp;
    int sockfd_tcp;
    int curr_sockfd;

	fd_set read_fds;
	int fdmax;
    int current_fd_type;

    std::queue<recv_holder> sock_queue;

    std::unordered_map<int, std::unique_ptr<BufferedTcpSocket>> tcp_sockets;

    void this_method_does_something_important_with_the_file_descriptors(fd_set*);
};
