/*
    Simple udp client
*/
#include <stdio.h> //printf
#include <string.h> //memset
#include <stdlib.h> //exit(0);
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include "tcp.h"
 
#define SERVER "127.0.0.1"
#define BUFLEN 512  //Max length of buffer
#define PORT 8888   //The port on which to send data
 
void die(char *s)
{
    perror(s);
    exit(1);
}





 
int main(void)
{
    struct sockaddr_in si_other;
    int s, i, slen=sizeof(si_other);
    

    char buf[BUFLEN];
    char message[BUFLEN];
 
    if ( (s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
        die("socket");
    }
 
    memset((char *) &si_other, 0, sizeof(si_other));
    si_other.sin_family = AF_INET;
    si_other.sin_port = htons(PORT);
     
    if (inet_aton(SERVER , &si_other.sin_addr) == 0)
    {
        fprintf(stderr, "inet_aton() failed\n");
        exit(1);
    }
 
    tcp_header header;
    tcp_packet packet;

    while(1)
    {
         
         
        //receive a reply and print it
        //clear the buffer by filling null, it might have previously received data
        //memset(buf,'\0', BUFLEN);
        
        tcp_header_init(&header, PORT, PORT, 2, 4, 1, 0, 0);
        tcp_packet_init(&packet, &header, (void *) buf, strlen(buf));

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

*/

    }
 
    close(s);
    return 0;
}
