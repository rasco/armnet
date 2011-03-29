#include "../board.h"
#include "ethernet.h"
#include "../lib.h"
#include "../kmem.h"
#include "ip.h"
#include "arp.h"

void ethernet_send(char *data, unsigned int length, char *dst, 
                   char *type)
{
    ethernet_header *header;
    char arpdst[6];
    
    if ( type[0] == 0x08 && type[1] == 0x00 ) // IP
    {
        if ( dst == NULL )
        {
            // If the IP stack didn't give us a destination, resolve it with ARP
            // If the destination isn't in the cache, arp_resolve will send a
            // request and queue the IP packet for sending later
            if ( arp_resolve(data, length, arpdst) )
                dst = arpdst;
            else 
                return;
        }
    }
    
    header = (ethernet_header*)data;
    
    memcpy(header->dest_addr, dst, 6);
    memcpy(header->src_addr, ethernet_opt_address, 6);
    
    header->ether_type[0] = type[0];
    header->ether_type[1] = type[1];
    
    //hexdump(data, length);
    
    if ( DEREF_INT(ADDR_ETH_BASE+ADDR_ETH_TSR) & 1<<4 )
    {
        DEREF_INT(ADDR_ETH_BASE+ADDR_ETH_TAR) = (unsigned int) data;
        DEREF_INT(ADDR_ETH_BASE+ADDR_ETH_TCR) = length;
        while(DEREF_INT(ADDR_ETH_BASE+ADDR_ETH_TCR) > 0); 
    }
    else
        printf("EMAC: Not ready to transmit next frame\n");
}

// Passes the received forward to the appropriate handler
void ethernet_recv_frame(char *buf, int len)
{
    ethernet_header *head = (ethernet_header*)buf;
    
    dprintf("received frame of length %d\n", len);
    //printf("type: %02x%02x", head->ether_type[0], head->ether_type[1]);
    //if ( memcmp(ethernet_opt_address, head->dest_addr, 6) != 0 )
    //    return;
    
    // ARP
    if ( head->ether_type[0] == 0x08 && head->ether_type[1] == 0x06 )
    {
        arp_recv(buf, len);
    }
    // IP
    else if ( head->ether_type[0] == 0x08 && head->ether_type[1] == 0x00 )
    {
        // Pass over the IP packet without the ethernet header and checksum
        ip_input(buf+sizeof(ethernet_header), len-sizeof(ethernet_header)-4);
    }
}


// Handles all received frames by passing them to ethernet_recv_frame and
// clearing the ownership flag afterwards
void ethernet_recv()
{
    int i;
    char *buf;
    int len;
    
    for ( i = 0; i < RBQ_LEN; i++ )
    {
        if ( rbq[i].word1 & 1 ) // Software owns this buffer
        {
            len = (rbq[i].word2)&0x7ff;
            buf = (char*)(rbq[i].word1&0xfffffffc);
            
            ethernet_recv_frame(buf, len);
            
            rbq[i].word1 &= 0xFFFFFFFE; // Clear ownership flag
        }
    }
}


// This function gets called at every interrupt caused by the EMAC
void ethernet_irq()
{
    unsigned int status = DEREF_INT(ADDR_ETH_BASE+ADDR_ETH_ISR);
    dprintf("Ethernet Interrupt ETH_ISR: %x\n", status);
    //DEREF_INT(ADDR_ETH_BASE+ADDR_ETH_RSR) = 7;
    
    if ( status & 1 << 2 )
        DEREF_INT(ADDR_ETH_BASE+ADDR_ETH_RBQP) = (int)rbq;
    if ( status & 1<<1 || status & 1<<2 ) // Receive Complete
    {
        //printf("Receive complete\n");
        ethernet_recv();
    }
}


// Initial setup of Receive Buffer Queue
void ethernet_setup_rbq()
{
    int i;
    
    printf("rbq: %x\n", rbq);
    
    for ( i = 0; i < RBQ_LEN; i++ )
    {
        rbq[i].word1 = (int)((int*)kmem_alloc(2048));
        rbq[i].word2 = 0;
    }
    
    // Wrap-bit for the last element
    rbq[RBQ_LEN-1].word1 |= 1<<1;
    
    // Tell the EMAC the address of our queue
    DEREF_INT(ADDR_ETH_BASE+ADDR_ETH_RBQP) = (int)rbq;
}
