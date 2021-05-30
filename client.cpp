#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/tcp.h>

#include <iostream>
#include "tcp_app.h"
#include "./utils.h"
#include "./MultiSocket.h"

#define UNKNOWN_COMMAND 1
#define MAX_COMMAND_SIZE 256

void usage(char *file)
{
    std::cerr << "Usage " << file << " client_id server_address server_port\n";
	exit(0);
}

int parse_subscription_cmd(char *type, char *topic, const char *buf,
        size_t size) {
    char str[MAX_COMMAND_SIZE];
    char *p;

    strncpy(str, buf, size);

    p = strtok(str, " \n");
    if (strncmp(p, "subscribe", 9) == 0) {
        *type = TYPE_SUB;    
    } else if (strncmp(p, "unsubscribe", 11) == 0) {
        *type = TYPE_UNSUB;    
    } else {
        return UNKNOWN_COMMAND;
    }

    p = strtok(NULL, " \n");
    if (p == NULL) {
        return UNKNOWN_COMMAND;
    }
    strncpy(topic, p, TOPIC_LEN);

    if (*type == TYPE_UNSUB) {
        return 0;
    }
    
    p = strtok(NULL, " \n");
    if (p == NULL) {
        return UNKNOWN_COMMAND;
    }
    if (p[0] == '1') {
        *type = TYPE_SUB_SF;
    }
    
    return 0;
}

int main(int argc, char *argv[])
{
	int sockfd_tcp, sockfd, n, ret;
	char buffer[BUFLEN];
    size_t buf_len;
    char id[CLIENT_ID_LEN];
    char topic[TOPIC_LEN];
    char type;
    subscriber_news news;

    std::cout.setf(std::ios::unitbuf);
    
	if (argc < 4) {
		usage(argv[0]);
	}

    DIE(strlen(argv[1]) >= CLIENT_ID_LEN, "id length too big");
    strcpy(id, argv[1]);

    in_addr addr_ip;
	ret = inet_aton(argv[2], &addr_ip);
	DIE(ret == 0, "inet_aton");

    uint16_t port = htons(atoi(argv[3]));

    MultiSocket multi;

    sockfd_tcp = multi.connect(addr_ip.s_addr, port);

    // Authentication
    buf_len = sizeof(buffer);
    generate_tcp_auth_packet(id, buffer, &buf_len);
    n = multi.send(sockfd_tcp, buffer, buf_len, 0);
    DIE(n < 0, "send");

    multi.select();
    multi.get_next_fd();

    int out;
    buf_len = sizeof(buffer);
    n = multi.recv(buffer, buf_len, 0, &out);
    DIE(n < 0, "multi.recv");

    if (get_tcp_packet_type(buffer, n) == TYPE_AUTH_DUPLICATE) {
        DIE(0, "duplicate id");
        return 0;
    } else if (get_tcp_packet_type(buffer, n) != TYPE_AUTH) {
        DIE(0, "Auth failed");
        return 0;
    }

    multi.add_fd(STDIN_FILENO);

    bool running = true;

	while (running) {
        multi.select();
        sockfd = multi.get_next_fd();
        while (sockfd >= 0) {
            if (sockfd == STDIN_FILENO) {
                // se citeste de la tastatura
                memset(buffer, 0, MAX_COMMAND_SIZE);
                fgets(buffer, MAX_COMMAND_SIZE - 1, stdin);
                buffer[strlen(buffer) - 1] = 0;

                // If empty
                if (strlen(buffer) == 0) {
                    continue;
                }

                // If exit
                if (strncmp(buffer, "exit", 4) == 0) {
                    running = false;
                    break;
                }

                int cmd_status = parse_subscription_cmd(&type, topic, buffer,
                        MAX_COMMAND_SIZE);
                if (cmd_status != 0) {
                    std::cerr << "Invalid command.\n";
                    continue;
                }

                generate_tcp_sub_packet(type, topic, buffer, &buf_len);
                n = multi.send(sockfd_tcp, buffer, buf_len, 0);
                DIE(n < 0, "send");

                if (type == TYPE_SUB || type == TYPE_SUB_SF) {
                    std::cout << "Subscribed to topic.\n";
                } else if (type == TYPE_UNSUB) {
                    std::cout << "Unsubscribed from topic.\n";
                }
            } else if (multi.has_closed()) {
                running = false;
                break;
            } else {
                int out;
                n = multi.recv(buffer, BUFLEN, 0, &out);

                parse_subscriber_news(buffer, n, &news);
                std::cout << news.addr << ':' << news.port;
                std::cout << " - " << news.topic;
                std::cout << " - " << news.data_type;
                std::cout << " - " << news.value << '\n';
            }

            sockfd = multi.get_next_fd();
        }
	}

    multi.close();

	return 0;
}
