#include "ethernet.h"
#include "arp.h"
#include "ip.h"
#include "../lib.h"
#include "../kmem.h"

void arp_recv(char *buf, int len)
{
    ethernet_header *header = (ethernet_header*)buf;
    arp_packet *packet = (arp_packet*)(buf+sizeof(ethernet_header));
    
    // Verifications
    if ( swaps(packet->htype) != 1 )
        return;
    if ( swaps(packet->ptype) != 0x0800 )
        return;
    if ( (packet->hlen) != 6 )
        return;
    if ( (packet->plen) != 4 )
        return;
    if ( packet->tpa != swapl(ip_opt_address) )
        return;
    
    // We received a request
    if ( swaps(packet->oper) == 1 )
    {
        // Copy sender address fields into target address fields
        memcpy(packet->tha, packet->sha, 6);
        memcpy(&packet->tpa, &packet->spa, 4);
        
        // Operation type: Reply
        packet->oper = swaps(0x0002); 
        
        // Insert our addresses
        memcpy(packet->sha, ethernet_opt_address, 6);
        packet->spa = swapl(ip_opt_address);
        
        // Ethernet Protocol type: ARP
        char ptype[] = {0x08, 0x06};
        
        // Ethernet destination: Broadcast
        char bcast[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
        
        ethernet_send(buf, len, bcast, ptype);
    }
    // We received a reply
    else if ( swaps(packet->oper) == 2 )
    {
        struct arp_entry *e = arp_table.next;
        struct arp_entry *newentry;
        struct arp_queue_entry *qe, *qelast;
        
        // Let's see if we have a packet queued for this destination
        qe = arp_queue.next;
        qelast = &arp_queue;
        while( qe != NULL )
        {
            ip_header *iphead = (ip_header*)((qe->data)
                + sizeof(ethernet_header));
            if (
                iphead->ip_dst == packet->spa
                || ( 
                    packet->spa == swapl(ip_opt_gateway) 
                    && 
                    ( (int)( swapl(iphead->ip_dst) & ip_opt_netmask ) 
                        != (ip_opt_address & ip_opt_netmask) )
                ) 
            )
            {
                char ptype[] = {0x08, 0x00};
                ethernet_send(qe->data, qe->len, packet->sha, ptype);
                qelast->next = qe->next;
                kmem_free(qe, sizeof(struct arp_queue_entry));
                qe = qelast;
            }
            qelast = qe;
            qe = qe->next;
        }
        
        // Send the packet, free it and remove it from the list
        if ( qe != NULL )
        {
            char ptype[] = {0x08, 0x00};
            ethernet_send(qe->data, qe->len, packet->sha, ptype);
            qelast->next = qe->next;
            kmem_free(qe, sizeof(struct arp_queue_entry));
        }
        
        // Search cache for existing entry
        while ( e != NULL )
        {
            if ( e->ip == packet->spa )
                break;
            e = e->next;
        }
        
        // There is already an entry for that IP... update it and return
        if ( e != NULL )
        {
            memcpy(e->hw, packet->sha, 6);
            return;
        }
        
        // Otherwise, create a new entry
        newentry = (struct arp_entry *)kmem_alloc(sizeof(struct arp_entry));
        newentry->ip = packet->spa;
        memcpy(newentry->hw, packet->sha, 6);
        
        e = arp_table.next;
        arp_table.next = newentry;
        newentry->next = e;
        
        
    }
}

// Broadcast an ARP request for the given ip
void arp_send(int ip)
{
    char buf[sizeof(ethernet_header)+sizeof(arp_packet)];
    arp_packet *packet = (arp_packet*)(buf + sizeof(ethernet_header));
    char bcast[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    char zero[] = {0, 0, 0, 0, 0, 0};
    char ptype[] = {0x08, 0x06};
    
    packet->htype = swaps(1);
    packet->ptype = swaps(0x0800);
    packet->hlen = 6;
    packet->plen = 4;
    packet->oper = swaps(1);
    memcpy(packet->sha, ethernet_opt_address, 6);
    packet->spa = swapl(ip_opt_address);
    memcpy(packet->tha, zero, 6);
    memcpy(&(packet->tpa), &ip, 4);
    
    ethernet_send(buf, sizeof(ethernet_header)+sizeof(arp_packet), bcast, 
        ptype
    );
}

// Looks up an IP packet's IP in the ARP table and copies the corresponding
// MAC address into dst.
// If the IP is not found in the table, send a request and queue the packet for
// later sending.
// Returns true on cache hit, false otherwise.
int arp_resolve(char *data, unsigned int length, char *dst)
{
    ip_header *iphead = (ip_header*)(data+sizeof(ethernet_header));
    struct arp_entry *entry = arp_table.next;
    struct arp_queue_entry *qentry;
    struct arp_queue_entry *newqentry;
    
    // If the address is not on our subnet, send the packet to the gateway
    if ( (int)( swapl(iphead->ip_dst) & ip_opt_netmask ) 
        != (ip_opt_address & ip_opt_netmask) )
    {
        
        // Search cache for gateway MAC 
        while ( entry != NULL )
        {
            if ( entry->ip == swapl(ip_opt_gateway) )
                break;
            entry = entry->next;
        }
        
        // Send packet to gateway
        if ( entry != NULL )
        {
            memcpy(dst, entry->hw, 6);
            return TRUE;
        }
        
        // We didn't find an entry for the gateway in the ARP table...
        // Send a request for the gateway MAC
        arp_send(swapl(ip_opt_gateway));
        
        // Queue packet
        newqentry = kmem_alloc(sizeof(struct arp_queue_entry));
        memcpy(newqentry->data, data, length);
        newqentry->len = length;
        qentry = arp_queue.next;
        arp_queue.next = newqentry;
        newqentry->next = qentry;
        
        return FALSE;
    }
    
    // Search cache
    while ( entry != NULL )
    {
        if ( entry->ip == iphead->ip_dst )
            break;
        entry = entry->next;
    }
    // If found, copy into dst and return
    if ( entry != NULL )
    {
        memcpy(dst, entry->hw, 6);
        return TRUE;
    }
    
    // Send request
    arp_send(iphead->ip_dst);
    
    // Queue packet
    newqentry = kmem_alloc(sizeof(struct arp_queue_entry));
    memcpy(newqentry->data, data, length);
    newqentry->len = length;
    qentry = arp_queue.next;
    arp_queue.next = newqentry;
    newqentry->next = qentry;
    
    return FALSE;
}

