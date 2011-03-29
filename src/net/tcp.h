#ifndef TCP_H
#define TCP_H

#include "ip.h"
#include "../types.h"

#define TCP_TIMEOUT 100
#define TCP_SYNSENT_TIMEOUT 5
#define TCP_SYNSENT_RETRIES 3
#define TCP_RETRANSMISSION_TIMEOUT 5
#define TCP_SNDBUF_MAXSIZE 1024*10
#define TCP_RECVBUF_MAXSIZE 80

int tcp_timeout;
int tcp_iss;

enum 
{
    TCP_HDR_CTRL_URG = 32,
    TCP_HDR_CTRL_ACK = 16,
    TCP_HDR_CTRL_PSH = 8,
    TCP_HDR_CTRL_RST = 4,
    TCP_HDR_CTRL_SYN = 2,
    TCP_HDR_CTRL_FIN = 1
};

enum
{
    TCP_ERR_CONNRESET = 1,
    TCP_ERR_CONNREFUSED,
    TCP_ERR_OTHER
};

typedef struct tcp_header
{
    unsigned short srcport,
                   dstport;
    unsigned int   seqnr,
                   acknr;
    unsigned short offsetcontrol,
                   window,
                   checksum,
                   urgentp;
} __attribute__((packed)) tcp_header;

struct tcp_buffer
{
    struct tcp_buffer *next;
    unsigned int seqnr;
    unsigned int len;
    char *data;
};

struct tcp_sock
{
    struct tcp_sock     *last;
    struct tcp_sock     *next;
    
    int                 id;
    
    unsigned int        faddr;
    unsigned short      fport;
    unsigned int        laddr;
    unsigned short      lport;
    
    struct tcp_buffer   snd_buf, 
                        rcv_buf,
                        rcv_outoforder_buf;
    
                        // Send sequence variables
    unsigned int        snd_una, snd_nxt, snd_wnd, snd_up, snd_wl1, snd_wl2, 
                        snd_iss;
    
                        // Receive sequence variables
    unsigned int        rcv_nxt, rcv_wnd, rcv_up, rcv_irs;
    
    unsigned int        mss;
    
    enum                {CLOSED, LISTEN, SYN_SENT, SYN_RCVD, ESTABLISHED, 
                        FIN_WAIT_1, FIN_WAIT_2, CLOSE_WAIT, CLOSING, LAST_ACK, 
                        TIME_WAIT} state;
    
    unsigned int        nextbyte, 
                        bytesbuffered,
                        sndbuf_size;
    
    unsigned int        synsent_timeout,
                        synsent_retries,
                        retransmission_timeout,
                        lastack_timeout,
                        timewait_timeout,
                        synrcvd_timeout;
    
    BOOL                listener;
    BOOL                closedbyuser;
    
    int                 err;
};

void tcp_timer();
void tcp_input(char *buf, int len);
void tcp_output(struct tcp_sock *s, tcp_header *header, char *options,
    char *data, int len);
int tcp_connect(int socket, struct sockaddr *addr);
int tcp_send(int socket, char *buf, unsigned int len);
int tcp_recv(int socket, char *buf, unsigned int len);
int tcp_bind(int socket, struct sockaddr *addr);
int tcp_socket();
int tcp_close(int socket);
int tcp_listen(int socket);

#endif

