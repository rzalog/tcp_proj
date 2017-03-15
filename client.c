#include <stdio.h> //printf
#include <string.h> //memset
#include <stdlib.h> //exit(0);
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>
#include "tcp.h"
 
#define OUTPUT_FILE "received.data"

 typedef struct {
 	int cur_ack_num;
 	int sockfd;
 	struct sockaddr_in *si_other;
 } socket_info;


void die(char *s)
{
    perror(s);
    exit(1);
}

void print_SEND(int ack_num, int retransmission, int syn, int fin)
{
  if (syn)
    fprintf(stdout, "Sending packet SYN");
  else
  {
    fprintf(stdout, "Sending packet %d", ack_num);
    if (retransmission)
        fprintf(stdout, " Retransmission");
    if (fin)
        fprintf(stdout, " FIN");
  }
  fprintf(stdout, "\n");
}

void print_RECV(int seq_num)
{
    fprintf(stdout, "Receiving packet %d\n", seq_num);
}

int handshake(socket_info *sock, char *fname)
{
	// Send initial packet to server
	tcp_packet send_packet;
	tcp_header_init(&send_packet.header, 0, 0, 0, 1, 0);
	tcp_packet_init(&send_packet, NULL, 0);

	if (send_tcp_packet(&send_packet, sock->sockfd, sock->si_other) < 0)
	{
		die("Couldn't send SYN");
	}
	print_SEND(0, 0, 1, 0);

	tcp_packet recv_packet;

	if (recv_tcp_packet(&recv_packet, sock->sockfd, sock->si_other) < 0 || !recv_packet.header.syn)
	{
		die("Couldn't receive SYN");
	}

	sock->cur_ack_num = recv_packet.header.seq_num + recv_packet.data_len;

	tcp_header_init(&send_packet.header, 0, sock->cur_ack_num, 1, 0, 0);
	tcp_packet_init(&send_packet, fname, strlen(fname));

	if (send_tcp_packet(&send_packet, sock->sockfd, sock->si_other) < 0)
	{
		die("Couldn't send filename");
	}

	return 0;	
}

int recv_file(socket_info *sock, tcp_packet *fin_packet)
{
	// do some stuff
	int fd = open(OUTPUT_FILE, O_WRONLY);

	tcp_packet window[WINDOW_SIZE];

	int base = 0;
	int end = 4;

	while (1) {
		tcp_packet recv_packet;
		recv_tcp_packet(&recv_packet, sock->sockfd, sock->si_other);

		// Check for connection close
		if (recv_packet.header.fin) {
			tcp_header_init(&fin_packet->header, 0, recv_packet.header.seq_num, 0, 0, 1);
			tcp_packet_init(fin_packet, NULL, 0);
			break;
		}

		// ACK
		tcp_packet ack_packet;
		tcp_header_init(&ack_packet.header, 0, recv_packet.header.seq_num, 1, 0, 0);
		tcp_packet_init(&ack_packet, NULL, 0);
		send_tcp_packet(&ack_packet, sock->sockfd, sock->si_other);

		// Process data (adjust window, write to file if needed)
	}

	// Finish receiving + ACK'ing any remaining packets
	while (0) {
		// do stuff
	}

	return 0;
}

int close_connection(socket_info *sock, tcp_packet *fin_packet)
{
	// FIN-ACK procedure

	close(sock->sockfd);
	return 0;
}

int main(int argc, char *argv[])
{
	struct hostent *server;
	struct sockaddr_in si_other;
	int server_port_no, sockfd, slen;

    if (argc != 4)
    {
        fprintf(stderr,"ERROR, ./client < server hostname >< server portnumber >< filename >\n");
        exit(1);
    }

    // Argument parsing
    server = gethostbyname(argv[1]);
    if (server == NULL) {
    	die("No such host");
    }

	server_port_no = atoi(argv[2]);
    char* file_name = argv[3];

    // create UDP socket
 	slen = sizeof(si_other);
    if ( (sockfd=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
        die("socket");
    }
 
    memset((char *) &si_other, 0, sizeof(si_other));
    si_other.sin_family = AF_INET;
    memcpy((void *) &si_other.sin_addr.s_addr, (void *) server->h_addr, server->h_length);
    si_other.sin_port = htons(server_port_no);

    socket_info sock;
    sock.sockfd = sockfd;
    sock.si_other = &si_other;

    if (handshake(&sock, file_name) < 0) {
    	die("Couldn't complete handshake");
    }

    tcp_packet fin_packet;
    if (recv_file(&sock, &fin_packet) < 0) {
    	die("Couldn't receive file");
    }

	if (close_connection(&sock, &fin_packet) < 0) {
		die("Couldn't close connection");
	}

    return 0;
}
