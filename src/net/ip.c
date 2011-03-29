#include "net.h"
#include "ethernet.h"
#include "../lib.h"
#include "ip.h"
#include "udp.h"
#include "tcp.h"

/*	$OpenBSD: in_cksum.c,v 1.6 2007/01/08 19:37:04 deraadt Exp $	*/
/*	$NetBSD: in_cksum.c,v 1.3 1995/04/22 13:53:48 cgd Exp $	*/

/*
 * Copyright (c) 1992 Regents of the University of California.
 * All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * @(#) Header: in_cksum.c,v 1.1 92/09/11 01:15:55 leres Exp  (LBL)
 */
/* http://ftp3.usa.openbsd.org/pub/OpenBSD/src/usr.sbin/ospfd/in_cksum.c
 * Checksum routine for Internet Protocol family headers.
 * This routine is very heavily used in the network
 * code and should be modified for each CPU to be as fast as possible.
 * In particular, it should not be this one.
 */
int
ip_cksum(const void *p, int llen)
{
	int sum = 0, oddbyte = 0, v = 0, len = (int)llen;
	const unsigned char *cp = p;

	/* we assume < 2^16 bytes being summed */
	while (len > 0) {
		if (oddbyte) {
			sum += v + *cp++;
			len--;
		}
		if (((long)cp & 1) == 0) {
			while ((len -= 2) >= 0) {
				sum += *(const unsigned short *)cp;
				cp += 2;
			}
		} else {
			while ((len -= 2) >= 0) {
#if BYTE_ORDER == BIG_ENDIAN
				sum += *cp++ << 8;
				sum += *cp++;
#else
				sum += *cp++;
				sum += *cp++ << 8;
#endif
			}
		}
		if ((oddbyte = len & 1) != 0)
#if BYTE_ORDER == BIG_ENDIAN
			v = *cp << 8;
#else
			v = *cp;
#endif
	}
	if (oddbyte)
		sum += v;
	sum = (sum >> 16) + (sum & 0xffff); /* add in accumulated carries */
	sum += sum >> 16;		/* add potential last carry */
	return (0xffff & ~sum);
}

void ip_input(char *buf, int len)
{
    ip_header *header = (ip_header*)buf;
    unsigned short sum = header->ip_sum;
    //printf("ip packet received\n");
    
    #if 1
    header->ip_sum = 0;
    if (ip_cksum(header, 20) != sum )
        return;
    #endif
    
    if ( header->ip_dst != swapl(ip_opt_address) )
        return;
    
    if (swaps(header->ip_len) > len )
        return;
    
    if ( header->ip_hlv != 0x45 )
        return;
    
    len = swaps(header->ip_len);
    
    if ( header->ip_p == 0x06 )
        tcp_input(buf, len);
    else if ( header->ip_p == 17 )
        udp_input(buf, len);
    
    //printf("finished handling ip packet\n");
}

// Sets up and sends one IP packet
int ip_output(unsigned int dest, unsigned char prot, char *data, 
    unsigned int length)
{
    if ( length > 1480 )
        return FALSE;
    
    //printf("ip: sending packet of length %d\n", length);
    
    unsigned short totallength = length + 4*5;
    
    ip_header *header = (ip_header*)(data + sizeof(linklayer_header));
    header->ip_hlv = 0;
    header->ip_hlv = (4 << 4);
    header->ip_hlv |= 5;
    header->ip_tos = 0x10;
    header->ip_len = (totallength>>8) | (totallength<<8);
    header->ip_id = 0;
    header->ip_off = 0;
    header->ip_ttl = 64;
    header->ip_p = prot;
    header->ip_sum = 0;
    header->ip_src = swapl(ip_opt_address);
    header->ip_dst = dest;
    
    header->ip_sum = ip_cksum(header, 20);
    
    // todo: get hardware address of destination
    char ptype[] = {0x08, 0x00};
    ethernet_send(data, length+sizeof(ip_header)+sizeof(linklayer_header), 
                  NULL, ptype);
    
    return 0;
}

