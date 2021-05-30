#pragma once

#include <map>
#include <list>
#include "./subscriber.h"

#define CLI_PACKET_DATA_SIZE 64
#define TOPIC_LEN  50
#define CLIENT_ID_LEN 11

#define TYPE_AUTH              1
#define TYPE_SUB               2
#define TYPE_SUB_SF            3
#define TYPE_UNSUB             4
#define TYPE_AUTH_DUPLICATE    5
#define TYPE_NOT_AUTH          6
#define TYPE_INVALID           7

typedef struct cli_packet {
    char type;
    char data[CLI_PACKET_DATA_SIZE];
} cli_packet;

// Special case of cli_packet
// Used for ID assignation
typedef struct cli_packet_auth {
    char type; // MUST BE TYPE_AUTH
    char id[CLIENT_ID_LEN]; 
    char data[CLI_PACKET_DATA_SIZE - CLIENT_ID_LEN];
} cli_packet_auth;

// Special case of cli_packet
// Used for subcribing/unsubscribing
typedef struct cli_packet_sub {
    char type; // MUST BE TYPE_SUB, TYPE_SUB_SF or TYPE_UNSUB
    char topic[TOPIC_LEN]; 
    char data[CLI_PACKET_DATA_SIZE - TOPIC_LEN];
} cli_packet_sub;

void generate_tcp_auth_packet(char *id, char *buf, size_t *len);
void generate_tcp_sub_packet(char type, char *topic, char *buf, size_t *len);
void change_tcp_type_packet(char type, char *buf, size_t *len);
char get_tcp_packet_type(char *buf, size_t len);
