#include "lib.h"
#include "process.h"
#include "kmem.h"

void process_switch()
{
    //printf("switching from %d to ", process_current->id);
    process_current = process_current->next;
    while ( process_current->sleep == TRUE )
        process_current = process_current->next;
    //printf(" %d\n", process_current->id);
}

void process_add(void (*f)())
{
    char *newpstack;
    struct process *newp;
    
    // Allocate memory for the process and insert it into the list
    newp = kmem_alloc(sizeof(struct process));
    if ( process_head == NULL )
    {
        // This case is only for the very first process added
        process_head = newp;
        process_tail = newp;
        process_current->next = newp; // Let dummy process point to the new one
        newp->next = newp;
    }
    else
    {
        newp->next = process_head;
        process_tail->next = newp;
        process_tail = newp;
    }
    newp->id = process_count++;
    
    // replace with udp_init_pcb(&newp->udpcb)
    newp->udpcb.next = NULL;
    newp->udpcb.id = 0;
    
    // replace with tcp_init_pcb(&newp->tcpcb)
    newp->tcpcb.next = NULL;
    newp->tcpcb.id = 0;
    
    // Setup the process' stack
    newpstack = kmem_alloc(1024*100);
    newpstack += 1024*100; // Point to the end of the stack
    newp->stack = newpstack;
    
    printf("New process: stack pointer: %p\n", newpstack);
    
    // Initial register values
    newp->registers[15] = (int)(f);
    newp->registers[13] = (int)newpstack;
    newp->cpsr = 0x00000050; // User mode with FIQ disabled
    
    // Options
    newp->sleep = FALSE;
}
