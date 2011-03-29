#include "board.h"
#include "interrupt.h"
#include "lib.h"
#include "kmem.h"
#include "cpsr.h"
#include "process.h"
#include "syscall.h"
#include "process.h"
#include "tasks.h"
#include "net/ethernet.h"
#include "net/arp.h"
#include "net/tcp.h"

void __attribute__ ((naked)) start() 
{
    // Remap
    DEREF_INT(ADDR_MC_BASE+ADDR_MC_REMAP) = 1;
    
    // Initialize kernel memory
    kmem_init();
    
    // Reserve stack memory and setup IRQ and SVC status and stack
    char *svcstack = kmem_alloc(1024*10);
    svcstack += 1024*100; // Point to the end of the stack
    interrupt_swistack = svcstack;
    printf("SVC stack: %p\n", svcstack);
    char *irqstack = kmem_alloc(1024*100);
    irqstack += 1024*100; // Point to the end of the stack
    printf("IRQ stack: %p\n", irqstack);
    asm(
        "mrs r1, cpsr\n"           // save cpsr
        "mov r0, #0x12\n"          // set mode to IRQ
        "orr r0, #0x80\n"          // set interrupt mask bit
        "msr cpsr, r0\n"           // switch mode to IRQ
        "mov r13, %[val1]\n"       //
        "bic r1, r1, #0x1f\n"      // clear mode
        "orr r1, #0xD3\n"          // set mode to SVC
        "msr cpsr, r1\n"           // switch mode to SVC
        "mov r13, %[val2]\n"       //
        : : [val1] "r" (irqstack), [val2] "r" (svcstack)
        : "r0", "r1", "sp"
    );
    
    // Setup interrupt vectors.
    DEREF_INT(0x00) = 0xe59ff018; // Reset
    DEREF_INT(0x04) = 0xe59ff018; // Undefined
    DEREF_INT(0x08) = 0xe59ff018; // SWI
    DEREF_INT(0x0C) = 0xe59ff018; // Prefetch Abort
    DEREF_INT(0x10) = 0xe59ff018; // Data Abort
    DEREF_INT(0x18) = 0xe51fff20; // IRQ
    DEREF_INT(0x1C) = 0xe59ff018; // FIQ
    
    // Write addresses of interrupt handlers behind the vector table
    DEREF_INT(0x00+0x20) = (int)&interrupt_resethandler;
    DEREF_INT(0x04+0x20) = (int)&interrupt_undefhandler;
    DEREF_INT(0x08+0x20) = (int)&interrupt_swi;
    DEREF_INT(0x0C+0x20) = (int)&interrupt_paborthandler;
    DEREF_INT(0x10+0x20) = (int)&interrupt_daborthandler;
    DEREF_INT(0x1C+0x20) = (int)&interrupt_fiqhandler;
    
    // Init systimer
    DEREF_INT(ADDR_ST_BASE+ADDR_ST_PIMR) = 0x100;
    DEREF_INT(ADDR_ST_BASE+ADDR_ST_IER) = 1;
    
    // Init DBGU (serial port)
    DEREF_INT(ADDR_DBGU_BASE+ADDR_DBGU_IER) = 1;
    
    // Init AIC
    DEREF_INT(ADDR_AIC_BASE+ADDR_AIC_SVR1) = (int)&interrupt_irq;
    DEREF_INT(ADDR_AIC_BASE+ADDR_AIC_SVR24) = (int)&interrupt_irq;
    
    DEREF_INT(ADDR_AIC_BASE+ADDR_AIC_IECR) = 0x2 | 1<<24;
    // Clear all interrupts just to be sure
    DEREF_INT(ADDR_AIC_BASE+ADDR_AIC_ICCR) = 0x2;
    DEREF_INT(ADDR_AIC_BASE+ADDR_AIC_EOICR) = 0x1;
    
    // Init Ethernet
    ethernet_setup_rbq();
    memcpy(ethernet_opt_address, "\x00\x50\xc2\x3a\xb9\xe7", 6);
    printf("ETH_CTL: %x\r\n", DEREF_INT(ADDR_ETH_BASE+ADDR_ETH_CTL));
    DEREF_INT(ADDR_ETH_BASE+ADDR_ETH_IER) = 6;
    DEREF_INT(ADDR_ETH_BASE+ADDR_ETH_IMR) = 0;
    printf("ETH_IMR: %x\r\n", DEREF_INT(ADDR_ETH_BASE+ADDR_ETH_IMR));
    
    // Init ARP
    arp_table.next = NULL;
    arp_queue.next = NULL;
    
    // Init IP
    ip_opt_address = pton(192,168,1,107);
    ip_opt_netmask = pton(255,255,255,0);
    ip_opt_gateway = pton(192,168,1,1);
    
    // Init TCP
    tcp_timeout = TCP_TIMEOUT;
    tcp_iss = 123;
    
    // Init processes
    process_count = 0;
    process_current = &process_dummy;
    process_dummy.next = &process_dummy;
    process_head = NULL;
    process_tail = NULL;
    
    process_add(&idler);
    process_add(&initprocess);
    
    // Enable usermode and interrupts and switch to the first process
    asm volatile(
        "mrs r0, cpsr\n"
        "bic r0, r0, #0x1f\n"
        "orr r0, #0x10\n" 
        "bic r0, r0, #0x80\n"
        "msr cpsr, r0\n"
        "mov r0, #0\n"
        //"b syscall\n"
    );
    while(1);
}

