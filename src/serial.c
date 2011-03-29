

#include "serial.h"

/**********************************************
 * Funktionen fuer die serielle Schnittstelle
 **********************************************/


/**
 * Wartet aktiv auf die serielle Schnittstelle, bis diese bereit ist ein Zeichen
 * zu verarbeiten und sendete dann ch.
 */
void putchar(char ch)
{
    while(!(*SERIAL_CSR & SERIAL_TX_RDY));

    *SERIAL_THR = ch;
}

/**
 * Mit Hilfe von putchar wird eine null-terminierte Zeichenkette Zeichen für
 * Zeichen an die serielle Schnittstelle geschickt
 */
void write(char* str)
{
    while(*str)             // Solange wiederholen, bis wir das letzte Zeichen(=NULL) bearbeiten
        putchar(*str++);    // Zeichen ausgeben und Zeiger um EINS erhöhen
}

void writeln(char* str)
{
    write(str);
    write("\r\n");
}

