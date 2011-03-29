#include "lib.h"
#include "serial.h"
#include "syscall.h"

unsigned int pton(char b1, char b2, char b3, char b4)
{
    return b1<<24 | b2<<16 | b3<<8 | b4;
}

unsigned int swapl(unsigned int in)
{
    return ((in >> 24) | (in << 24) 
        | ((in & 0xff00) << 8) | ((in >> 8) & 0xff00));
}

unsigned short swaps(unsigned short in)
{
    return (in >> 8) | (in << 8);
}


void addprocess(void (*f)())
{
    struct syscall_arguments args;
    
    args.arg1 = (int)f;
    
    syscall(ADDPROCESS, &args);
}

int yield()
{
    struct syscall_arguments args;
    
    return syscall(YIELD, &args);
}

int udprecvfrom(int socket, struct sockaddr *src, char *buf, unsigned int len)
{
    struct syscall_arguments args;
    
    args.arg1 = socket;
    args.arg2 = (int)src;
    args.arg3 = (int)buf;
    args.arg4 = len;
    
    return syscall(UDPRECV, &args);
}

int udpsendto(int socket, struct sockaddr *dst, char *buf, unsigned int len)
{
    struct syscall_arguments args;
    
    args.arg1 = socket;
    args.arg2 = (int)dst;
    args.arg3 = (int)buf;
    args.arg4 = len;
    
    return syscall(UDPSEND, &args);
}

int udpsocket()
{
    return syscall(UDPSOCKET, NULL);
}

int udpbind(int socket, struct sockaddr *src)
{
    struct syscall_arguments args;
    
    args.arg1 = socket;
    args.arg2 = (int)src;
    
    return syscall(UDPBIND, &args);
}

int udpclose(int socket)
{
    struct syscall_arguments args;
    
    args.arg1 = socket;
    
    return syscall(UDPCLOSE, &args);
}

int tcpconnect(int socket, struct sockaddr *dst)
{
    struct syscall_arguments args;
    
    args.arg1 = socket;
    args.arg2 = (int)dst;
    
    return syscall(TCPCONNECT, &args);
}

int tcprecv(int socket, char *buf, unsigned int len)
{
    struct syscall_arguments args;
    
    args.arg1 = socket;
    args.arg2 = (int)buf;
    args.arg3 = len;
    
    return syscall(TCPRECV, &args);
}

int tcpsend(int socket, char *buf, unsigned int len)
{
    struct syscall_arguments args;
    
    args.arg1 = socket;
    args.arg2 = (int)buf;
    args.arg3 = len;
    
    return syscall(TCPSEND, &args);
}

int tcpsocket()
{
    return syscall(TCPSOCKET, NULL);
}

int tcpbind(int socket, struct sockaddr *src)
{
    struct syscall_arguments args;
    
    args.arg1 = socket;
    args.arg2 = (int)src;
    
    return syscall(TCPBIND, &args);
}

int tcplisten(int socket)
{
    struct syscall_arguments args;
    
    args.arg1 = socket;
    
    return syscall(TCPLISTEN, &args);
}

int tcpclose(int socket)
{
    struct syscall_arguments args;
    
    args.arg1 = socket;
    
    return syscall(TCPCLOSE, &args);
}

// Issue system call
// Syscall number will be in r0, args in r1
int __attribute__ ((naked)) syscall(int num, struct syscall_arguments *args)
{
    asm(
        "push {lr}\n"
        "swi #0\n"
        "pop {pc}\n"
    );
    
    return 0;
}

char* hexChar = "0123456789ABCDEF";
void hexdump(char *buf, int len)
{
    int m,n;
    for ( m=16,n = 0; n < len; n++ ) {
        if ( n == m ) {
            printf("\n"); 
            m += 16;
        }
        else if ( n == m-8 )
            printf(" ");
        printf("%02x ", buf[n]);
    }
    printf("\n");
}


inline char convertNibleHex(unsigned char b){
    return hexChar[b];
}

void *memset (void *s, int c, unsigned long n)
{
    char *p = s;
    while (n--) *p++ = (char)c;
    return s;
}

int memcmp(const char *dest, const char *src, int n)
{
    while (--n > 0 && (*dest == *src)) {
        dest++;
        src++;
    }

    return(*dest - *src);
}

void memcpy(void *t, const void *src, unsigned long size) 
{
    char *newT = (char*)t;
    char *newS = (char*)src;
    unsigned int i;
    for(i=0;i<size;i++) {
        newT[i] = newS[i];
    }			
}

char *strcpy(char *d, const char *s)
{
    char    *rval = d;

    while((*d++ = *s++));
    return(rval);
}


char *strncpy(char *d, const char *s, int len)
{
    char    *rval = d;

    while(len-- > 0 && (*d++ = *s++));
    while(len-- > 0)    *d++ = 0;
    return(rval);
}


int strcmp(const char *dest, const char *src)
{
    while (*dest && (*dest == *src)) {
        dest++;
        src++;
    }

    return(*dest - *src);
}


int strncmp(const char *dest, const char *src, int n)
{
    while (--n > 0 && *dest && (*dest == *src)) {
        dest++;
        src++;
    }

    return(*dest - *src);
}


int strlen(const char *s)
{
    register int    n;

    for(n = 0; *s++; n++);
    return(n);
}

int atoi(const char *s)
{
    unsigned val = 0;
    int      neg = 0;

    while (*s == ' ' || *s == '\t')  s++; // ignore beginning white space 
    if    (*s == '-')                s++, neg = 1;
    while (*s >= '0' && *s <= '9')   val = 10 * val + (*s++ - '0'); //convert integer value

    return neg? -(int)val : (int)val;
}
/********* ab hier************/
void __putchar(char c, char** str){
    **str = c;
    (*str)++;
}

int __putdec(int n, int base, char** str)
{
    int new_base = base * 10;

    if(n >= new_base) n = __putdec(n, new_base, str);

    char a;
    if(n >= 0){
        for(a = '0' - 1; n >= 0; n-=base){
            ++a;
        }
    } else {
        for(a = '9' + 1; n < 0; n+= base) --a;
    }

    __putchar(a, str);
    return n;
}

//for base 16 and 8
void __putother(unsigned int n, char** str, int base){
    int show = 0;
    int shift = base == 16 ? 28 : 30;
    int bits = base == 16 ? 4 : 3;
    base--; //create mask

    for(; shift >= 0; shift-=bits){
        unsigned int i = (n >> shift) & base;
        if(i > 0 || show || !shift){
            __putchar( convertNibleHex(i), str);
            show = 1;
        }
    }
}

//base can be 10, 16 or 8
int unsigned_to_str(char *s, int maxlen, unsigned value, int base)
{
    char field[32];                            /* Zwischenspeicher                             */
    char *orig_s = s;                          /* zur Berechnung der konvertierten Zeichen     */
    char *limit = s + maxlen;                  /* zeigt hinter das letzte benutzbare Zeichen   */
    char *f = field;

    if (value == 0){                           /* bei Wert == 0 ist die Konvertierung simpel   */
        if (maxlen > 0) {
            *s = '0';
            return 1;
        } else return 0;
    } 
    if(base == 10){
        int n = value;
        if(n >= 10){
            n = __putdec(value, 10, &f);
            n = n < 0 ? 10 + n : n;
        }
        __putchar(n + '0', &f);
    } else __putother(value, &f, base);

    //copy number string
    char *tf = field;
    while(tf < f && s < limit)
        *s++ = *tf++;

    return s - orig_s;                                                  /* Anzahl der Zeichen zurueckgeben              */
}

static void pad_string(char **dest, char *last, int count, int filler)
{
    char *s = *dest;

    while (count > 0 && s < last)
    {
        *s++ = filler;
        count--;
    }
    *dest = s;
}

int vsnprintf(char *dest, int maxlen, char *format, int *argp)
{
    char field[32];                                           /* zum Formatieren der Zahlen                   */
    char *orig_dest = dest;                                           /* zur Berechnug der Zeichenanzahl              */
    char *last = dest + (maxlen - 1);                                     /* letzte benutzbare Speicherstelle             */
    char c;                                             /* zum Speichern eines char-Parameters          */
    char *s;                                              /* Laufvariable                                 */
    int len;                                              /* temp. Variable                               */
    int width;                                              /* Breite der Ausgabe                           */
    int d;                                              /* zum Speichern eines int-Parameters           */
    int u;                                              /* zum Speichern eines unsigned-Parameters      */
    int left_justify;                                           /* linksbuendige Ausgabe                        */
    int filler;                                             /* Zeichen zur buendigen Ausgabe                */
    int alternate;                                            /* Alternative Darstellung (man -s 3s printf)   */
    int use_sign;                                             /* immer Vorzeichen drucken                     */

    while (*format)
    {
        switch (*format)
        {
            case '%':
                format++;                                           /* Formatanweisung ueberspringen                */

                if (*format == '#')                                         /* alternative Darstellungsform                 */
                {
                    alternate = 1;
                    format++;
                }
                else
                    alternate = 0;
                if (!*format)                                           /* Abruch der Schleife                          */
                    continue;

                if (*format == '-')                                         /* linksbuendige Ausgabe                        */
                {
                    left_justify = 1;
                    format++;
                }
                else
                    left_justify = 0;
                if (!*format)                                           /* Abruch der Schleife                          */
                    continue;

                if (*format == '+')                                         /* immer Vorzeichen ausgeben                    */
                {
                    use_sign = 1;
                    format++;
                }
                else
                    use_sign = 0;
                if (!*format)                                           /* Abruch der Schleife                          */
                    continue;

                if (*format == '0')                                         /* fuehrende Nullen fuer's padding              */
                {
                    filler = '0';
                    format++;
                }
                else
                    filler = ' ';
                if (!*format)                                           /* Abruch der Schleife                          */
                    continue;

                if (*format >= '0' && *format <= '9')                           /* Breite der Ausgabe                           */
                {
                    width = *format - '0';
                    format++;
                    while (*format >= '0' && *format <= '9')
                    {
                        width = width * 10 + (*format - '0');
                        format++;
                    }
                }
                else
                    width = 0;

                if (!*format)                                           /* Abbruch der Schleife                         */
                    continue;

                switch (*format)
                {
                    case 'c':                                                     /* character                                    */
                        c = (char) *argp;
                        argp++;

                        width--;
                        if (!left_justify)
                            pad_string(&dest, last, width, ' ');
                        if (dest < last)
                            *dest++ = c;
                        if (left_justify)
                            pad_string(&dest, last, width, ' ');
                        break;

                    case 's':                                                     /* string                                       */
                        s = (char *) *argp;
                        argp++;
                        len = strlen(s);
                        width -= len;

                        if (!left_justify)
                            pad_string(&dest, last, width, ' ');
                        while (*s && dest < last)
                            *dest++ = *s++;
                        if (left_justify)
                            pad_string(&dest, last, width, ' ');
                        break;

                    case 'd':                                                     /* signed integer                               */
                    case 'i':
                        d = (int) *argp;
                        argp++;
                        if (dest < last)
                        {
                            if (d < 0)
                            {
                                *dest++ = '-';
                                d = -d;
                                width--;
                            }
                            else if (use_sign)
                            {
                                *dest++ = '+';
                                width--;
                            }

                            len = unsigned_to_str(field, sizeof(field) - 1, d, 10);
                            field[len] = '\0';
                            width -= len;
                            if (!left_justify)
                                pad_string(&dest, last, width, filler);
                            s = field;
                            while (*s && dest < last)
                                *dest++ = *s++;
                            if (left_justify)
                                pad_string(&dest, last, width, filler);
                        }
                        break;

                    case 'u':                                                     /* unsigned integer                             */
                        u = (unsigned int) *argp;
                        argp++;
                        len = unsigned_to_str(field, sizeof(field) - 1, u, 10);
                        field[len] = '\0';
                        width -= len;
                        if (!left_justify)
                            pad_string(&dest, last, width, filler);
                        s = field;
                        while (*s && dest < last)
                            *dest++ = *s++;
                        if (left_justify)
                            pad_string(&dest, last, width, filler);
                        break;

                    case 'o':                                                     /* integer, Oktaldarstellung                    */
                        u = (int) *argp;
                        argp++;
                        len = unsigned_to_str(field, sizeof(field) - 1, u, 8);
                        field[len] = '\0';
                        width -= len;
                        if (alternate)
                            width--;
                        if (!left_justify)
                            pad_string(&dest, last, width, filler);
                        if (alternate)
                            if (dest < last)
                                *dest++ = '0';
                        s = field;
                        while (*s && dest < last)
                            *dest++ = *s++;
                        if (left_justify)
                            pad_string(&dest, last, width, filler);
                        break;

                    case 'p':                                                     /* pointer                                      */
                        filler = '0';
                        width = 10;
                        alternate = 1;                                              /* FALLTHRU!!!                                  */

                    case 'x':                                                     /* integer, Hexdarstellung                      */
                        d = (int) *argp;
                        argp++;
                        len = unsigned_to_str(field, sizeof(field) - 1, d, 16);
                        field[len] = '\0';
                        width -= len;
                        if (alternate)
                        {
                            width -= 2;
                            if (dest < last)
                                *dest++ = '0';
                            if (dest < last)
                                *dest++ = 'x';
                        }
                        if (!left_justify)
                            pad_string(&dest, last, width, filler);
                        s = field;
                        while (*s && dest < last)
                            *dest++ = *s++;
                        if (left_justify)
                            pad_string(&dest, last, width, filler);
                        break;

                    default:                                                      /* alle anderen Formatangaben einfach kopieren, */
                        if (dest < last)                                            /* z.B. bei `%%'                                */
                            *dest++ = *format;
                        break;
                }
                break;
		
	    case '\n':
		*dest++ = '\r';
		*dest++ = '\n';
		break;

            default:                                                          /* Zeichen ohne `%' normal kopieren             */
                if (dest < last)
                    *dest++ = *format;
                break;
        }
        format++;
    }
    *dest++ = '\0';                                                       /* String terminieren                           */

    return dest - orig_dest;                                              /* Anzahl der konvertierten Zeichen zurueck     */
}

void printf (char *str, ...) {
    char dest [1024];
    int* argp = (int*) (PTR_ADDR(&str) + sizeof(int));
    vsnprintf(dest, 1024, str, argp);
    write(dest);
}

void sprintf (char *dst, char *str, ...) {
    int* argp = (int*) (PTR_ADDR(&str) + sizeof(int));
    vsnprintf(dst, 1024, str, argp);
}

void dprintf (char *str, ...)
{
#ifdef DEBUG
    char dest [1024];
    int* argp = (int*) (PTR_ADDR(&str) + sizeof(int));
    vsnprintf(dest, 1024, str, argp);
    write(dest);
#endif
}
