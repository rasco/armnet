#ifndef IP_H
#define IP_H

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;

int ip_opt_address;
int ip_opt_netmask;
int ip_opt_gateway;

typedef struct ip_header 
{
    uint8_t        ip_hlv;
    uint8_t        ip_tos;
    uint16_t       ip_len;
    uint16_t       ip_id;
    uint16_t       ip_off;
    uint8_t        ip_ttl;
    uint8_t        ip_p;
    uint16_t       ip_sum;
    unsigned int   ip_src;
    unsigned int   ip_dst;
} __attribute__((packed)) ip_header;

struct sockaddr
{
    int family;
    unsigned int addr;
    unsigned short port;
};

void            ip_input(char *buf, int len);
int             ip_output(unsigned int dest, unsigned char prot, char *data, 
                    unsigned int length);

#endif
