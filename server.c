#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "helpers.h"
#include "utils.h"
#include "Set.h"
#include "Hashtable.h"

#define CLI_TCP_ANON 1
#define CLI_TCP_AUTH 2

#define CLI_PACKET_DATA_SIZE 64
#define TOPIC_LEN  50
#define TYPE_AUTH     1
#define TYPE_SUB      2
#define TYPE_SUB_SF   3
#define TYPE_UNSUB    4

typedef struct subscriber {
    struct sockaddr_in addr;
    int state;
    void *subscriptions;
    char id[CLIENT_ID_LEN]; 
} subscriber;

typedef struct cli_packet {
    char type;
    char data[CLI_PACKET_DATA_SIZE];
} sub_packet;

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

void print_hex(char *data, size_t size) {
    printf("%lu\n", size);
    for (size_t i = 0; i < size; i++) {
        printf("%02hhx", data[i]);
    }
    printf("\n");
}

subscriber* new_subscriber(struct sockaddr_in addr) {
    subscriber *client = malloc(sizeof(*client));
    client->addr = addr;
    client->state = CLI_TCP_ANON;

    return client;
}

int is_auth(subscriber *client) {
    return client->state == CLI_TCP_AUTH;
}

void usage(char *file)
{
	fprintf(stderr, "Usage: %s server_port\n", file);
	exit(0);
}

int max_int(int a, int b) {
    return a > b ? a : b;
}

int accept_connection(int sockfd) {
	struct sockaddr_in cli_addr;
	socklen_t clilen;
    
    clilen = sizeof(cli_addr);
    int newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
    DIE(newsockfd < 0, "accept");

    return newsockfd;
}

void process_msg(char *buf, size_t size) {
   print_hex(buf, size); 
}

int cmp_topic(void *k1, void *k2) {
    return strncmp((char*)k1, (char*)k2, TOPIC_LEN);
}

int cmp_client_id(void *k1, void *k2) {
    return strncmp((char*)k1, (char*)k2, CLIENT_ID_LEN);
}

void print_str(void *str) {
    printf("yoy: %s\n", (char*)str);
}

void do_nothing(void *_) {}

int main(int argc, char *argv[]) {
    int sockfd_udp, sockfd_tcp;
	int portno;
	char buffer[BUFLEN];
	struct sockaddr_in serv_addr;
	int n, i, ret;
    Hashtable topic_table;
    Hashtable subscribers;

    setvbuf(stdout, NULL, _IONBF, BUFSIZ);

    topic_table = new_ht(TOPIC_LEN, cmp_topic);
    subscribers = new_ht(CLIENT_ID_LEN, cmp_client_id); 

	fd_set read_fds;	// multimea de citire folosita in select()
	fd_set tmp_fds;		// multime folosita temporar
	int fdmax;			// valoare maxima fd din multimea read_fds

	if (argc < 2) {
		usage(argv[0]);
	}

	// se goleste multimea de descriptori de citire (read_fds) si multimea temporara (tmp_fds)
	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);

	sockfd_udp = socket(AF_INET, SOCK_DGRAM, 0);
	DIE(sockfd_udp < 0, "socket udp");
	sockfd_tcp = socket(AF_INET, SOCK_STREAM, 0);
	DIE(sockfd_tcp < 0, "socket tcp");

	portno = atoi(argv[1]);
	DIE(portno == 0, "atoi");

	memset((char *) &serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(portno);
	serv_addr.sin_addr.s_addr = INADDR_ANY;

	ret = bind(sockfd_udp, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr));
	DIE(ret < 0, "bind udp");
	ret = bind(sockfd_tcp, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr));
	DIE(ret < 0, "bind tcp");

	ret = listen(sockfd_tcp, MAX_CLIENTS);
	DIE(ret < 0, "listen tcp");

	// se adauga noul file descriptor (socketul pe care se asculta conexiuni) in multimea read_fds
	FD_SET(sockfd_udp, &read_fds);
	FD_SET(sockfd_tcp, &read_fds);
	fdmax = max_int(sockfd_udp, sockfd_tcp);

	while (1) {
		tmp_fds = read_fds; 

		ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
		DIE(ret < 0, "select");

		for (i = 0; i <= fdmax; i++) {
            if (!FD_ISSET(i, &tmp_fds)) {
                continue;
            }
            if (i == sockfd_udp) {
                //int newsockfd = accept_connection(i);

                printf("UDP message\n");

                memset(buffer, 0, BUFLEN);
                n = recv(i, buffer, sizeof(buffer), 0);
                DIE(n < 0, "recv");

                process_msg(buffer, n);
                printf("%.50s\n\n", buffer);

                //FD_SET(newsockfd, &read_fds);
                /*if (newsockfd > fdmax) { 
                    fdmax = newsockfd;
                }*/
            } else if (i == sockfd_tcp) {
                int newsockfd = accept_connection(i);

                FD_SET(newsockfd, &read_fds);
                if (newsockfd > fdmax) { 
                    fdmax = newsockfd;
                }
            } else {
                // s-au primit date pe unul din socketii de client,
                // asa ca serverul trebuie sa le receptioneze
                memset(buffer, 0, BUFLEN);
                n = recv(i, buffer, sizeof(buffer), 0);
                DIE(n < 0, "recv");

                process_msg(buffer, n);

                if (n == 0) {
                    // conexiunea s-a inchis
                    printf("Socket-ul client %d a inchis conexiunea\n", i);
                    close(i);
                    
                    // se scoate din multimea de citire socketul inchis 
                    FD_CLR(i, &read_fds);
                } else {
                    printf ("S-a primit de la clientul de pe socketul %d mesajul: %s\n", i, buffer);
                }
            }
		}
	}

	close(sockfd_tcp);

	return 0;
}
