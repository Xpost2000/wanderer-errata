#ifndef WANDERER_DIALOGUE_H
#define WANDERER_DIALOGUE_H

#include "common.h"

#include "wanderer_game_event.h"

#include "wanderer_types_forward_declarations.h"

#define MAX_DIALOGUE_STRING_LENGTH 512

enum dialogue_termination_type{
    DIALOGUE_TERMINATOR_NONE,

    DIALOGUE_TERMINATOR_DEFAULT,
    DIALOGUE_TERMINATOR_MERCHANT,

    DIALOGUE_TERMINATOR_COUNTS
};

typedef struct dialogue_event {
    game_event event;
}dialogue_event;

typedef struct dialogue_node dialogue_node;
typedef struct dialogue_choice{
    dialogue_node* parent;

    char text[MAX_DIALOGUE_STRING_LENGTH];
    u32 jump_to;

    u32 event_count;
    dialogue_event* events;

    u8 terminator;
}dialogue_choice;

// this is the contents of the original scene node.
enum dialogue_node_type{
    DIALOGUE_NODE_DIALOGUE,
    DIALOGUE_NODE_SELECTOR,
    DIALOGUE_NODE_TYPE_COUNT
};

enum dialogue_scene_selection_checkers{
    DIALOGUE_CHECK_GAME_VARIABLE,
    DIALOGUE_CHECK_CONDITION_COUNT
};

enum comparison{
    INVALID_COMPARISON = 0,
    LESS_THAN = BIT(0),
    GREATER_THAN = BIT(1),
    EQUAL_TO = BIT(2),
    NOT_EQUAL_TO = BIT(3),

    COMPARISON_COUNT = 5
};

typedef struct dialogue_selection_condition{
    i16 condition;
    union{
        struct{
            char variable_name[MAX_DIALOGUE_STRING_LENGTH];
            i32 against;

            u16 comparison_type;
        }check_variable;
    };

    u32 success_jump;

    b32 has_fail;
    u32 fail_jump;
}dialogue_selection_condition;

typedef struct dialogue_node_selector{
    u64 condition_count;
    dialogue_selection_condition* conditions;
}dialogue_node_selector;

typedef struct dialogue_node_dialogue{
    actor* speaker;
    char text[MAX_DIALOGUE_STRING_LENGTH];

    u64 choice_count;
    dialogue_choice* choices;
}dialogue_node_dialogue;

typedef struct dialogue_node{
    u8 type;
    dialogue_scene* parent;
    union{
        dialogue_node_dialogue dialogue;
        dialogue_node_selector selector;
    };
}dialogue_node;

typedef struct dialogue_info{
    b32 initiated;
    u64 current_node;

    u64 node_count;
    dialogue_node* nodes;
}dialogue_info;

void dialogue_info_try_to_select_node(dialogue_info* dialogue, game_state* state, game_event_stack* event_stack);
void dialogue_info_follow_choice(dialogue_info* dialogue, u32 choice, game_state* state, game_event_stack* event_stack);

void dialogue_node_set_text(dialogue_node* node, char* text);
void dialogue_choice_set_text(dialogue_choice* choice, char* text);

dialogue_node* dialogue_info_push_selector_node(dialogue_info* dialogue);
dialogue_node* dialogue_info_push_node(dialogue_info* dialogue);

dialogue_choice* dialogue_node_push_goodbye_choice(dialogue_node* node);
dialogue_choice* dialogue_node_push_trade_menu_choice(dialogue_node* node);
dialogue_choice* dialogue_node_push_choice(dialogue_node* node);

dialogue_event* dialogue_choice_push_set_variable_event(dialogue_choice* choice, char* variable_name, i32 set_to, i32 max_use_times);
dialogue_event* dialogue_choice_push_critical_plot_point(dialogue_choice* choice);

dialogue_selection_condition* dialogue_selector_node_push_condition(dialogue_node* selector_node);

void dialogue_selection_condition_set_success_jump(dialogue_selection_condition* condition, u32 jump_to);
void dialogue_selection_condition_set_failure_jump(dialogue_selection_condition* condition, u32 jump_to);

void dialogue_selection_condition_set_check_variable(dialogue_selection_condition* condition, char* variable_name, i32 against, u16 compare_type);

#endif
