#ifndef __KMEM_H__
#define __KMEM_H__

#define BOOL int

#define KMEM_SIZE        0x01000000
#define KMEM_CHUNKSIZE   0x400

typedef struct kmem_freelist {
	unsigned long size;
	struct kmem_freelist *next, *last;
} kmem_freelist;

kmem_freelist kmem_head;

// Each chunk of memory will be aligned to 1kb blocks
/* EMAC: rbq alignment as per Errata #11 (64 bytes) is insufficient! */
char kmem_mem[KMEM_SIZE] __attribute__ ((aligned (0x400)));

void kmem_echo();
void *kmem_alloc( unsigned long size );
void kmem_free( void *mem, unsigned long size );
BOOL kmem_init();

#endif
