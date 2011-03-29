#include "interrupt.h"
#include "lib.h"
#include "irq.h"
#include "cpsr.h"
#include "syscall.h"
#include "process.h"

/* The SWI handler entry and exit point. It works much like a function call.
 * First, in SVC mode, push user pc (lr) and user cpsr (spsr) on the SVC stack.
 * Then enter SYSTEM mode. Push user fp (because it is used), and copy user pc
 * and user cpsr from the SVC stack to the user stack and then enter the 
 * high-level syscall handler.
 * 
 * The user stack will look like this right before branching to the C handler:
 * _________________
 * | user fp        |
 * | user psr       |
 * | return address |
 * |                | <- user SP points here
 */
void __attribute__ ((naked)) interrupt_swi()
{
    asm volatile(
        "mov sp, %[swistack]\n"
        "stmfd sp!, {lr}\n"         // save user pc
        "mrs r14, spsr\n"           // save user psr
        "stmfd sp!, {r14}\n"        // 
        "MSR cpsr,#0xdf\n"          // switch to system mode
        "push {fp}\n"               // save user fp
        "mov fp, %[swistack]\n"
        "sub fp, #8\n"
        "ldmfd fp!, {r14}\n"        // get user psr from svc stack
        "push {r14}\n"              // push it
        "ldmfd fp!, {lr}\n"         // get user pc from svc stack
        "push {lr}\n"               // push it
                                    // from here on everything is saved on the 
                                    // user stack just like in an ordinary
                                    // function call, but including the psr,
                                    // which we'll need to return to the 
                                    // previous mode.
        "bl syscall_handler\n"      // call c syscall handler
        "push {r0}\n"               // push return value which is in r0
        "add sp, #4\n"
        "pop {lr}\n"
        "pop {r0}\n"
        "pop {fp}\n"
        "msr cpsr, r0\n"            // return to usermode OR system mode (if we
                                    // issued a system call from within a system
                                    // call)
        "ldr r0, [sp, #-16]\n"      // get that return value
        "mov pc, lr\n"              // beam us up scotty!
        : : [swistack] "r" (interrupt_swistack)
        :
    );
}

void __attribute__ ((naked)) interrupt_irq()
{
    int *sp;
    
    asm volatile(
        "sub lr, lr, #4\n"
        "stmfd sp!, {lr}\n"         // save context
        "stmfd sp, {r14}^\n"
        "sub sp, #4\n"
        "stmfd sp, {r13}^\n"
        "sub sp, #4\n"
        "stmfd sp!, {r0-r12}\n"
        "mrs r8, spsr\n"
        "stmfd sp!, {r8}\n"
    );
    
    sp = get_sp();
    process_current->cpsr = sp[0];
    memcpy(process_current->registers, sp+1, 16*4);
    irq_chooser(sp);
    sp[0] = process_current->cpsr;
    memcpy(sp+1, process_current->registers, 16*4);
    
    asm volatile(
        "ldmfd sp!, {r4}\n"
        "msr spsr, r4\n"
        "ldmfd sp!, {r0-r12}\n"
        "ldmfd sp, {r13}^\n"
        "add sp, #4\n"
        "ldmfd sp, {r14}^\n"
        "add sp, #4\n"
        "ldmfd sp!, {lr}\n"
        "movs pc, lr\n"
    );
}

void __attribute__ ((naked)) interrupt_resethandler()
{
    printf("reset interrupt\n");
    while(1);
}

void __attribute__ ((naked)) interrupt_undefhandler()
{
    printf("undef interrupt\n");
    while(1);
}

void __attribute__ ((naked)) interrupt_paborthandler()
{
    printf("prefetch abort interrupt\n");
    while(1);
}

void __attribute__ ((naked)) interrupt_daborthandler()
{
    printf("data abort interrupt\n");
    while(1);
}

void __attribute__ ((naked)) interrupt_fiqhandler()
{
    printf("fiq interrupt\n");
    while(1);
}

