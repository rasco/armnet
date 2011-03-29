#ifndef _CPSR_H
#define _CPSR_H

#define PSR_IRQ_MASK (1 << 7)
#define PSR_FIQ_MASK (1 << 6)

#define CPU_USER_MODE                (0x10)
#define CPU_SYSTEM_MODE              (0x1F)
#define CPU_FIR_MODE                 (0x11)
#define CPU_IRQ_MODE                 (0x12)
#define CPU_SUPERVISOR_MODE          (0x13)
#define CPU_UNDEFINED_EXCEPTION_MODE (0x1B)
#define CPU_ABORT_MODE               (0x17)

#define CPU_NO_INTERRUPTS            (0xc0)

void set_cpsr(unsigned long new_cpsr);
unsigned long get_cpsr();

void set_spsr(unsigned long new_spsr);
unsigned long get_spsr();

void set_sp(void* new_sp);

void *get_sp();

#endif  /* _CPSR_H */
