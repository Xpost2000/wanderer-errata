#ifndef CONFIG_H
#define CONFIG_H

#include "common.h"
#include "memory_pool.h"

enum config_token_type{
    CONFIG_TOKEN_IDENTIFIER,
    CONFIG_TOKEN_STRING,
    CONFIG_TOKEN_NUMBER,
    CONFIG_TOKEN_LEFT_BRACKET,
    CONFIG_TOKEN_RIGHT_BRACKET,
    CONFIG_TOKEN_LEFT_SQUARE_BRACKET,
    CONFIG_TOKEN_RIGHT_SQUARE_BRACKET,
    CONFIG_TOKEN_COMMA,
    CONFIG_TOKEN_COLON,
    CONFIG_TOKEN_EOF,
    CONFIG_TOKEN_TYPES
};

static const char* config_token_type_strings[CONFIG_TOKEN_TYPES] = {
    STRINGIFY( CONFIG_TOKEN_IDENTIFIER ),
    STRINGIFY( CONFIG_TOKEN_STRING ),
    STRINGIFY( CONFIG_TOKEN_NUMBER ),
    STRINGIFY( CONFIG_TOKEN_LEFT_BRACKET ),
    STRINGIFY( CONFIG_TOKEN_RIGHT_BRACKET ),
    STRINGIFY( CONFIG_TOKEN_LEFT_SQUARE_BRACKET ),
    STRINGIFY( CONFIG_TOKEN_RIGHT_SQUARE_BRACKET ),
    STRINGIFY( CONFIG_TOKEN_COMMA ),
    STRINGIFY( CONFIG_TOKEN_COLON ),
    STRINGIFY( CONFIG_TOKEN_EOF )
};

typedef struct config_token{
    u8 type;
    char value[128];
}config_token;

b32 config_token_identifier_equal( config_token* token, char* val );

typedef struct config_token_parse_result{
    b32 valid;
    f32 number;
}config_token_parse_result;

config_token_parse_result config_token_parse_number( config_token* token );

typedef struct config_token_list{
    size_t capacity;
    size_t count;
    config_token* tokens;
}config_token_list;

config_token_list config_tokenize( char* source_text, size_t source_text_length );
void config_token_list_destroy( config_token_list* list );

enum config_variable_value_type{
    CONFIG_VARIABLE_STRING,
    CONFIG_VARIABLE_NUMBER,
    CONFIG_VARIABLE_SYMBOL,
    CONFIG_VARIABLE_LIST,

    CONFIG_VARIABLE_BLOCK,

    CONFIG_VARIABLE_TYPE_COUNT
};

static const char* config_variable_value_type_strings[CONFIG_VARIABLE_TYPE_COUNT] = {
    STRINGIFY( CONFIG_VARIABLE_STRING ),
    STRINGIFY( CONFIG_VARIBLE_NUMBER ),
    STRINGIFY( CONFIG_VARIABLE_SYMBOL ),
    STRINGIFY( CONFIG_VARIABLE_LIST ),
    STRINGIFY( CONFIG_VARIABLE_BLOCK )
};

typedef struct config_variable config_variable;
typedef struct config_variable_value config_variable_value;
typedef struct config_variable_list config_variable_list;
typedef struct config_variable_block config_variable_block;

typedef struct config_variable_list{
    u64 count;
    config_variable_value* items;
}config_variable_list;

typedef struct config_variable_block{
    u64 count;
    config_variable* items;
}config_variable_block;

config_variable* config_block_find_first_variable( config_variable_block* block, char* variable_name );

typedef struct config_variable_value{
    u8 type;
    union{
        char value[128];
        config_variable_list* as_list;
        config_variable_block* as_block;
    };
};

config_variable_value config_variable_value_new_block( void );
config_variable_value config_variable_value_new_list( void );
config_variable_value config_variable_value_new_string( const char* value );
config_variable_value config_variable_value_new_symbol( const char* value );
config_variable_value config_variable_value_new_number_from_i32( i32 value );
config_variable_value config_variable_value_new_number_from_f32( f32 value );

typedef struct config_variable{
    char name[128];
    config_variable_value value;
}config_variable;

config_variable config_variable_new_block( const char* name );
config_variable config_variable_new_list( const char* name );
config_variable config_variable_new_string( const char* name, const char* value );
config_variable config_variable_new_symbol( const char* name, const char* value );
config_variable config_variable_new_number_from_i32( const char* name, i32 value );
config_variable config_variable_new_number_from_f32( const char* name, f32 value );

typedef struct config_info{
    u64 count;
    config_variable* variables;
}config_info;

void config_variable_list_push_item( config_variable_list* list, config_variable_value item );
void config_info_push_variable( config_info* config, config_variable var );
void config_variable_block_push_item( config_variable_block* block, config_variable var );

void config_info_free_all_fields( config_info* config );
config_info config_info_parse_all_tokens( config_token_list* tokens );
config_variable* config_info_find_first_variable( config_info* config, char* variable_name );

i32 config_variable_value_try_integer( config_variable_value* var );
i32 config_variable_try_integer( config_variable* var );

f32 config_variable_value_try_float( config_variable_value* var );
f32 config_variable_try_float( config_variable* var );

const char* config_variable_value_try_string( config_variable_value* var );
const char* config_variable_try_string( config_variable* var );

u64 config_block_count_variables_of_type( config_variable_block* block, u8 type );
u64 config_info_count_variables_of_type( config_info* info, u8 type );

config_token_list config_tokenize_from_file( const char* file_path );
config_info config_parse_from_file( const char* file_path );

// figuring out better api to serialize.
void config_variable_block_dump_as_text_to_stderr( config_variable_block* variable_block );
void config_variable_list_dump_as_text_to_stderr( config_variable_list* variable_list );
void config_variable_value_dump_as_text_to_stderr( config_variable_value* variable_value );
void config_variable_dump_as_text_to_stderr( config_variable* variable );
void config_info_dump_as_text_to_stderr( config_info* config );

// TODO(jerry): Make a strings module so I don't have weird string handling,
// and have it be fast-ish ( I know the game does no string handling, but just for
// good practice )
// These all return dynamically allocated strings
// remember to free these.
char* config_variable_block_dump_as_text_as_str( config_variable_block* variable_block );
char* config_variable_list_dump_as_text_as_str( config_variable_list* variable_list );
char* config_variable_value_dump_as_text_as_str( config_variable_value* variable_value );
char* config_variable_dump_as_text_as_str( config_variable* variable );

// requires a memory pool.
char* config_info_dump_as_text_as_str( config_info* config );
void config_info_dump_as_text_into_file( config_info* config, const char* file_name );

#endif
