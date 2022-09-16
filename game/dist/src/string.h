#ifndef STRING_H
#define STRING_H

#include "common.h"

// C_string compatible functions will just be called
// str_*function* I guess.
// string_ is for the string with header stuff.

void strings_initialize( void );
void strings_finish( void );

char* strings_make_static(const char* str);

b32 string_equal( char* a, char* b );
i32 string_compare( char* a, char* b );

char* string_new_empty( void );
char* string_new_c_str( char* c_str );
char* string_new( char* string, u64 length );
char* string_dup( char* string );

void string_free( char* string );

u64 string_length( char* string );

char* string_push( char* dest, char c );
char* string_pop( char* dest );

char* string_concatenate( char* dest, char* src );
/*use string_dup char* string_copy( char* dest, char* src ); */

char* string_format_append( char* string, const char* format_string, ... );
char* string_format( const char* format_string, ... );

#endif
