#include <stdio.h> //printf
#include <string.h> //memset
#include <stdlib.h> //exit(0);
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <sys/stat.h>
#include "tcp.h"

#define BILLION 1000000000
#define TIMEOUT_IN_US 500000
 
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


int handshake(socket_info *sock, tcp_packet *first_packet)
{

    // Receive packet from the client
    tcp_packet client_packet;

    if (recv_tcp_packet(&client_packet, sock->sockfd, sock->si_other) < 0 || !client_packet.header.syn) {
      die("Couldn't receive SYN\n");
    }

    // Send response
    tcp_packet send_packet;
    tcp_header_init(&send_packet.header, sock->cur_seq_num, 0, 1, 1, 0);
    tcp_packet_init(&send_packet, NULL, 0);

    while (1)
    {
        send_tcp_packet(&send_packet, sock->sockfd, sock->si_other);

        fd_set set;
        struct timeval timeout = {0, TIMEOUT_IN_US};

        // select, recv
        FD_ZERO(&set);
        FD_SET(sock->sockfd, &set);

        if (select(sock->sockfd+1, &set, NULL, NULL, &timeout) >= 1) {
            recv_tcp_packet(first_packet, sock->sockfd, sock->si_other);
            break;
        }
    }

    // For some reason you have to do this
    sock->cur_seq_num++;


    return 0;
}

int send_file(socket_info *sock, char *fname)
{
	int fd = open(fname, O_RDONLY);
	int i;

	if (fd < 0)
	{
	    die("Couldn't Open File");
	}

	struct stat fileStat;
    fstat(fd, &fileStat);
    long fileSize = fileStat.st_size;

    char *buffer = (char *) malloc(sizeof(char) * fileSize);
    int bytes_read = read(fd, buffer, fileSize);
    if (bytes_read < 0)
    {
    	die("Couldn't Read File");
    }


    int packets_to_send = fileSize / TCP_MAX_DATA_LEN;
    if (fileSize % TCP_MAX_DATA_LEN != 0)
    	packets_to_send += 1;

    int *acked = (int *) malloc(sizeof(int) * packets_to_send);
    int *sent = (int *) malloc(sizeof(int) * packets_to_send);
    int *expected_acks = (int *) malloc(sizeof(int) * packets_to_send);

    for (i = 0; i < packets_to_send; i++)
    {
    	acked[i] = 0;
    	sent[i] = 0;
    	expected_acks[i] = 0;
    }

    int base = 0;

    long base_byte_position = 0;

    int base_seq_num = sock->cur_seq_num;

    while (base < packets_to_send)
    {
        int packets_sent = 0;

    	for (i = 0; i < 5; i++)
    	{
    		if (!acked[base + i])
    		{
    			int seq_num_to_use = (base_seq_num + (base + i) * TCP_MAX_DATA_LEN) % MAX_SEQ_NUM;
	    		tcp_packet send_packet;
	    		tcp_header_init(&send_packet.header, seq_num_to_use, sent[base + i], 1, 0, 0);
                
                int byte_offset = base_byte_position + i * TCP_MAX_DATA_LEN;
                int data_len = (fileSize - byte_offset < TCP_MAX_DATA_LEN) ? fileSize - byte_offset : TCP_MAX_DATA_LEN;
	    		tcp_packet_init(&send_packet, (buffer+byte_offset), data_len);

	    		// send
	    		send_tcp_packet(&send_packet, sock->sockfd, sock->si_other);
                print_SEND(send_packet.header.seq_num, sent[base + i], 0, 0);

	    		sent[base + i] = 1;
	    		expected_acks[base + i] = seq_num_to_use;
                packets_sent++;
	    	}
    	}

    	// select, recv
    	fd_set set;
		struct timeval timeout = {0, TIMEOUT_IN_US};

        int packets_acked = 0;

        while (packets_acked < packets_sent)
        {
    		FD_ZERO(&set);
    		FD_SET(sock->sockfd, &set);

    		if (select(sock->sockfd+1, &set, NULL, NULL, &timeout) < 1) {
    			break;
    		}

            tcp_packet recv_packet;
            recv_tcp_packet(&recv_packet, sock->sockfd, sock->si_other);
            print_RECV(recv_packet.header.ack_num);

            for (i = 0; i < 5; i++)
            {
                if (!acked[base + i])
                {
                    if (recv_packet.header.ack_num == expected_acks[base + i])
                    {
                        acked[base + i] = 1;
                        packets_acked++;
                        break;
                    }
                }
            }
        }

    	// advance window
        while (acked[base])
        {
            base++;
            base_byte_position += TCP_MAX_DATA_LEN;
        }

    }

    sock->cur_seq_num = (base_seq_num + fileSize) % MAX_SEQ_NUM;

	return 0;
}

int close_connection(socket_info *sock)
{
    tcp_packet fin_packet;
    tcp_header_init(&fin_packet.header, sock->cur_seq_num, 0, 0, 0, 1);
    tcp_packet_init(&fin_packet, NULL, 0);

    int is_retransmission = 0;

    fd_set set;
    struct timeval timeout;

    do {
        send_tcp_packet(&fin_packet, sock->sockfd, sock->si_other);
        print_SEND(fin_packet.header.seq_num, is_retransmission, 0, 1);

        timeout.tv_sec = 0;
        timeout.tv_usec = TIMEOUT_IN_US;

        FD_ZERO(&set);
        FD_SET(sock->sockfd, &set);
    } while (select(sock->sockfd+1, &set, NULL, NULL, &timeout) < 1);

    tcp_packet fin_ack_packet;
    recv_tcp_packet(&fin_ack_packet, sock->sockfd, sock->si_other);

	if (!(fin_ack_packet.header.fin && fin_ack_packet.header.ack)) {
		printf("NOTE: Client closed connection incorrectly - did not receive FIN-ACK\n");
	}

	tcp_packet ack_packet;
	tcp_header_init(&ack_packet.header, 0, 0, 1, 0, 0);
	tcp_packet_init(&ack_packet, NULL, 0);

    timeout.tv_sec = 0;
    timeout.tv_usec = 2 * TIMEOUT_IN_US;

    FD_ZERO(&set);
    FD_SET(sock->sockfd, &set);

    do {
        send_tcp_packet(&ack_packet, sock->sockfd, sock->si_other);
    } while (select(sock->sockfd+1, &set, NULL, NULL, &timeout) >= 1);

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

while(1) {

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

		
	close(sockfd);

}

    return 0;
}
