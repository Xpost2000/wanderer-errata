#include "wanderer_lists.h"

// actor list
u64 actors_list_first_dead( actors_list* list ){
    u64 actor_index = 0;

    for(; actor_index < list->count; ++actor_index ){
        actor* current_actor = actors_list_get_actor( list, actor_index );
        if( current_actor->health_points <= 0 ){
            return actor_index;
        }
    }

    return 0;
}

u64 actors_list_push_actor( actors_list* list, actor actor ){
    if( list ){
        paged_memory_pool_allocate( &list->actor_paged_pool, sizeof(struct actor) );
        list->count = (list->actor_paged_pool.used) / (sizeof(struct actor));

        u64 first_free = actors_list_first_dead( list );
        struct actor* actors = (struct actor*)(list->actor_paged_pool.memory);
        actors[first_free] = actor;
        actors[first_free].ref_id = first_free;

        assert(actors != NULL);
        assert(list != NULL );

        return first_free;
    }else{
        return 0;
    }

    return 0;
}

actor* actors_list_get_actor( actors_list* list, u64 index ){
    if( list ){
        actor* actors = (actor*)(list->actor_paged_pool.memory);
        return &actors[index];
    }

    return NULL;
}

// tilemap list
u64 tilemap_list_push_tilemap( tilemap_list* list, tilemap map ){
    if( list ){
        paged_memory_pool_allocate( &list->tilemap_paged_pool, sizeof(struct tilemap) );
        list->count = (list->tilemap_paged_pool.used) / (sizeof(struct tilemap));
        struct tilemap* maps = (struct tilemap*)(list->tilemap_paged_pool.memory);

        maps[list->current] = map;

        assert(maps != NULL);
        assert(list != NULL );
        
        return list->current++;
    }else{
        return 0;
    }
}

u64 tilemap_list_new_tilemap( tilemap_list* list ){
    if( list ){
        paged_memory_pool_allocate( &list->tilemap_paged_pool, sizeof(struct tilemap) );
        list->count = (list->tilemap_paged_pool.used) / (sizeof(struct tilemap));

        return list->current++;
    }else{
        return 0;
    }
}

tilemap* tilemap_list_get_tilemap( tilemap_list* list, u64 index ){
    if( list ){
        tilemap* map = (list->tilemap_paged_pool.memory);
        return &map[index];
    }

    return NULL;
}
