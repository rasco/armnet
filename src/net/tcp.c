#include "net.h"
#include "ip.h"
#include "tcp.h"
#include "../lib.h"
#include "../kmem.h"
#include "../process.h"
#include "../board.h"
#include "../interrupt.h"

// Macros for comparing sequence numbers
#define TCP_SEQ_LT(a,b)             ((int)((a)-(b)) < 0)
#define TCP_SEQ_LEQ(a,b)            ((int)((a)-(b)) <= 0)
#define TCP_SEQ_GT(a,b)             ((int)((a)-(b)) > 0)
#define TCP_SEQ_GEQ(a,b)            ((int)((a)-(b)) >= 0)

// Fills in all TCP header fields except for the checksum, which is set to zero
#define tcp_fillheader(header, lport, fport, seq, ack, offset, flags, win, \
                       urg) \
    header->srcport = swaps(lport); \
    header->dstport = swaps(fport); \
    header->seqnr = swapl(seq); \
    header->acknr = swapl(ack); \
    header->offsetcontrol = (offset<<12); \
    header->offsetcontrol |= flags; \
    header->offsetcontrol = swaps(header->offsetcontrol); \
    header->window = swaps(win/64); \
    header->checksum = 0; \
    header->urgentp = 0;

void tcp_sendsyn(struct tcp_sock *s);

unsigned short tcp_checksum(int srcaddr, int dstaddr, char *buf, int len)
{
    int i;
    unsigned short word;
    unsigned int sum = 0;
    char *srcaddrc = (char*)&srcaddr;
    char *dstaddrc = (char*)&dstaddr;
    
    // Pseudo IP header
    for ( i = 0; i < 4; i = i + 2)
    {
        word = ( (srcaddrc[i] << 8) & 0xFF00) + ( srcaddrc[i+1] & 0xFF );
        sum = sum + word;	
    }
    for ( i = 0; i < 4; i = i + 2)
    {
        word = ( (dstaddrc[i] <<8 ) & 0xFF00)+( dstaddrc[i+1] & 0xFF );
        sum = sum + word;
    }
    sum = sum + 6 + len;
    
    // TCP header + data
    for ( i = 0; i < len - 2; i = i+2 )
    {
        word = ((buf[i]<<8) & 0xFF00) + (buf[i+1] & 0xFF);
        sum = sum + (unsigned long)word;
    }
    if ( len & 0x1 )
        word = ((buf[i]<<8)&0xFF00);
    else
        word = ((buf[i]<<8)&0xFF00) + (buf[i+1]&0xFF);
    sum += (unsigned long)word;
    
    while (sum >> 16)
        sum = (sum & 0xFFFF)+(sum >> 16);
    sum = ~sum;
    
    return ((unsigned short) sum);
}

// Delete the socket and all of its buffers
void tcp_deletesocket(struct tcp_sock *s)
{
    struct tcp_buffer *b, *lastb;
    
    // Delete retransmit buffers
    b = s->snd_buf.next;
    while( b != NULL )
    {
        lastb = b;
        b = b->next;
        kmem_free(lastb->data, lastb->len);
        kmem_free(lastb, sizeof(struct tcp_buffer));
    }
    
    // Delete receive buffers
    b = s->rcv_buf.next;
    while( b != NULL )
    {
        lastb = b;
        b = b->next;
        kmem_free(lastb->data, lastb->len);
        kmem_free(lastb, sizeof(struct tcp_buffer));
    }
    
    // Delete out of order receive buffers
    b = s->rcv_outoforder_buf.next;
    while( b != NULL )
    {
        lastb = b;
        b = b->next;
        kmem_free(lastb->data, lastb->len);
        kmem_free(lastb, sizeof(struct tcp_buffer));
    }
    
    // Remove it from the list...
    s->last->next = s->next;
    if ( s->next != NULL )
        s->next->last = s->last;
    
    dprintf("socket %d deleted\n", s->id);
    kmem_free(s, sizeof(struct tcp_sock));
}

void tcp_timer()
{
    tcp_iss++;
    if ( tcp_timeout-- > 0 )
        return;
    
    tcp_timeout = TCP_TIMEOUT;
    
    struct tcp_sock *s, *next;
    struct tcp_buffer *b;
    struct tcp_header tcph;
    struct process *p = process_head;
    do
    {
        s = p->tcpcb.next;
        while(s != NULL )
        {
            next = s->next;
            if ( s->state == SYN_SENT )
            {
                if ( s->synsent_timeout-- <= 0 )
                {
                    if ( s->synsent_retries <= 0 )
                    {
                        s->state = CLOSED;
                        p->sleep = FALSE;
                    }
                    else
                        tcp_sendsyn(s);
                }
            }
            else if ( s->state == LAST_ACK )
            {
                if ( s->lastack_timeout-- <= 0 )
                {
                    dprintf("lastack timeout. socket deleted.\n");
                    tcp_deletesocket(s);
                    s = next;
                    continue;
                }
            }
            else if ( s->state == TIME_WAIT && s->timewait_timeout-- <= 0 )
            {
                dprintf("TIME_WAIT timeout. socket deleted.\n");
                tcp_deletesocket(s);
                s = next;
                continue;
            }
            else if ( s->state == SYN_RCVD && s->synrcvd_timeout-- <= 0 )
            {
                tcp_header replyheader;
                // We received no ACK, drop the connection, go back to listen
                // state and send a RST
                s->state = LISTEN;
                s->faddr = 0;
                s->fport = 0;
                
                // Send RST
                tcp_fillheader((&replyheader), s->lport, s->fport, 
                    s->snd_iss, s->rcv_nxt, 5, 
                    TCP_HDR_CTRL_RST, s->rcv_wnd, 0);
                tcp_output(s, &replyheader, NULL, NULL, 0);
            }
            if ( s->snd_buf.next != NULL ) 
            {
                if ( s->retransmission_timeout++ >= TCP_RETRANSMISSION_TIMEOUT )
                {
                    // Prepare segment
                    b = s->snd_buf.next;
                    while ( b != NULL )
                    {
                        if ( s->snd_wnd < b->len )
                        {
                            // Send a window probe
                            tcp_fillheader((&tcph), s->lport, s->fport, 
                                b->seqnr, s->rcv_nxt, 5, 
                                TCP_HDR_CTRL_ACK | TCP_HDR_CTRL_PSH, 
                                s->rcv_wnd, 0);
                            tcp_output(s, &tcph, NULL, b->data, b->len);
                            break;
                        }
                        s->snd_wnd -= b->len;
                        tcp_fillheader((&tcph), s->lport, s->fport, b->seqnr, 
                            s->rcv_nxt, 5, TCP_HDR_CTRL_ACK | TCP_HDR_CTRL_PSH, 
                            s->rcv_wnd, 0);
                        tcp_output(s, &tcph, NULL, b->data, b->len);
                        b = b->next;
                    }
                    s->retransmission_timeout = 0;
                }
            }
            s = next;
        }
        p = p->next;
    } while( p != process_head );
}

// Pass TCP data out to IP
// len is the length of the tcp data
void tcp_output(struct tcp_sock *s, tcp_header *header, char *options, 
    char *data, int len)
{
    int optlen = ((swaps(header->offsetcontrol)>>12) - 5) * 4;
    char packet[sizeof(linklayer_header) + sizeof(ip_header) + 
        sizeof(tcp_header) + optlen + len];
    
    memcpy(packet + sizeof(linklayer_header) + sizeof(ip_header), header, 
        sizeof(tcp_header));
    if ( optlen > 0 )
        memcpy(packet + sizeof(linklayer_header) + sizeof(ip_header) + 
            sizeof(tcp_header), options, optlen);
    if ( len > 0 )
        memcpy(packet + sizeof(linklayer_header) + sizeof(ip_header) + 
            sizeof(tcp_header) + optlen, data, len);
    
    header = (tcp_header *)(packet + sizeof(linklayer_header) 
        + sizeof(ip_header));
    header->checksum = 0;
    header->checksum = tcp_checksum(swapl(s->laddr), swapl(s->faddr), 
        packet + sizeof(linklayer_header) + sizeof(ip_header), 
        sizeof(tcp_header) + optlen + len );
    header->checksum = swaps(header->checksum);
    
    // Pass our packet to IP
    ip_output(swapl(s->faddr), 0x06, packet, sizeof(tcp_header) + optlen + len);
    
    //dprintf("tcp output: %d data bytes sent (seq: %d)\n", len, 
    //    swapl(header->seqnr));
}

void tcp_input_listen(struct process *p, struct tcp_sock *s, 
    struct ip_header *iph, struct tcp_header *tcph, int controlflags)
{
    tcp_header replyheader;
    
    // Ignore RST
    if ( controlflags & TCP_HDR_CTRL_RST )
        return;
    
    // Any ACK in listen state is bad
    if ( controlflags & TCP_HDR_CTRL_ACK )
    {
        // Send reset and return
        tcp_fillheader((&replyheader), s->lport, s->fport, 
            tcph->acknr, 0, 5, TCP_HDR_CTRL_RST, s->rcv_wnd, 0);
        tcp_output(s, &replyheader, NULL, NULL, 0);
        return;
    }
    
    if ( controlflags & TCP_HDR_CTRL_SYN )
    {
        // Check security/compartment/precedence (todo)
        
        s->rcv_nxt = swapl(tcph->seqnr) + 1;
        s->rcv_irs = swapl(tcph->seqnr);
        s->snd_iss = tcp_iss;
        // Set foreign address and port
        s->faddr = swapl(iph->ip_src);
        s->fport = swaps(tcph->srcport);
        
        // Send SYN, ACK
        tcp_fillheader((&replyheader), s->lport, s->fport, 
            s->snd_iss, s->rcv_nxt, 5, 
            TCP_HDR_CTRL_SYN | TCP_HDR_CTRL_ACK, s->rcv_wnd, 0);
        tcp_output(s, &replyheader, NULL, NULL, 0);
        
        s->snd_nxt = s->snd_iss + 1;
        s->snd_una = s->snd_iss;
        
        // Switch state
        s->state = SYN_RCVD;
        
        s->synrcvd_timeout = 5;
    }
}

void tcp_input_synsent(struct process *p, struct tcp_sock *s, 
    struct ip_header *iph, struct tcp_header *tcph, int controlflags)
{
    tcp_header replyheader;
    BOOL ackacceptable = FALSE;
    
    // Check for the ACK flag
    if ( controlflags & TCP_HDR_CTRL_ACK )
    {
        if ( TCP_SEQ_LEQ(swapl(tcph->acknr), s->snd_iss)
            || TCP_SEQ_GT(swapl(tcph->acknr), s->snd_nxt) )
        {
            // Bad ACK: drop the packet (return)
            if ( !(controlflags & TCP_HDR_CTRL_RST) )
            {
                // Send a reset
                tcp_fillheader((&replyheader), s->lport, s->fport, 
                    swapl(tcph->acknr), 0, 5, TCP_HDR_CTRL_RST, 
                    s->rcv_wnd, 0);
                tcp_output(s, &replyheader, NULL, NULL, 0);
            }
            return;
        }
        if ( TCP_SEQ_LEQ(s->snd_una, swapl(tcph->acknr)) 
            && TCP_SEQ_LEQ(swapl(tcph->acknr), s->snd_nxt) )
        {
            ackacceptable = TRUE;
        }
    }
    
    // Check for the RST flag
    if ( controlflags & TCP_HDR_CTRL_RST )
    {
        if ( ackacceptable )
        {
            // Connection refused by the other side
            s->state = CLOSED;
            
            // Wake up the caller
            p->sleep = FALSE;
            s->err = TCP_ERR_CONNREFUSED;
        }
        return;
    }
    
    // Check security and precedence (todo)
    
    // Check for the SYN flag
    if ( controlflags & TCP_HDR_CTRL_SYN )
    {
        s->rcv_nxt = swapl(tcph->seqnr)+1;
        s->rcv_irs = swapl(tcph->seqnr);
        
        if ( ackacceptable )
            s->snd_una = swapl(tcph->acknr);
        
        if ( TCP_SEQ_GT(s->snd_una, s->snd_iss) )
        {
            // The connection is hereby established
            // Send ACK
            tcp_fillheader((&replyheader), s->lport, s->fport, s->snd_nxt, 
                s->rcv_nxt, 5, TCP_HDR_CTRL_ACK, s->rcv_wnd, 0);
            tcp_output(s, &replyheader, NULL, NULL, 0);
            
            s->state = ESTABLISHED;
            s->nextbyte = s->rcv_nxt;
            s->snd_wnd = swaps(tcph->window)*64;
            s->snd_wl1 = swapl(tcph->seqnr);
            s->snd_wl2 = swapl(tcph->acknr);
            
            // Process options (todo)
            
            // Wake up calling process
            p->sleep = FALSE;
        }
        else
        {
            // We only received a SYN, so go into SYN_RCVD state
            tcp_fillheader((&replyheader), s->lport, s->fport, s->snd_iss, 
                s->rcv_nxt, 5, TCP_HDR_CTRL_ACK | TCP_HDR_CTRL_SYN, s->rcv_wnd,
                0);
            tcp_output(s, &replyheader, NULL, NULL, 0);
            
            s->state = SYN_RCVD;
            
            // Queue controls for processing when established (todo)
        }
    }
}

void tcp_input_default(struct process *p, struct tcp_sock *s, 
    struct ip_header *iph, struct tcp_header *tcph, int controlflags,
    int datalength, char *data)
{
    tcp_header replyheader;
    struct tcp_buffer *b, *newb, *lastb, *ooob;
    
    if ( !( 
        ( 
        TCP_SEQ_LEQ(s->rcv_nxt, swapl(tcph->seqnr)) 
        && TCP_SEQ_LT(swapl(tcph->seqnr), s->rcv_nxt + s->rcv_wnd) 
        )
        || 
        ( 
        TCP_SEQ_LEQ(s->rcv_nxt, swapl(tcph->seqnr) + datalength - 1)
        && TCP_SEQ_LT(swapl(tcph->seqnr) + datalength - 1,
        s->rcv_nxt + s->rcv_wnd) 
        )
        ))
    {
        if ( controlflags & TCP_HDR_CTRL_RST )
            return;
        
        // Segment not acceptable (does not lie in receive window)
        // Send ACK and drop the segment (return)
        tcp_fillheader((&replyheader), s->lport, s->fport, s->snd_nxt, 
            s->rcv_nxt, 5, TCP_HDR_CTRL_ACK, s->rcv_wnd, 0);
        tcp_output(s, &replyheader, NULL, NULL, 0);
        
        return;
    }
    
    if ( controlflags & TCP_HDR_CTRL_RST )
    {
        if ( s->state == SYN_RCVD )
        {
            // delete retransmission queue (todo) 
            // do we even need to delete it??
            // the caller should be blocked (in connect or listen) so
            // he couldnt have sent anything yet...
            
            if ( s->listener == TRUE )
            {
                // Go back to listen and unconnect
                s->state = LISTEN;
                s->faddr = 0;
                s->fport = 0;
                return;
            }
            // Connection refused
            s->err = TCP_ERR_CONNREFUSED;
            p->sleep = FALSE;
            s->state = CLOSED;
            return;
        }
        else if ( s->state == ESTABLISHED || s->state == FIN_WAIT_1
            || s->state == FIN_WAIT_2 || s->state == CLOSE_WAIT )
        {
            // If the RST bit is set then, any outstanding RECEIVEs and 
            // SEND should receive "reset" responses. (todo)
            // All segment queues should be flushed. (todo)
            // Notify user
            s->err = TCP_ERR_CONNRESET;
            s->state = CLOSED;
            p->sleep = FALSE;
            return;
        }
        else if ( s->state == CLOSING || s->state == LAST_ACK
            || s->state == TIME_WAIT )
        {
            s->state = CLOSED;
            return;
        }
    }
    
    // Check security and precedence (todo)
    
    if ( controlflags & TCP_HDR_CTRL_SYN )
    {
        // send reset
        dprintf("bad syn received. sending reset.\n");
        tcp_fillheader((&replyheader), s->lport, s->fport, 
            0, swapl(tcph->seqnr), 5, 
            TCP_HDR_CTRL_RST | TCP_HDR_CTRL_ACK, s->rcv_wnd, 0);
        tcp_output(s, &replyheader, NULL, NULL, 0);
        s->state = CLOSED;
        s->err = TCP_ERR_CONNRESET;
        // Flush segment queues (todo)
        p->sleep = FALSE;
        
        // If the user has already closed the socket, we can safely
        // delete it
        if ( s->closedbyuser )
            tcp_deletesocket(s);
        return;
    }
    
    if ( !(controlflags & TCP_HDR_CTRL_ACK) )
        return;
    
    if ( s->state == SYN_RCVD )
    {
        dprintf("received an ack in syn_rcvd state\n");
        if ( TCP_SEQ_LEQ(s->snd_una, swapl(tcph->acknr)) 
            && TCP_SEQ_LEQ(swapl(tcph->acknr), s->snd_nxt) )
        {
            dprintf("established\n");
            s->state = ESTABLISHED;
            p->sleep = FALSE;
            s->nextbyte = s->rcv_nxt;
        }
        else
        {
            dprintf("bad ack\n");
            // Send reset
            tcp_fillheader((&replyheader), s->lport, s->fport, 
                tcph->acknr, 0, 5, TCP_HDR_CTRL_RST, s->rcv_wnd, 
                0);
            tcp_output(s, &replyheader, NULL, NULL, 0);
        }
    }
    else if ( s->state == ESTABLISHED || s->state == FIN_WAIT_1 
        || s->state == FIN_WAIT_2 || s->state == CLOSE_WAIT )
    {
        if ( TCP_SEQ_LT(s->snd_una, swapl(tcph->acknr)) 
            && TCP_SEQ_LEQ(swapl(tcph->acknr), s->snd_nxt) )
        {
            s->snd_una = swapl(tcph->acknr);
            
            // Update send window 
            if ( TCP_SEQ_LT(s->snd_wl1, swapl(tcph->seqnr)) 
                || ( s->snd_wl1 == swapl(tcph->seqnr) 
                    && TCP_SEQ_LEQ(s->snd_wl2, swapl(tcph->acknr)) )
               )
            {
                s->snd_wnd = swaps(tcph->window)*64;
                dprintf("snd_wnd: %d\n", s->snd_wnd*64);
                s->snd_wl1 = swapl(tcph->seqnr);
                s->snd_wl2 = swapl(tcph->acknr);
            }
            
            // Delete retransmission buffers up to the ack
            b = s->snd_buf.next;
            while( b != NULL )
            {
                if ( TCP_SEQ_GT(b->seqnr + b->len, s->snd_una) )
                    break;
                lastb = b;
                b = b->next;
                s->sndbuf_size -= lastb->len;
                kmem_free(lastb->data, lastb->len);
                kmem_free(lastb, sizeof(struct tcp_buffer));
                s->retransmission_timeout = 0;
                dprintf("deleted a retransmission buffer\n");
            }
            s->snd_buf.next = b;
            
            if ( s->state == FIN_WAIT_1 )
                if ( swapl(tcph->acknr) == s->snd_nxt )
                    s->state = FIN_WAIT_2;
            if ( s->state == CLOSING )
            {
                if ( swapl(tcph->acknr) == s->snd_nxt )
                    s->state = TIME_WAIT;
                else
                    return;
            }
        }
        else if ( TCP_SEQ_GT(swapl(tcph->acknr), s->snd_nxt) )
        {
            // Send ack, drop and return
            tcp_fillheader((&replyheader), s->lport, s->fport, 
                s->snd_nxt, tcph->acknr, 5, TCP_HDR_CTRL_ACK,
                s->rcv_wnd, 0);
            tcp_output(s, &replyheader, NULL, NULL, 0);
            return;
        }
    }
    else if ( s->state == LAST_ACK )
    {
        if ( swapl(tcph->acknr) == s->snd_nxt )
        {
            dprintf("last ack received. socket closed.\n");
            s->state = CLOSED;
            tcp_deletesocket(s);
        }
        return;
    }
    else if ( s->state == TIME_WAIT )
    {
        // "The only thing that can arrive in this state is a
        // retransmission of the remote FIN.  Acknowledge it, and 
        // restart them 2 MSL timeout."
        tcp_fillheader((&replyheader), s->lport, s->fport, 
            s->snd_nxt, s->rcv_nxt, 5, TCP_HDR_CTRL_ACK, s->rcv_wnd, 
            0);
        tcp_output(s, &replyheader, NULL, NULL, 0);
    }
    
    // Check URG bit (todo)
    
    // Process segment text
    if ( s->state == ESTABLISHED || s->state == FIN_WAIT_1 
        || s->state == FIN_WAIT_2)
    {
        // Is this segment in order and did we receive data?
        // Only accept and buffer the segment if the recvbuffer has room
        if ( s->rcv_nxt == swapl(tcph->seqnr) && datalength > 0
            && s->rcv_wnd >= (unsigned int)datalength )
        {
            // Advance rcv_nxt..
            s->rcv_nxt += datalength;
            // Adjust receive window 
            s->rcv_wnd -= datalength;
            
            // Copy data into the in-order receive buffer queue
            b = &(s->rcv_buf);
            while ( b->next != NULL )
                b = b->next;
            newb = kmem_alloc(sizeof(struct tcp_buffer));
            newb->data = kmem_alloc(datalength);
            //memcpy(newb->data, data, datalength);
            newb->len = datalength;
            newb->seqnr = swapl(tcph->seqnr);
            newb->next = NULL;
            b->next = newb;
            
            
            #if 0
            // Put out of order buffers at the end of the in-order queue
            // if they are now in order. Inefficient
            ooob = &(s->rcv_outoforder_buf);
            while ( ooob->next != NULL )
            {
                //printf("putting ooob buf into in-order queue\n");
                
                if ( ooob->next->seqnr != s->rcv_nxt )
                {
                    ooob = ooob->next;
                    continue;
                }
                
                dprintf("ordering\n");
                // Remove from out of order queue
                lastb = ooob;
                ooob = ooob->next;
                lastb->next = ooob->next;
                
                // Insert into in-order queue
                b = &(s->rcv_buf);
                while ( b->next != NULL ) // Traverse to the end
                    b = b->next;
                b->next = ooob; // Insert it.. b->next is NULL anyway.
                ooob->next = NULL;
                
                // Advance rcv_nxt
                s->rcv_nxt += ooob->len;
                
                // Continue looking for more out of order buffers
                ooob = lastb;
            }
            #endif
            
            // ...and send ACK
            tcp_fillheader((&replyheader), s->lport, s->fport, 
                s->snd_nxt, s->rcv_nxt, 5, TCP_HDR_CTRL_ACK, s->rcv_wnd, 
                0);
            tcp_output(s, &replyheader, NULL, NULL, 0);
            
            s->bytesbuffered += datalength;
            
            p->sleep = FALSE;
        }
        else if ( s->rcv_nxt != swapl(tcph->seqnr) )
        {
        
            dprintf("seg out of order\n");
            
            // Send duplicate ACK
            tcp_fillheader((&replyheader), s->lport, s->fport, 
                s->snd_nxt, s->rcv_nxt, 5, TCP_HDR_CTRL_ACK, s->rcv_wnd, 
                0);
            tcp_output(s, &replyheader, NULL, NULL, 0);
            
            #if 0
            if ( s->rcv_wnd > (unsigned int)datalength * 2 )
            {
                s->rcv_wnd -= datalength;
                s->bytesbuffered += datalength;
                // Copy data into the out of order receive buffer queue
                b = &(s->rcv_outoforder_buf);
                while ( b->next != NULL )
                    b = b->next;
                newb = kmem_alloc(sizeof(struct tcp_buffer));
                newb->data = kmem_alloc(datalength);
                memcpy(newb->data, data, datalength);
                newb->len = datalength;
                newb->seqnr = swapl(tcph->seqnr);
                newb->next = NULL;
                b->next = newb;
            }
            #endif
        }
        if ( s->rcv_wnd < (unsigned int) datalength )
            dprintf("window zero\n");
    }
    
    // Check FIN flag
    if ( controlflags & TCP_HDR_CTRL_FIN )
    {
        if ( s->state == LISTEN || s->state == SYN_SENT )
            return;
        dprintf("fin flag set\n");
        s->rcv_nxt++;
        if ( s->state == ESTABLISHED || s->state == SYN_RCVD )
        {
            s->state = CLOSE_WAIT;
            p->sleep = FALSE;
        }
        else if ( s->state == FIN_WAIT_1 )
        {
            if ( s->snd_una + 1 == s->snd_nxt )
            {
                dprintf("fin_wait_1: fin was acked. enter time_wait\n");
                s->state = TIME_WAIT;
                s->timewait_timeout = 10;
            }
            else
                s->state = CLOSING;
        }
        else if ( s->state == FIN_WAIT_2 )
        {
            s->state = TIME_WAIT;
            s->timewait_timeout = 10;
        }
        else if ( s->state == TIME_WAIT )
            s->timewait_timeout = 10;
        
        // ACK the FIN
        tcp_fillheader((&replyheader), s->lport, s->fport, 
            s->snd_nxt, s->rcv_nxt, 5, TCP_HDR_CTRL_ACK, s->rcv_wnd, 
            0);
        tcp_output(s, &replyheader, NULL, NULL, 0);
    }
}

void tcp_input(char *buf, int len)
{
    struct tcp_sock *s = NULL, *ls = NULL;
    ip_header *iph = (ip_header*)buf;
    tcp_header *tcph = (tcp_header*)(buf+sizeof(ip_header));
    char *data;
    int dataoffset, datalength;
    int controlflags;
    unsigned short checksum;
    tcp_header replyheader;
    
    if ( (unsigned)len < (sizeof(tcp_header) + sizeof(ip_header)))
        return;
    
    dataoffset = ( swaps(tcph->offsetcontrol) >> 12 ) *  4;
    datalength = len - sizeof(ip_header) - dataoffset;
    data = buf + sizeof(ip_header) + dataoffset;
    controlflags = swaps(tcph->offsetcontrol) & 0x3F;
    
    // Sanity checks...
    if ( datalength < 0 )
        return;
    
    // Verify checksum
    #if 1
    checksum = tcph->checksum;
    tcph->checksum = 0;
    if ( tcp_checksum(iph->ip_src, iph->ip_dst, (char*)tcph, 
        len-sizeof(ip_header)) != swaps(checksum) )
        return;
    #endif
    
    // Look for the socket with local port = packet's destination port
    // and foreign port = packet's source port
    // and foreign address = packet's source address.
    // Allow matching with listening sockets (sockets that have zero as their
    // foreign port and address.
    struct process *p = process_head;
    do
    {
        s = p->tcpcb.next;
        while(s != NULL )
        {
            dprintf("s: %x\n", s->next);
            // Look for a socket with that local port
            if ( s->state != CLOSED && (s->lport == swaps(tcph->dstport)) )
            {
                // Are that socket's foreign port and IP also matched?
                if ( swaps(tcph->srcport) == s->fport 
                    && s->faddr == swapl(iph->ip_src) )
                    break;
                // If not, is this a listening socket?
                else if ( s->fport == 0 && s->faddr == 0 && s->listener )
                    ls = s;
            }
            s = s->next;
        }
        // If s is not null we found a matching socket
        if ( s != NULL )
            break;
        p = p->next;
    } while( p != process_head );
    
    // No matching sockets found
    if ( s == NULL && ls == NULL )
        // Send RST (todo)
        return;
    // We found no connected socket, so pass the segment to the listener
    if ( s == NULL ) 
        s = ls;
    
    dprintf("tcp_input on socket %x\n", s);//, (s->state));
    dprintf("seg seq: %d, nxt: %d, datalength: %d\n", swapl(tcph->seqnr), 
        s->rcv_nxt, datalength);
    dprintf("ack: %d, nxt: %d\n", swapl(tcph->acknr), s->snd_nxt);
    
    switch (s->state)
    {
        case LISTEN:
            tcp_input_listen(p, s, iph, tcph, controlflags);
        break;
        case SYN_SENT:
            tcp_input_synsent(p, s, iph, tcph, controlflags);
        break;
        default:
            tcp_input_default(p, s, iph, tcph, controlflags, datalength, data);
    }
}

void tcp_sendsyn(struct tcp_sock *s)
{
    struct tcp_header tcph;
    char options[8];
    
    // Prepare segment
    tcp_fillheader((&tcph), s->lport, s->fport, s->snd_iss, 0, 7, 
        TCP_HDR_CTRL_SYN, s->rcv_wnd, 0);
    
    options[0] = 0x02;
    options[1] = 0x04;
    options[2] = 0x05;
    options[3] = 0xb4;
    
    options[4] = 0x01;
    options[5] = 0x03;
    options[6] = 0x03;
    options[7] = 0x06;
    
    tcp_output(s, &tcph, options, NULL, 0);
    
    s->synsent_timeout = TCP_SYNSENT_TIMEOUT;
    s->synsent_retries--;
}

int tcp_connect(int socket, struct sockaddr *addr)
{
    struct tcp_sock *s;
    
    interrupt_mask();
    
    s = process_current->tcpcb.next;
    while ( s != NULL )
    {
        if ( s->id == socket )
            break;
        s = s->next;
    }
    
    if ( s == NULL )
        return -1;
    
    // Set socket connection state
    s->state = SYN_SENT;
    s->snd_iss = tcp_iss;
    s->snd_nxt = s->snd_iss+1;
    s->snd_una = s->snd_iss;
    s->rcv_wnd = TCP_RECVBUF_MAXSIZE*64;
    s->bytesbuffered = 0;
    s->faddr = addr->addr;
    s->fport = addr->port;
    s->synsent_retries = TCP_SYNSENT_RETRIES;
    s->mss = 536;
    s->listener = FALSE;
    s->err = 0;
    s->sndbuf_size = 0;
    s->retransmission_timeout = 0;
    
    tcp_sendsyn(s);
    
    while ( s->state == SYN_SENT || s->state == SYN_RCVD )
        interrupt_sleepinsyscall();
    
    return s->err > 0 ? -1 : 0;
}

int tcp_send(int sock, char *buf, unsigned int len)
{
    struct process *p;
    struct tcp_sock *s;
    struct tcp_buffer *b, *newb;
    struct tcp_header tcph;
     
    interrupt_mask();
    
    p = process_current;
    
    if ( len == 0 )
        return -1;
    
    // Find the socket specified
    s = p->tcpcb.next;
    while ( s != NULL )
    {
        if ( sock == s->id )
            break;
        s = s->next;
    }
    
    // Such a socket does not exist
    if ( s == NULL )
        return -1;
    
    if ( s->state != ESTABLISHED )
        return -1;
    
    // Send at most s->mss bytes
    if ( len > s->mss )
        len = s->mss;
    
    if ( s->sndbuf_size >= TCP_SNDBUF_MAXSIZE )
        return 0;
    
    // Loop to the end of the retransmission queue
    b = &(s->snd_buf);
    while(b->next != NULL)
        b = b->next;
    
    newb = kmem_alloc(sizeof(struct tcp_buffer));
    newb->data = kmem_alloc(len);
    memcpy(newb->data, buf, len);
    newb->len = len;
    newb->seqnr = s->snd_nxt;
    newb->next = b->next;
    b->next = newb;
    
    s->sndbuf_size += len;
    
    // Is the send window large enough for len bytes?
    if ( s->snd_wnd > len )
    {
        // Prepare segment
        tcp_fillheader((&tcph), s->lport, s->fport, s->snd_nxt, s->rcv_nxt, 5, 
            TCP_HDR_CTRL_ACK | TCP_HDR_CTRL_PSH, s->rcv_wnd, 0);
        
        tcp_output(s, &tcph, NULL, newb->data, len);
        
        s->snd_wnd -= len;
    }
    
    s->snd_nxt += len;
    
    return len;
}

int tcp_recv(int sock, char *buf, unsigned int len)
{
    tcp_header ackheader;
    unsigned int rlen, mlen, roffset, bytescopied = 0;
    struct process *p;
    struct tcp_sock *s;
    struct tcp_buffer *b;
    
    interrupt_mask();
    
    if ( len == 0 )
        return -1;
    
    p = process_current;
    
    // Find the socket specified
    s = p->tcpcb.next;
    while ( s != NULL )
    {
        if ( sock == s->id )
            break;
        s = s->next;
    }
    // Such a socket does not exist
    if ( s == NULL )
        return -1;
    
    restart:
    
    if ( s->state == CLOSED )
        return -1;
    
    if ( s->state == CLOSING || s->state == LAST_ACK || s->state == TIME_WAIT )
        return -1;
    
    if ( s->state == CLOSE_WAIT )
        if ( s->rcv_buf.next == NULL )
            return 0;
    
    // If the socket's first buffer element is non-null, there is new data
    b = s->rcv_buf.next;
    if ( b == NULL )
    {
        interrupt_sleepinsyscall();
        goto restart;
    }
    
    while ( len > 0 && b != NULL )
    {
        // Begin copying from s->nextbyte
        
        roffset = 
            TCP_SEQ_GT(s->nextbyte, b->seqnr) ? s->nextbyte - b->seqnr : 0;
        dprintf("copying from offset %d\n", roffset);
        // mlen is the number of bytes left to read from this buffer
        mlen = b->len - roffset;
        
        // Copy 'len' bytes at max (rlen = min(len, mlen))
        rlen = (mlen > len ? len : mlen);
        len -= rlen;
        dprintf("%d bytes\n", rlen);
        dprintf("from a buffer of length %d\n", b->len);
        memcpy(buf + bytescopied, b->data + roffset, rlen);
        bytescopied += rlen;
        
        // Advance s->nextbyte by the number of bytes we've read
        s->nextbyte += rlen;
                
        if ( s->nextbyte >= b->seqnr + b->len )
        {
            //dprintf("deleting buffer\n");
            // Delete and free the buffer
            s->rcv_buf.next = b->next;
            s->bytesbuffered -= b->len;
            s->rcv_wnd += b->len;
            kmem_free(b->data, b->len);
            kmem_free(b, sizeof(struct tcp_buffer));
        }
        
        b = s->rcv_buf.next;
    }
    
    return bytescopied; // We're finished; return to caller
}

int tcp_bind(int socket, struct sockaddr *addr)
{
    struct tcp_sock *s;
    
    interrupt_mask();
    
    // See if this port is already taken by another socket
    struct process *p = process_head;
    do
    {
        s = p->tcpcb.next;
        while ( s != NULL )
        {
            if ( s->lport == addr->port )
                return -1;
            s = s->next;
        }
        p = p->next;
    } while(p != process_head);
    
    // Traverse to our socket and set the port
    s = &(process_current->tcpcb);
    while ( s->next != NULL )
    {
        s = s->next;
        if ( s->id == socket )
        {
            s->lport = addr->port;
            return 0;
        }
    }
    
    return -1; // Failure return value
}

int tcp_socket()
{
    int id = 0, lastid = 0;
    struct tcp_sock *s;
    struct tcp_sock *newsock;
    
    interrupt_mask();
    
    s = &process_current->tcpcb;
    newsock = kmem_alloc(sizeof(struct tcp_sock));
    
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
    newsock->closedbyuser = FALSE;
    newsock->faddr = 0;
    newsock->fport = 0;
    newsock->laddr = ip_opt_address;
    newsock->lport = 0;
    newsock->state = CLOSED;
    newsock->snd_buf.next = NULL;
    newsock->rcv_buf.next = NULL;
    newsock->rcv_outoforder_buf.next = NULL;
    newsock->err = 0;
    s->mss = 536;
    s->bytesbuffered = 0;
    s->snd_wnd = 536*2;
    s->sndbuf_size = 0;
    
    // Return the socket ID
    return newsock->id;
}

int tcp_listen(int socket)
{
    struct tcp_sock *s;
    struct process *p;
    
    interrupt_mask();
    
    p = process_current;
    
    // Find the socket specified
    s = process_current->tcpcb.next;
    while ( s != NULL )
    {
        if ( socket == s->id )
            break;
        s = s->next;
    }
    
    // Such a socket does not exist
    if ( s == NULL )
        return -1;
    
    // Allow listen only on closed sockets
    if ( s->state != CLOSED )
        return -1;
    
    if ( s->lport == 0 )
        return -1;
    
    s->state = LISTEN;
    s->err = 0;
    s->bytesbuffered = 0;
    s->sndbuf_size = 0;
    
    while ( s->state == SYN_SENT || s->state == SYN_RCVD || s->state == LISTEN )
        interrupt_sleepinsyscall();
    
    return s->err > 0 ? -1 : 0;
}

int tcp_close(int socket)
{
    struct tcp_sock *s;
    struct tcp_header tcph;
    
    interrupt_mask();
    
    // Find the socket specified
    s = process_current->tcpcb.next;
    while ( s != NULL )
    {
        if ( socket == s->id )
            break;
        s = s->next;
    }
    
    // Such a socket does not exist
    if ( s == NULL )
        return -1;
    
    if ( s->state == CLOSING || s->state == LAST_ACK || s->state == TIME_WAIT )
        return -1;
    
    if ( s->state == CLOSED )
    {
        tcp_deletesocket(s);
        return 0;
    }
    
    s->listener = TRUE;
    s->closedbyuser = TRUE;
    s->lastack_timeout = 5;
    
    tcp_fillheader((&tcph), s->lport, s->fport, s->snd_nxt++, s->rcv_nxt, 5,
            TCP_HDR_CTRL_FIN | TCP_HDR_CTRL_ACK, s->rcv_wnd, 0);
        tcp_output(s, &tcph, NULL, NULL, 0);
    
    if ( s->state == CLOSE_WAIT )
        s->state = LAST_ACK;
    else if ( s->state == ESTABLISHED )
        s->state = FIN_WAIT_1;
    
    return 0;
}

