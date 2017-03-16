#include <stdio.h> //printf
#include <string.h> //memset
#include <stdlib.h> //exit(0);
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include "tcp.h"

#define BILLION 1000000000
#define TIMEOUT_IN_NS 500000000

#define OUTPUT_FILE "received.data"

 typedef struct {
 	int cur_ack_num;
 	int sockfd;
 	struct sockaddr_in *si_other;
 } socket_info;


typedef struct {
	tcp_packet *packet;
	struct timespec time_stamp;
	int has_been_acked;
	pthread_t timeout_thread;
	int sockfd;
	const struct sockaddr_in *dest_addr;
    int is_syn;
    int is_fin;
} packet_timeout;


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
        fprintf(stdout, "Sending packet %d", ack_num);

	if (retransmission)
	    fprintf(stdout, " Retransmission");
	if (fin)
	    fprintf(stdout, " FIN");

    fprintf(stdout, "\n");
}

void print_RECV(int seq_num)
{
    fprintf(stdout, "Receiving packet %d\n", seq_num);
}

void *timeout_check(void *p_timeout_)
{
	packet_timeout *p_timeout = (packet_timeout*) p_timeout_;

	while (!p_timeout->has_been_acked)
	{
		// check timestamp
		struct timespec cur_time;
		clock_gettime(CLOCK_MONOTONIC, &cur_time);

		// if timeout time has passed
		uint64_t diff = BILLION * (cur_time.tv_sec - p_timeout->time_stamp.tv_sec);
		diff += cur_time.tv_nsec - p_timeout->time_stamp.tv_nsec;

		if (diff > TIMEOUT_IN_NS)
		{
			int n = 1;
			while (n != 0)
			{
				n = send_tcp_packet(p_timeout->packet, p_timeout->sockfd, p_timeout->dest_addr);
			}
			print_SEND(p_timeout->packet->header.ack_num, 1, p_timeout->packet->header.syn, p_timeout->packet->header.fin);

			// reset timestamp
			clock_gettime(CLOCK_MONOTONIC, &p_timeout->time_stamp);
		}
	}

	free(p_timeout);
	p_timeout_ = NULL;

	return NULL;
}

int send_and_timeout(packet_timeout *p_timeout, tcp_packet *send_packet, int sockfd, const struct sockaddr_in *dest_addr)
{
	// Just a simple wrapper function
	//p_timeout = (packet_timeout *)malloc(sizeof(packet_timeout));
	p_timeout->has_been_acked = 0;
	p_timeout->packet = send_packet;
	p_timeout->sockfd = sockfd;
	p_timeout->dest_addr = dest_addr;

	clock_gettime(CLOCK_MONOTONIC, &p_timeout->time_stamp);
	int n = send_tcp_packet(p_timeout->packet, sockfd, dest_addr);
	if (n == 0) {
		pthread_create(&p_timeout->timeout_thread, NULL, timeout_check, (void *) send_packet);
	} else {
		free(p_timeout);
		p_timeout = NULL;
	}

	return n;
}

int handshake(socket_info *sock, char *fname, packet_timeout *p_timeout)
{
	// Send initial packet to server: SYN
	tcp_packet send_packet;
	tcp_header_init(&send_packet.header, 0, 0, 0, 1, 0);
	tcp_packet_init(&send_packet, NULL, 0);

	//packet_timeout p_timeout;
	if (send_and_timeout(p_timeout, &send_packet, sock->sockfd, sock->si_other) < 0)
	{
		die("Coudln't send SYN");
	}
	print_SEND(0, 0, 1, 0);

	tcp_packet recv_packet;
	if (recv_tcp_packet(&recv_packet, sock->sockfd, sock->si_other) < 0 || !recv_packet.header.syn)
	{
		die("Couldn't receive SYN ACK");
	}
	p_timeout.has_been_acked = 1;

	// don't need to print receiving SYN ACK

	sock->cur_ack_num = recv_packet.header.seq_num + recv_packet.data_len;

	tcp_header_init(&send_packet.header, 0, sock->cur_ack_num, 1, 0, 0);
	tcp_packet_init(&send_packet, fname, strlen(fname));

	if (send_and_timeout(p_timeout, &send_packet, sock->sockfd, sock->si_other) < 0)
	{
		die("Couldn't send filename");
	}
	print_SEND(sock->cur_ack_num, 0, 0, 0);


	return 0;	
}

int recv_file(socket_info *sock, tcp_packet *fin_packet, packet_timeout *p_timeout)
{
	// do some stuff
	int fd = open(OUTPUT_FILE, O_WRONLY);

	tcp_packet window[WINDOW_SIZE];

	int base = 0;
	int end = 4;

    int base_ack_num = sock->cur_ack_num;

    int first_packet_received = 1;

	while (1)
    {
		tcp_packet recv_packet;
		recv_tcp_packet(&recv_packet, sock->sockfd, sock->si_other);
        print_RECV(recv_packet.header.seq_num);

        // the filename packet is ACKed on receiving first packet
        if (first_packet_received)
        {
            p_timeout->has_been_acked = 1;
            first_packet_received = 0;
        }

		// Check for connection close
		if (recv_packet.header.fin)
        {
			tcp_header_init(&fin_packet->header, 0, recv_packet.header.seq_num, 1, 0, 1);
			tcp_packet_init(fin_packet, NULL, 0);
			break;
		}
        else
        {
            // Process data (adjust window, write to file if needed)
            



            // need to determine if ACK to be sent is a retransmission

    		// create and send ACK
    		tcp_packet ack_packet;
    		tcp_header_init(&ack_packet.header, 0, recv_packet.header.seq_num, 1, 0, 0);
    		tcp_packet_init(&ack_packet, NULL, 0);
    		send_tcp_packet(&ack_packet, sock->sockfd, sock->si_other);

            // print send
    		

        }
        
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

    packet_timeout p_timeout; //used for the ack of the filename from the first data packet

    if (handshake(&sock, file_name, &p_timeout) < 0) {
    	die("Couldn't complete handshake");
    }

    tcp_packet fin_packet;
    if (recv_file(&sock, &fin_packet, &p_timeout) < 0) {
    	die("Couldn't receive file");
    }

	if (close_connection(&sock, &fin_packet) < 0) {
		die("Couldn't close connection");
	}

    return 0;
}
