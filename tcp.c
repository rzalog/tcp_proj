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

void tcp_header_init(tcp_header *header, int seq_num, int ack_num, int ack_flag, int syn_flag, int fin_flag) {
  header->seq_num = seq_num;
  header->ack_num = ack_num;
  header->ack = ack_flag;
  header->syn = syn_flag;
  header->fin = fin_flag;
}

void tcp_packet_init(tcp_packet *packet, void *data, int data_len) {
  if (data_len != 0) {
    memcpy(packet->data, data, data_len);
  }
  
  packet->data_len = data_len;
}

int send_tcp_packet(tcp_packet* send_packet, int sockfd, const struct sockaddr_in *dest_addr) {
  if (sendto(sockfd, (void *) send_packet, send_packet->data_len+TCP_HEADER_LEN, 0, (struct sockaddr *) dest_addr, sizeof(struct sockaddr_in)) < send_packet->data_len+TCP_HEADER_LEN) {
    return -1;
  }

  return 0;
}

int recv_tcp_packet(tcp_packet* recv_packet, int sockfd, const struct sockaddr_in *src_addr) {
  unsigned int slen = sizeof(struct sockaddr);
  
  int n = recvfrom(sockfd, (void *) recv_packet, TCP_HEADER_LEN+TCP_MAX_DATA_LEN, 0, (struct sockaddr *) src_addr, &slen);

  if (n < 0) {
    return -1;
  }
  recv_packet->data_len = n - TCP_HEADER_LEN;
  
  return n;
}

/*
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


int f_write_packet(f_socket *sockfd, tcp_packet *packet, void *data, int data_len, int ack_flag, int syn_flag, int fin_flag)
{
  // Actually send the packet
  tcp_header_init(&(packet->header), sockfd->cur_seq_num, sockfd->cur_ack_num, ack_flag, syn_flag, fin_flag);
  tcp_packet_init(&packet, data, data_len);

  if (send_tcp_packet(&packet, sockfd->fd, sockfd->dest_addr) < 0) {
    return -1;
  }

  // Update all the state
  sockfd->cur_seq_num += data_len;
}

int f_read_packet(f_socket *sockfd, tcp_packet *packet)
{
  // Actually read the packet
  if (recv_tcp_packet(&packet, sockfd->fd, sockfd->dest_addr) < 0) {
    return -1;
  }
  
  // Update state
  sockfd->ack_num += recv_packet->data_len;
}
*/