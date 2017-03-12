#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdlib.h>
#include <strings.h>
#include <arpa/inet.h>
#include "tcp.h"

void header_flags_init(header_flags *flags, int ack, int syn, int fin) {
  char flag_set = 0;
  if (ack) {
    flag_set |= ACK_FLAG_MASK;
  }
  if (syn) {
    flag_set |= SYN_FLAG_MASK;
  }
  if (fin) {
    flag_set |= FIN_FLAG_MASK;
  }

  flags->flag_set = flag_set;
}

int test_mask(char test, char mask) {
  return test & mask;
}

int ack_flagged(header_flags flag) {
  return test_mask(flag.flag_set, ACK_FLAG_MASK);
}

int syn_flagged(header_flags flag) {
  return test_mask(flag.flag_set, SYN_FLAG_MASK);
}

int fin_flagged(header_flags flag) {
  return test_mask(flag.flag_set, FIN_FLAG_MASK);
}

void tcp_header_init(tcp_header *header, short src_port, short dest_port, int seq_num, int ack_num, int ack_flag, int syn_flag, int fin_flag) {
  // Passed in values
  header->src_port = src_port;
  header->dest_port = dest_port;
  header->seq_num = seq_num;
  header->ack_num = ack_num;

  header_flags_init(&(header->flags), ack_flag, syn_flag, fin_flag);

  // Values that are going to be constant or computed
  header->header_len = TCP_HEADER_LEN;
  header->recv_wndw = RECV_WNDW_DEFAULT;
  header->check_sum = 0;
  header->urgent_data_pointer = 0;
}

void tcp_packet_init(tcp_packet *packet, void *data, int data_len) {
  if (data_len != 0) {
    memcpy(packet->data, data, data_len);
  }
  
  packet->data_len = data_len;
}

int send_tcp_packet(tcp_packet* send_packet, int sock_fd, const struct sockaddr_in *dest_addr) {
  if (sendto(sock_fd, (void *) send_packet, send_packet->data_len+TCP_HEADER_LEN, 0, (struct sockaddr *) dest_addr, sizeof(struct sockaddr_in)) < send_packet->data_len+TCP_HEADER_LEN) {
    return -1;
  }
  
  return 0;
}

int recv_tcp_packet(tcp_packet* recv_packet, int sock_fd, const struct sockaddr_in *src_addr) {
  unsigned int slen = sizeof(struct sockaddr);
  
  int n = recvfrom(sock_fd, (void *) recv_packet, TCP_HEADER_LEN+TCP_MAX_DATA_LEN, 0, (struct sockaddr *) src_addr, &slen);

  if (n < 0) {
    return -1;
  }
  recv_packet->data_len = n - TCP_HEADER_LEN;
  
  return n;
}

f_socket* f_socket_init()
{
  f_socket* sockfd = (f_socket *)malloc(sizeof(f_socket));
  if ((sockfd->fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
    return NULL;
  }
  
  return sockfd;
}

int f_bind(f_socket *sockfd, struct sockaddr_in *addr)
{
  sockfd->src_port = addr->sin_port;
  sockfd->dest_port = 0;

  if (bind(sockfd->fd, (struct sockaddr *) addr, sizeof(struct sockaddr)) < 0) {
    return -1;
  }

  return 0;
}
