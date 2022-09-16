/*
  Win32 platform implementation of stuff.
*/

#include "platform.h"

#include <windows.h>

#ifdef DEBUG_BUILD
static size_t memory_allocate_counter = 0;
size_t platform_debug_get_memory_allocation_count(void) {
    return memory_allocate_counter;
}
#endif

/*
  NOTE(jerry):
  Use Operating System functions instead of C Standard Library functions
  for memory allocation... Kind of defeats the purpose of using
  the windows API for anything more than windows....
  
  Maybe also don't dynamically allocate that buffer and just
  pass in the buffer as a parameter? At the point I'm imitating most standard
  OS file utilities so eh.
*/
static u64 get_win32_file_size( const char* file_name ){
    HANDLE file_handle = CreateFileA( file_name,
                                      GENERIC_READ,
                                      FILE_SHARE_READ,
                                      NULL,
                                      OPEN_EXISTING,
                                      FILE_ATTRIBUTE_NORMAL,
                                      NULL );
    if( file_handle != INVALID_HANDLE_VALUE ){
        size_t file_size = GetFileSize( file_handle, NULL );

        CloseHandle( file_handle );
        return file_size;
    }else{
        return 0;
    }
}

static b32 does_win32_file_exist( const char* file_name ) {
    HANDLE file_handle = CreateFileA( file_name,
                                      GENERIC_READ,
                                      FILE_SHARE_READ,
                                      NULL,
                                      OPEN_EXISTING,
                                      FILE_ATTRIBUTE_NORMAL,
                                      NULL );

    return (file_handle != INVALID_HANDLE_VALUE);
}

static void write_buffer_into_win32_file( const char* file_name,
                                          u8* buffer,
                                          size_t buffer_length ){
    HANDLE file_handle = CreateFileA( file_name,
                                      GENERIC_WRITE,
                                      FILE_SHARE_WRITE,
                                      NULL,
                                      CREATE_ALWAYS,
                                      FILE_ATTRIBUTE_NORMAL,
                                      NULL );
    if( file_handle != INVALID_HANDLE_VALUE ){
        DWORD written_bytes = 0;

        WriteFile( file_handle,
                   (LPCVOID)buffer,
                   buffer_length,
                   &written_bytes,
                   NULL );

        CloseHandle( file_handle );
    }else{
        return;
    }
}

// special case.
static void platform_directory_push_dynamically_allocated( platform_directory* dir,
                                                           platform_file_info file_info ){
    stub_important_code("Dynamical allocation path not implemented for platform_directory_push");
}

static void platform_directory_push( platform_directory* dir, 
                                     platform_file_info file_info ){
    platform_file_info* current = &dir->files[ dir->count++ ];
    *current = file_info;
}

static void get_win32_files_in_directory( const char* directory_path, platform_directory* out_listing ){
    WIN32_FIND_DATA found_file_data = {};
    char* valid_directory_path = memory_allocate( strlen(directory_path) + 3 );
    strcpy( valid_directory_path, directory_path );
    strcat( valid_directory_path, "/*");
    HANDLE file = FindFirstFile( valid_directory_path, &found_file_data );
    memory_deallocate( valid_directory_path );

    const b32 dynamically_allocated = (out_listing->capacity == 0);

    if( file != INVALID_HANDLE_VALUE ){
        do{
            platform_file_info file_info = {};

            if( found_file_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ){
                file_info.type = PLATFORM_FILE_TYPE_DIRECTORY;
            }

            strncpy( file_info.file_name, found_file_data.cFileName, PLATFORM_FILE_NAME_MAX );
            file_info.size = 
                (found_file_data.nFileSizeHigh * (MAXDWORD + 1)) + found_file_data.nFileSizeLow;

            if( dynamically_allocated ){
                platform_directory_push_dynamically_allocated( out_listing, file_info );
            }else{
                platform_directory_push( out_listing, file_info );
            }
        }while( FindNextFile(file, &found_file_data) );
    }
    
    FindClose( file );
}

static u8* load_win32_file_into_buffer( const char* file_name ){
    u8* file_buffer;

    HANDLE file_handle = CreateFileA( file_name,
                                      GENERIC_READ,
                                      FILE_SHARE_READ,
                                      NULL,
                                      OPEN_EXISTING,
                                      FILE_ATTRIBUTE_NORMAL,
                                      NULL );

    fprintf( stderr, "Attempting to read \"%s\"\n", file_name );

    if( file_handle != INVALID_HANDLE_VALUE ){
        size_t file_size = GetFileSize( file_handle, NULL );
        //makes windows happy?
        size_t read_bytes = 0;

        file_buffer = memory_allocate( file_size + 1 );

        if( file_buffer ){
            ReadFile( file_handle,
                      file_buffer,
                      file_size,
                      (LPDWORD)&read_bytes, NULL );

            fprintf( stderr, "Read %lld bytes from file\n", read_bytes );
        }else{
            fprintf( stderr, "Could not allocate memory to store file\n" );
        }
        file_buffer[file_size] = 0;
    }else{
        fprintf( stderr, "Could not open file" );
    }

    CloseHandle( file_handle );
    return file_buffer;
}

// I'm going to encode the amount of memory
// by allocating a few extra bytes... Sorry.
static u64 size_of_header = sizeof(u64);

#if 1
static u8* memory_win32_allocate(
        const void* base_address,
        const u64 bytes_to_alloc ){
    u8* memory;

    memory = VirtualAlloc( base_address,
                           bytes_to_alloc + size_of_header,
                           MEM_COMMIT | MEM_COMMIT,
                           PAGE_READWRITE );
    *((u64*)(memory)) = bytes_to_alloc;

    return memory + size_of_header;
}

static void memory_win32_free( u8* memory ){
    if( memory ){
#ifdef DEBUG_BUILD
    memory_allocate_counter--;
#endif
        u8* actual_base_address = (memory) - size_of_header;

        u64 size_of_memory = (u64)*(actual_base_address) + size_of_header;
    
        VirtualFree( memory, 
                     size_of_memory,
                     MEM_RELEASE );
    }
}
#else
static u8* memory_win32_allocate(
        const void* base_address,
        const u64 bytes_to_alloc ){
#ifdef DEBUG_BUILD
    memory_allocate_counter++;
#endif
    u8* memory = malloc(bytes_to_alloc);
    return memory;
}

static void memory_win32_free( u8* memory ){
    if( memory ){
        free(memory);
    }
}
#endif

u8* load_file_into_buffer( const char* file_name ){
    return load_win32_file_into_buffer( file_name );
}

u8* memory_allocate( const u64 bytes_to_alloc ){
    u8* memory_base_address =
        memory_win32_allocate( 0,
                               bytes_to_alloc );

    return memory_base_address;
}

u64 get_file_size( const char* file_name ){
    return get_win32_file_size( file_name );
}

void memory_deallocate( u8* memory ){
    memory_win32_free( memory );
}

void get_files_in_directory( const char* directory, platform_directory* out_listing ){
    get_win32_files_in_directory( directory, out_listing );
}

void write_buffer_into_file( const char* file_name,
                             u8* buffer,
                             size_t buffer_length ){
    write_buffer_into_win32_file( file_name, buffer, buffer_length );
}

b32 does_file_exist( const char* file_name ){
    return does_win32_file_exist( file_name );
}
