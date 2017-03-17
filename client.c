#include <stdio.h> //printf
#include <string.h> //memset
#include <stdlib.h> //exit(0);
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>
#include <time.h>
#include "tcp.h"

#define BILLION 1000000000
#define TIMEOUT_IN_US 500000

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


int handshake(socket_info *sock)
{
	tcp_packet syn_packet;
    tcp_header_init(&syn_packet.header, 0, 0, 0, 1, 0);
    tcp_packet_init(&syn_packet, NULL, 0);

	int is_retransmission = 0;

	fd_set set;
	struct timeval timeout;

    do {
        send_tcp_packet(&syn_packet, sock->sockfd, sock->si_other);
        print_SEND(0, is_retransmission, 1, 0);

        timeout.tv_sec = 0;
        timeout.tv_usec = TIMEOUT_IN_US;

        FD_ZERO(&set);
        FD_SET(sock->sockfd, &set);
    } while (select(sock->sockfd+1, &set, NULL, NULL, &timeout) < 1);

    tcp_packet syn_ack_packet;
    recv_tcp_packet(&syn_ack_packet, sock->sockfd, sock->si_other);

    sock->cur_ack_num = syn_ack_packet.header.seq_num + syn_ack_packet.data_len + 1;

	return 0;	
}

int recv_file(socket_info *sock, char *fname, tcp_packet * fin_packet)
{	
	// send filename to server
	tcp_packet filename_packet;
    tcp_header_init(&filename_packet.header, 0, sock->cur_ack_num, 1, 0, 0);
	tcp_packet_init(&filename_packet, fname, strlen(fname));

	int is_retransmission = 0;

	fd_set set;
	struct timeval timeout;

	// retransmit if timeout
	do {
        send_tcp_packet(&filename_packet, sock->sockfd, sock->si_other);
        print_SEND(0, is_retransmission, 1, 0);

        timeout.tv_sec = 0;
        timeout.tv_usec = TIMEOUT_IN_US;

        FD_ZERO(&set);
        FD_SET(sock->sockfd, &set);
    } while (select(sock->sockfd+1, &set, NULL, NULL, &timeout) < 1);


	// set up file receiving
	int fd = open(OUTPUT_FILE, O_WRONLY|O_CREAT|O_TRUNC,0640);

	tcp_packet window[WINDOW_SIZE];
	int window_slot_filled[WINDOW_SIZE];
	memset(window_slot_filled, 0, WINDOW_SIZE * sizeof(int));

	unsigned int base = 0;

	unsigned int index = 0;

    unsigned int base_seq_num = sock->cur_ack_num;
	unsigned int pseudo_seq_num;

    unsigned int first_packet_received = 1;

    unsigned int wrap_arounds = 0;

    int wrap_around_flag = 1;
    is_retransmission = 0;

	int done = 0;

	while (1)
    {	
    	// help to determine the 'pseudo-sequence number'
    	if ( (base_seq_num % MAX_SEQ_NUM) > (MAX_SEQ_NUM - RECV_WNDW_DEFAULT) && wrap_around_flag)
    	{
    		wrap_arounds += 1;
    		wrap_around_flag = 0;
    	}
    	else if ( (base_seq_num % MAX_SEQ_NUM) < (MAX_SEQ_NUM - RECV_WNDW_DEFAULT))
    	{
    		wrap_around_flag = 1;
    	}

		tcp_packet recv_packet;
		recv_tcp_packet(&recv_packet, sock->sockfd, sock->si_other);
        print_RECV(recv_packet.header.seq_num);
        is_retransmission = 0;

        
        // create pseudo-sequence number
        if (recv_packet.header.seq_num < MAX_SEQ_NUM - RECV_WNDW_DEFAULT)
        {
        	pseudo_seq_num = recv_packet.header.seq_num + MAX_SEQ_NUM * wrap_arounds;
        }
        else
        {
        	pseudo_seq_num = recv_packet.header.seq_num + MAX_SEQ_NUM * (wrap_arounds-1);
        }

		// Check for connection close
		if (recv_packet.header.fin)
        {
			tcp_header_init(&fin_packet->header, 0, recv_packet.header.seq_num, 1, 0, 1);
			tcp_packet_init(fin_packet, NULL, 0);
			done = 1;
			break;
		}
        else
        {
            // Process data (adjust window, write to file if needed)
            
            if (pseudo_seq_num >= base_seq_num)
            {
            	// get packet's index in window buffer relative to base
            	index = ( (pseudo_seq_num - base_seq_num));
				index = index/TCP_MAX_DATA_LEN;
				index = index + base;
				index = index % WINDOW_SIZE;
				
		// have not received packet yet
            	if (window_slot_filled[index] == 0)
            	{
					memcpy(&window[index], &recv_packet, sizeof(tcp_packet));
					window_slot_filled[index] = 1;
				}
				else
				{
					is_retransmission = 1;
				}

            }
            else
            {
            	// ignore packet bc before window
            	// this is a retransmission
            	is_retransmission = 1;
            }

            // write to file
		if (!done) {
		    while (window_slot_filled[base] == 1)
		    {
		    	int bytes_written = write(fd, window[base].data, window[base].data_len);

		    	base_seq_num = base_seq_num + window[base].data_len;

		    	window_slot_filled[base] = 0;
		    	base = (base + 1)%5;
		    }

			fsync(fd);

		    // need to determine if ACK to be sent is a retransmission

	    		// create and send ACK
	    		tcp_packet ack_packet;
	    		tcp_header_init(&ack_packet.header, 0, recv_packet.header.seq_num, 1, 0, 0);
	    		tcp_packet_init(&ack_packet, NULL, 0);
	    		send_tcp_packet(&ack_packet, sock->sockfd, sock->si_other);

		    // print send
	    		print_SEND(recv_packet.header.seq_num, is_retransmission, 0, 0);

		}
		}
        
	}

	return 0;
}

int close_connection(socket_info *sock, tcp_packet *fin_packet)
{
	// FIN-ACK procedure
	tcp_packet fin_ack_packet;
	tcp_header_init(&fin_ack_packet.header, 0, fin_packet->header.seq_num, 1, 0, 1);
	tcp_packet_init(&fin_ack_packet, NULL, 0);

	int is_retransmission = 0;

	fd_set set;
	struct timeval timeout;

    do {
        send_tcp_packet(&fin_ack_packet, sock->sockfd, sock->si_other);
        print_SEND(fin_ack_packet.header.seq_num, is_retransmission, 0, 1);

        timeout.tv_sec = 0;
        timeout.tv_usec = TIMEOUT_IN_US;

        FD_ZERO(&set);
        FD_SET(sock->sockfd, &set);
    } while (select(sock->sockfd+1, &set, NULL, NULL, &timeout) < 1);

	tcp_packet final_ack_packet;
	recv_tcp_packet(&final_ack_packet, sock->sockfd, sock->si_other);

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


    if (handshake(&sock) < 0) {
    	die("Couldn't complete handshake");
    }

    tcp_packet fin_packet;
    if (recv_file(&sock, file_name, &fin_packet) < 0) {
    	die("Couldn't receive file");
    }

	if (close_connection(&sock, &fin_packet) < 0) {
		die("Couldn't close connection");
	}

    return 0;
}
