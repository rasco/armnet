#ifndef INTERRUPT_H
#define INTERRUPT_H

// Sets the interrupt bit (enable interrupts)
#define interrupt_mask() \
    asm volatile( \
        "push {r0}\n" \
        "mrs r0, cpsr\n" \
        "orr r0, r0, #0x80\n" \
        "msr cpsr, r0\n" \
        "pop {r0}\n" \
    );

// Clears the interrupt bit (enable interrupts)
#define interrupt_unmask() \
    asm volatile( \
        "push {r0}\n" \
        "mrs r0, cpsr\n" \
        "bic r0, r0, #0x80\n" \
        "msr cpsr, r0\n" \
        "pop {r0}\n" \
    );

// The calling process will be put to sleep. As soon as it is activated and 
// loaded by the timer interrupt it will break out of the loop.
#define interrupt_sleepinsyscall() \
{ \
    interrupt_mask(); \
    process_current->sleep = TRUE; \
    interrupt_unmask(); \
    while(1) \
    { \
        interrupt_mask(); \
        if ( process_current->sleep == FALSE ) \
            break; \
        interrupt_unmask(); \
    } \
}

char *interrupt_swistack;

void __attribute__ ((interrupt ("SWI"))) interrupt_swi();
void interrupt_irq();
void interrupt_resethandler();
void interrupt_undefhandler();
void interrupt_paborthandler();
void interrupt_daborthandler();
void interrupt_fiqhandler();

#endif
