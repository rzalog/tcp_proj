#include <unistd.h>
#include <stdio.h>
#include <sys.types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdlib.h>
#include <strings.h>
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

int test_mask(void *test, int mask) {
  int test_int = *((int *) test);
  return test_int & mask;
}

int ack_flagged(header_flags* flag) {
  return test_mask(&(flag->flag_set), ACK_FLAG_MASK);
}

int syn_flagged(header_flags* flag) {
  return test_mask(&(flag->flag_set), SYN_FLAG_MASK);
}

int fin_flagged(header_flags* flag) {
  return test_mask(&(flag->flag_set), FIN_FLAG_MASK);
}

void tcp_header_init(tcp_header *header, short src_port, short dest_port, int seq_num, int ack_num, header_flags flags) {
  // Passed in values
  header->src_port = src_port;
  header->dest_port = dest_port;
  header->seq_num = seq_num;
  header->ack_num = ack_num;
  header->flags = flags;

  // Values that are going to be constant or computed
  header->header_len = 20;
  header->recv_wndw = RECV_WNDW_DEFAULT;
  header->check_sum = 0;
  header->urgent_data_pointer = 0;
}

void tcp_packet_init(tcp_packet *packet, tcp_header *header, void *data, int data_len) {
  packet->header = header;
  packet->header_len = TCP_HEADER_LEN;
  packet->data = data;
  packet->data_len = data_len;
}

int send_tcp_packet(tcp_packet* send_packet, int sock_fd, const struct sockaddr_in *dest_addr) {
  if (sendto(sock_fd, (void *) send_packet->header, send_packet->header_len, 0, (struct sockaddr *) dest_addr, sizeof(struct sockaddr)) < send_packet->header_len) {
    return -1;
  }

  if (sendto(sock_fd, (void *) send_packet->data, send_packet->data_len, 0, (struct sockaddr *) dest_addr, sizeof(struct sockaddr)) < send_packet->data_len) {
    return -1;
  }
  
  return 0;
}

int recv_tcp_packet(tcp_packet* recv_packet, int sock_fd) {
  int slen = sizeof(struct sockaddr);
  
  if (recvfrom(sock_fd, (void *) send_packet->header, TCP_HEADER_LEN, 0, (struct sockaddr *) dest_addr, &slen) < TCP_HEADER_LEN) {
    return -1;
  }

  int n = sendto(sock_fd, (void *) send_packet->data, TCP_MAX_DATA_LEN, 0, (struct sockaddr *) dest_addr, &slen);
  if (n < 0) {
    return -1;
  }
  send_packet->data_len = n;
  
  return 0;
}
