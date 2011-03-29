#ifndef __LIB_H__
#define __LIB_H__

#define PTR_ADDR(x) ((unsigned long)x)

#include "types.h"

#include "net/ip.h"
#include "syscall.h"

int udprecvfrom(int socket, struct sockaddr *src, char *buf, unsigned int len);
int udpsendto(int socket, struct sockaddr *dst, char *buf, unsigned int len);
int udpsocket();
int udpbind(int socket, struct sockaddr *src);
int udpclose(int socket);

int tcpconnect(int socket, struct sockaddr *dst);
int tcprecv(int socket, char *buf, unsigned int len);
int tcpsend(int socket, char *buf, unsigned int len);
int tcpsocket();
int tcpbind(int socket, struct sockaddr *src);
int tcplisten(int socket);
int tcpclose(int socket);

unsigned int    pton(char b1, char b2, char b3, char b4);
unsigned int    swapl(unsigned int in);
unsigned short  swaps(unsigned short in);

int yield();
void addprocess(void (*f)());



int __attribute__ ((naked)) syscall(int num, struct syscall_arguments *args);

void hexdump(char *buf, int len);

/*
 * vsnprintf - Dies ist das Arbeitstier der printf-Familie.
 **********************************************************
 *  Parameter:
 *  dest = Ergebins String
 *  maxlen = maximal geschriebene Zeichen + Nullbyte (\0)
 *  format = Source String
 *  argp = Zeiger auf das 1. Element in der Parameterliste
 *  return = Gesamtanzahl geschriebener Zeichen + Nullbyte
 */
int vsnprintf(char *dest, int maxlen, char *format, int *argp);


/****** weitere nützliche Hilfsfunktionen ********/

/*
* Die Funktion memset() fÃ¼llt den Speicherbereich mit bestimmten Werten auf
* *s : Adresse
* c : aufzufÃ¼llender Wert
* n : ersten n Bytes  
*/
void *memset (void *s, int c, unsigned long n);

/*
* Die Funktion memcpy() kopiert eine bestimmte Anzahl von Bytes
* *t: Destination Puffer
* *src: Source Puffer
* size: Anzahl zu kopierender Bytes
*/
void memcpy(void *t, const void *src, unsigned long size) ;

/*
* Die Funktion strcpy() kopiert einen String
* *s : Source String
* *d : Destination String
*/
char *strcpy(char *d, const char *s);

/*
* Die Funktion strncpy() kopiert n Zeichen eines Strings
* *s : Source String
* *d : Destination String
* len : n zu kopierende Zeichen
*/

char *strncpy(char *d, const char *s, int len);

/*
* Die Funktion strcmp() vergleicht zwei Strings
* *src : 1. String
* *dest : 2. String
* Rückgabwert 0 : falls src und dest identisch sind
* Rückgabwert <0 : falls dest kleiner als src
* Rückgabwert >0 : falls dest grÖßer als src
*/
int strcmp(const char *dest, const char *src);

/*
* Die Funktion strcmp() vergleicht n Zeichen zweier Strings
* *src : 1. String
* *dest : 2. String
* n: Anzahl der zu vergleichenden Zeichen
* Rückgabwert 0 : falls src und dest identisch sind
* Rückgabwert <0 : falls dest kleiner als src
* Rückgabwert >0 : falls dest grÃ¶ÃŸer als src
*/
int strncmp(const char *dest, const char *src, int n);

/*
* Die Funktion strlen() ermittelt die Länge eines Strings
* *s : String
* Rückgabewert : Länge des Strings s
*/
int strlen(const char *s);

/*
* Die Funktion atoi() wandelt einen String in einen int-Wert um 
* *s: String
* Rückgabewert : int-Wert
*/
int atoi(const char *s);

/**
 * Die Funktion void printf (char *str, ...) implementiert die Ausgabe 
 * auf der seriellen Schnittstelle
 * Achtung: maximal 1024 Zeichen sind möglich
 * Formatierungszeichen:
 *  %d %i   Decimal signed integer.
 *  %o  Octal integer.
 *  %x %X   Hex integer.
 *  %u  Unsigned integer.
 *  %c  Character.
 *  %s  String. siehe unten.
 *  %f  double
 *  %e %E   double.
 *  %g %G   double.
 *  %p  zeiger.
 *  %n  Number of characters written by this printf.
 *      No argument expected.
 *  %%  %. No argument expected.
*/
void printf (char *str, ...) ;

void sprintf (char *dst, char *str, ...);
void dprintf (char *str, ...);

#endif
