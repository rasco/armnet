#ifndef ARP_H
#define ARP_H

typedef struct arp_packet
{
    unsigned short htype;
    unsigned short ptype;
    unsigned char hlen;
    unsigned char plen;
    unsigned short oper;
    char sha[6];
    unsigned int spa;
    char tha[6];
    unsigned int tpa;
} __attribute__((packed)) arp_packet;

struct arp_entry 
{
    struct arp_entry *next;
    unsigned int ip;
    unsigned char hw[6];
};

struct arp_entry arp_table;

struct arp_queue_entry
{
    struct arp_queue_entry *next;
    char data[2048];
    int len;
};

struct arp_queue_entry arp_queue;

void arp_recv(char *buf, int len);
int arp_resolve(char *data, unsigned int length, char *dst);

#endif
