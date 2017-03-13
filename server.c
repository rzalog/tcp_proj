#include <stdio.h> //printf
#include <string.h> //memset
#include <stdlib.h> //exit(0);
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include "tcp.h"

#define SERVER "127.0.0.1"
 
#define BUFLEN 512  //Max length of buffer
#define PORT 8888   //The port on which to listen for incoming data

//#define PORT 8888   //The port on which to listen for incoming data
 
void die(char *s)
{
    perror(s);
    exit(1);
}

f_socket* f_accept(f_socket *sockfd, struct sockaddr_in *addr, socklen_t *addrlen)
{
  // Configure our socket (because we know it is a server now)
  sockfd->cur_seq_num = SERVER_DEFAULT_SEQ_NUM;

  // Receive packet from the client
  tcp_packet client_packet;
  if (f_read_packet(sockfd, &client_packet) < 0 || !client_packet.syn) {
    return NULL;
  }

  // Send response
  tcp_packet send_packet;
  if (f_write_packet(sockfd, &send_packet, NULL, 0, 1, 1, 0) < 0) {
    return NULL;
  }

  // Get back the ACK (assume no data being sent initially)
  if (f_read_packet(sockfd, &client_packet) < 0 || !client_packet.syn) {
    return NULL;
  }

  return sockfd;
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
     
    int s, i, slen = sizeof(si_other) , recv_len;
    char buf[] = "this is some data";
     
    //create a UDP socket
    if ((s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
        die("socket");
    }
     
    // zero out the structure
    memset((char *) &si_me, 0, sizeof(si_me));
     
    si_me.sin_family = AF_INET;
    si_me.sin_port = htons(portno);
    si_me.sin_addr.s_addr = htonl(INADDR_ANY);
/*

    memset((char *) &si_other, 0, sizeof(si_other));
    si_other.sin_family = AF_INET;
    si_other.sin_port = htons(PORT);
     
    if (inet_aton(SERVER , &si_other.sin_addr) == 0)
    {
        fprintf(stderr, "inet_aton() failed\n");
        exit(1);
    }
    */
     
    //bind socket to port
    if( bind(s , (struct sockaddr*)&si_me, sizeof(si_me) ) == -1)
    {
        die("bind");
    }
     
    tcp_header header;
    tcp_packet packet;

    tcp_header_init(&packet.header, PORT, PORT, 0, 6, 1, 0, 0);
    tcp_packet_init(&packet, (void *) buf, strlen(buf));
    //tcp_header_init(&header, PORT, PORT, 0, 6, 1, 0, 0);
    //tcp_packet_init(&packet, &header, (void *) buf, strlen(buf));

    //keep listening for data
    while(1)
    {
        /*
        printf("Waiting for data...");
        fflush(stdout);
	memset(buf,'\0', BUFLEN);
         
        //try to receive some data, this is a blocking call
        if ((recv_len = recvfrom(s, buf, BUFLEN, 0, (struct sockaddr *) &si_other, &slen)) == -1)
	//if ((recv_len = read(s, buf, BUFLEN)) == -1)
        {
            die("recvfrom()");
        }
         
        //print details of the client/peer and the data received
        printf("Received packet from %s:%d\n", inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port));
        printf("Data: %s\n" , buf);
         */

        recv_tcp_packet(&packet, s, &si_other);

        tcp_header_init(&packet.header, PORT, PORT, 0, 6, 1, 0, 0);
        tcp_packet_init(&packet, (void *) buf, strlen(buf));

        int n = send_tcp_packet(&packet, s, &si_other);


        if (n < 0)
        {
            die("sendto()");
        }

    }
 
    close(s);
    return 0;
}
