//#define STBI_MALLOC memory_allocate
//I don't have a realloc at the moment.
//#define STBI_FREE memory_deallocate

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "bmp_loader.h"
#include "platform.h"

/*
  NOTE(jerry):
  Clean this up maybe. Also
  remove dependence on C Standard Library if possible,
  though that's not really essential.
*/
bitmap_image* load_bitmap_image( const char* file_name ){
    bitmap_image* bmp_image = memory_allocate(sizeof(bitmap_image));

    u8* bitmap_file_buffer = load_file_into_buffer( file_name );
    u8* bitmap_file_buffer_start = bitmap_file_buffer;

    //fprintf(stderr, "Attempting to read \"%s\" as BMP\n", file_name);

    // TODO(jerry): Change so it works on non-gcc compilers...
    // there does not appear to be a portable alignment changer( well there is alignof,
    // but that requires operating on EVERY single field which is eh? )
    // it seems on linux cross-compiling this doesn't work????
    struct __attribute__((__packed__)) bitmap_file_header{
        char bmp_id[2];
        u32 bmp_size;
        u32 reserved;
        u32 bmp_pixel_offset;
    };

    struct __attribute__((__packed__)) bitmap_info_header{
        u32 header_size;

        i32 bmp_width;
        i32 bmp_height;

        u16 color_planes;
        u16 bits_per_pixel;

        u32 compression_type;
        u32 image_size;

        u32 pixels_per_meter[2];

        u32 colors_used;
        u32 important_colors;
    };

    // this is technically just used to declare
    // a table of constants. so eh.
    enum bmp_compression_method{
        BMP_RGB = 0,
        BMP_RLE8 = 1,
        BMP_RLE4 = 2,
        BMP_BITFIELDS = 3,
        BMP_JPEG = 4,
        BMP_PNG = 5,
        BMP_ALPHABITFIELDS = 6,
        BMP_CMYK = 11,
        BMP_CMYKRLE8 = 12,
        BMP_CMYKRLE4 = 13,

        COMPRESSION_TYPES_COUNT = 10
    };

    struct bitmap_file_header file_head;
    struct bitmap_info_header bmp_info;

    /* 
       NOTE(jerry): 
       Since this is temporary code
       I'll let this really odd error slide.

       assert( sizeof(file_head) == 14 );
       assert( sizeof(bmp_info) == 40 );
    */

    if( bitmap_file_buffer ){
        u8 header_bytes[14];

        memcpy(header_bytes, bitmap_file_buffer, sizeof(header_bytes)); 
        bitmap_file_buffer += sizeof(header_bytes);

        //fprintf(stderr, "Reading file header\n");

        memcpy(&file_head, header_bytes, sizeof(header_bytes));

        /*Apparently __attribute__((__packed__)) doesn't work when
         I cross compile from linux so idk man.*/
        file_head.bmp_pixel_offset = *((u32*)(&header_bytes[10]));

        if(file_head.bmp_id[0] == 'B' &&
           file_head.bmp_id[1] == 'M'){

            //fprintf(stderr, "Reading core header\n");
            memcpy(&bmp_info, bitmap_file_buffer, sizeof(bmp_info));
            bitmap_file_buffer += sizeof(bmp_info);
            
            /*
            fprintf(stderr, "bmp_width: %d\nbmp_height: %d\nbits_per_pixel: %d\n",
                    bmp_info.bmp_width, bmp_info.bmp_height,
                    bmp_info.bits_per_pixel);
                    */

            switch( bmp_info.bits_per_pixel ){
                case 32:
                case 24:
                {
//                  fprintf(stderr, "detected RGB or RGBA. Good.\n");
                }
                break;
                //TODO(jerry): less than true color?
                case 16:
                case 8:
                {
//                  fprintf(stderr, "16 bit and 8 bit color? I'm not sure what'll happen\n");
                }
                break;
            }

            switch( bmp_info.compression_type ){
                case BMP_RGB:
                {
//                  fprintf(stderr, "Uncompressed image found. Good.\n");
                }
                break;
                //TODO(jerry): Compression maybe? Eh
                case BMP_RLE8:
                case BMP_RLE4:
                case BMP_BITFIELDS:
                case BMP_JPEG:
                case BMP_PNG:
                case BMP_ALPHABITFIELDS:
                case BMP_CMYK:
                case BMP_CMYKRLE8:
                case BMP_CMYKRLE4:
                default:
                {
//                  fprintf(stderr, "A form of compression has been found...\n'");
                }
                break;
            }

            u8* pixels_start = bitmap_file_buffer_start + file_head.bmp_pixel_offset;
            u16 pixel_pitch = (bmp_info.bits_per_pixel) / 8;

            bmp_image->pixels = memory_allocate( 4 *
                                        bmp_info.bmp_width *
                                        bmp_info.bmp_height );

            bmp_image->width = bmp_info.bmp_width;
            bmp_image->height = bmp_info.bmp_height;

            // TODO(jerry): Handle that case.
            b32 y_inverted = (bmp_info.bmp_height < 0);

            u32 TRANSPARENCY_COLOR;

            // I think this is the only place I'll ever check for endianness.
            if( get_system_endianness() == SYSTEM_LITTLE_ENDIAN ){
                TRANSPARENCY_COLOR = 0xFFFF00FF;
            }else{
                TRANSPARENCY_COLOR = 0xFF00FFFF;
            }

            for( u64 y = 0; y < bmp_image->height; ++y ){
                for( u64 x = 0; x < bmp_image->width; ++x ){
                    u64 pixel_index = (y * (bmp_image->width*4)) + (x*4);
                    u64 bmp_pixel_index = (y * (bmp_image->width*3)) + (x*3);

                    bmp_image->pixels[pixel_index+0] = pixels_start[bmp_pixel_index+2];
                    bmp_image->pixels[pixel_index+1] = pixels_start[bmp_pixel_index+1];
                    bmp_image->pixels[pixel_index+2] = pixels_start[bmp_pixel_index+0];

                    bmp_image->pixels[pixel_index+3] = 255;

                    // NOTE(jerry):
                    // since this is load time it probably doesn't matter yet,
                    // I could technically make the pixels be represented by this union.
                    union{
                        u32 pixel_val;
                        u8 rgba[4];
                    }pixel_punt;

                    pixel_punt.rgba[0] = pixels_start[bmp_pixel_index+2]; 
                    pixel_punt.rgba[1] = pixels_start[bmp_pixel_index+1]; 
                    pixel_punt.rgba[2] = pixels_start[bmp_pixel_index+0]; 
                    pixel_punt.rgba[3] = 255;
                    
#define TRANSPARENCY_METHOD_B
#ifdef TRANSPARENCY_METHOD_A
                    if( (pixel_punt.pixel_val) == TRANSPARENCY_COLOR ){
                        bmp_image->pixels[pixel_index+3] = 0;
                    }
#else
                    if( pixel_punt.rgba[0] == 255 &&
                        pixel_punt.rgba[1] == 0   &&
                        pixel_punt.rgba[2] == 255 &&
                        pixel_punt.rgba[3] == 255){
                        bmp_image->pixels[pixel_index+3] = 0;
                    }
#endif
                }
            }
        }else{
            fprintf(stderr, "This is not a BMP file\n");
        }
    }

    //TODO(jerry): Maybe handle the color table.

    memory_deallocate(bitmap_file_buffer_start);
    return bmp_image;
}

bitmap_image* load_image( const char* file_name ){
    char* is_bmp_extension = strstr( file_name, ".bmp" );

    // I've been caught by this way too many times...
    if (!does_file_exist(file_name)) {
        fprintf(stderr, "File %s might not exist!\n", file_name);
    }

    if( is_bmp_extension ){
        return load_bitmap_image( file_name );
    }else{
        bitmap_image* bmp_image = memory_allocate(sizeof(bitmap_image));

        int components_per_pixel;

        u8* file_bytes = load_file_into_buffer( file_name );
        u64 file_size = get_file_size( file_name );

        printf("%d, components per pixel?\n", components_per_pixel);

        stbi_set_flip_vertically_on_load(1);
        bmp_image->pixels = stbi_load_from_memory( 
                file_bytes, file_size,
                &bmp_image->width, 
                &bmp_image->height, 
                &components_per_pixel, 4 );

        memory_deallocate( file_bytes );
        return bmp_image;
    }
}

void free_bitmap_image( bitmap_image* bmp ){
    if( bmp ){
        memory_deallocate( bmp->pixels );
        memory_deallocate( bmp );

        bmp = NULL;
    }
}
