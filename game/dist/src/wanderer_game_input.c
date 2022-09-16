#include "wanderer_game_input.h"

// Game Action System
void game_action_bindings_set_current_context( game_action_bindings* bindings, u16 context ){
    bindings->current_context = context;
}

/*TODO(jerry): GAME_INPUT_MODE_ALL DOES NOT WORK*/
void game_action_bindings_add_binding( game_action_bindings* bindings,
        u16 input_context, u16 bound_action,
        u16 key_bound, u16 input_type ){
    if( input_context == GAME_INPUT_MODE_ALL ){
        // NOTE(jerry):
        // literally. Binding. To. All. Contexts...
        for(u64 binding_context = 0;
            binding_context < GAME_ACTION_BINDING_CONTEXT_COUNT;
            ++binding_context ){
            game_action_binding_context* selected_context = 
                &bindings->binding_contexts[binding_context];

            game_action_binding_array* binding_list = 
                &selected_context->bindings[bound_action];
            game_action_binding* selected_binding = 
                &binding_list->bindings[ binding_list->bindings_used++ ];

            selected_binding->bound_to = bound_action;
            selected_binding->key = key_bound;
            selected_binding->input_type = input_type;
        }
        fprintf(stderr, "[ALL CONTEXT]binding for action : %d\n", bound_action ); 
    }else{
        // needs assertions?
        game_action_binding_context* selected_context = 
            &bindings->binding_contexts[input_context];

        game_action_binding_array* binding_list = 
            &selected_context->bindings[bound_action];
        game_action_binding* selected_binding = 
            &binding_list->bindings[ binding_list->bindings_used++ ];

        selected_binding->bound_to = bound_action;
        selected_binding->key = key_bound;
        selected_binding->input_type = input_type;

        fprintf(stderr, "input system bound action : %d to key : %d to input type : %d : context : %d\n", 
                selected_binding->bound_to, selected_binding->key, selected_binding->input_type, input_context);
    }
}

b32 game_action_bindings_action_active( game_action_bindings* bindings, u16 action ){
    if( bindings->disable_input ){
        return false;
    }

    b32 result = false;

    for( u32 binding_context_index = 0;
            binding_context_index < GAME_ACTION_BINDING_CONTEXT_COUNT;
            ++binding_context_index ){
        if( (binding_context_index == bindings->current_context) ){
            game_action_binding_context* binding_context =
                &bindings->binding_contexts[binding_context_index];

            game_action_binding_array* bindings_list =
                &binding_context->bindings[action];

            for( u32 action_binding_index = 0;
                    action_binding_index < bindings_list->bindings_used;
                    ++action_binding_index ){
                game_action_binding* current_binding = 
                    &bindings_list->bindings[action_binding_index];

                switch( current_binding->input_type ){
                    case GAME_INPUT_KEY_DOWN:
                    {
                        b32 key_down = input_is_key_down( bindings->input, 
                                current_binding->key );

                        if( key_down ){
                            result = true;
                        }
                    }
                    break;
                    case GAME_INPUT_KEY_PRESSED:
                    {
                        b32 key_pressed = input_is_key_pressed( bindings->input,
                                current_binding->key );

                        if( key_pressed ){
                            result = true;
                        }
                    }
                    break;
                    default:
                    {
                    }
                    break;
                }
            }
        }
    }

    return result;
}

