#include "platform.h"
#include "memory_pool.h"

void memory_pool_init( memory_pool* pool, u64 capacity ){
    if( pool ){
        pool->used = 0;
        pool->capacity = capacity;

        pool->memory = memory_allocate( capacity );
        memset( pool->memory, 0, capacity );
    }
}

void paged_memory_pool_init( paged_memory_pool* paged_pool, u64 page_size ){
    if( paged_pool ){
        paged_pool->used = 0;
        paged_pool->capacity = 0;

        paged_pool->page_size = page_size;
        paged_pool->pages = 0;

        paged_pool->memory = NULL;
    }
}

u8* paged_memory_pool_allocate( paged_memory_pool* paged_pool, u64 size ){
    if( paged_pool ){
        if( (paged_pool->used + size) >= paged_pool->capacity ){
#ifdef DEBUG_BUILD
            fprintf( stderr, "[%s] : Allocating new page of memory of size %lld bytes!\n", paged_pool->tag_name, paged_pool->page_size );
#endif

            // NOTE(jerry):..... How did it take me this long to see this? ((_size_??????) / (paged_pool->page_size))
            paged_pool->pages = ((paged_pool->used + size) / (paged_pool->page_size)) + 1;

            paged_pool->capacity = paged_pool->pages * paged_pool->page_size;

            u8* reallocated_buffer = memory_allocate( paged_pool->capacity );

            if( reallocated_buffer ){
                memset( reallocated_buffer, 0, paged_pool->capacity );

#ifdef DEBUG_BUILD
                fprintf( stderr, "[%s]: New capacity : %lld bytes\n", paged_pool->tag_name, paged_pool->capacity );
#endif

                if( paged_pool->memory ){
#ifdef DEBUG_BUILD
                    fprintf( stderr, "[%s] : Reallocated memory! Need to recopy(%lld bytes)\n", paged_pool->tag_name, paged_pool->used );
#endif
                    /*replace memcpy*/
                    memcpy( reallocated_buffer, paged_pool->memory, paged_pool->used );

                    memory_deallocate( paged_pool->memory );
                    paged_pool->memory = NULL;
                }

#ifdef DEBUG_BUILD
                fprintf( stderr, "[%s]: Reassigning pointers.\n", paged_pool->tag_name );
#endif
                paged_pool->memory = reallocated_buffer;
            }
        }

        u8* return_address = paged_pool->memory + paged_pool->used;
        paged_pool->used += size;

#ifdef DEBUG_BUILD
        fprintf( stderr, "[%s] : allocated new memory! (%lld bytes)\n", paged_pool->tag_name, size );
#endif

        return return_address;
    }

    return NULL;
}

static u64 alignment_padding( u64 base, u64 alignment ){
    u64 padding = (((base / alignment) + 1) * alignment) - base;

    return padding;
}

u8* memory_pool_unaligned_allocate( memory_pool* pool, u64 size ){
    if( pool ){
#if 0
        fprintf(stderr, "memory_pool unaligned_allocation!\n");
#endif
        if( pool->used >= pool->capacity ){
            return NULL;
        }else{
            u8* return_address = pool->memory + pool->used;
            pool->used += size;
            return return_address;
        }
    }

    return NULL;
}

u8* memory_pool_aligned_allocate( memory_pool* pool, u64 size ){
    if( pool ){
#if 0
        fprintf(stderr, "memory_pool aligned_allocation!\n");
#endif
        if( pool->used >= pool->capacity ){
            return NULL;
        }else{
            u8* return_address = pool->memory + pool->used;
            u64 pad = alignment_padding( (u64)return_address, 8 );

            pool->used += size;
            pool->used += pad;

            if (pool->used >= pool->capacity) {
                fprintf(stderr, "ran out of memory.\n");
            }

            u8* aligned_address = return_address + pad;
            memset(aligned_address, 0, size);

            return aligned_address;
        }
    }

    return NULL;
}

u8* memory_pool_allocate( memory_pool* pool, u64 size ){
#if 1
    return memory_pool_aligned_allocate(pool, size);
#else
    return memory_pool_unaligned_allocate(pool, size);
#endif
}

void paged_memory_pool_finish( paged_memory_pool* paged_pool ){
    if( paged_pool ){
        memory_deallocate( paged_pool->memory );
        paged_pool->memory = NULL;

        paged_pool->used = 0;
        paged_pool->capacity = 0;

        paged_pool->page_size = 0;
        paged_pool->pages = 0;
    }
}

void memory_pool_finish( memory_pool* pool ){
    if( pool ){
        memory_deallocate( pool->memory );

        pool->memory = NULL;

        pool->capacity = 0;
        pool->used = 0;
    }
}

void memory_sub_pool_init( memory_pool* pool, void* buffer, u64 capacity ){
    if( pool ){
        pool->used = 0;
        pool->capacity = capacity; 
        pool->memory = buffer;
    }
}
