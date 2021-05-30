#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/tcp.h> 
#include <aliases.h>

#include <iostream>
#include <map>
#include <list>

#include "./utils.h"
#include "./subscriber.h"
#include "./topic.h"
#include "./tcp_app.h"
#include "./MultiSocket.h"

#define CLI_TCP_ANON 1
#define CLI_TCP_AUTH 2

#define CLI_PACKET_DATA_SIZE 64

void usage(char *file)
{
    std::cerr << "Usage: " << file << " server_port\n";
	exit(0);
}

void send_packet_to_sub(MultiSocket &multi, int sockfd, shared_packet packet) {
    int cnt = multi.send(sockfd, packet.data.get(), packet.size, 0);
    DIE(cnt < 0, "send to sub");
}

void send_news_to_subscribers(MultiSocket &buffered_tcp,
        std::map<std::string, subscriber> &clients, 
        std::map<std::string, std::map<std::string, char>>
        &topic_map, shared_packet packet) {

    std::string topic = get_news_topic(packet.data.get());

    // Send the packet to every subscribed client
    // Or put it in the sf queue if sf=1
    for(auto entry : topic_map[topic]) {
        std::string id = entry.first;
        int sf = entry.second;

        if (clients[id].state == CLI_DISCONNECTED) {
            if (sf == 0) {
                continue;
            }
      
            clients[id].subs_queue.push(packet);

            continue;
        }

        send_packet_to_sub(buffered_tcp, clients[id].sockfd, packet);
    }
}

/* Forwards the packets that were stored when the client was offline.
 * This function is called when a subscriber connects to the server.
 */
void unload_news_queue(MultiSocket &buffered_tcp, subscriber sub) {
    while(!sub.subs_queue.empty()) {
        shared_packet packet = sub.subs_queue.front();
        sub.subs_queue.pop();
        send_packet_to_sub(buffered_tcp, sub.sockfd, packet);
    }
}

void print_auth_confirmation(int sockfd, char *buf) {
    cli_packet_auth *auth = (cli_packet_auth*)buf;
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);

    getpeername(sockfd, (struct sockaddr*)(&addr), &addrlen);

    std::cout << "New client " << auth->id << " connected from ";
    std::cout << inet_ntoa(addr.sin_addr) << ':' << 
        (uint16_t)ntohs(addr.sin_port) << ".\n";
}

void print_auth_decline(char *buf) {
    cli_packet_auth *auth = (cli_packet_auth*)buf;

    std::cout << "Client " << auth->id << " already connected.\n";
}

char process_tcp_client_packet(std::map<std::string,
        std::map<std::string, char>> &topic_map, 
        std::map<std::string, subscriber> &clients,
        std::map<int, std::string> &ids_map,
        int sockfd, char *buf, size_t size) {
    cli_packet packet;

    DIE(size > sizeof(packet), "tcp packet too big");

    memcpy(&packet, buf, size);
    ((char*)&packet)[size] = 0;

    if (packet.type == TYPE_AUTH) {

        cli_packet_auth *auth = (cli_packet_auth*)(&packet);
        // Create client entry if it isn't already
        if (clients.count(auth->id) == 0) {
            clients[auth->id] = new_subscriber();
        }

        // Another client with the same id is already connected
        if (clients[auth->id].state == CLI_CONNECTED) {
            packet.type = TYPE_AUTH_DUPLICATE;
            return TYPE_AUTH_DUPLICATE; 
        }

        clients[auth->id].state = CLI_CONNECTED;
        clients[auth->id].sockfd = sockfd;

        ids_map[sockfd] = auth->id;
    } else if (packet.type == TYPE_SUB || packet.type == TYPE_SUB_SF) {
        if (ids_map.count(sockfd) == 0) {
            return TYPE_NOT_AUTH; 
        }
        std::string id = ids_map[sockfd];
        cli_packet_sub *sub = (cli_packet_sub*)(&packet);

        if (packet.type == TYPE_SUB) {
            topic_map[sub->topic][id] = 0;
        } else if (packet.type == TYPE_SUB_SF) {
            topic_map[sub->topic][id] = 1;
        }
    } else if (packet.type == TYPE_UNSUB) {
        if (ids_map.count(sockfd) == 0) {
            return TYPE_NOT_AUTH; 
        }
        std::string id = ids_map[sockfd];
        cli_packet_sub *sub = (cli_packet_sub*)(&packet);
    
        topic_map[sub->topic].erase(id);
    }

    return packet.type;
}

void handle_tcp_app_recv (
        MultiSocket &multi,
        std::map<std::string, std::map<std::string, char>> &topic_map,
        std::map<std::string, subscriber> &clients,
        std::map<int, std::string> &ids_map,
        int sockfd) {

	char buffer[BUFLEN];
    size_t buflen;

    int out;

    memset(buffer, 0, sizeof(buffer));
    buflen = sizeof(buffer);

    buflen = multi.recv(buffer, buflen, 0, &out);
    DIE(out == BUFFER_TOO_SMALL, "recv");

    if (out == PACKET_NOT_READY) {
        return;
    }

    char type = process_tcp_client_packet(topic_map, clients, ids_map,
            sockfd, buffer, buflen);

    if (type == TYPE_AUTH) {
        print_auth_confirmation(sockfd, buffer);
        multi.send(sockfd, buffer, buflen, 0);
        unload_news_queue(multi, clients[ids_map[sockfd]]);
    } else if (type == TYPE_AUTH_DUPLICATE) {
        print_auth_decline(buffer);
        buflen = sizeof(buffer);
        change_tcp_type_packet(TYPE_AUTH_DUPLICATE, buffer, &buflen);
        multi.send(sockfd, buffer, buflen, 0);
        multi.close(sockfd);
    }
}


int main(int argc, char *argv[]) {
	int portno;
	char buffer[BUFLEN];
    size_t buflen;
    std::map<std::string, std::map<std::string, char>> topic_map;
    std::map<std::string, subscriber> clients;
    std::map<int, std::string> ids_map;
    int fd;

    std::cout.setf(std::ios::unitbuf);

	if (argc < 2) {
		usage(argv[0]);
	}

	portno = atoi(argv[1]);
	DIE(portno == 0, "atoi");

    MultiSocket multi(portno);
    // Enable file descriptors for udp and tcp
    multi.enable_udp();
    multi.enable_tcp_connections();

    // Add stdin file descriptor
    multi.add_fd(STDIN_FILENO);

    bool running = true;

	while (running) {
        // Get an available file descriptor or wait for one
        fd = multi.get_next_fd();
        if (fd < 0) {
            multi.select();
            fd = multi.get_next_fd();
        }
        
        // Handle stdin input
        if (fd == STDIN_FILENO) {
            fgets(buffer, sizeof(buffer) - 1, stdin);
            if (strncmp(buffer, "exit", 4) == 0) {
                running = false;
                break;
            }
            continue;
        }

        // Handle udp datagrams
        if (multi.is_udp()) {
            struct sockaddr_in addr_udp;
            socklen_t addr_udp_len = sizeof(addr_udp);

            memset(buffer, 0, BUFLEN);
            buflen = recvfrom(fd, buffer, sizeof(buffer), 0,
                    (struct sockaddr*)(&addr_udp), &addr_udp_len); 
            DIE(buflen < 0, "recv");

            shared_packet news_packet;
            news_packet = get_sendable_news(buffer, buflen, addr_udp);

            send_news_to_subscribers(multi, clients, topic_map,
                    news_packet);

            continue;
        }

        // Handle tcp connection
        if (multi.is_tcp_connection()) {
            multi.accept();
            continue;
        }
        
        if (multi.has_closed()) {
            std::cout << "Client " + ids_map[fd] + " disconnected.\n";

            // Set subscriber as disconnected
            std::string id = ids_map[fd];
            clients[id].state = CLI_DISCONNECTED;
            // Erase socket entry
            ids_map.erase(fd);

            continue;
        }

        handle_tcp_app_recv(multi, topic_map, clients, ids_map, fd);
	}

    multi.close();

	return 0;
}
