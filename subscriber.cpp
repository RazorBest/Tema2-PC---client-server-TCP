#include "./subscriber.h"
#include <cstddef>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <string>
#include <sstream>
#include <sys/socket.h>
#include <stdint.h>
#include <arpa/inet.h>

#include "./utils.h"

#define DATA_TYPE_INT 0
#define DATA_TYPE_SHORT_REAL 1
#define DATA_TYPE_FLOAT 2
#define DATA_TYPE_STRING 3

subscriber new_subscriber() {
    subscriber client;
    client.sockfd = -1;
    client.state = CLI_DISCONNECTED;

    return client;
}

shared_packet get_sendable_news(const char *buf, size_t size,
        struct sockaddr_in saddr) {
    shared_packet packet;
    struct in_addr addr;
    uint16_t port;
    size_t final_size = size + sizeof(addr) + sizeof(port);

    // Get socket data
    addr = saddr.sin_addr;
    port = htons(saddr.sin_port);

    std::shared_ptr<char[]> new_buf(new char[final_size],
            std::default_delete<char[]>());
    size_t off = 0;

    memcpy(new_buf.get() + off, &addr, sizeof(addr));
    off += sizeof(addr);
    memcpy(new_buf.get() + off, &port, sizeof(port));
    off += sizeof(port);
    memcpy(new_buf.get() + off, buf, size);
    off += size;

    packet.data = new_buf;
    packet.size = off;

    return packet;
}

std::string get_news_topic(const char *buf) {
    return std::string(buf + sizeof(struct in_addr) + sizeof(uint16_t));
}

void parse_subscriber_news(char *buf, size_t size, subscriber_news *news) {
    news->addr = inet_ntoa(*(struct in_addr*)buf);
    buf += sizeof(struct in_addr); 
    size -= sizeof(struct in_addr);

    news->port = std::to_string(*(uint16_t*)(buf));
    buf += sizeof(uint16_t);
    size -= sizeof(uint16_t);

    memcpy(news->topic, buf, TOPIC_LEN);
    news->topic[TOPIC_LEN] = 0;
    buf += TOPIC_LEN;
    size -= TOPIC_LEN;

    std::ostringstream out;
    long long sign, val;
    double sign_real, real_val, exp;
    char data_type = buf[0];
    buf++;
    size--;

    switch (data_type) {
        case DATA_TYPE_INT : 
            news->data_type = "INT";

            // sign is 1 or 0
            sign = (*(char*)buf) ? -1 : 1;
            buf++;
            val = ntohl(*(uint32_t*)(buf));
            val *= sign;
            news->value = std::to_string(val);
            break;
        case DATA_TYPE_SHORT_REAL:
            news->data_type = "SHORT_REAL";

            real_val = ntohs(*(uint16_t*)buf);
            real_val /= 100;
            out.precision(2);
            out << std::fixed << real_val;
            news->value = out.str();
            break;
        case DATA_TYPE_FLOAT:
            news->data_type = "FLOAT";

            sign_real = (*(char*)buf) ? -1 : 1;
            buf++;
            real_val = ntohl(*(uint32_t*)buf);
            buf += 4;
            exp = *(uint8_t*)buf;
            real_val /= my_pow(10, exp);
            real_val *= sign_real;

            out.precision(10);
            out << real_val;
            news->value = out.str();
            break;
        case DATA_TYPE_STRING:
            news->data_type = "STRING";

            news->value = std::string(buf, size);
            break;
    }
}
