#ifndef UDP_H
#define UDP_H

#include "ip.h"

#define UDP_MAXBUFFER 32

typedef struct udp_header
{
    unsigned short udp_srcport;
    unsigned short udp_dstport;
    unsigned short udp_len;
    unsigned short udp_cksum;
} udp_header;

struct udp_buffer
{
    struct udp_buffer *next;
    char buf[2048];
    unsigned int len;
};

struct udp_sock
{
    struct udp_sock *last;
    struct udp_sock *next;
    unsigned int faddr;
    unsigned short fport;
    unsigned int laddr;
    unsigned short lport;
    struct udp_buffer buffer;
    int id;
};

void    udp_input(char *buf, int len);
int     udp_output(int socket, struct sockaddr *dst, char *buf, 
            unsigned int len);
int     udp_recvfrom(int socket, struct sockaddr *src, char *buf, 
            unsigned int len);
int     udp_bind(int socket, struct sockaddr *addr);
int     udp_socket();
int     udp_close(int socket);

#endif

