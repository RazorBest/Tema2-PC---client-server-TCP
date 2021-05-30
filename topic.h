#include <stdint.h>
#include <netinet/in.h>
#pragma once

#define TOPIC_LEN  50
#define DATA_TYPE_INT 0
#define DATA_TYPE_SHORT_REAL 1
#define DATA_TYPE_FLOAT 2
#define DATA_TYPE_STRING 3
#define MAX_STRING_SIZE 1501

typedef struct topic_data {
    char topic[TOPIC_LEN];
    char data_type;
    char data[MAX_STRING_SIZE];
    char size;
} topic_data;

typedef struct topic_data_int {
    char topic[TOPIC_LEN];
    char data_type;
    char sign;
    uint32_t value;
} topic_data_int;

typedef struct topic_data_short {
    char topic[TOPIC_LEN];
    char data_type;
    uint16_t value;
} topic_data_short;

typedef struct topic_data_float {
    char topic[TOPIC_LEN];
    char data_type;
    char sign;
    uint32_t value;
    uint8_t e;
} topic_data_float ;

void parse_topic(char *data, size_t size, topic_data *topic);
