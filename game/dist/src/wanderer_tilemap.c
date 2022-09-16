#include "wanderer_tilemap.h"

map_item_pickup* tilemap_get_pickup( tilemap* tm, u64 index ){
    return &tm->pickups[index];
}

u64 tilemap_new_item_pickup( tilemap* tm ){
    u64 first_free = 0;
    {
        for( ;first_free < TILEMAP_MAX_ITEM_PICKUPS; 
              ++first_free ){
            if( !tm->pickups[first_free].contains_item ){
                tm->pickups[first_free].contains_item = true;
                tm->pickup_count++;
                return first_free;
            }
        }
    }
    return 0;
}

u64 tilemap_push_item_pickup( tilemap* tm, map_item_pickup pickup ){
    u64 first_free = 0;
    {
        for( ;
                first_free < TILEMAP_MAX_ITEM_PICKUPS; 
                ++first_free ){
            if( !tm->pickups[first_free].contains_item ){
                tm->pickups[first_free].contains_item = true;
                tm->pickup_count++;
                break;
            }
        }
    }

    map_item_pickup* new_pickup = &tm->pickups[ first_free ];
    (*new_pickup) = pickup;

    return tm->pickup_count++;
}

map_trigger* tilemap_get_trigger( tilemap* tm, u64 index ){
    return &tm->triggers[index];
}

u64 tilemap_new_trigger( tilemap* tm ){
    return tm->trigger_count++;
}

u64 tilemap_push_trigger( tilemap* tm, map_trigger trigger ){
    map_trigger* new_trigger = &tm->triggers[ tm->trigger_count ];
    (*new_trigger) = trigger;
    return tm->trigger_count++;
}

container* tilemap_get_container( tilemap* tm, u64 index ){
    return &tm->containers[index];
}

u64 tilemap_push_container( tilemap* tm, container container ){
    struct container* new_container = &tm->containers[ tm->container_count ];
    (*new_container) = container;
    return tm->container_count++;
}

u64 tilemap_new_container( tilemap* tm ){
    return tm->container_count++;
}
