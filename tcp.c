#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
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

