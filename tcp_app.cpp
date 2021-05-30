#include <cstring>
#include "./tcp_app.h"
#include "./topic.h"
#include "./utils.h"

void generate_tcp_auth_packet(char *id, char *buf, size_t *len) {
    cli_packet_auth *auth = (cli_packet_auth*)buf;

    auth->type = TYPE_AUTH;
    strncpy(auth->id, id, CLIENT_ID_LEN); 
    *len = sizeof(auth->type) + CLIENT_ID_LEN;
}

void change_tcp_type_packet(char type, char *buf, size_t *len) {
    cli_packet_auth *auth = (cli_packet_auth*)buf;

    auth->type = TYPE_AUTH_DUPLICATE;
    *len = sizeof(auth->type);
}

void generate_tcp_sub_packet(char type, char *topic, char *buf, size_t *len) {
    cli_packet_sub *sub = (cli_packet_sub*)buf;

    strncpy(sub->topic, topic, TOPIC_LEN); 
    sub->type = type;

    *len = sizeof(sub->type) + strnlen(sub->topic, TOPIC_LEN); 
}

char get_tcp_packet_type(char *buf, size_t len) {
    cli_packet *packet = (cli_packet*)buf;

    size_t expected_size = buf - (char*)(&packet->type);
    expected_size += sizeof(packet->type);
    if (len < expected_size) {
        return TYPE_INVALID;
    }

    return packet->type;
}
