#include <stdio.h> //printf
#include <string.h> //memset
#include <stdlib.h> //exit(0);
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <time.h>
#include "tcp.h"

#define BILLION 1000000000
#define TIMEOUT_IN_NS 500000000
 
void die(char *s)
{
    perror(s);
    exit(1);
}

typedef struct {
	int cur_seq_num;
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
} packet_timeout;

void print_SEND(int seq_num, int retransmission, int syn, int fin)
{
    fprintf(stdout, "Sending packet %d 5120", seq_num);
    if (retransmission)
        fprintf(stdout, " Retransmission");
    if (syn)
        fprintf(stdout, " SYN");
    if (fin)
        fprintf(stdout, " FIN");
    fprintf(stdout, "\n");
}

void print_RECV(int ack_num)
{
    fprintf(stdout, "Receiving packet %d\n", ack_num);
}

void *timeout_check(void *p_timeout_)
{
	packet_timeout *p_timeout = (packet_timeout *) p_timeout_;

	while (!p_timeout->has_been_acked) {
		struct time_spec cur_time;
		clock_gettime(CLOCK_MONOTONIC, &cur_time);

		uint64_t diff = BILLION * (cur_time.tv_sec - time_stamp.tv_sec);
		diff += cur_time.tv_nsec - time_stamp.tv_nsec;

		if (diff > TIMEOUT_IN_NS) {
			int n 1;
			while (n != 0) {
				n = send_tcp_packet(p_timeout->packet, p_timeout->sockfd, p_timeout->dest_addr);
			}

			// Reset timestamp
			clock_gettime(CLOCK_MONOTONIC, &time_stamp);
		}
	}

	free(p_timeout);

	return NULL;
}

int send_and_timeout(packet_timeout *p_timeout, tcp_packet *send_packet, int sockfd, const struct sockaddr_in *dest_addr)
{
	// Just a simple wrapper function
	p_timeout = (packet_timeout *)malloc(sizeof(packet_timeout));
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

int handshake(socket_info *sock, tcp_packet *first_packet)
{
    // Receive packet from the client
    tcp_packet client_packet;

    if (recv_tcp_packet(&client_packet, sock->sockfd, sock->si_other) < 0 || !client_packet.header.syn) {
      die("Couldn't receive SYN");
    }

    // Send response
    tcp_packet send_packet;
    tcp_header_init(&send_packet.header, sock->cur_seq_num, 0, 1, 1, 0);
    tcp_packet_init(&send_packet, NULL, 0);

    packet_timeout *p_timeout;
    if (send_and_timeout(p_timeout, &send_packet, sock->sockfd, sock->si_other) < 0) {
      die("Couldn't send SYN ACK");
    }
    print_SEND(sock->cur_seq_num, 0, 1, 0);

    // receive ACK to complete handshake
    if (recv_tcp_packet(first_packet, sock->sockfd, sock->si_other) < 0) {
      die("Couldn't receive ACK to complete handshake");
    }
    p_timeout->has_been_acked = 1;

    // For some reason you have to do this
    sock->cur_seq_num++;

    return 0;
}

int send_file(socket_info *sock, char *fname)
{
	int fd = open(fname, O_RDONLY);

	if (fd < 0)
	{
	    die("Couldn't Open File");
	}

	tcp_packet window[WINDOW_SIZE]; // 5120 / 1024
	packet_timeout *p_timeouts[WINDOW_SIZE];

	int base = 0;
	int end = 4;
	int next = base;

	int packets_sent = 0;
	int acks_received = 0;

	int more_data = 1;

	while (more_data || (base != end))
	{
	  	while (next != (end + 1) % WINDOW_SIZE)
		{
			// Read from the file into temp buffer
			char buf[TCP_MAX_DATA_LEN];
			int bytes_read = read(fd, buf, TCP_MAX_DATA_LEN);

			tcp_header_init(&window[next].header, sock->cur_seq_num, 0, 1, 0, 0);
			tcp_packet_init(&window[next], (void *) buf, bytes_read);

			if (bytes_read < TCP_MAX_DATA_LEN) // read the last packet
			{
				more_data = 0;
			}

			if (send_tcp_packet(&window[next], sock->sockfd, sock->si_other) < 0)
			{
			  die("Sending data");
			}
			print_SEND(sock->cur_seq_num, 0, 0, 0);

			sock->cur_seq_num += bytes_read;
			//set time at next index in time array
			// OR even better, set an actual timeout... timer

			next = (next + 1) % WINDOW_SIZE;
		}

	// Receive ACK
	tcp_packet recv_packet;
	recv_tcp_packet(&recv_packet, sock->sockfd, sock->si_other);

	int ack = recv_packet.header.ack_num;
	print_RECV(ack);

	// Free the packet_timeout

	// Adjust window size
	if (window[base].header.seq_num + window[base].data_len == ack)
	{
		base += 1; // adjust up to last unacked packet
	}
	// adjust base
	if (more_data) {
	// adjust end
	}
	}
	return 0;
}

int close_connection(socket_info *sock)
{
	// FIN-ACK procedure
	tcp_packet fin_packet;
	tcp_header_init(&fin_packet.header, sock->cur_seq_num, 0, 0, 0, 1);
	tcp_packet_init(&fin_packet, NULL, 0);

	// This part is confusing, their description doesn't match
	//	the book's description of closing the connection

	close(sock->sockfd);
	return 0;
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        fprintf(stderr, "Error, no port provided\n");
        exit(1);
    }

    int portno = atoi(argv[1]);


    struct sockaddr_in si_me, si_other;
    int sockfd, i, slen = sizeof(si_other), recv_len;
     
    //create a UDP socket
    if ((sockfd=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
        die("socket");
    }
     
    // zero out the structure
    memset((char *) &si_me, 0, sizeof(si_me));
     
    si_me.sin_family = AF_INET;
    si_me.sin_port = htons(portno);
    si_me.sin_addr.s_addr = htonl(INADDR_ANY);

     
    //bind socket to port
    if( bind(sockfd, (struct sockaddr*)&si_me, sizeof(si_me) ) == -1)
    {
        die("bind");
    }

    socket_info sock;
    sock.cur_seq_num = SERVER_DEFAULT_SEQ_NUM;
    sock.sockfd = sockfd;
    sock.si_other = &si_other;

    while(1) {
	    tcp_packet first_packet;
	    if (handshake(&sock, &first_packet) < 0) {
	    	die("Couldn't complete handshake");
	    }

	    // get filename
	    char *fname = malloc(first_packet.data_len + 1);
	    memset(fname, '\0', first_packet.data_len + 1);
	    memcpy(fname, first_packet.data, first_packet.data_len);

	    if (send_file(&sock, fname) < 0) {
	    	die("Couldn't send file");
	    }

	    if (close_connection(&sock) < 0) {
	    	die("Couldn't close connection");
	    }
	}
	
    return 0;
}
