/*
  NOTE(jerry): I think I messed something up although it seems to work
  exactly as I think it should.
  
  I messed up the counters?
 */
#include "string.h"
#include "platform.h"

#include "memory_pool.h"

#include <assert.h>
#include <stdarg.h>
#include <stdio.h> // snprintf

typedef struct string_header{
    u64 capacity;
    u64 used;
}string_header;

static memory_pool static_strings = {};

void strings_initialize( void ){
    memory_pool_init(&static_strings, MB(6));
}

void strings_finish( void ){
    memory_pool_finish(&static_strings);
}

char* strings_make_static(const char* str) {
    u64 allocation_size = strlen(str) + sizeof(string_header);
    char* new_string = memory_pool_allocate(&static_strings, allocation_size);
    string_header* header = new_string;

    header->capacity = allocation_size;
    header->used = allocation_size;

    return new_string + sizeof(string_header);
}

static void* strings_internal_allocate( size_t amount ){
    char* result = memory_allocate(amount + sizeof(string_header));
    string_header* header = result;
    header->capacity = amount;
    header->used = 0;
    return result + sizeof(string_header);
}

static void strings_internal_free( void* pointer ){
    char* begin = (char*)pointer - sizeof(string_header);
    fprintf(stderr, "strings free?\n");
    memory_deallocate(begin);
}

char* string_new_empty( void ){
    char* address = strings_internal_allocate(2);
    address[0] = '\0';
    return address;
}

char* string_new_c_str( char* c_str ){
    u64 length_of_c_str = strlen(c_str);
    return string_new(c_str, length_of_c_str);
}

char* string_new( char* string, u64 length ){
    char* address = strings_internal_allocate(length+1);
    string_header* header = address - sizeof(string_header);
    header->used = length;

    for( unsigned ch = 0; ch < length; ++ch ){
        address[ch] = string[ch];
    }
    address[length] = 0;

    return address;
}

char* string_dup( char* string ){
    // assuming other string is in the same format.
    return string_new( string, string_length(string) );
}

void string_free( char* string ){
    strings_internal_free(string);
}

b32 string_equal( char* a, char* b ){
    // pointer check can help I guess.
    if( a == b ){
        return true;
    }else{
        size_t a_len = strlen(a);
        size_t b_len = strlen(b);

        if( a_len == b_len ){
            for( unsigned ch = 0; ch < a_len; ++ch ){
                if( a[ch] != b[ch] ){
                    return false;
                }
            }

            return true;
        }else{
            return false;
        }
    }

    return false;
}

// not done.
i32 string_compare( char* a, char* b ){
    return strcmp(a, b);
}

static u64 string_capacity( char* string ){
    string_header* header = string - sizeof(string_header);
    return header->capacity;
}

u64 string_length( char* string ){
    string_header* header = string - sizeof(string_header);
    return header->used;
}

static u64 string_allocable_space( char* string ){
    size_t length = string_length(string);
    size_t capacity = string_capacity(string);

    return capacity - length;
}

static char* string_alloc_with_enough_space( char* string, size_t required ){
    char* result = string;

    if( string_allocable_space(string) < required ){
        printf("make new\n");
        string_free(string);
        result = strings_internal_allocate(required+1); 
    }

    return result;
}

// dest should be a "string" not cstr.
char* string_concatenate( char* dest, char* src ){
    size_t size_to_alloc = 0;
    size_t src_length = strlen(src);
    size_t dest_length = string_length(dest);
    size_to_alloc = dest_length + src_length;

    char* new_string = string_alloc_with_enough_space(dest, size_to_alloc);

    string_header* new_head = new_string - sizeof(string_header);
    new_head->used = size_to_alloc;

    unsigned chptr = 0;
    for( unsigned ch = 0; ch < dest_length; ++ch ){
        new_string[chptr] = dest[ch];
        chptr++;
    }

    for( unsigned ch = 0; ch < src_length; ++ch ){
        new_string[chptr] = src[ch];
        chptr++;
    }

    return new_string;
}

char* string_format_append( char* string, const char* format_string, ... ){
    va_list args;

    va_start(args, format_string);
    size_t amount_to_write = vsnprintf(NULL, 0, format_string, args);
    va_end(args);

    size_t arg_len = string_length(string);
    char* new_string = string_alloc_with_enough_space(string, amount_to_write + arg_len + 1);
    new_string = string_concatenate(new_string, string);

    va_start(args, format_string);
    vsnprintf(new_string + arg_len, amount_to_write+1, format_string, args);
    va_end(args);

    string_header* head = new_string - sizeof(string_header);
    head->used = arg_len + amount_to_write;


    return new_string;
}

char* string_format( const char* format_string, ... ){
    va_list args;
    va_start(args, format_string);
    size_t amount_to_write = vsnprintf(NULL, 0, format_string, args);
    va_end(args);
    char* new_string = strings_internal_allocate(amount_to_write+1);
    va_start(args, format_string);
    vsnprintf(new_string, amount_to_write+1, format_string, args);
    va_end(args);

    string_header* head = new_string - sizeof(string_header);
    head->used = amount_to_write;


    return new_string;
}
