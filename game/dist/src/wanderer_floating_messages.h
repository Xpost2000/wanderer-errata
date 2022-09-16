#ifndef WANDERER_FLOATING_MESSAGES_H
#define WANDERER_FLOATING_MESSAGES_H
#include "common.h"
#include "color.h"
#include "vec2.h"
/*
 * It's kind of like the Baldur's Gate thing?
 *
 * Or maybe it's just a crpg thing.
 */
#define MAX_FLOATING_MESSAGES 128
#define FLOATING_MESSAGE_BUFFER_LENGTH 255

// why did I SOA this thing????
typedef struct floating_message {
    f32 max_lifetime;
    f32 lifetime;
    f32 height;
    vec2 position;
    colorf color;
    char message[FLOATING_MESSAGE_BUFFER_LENGTH];
} floating_message;

typedef struct floating_messages{
    floating_message messages[MAX_FLOATING_MESSAGES];
}floating_messages;

void floating_messages_update(floating_messages* msgs, float delta_time);
u32 floating_messages_get_first_free(floating_messages* msgs);
void floating_messages_push_message(floating_messages* msgs, char* message, vec2 position, colorf color, f32 lifetime);

#endif
