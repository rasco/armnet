#ifndef PROCESS_H
#define PROCESS_H

#include "net/udp.h"
#include "net/tcp.h"
#include "types.h"

struct process
{
    struct process *next;
    int registers[16];
    int cpsr;
    
    int id;
    void *stack;
    BOOL sleep;
    
    struct udp_sock udpcb;
    struct tcp_sock tcpcb;
};

struct process *process_current, *process_head, *process_tail;
struct process process_dummy;
int process_count;

void process_switch();
void process_add(void (*f)());

#endif

