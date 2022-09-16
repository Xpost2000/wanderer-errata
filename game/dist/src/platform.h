#ifndef PLATFORM_H
#define PLATFORM_H

#include "common.h"

/*
  NOTE(jerry):
  defines functions and other commonly accessed platform stuff that
  can only be implemented by the OS.

  This doesn't include windowing or whatever that is handled
  in the actual platform layer itself.

  Implemented in
  
  OS_main.c
  or
  platform_OS.c files.
  
  for now I think I only use load_file_into_buffer so eh.
*/
#define PLATFORM_FILE_NAME_MAX 260
// I seriously hope this doesn't get exceeded...
// TODO(jerry): Just dynamially allocate this lmao.
#define PLATFORM_MAX_FILES_IN_DIRECTORY 256

enum platform_file_info_type{
    PLATFORM_FILE_TYPE_FILE,
    PLATFORM_FILE_TYPE_DIRECTORY,

    PLATFORM_FILE_TYPES
};

typedef struct platform_file_info{
    u8 type;
    char file_name[PLATFORM_FILE_NAME_MAX];
    u64 size;
}platform_file_info;

typedef struct platform_directory{
    u32 capacity;

    u32 count;
    platform_file_info* files;
}platform_directory;

void write_buffer_into_file( const char* file_name, u8* buffer, size_t buffer_length );
u8* load_file_into_buffer( const char* file_name );
u64 get_file_size( const char* file_name );
b32 does_file_exist( const char* file_name );

u8*  memory_allocate( const u64 bytes_to_alloc );
void memory_deallocate( u8* memory );
void get_files_in_directory( const char* directory, platform_directory* out_listing );

#ifdef DEBUG_BUILD
size_t platform_debug_get_memory_allocation_count(void);
#endif

// implemented in the platform mains
void platform_move_mouse( int x, int y );

#endif
