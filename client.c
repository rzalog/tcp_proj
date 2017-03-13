#include <stdio.h> //printf
#include <string.h> //memset
#include <stdlib.h> //exit(0);
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include "tcp.h"
 
 
void die(char *s)
{
    perror(s);
    exit(1);
}
/*
f_socket f_connect(f_socket *sockfd, struct sockaddr_in *addr, socklen_t *addrlen)
{
  // Configure socket for client
  sockfd->cur_seq_num = CLIENT_DEFAULT_SEQ_NUM;

  // Send initial packet to server
  tcp_packet send_packet;
  tcp_header_init(&send_packet.header, sockfd->src_port, addr->sin_port, sockfd->cur_seq_num, 0, 0, 1, 0);
  tcp_packet_init(&send_packet, NULL, 0);

  send_tcp_packet(&send_packet, sockfd->fd, addr);

  sockfd->

  // Receive response
  tcp_packet server_packet;
  recv_tcp_packet(&client_packet, sockfd->fd, sockfd->destaddr);

  if (!syn_flagged(server_packet.header.flags) || !ack_flagged(server_packet.header.flags)) {
    return NULL;
  }

  sockfd->cur_ack_num = server_packet.header.seq_num + 1;
  
  // Send back final ACK
}*/


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



 
int main(int argc, char *argv[])
{
    if (argc != 4)
    {
        fprintf(stderr,"ERROR, ./client < server hostname >< server portnumber >< filename >\n");
        exit(1);
    }

    char* server_host_name = argv[1];
    int server_port_no = atoi(argv[2]);
    char* file_name = argv[3];

    // create UDP socket
    struct sockaddr_in si_other;
    int sockfd, i, slen=sizeof(si_other);
 
    if ( (sockfd=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
        die("socket");
    }
 
    memset((char *) &si_other, 0, sizeof(si_other));
    si_other.sin_family = AF_INET;
    si_other.sin_port = htons(server_port_no);
     
    if (inet_aton(server_host_name , &si_other.sin_addr) == 0)
    {
        fprintf(stderr, "inet_aton() failed\n");
        exit(1);
    }
 


    /////////////////////////////////// HANDSHAKE

    int cur_ack_num;
    int cur_seq_num;

    // Send initial packet to server
    tcp_packet send_packet;
    tcp_header_init(&send_packet.header, CLIENT_DEFAULT_SEQ_NUM, 0, 0, 1, 0);
    tcp_packet_init(&send_packet, NULL, 0);

    if (send_tcp_packet(&send_packet, sockfd, &si_other) < 0)
    {
      die("Couldn't send SYN\n");
    }
    print_SEND(0, 0, 1, 0);


    tcp_packet recv_packet;

    if (recv_tcp_packet(&recv_packet, sockfd, &si_other) < 0 || !recv_packet.header.syn)
    {
      die("Couldn't receive SYN\n");
    }

    cur_ack_num = recv_packet.header.seq_num + recv_packet.data_len + 1;


    tcp_header_init(&send_packet.header, CLIENT_DEFAULT_SEQ_NUM, cur_ack_num, 1, 0, 0);
    tcp_packet_init(&send_packet, file_name, strlen(file_name));

    if (send_tcp_packet(&send_packet, sockfd, &si_other) < 0)
    {
      die("Couldn't send filename\n");
    }
    print_SEND(cur_ack_num, 0, 0, 0);

/*
    while(1)
    {
         
         
        //receive a reply and print it
        //clear the buffer by filling null, it might have previously received data
        //memset(buf,'\0', BUFLEN);
        
        tcp_header_init(&packet.header, PORT, PORT, 2, 4, 1, 0, 0);
        tcp_packet_init(&packet, (void *) buf, strlen(buf));

        int n = send_tcp_packet(&packet, s, &si_other);

        if (n < 0)
        {
            die ("sendto");
        }



        int bytes_received = recv_tcp_packet(&packet, s, &si_other);

        //try to receive some data, this is a blocking call
        //int n = recvfrom(s, packet, TCP_HEADER_LEN + TCP_MAX_DATA_LEN, 0, (struct sockaddr *) &si_other, &slen);

        if (bytes_received < 0)
        {
            die("recvfrom()");
        }

        //packet->data_len = n - TCP_HEADER_LEN;

        int ack_num;
        if (ack_flagged(packet.header.flags))
        {
            ack_num = packet.header.ack_num;
            printf("ack_num: %d\ndata length: %d\n", ack_num, packet.data_len);
        }
        
        printf("received something\n");


/*
        //send the message
        if (sendto(s, message, strlen(message) , 0 , (struct sockaddr *) &si_other, slen)==-1)
        {
            die("sendto()");
        }



    }
 */
    close(sockfd);
    return 0;
}
