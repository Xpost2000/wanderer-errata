#ifndef WANDERER_ITEM_H
#define WANDERER_ITEM_H

#include "common.h"

enum item_type{
    ITEM_TYPE_NONE,

    ITEM_TYPE_MISC,
    ITEM_TYPE_KEY,
    ITEM_TYPE_WEAPON,
    ITEM_TYPE_MISSILE_AMMUNITION,
    ITEM_TYPE_CURRENCY,

    ITEM_TYPE_COUNT
};

static char* item_type_strings[] = {
    STRINGIFY(ITEM_TYPE_NONE),
    STRINGIFY(ITEM_TYPE_MISC),
    STRINGIFY(ITEM_TYPE_KEY),
    STRINGIFY(ITEM_TYPE_WEAPON),
    STRINGIFY(ITEM_TYPE_MISSILE_AMMUNITION),
    STRINGIFY(ITEM_TYPE_CURRENCY)
};

static char* item_type_names[] = {
    "None",
    "Misc",
    "Key",
    "Weapon",
    "Ammo",
    "Currency",

    STRINGIFY(ITEM_TYPE_COUNT)
};

// TODO(jerry): remodel damage to account for enchanted items.
typedef struct rolled_range{
    i32 min;
    i32 max;
    i32 bonus;
} rolled_range;

typedef struct rolled_range healing_range;
typedef struct rolled_range damage_range;

enum damage_types{
    /*
      used for attacks that should just "hurt", but
      aren't really magical in nature or what not. It just hurts...
     */
    DAMAGE_TYPE_IMPOSSIBLE,
    // actual damage types
    DAMAGE_TYPE_PHYSICAL,
    DAMAGE_TYPE_PHYSICAL_SLASHING,
    DAMAGE_TYPE_PHYSICAL_THRUSTING,
    DAMAGE_TYPE_PHYSICAL_BLUNT,

    DAMAGE_TYPE_MAGIC,
    DAMAGE_TYPE_FIRE,
    DAMAGE_TYPE_ICE,
    DAMAGE_TYPE_SHOCK,

    DAMAGE_TYPES
};

static char* damage_type_strings[] = {
    STRINGIFY(DAMAGE_TYPE_IMPOSSIBLE),
    STRINGIFY(DAMAGE_TYPE_PHYSICAL),
    STRINGIFY(DAMAGE_TYPE_PHYSICAL_SLASHING),
    STRINGIFY(DAMAGE_TYPE_PHYSICAL_THRUSTING),
    STRINGIFY(DAMAGE_TYPE_PHYSICAL_BLUNT)
    STRINGIFY(DAMAGE_TYPE_MAGIC),
    STRINGIFY(DAMAGE_TYPE_FIRE),
    STRINGIFY(DAMAGE_TYPE_ICE),
    STRINGIFY(DAMAGE_TYPE_SHOCK)
};

static char* damage_type_names[] = {
    "Unblockable",
    "Physical",
    "Physical (Slashing)",
    "Physical (Thrusting)",
    "Physical (Blunt)",
    "Magical",
    "Fire",
    "Ice",
    "Shock"
};

#define NON_PHYSICAL_ENCHANTMENT_LEVEL 250
typedef struct damage_info {
    u8 enchantment_level; // only checked on physical damage technically.
    u8 type;
    i32 damage;
} damage_info;

damage_info pure_physical_damage(i32 damage, u8 enchantment_level);
damage_info slash_damage(i32 damage, u8 enchantment_level);
damage_info thrust_damage(i32 damage, u8 enchantment_level);
damage_info blunt_damage(i32 damage, u8 enchantment_level);
damage_info magic_damage(i32 damage);

enum item_class_id{
    ITEM_CLASS_MISC,

    ITEM_CLASS_KEY,

    ITEM_CLASS_SWORD,
    ITEM_CLASS_MISSILE_WEAPON,

    ITEM_CLASS_SHIELD,
    ITEM_CLASS_ARROW,
    ITEM_CLASS_CROSSBOW_BOLT,
    ITEM_CLASS_SLING_BALL,

    ITEM_CLASS_CURRENCY,

    ITEM_CLASS_TYPES
};

static char* item_class_id_strings[] = {
    STRINGIFY(ITEM_CLASS_MISC),
    STRINGIFY(ITEM_CLASS_KEY),
    STRINGIFY(ITEM_CLASS_SWORD),
    STRINGIFY(ITEM_CLASS_MISSILE_WEAPON),
    STRINGIFY(ITEM_CLASS_SHIELD),
    STRINGIFY(ITEM_CLASS_ARROW),
    STRINGIFY(ITEM_CLASS_CROSSBOW_BOLT),
    STRINGIFY(ITEM_CLASS_SLING_BALL),
    STRINGIFY(ITEM_CLASS_CURRENCY)
};

static char* item_class_names[] = {
    "Misc.",
    "Sword",
    "Missile Weapon",
    "Shield",
    "Arrow",
    "Crossbow Bolt",
    "Sling Ball",
    "Currency",

    STRINGIFY(ITEM_CLASS_TYPES)
};

typedef struct item_weapon{
    /*only applies to physical.*/
    i8 enchant_level;

    b32 takes_ammunition;
    // will look in the quick ammo slots for suitable ammo
    // matching the class.
    u32 ammunition_class_id;

    // I don't appear to use this yet so I will undefine it.
#if 0
    damage_info damage;
#endif
}item_weapon;

#define ITEM_NAME_STR_LENGTH 64
#define ITEM_DESCRIPTION_STR_LENGTH 255

typedef struct item{
    // TODO(jerry): Make an item database to reduce memory usage.
    char name[ITEM_NAME_STR_LENGTH];
    char description[ITEM_DESCRIPTION_STR_LENGTH];

    // should have instance id?
    u8 type;
	u32 id;
    /*general type of item.*/
    u32 class_id;

    u16 stack_max; 
    u16 weight;

    u32 value;

    union{
        item_weapon as_weapon;
    };

    u32 atlas_icon;
}item;

void item_set_name(item* self, const char* name);
void item_set_description(item* self, const char* description);

typedef struct item_slot{
    // this is potentially very dangerous, to do.
    // but I don't think I'll add items at runtime... So we're
    // pretty safe. Until that changes.
    item* item_ptr; 
    i32 item_count;
    u32 owner_index;
}item_slot;

typedef struct item_dictionary{
    u64 max_capacity;
    u64 item_count;
    item* items;
} item_dictionary;

void item_dictionary_initialize(item_dictionary* self, u64 capacity);
void item_dictionary_finish(item_dictionary* self);

void item_dictionary_add_item( item_dictionary* dictionary, item to_add );
item_slot item_dictionary_make_instance_of( item_dictionary* dictionary, char* name );

#if 0
// do not actually make anything of struct inventory
// at least within the game code. This is just to cast stuff
// into basically. unsafe C polymorphic whatever thing.
enum inventory_flags {
    INVENTORY_FLAGS_NONE,
    INVENTORY_FLAGS_CANNOT_LOOT = 0x1,
    INVENTORY_FLAGS_BELONGS_TO_ACTOR = 0x2,
    INVENTORY_FLAGS_BELONGS_TO_CONTAINER = 0x4,
    INVENTORY_FLAGS_COUNT = 4
};

/*
 * Weird reason to use variadic arguments...
 * I might rethink using it this way...
 */
typedef struct inventory_base{
    u8 flags;
    u32 count;
    item_slot items[];
}inventory_base;

b32 inventory_add_item(inventory_base* self, item_slot item);
#endif

b32 item_slot_equal(item_slot* a, item_slot* b);
b32 item_slot_is_empty(item_slot* slot);
i32 item_slot_can_fit_more(item_slot* slot);

b32 item_slot_add_to(item_slot* slot, item_slot* other);

u8 map_string_to_item_class_id(const char* string);
u8 map_string_to_item_type(const char* string);

#endif
