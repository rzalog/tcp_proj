#ifndef TCP_H
#define TCP_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>

// Constants
#define TCP_HEADER_LEN 20
#define TCP_MAX_DATA_LEN 1004
#define RECV_WNDW_DEFAULT 5120
#define WINDOW_SIZE 5

#define SERVER_DEFAULT_SEQ_NUM 5095
#define CLIENT_DEFAULT_SEQ_NUM 10
#define MAX_SEQ_NUM 30720

typedef struct {
  int seq_num;
  int ack_num;
  int ack;
  int syn;
  int fin;
} tcp_header;

typedef struct {
  tcp_header header;
  char data[TCP_MAX_DATA_LEN];
  int data_len;
} tcp_packet; // 1024 bytes


void tcp_header_init(tcp_header *header, int seq_num, int ack_num, int ack_flag, int syn_flag, int fin_flag);
void tcp_packet_init(tcp_packet *packet, void *data, int data_len);

int send_tcp_packet(tcp_packet* send_packet, int sockfd, const struct sockaddr_in *dest_addr);
int recv_tcp_packet(tcp_packet* recv_packet, int sockfd, const struct sockaddr_in *src_addr);

#endif
