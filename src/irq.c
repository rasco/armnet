#include "irq.h"
#include "board.h"
#include "lib.h"
#include "cpsr.h"
#include "process.h"
#include "net/ethernet.h"
#include "net/tcp.h"

void irq_systimer(int *sp);
void irq_serial_read(void);

/* Interrupt handler for the System Timer (ST)
 * */
void irq_systimer(int *sp)
{
    tcp_timer();
    process_switch(sp);
}

/* Interrupt handler for serial port input (DBGU)
 * */
void irq_serial_read(void)
{
    char res;
    res = *((char*)(ADDR_DBGU_BASE+0x18));
    printf("%c\n", res);
    
    if ( res == 't' )
    {
        printf("========================================"\
               "========================================\n");
        printf("TCP sockets:\n");
        struct process *p = process_head;
        struct tcp_sock *s;
        do
        {
            printf("\tprocess %d:\n", p->id);
            s = p->tcpcb.next;
            while(s != NULL )
            {
                printf("\t\tid: %d, state: %d, buffered: %d, wnd: %d\n"\
                    "\t\tsnd_wnd: %d, snd_buf: %d\n",
                    s->id, s->state, s->bytesbuffered, s->rcv_wnd, s->snd_wnd,
                    s->sndbuf_size);
                s = s->next;
            }
            p = p->next;
        } while( p != process_head );
        printf("========================================"\
               "========================================\n");
    }
    if ( res == 'u' )
    {
        printf("========================================"\
               "========================================\n");
        printf("UDP sockets:\n");
        struct process *p = process_head;
        struct udp_sock *s;
        do
        {
            printf("\tprocess %d:\n", p->id);
            s = p->udpcb.next;
            while(s != NULL )
            {
                printf("\t\tid: %d\n", s->id);
                s = s->next;
            }
            p = p->next;
        } while( p != process_head );
        printf("========================================"\
               "========================================\n");
    }
    if ( res == 'p' )
    {
        printf("========================================"\
               "========================================\n");
        printf("Processes:\n");
        struct process *p = process_head;
        do
        {
            printf("process %d: sleeping: %s, stack: %x, current sp: %x, "\
                "mode: %x\n", 
                p->id, p->sleep ? "yes" : "no", p->stack, p->registers[13], 
                p->cpsr&0x1f);
            p = p->next;
        } while( p != process_head );
        printf("========================================"\
               "========================================\n");
    }
}

/* See which device caused IRQ and call its handler
 * */
void irq_chooser(int *sp)
{
    int num = DEREF_INT(ADDR_AIC_BASE+ADDR_AIC_ISR); // IVR?
    
    if ( num == 1 )
    {
        if ( (DEREF_INT(ADDR_ST_BASE+ADDR_ST_SR) & 1) == 1 )
            irq_systimer(sp);
        else if ( ( DEREF_INT(ADDR_DBGU_BASE+ADDR_DBGU_SR) & 1 ) == 1 )
            irq_serial_read();
	    else
            printf("Unknown IRQ 1\n");
    }
    else if ( num == 24 )
    {
        ethernet_irq();
    }
    else printf("Unknown IRQ (%x)\n", num);
    
    // Clear the interrupt line
    DEREF_INT(ADDR_AIC_BASE+ADDR_AIC_EOICR) = 0xffffffff;
}

