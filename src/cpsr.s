

; void set_cpsr(uint32_t new_cpsr)
.globl set_cpsr
set_cpsr:
  MSR CPSR_c, r0
  mov pc, lr

; uint32_t get_cpsr()
.globl get_cpsr
get_cpsr:
  MRS r0, cpsr
  mov pc, lr

; void set_spsr(uint32_t new_spsr)
.globl set_spsr
set_spsr:
  MSR SPSR, r0
  mov pc, lr

; uint32_t get_spsr()
.globl get_spsr
get_spsr:
  MRS r0, spsr
  mov pc, lr

; void set_sp(void* new_sp)
.globl set_sp
set_sp:
  mov sp, r0
  mov pc, lr

; void *get_sp();
.globl get_sp
get_sp:
   mov r0, sp
   mov pc,lr

; void *get_r10();
.globl get_r10
get_r10:
//   ldr r10, [sp, #4]
   mov r0, r10
   mov pc,lr
