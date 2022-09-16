#include "wanderer_floating_messages.h"

// floating messages stuff
void floating_messages_update( floating_messages* messages, float delta_time ){
    const f32 message_float_up_speed = 75.f; // in pixels

    for (unsigned msg_index = 0; msg_index < MAX_FLOATING_MESSAGES; ++msg_index) {
            floating_message* current_message = &messages->messages[msg_index]; 
            b32 message_alive = current_message->lifetime > 0.0f;

            if (message_alive) {
                current_message->height -= message_float_up_speed * delta_time;
                current_message->lifetime -= delta_time;
            }
    }
}

u32 floating_messages_get_first_free( floating_messages* msgs ){
    for (unsigned free_index = 0;
         free_index < MAX_FLOATING_MESSAGES;
         ++free_index) {
        if (roundf(msgs->messages[free_index].lifetime) == 0.0f) {
            return free_index;
        }
    }

    return 0;
}

void floating_messages_push_message(floating_messages* msgs,
                                    char* message,
                                    vec2 position,
                                    colorf color,
                                    f32 lifetime) {
    u32 first_free_index = floating_messages_get_first_free(msgs);
    floating_message* first_free = &msgs->messages[first_free_index];

    first_free->lifetime = lifetime;
    first_free->max_lifetime = lifetime;
    first_free->position = position;
    first_free->color = color;
    first_free->height = 0.0f;

    strncpy(first_free->message, message, FLOATING_MESSAGE_BUFFER_LENGTH);
}
