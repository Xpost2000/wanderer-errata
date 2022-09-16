#ifndef WANDERER_ASSETS_H
#define WANDERER_ASSETS_H

#include "common.h"
#include "memory_pool.h"

#include "vec2.h"

typedef struct renderer renderer;

#define NEW_ACTOR_MODEL_NAME_SIZE 64
enum actor_model_scale{
    ACTOR_MODEL_SCALE_SMALL,

    ACTOR_MODEL_SCALE_MEDIUM,
    ACTOR_MODEL_SCALE_LARGE,
    ACTOR_MODEL_SCALE_GIANT,

    ACTOR_MODEL_SCALE_COUNT
};

static const char* actor_model_scale_strings[ACTOR_MODEL_SCALE_COUNT] =
{
    STRINGIFY(ACTOR_MODEL_SCALE_SMALL),
    STRINGIFY(ACTOR_MODEL_SCALE_MEDIUM),
    STRINGIFY(ACTOR_MODEL_SCALE_LARGE),
    STRINGIFY(ACTOR_MODEL_SCALE_GIANT)
};

enum actor_model_animation_group_names{
    ACTOR_ANIMATION_GROUP_IDLE,
    ACTOR_ANIMATION_GROUP_DEAD,
    ACTOR_ANIMATION_GROUP_IDLE_ATTACK_READY,
    ACTOR_ANIMATION_GROUP_ATTACK,
    ACTOR_ANIMATION_GROUP_SPELL_CAST,
    ACTOR_ANIMATION_GROUP_DYING,
    ACTOR_ANIMATION_GROUP_WALK,
    ACTOR_ANIMATION_GROUP_INTERACT,
    ACTOR_ANIMATION_GROUP_INJURED,

    ACTOR_ANIMATION_GROUP_COUNT
};

static const char* actor_model_animation_group_names_strings[ACTOR_ANIMATION_GROUP_COUNT] =
{
    STRINGIFY(ACTOR_ANIMATION_GROUP_IDLE),
    STRINGIFY(ACTOR_ANIMATION_GROUP_DEAD),
    STRINGIFY(ACTOR_ANIMATION_GROUP_IDLE_ATTACK_READY),
    STRINGIFY(ACTOR_ANIMATION_GROUP_ATTACK),
    STRINGIFY(ACTOR_ANIMATION_GROUP_SPELL_CAST),
    STRINGIFY(ACTOR_ANIMATION_GROUP_DYING),
    STRINGIFY(ACTOR_ANIMATION_GROUP_WALK),
    STRINGIFY(ACTOR_ANIMATION_GROUP_INTERACT),
    STRINGIFY(ACTOR_ANIMATION_GROUP_INJURED)
};

enum actor_model_animation_set_direction{
    ANIMATION_SET_DIRECTION_0, // face right ( default )
    ANIMATION_SET_DIRECTION_45,
    ANIMATION_SET_DIRECTION_90,
    ANIMATION_SET_DIRECTION_135,
    ANIMATION_SET_DIRECTION_180,
    ANIMATION_SET_DIRECTION_225,
    ANIMATION_SET_DIRECTION_270,
    ANIMATION_SET_DIRECTION_315,
    ANIMATION_SET_DIRECTION_360,

    // no direction.
    ANIMATION_SET_DIRECTION_OVERRIDE,
    ANIMATION_SET_DIRECTION_COUNT
};

static const char* actor_model_animation_set_direction_strings[ANIMATION_SET_DIRECTION_COUNT] =
{
    STRINGIFY(ANIMATION_SET_DIRECTION_0),
    STRINGIFY(ANIMATION_SET_DIRECTION_45),
    STRINGIFY(ANIMATION_SET_DIRECTION_90),
    STRINGIFY(ANIMATION_SET_DIRECTION_135),
    STRINGIFY(ANIMATION_SET_DIRECTION_180),
    STRINGIFY(ANIMATION_SET_DIRECTION_225),
    STRINGIFY(ANIMATION_SET_DIRECTION_270),
    STRINGIFY(ANIMATION_SET_DIRECTION_315),
    STRINGIFY(ANIMATION_SET_DIRECTION_360),
    STRINGIFY(ANIMATION_SET_DIRECTION_OVERRIDE)
};

enum actor_model_animation_set_flags{
    ANIMATION_SET_FLAG_LOOPABLE = BIT(0),
    ANIMATION_SET_FLAG_TEST_RESERVED = BIT(1),
    ANIMATION_SET_FLAG_COUNT = 2
};

static const char* actor_model_animation_set_flag_strings[ANIMATION_SET_FLAG_COUNT] = 
{
    STRINGIFY(ANIMATION_SET_FLAG_LOOPABLE),
    STRINGIFY(ANIMATION_SET_FLAG_TEST_RESERVED)
};

// general types
enum game_asset_type{
    GAME_ASSET_NONE,

    // Core Asset Types ( sounds, and stuff like that )
    GAME_ASSET_FONT,
    GAME_ASSET_BITMAP,

    // Sub Assets ( composed of other assets. Models, animations, cutscenes? )
    GAME_ASSET_SPRITESHEET,
    GAME_ASSET_ACTOR_MODEL,
    GAME_ASSET_TYPE_COUNT
};

static char* game_asset_type_strings[GAME_ASSET_TYPE_COUNT] = {
    STRINGIFY(GAME_ASSET_NONE),
    STRINGIFY(GAME_ASSET_FONT),
    STRINGIFY(GAME_ASSET_BITMAP),
    STRINGIFY(GAME_ASSET_SPRITESHEET),
    STRINGIFY(GAME_ASSET_ACTOR_MODEL)
};

// a weird fat handle?
typedef struct game_asset_handle{
    u32 id;
}game_asset_handle;

typedef struct game_asset_font{
    u32 renderer_id;
    f32 size;
}game_asset_font;

// should each frame have flags?
typedef struct actor_model_animation_frame{
    char name[260];

    union{
        game_asset_handle texture_id;
        u32 spritesheet_index;
    };

    u32 width;
    u32 height;

    f32 origin_x;
    f32 origin_y;

    f32 time_to_next_frame;
}actor_model_animation_frame;

typedef struct actor_model_animation_set{
    char name[NEW_ACTOR_MODEL_NAME_SIZE];
    u8 flags;

    game_asset_handle spritesheet_id;

    u8 direction;
    u8 animation_frame_count;
    actor_model_animation_frame* frames;
}actor_model_animation_set;

typedef struct actor_model_animation_group{
    u8 animation_set_count;
    actor_model_animation_set* animation_sets;
}actor_model_animation_group;

// returns a list of candidates.
// pick at discretion
u32 actor_model_animation_group_find_animation_set_for_direction( actor_model_animation_group* group, vec2 direction );
void actor_model_animation_group_find_animation_sets_for_direction( actor_model_animation_group* group, vec2 direction, size_t list_size, u32* list );

typedef struct actor_model{
    char name[NEW_ACTOR_MODEL_NAME_SIZE];
    u8 model_scale;
    f32 draw_scale;
    actor_model_animation_group animation_groups[ACTOR_ANIMATION_GROUP_COUNT];
}actor_model;
typedef actor_model game_asset_actor_model;

// decide if I really care about storing the
// pixels???
typedef struct game_asset_bitmap{
   u32 renderer_id;

    u32 width;
    u32 height;
}game_asset_bitmap;

typedef struct game_asset_spritesheet_rect{
    u32 x;
    u32 y;
    u32 w;
    u32 h;
}game_asset_spritesheet_rect;

#define MAX_SUPPORTED_SPRITESHEET_SUBRECTS 100
typedef struct game_asset_spritesheet{
    game_asset_handle matching_bitmap;
    u32 sprite_rect_count;
    game_asset_spritesheet_rect* sprite_rects;
}game_asset_spritesheet;

enum game_asset_load_spritesheet_settings_type{
    GAME_ASSET_SPRITESHEET_TYPE_TILED,
    GAME_ASSET_SPRITESHEET_TYPE_ATLAS,

    GAME_ASSET_SPRITESHEET_TYPE_COUNT
};

// for building a spritesheet in memory.
typedef struct game_asset_load_spritesheet_settings{
    u8 for_spritesheet_type;
    char* atlas_info_filename;
    union{
        struct{
            u32 rows;
            u32 columns;

            u32 tile_width;
            u32 tile_height;

            i16 padding_x;
            i16 padding_y;
        }tiled;
        struct{
            // dummy.
        }atlased;
    };
}game_asset_load_spritesheet_settings;

// I should probably give this thing
// a scratch buffer to store the file data
// for these things ( cpu representation ) in
// the case I actually need to do something about
// them...
typedef struct game_asset{
    u16 type;
    char name[255];
    union{
        game_asset_font font;
        game_asset_bitmap bitmap;
        game_asset_spritesheet spritesheet;
        game_asset_actor_model actor_model;
    };
}game_asset;

// Should probably define a bucket size that just
// expands or something...
// or just make it a bunch of linked lists? ( at least in traversal )
// technically all of these will still be linear.
// This will allow me to key multiple items with the same name as technically
// some of these items are loaded as multiple of the same type?
//
// "test.png" - can be loaded as BITMAP or SPRITESHEET......
#define MAX_GAME_ASSETS_IN_GAME 8000
typedef struct game_assets{
    memory_pool asset_memory_pool;
    renderer* renderer_context;
    game_asset assets[MAX_GAME_ASSETS_IN_GAME];
}game_assets;

void game_assets_init( game_assets* assets, renderer* renderer );
void game_assets_finish( game_assets* assets );

// separate into truetype and bitmap game font....
game_asset_handle game_asset_null( void );
game_asset_handle game_asset_load_font( game_assets* assets, const char* font_path, f32 size );
game_asset_handle game_asset_load_bitmap( game_assets* assets, const char* bitmap_path );
game_asset_handle game_asset_load_actor_model( game_assets* assets, const char* model_path );
game_asset_handle game_asset_make_spritesheet( game_assets* assets, const char* spritesheet_bitmap_path, game_asset_load_spritesheet_settings load_settings );
game_asset_handle game_asset_load_spritesheet( game_assets* assets, const char* spritesheet_info_path );
void game_asset_unload_asset_with_handle( game_assets* assets, game_asset_handle asset );
void game_asset_unload_asset_with_key( game_assets* assets, const char* key );

game_asset* game_asset_get_from_handle( game_assets* assets, game_asset_handle asset );
game_asset* game_asset_get_from_key( game_assets* assets, const char* key );

#endif
