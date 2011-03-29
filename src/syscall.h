#ifndef SYSCALL_H
#define SYSCALL_H

enum syscalls
{
    ADDPROCESS,
    YIELD,
    SLEEP,
    NOTIFY,
    RECV,
    SEND,
    UDPSEND,
    UDPRECV,
    UDPBIND,
    UDPSOCKET, 
    UDPCLOSE,
    TCPCONNECT,
    TCPSEND,
    TCPRECV,
    TCPBIND,
    TCPSOCKET, 
    TCPCLOSE,
    TCPLISTEN
};

struct syscall_arguments
{
    int arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10;
};

int syscall_handler(int num, struct syscall_arguments *args);

#endif

