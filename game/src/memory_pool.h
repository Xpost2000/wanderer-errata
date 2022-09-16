#ifndef MEMORY_POOL_H
#define MEMORY_POOL_H

#include "common.h"

typedef struct memory_pool memory_pool;
typedef struct paged_memory_pool paged_memory_pool;

typedef struct memory_pool{
    char* tag_name;

    u64 used;
    u64 capacity;

    u8* memory;
}memory_pool;

typedef struct paged_memory_pool{
    char* tag_name;

    u64 used;

    u64 capacity;
    u64 page_size;

    u64 pages;

    u8* memory;
}paged_memory_pool;

void memory_pool_init( memory_pool* pool, u64 capacity );
void paged_memory_pool_init( paged_memory_pool* paged_pool, u64 page_size );

u8* paged_memory_pool_allocate( paged_memory_pool* paged_pool, u64 size );
u8* memory_pool_allocate( memory_pool* pool, u64 size );
u8* memory_pool_aligned_allocate( memory_pool* pool, u64 size );
u8* memory_pool_unaligned_allocate( memory_pool* pool, u64 size );

void paged_memory_pool_finish( paged_memory_pool* paged_pool );
void memory_pool_finish( memory_pool* pool );

void memory_sub_pool_init( memory_pool* pool, void* buffer, u64 capacity );

#endif
