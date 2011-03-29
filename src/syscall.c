#include "lib.h"
#include "process.h"
#include "syscall.h"
#include "interrupt.h"
#include "process.h"
#include "net/udp.h"
#include "net/tcp.h"

int syscall_handler(int num, struct syscall_arguments *args)
{
    interrupt_unmask();
    
    switch (num)
    {
        case ADDPROCESS:
            interrupt_mask();
                process_add((void (*)())args->arg1);
            interrupt_unmask();
        break;
        case YIELD:
            interrupt_sleepinsyscall();
        break;
        
        case UDPSEND:
            return udp_output(args->arg1, (struct sockaddr*)args->arg2, 
                (char *)args->arg3, args->arg4);
        break;
        case UDPRECV:
            return udp_recvfrom(args->arg1, (struct sockaddr*)args->arg2, 
                (char *)args->arg3, args->arg4);
        break;
        case UDPSOCKET: 
            return udp_socket();
        break;
        case UDPBIND:
            return udp_bind(args->arg1, (struct sockaddr*)args->arg2);
        break;
        case UDPCLOSE:
            return udp_close(args->arg1);
        break;
        
        case TCPCONNECT:
            return tcp_connect(args->arg1, (struct sockaddr*)args->arg2);
        break;
        
        case TCPSEND:
            return tcp_send(args->arg1, (char *)args->arg2, args->arg3);
        break;
        case TCPRECV:
            return tcp_recv(args->arg1, (char *)args->arg2, args->arg3);
        break;
        case TCPSOCKET: 
            return tcp_socket();
        break;
        case TCPBIND:
            return tcp_bind(args->arg1, (struct sockaddr*)args->arg2);
        break;
        case TCPCLOSE:
            return tcp_close(args->arg1);
        break;
        case TCPLISTEN:
            return tcp_listen(args->arg1);
        break;
    }
    
    return 0;
}

