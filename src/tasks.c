#include "tasks.h"
#include "lib.h"
#include "syscall.h"
#include "board.h"
#include "net/ip.h"
#include "net/udp.h"
#include "net/tcp.h"

void tcplistener();
void tcpsender();

void initprocess()
{
    printf("System started. Press 'p', 't' or 'u' to get information\n");
    sntpclient();
    //addprocess(&tcplistener);
    //addprocess(&tcptester);
    //addprocess(&tcpsender);
    while(1)
    {
        yield();
    }
}

void sntpclient()
{
    unsigned int t;
    int n = 0;
    char buf[48];
    struct sockaddr addr;
    int sock;
    
    printf("Getting network time...\n");
    sock = udpsocket();
    addr.port = (12355);
    n = udpbind(sock, &addr);
    if ( n != 0 )
    {
        printf("sntpclient: could not bind socket\n");
        while(1);
    }
    addr.addr = (pton(130,149,17,21));
    addr.port = (123);
    memset(buf, 0, 48);
    buf[0] = 0x1b;
    udpsendto(sock, &addr, buf, 48);
    n = udprecvfrom(sock, &addr, buf, 48);
    if ( n <= 0 )
    {
        printf("couldnt get network time! %d\n", n);
        while(1);
    }
    memcpy(&t, buf+40, 4);
    t = swapl(t);
    t -= 2208988800u;
    printf("NTP timestamp: %d\n", t);
    tcp_iss = t;
    udpclose(sock);
}

void tcplistener()
{
    int s, n;
    char buf[2048];
    struct sockaddr addr;
    
    while(1)
    {
        s = tcpsocket();
        printf("Socket: %d\n", s);
        addr.port = 23;
        while (tcpbind(s, &addr) != 0);
        //printf("Bind: %d\n", n);
        printf("Listening...\n");
        n = tcplisten(s);
        printf("listen: %d\n", n);
        printf("Connection accepted.\n");
        n = tcprecv(s, buf, 1000);
        if ( n == 0 || n == -1 )
        {
            tcpclose(s);
            continue;
        }
        printf("received %d bytes\n", n);
        hexdump(buf, n);
        sprintf(buf, "Hello and bye!\n");
        tcpsend(s, buf, strlen(buf));
        tcpclose(s);
        n=0;
        
        //while(n++<5000000);
    }
    while(1);
}

void tcptester()
{
    int i = 0, n = 0, total = 0;
    int r = 0;
    char buf[90000];
    struct sockaddr addr;
    int sock;
    int t1, t2;
    
    while(1)
    {
        sock = tcpsocket();
        
        printf("Socket: %d\n", sock);
        
        addr.port = 34995+(i++);
        n = tcpbind(sock, &addr);
        
        printf("Bind: %d\n", n);
        
    #if 1
        addr.addr = pton(192,168,1,102);
        addr.port = 1234;
    #endif
    #if 0
        addr.addr = pton(85,10,200,174);
        addr.port = 80;
    #endif
    #if 0
        addr.addr = pton(193,99,144,80);
        addr.port = 80;
    #endif
        
        n = tcpconnect(sock, &addr);
        
        printf("Connect: %d\n", n);
        
        if ( n != 0 )
        {
            tcpclose(sock);
            continue;
        }
        total = 0;
        sprintf(buf, "GET / HTTP/1.1%c%cHost: www.heise.de%c%c%c%c\x00", 0x0d, 0x0a, 0x0d, 0x0a, 0x0d, 0x0a);
        tcpsend(sock, buf, strlen(buf));
        printf("Request sent. Waiting for reply...\n");
        
        t1 = DEREF_INT(ADDR_ST_BASE+ADDR_ST_CRTR) & 0xFFFFF;
        while(1)
        {
            n = tcprecv(sock, buf, 89000);
            if ( n == 0 || n == -1 )
            {
                printf("Disconnected\n");
                printf("%d bytes received\n", total);
                tcpclose(sock);
                t2 = DEREF_INT(ADDR_ST_BASE+ADDR_ST_CRTR) & 0xFFFFF;
                printf("t1: %d, t2: %d, t2-t1: %d\n", t1, t2, t2-t1);
                while(1);
                break;
            }
            total += n;
            printf("\nReceived %d bytes (%d total)\n", n, total);
        }
    }
    
}

void tcpsender()
{
    int i = 0, n = 0, total = 0;
    int r = 0;
    char buf[10000];
    struct sockaddr addr;
    int sock;
    int t1, t2;
    
    while(1)
    {
        sock = tcpsocket();
        
        printf("Socket: %d\n", sock);
        
        addr.port = 34995+(i++);
        n = tcpbind(sock, &addr);
        
        printf("Bind: %d\n", n);

        addr.addr = pton(192,168,1,102);
        addr.port = 1234;

        n = tcpconnect(sock, &addr);
        
        printf("Connect: %d\n", n);
        
        if ( n != 0 )
        {
            tcpclose(sock);
            printf("Connection failed\n");
            while(1);
        }
        total = 0;
        
        printf("Connected\n");
        
        t1 = DEREF_INT(ADDR_ST_BASE+ADDR_ST_CRTR) & 0xFFFFF;
        while(1)
        {
            n = tcpsend(sock, buf, 1500);
            if ( n == -1 )
                break;
            total += n;
            if ( total > 1024*1024*5 )
            {
                tcpclose(sock);
                t2 = DEREF_INT(ADDR_ST_BASE+ADDR_ST_CRTR) & 0xFFFFF;
                printf("t1: %d, t2: %d, t2-t1: %d\n", t1, t2, t2-t1);
                while(1);
            }
        }
    }
    
}

void udplistener()
{
    int n = 0;
    int r = 0;
    char buf[2048];
    struct sockaddr addr;
    
    int sock;
    
    sock = udpsocket();
    
    printf("Created socket: %d\n", sock);
    
    addr.port = swaps(15534);
    n = udpbind(sock, &addr);
    
    if ( n != 0 )
    {
        printf("Failed binding the socket\n");
        while(1)
        {
            yield();
        }
    }
    
    while(1)
    {
        n = udprecvfrom(sock, &addr, buf, 50);
        printf("Recv result: %d\n", n);
        buf[n-1] = 0;
        printf("%s\n", buf);
        sprintf(buf, "Hello, subject number %d\n", r);
        udpsendto(sock, &addr, buf, strlen(buf));
        r++;
    }
}

void udplistener2()
{
    int n = 0;
    int r = 0;
    char buf[2048];
    struct sockaddr addr;
    
    int sock;
    
    sock = udpsocket();
    
    printf("L2 Created socket: %d\n", sock);
    
    addr.port = swaps(15534);
    n = udpbind(sock, &addr);
    
    if ( n != 0 )
    {
        printf("L2 Failed binding the socket\n");
        while(1)
        {
            yield();
        }
    }
    
    while(1)
    {
        n = udprecvfrom(sock, &addr, buf, 50);
        printf("L2 Recv result: %d\n", n);
        //hexdump(buf, n);
        buf[n-1] = 0;
        printf("%s\n", buf);
        sprintf(buf, "L2 Hello, subject number %d\n", r);
        udpsendto(sock, &addr, buf, strlen(buf));
        r++;
    }
}

void idler()
{
    int i = 0;
    while(1)
    {
        //yield();
    }
}

void p1loop()
{
    int i = 0;
    int n = 0, m = 1;
    while(1)
    {
        while(n++ < 20)
        {
            while(i++<100000);
            printf("A");
            i = 0;
        }
        printf("\n");
        n = 0;
        i = 0;
    }
}

void p2loop()
{
    int i = 0;
    int n = 0;
    while(1)
    {
        while(n++ < 30)
        {
            while(i++<100000);
            printf("B");
            i = 0;
        }
        printf("Notify result: %d\n", n);
        n = 0;
    }
}

void p3loop()
{
    int i = 0;
    int n = 0, m = 1;
    while(1)
    {
        while(i++<100000);
        printf("C");
        i = 0;
    }
}

void p4loop()
{
    int i = 0;
    int n = 0, m = 1;
    while(1)
    {
        while(i++<100000);
        printf("D");
        i = 0;
    }   
}

void p5loop()
{
    int i = 0;
    int n = 0, m = 1;
    while(1)
    {
        while(i++<100000);
        printf("E");
        i = 0;
    }   
}

void p6loop()
{
    int i = 0;
    int n = 0, m = 1;
    while(1)
    {
        while(i++<100000);
        printf("F");
        i = 0;
    }   
}

