#ifndef ETHERNET_H
#define ETHERNET_H

#define RBQ_LEN 1024

char ethernet_opt_address[6];

typedef struct rbq_elem
{
    unsigned int word1;
    unsigned int word2;
} __attribute__((packed)) rbq_elem;

rbq_elem rbq[RBQ_LEN] __attribute__ ((aligned (1024)));

typedef struct ethernet_header
{
    char dest_addr[6];
    char src_addr[6];
    char ether_type[2];
} __attribute__((packed)) ethernet_header;

void ethernet_setup_rbq(void);
void ethernet_irq(void);
void ethernet_recv();
void ethernet_send(char *data, unsigned int length, char *dst, 
                   char *type);

#endif
