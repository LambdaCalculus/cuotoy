
//http://www.ccs.neu.edu/home/amislove/teaching/cs4700/fall09/handouts/project1-primer.pdf

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <netdb.h>
#include "dnsparse.h"

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
    printf(" answer:%s type:%02X class:%02X offset:%ld\n", domain, qdtype, qdclass, b-buffer);
    // print_hex(b, n+buffer-b);
    int ttl = ntohs(*(uint32_t *)b); b+=4;
    uint16_t len = ntohs(*(uint16_t *)b); b +=2;
    uint8_t ip[4];
    ip[0]=*b; b +=1;
    ip[1]=*b; b +=1;
    ip[2]=*b; b +=1;
    ip[3]=*b; b +=1;
    printf("ttl:%d len:%d %d:%d:%d:%d %ld\n", ttl, len, ip[0], ip[1], ip[2], ip[3], b-buffer);
  }
  if(n==b-buffer)
    printf("success\n");
}

void print_hex(char *buffer, size_t n) {
  for (int i = 0; i < n; i++)
  {
    printf("%02X", buffer[i]);
  }
  printf("\n");
}
