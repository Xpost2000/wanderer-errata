#include "config.h"
#include "platform.h"
#include "string.h"

static config_variable_list* config_parse_list( config_token_list* tokens, u64* token_index );
static config_variable_block* config_parse_block( config_token_list* tokens, u64* token_index );
static void config_variable_list_free( config_variable_list* list );
static void config_variable_block_free( config_variable_block* block );
static void config_variable_value_free( config_variable_value* variable_value );
static void config_variable_free( config_variable* variable );

u64 config_block_count_variables_of_type( config_variable_block* block, u8 type ){
    u64 result = 0;

    for( u64 block_item = 0; block_item < block->count; ++block_item ){
        config_variable* current_item = &block->items[block_item];
        if( current_item->value.type == type ){
            result++;
        }
    }

    return result;
}

u64 config_info_count_variables_of_type( config_info* info, u8 type ){
    u64 result = 0;

    for( u64 item = 0; item < info->count; ++item ){
        config_variable* current_item = &info->variables[item];
        if( current_item->value.type == type ){
            result++;
        }
    }

    return result;
}

static config_variable_list* config_allocate_variable_list( void ){
    config_variable_list* result = memory_allocate( sizeof(config_variable_list) );

    result->items = NULL;
    result->count = 0;

    return result;
}

static config_variable_block* config_allocate_variable_block( void ){
    config_variable_block* result = memory_allocate( sizeof(config_variable_block) );

    result->items = NULL;
    result->count = 0;

    return result;
}

config_variable_value config_variable_value_new_block( void ){
    config_variable_value result = (config_variable_value) {
        .type = CONFIG_VARIABLE_BLOCK,
        .as_block = config_allocate_variable_block()
    };

    return result;
}

config_variable_value config_variable_value_new_list( void ){
    config_variable_value result = (config_variable_value) {
        .type = CONFIG_VARIABLE_LIST,
        .as_list = config_allocate_variable_list()
    };

    return result;
}

config_variable_value config_variable_value_new_string( const char* value ){
    config_variable_value result = (config_variable_value){
        .type = CONFIG_VARIABLE_STRING
    };

    strncpy( result.value, value, 128 );

    return result;
}

config_variable_value config_variable_value_new_symbol( const char* value ){
    config_variable_value result = (config_variable_value){
        .type = CONFIG_VARIABLE_SYMBOL
    };

    strncpy( result.value, value, 128 );

    return result;
}

config_variable_value config_variable_value_new_number_from_i32( i32 value ){
    config_variable_value result = (config_variable_value){
        .type = CONFIG_VARIABLE_NUMBER
    };

    snprintf(result.value, 128, "%d", value);

    return result;
}

config_variable_value config_variable_value_new_number_from_f32( f32 value ){
    config_variable_value result = (config_variable_value){
        .type = CONFIG_VARIABLE_NUMBER
    };

    snprintf(result.value, 128, "%3.3f", value);

    return result;
}

// separator

config_variable config_variable_new_block( const char* name ){
    config_variable result = (config_variable){
        .value = config_variable_value_new_block()
    };

    strncpy( result.name, name, 128 );

    return result;
}

config_variable config_variable_new_list( const char* name ){
    config_variable result = (config_variable){
        .value = config_variable_value_new_list()
    };

    strncpy( result.name, name, 128 );

    return result;
}

config_variable config_variable_new_string( const char* name, const char* value ){
    config_variable result = (config_variable){
        .value = config_variable_value_new_string(value)
    };

    strncpy( result.name, name, 128 );

    return result;
}

config_variable config_variable_new_symbol( const char* name, const char* value ){
    config_variable result = (config_variable){
        .value = config_variable_value_new_symbol(value)
    };

    strncpy( result.name, name, 128 );

    return result;
}

config_variable config_variable_new_number_from_i32( const char* name, i32 value ){
    config_variable result = (config_variable){
        .value = config_variable_value_new_number_from_i32(value)
    };

    strncpy( result.name, name, 128 );

    return result;
}

config_variable config_variable_new_number_from_f32( const char* name, f32 value ){
    config_variable result = (config_variable){
        .value = config_variable_value_new_number_from_f32(value)
    };

    strncpy( result.name, name, 128 );

    return result;
}

config_token_list config_tokenize_from_file( const char* file_path ){
    u8* file_buffer = load_file_into_buffer( file_path );
    u64 file_buffer_length = get_file_size( file_path );
    config_token_list tokens = config_tokenize( file_buffer, file_buffer_length );

#if 0
    for( u64 token_index = 0; token_index < tokens.count; ++token_index ){
        config_token* current_token = &tokens.tokens[token_index];
        fprintf(stderr, "tok : %s : %s\n",
                config_token_type_strings[current_token->type],
                current_token->value);
    }
#endif

    memory_deallocate( file_buffer );
    return tokens;
}

config_info config_parse_from_file( const char* file_path ){
    config_token_list tokens = config_tokenize_from_file( file_path );
    config_info result = config_info_parse_all_tokens( &tokens );
    config_token_list_destroy( &tokens );

    return result;
}

static b32 is_whitespace( char ch ){
    if( ch == '\n' ||
        ch == '\t' ||
        ch == '\r' ||
        ch == ' ' ){
        return true;
    }
    return false;
}

static b32 is_uppercase( char ch ){
    return ( ch >= 'A' && ch <= 'Z' );
}

static b32 is_lowercase( char ch ){
    return ( ch >= 'a' && ch <= 'z' );
}

static b32 is_alphabetic( char ch ){
    return is_uppercase(ch) ||
           is_lowercase(ch);
}

static b32 is_numeric( char ch ){
    return ( ch >= '0' && ch <= '9' );
}

static b32 is_valid_identifier_character( char ch ){
    static char* forbidden_characters = "\"\'{}[],:\t\r\n ";

    for( char* cursor = forbidden_characters;
         *cursor;
         cursor++ ){
        if( ch == *cursor ){
            return false;
        }
    }

    return true;
}

void config_token_list_destroy( config_token_list* list ){
    memory_deallocate( list->tokens );
}

static void config_token_list_push( config_token_list* list, config_token value ){
    if( list->count < list->capacity ){
        list->tokens[list->count++] = value;
    }else{
        config_token* new_tokens_list = memory_allocate( sizeof(config_token) * (list->capacity*2) );
        memcpy(new_tokens_list, list->tokens, sizeof(config_token) * (list->count));
        list->capacity *= 2;
        
        memory_deallocate( list->tokens );
        list->tokens = new_tokens_list;
    }
}

static void config_token_list_preallocate( config_token_list* list, size_t starting_capacity ){
    list->capacity = starting_capacity;
    list->count = 0;

    list->tokens = memory_allocate( sizeof(config_token) * list->capacity );
}

b32 config_token_identifier_equal( config_token* token, char* val ){
    if( token->type == CONFIG_TOKEN_IDENTIFIER ){
        i32 comparison = strcmp( token->value, val );

        return (comparison == 0);
    }else{
        return false;
    }
}

config_token_parse_result config_token_parse_number( config_token* token ){
    config_token_parse_result result = {};
    if( token->type == CONFIG_TOKEN_NUMBER ){
        result.valid = true;
        result.number = atof( token->value );
    }else{
        result.valid = false;
    }

    return result;
}

i32 config_variable_try_integer( config_variable* var ){
    if( var ){
        if( var->value.type == CONFIG_VARIABLE_NUMBER ){
            return config_variable_value_try_integer( &var->value );
        }else{
            return 0;
        }
    }else{
        return 0;
    }
}

f32 config_variable_try_float( config_variable* var ){
    if( var ){
        if( var->value.type == CONFIG_VARIABLE_NUMBER ){
            return config_variable_value_try_float( &var->value );
        }else{
            return 0.0f;
        }
    }else{
        return 0.0f;
    }
}

const char* config_variable_value_try_string( config_variable_value* var ){
    if( var ){
        return var->value;
    }
}

const char* config_variable_try_string( config_variable* var ){
    if( var ){
        if( var->value.type == CONFIG_VARIABLE_SYMBOL ||
            var->value.type == CONFIG_VARIABLE_STRING ){
            return config_variable_value_try_string( &var->value );
        }else{
            return NULL;
        }
    }else{
        return NULL;
    }
}

static void config_variable_list_free( config_variable_list* list ){
    for( unsigned item_index = 0;
         item_index < list->count;
         ++item_index ){
        config_variable_value_free( &list->items[item_index] );
    }
    memory_deallocate(list->items);
}

static void config_variable_block_free( config_variable_block* block ){
    for( unsigned item_index = 0;
         item_index < block->count;
         ++item_index ){
        config_variable_free( &block->items[item_index] );
    }
    memory_deallocate(block->items);
}

static void config_variable_value_free( config_variable_value* variable_value ){
    switch( variable_value->type ){
        case CONFIG_VARIABLE_LIST:
        {
            config_variable_list_free(variable_value->as_list);
            memory_deallocate(variable_value->as_list);
        }
        break;
        case CONFIG_VARIABLE_BLOCK:
        {
            config_variable_block_free(variable_value->as_block);
            memory_deallocate(variable_value->as_block);
        }
        break;
    }
}

static void config_variable_free( config_variable* variable ){
    config_variable_value_free(&variable->value);
}

void config_info_free_all_fields( config_info* config ){
    for( unsigned variable_index = 0;
         variable_index < config->count;
         ++variable_index ){
        config_variable* current_variable = &config->variables[variable_index];
        config_variable_free(current_variable);
    }
    memory_deallocate(config->variables);
}

void config_variable_list_push_item( config_variable_list* list, 
                                     config_variable_value item ){
    list->count++;
    if( !list->items ){
        list->items = memory_allocate( sizeof(config_variable_value) );
    }else{
        config_variable_value* new_items_list = 
            memory_allocate( sizeof(config_variable_value) * list->count );
        memcpy( new_items_list,
                list->items,
                sizeof(config_variable_value) * (list->count - 1) );
        memory_deallocate(list->items);
        list->items = new_items_list;
    }

    list->items[list->count - 1] = item;
}

void config_variable_block_push_item( config_variable_block* block, config_variable var ){
    block->count++;
    if( !block->items ){
        block->items = memory_allocate( sizeof(config_variable) );
        block->items[block->count-1] = var;
    }else{
        config_variable* new_list = memory_allocate( sizeof(config_variable) * block->count );
        memcpy(new_list, 
               block->items,
               sizeof(config_variable) * (block->count - 1));
        memory_deallocate(block->items);
        block->items = new_list;
        block->items[block->count-1] = var;
    }
}

void config_info_push_variable( config_info* config, config_variable var ){
    config->count++;
    if( !config->variables ){
        config->variables = memory_allocate( sizeof(config_variable) );
        config->variables[config->count-1] = var;
    }else{
        config_variable* new_list = memory_allocate( sizeof(config_variable) * config->count );
        memcpy(new_list, 
               config->variables,
               sizeof(config_variable) * config->count - 1);
        memory_deallocate(config->variables);
        config->variables = new_list;
        config->variables[config->count-1] = var;
    }
}

static config_variable_block* config_parse_block( config_token_list* tokens, u64* token_index ){
    config_variable_block* block_contents = config_allocate_variable_block();
    (*token_index)++;

    for( *token_index; *token_index < tokens->count; (*token_index)++ ){
        config_token* first_token = &tokens->tokens[*token_index];

        if( first_token->type == CONFIG_TOKEN_RIGHT_BRACKET ){
            if( block_contents->count == 0 ){
                fprintf(stderr, "This block is empty... is this a mistake?\n");
            }
            return block_contents;
        }

        config_token* second_token = &tokens->tokens[++(*token_index)];
        if( *token_index-1 > tokens->count ){
            break;
        }

        if( second_token->type == CONFIG_TOKEN_COLON ){
            config_token* third_token = &tokens->tokens[++(*token_index)];

            if( *token_index-1 > tokens->count ){
                break;
            }

            switch( third_token->type ){
                case CONFIG_TOKEN_IDENTIFIER:
                {
                    config_variable symbol_variable = {};
                    strncpy( symbol_variable.name, first_token->value, 128 );
                    symbol_variable.value.type = CONFIG_VARIABLE_SYMBOL;
                    strncpy( symbol_variable.value.value, third_token->value, 128 );

                    config_variable_block_push_item( block_contents, symbol_variable );

#if 0
                    fprintf(stderr,
                            "created symbol : %s with value %s\n",
                            symbol_variable.name,
                            symbol_variable.value.value);
#endif
                }
                break;
                case CONFIG_TOKEN_STRING:
                {
                    config_variable string_variable = {};
                    strncpy( string_variable.name, first_token->value, 128 );
                    string_variable.value.type = CONFIG_VARIABLE_STRING;
                    strncpy( string_variable.value.value, third_token->value, 128 );

                    config_variable_block_push_item( block_contents, string_variable );

#if 0
                    fprintf(stderr,
                            "created string : %s with value \"%s\"\n",
                            string_variable.name,
                            string_variable.value.value);
#endif
                }
                break;
                case CONFIG_TOKEN_NUMBER:
                {
                    config_variable number_variable = {};
                    strncpy( number_variable.name, first_token->value, 128 );
                    number_variable.value.type = CONFIG_VARIABLE_NUMBER;
                    strncpy( number_variable.value.value, third_token->value, 128 );

                    config_variable_block_push_item( block_contents, number_variable );

#if 0
                    fprintf(stderr,
                            "created number : %s with value %s\n",
                            number_variable.name,
                            number_variable.value.value);
#endif
                }
                break;
                case CONFIG_TOKEN_LEFT_SQUARE_BRACKET:
                {
                    config_variable list_variable = {};
                    strncpy( list_variable.name, first_token->value, 128 );
                    list_variable.value.type = CONFIG_VARIABLE_LIST;
                    list_variable.value.as_list = config_parse_list( tokens, token_index );
#if 0
                    fprintf(stderr,
                            "created list : %s with %d items\n",
                            list_variable.name,
                            list_variable.value.as_list->count);
#endif
                    config_variable_block_push_item( block_contents, list_variable );
                }
                break;
                case CONFIG_TOKEN_LEFT_BRACKET:
                {
                    config_variable block_variable = {};
                    strncpy( block_variable.name, first_token->value, 128 );
                    block_variable.value.type = CONFIG_VARIABLE_BLOCK;
                    block_variable.value.as_block = config_parse_block( tokens, token_index );
#if 0
                    fprintf(stderr,
                            "created block : %s with %d items\n",
                            block_variable.name,
                            block_variable.value.as_block->count);
#endif
                    config_variable_block_push_item( block_contents, block_variable );
                }
                break;
                case CONFIG_TOKEN_EOF:
                default:
                {
                    fprintf(stderr, "INVALID TOKEN\n");
                }
                break;
            }
        }else{
            // error.
        }
    }
    fprintf(stderr, "finished assembling block\n");
    return block_contents;
}

static config_variable_list* config_parse_list( config_token_list* tokens, u64* token_index ){
    config_variable_list* list_contents = config_allocate_variable_list();
    (*token_index)++;
    for( *token_index; *token_index < tokens->count; (*token_index)++ ){
        config_token* current_token = &tokens->tokens[*token_index];
#if 0
        fprintf(stderr,
                "parse_list current_token : %s :val: %s\n",
                config_token_type_strings[current_token->type],
                current_token->value);
#endif
        if( current_token->type == CONFIG_TOKEN_LEFT_SQUARE_BRACKET ){
#if 0
            fprintf(stderr, "creating sub list\n");
#endif
            config_variable_value sub_list = {
                .type = CONFIG_VARIABLE_LIST,
                .as_list = config_parse_list( tokens, token_index )
            };
#if 0
            fprintf(stderr, "list pushed list!\n");
#endif
            config_variable_list_push_item( list_contents, sub_list );
        }else if( current_token->type == CONFIG_TOKEN_LEFT_BRACKET ||
                  current_token->type == CONFIG_TOKEN_RIGHT_BRACKET ){
            fprintf(stderr, "Bracket found. Blocks should not be members of lists!\n");
            // memory leak, crash, whatever. This shouldn't happen.
            return NULL;
        }else{
            config_token* item_value = &tokens->tokens[(*token_index)];

            if( item_value->type == CONFIG_TOKEN_RIGHT_SQUARE_BRACKET ){
                return list_contents;
            }

            config_variable_value item = {};
            switch( item_value->type ){
                case CONFIG_TOKEN_NUMBER:
                {
                    item.type = CONFIG_VARIABLE_NUMBER;
                    strncpy( item.value, item_value->value, 128 );
#if 0
                    fprintf(stderr, "list pushed number!\n");
#endif
                }
                break;
                case CONFIG_TOKEN_STRING:
                {
                    item.type = CONFIG_VARIABLE_STRING;
                    strncpy( item.value, item_value->value, 128 );
#if 0
                    fprintf(stderr, "list pushed string!\n");
#endif
                }
                break;
                case CONFIG_TOKEN_IDENTIFIER:
                {
                    item.type = CONFIG_VARIABLE_SYMBOL;
                    strncpy( item.value, item_value->value, 128 );
#if 0
                    fprintf(stderr, "list pushed symbol = %s\n", item.value);
#endif
                }
                break;
            }
            config_variable_list_push_item( list_contents, item );
        }

        config_token* separator = &tokens->tokens[++(*token_index)];
        if( separator->type == CONFIG_TOKEN_COMMA ){
            continue;
        }else if( separator->type == CONFIG_TOKEN_RIGHT_SQUARE_BRACKET ){
#if 0
            fprintf(stderr, "right bracket finished assembly\n");
#endif
            return list_contents;
        }
    }
#if 0
    fprintf(stderr, "finished assembling list\n");
#endif
    return list_contents;
}

config_info config_info_parse_all_tokens( config_token_list* tokens ){
    config_info result = {};

    for( u64 token_index = 0; token_index < tokens->count; ++token_index ){
        config_token* first_token = &tokens->tokens[token_index];
        config_token* second_token = &tokens->tokens[++token_index];
        if( token_index-1 > tokens->count ){
            break;
        }

        if( second_token->type == CONFIG_TOKEN_COLON ){
            config_token* third_token = &tokens->tokens[++token_index];

            if( token_index-1 > tokens->count ){
                break;
            }

            switch( third_token->type ){
                case CONFIG_TOKEN_IDENTIFIER:
                {
                    config_variable symbol_variable = {};
                    strncpy( symbol_variable.name, first_token->value, 128 );
                    symbol_variable.value.type = CONFIG_VARIABLE_SYMBOL;
                    strncpy( symbol_variable.value.value, third_token->value, 128 );

                    config_info_push_variable( &result, symbol_variable );

#if 0
                    fprintf(stderr,
                            "created symbol : %s with value %s\n",
                            symbol_variable.name,
                            symbol_variable.value.value);
#endif
                }
                break;
                case CONFIG_TOKEN_STRING:
                {
                    config_variable string_variable = {};
                    strncpy( string_variable.name, first_token->value, 128 );
                    string_variable.value.type = CONFIG_VARIABLE_STRING;
                    strncpy( string_variable.value.value, third_token->value, 128 );

                    config_info_push_variable( &result, string_variable );

#if 0
                    fprintf(stderr,
                            "created string : %s with value \"%s\"\n",
                            string_variable.name,
                            string_variable.value.value);
#endif
                }
                break;
                case CONFIG_TOKEN_NUMBER:
                {
                    config_variable number_variable = {};
                    strncpy( number_variable.name, first_token->value, 128 );
                    number_variable.value.type = CONFIG_VARIABLE_NUMBER;
                    strncpy( number_variable.value.value, third_token->value, 128 );

                    config_info_push_variable( &result, number_variable );

#if 0
                    fprintf(stderr,
                            "created number : %s with value %s\n",
                            number_variable.name,
                            number_variable.value.value);
#endif
                }
                break;
                case CONFIG_TOKEN_LEFT_SQUARE_BRACKET:
                {
                    config_variable list_variable = {};
                    strncpy( list_variable.name, first_token->value, 128 );
                    list_variable.value.type = CONFIG_VARIABLE_LIST;
                    list_variable.value.as_list = config_parse_list( tokens, &token_index );
#if 0
                    fprintf(stderr,
                            "created list : %s with %d items\n",
                            list_variable.name,
                            list_variable.value.as_list->count);
#endif
                    config_info_push_variable( &result, list_variable );
                }
                break;
                case CONFIG_TOKEN_LEFT_BRACKET:
                {
                    config_variable block_variable = {};
                    strncpy( block_variable.name, first_token->value, 128 );
                    block_variable.value.type = CONFIG_VARIABLE_BLOCK;
                    block_variable.value.as_block = config_parse_block( tokens, &token_index );
#if 0
                    fprintf(stderr,
                            "created block : %s with %d items\n",
                            block_variable.name,
                            block_variable.value.as_block->count);
#endif
                    config_info_push_variable( &result, block_variable );
                            
                }
                break;
                case CONFIG_TOKEN_EOF:
                default:
                {
                    fprintf(stderr, "INVALID TOKEN\n");
                }
                break;
            }
        }else{
            // blocks are handled differently... They are technically treated
            // as a weird variable declaration.
        }
    }

    return result;
}

config_variable* config_block_find_first_variable( config_variable_block* block, char* variable_name ){
    for( u64 variable_index = 0; variable_index < block->count; ++variable_index ){
        config_variable* current_variable = &block->items[variable_index];
        if( strcmp(current_variable->name, variable_name) == 0 ){
            return current_variable;
        }
    }

    return NULL;
}

config_variable* config_info_find_first_variable( config_info* config, char* variable_name ){
    for( u64 variable_index = 0; variable_index < config->count; ++variable_index ){
        config_variable* current_variable = &config->variables[variable_index];
        if( strcmp(current_variable->name, variable_name) == 0 ){
            return current_variable;
        }
    }

    return NULL;
}

f32 config_variable_value_try_float( config_variable_value* var ){
    f32 result;

    switch( var->type ){
        case CONFIG_VARIABLE_NUMBER:
        {
            result = atof( var->value );
        }
        break;
        default:
        {
            if( var->type != CONFIG_VARIABLE_LIST ){
                fprintf(stderr,
                        "Error: try_float found a non numeric value(%s) when converting.\n",
                        var->value);
            }
        }
        break;
    }

    return result;
}

i32 config_variable_value_try_integer( config_variable_value* var ){
    i32 result;

    switch( var->type ){
        case CONFIG_VARIABLE_NUMBER:
        {
            // to avoid too many errors, I will atof and truncate from
            // float to integer.
            f32 floating_point = atof( var->value );
            result = (i32)floating_point;
        }
        break;
        default:
        {
            if( var->type != CONFIG_VARIABLE_LIST ){
                fprintf(stderr,
                        "Error: try_integer found a non numeric value(%s) when converting.\n",
                        var->value);
            }
        }
        break;
    }

    return result;
}

// dynamically allocated because this is not meant to stay around long.
config_token_list config_tokenize( char* source_text, size_t source_text_length ){
    config_token_list result = {};
    config_token_list_preallocate( &result, 512 );

    char* cursor = source_text;

    /* fprintf(stderr, "SOURCE TEXT : %s\n", source_text); */
    
    while( *cursor ){
        switch( *cursor ){
            case '/':
            {
                if( *(cursor + 1) == '/' ){
                    while( *cursor != '\n' && *cursor != 0 ){ 
                        cursor++; 
                    }
                }else if( *(cursor + 1) == '*' ){
                    while( !(*(cursor) == '*' && 
                             *(cursor + 1) == '/')  ){
                        cursor++;
                    }
                    cursor++;
                }
            }
            break;
            case '\"':
            {
                config_token new_string = { 
                    .type = CONFIG_TOKEN_STRING 
                };

                char* string_cursor = new_string.value;
                cursor++;
                while( *(cursor) && *(cursor) != '\"' ){
                    *string_cursor = *cursor;
                    cursor++;
                    string_cursor++;
                }
                //cursor++;
                config_token_list_push( &result, new_string );
            }
            break;
            case '{':
            {
                config_token_list_push( &result, (config_token){ .type = CONFIG_TOKEN_LEFT_BRACKET, .value[0] = '{' } );
            }
            break;
            case '}':
            {
                config_token_list_push( &result, (config_token){ .type = CONFIG_TOKEN_RIGHT_BRACKET, .value[0] = '}' } );
            }
            break;
            case ']':
            {
                config_token_list_push( &result, (config_token){ .type = CONFIG_TOKEN_RIGHT_SQUARE_BRACKET, .value[0] = ']' } );
            }
            break;
            case '[':
            {
                config_token_list_push( &result, (config_token){ .type = CONFIG_TOKEN_LEFT_SQUARE_BRACKET, .value[0] = '[' } );
            }
            break;
            case ',':
            {
                config_token_list_push( &result, (config_token){ .type = CONFIG_TOKEN_COMMA, .value[0] = ',' } );
            }
            break;
            case ':':
            {
                config_token_list_push( &result, (config_token){ .type = CONFIG_TOKEN_COLON, .value[0] = ':' } );
            }
            break;
            case '\t':
            case '\r':
            case '\n':
            case ' ':
            {
            }
            break;
            default:
            {
                config_token identifier = (config_token){ 
                    .type = CONFIG_TOKEN_IDENTIFIER 
                };

                b32 parse_number = is_numeric(*cursor);
                b32 found_decimal = false;

                if( parse_number ){
                    identifier.type = CONFIG_TOKEN_NUMBER;
                }

                char* string_cursor = identifier.value;
                while( (*cursor) ){
                    if( !is_valid_identifier_character(*cursor) ){
                        break;
                    }

                    if( is_whitespace(*cursor) ){
                        break;
                    }

                    if( parse_number ){
                        if( !is_numeric(*cursor) && *cursor != '.' ){
                            //cursor--;
                            break;
                        }

                        if( *cursor == '.' ){
                            if( !found_decimal ){
                                found_decimal = true;
                            }else{
                                fprintf(stderr, "syntax warning?\n");
                            }
                        }
                    }

                    *string_cursor = *cursor;

                    string_cursor++;
                    cursor++;
                }

                /* fprintf(stderr, "ident read as : %s\n", identifier.value); */
                cursor--;
                config_token_list_push( &result, identifier );
            }
            break;
        }
        cursor++;
    }

    return result;
}

// global state.
static i32 dump_current_scope_count = 0;

static void send_indent_to_stderr( void ){
    for( unsigned indent_level = 0;
         indent_level != dump_current_scope_count;
         ++indent_level ){
        fprintf(stderr, "\t");
    }
}

void config_variable_list_dump_as_text_to_stderr( config_variable_list* variable_list ){
    for( u64 item_index = 0; item_index < variable_list->count; ++item_index ){
        config_variable_value* current_item = &variable_list->items[item_index];
#ifdef TRY_TO_PRETTIFY_LIST_DUMP
        if( current_item->type == CONFIG_VARIABLE_LIST ){
            // perserve mine eyes.
            fprintf(stderr, "\n");
            send_indent_to_stderr();
        }
#endif

        config_variable_value_dump_as_text_to_stderr( current_item );

        if( (item_index + 1) < variable_list->count ){
            fprintf(stderr, ", ");
        }
    }
}

void config_variable_block_dump_as_text_to_stderr( config_variable_block* variable_block ){
    for( u64 item_index = 0; item_index < variable_block->count; ++item_index ){
        config_variable* current_variable = &variable_block->items[item_index];
        config_variable_dump_as_text_to_stderr( current_variable );
    }
}

void config_variable_value_dump_as_text_to_stderr( config_variable_value* variable_value ){
    switch( variable_value->type ){
        case CONFIG_VARIABLE_STRING:
        {
            fprintf(stderr, "\"%s\"", variable_value->value);
        }
        break;
        case CONFIG_VARIABLE_NUMBER:
        case CONFIG_VARIABLE_SYMBOL:
        {
            fprintf(stderr, "%s", variable_value->value);
        }
        break;
        case CONFIG_VARIABLE_LIST:
        {
            // ugly with nested lists, needs human to make look good.
            fprintf(stderr, "[");
            config_variable_list_dump_as_text_to_stderr( variable_value->as_list );
            fprintf(stderr, "]");
        }
        break;
        case CONFIG_VARIABLE_BLOCK:
        {
            fprintf(stderr, "{\n");
            dump_current_scope_count++;
            config_variable_block_dump_as_text_to_stderr( variable_value->as_block );
            dump_current_scope_count--;
            send_indent_to_stderr();
            fprintf(stderr, "}");
        }
        break;
        default:
        {
        }
        break;
    }
}

void config_variable_dump_as_text_to_stderr( config_variable* variable ){
    send_indent_to_stderr();
    fprintf(stderr, "%s : ", variable->name);
    config_variable_value_dump_as_text_to_stderr( &variable->value );
    fprintf(stderr, "\n");
}

void config_info_dump_as_text_to_stderr( config_info* config ){
    dump_current_scope_count = 0;
    for( u64 var_index = 0; var_index < config->count; ++var_index ){
        config_variable* current_var = &config->variables[var_index];
        config_variable_dump_as_text_to_stderr( current_var );
    }
}

char* config_variable_block_dump_as_text_as_str( config_variable_block* variable_block ){
    char* block_text = string_new_empty();

    for( u64 item_index = 0; item_index < variable_block->count; ++item_index ){
        config_variable* current_variable = &variable_block->items[item_index];
        char* variable_string = config_variable_dump_as_text_as_str( current_variable );
        block_text = string_format_append(block_text, "%s", variable_string);
        string_free( variable_string );
    }

    return block_text;
}

char* config_variable_list_dump_as_text_as_str( config_variable_list* variable_list ){
    char* result = string_new_empty();

    for( u64 item_index = 0; item_index < variable_list->count; ++item_index ){
        config_variable_value* current_item = &variable_list->items[item_index];
        char* item_string = config_variable_value_dump_as_text_as_str( current_item );

        result = string_format_append(result, "%s", item_string);
        if( (item_index + 1) < variable_list->count ){
            result = string_format_append(result, ", ");
        }

        string_free(item_string);
    }

    return result;
}

char* config_variable_value_dump_as_text_as_str( config_variable_value* variable_value ){
    char* result = string_new_empty();

    switch( variable_value->type ){
        case CONFIG_VARIABLE_STRING:
        {
            result = string_format_append(result, "\"%s\"", variable_value->value);
        }
        break;
        case CONFIG_VARIABLE_NUMBER:
        case CONFIG_VARIABLE_SYMBOL:
        {
            result = string_format_append(result, "%s", variable_value->value);
        }
        break;
        case CONFIG_VARIABLE_LIST:
        {
            char* variable_list_string = config_variable_list_dump_as_text_as_str( variable_value->as_list );
            result = string_format_append(result, "[%s]", variable_list_string);
            string_free(variable_list_string);
        }
        break;
        case CONFIG_VARIABLE_BLOCK:
        {
            dump_current_scope_count++;
            char* block_list_string = config_variable_block_dump_as_text_as_str( variable_value->as_block );
            dump_current_scope_count--;

            result = string_format_append(result, "{\n%s", block_list_string);
            for( unsigned indent_level = 0;
                    indent_level < dump_current_scope_count;
                    ++indent_level ){
                result = string_format_append(result, "\t");
            }
            result = string_format_append(result, "}");
            string_free(block_list_string);
        }
        break;
        default:
        {
            fprintf(stderr, "this shouldn't happen\n");
            return NULL;
        }
        break;
    }

    return result;
}

char* config_variable_dump_as_text_as_str( config_variable* variable ){
    char* variable_value = config_variable_value_dump_as_text_as_str( &variable->value );
    char* result = string_new_empty();

    for( unsigned indent_level = 0;
         indent_level < dump_current_scope_count;
         ++indent_level ){
        result = string_format_append(result, "\t");
    }
    result = string_format_append(result, "%s : %s\n", variable->name, variable_value);

    string_free(variable_value);
    return result;
}

char* config_info_dump_as_text_as_str( config_info* config ){
    dump_current_scope_count = 0;
    char* start = string_new_empty();

    for( u64 var_index = 0; var_index < config->count; ++var_index ){
        config_variable* current_var = &config->variables[var_index];
        char* variable_string = config_variable_dump_as_text_as_str( current_var );

        if( variable_string ){
            start = string_format_append(start, "%s", variable_string);
            string_free(variable_string);
        }
    }

    return start;
}

void config_info_dump_as_text_into_file( config_info* config, const char* file_name ){
    char* entire_config_as_string = config_info_dump_as_text_as_str( config );
#if 0
    fprintf(stderr, "??\n%s\n", entire_config_as_string);
#endif

    write_buffer_into_file( file_name,
                            entire_config_as_string,
                            string_length(entire_config_as_string) );

    string_free(entire_config_as_string);
}
