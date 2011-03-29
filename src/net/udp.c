#include "net.h"
#include "udp.h"
#include "../kmem.h"
#include "../lib.h"
#include "../process.h"
#include "../interrupt.h"

// (todo): no more swap stuff for the user and proper socket ids

// Gets called by ip_input, when a UDP/IP packet has arrived
void udp_input(char *buf, int len)
{
    int i = 0;
    struct udp_sock *s;
    struct udp_buffer *b, *newb;
    ip_header *iph = (ip_header*)buf;
    udp_header *udph = (udp_header*)(buf+sizeof(ip_header));
    unsigned short port = udph->udp_dstport;
    unsigned int udplen = swaps(udph->udp_len);
    
    #if 0
    dprintf("UDP input on port %d\n", swaps(port));
    hexdump(buf, len);
    dprintf("udp_len: %d\n", udplen);
    dprintf("len: %d\n", len-sizeof(ip_header));
    #endif
    
    if ( udplen > len-sizeof(ip_header) )
        return;
    
    struct process *p = process_head;
    do
    {
        s = p->udpcb.next;
        
        while ( s != NULL )
        {
            if ( s->lport == port )
            {
                // Traverse to the end of the buffer list
                b = &(s->buffer);
                while ( b->next != NULL )
                {
                    b = b->next;
                    i++;
                }
                
                // This socket's buffer is full, forget about the packet
                if ( i >= UDP_MAXBUFFER )
                    return;
                
                // Add a new buffer
                newb = kmem_alloc(sizeof(struct udp_buffer));
                memcpy(newb->buf, buf, len);
                newb->len = len;
                newb->next = NULL;
                b->next = newb;
                
                // Wake up the process
                p->sleep = FALSE;
                
                return;
            }
            s = s->next;
        }
        p = p->next;
    } while(p != process_head);
}

int udp_output(int sock, struct sockaddr *dst, char *buf, 
    unsigned int len)
{
    udp_header *header;
    char *packet;
    struct udp_sock *s;
    struct process *p = process_current;
    
    // Find the socket specified
    s = p->udpcb.next;
    while ( s != NULL )
    {
        if ( sock == s->id )
            break;
        s = s->next;
    }
    
    // Such a socket does not exist
    if ( s == NULL )
        return -1;
    
    // Allocate memory for all the lower layer headers to fit in
    packet = kmem_alloc(
        len + sizeof(udp_header) + sizeof(ip_header) + sizeof(linklayer_header));
    header = (udp_header*)(packet + sizeof(ip_header) 
        + sizeof(linklayer_header));
    
    // Copy over the buffer into the packet, starting behind the udp header
    memcpy(packet + 
        sizeof(udp_header) + sizeof(ip_header) + sizeof(linklayer_header), 
        buf, len
    );
    
    // Set udp header information
    header->udp_dstport = swaps(dst->port);
    header->udp_srcport = swaps(s->lport);
    len += sizeof(udp_header);
    header->udp_len = (0xffff & (len<<8)) | (len>>8);
    header->udp_cksum = 0;
    
    // Pass our packet to IP
    ip_output(swapl(dst->addr), 17, packet, len);
    
    kmem_free(packet, 
        len+sizeof(udp_header)+sizeof(ip_header)+sizeof(linklayer_header)
    );
    
    return len - sizeof(udp_header);
}

int udp_recvfrom(int sock, struct sockaddr *src, char *buf, unsigned int len)
{
    struct process *p;
    struct udp_sock *s;
    struct udp_buffer *b;
    ip_header *iph;
    udp_header *udph;
    unsigned int rlen, mlen;
    
    interrupt_mask();
    
    p = process_current;
    
    // Find the socket specified
    s = p->udpcb.next;
    while ( s != NULL )
    {
        if ( sock == s->id )
            break;
        s = s->next;
    }
    
    // Such a socket does not exist
    if ( s == NULL )
        return -1;
    
    // If the socket's first buffer element is non-null, there is new data
    while ( (b = s->buffer.next) == NULL )
        interrupt_sleepinsyscall();
    
    // The IP and UDP headers are in the buffer
    iph = (ip_header*)b->buf;
    udph = (udp_header*)(b->buf + sizeof(ip_header));
    
    // mlen will be the length of the udp message (without headers)
    mlen = swaps(udph->udp_len) - sizeof(udp_header);
    
    // Copy 'len' bytes at max
    rlen =  (mlen > len ? len : mlen);
    
    memcpy(buf, b->buf + sizeof(ip_header) + sizeof(udp_header), rlen);
    
    // Put source information into 'src'
    src->addr = iph->ip_src;
    src->port = udph->udp_srcport;
    
    // Delete and free the buffer
    s->buffer.next = b->next;
    kmem_free(b, sizeof(struct udp_buffer));
    
    return rlen; // We're finished; return to caller
}

int udp_socket()
{
    int id = 0, lastid = 0;
    struct udp_sock *s = &(process_current->udpcb);
    struct udp_sock *newsock;
    
    interrupt_mask();
    
    newsock = kmem_alloc(sizeof(struct udp_sock));
    
    // Choose a free socket ID
    while ( s->next != NULL )
    {
        id = s->next->id;
        if ( id > lastid+1 ) // gap detected
            break;
        lastid = id;
        s = s->next;
    }
    
    newsock->id = lastid + 1;
    
    // Insert it into the list
    newsock->next = s->next;
    s->next = newsock;
    newsock->last = s;
    newsock->next->last = newsock;
    
    newsock->faddr = 0;
    newsock->fport = 0;
    newsock->laddr = 0;
    newsock->lport = 0;
    newsock->buffer.next = NULL;
    
    return newsock->id;
}

// Binds a local port to a socket
int udp_bind(int sock, struct sockaddr *addr)
{
    int i = 0;
    struct udp_sock *s;
    
    interrupt_mask();
    
    // See if this port is already taken by another socket
    struct process *p = process_head;
    do
    {
        s = p->udpcb.next;
        
        while ( s != NULL )
        {
            if ( s->lport == addr->port )
                return -1;
            s = s->next;
        }
        p = p->next;
    } while(p != process_head);
    
    // Traverse to our socket and set the port
    s = process_current->udpcb.next;
    while ( s != NULL )
    {
        if ( sock == s->id )
            break;
        s = s->next;
    }
    
    if ( s == NULL )
        return -1;
    
    s->lport = addr->port;
    return 0; // Success return value
}

// Frees the socket and all of its buffers
int udp_close(int socket)
{
    int i = 0;
    struct udp_sock *s; 
    struct udp_buffer *b, *lastb;
    
    interrupt_mask();
    
    // Find the socket
    s = process_current->udpcb.next;
    while ( s != NULL )
    {
        if ( socket == s->id )
            break;
        s = s->next;
    }
    
    // Such a socket does not exist
    if ( s == NULL )
        return -1;
    
    // Free all buffers
    b = s->buffer.next;
    while( b != NULL )
    {
        lastb = b;
        b = b->next;
        kmem_free(lastb, sizeof(struct udp_buffer));
    }
    
    // Remove the socket from the list and free it
    s->last->next = s->next;
    if ( s->next != NULL )
        s->next->last = s->last;
    kmem_free(s, sizeof(struct udp_sock));
    
    return 0;
}
