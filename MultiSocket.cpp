#include "MultiSocket.h"
#include <sys/socket.h>
#include <netinet/tcp.h> 
#include <cstring>
#include <unistd.h>

MultiSocket::MultiSocket() {
    sockfd_udp = -1;
    sockfd_tcp = -1;
    curr_sockfd = -2;
    current_fd_type = SOCK_NULL; 
    fdmax = -1;

    // Set a NULL sockaddr
	memset((char *) &serv_addr, 0, sizeof(serv_addr));

    // Clear the read descriptor set and the write descriptor set
	FD_ZERO(&read_fds);
}

MultiSocket::MultiSocket(int port) {
    sockfd_udp = -1;
    sockfd_tcp = -1;
    curr_sockfd = -2;
    current_fd_type = SOCK_NULL; 
    fdmax = -1;

    // Set the sockaddr structure for server sockets
	memset((char *) &serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(port);
	serv_addr.sin_addr.s_addr = INADDR_ANY;

    // Clear the read descriptor set and the write descriptor set
	FD_ZERO(&read_fds);
}

int MultiSocket::enable_udp() {
    int ret;

	sockfd_udp = socket(AF_INET, SOCK_DGRAM, 0);
	DIE(sockfd_udp < 0, "socket udp");
	ret = bind(sockfd_udp, (struct sockaddr*)(&serv_addr),
            sizeof(struct sockaddr));
	DIE(ret < 0, "bind udp");

	FD_SET(sockfd_udp, &read_fds);
	fdmax = max_int(fdmax, sockfd_udp);

    return sockfd_udp;
}

int MultiSocket::connect(uint32_t s_addr, uint16_t port) {
    int sockfd;
    int ret;

    // Create a TCP socket
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	DIE(sockfd < 0, "socket");

    // Disable Neagle algorithm
    int flag = 1;
    int result = setsockopt(sockfd , IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(int));  
    DIE(result < 0, "accept");

    // Set the server ipv4 and port
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = port;
	serv_addr.sin_addr.s_addr = s_addr;

	ret = ::connect(sockfd, (struct sockaddr*)(&serv_addr), sizeof(serv_addr));
	DIE(ret < 0, "connect");

    // Add the file descriptor in the selection set
    FD_SET(sockfd, &read_fds);
	fdmax = max_int(fdmax, sockfd);

    // Create a buffered socket
    tcp_sockets[sockfd] = std::make_unique<BufferedTcpSocket>(sockfd); 

    return sockfd;
}

int MultiSocket::enable_tcp_connections() {
    int ret;

	sockfd_tcp = socket(AF_INET, SOCK_STREAM, 0);
	DIE(sockfd_udp < 0, "socket tcp");
	ret = bind(sockfd_tcp, (struct sockaddr*)(&serv_addr),
            sizeof(struct sockaddr));
	DIE(ret < 0, "bind tcp");

	ret = listen(sockfd_tcp, MAX_CLIENTS);
	DIE(ret < 0, "listen tcp");

    // Disable Neagle algorithm
    int flag = 1;
    int result = setsockopt(sockfd_tcp, IPPROTO_TCP, TCP_NODELAY, &flag,
            sizeof(int));  
    DIE(result < 0, "accept");

	FD_SET(sockfd_tcp, &read_fds);
	fdmax = max_int(fdmax, sockfd_tcp);

    return sockfd_tcp;
}

void MultiSocket::add_fd(int fd) {
    FD_SET(fd, &read_fds);
}

void MultiSocket::this_method_does_something_important_with_the_file_descriptors(fd_set *selected_fds) {
    int ret;
    int out;
    int fd_type;

    for (int fd = 0; fd <= fdmax; fd++) {
        if (!FD_ISSET(fd, selected_fds)) {
            continue;
        }

        if (tcp_sockets.count(fd) > 0) {
            ret = tcp_sockets[fd]->recv(0, &out);
            // If the connection closes or the packet is too big
            if (ret <= 0 || out == PACKET_TOO_BIG) {
                this->close(fd);
                fd_type = SOCK_CLOSE;
            } else if (tcp_sockets[fd]->packet_ready()) {
                fd_type = SOCK_TCP;
            }
        } else if (fd == sockfd_tcp) {
            fd_type = SOCK_CONNECT;
        } else if (fd == sockfd_udp) {
            fd_type = SOCK_UDP;
        } else {
            fd_type = FD_UNBUFFERED;
        }

        sock_queue.push({fd, fd_type});
    }

}

void MultiSocket::select() {
    int ret;
	fd_set selected_fds;

    while (sock_queue.empty()) {
        selected_fds = read_fds;
        ret = ::select(fdmax + 1, &selected_fds, NULL, NULL, NULL);
        DIE(ret < 0, "select");
        this_method_does_something_important_with_the_file_descriptors(&selected_fds);
    }
}

int MultiSocket::get_next_fd() {
    // Return -1 if there is no next fd
    if (sock_queue.empty()) {
        current_fd_type = SOCK_NULL;
        curr_sockfd = -1;
        return -1;
    }

    recv_holder hold = sock_queue.front();

    // If it is an unhandled fd, just return it
    if (hold.type != SOCK_TCP) {
        sock_queue.pop();
        current_fd_type = hold.type;
        curr_sockfd = hold.fd;
        return hold.fd;
    } 

    // Otherwise, it must be a bufferd tcp socket
    // Remove it from the queue just after its buffered got cleared
    while (tcp_sockets.count(hold.fd) == 0 || 
            !tcp_sockets[hold.fd]->packet_ready()) {
        sock_queue.pop();
        if (sock_queue.empty()) {
            current_fd_type = SOCK_NULL;
            curr_sockfd = -1;
            return -1;
        }
        hold = sock_queue.front();
    }

    current_fd_type = hold.type;
    curr_sockfd = hold.fd;
    return hold.fd;
}

bool MultiSocket::is_tcp_connection() {
   return current_fd_type == SOCK_CONNECT; 
}

bool MultiSocket::is_udp() {
   return current_fd_type == SOCK_UDP; 
}

bool MultiSocket::has_closed() {
    return current_fd_type == SOCK_CLOSE;
}

int MultiSocket::accept() {
	struct sockaddr_in cli_addr;
	socklen_t clilen;
    
    clilen = sizeof(cli_addr);

    int newsockfd = ::accept(sockfd_tcp, (struct sockaddr*) &cli_addr,
            &clilen);
    DIE(newsockfd < 0, "accept");

    // Disable Neagle algorithm
    int flag = 1;
    int result = setsockopt(newsockfd, IPPROTO_TCP, TCP_NODELAY, &flag,
            sizeof(int));  
    DIE(result < 0, "accept");

    FD_SET(newsockfd, &read_fds);
    fdmax = max_int(fdmax, newsockfd);

    // Create a buffered socket
    tcp_sockets[newsockfd] = std::make_unique<BufferedTcpSocket>(newsockfd);
    // tcp_sockets[newsockfd] = BufferedTcpSocket(newsockfd); 

    return newsockfd;
}


void MultiSocket::close() {
    for (auto it = tcp_sockets.begin(); it != tcp_sockets.end(); it++) {
        ::close(it->first);
    }
    tcp_sockets.clear();

    FD_ZERO(&read_fds); 
    fdmax = -1;

    if (sockfd_tcp >= 0) {
        ::close(sockfd_tcp);
    }
    if (sockfd_udp >= 0) {
        ::close(sockfd_udp);
    }
}

void MultiSocket::close(int sockfd) {
    ::close(sockfd);
    if (FD_ISSET(sockfd, &read_fds)) {
        FD_CLR(sockfd, &read_fds);
    }
    // Remove buffered tcp socket if it exists
    tcp_sockets.erase(sockfd);
}

bool MultiSocket::has_packet(int sockfd) {
    // Use an iterator in order to search the map only once
    auto sock_it = tcp_sockets.find(sockfd);
    if (sock_it == tcp_sockets.end()) {
        return false;
    }

    return (sock_it->second)->packet_ready();
}

size_t MultiSocket::send(int sockfd, void *buf, uint32_t len, int flags) {
    auto sock_it = tcp_sockets.find(sockfd);
    if (sock_it == tcp_sockets.end()) {
        return -1;
    }

    return (sock_it->second)->send(buf, len, flags);
} 

size_t MultiSocket::recv(void *buf, uint32_t len, int flags, int *out) {
    int buflen;
    *out = 0;

    auto sock_it = tcp_sockets.find(curr_sockfd);
    if (sock_it == tcp_sockets.end()) {
        *out = UNBUFFERED_SOCKET;
        return -1;
    }

    if (!(sock_it->second)->packet_ready()) {
        *out = PACKET_NOT_READY;
        return 0;
    }

    buflen = (sock_it->second)->write_packet((char*)buf);

    return buflen;
}
