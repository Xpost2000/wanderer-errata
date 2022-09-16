#ifndef WANDERER_DEBUG_CODE_H
#define WANDERER_DEBUG_CODE_H

/*
 * NOTE(jerry):
 * Debug related code?
 *
 * For now just contains the debug buffer.
 */

#include "common.h"

#define MAX_DEBUG_MESSAGES 8192

// does not store history and overrides itself
// not too useful right now but I'm not expecting
// enough messages to actually have this as a problem
typedef struct debug_message_buffer{
    u64 current;
    char* messages[MAX_DEBUG_MESSAGES];
}debug_message_buffer;

static void debug_message_buffer_clear( debug_message_buffer* msgs ){
    memset( msgs, 0, MAX_DEBUG_MESSAGES );
}

static void debug_message_buffer_push_message( debug_message_buffer* msgs,
                                               char* message_string ){
    msgs->messages[ msgs->current ] = message_string;

    msgs->current += 1;
    msgs->current %= MAX_DEBUG_MESSAGES;
}

#endif
