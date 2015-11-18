/*
 * udpserver.c - A simple UDP echo server
 * usage: udpserver <port>
 */
//http://www.ccs.neu.edu/home/amislove/teaching/cs4700/fall09/handouts/project1-primer.pdf


#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


#pragma pack (1)
typedef struct dnsheader {
    uint16_t id, flags;
    uint16_t qdcount, ancount, nscount, arcount;
} dnsheader;
#pragma pack ()
void print_hex(char *buffer, size_t n) ;

int parse_label(char *buffer, char *label){
  char *b = buffer;
  uint32_t c = *b++;
  int index = 0;
  while (c!=0) {
    if(c>192){
      b--;
      uint16_t offset = ntohs(*(uint16_t *)b&~(1<<16)&~(1<<15));
      parse_label(b-offset, &label[index]);
      return b+2-buffer;
    }
    for (size_t ii = 0; ii < c; ii++) {
      label[index++] = *b++;
    }
    label[index++] = '.';
    c = *b++;
  }
  label[index-1] = '\0';
  return b-buffer;
}

void parse_dns(char *buffer, size_t n) {
  dnsheader *buf = (dnsheader *)buffer;
  dnsheader header;
  char domain[256];
  header.id = buf->id;
  header.flags = buf->flags;
  header.qdcount = ntohs(buf->qdcount);
  header.ancount = ntohs(buf->ancount);
  header.nscount = ntohs(buf->nscount);
  header.arcount = ntohs(buf->arcount);
  printf("Header id:%d, flags:%d, qdcount:%d, ancount:%d, nscount:%d, arcount:%d\n",
    header.id, header.flags, header.qdcount, header.ancount, header.nscount, header.arcount);
  char *b = buffer + sizeof(dnsheader);
  print_hex(b, n+buffer-b);
  for(int i=0; i<header.qdcount; ++i){
    b += parse_label(b, domain);
    uint16_t qdtype, qdclass;
    qdtype = ntohs(*(uint16_t *)b); b +=2;
    qdclass = ntohs(*(uint16_t *)b); b +=2;
    printf(" question:%s type:%02X class:%02X offset:%ld\n", domain, qdtype, qdclass, b-buffer);
  }
  print_hex(b, n+buffer-b);
  for(int i=0; i<header.ancount; ++i){
    b += parse_label(b, domain);
    uint16_t qdtype, qdclass;
    qdtype = ntohs(*(uint16_t *)b); b +=2;
    qdclass = ntohs(*(uint16_t *)b); b +=2;
    printf(" question:%s type:%02X class:%02X offset:%ld\n", domain, qdtype, qdclass, b-buffer);
    print_hex(b, n+buffer-b);
    int ttl = ntohs(*(uint32_t *)b); b+=4;
    uint16_t len = ntohs(*(uint16_t *)b); b +=2;
    uint16_t len2 = ntohs(*(uint16_t *)b); b +=2;
    printf(" question:%s type:%02X class:%02X offset:%ld\n", domain, qdtype, qdclass, b-buffer);
    printf("domain:%s ttl:%d len:%d %d %s %ld\n", domain, ttl, len, len2, b+5, b-buffer);
    print_hex(b, n+buffer-b);
  }
}

void print_hex(char *buffer, size_t n) {
  for (int i = 0; i < n; i++)
  {
    printf("%02X", buffer[i]);
  }
  printf("\n");
}



#define BUFSIZE 1024

/*
 * error - wrapper for perror
 */
void error(char *msg) {
  perror(msg);
  exit(1);
}

int main(int argc, char **argv) {
  int sockfd; /* socket */
  int portno; /* port to listen on */
  unsigned int clientlen; /* byte size of client's address */
  struct sockaddr_in serveraddr; /* server's addr */
  struct sockaddr_in clientaddr; /* client addr */
  struct hostent *hostp; /* client host info */
  char buf[BUFSIZE]; /* message buf */
  char *hostaddrp; /* dotted decimal host addr string */
  int optval; /* flag value for setsockopt */
  int n; /* message byte size */

  int sendsockfd; /* socket */
  struct sockaddr_in googleaddr;
  unsigned int googlelen; /* byte size of client's address */

  /*
   * check command line arguments
   */
  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    // exit(1);
  }
  // portno = atoi(argv[1]);
  portno = 53;

  /*
   * socket: create the parent socket
   */
  sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0)
    error("ERROR opening socket");
  sendsockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sendsockfd < 0)
    error("ERROR opening socket");
  bzero((char *) &googleaddr, sizeof(googleaddr));
  googleaddr.sin_family = AF_INET;
  inet_aton("114.114.114.114", &googleaddr.sin_addr); // store IP in antelope
  googleaddr.sin_port = htons(53);
  /* setsockopt: Handy debugging trick that lets
   * us rerun the server immediately after we kill it;
   * otherwise we have to wait about 20 secs.
   * Eliminates "ERROR on binding: Address already in use" error.
   */
  optval = 1;
  setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR,
	     (const void *)&optval , sizeof(int));

  /*
   * build the server's Internet address
   */
  bzero((char *) &serveraddr, sizeof(serveraddr));
  serveraddr.sin_family = AF_INET;
  serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
  serveraddr.sin_port = htons((unsigned short)portno);

  /*
   * bind: associate the parent socket with a port
   */
  if (bind(sockfd, (struct sockaddr *) &serveraddr,
	   sizeof(serveraddr)) < 0)
    error("ERROR on binding");

  /*
   * main loop: wait for a datagram, then echo it
   */
  clientlen = sizeof(clientaddr);
  while (1) {

    /*
     * recvfrom: receive a UDP datagram from a client
     */
    bzero(buf, BUFSIZE);
    n = recvfrom(sockfd, buf, BUFSIZE, 0,
		 (struct sockaddr *) &clientaddr, &clientlen);
    if (n < 0)
      error("ERROR in recvfrom");

    /*
     * gethostbyaddr: determine who sent the datagram
     */
    hostp = gethostbyaddr((const char *)&clientaddr.sin_addr.s_addr,
			  sizeof(clientaddr.sin_addr.s_addr), AF_INET);
    if (hostp == NULL)
      error("ERROR on gethostbyaddr");
    hostaddrp = inet_ntoa(clientaddr.sin_addr);
    if (hostaddrp == NULL)
      error("ERROR on inet_ntoa\n");
    printf("server received datagram from %s (%s)\n",
	   hostp->h_name, hostaddrp);
    printf("server received %lu/%d bytes: %s\n", strlen(buf), n, buf);
    print_hex(buf, n);
    parse_dns(buf, n);



      /* send the message to the server */
      googlelen = sizeof(googleaddr);
      n = sendto(sendsockfd, buf, n, 0, (struct sockaddr *)&googleaddr, googlelen);
      if (n < 0)
        error("ERROR in sendto");

      bzero(buf, BUFSIZE);

      /* print the server's reply */
      n = recvfrom(sendsockfd, buf, BUFSIZE, 0, (struct sockaddr *)&googleaddr, &googlelen);
      if (n < 0)
        error("ERROR in recvfrom");
      printf("Echo from server: %d %s\n", n, buf);
      print_hex(buf, n);
      parse_dns(buf, n);

      /*
       * sendto: echo the input back to the client
       */
      n = sendto(sockfd, buf, n, 0,
  	       (struct sockaddr *) &clientaddr, clientlen);
      if (n < 0)
        error("ERROR in sendto");

  }
}
