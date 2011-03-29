

#ifndef _SERIAL_H
#define	_SERIAL_H

// Basisadresse des Controllers für die serielle Schnittstelle
#define SERIAL_BASE     (0xFFFFF200)

// Offset für das "Channel Status Register"
// Bit 0 gesetzt = Zeichen sind empfangen wurden und warten auf Abholung
// Bit 1 gesetzt = Zeichen dürfen für das Versenden an die serielle Schnittstelle
//                 gesendet werden
#define SERIAL_CSR      (volatile unsigned long*)(SERIAL_BASE + 0x14)

// Offset für das Transmitter Holding Register
// Stell das Schreibregister der seriellen Schnittstelle dar, in das Zeichen
// zum Versenden geschrieben werden können
#define SERIAL_THR      (volatile unsigned long*)(SERIAL_BASE + 0x1C)

#define SERIAL_TX_RDY   (1 << 1)

void putchar(char ch);
void write(char* str);
void writeln(char *string);


#endif	/* _SERIAL_H */

