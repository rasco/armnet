#include "kmem.h"
#include "lib.h"

void kmem_echo()
{
	kmem_freelist *p = &kmem_head;
	int n = 0;
	while(p!=NULL) {
		printf("%d\tp:%x, size: %x, next:%x, last:%x\n",n++, p, p->size, 
		    p->next, p->last);
		p=p->next;
	}
}

void *kmem_alloc(unsigned long size)
{
	kmem_freelist new;
	kmem_freelist *p = kmem_head.next;
	
	if ( p == NULL )
	{
	    printf("kmem_alloc: no free space\n");
	    while(1);
	}
    
	// Align the size to 1kb if necessary
	if ( size & 0x3FF )
	{
		size &= 0xFFFFFC00;
		size += 0x400;
	}
	// Look for a big enough element
	while(p->size < size)
	{
		if ( p->next == NULL )
		{
		    printf("kmem_alloc: no large enough free space\n");
		    while(1);
		}
		else p = p->next;
    }
	if ( p->size == size )
		p->last->next = p->next;
	else
	{
		new.size = p->size - size;
		new.next = p->next;
		new.last = p->last;
		p->last->next = (kmem_freelist*)((int)(p)+size);
		memcpy((void*)((int)(p)+size), &new, sizeof(kmem_freelist));
	}
	if ( p->next != NULL )
		p->next->last = p->last;
	//dprintf("allocated at %x size %d\n", p, size);
	return (void*)p;
}

void kmem_free( void *mem, unsigned long size )
{
    if ( size & 0x3FF )
	{
		size &= 0xFFFFFC00;
		size += 0x400;
	}    
    
	kmem_freelist new;
	kmem_freelist *p = kmem_head.next;
	
	while(p < (kmem_freelist*)(mem))
		if ( p == NULL ) break;
		else p = p->next;

	new.size = size;
	new.next = p;
	if (p==NULL) 
	{
		printf("p was null\n");
		new.last = &kmem_head;
		kmem_head.next=mem;
	}
	else
	{
		new.last = p->last;
		p->last->next = mem;
		p->last = mem;
	}
	memcpy(mem, &new, sizeof(new));
	
	p = kmem_head.next;
	while(p!=NULL)
	{
		if ( (kmem_freelist*)((int)(p)+p->size) == p->next )
		{
			p->size = p->size + p->next->size;
			p->next = p->next->next;
			if ( p->next != NULL )
				p->next->last = p;
			continue;
		}
		p=p->next;
	}
}

BOOL kmem_init()
{
	kmem_freelist *elem = (kmem_freelist*)kmem_mem;
	
	elem->size = KMEM_SIZE;
	elem->next = NULL;
	elem->last = &kmem_head;
	
	kmem_head.size = 0;
	kmem_head.next = elem;
	kmem_head.last = NULL;
	kmem_freelist testelem;
	
	return TRUE;
}

