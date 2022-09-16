#include "wanderer.h"

#include "wanderer_dialogue.h"

void dialogue_node_set_text(dialogue_node* node, char* text){
    strncpy(node->dialogue.text, text, MAX_DIALOGUE_STRING_LENGTH);
}

void dialogue_choice_set_text(dialogue_choice* choice, char* text){
    strncpy(choice->text, text, MAX_DIALOGUE_STRING_LENGTH);
}

dialogue_node* dialogue_info_push_selector_node(dialogue_info* dialogue){
    dialogue_node* selector_node = &dialogue->nodes[dialogue->node_count];
    dialogue->node_count++;

    selector_node->parent = dialogue;
    selector_node->type = DIALOGUE_NODE_SELECTOR;
    selector_node->selector.condition_count = 0;

    return selector_node;
}

dialogue_node* dialogue_info_push_node(dialogue_info* dialogue){
    dialogue_node* node = &dialogue->nodes[dialogue->node_count];
    dialogue->node_count++;

    node->parent = dialogue;
    node->type = DIALOGUE_NODE_DIALOGUE;

    node->dialogue.speaker = NULL;
    node->dialogue.choice_count = 0;

    return node;
}

dialogue_choice* dialogue_node_push_goodbye_choice(dialogue_node* node){
    if( node->type == DIALOGUE_NODE_DIALOGUE ){
        dialogue_node_dialogue* dialogue_node = &node->dialogue;
        dialogue_choice* goodbye_choice = &dialogue_node->choices[dialogue_node->choice_count];
        dialogue_node->choice_count++;
        goodbye_choice->parent = node;

        goodbye_choice->terminator = DIALOGUE_TERMINATOR_DEFAULT;
        goodbye_choice->event_count = 0;
        goodbye_choice->jump_to = 0;

        return goodbye_choice;
    }else{
        return NULL;
    }
}

dialogue_choice* dialogue_node_push_trade_menu_choice(dialogue_node* node){
    if( node->type == DIALOGUE_NODE_DIALOGUE ){
        dialogue_node_dialogue* dialogue_node = &node->dialogue;
        dialogue_choice* trade_menu_choice = &dialogue_node->choices[dialogue_node->choice_count];
        dialogue_node->choice_count++;
        trade_menu_choice->parent = node;

        trade_menu_choice->terminator = DIALOGUE_TERMINATOR_MERCHANT;
        trade_menu_choice->event_count = 0;
        trade_menu_choice->jump_to = 0;

        return trade_menu_choice;
    }else{
        return NULL;
    }
}

dialogue_choice* dialogue_node_push_choice(dialogue_node* node){
    if( node->type == DIALOGUE_NODE_DIALOGUE ){
        dialogue_node_dialogue* dialogue_node = &node->dialogue;
        dialogue_choice* choice = &dialogue_node->choices[dialogue_node->choice_count];
        dialogue_node->choice_count++;
        choice->parent = node;

        choice->terminator = DIALOGUE_TERMINATOR_NONE;
        choice->event_count = 0;
        choice->jump_to = 0;

        return choice;
    }else{
        return NULL;
    }
}

dialogue_event* dialogue_choice_push_set_variable_event(dialogue_choice* choice,
                                                        char* variable_name,
                                                        i32 set_to,
                                                        i32 max_use_times){
    dialogue_event* event = &choice->events[choice->event_count];

    choice->event_count++;

    event->event.event_type = GAME_EVENT_SET_VARIABLE;
    event->event.used_times = 0;
    event->event.max_use_times = max_use_times;
    event->event.set_variable.variable_name = variable_name;
    event->event.set_variable.new_value = set_to;

    return event;
}

dialogue_event* dialogue_choice_push_critical_plot_point(dialogue_choice* choice){
    dialogue_event* event = &choice->events[choice->event_count];

    choice->event_count++;

    event->event.event_type = GAME_EVENT_CRITICAL_PLOT_POINT_REACHED;
    event->event.used_times = 0;
    event->event.max_use_times = 1;

    return event;
}

void dialogue_selection_condition_set_check_variable(dialogue_selection_condition* condition,
                                                     char* variable_name,
                                                     i32 against,
                                                     u16 compare_type){
    strncpy(condition->check_variable.variable_name, variable_name, MAX_DIALOGUE_STRING_LENGTH);
    condition->condition = DIALOGUE_CHECK_GAME_VARIABLE;
    condition->check_variable.against = against;
    condition->check_variable.comparison_type = compare_type;
}

dialogue_selection_condition* dialogue_selector_node_push_condition(dialogue_node* selector_node){
    dialogue_node_selector* selector = &selector_node->selector;
    dialogue_selection_condition* condition = &selector->conditions[selector->condition_count];
    selector->condition_count++;

    condition->condition = 0;
    condition->success_jump = 0;
    condition->fail_jump = 0;
    condition->has_fail = false;

    return condition;
}

void dialogue_selection_condition_set_success_jump(dialogue_selection_condition* condition, u32 jump_to){
    condition->success_jump = jump_to;
}

void dialogue_selection_condition_set_failure_jump(dialogue_selection_condition* condition, u32 jump_to){
    condition->has_fail = true;
    condition->fail_jump = jump_to;
}

static bool dialogue_selection_condition_evaluate(dialogue_selection_condition* condition,
                                                  game_state* state,
                                                  game_event_stack* event_stack){
    bool result = false;

    switch( condition->condition ){
        case DIALOGUE_CHECK_GAME_VARIABLE:
        {
            char* variable_name = condition->check_variable.variable_name;
            game_variable* variable_to_check = game_variable_dictionary_find(&state->variables, variable_name);
            fprintf(stderr, "var name : %s\n", variable_name);

            if( variable_to_check && variable_name ){
                fprintf(stderr, "var val : %d vs %d\n", variable_to_check->value, condition->check_variable.against);
                if( condition->check_variable.comparison_type & LESS_THAN ){
                    result |= (variable_to_check->value < condition->check_variable.against);
                }

                if( condition->check_variable.comparison_type & GREATER_THAN ){
                    result |= (variable_to_check->value > condition->check_variable.against);
                }

                if( condition->check_variable.comparison_type & EQUAL_TO ){
                    result |= (variable_to_check->value == condition->check_variable.against);
                }

                if( condition->check_variable.comparison_type & NOT_EQUAL_TO ){
                    result |= (variable_to_check->value != condition->check_variable.against);
                }
            }
        }
        break;
    }

    return result;
}

void dialogue_info_try_to_select_node(dialogue_info* dialogue,
                                      game_state* state,
                                      game_event_stack* event_stack){
    dialogue_node* current_node = &dialogue->nodes[dialogue->current_node];
    u32 new_current_node = dialogue->current_node;

    if( current_node->type == DIALOGUE_NODE_SELECTOR ){
        for( unsigned conditional = 0;
             conditional < current_node->selector.condition_count;
             ++conditional ){
            dialogue_selection_condition* condition =
                &current_node->selector.conditions[conditional];

            if(dialogue_selection_condition_evaluate(condition, state, event_stack)){
                new_current_node = condition->success_jump;
                break;
            }else{
                if(condition->has_fail){
                    new_current_node = condition->fail_jump;
                    break;
                }else{
                    continue;
                }
            }
        }

        dialogue->current_node = new_current_node;
        dialogue_node* new_node = &dialogue->nodes[dialogue->current_node];

        if( new_node->type == DIALOGUE_NODE_SELECTOR ){
            dialogue_info_try_to_select_node(dialogue, state, event_stack);
        }
    }else{
        return;
    }
}

static void dialogue_choice_submit_events_to_stack(dialogue_choice* choice,
                                                   game_event_stack* stack){
    for( unsigned event_index = 0;
         event_index < choice->event_count;
         ++event_index ){
        dialogue_event* event = &choice->events[event_index];
        game_event_stack_push(stack, &event->event);
    }
}

void dialogue_info_follow_choice(dialogue_info* dialogue,
                                 u32 choice,
                                 game_state* state,
                                 game_event_stack* event_stack){
    dialogue_node* current_node = &dialogue->nodes[dialogue->current_node];
    assert(current_node->type != DIALOGUE_NODE_SELECTOR);

    dialogue_choice* choice_to_follow = &current_node->dialogue.choices[choice];
    stub_less_important("Submit game events to stack missing...");
    dialogue_choice_submit_events_to_stack(choice_to_follow, event_stack);
    // submit events to stack missing.

    switch( choice_to_follow->terminator ){
        case DIALOGUE_TERMINATOR_NONE:
        {
            dialogue->current_node = choice_to_follow->jump_to;
            fprintf(stderr, "jumping to : %d\n", choice_to_follow->jump_to);
        }
        break;
        case DIALOGUE_TERMINATOR_DEFAULT:
        {
            dialogue->initiated = false;
            // probably shouldn't be doing this.
            state->active_dialogue = NULL;
            state->ui_info.mode = GAME_UI_STATE_INGAME;
        }
        break;
        case DIALOGUE_TERMINATOR_MERCHANT:
        {
            stub_less_important("open game ui trading menu");
            // open game ui trading menu
        }
        break;
        default:
        {
        }
        break;
    }
}
