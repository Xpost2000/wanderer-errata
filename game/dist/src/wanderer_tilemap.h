#ifndef WANDERER_TILEMAP_H
#define WANDERER_TILEMAP_H

#include "common.h"
#include "collision.h"

#include "wanderer_cutscene.h"
#include "wanderer_actor.h"

#define TILEMAP_MAX_TRIGGERS 32

#define TILEMAP_MAX_ITEM_PICKUPS 256
#define TILEMAP_MAX_CONTAINERS 64

#define TILEMAP_WIDTH 40
#define TILEMAP_HEIGHT 40

typedef struct map_tile{
    i32 type;
    b32 solid;

    b32 wall_north;
    b32 wall_west;

    i32 x;
    i32 y;
}map_tile;

// technically an arbituary shape?
// for now will just be a square that
// does things when interacted with.
enum trigger_type{
    MAP_TRIGGER_EMPTY,

    MAP_TRIGGER_TEST_MESSAGE,
    MAP_TRIGGER_TRANSITION,

    MAP_TRIGGER_PLAY_CUTSCENE,

    MAP_TRIGGERS_MAX
};

enum trigger_shape{
    MAP_TRIGGER_SHAPE_UNKNOWN,
    MAP_TRIGGER_SHAPE_RECTANGLE,
    MAP_TRIGGER_SHAPE_CIRCLE,

    MAP_TRIGGER_SHAPES
};

typedef struct map_trigger{
    char* trigger_name;

    i64 used_times;
    i64 max_use_times;

    u32 trigger_type;
    u32 trigger_shape;

    union{
        circle circle;
        rectangle rect;
    }shape;

    union{
        struct trigger_message{
            char* message;
            f32 life_time;
        }msg;

        struct trigger_transition{
            f32 at_x;
            f32 at_y;

            u32 to_map_index;
        }transition;

        struct trigger_start_cutscene{
            cutscene_info* scene_ptr;
        }cutscene;
    }trigger_info;
}map_trigger;

// This should be phased out and become a container
// instead of a specific map item.
typedef struct map_item_pickup{
    b32 contains_item;

    f32 x;
    f32 y;

    f32 w;
    f32 h;

    item_slot item_to_pickup;
}map_item_pickup;

typedef struct tilemap{
    //NOTE(jerry):
    // Dynamically allocate this so it actually has a point.
    u32 width_min; 
    u32 height_min;
    u32 width;
    u32 height;

    u32 container_count;
    u32 trigger_count;
    u32 pickup_count;

    map_item_pickup pickups[TILEMAP_MAX_ITEM_PICKUPS];
    map_trigger triggers[TILEMAP_MAX_TRIGGERS];
    map_tile tiles[TILEMAP_HEIGHT][TILEMAP_WIDTH];
    container containers[TILEMAP_MAX_CONTAINERS];
}tilemap;

container* tilemap_get_container( tilemap* tm, u64 index );
u64 tilemap_push_container( tilemap* tm, container container );
u64 tilemap_new_container( tilemap* tm );

map_item_pickup* tilemap_get_pickup( tilemap* tm, u64 index );
u64 tilemap_push_item_pickup( tilemap* tm, map_item_pickup pickup );
u64 tilemap_new_item_pickup( tilemap* tm );

map_trigger* tilemap_get_trigger( tilemap* tm, u64 index );
u64 tilemap_push_trigger( tilemap* tm, map_trigger trigger );
u64 tilemap_new_trigger( tilemap* tm );

#endif
