#ifndef BMP_LOADER_H
#define BMP_LOADER_H

#include "common.h"

/* NOTE(jerry):
   This is probably temporary.
*/

typedef struct{
    u64 width;
    u64 height;

    // RGBA pixels.
    // NOTE(jerry):
    // Use zero / one length array to allow for variable-length objects.
    // This also reduces the memory management work ( single allocation ).
    u8* pixels;
}bitmap_image;

void free_bitmap_image( bitmap_image* bmp );
bitmap_image* load_bitmap_image( const char* file_name );
bitmap_image* load_image( const char* file_name );
/*HIJACKING TO USE STB_IMAGE*/

#endif
