#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "./topic.h"
#include "./utils.h"

#define INT_TOPIC_SIZE TOPIC_LEN + 6
#define SHORT_REAL_TOPIC_SIZE TOPIC_LEN + 3
#define FLOAT_TOPIC_SIZE TOPIC_LEN + 7

#define MAX_STRING_INT_LEN 12
#define MAX_STRING_SHORT_REAL_LEN 7
#define MAX_STRING_FLOAT_LEN 13

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
