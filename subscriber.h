#pragma once

#include <sys/socket.h>
#include <netinet/in.h>
#include <queue>
#include <string>

#include "./utils.h"

#define TOPIC_LEN  50
#define MAX_DATA_LEN 1501

#define CLI_DISCONNECTED 0
#define CLI_CONNECTED    1

struct subscriber_news {
    std::string addr;
    std::string port;
    char topic[TOPIC_LEN + 1];
    std::string data_type;
    std::string value;
};

typedef struct subscriber {
    int sockfd;
    char state;
    std::queue<shared_packet> subs_queue;
} subscriber;

subscriber new_subscriber();

shared_packet get_sendable_news(const char *buf, size_t size, struct sockaddr_in saddr);

std::string get_news_topic(const char *buf);

void parse_subscriber_news(char *buf, size_t size, subscriber_news *news);
