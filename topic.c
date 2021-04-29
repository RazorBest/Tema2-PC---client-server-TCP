#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <netinet/in.h>
#include <stdio.h>

#define TOPIC_LEN  50
#define DATA_TYPE_INT 0
#define DATA_TYPE_SHORT_REAL 1
#define DATA_TYPE_FLOAT 2
#define DATA_TYPE_STRING 3
#define MAX_STRING_SIZE 1501
#define INT_TOPIC_SIZE TOPIC_LEN + 6
#define SHORT_REAL_TOPIC_SIZE TOPIC_LEN + 3
#define FLOAT_TOPIC_SIZE TOPIC_LEN + 7

#define MAX_STRING_INT_LEN 12
#define MAX_STRING_SHORT_REAL_LEN 7
#define MAX_STRING_FLOAT_LEN 13

typedef struct subscription_data {
    struct in_addr addr;
    uint16_t port;
} subscription_data;

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

void parse_topic(char *data, size_t size, topic_data *topic) {
    topic_data_int *int_topic = (topic_data_int*)topic;
    topic_data_short *short_topic = (topic_data_short*)topic;
    topic_data_float *float_topic = (topic_data_float*)topic;

    memcpy(topic, data, size);

    switch (topic->data_type) {
        case DATA_TYPE_INT : 
            int_topic->value = ntohl(int_topic->value);
            break;
        case DATA_TYPE_SHORT_REAL:
            short_topic->value = ntohs(short_topic->value);
            break;
        case DATA_TYPE_FLOAT:
            float_topic->value = ntohl(float_topic->value);
            break;
        case DATA_TYPE_STRING:
            topic->size = size - TOPIC_LEN - 1;
            break;
    }
}

double my_pow(double a, uint8_t e) {
    double res = 1;

    for (int i = 0; i < e; i++) {
        res *= a;
    }

    return res;
}

void parse_topic_value_to_string(topic_data *topic, char *str) {
    topic_data_int *int_topic = (topic_data_int*)topic;
    topic_data_short *short_topic = (topic_data_short*)topic;
    topic_data_float *float_topic = (topic_data_float*)topic;
    long long val;
    double real_val;

    switch (topic->data_type) {
        case DATA_TYPE_INT : 
            val = int_topic->value;
            val *= (int_topic->sign ? -1 : 1);
            snprintf(str, MAX_STRING_INT_LEN, "%lld", val);
            break;
        case DATA_TYPE_SHORT_REAL:
            real_val = short_topic->value;
            real_val /= 100;
            snprintf(str, MAX_STRING_SHORT_REAL_LEN, "%f", real_val);
            break;
        case DATA_TYPE_FLOAT:
            real_val = float_topic->value;
            real_val /= my_pow(10, float_topic->e);
            real_val *= (float_topic->sign ? -1 : 1);
            snprintf(str, MAX_STRING_FLOAT_LEN, "%f", real_val);
            break;
        case DATA_TYPE_STRING:
            snprintf(str, topic->size, "%s", topic->data);
            break;
    }
}
