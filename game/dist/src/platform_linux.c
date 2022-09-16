/*
 * Linux or generally POSIX things.
 *
 * NOTE(jerry):
 *
 * Everything here is a dummy just to actually compile
 * on linux.
 */

#include "platform.h"

#include <dirent.h>
#include <sys/stat.h> // TODO(jerry): Replace C standard library with linux system stuff.
#include <sys/types.h>
#include <sys/sysmacros.h>
#include <unistd.h>

#include <stdio.h>
#include <stdlib.h>

#if 0
#pragma message "Linux is using dummy implementations for now(standard library for things.)"
#endif

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

static void get_linux_files_in_directory( const char* directory_path,
                                          platform_directory* out_listing ){
    DIR* directory_information;
    struct dirent* directory_entry;

    // good to know I need to abstract filepath as well.
    // good to know I might want to start writing some string helpers...
    char* acceptable_file_path = memory_allocate( strlen( directory_path ) + 2 );
    memset(acceptable_file_path, 0, strlen( directory_path ) + 2);
    strcat(acceptable_file_path, "./");
    strcat(acceptable_file_path, directory_path);

    directory_information = opendir( acceptable_file_path );

    const b32 dynamically_allocated = (out_listing->capacity == 0);

    if(directory_information){
        while( (directory_entry = readdir(directory_information)) ){
            platform_file_info file_info = {};
            strncpy( file_info.file_name, directory_entry->d_name, PLATFORM_FILE_NAME_MAX );
            {
                struct stat file_stat_info;

                char full_path_name[PLATFORM_FILE_NAME_MAX] = {};
                memset(full_path_name, 0, PLATFORM_FILE_NAME_MAX);
                strcat(full_path_name, acceptable_file_path);
                strcat(full_path_name, file_info.file_name);

                int stat_result = stat(full_path_name, &file_stat_info);
                /* fprintf(stderr, "stating : %s\n", full_path_name); */
                if(stat_result){
                    // error
                }else{
                    file_info.size = file_stat_info.st_size;
                }

                if( S_ISDIR(file_stat_info.st_mode) ) {
                    file_info.type = PLATFORM_FILE_TYPE_DIRECTORY;
                }else{
                    file_info.type = PLATFORM_FILE_TYPE_FILE;
                }
            }

            if( dynamically_allocated ){
                platform_directory_push_dynamically_allocated( out_listing, file_info );
            }else{
                platform_directory_push( out_listing, file_info );
            }
        }
        closedir(directory_information);
    }
    memory_deallocate( acceptable_file_path );
}

static u64 get_linux_file_size( const char* file_name ){
    FILE* file_handle = fopen( file_name, "rb" );

    fseek( file_handle, 0, SEEK_END );
    u64 file_size = ftell( file_handle );
    fseek( file_handle, 0, SEEK_SET );

    fclose(file_handle);
    return file_size;
}

static u8* load_linux_file_into_buffer( const char* file_name ){    
    u8* file_buffer = NULL;

    FILE* file_handle = fopen( file_name, "rb" );

    if( ferror( file_handle ) ){
        perror( "load_linux_file_into_buffer fread error: " );
    }

    if( file_handle ){
        fseek( file_handle, 0, SEEK_END );
        u64 file_size = ftell( file_handle );
        fseek( file_handle, 0, SEEK_SET );

        if( ferror( file_handle ) ){
            perror( "load_linux_file_into_buffer fread error: " );
        }

        file_buffer = malloc( file_size + 1 );

        if( file_buffer ){
            u64 read_bytes = fread( file_buffer, 1, file_size, file_handle );

            if( feof( file_handle ) ){
                fprintf( stderr, "EOF found?\n" );
            }

            if( ferror( file_handle ) ){
                perror( "load_linux_file_into_buffer fread error: " );
            }

        }else{
            fprintf( stderr, "Could not allocate memory to store file\n" );
        }
        file_buffer[file_size] = 0;
    }

    fclose(file_handle);

    return file_buffer;
}

static b32 does_linux_file_exist( const char* file_name ) {
    struct stat file_stat_info = {};
    int stat_result = stat(file_name, &file_stat_info);

    if (stat_result) {
        return false;
    } else if (stat_result == 0) {
        return true;
    }

    return true;
}

static void write_linux_buffer_into_file( const char* file_name,
                                          u8* buffer,
                                          size_t buffer_length ){
    // for now using the C runtime.
    FILE* file_handle = fopen( file_name, "w+" );

    if( file_handle ){
        fwrite( buffer, buffer_length, 1, file_handle );
    }

    fclose(file_handle);
}

static u8* memory_linux_allocate( char* base_address, 
                                  const u64 bytes_to_alloc  ){
    u8* memory = calloc( bytes_to_alloc, 1 );

    if( memory ){
        //fprintf(stderr, "Malloc succeeded! : %p of size %ld\n", memory, bytes_to_alloc);
    }

    return memory;
}

static void memory_linux_free( char* memory ){
    free( memory );
}

u8* load_file_into_buffer( const char* file_name ){
    u8* file_buffer = load_linux_file_into_buffer( file_name );
    return file_buffer;
}

u8* memory_allocate( const u64 bytes_to_alloc ){
    u8* memory_base_address =
        memory_linux_allocate( 0, bytes_to_alloc );

    return memory_base_address;
}

u64 get_file_size( const char* file_name ){
    return get_linux_file_size( file_name );
}

void memory_deallocate( u8* memory ){
    memory_linux_free( memory );
}

void get_files_in_directory( const char* directory, platform_directory* out_listing ){
    get_linux_files_in_directory( directory, out_listing );
}

void write_buffer_into_file( const char* file_name, u8* buffer, size_t buffer_length ){
    write_linux_buffer_into_file( file_name, buffer, buffer_length );
}

b32 does_file_exist( const char* file_name ){
    return does_linux_file_exist( file_name );
}
