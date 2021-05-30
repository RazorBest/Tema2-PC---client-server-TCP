#pragma once

#include <cstdint>

#define BUFLEN		2048    // dimensiunea maxima a calupului de date
#define MAX_CLIENTS	10	    // numarul maxim de clienti in asteptare

#include <stdio.h>
#include <stdlib.h>
#include <memory>

/*
 * Macro de verificare a erorilor
 * Exemplu:
 *     int fd = open(file_name, O_RDONLY);
 *     DIE(fd == -1, "open failed");
 */

#define DIE(assertion, call_description)	\
	do {									\
		if (assertion) {					\
			fprintf(stderr, "(%s, %d): ",	\
					__FILE__, __LINE__);	\
			perror(call_description);		\
			exit(EXIT_FAILURE);				\
		}									\
	} while(0)

using std::size_t;

struct shared_packet {
    size_t size;
    std::shared_ptr<char[]> data;
};

int max_int(int a, int b);
double my_pow(double a, uint8_t e);
void print_hex(char *data, size_t size);
