#ifndef BOARD_H
#define BOARD_H

// MC - Memory Controller
#define ADDR_MC_BASE     0xFFFFFF00
#define ADDR_MC_REMAP    0x00 // Remap command

// AIC - Advanced Interrupt Controller
#define ADDR_AIC_BASE    0xFFFFF000
#define ADDR_AIC_SMR0    0x00
#define ADDR_AIC_SVR0    0x80
#define ADDR_AIC_SVR1    0x84
#define ADDR_AIC_SVR2    0x88
#define ADDR_AIC_SVR24   ADDR_AIC_SVR0 + 24*4
#define ADDR_AIC_IVR     0x100 // Interrupt Vector
#define ADDR_AIC_ISR     0x108 // Interrupt Source Number
#define ADDR_AIC_IMR     0x110 // Interrupt Mask
#define ADDR_AIC_CISR    0x114 // Core Status Register
#define ADDR_AIC_IECR    0x120 // Enable command
#define ADDR_AIC_ICCR    0x128 // Clear command
#define ADDR_AIC_EOICR   0x130 // End Of Interrupt command

// ST - System Timer
#define ADDR_ST_BASE     0xFFFFFD00
#define ADDR_ST_PIMR     0x04 // Period
#define ADDR_ST_SR       0x10 // Status
#define ADDR_ST_IER      0x14 // Interrupt Enable
#define ADDR_ST_CRTR     0x24 // Current Real Time Register

// RTC - Real-Time Clock
#define ADDR_RTC_BASE    0xFFFFFE00
#define ADDR_RTC_CR      0x00
#define ADDR_RTC_MR      0x04
#define ADDR_RTC_TIMR    0x08
#define ADDR_RTC_CALR    0x0C

#define ADDR_TC_BASE    0xFFFA00000
#define ADDR_TC_RA_C0   0x14

// DBGU - Debug Unit
#define ADDR_DBGU_BASE   0xFFFFF200
#define ADDR_DBGU_IER    0x08
#define ADDR_DBGU_IMR    0x10
#define ADDR_DBGU_SR     0x14

// Ethernet
#define ADDR_ETH_BASE    0xFFFBC000
#define ADDR_ETH_CTL     0x00
#define ADDR_ETH_CFG     0x04
#define ADDR_ETH_SR      0x08
#define ADDR_ETH_TAR     0x0C
#define ADDR_ETH_TCR     0x10
#define ADDR_ETH_TSR     0x14
#define ADDR_ETH_RBQP    0x18
#define ADDR_ETH_RSR     0x20
#define ADDR_ETH_ISR     0x24
#define ADDR_ETH_IER     0x28
#define ADDR_ETH_IDR     0x2C
#define ADDR_ETH_IMR     0x30
#define ADDR_ETH_MAN     0x34

// Macro for quickly reading and writing integer values
#define DEREF_INT(addr) *((int*)(addr))

#endif

