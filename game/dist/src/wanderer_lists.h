#ifndef WANDERER_LISTS_H
#define WANDERER_LISTS_H

#include "common.h"

#include "wanderer_types_forward_declarations.h"

#include "memory_pool.h"

#include "wanderer_actor.h"
#include "wanderer_dialogue.h"
#include "wanderer_tilemap.h"

typedef struct actors_list{
    u64 count;

    paged_memory_pool actor_paged_pool;
}actors_list;

typedef struct tilemap_list{
    u64 count;
    u64 current;

    paged_memory_pool tilemap_paged_pool;
}tilemap_list;

u64 tilemap_list_push_tilemap( tilemap_list* list, tilemap map );
u64 tilemap_list_new_tilemap( tilemap_list* list );
tilemap* tilemap_list_get_tilemap( tilemap_list* list, u64 index );

u64 actors_list_push_actor( actors_list* list, actor actor );
actor* actors_list_get_actor( actors_list* list, u64 index );
u64 actors_list_first_dead( actors_list* list );

#endif
