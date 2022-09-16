#include "wanderer_magic.h"
#include "wanderer_actor.h"

void spell_dictionary_init(spell_dictionary* self, u64 max_capacity) {
    self->spells = memory_allocate(sizeof(spell_dictionary_key_value_pair) * self->count);
    self->count = 0;
}

void spell_dictionary_finish(spell_dictionary* self) {
    memory_deallocate(self->spells);
}

spell* spell_dictionary_find_spell(spell_dictionary* self, const char* id) {
    for (u64 spell_index = 0; spell_index < self->count; ++spell_index) {
        if (string_compare(id, self->spells[spell_index].key) == 0) {
            return &self->spells[spell_index].value;
        }
    }

    return NULL;
}

void spell_dictionary_add_spell(spell_dictionary* self, spell to_add, const char* lookup_name) {
    u64 current_free = self->count++;
    spell_dictionary_key_value_pair* free_spell = &self->spells[current_free];
    free_spell->value = to_add;
    free_spell->key = lookup_name;
}

void spell_set_name(spell* self, const char* name) {
    if (name) {
        self->name = name;
    } else {
        self->name = "DEFAULT SPELL NAME";
    }
}

void spell_set_description(spell* self, const char* description) {
    if (description) {
        self->description = description;
    } else {
        self->description = "DEFAULT SPELL DESCRIPTION TEXT";
    }
}

void spell_push_effect(spell* self, magic_effect effect) {
    self->effects[self->effect_count++] = effect;
}

spell_target make_spell_target_area(vec2 position, f32 radius) {
    spell_target result = {
        .type = SPELL_TARGET_AREA,
        .area.radius = radius,
        .area.position = position
    };

    return result;
}

spell_target make_spell_target_actor(actor* pointer) {
    spell_target result = {
        .type = SPELL_TARGET_ACTOR,
        .pointer = pointer
    };

    return result;
}

void actor_apply_effect(actor* self, game_state* state, magic_effect effect) {
    // handle resistances.
    switch (effect.effect_type) {
        case MAGIC_EFFECT_DAMAGE_HEALTH:
        {
            fprintf(stderr, "eft: MAGIC_EFFECT_DAMAGE_HEALTH\n");
            damage_info damage = {
                .enchantment_level = effect.damaging.enchantment_level,
                .type = effect.damaging.damage_type
            };

            i32 rolled_damage = random_integer_ranged(effect.damaging.damage.min, effect.damaging.damage.max) + effect.damaging.damage.bonus;

            damage.damage = rolled_damage;
            fprintf(stderr,
                    "hurting with %d (m : %d, mx : %d, bn : %d)\n",
                    rolled_damage,
                    effect.damaging.damage.min,
                    effect.damaging.damage.max,
                    effect.damaging.damage.bonus);

            actor_hurt(self, state, damage);
        }
        break;
        case MAGIC_EFFECT_RESTORE_HEALTH:
        {
            fprintf(stderr, "eft: MAGIC_EFFECT_RESTORE_HEALTH\n");

            i32 rolled_healing = random_integer_ranged(effect.roll_range.min, effect.roll_range.max) + effect.roll_range.bonus;
            fprintf(stderr,
                    "hurting with %d (m : %d, mx : %d, bn : %d)\n",
                    rolled_healing,
                    effect.roll_range.min,
                    effect.roll_range.max,
                    effect.roll_range.bonus);

            actor_heal(self, state, rolled_healing);
        }
        break;
        case MAGIC_EFFECT_RESTORE_ACTION_POINTS:
        {
            fprintf(stderr, "eft: MAGIC_EFFECT_RESTORE_ACTION_POINTS\n");
        }
        break;
        case MAGIC_EFFECT_DAMAGE_ACTION_POINTS:
        {
            fprintf(stderr, "eft: MAGIC_EFFECT_DAMAGE_ACTION_POINTS\n");
        }
        break;
        case MAGIC_EFFECT_KILL_TARGET:
        {
            fprintf(stderr, "eft: MAGIC_EFFECT_KILL_TARGET\n");
        }
        break;
        case MAGIC_EFFECT_VISUAL_FLASH:
        {
            fprintf(stderr, "eft: MAGIC_EFFECT_VISUAL_FLASH\n");
        }
        break;
    }
}

magic_effect test_heal_spell_effect(void) {
    magic_effect result = {
        .effect_type = MAGIC_EFFECT_RESTORE_HEALTH,
        .target_flags = MAGIC_EFFECT_TARGET_FLAGS_CASTER_AND_TARGET,
        .resist_flags = MAGIC_EFFECT_RESIST_FLAG_NONE,
        .delay_duration = 0,
        .duration = 0,
    };

    result.roll_range.min = 5;
    result.roll_range.max = 5;
    result.roll_range.bonus = 0;

    return result;
}

magic_effect test_flash_visual_effect(colorf rgba, i32 flashes, f32 time_between_flashes, f32 flash_linger_time) {
    magic_effect result = {};

    return result;
}

magic_effect test_delayed_poison_spell_effect(void) {
    magic_effect result = {
        .effect_type = MAGIC_EFFECT_DAMAGE_HEALTH,
        .target_flags = MAGIC_EFFECT_TARGET_FLAGS_CASTER,
        .resist_flags = MAGIC_EFFECT_RESIST_FLAG_NONE,
        .delay_duration = 3,
        .duration = 5,
    };
    result.damaging.damage.bonus = 2;
    result.damaging.damage.min = 6;
    result.damaging.damage.max = 12;
    result.damaging.damage_type = DAMAGE_TYPE_MAGIC;
    result.damaging.enchantment_level = NON_PHYSICAL_ENCHANTMENT_LEVEL;
    return result;
}

magic_effect test_poison_spell_effect(void) {
    magic_effect result = {
        .effect_type = MAGIC_EFFECT_DAMAGE_HEALTH,
        .target_flags = MAGIC_EFFECT_TARGET_FLAGS_CASTER,
        .resist_flags = MAGIC_EFFECT_RESIST_FLAG_NONE,
        .delay_duration = 0,
        .duration = 5,
    };
    result.damaging.damage.bonus = 2;
    result.damaging.damage.min = 6;
    result.damaging.damage.max = 12;
    result.damaging.damage_type = DAMAGE_TYPE_MAGIC;
    result.damaging.enchantment_level = NON_PHYSICAL_ENCHANTMENT_LEVEL;
    return result;
}
