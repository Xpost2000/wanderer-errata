#ifndef COMMON_H
#define COMMON_H

/*
  Typedefs and some convenience macros?
  and other commonly included headers and stuff.
*/
#include <stdalign.h>

#include <time.h>

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <stdint.h>
#include <stdbool.h>

#include <math.h>
#include <string.h>

#define STRINGIFY(x) #x
#define ARRAY_COUNT(x) (sizeof(x) / sizeof(x[0]))

#define KB( sz ) ((sz) * 1024LL)
#define MB( sz ) (KB(sz) * 1024LL)
#define GB( sz ) (MB(sz) * 1024LL)
#define TB( sz ) (GB(sz) * 1024LL)

#define BIT( bit ) (1 << bit)

#ifdef DEBUG_BUILD
#define stub_important_code( stub_msg )				\
	do{												\
		fprintf(stderr, "TODO:%s:%d: STUB: %s\n", __FILE__, __LINE__, stub_msg); \
        abort(); \
	}while(0)	
#ifndef SUPPRESS_STUBS
// meant to replace comments
// and catch my attention by crashing.
#define stub_less_important( stub_msg )					\
	do{													\
		fprintf(stderr, "NOTE:%s:%d: STUB: %s\n", __FILE__, __LINE__, stub_msg); \
	}while(0)
#else
#define stub_less_important( stub_msg )
#endif
#else
#define stub_less_important( stub_msg )
#define stub_important_code( stub_msg )
#endif

typedef int64_t i64;
typedef uint64_t u64;
typedef int32_t i32;
typedef uint32_t u32;
typedef int16_t i16;
typedef uint16_t u16;
typedef int8_t i8;
typedef uint8_t u8;

typedef u8 byte;

typedef int32_t b32;

typedef float f32;
typedef double f64;

/*NOTE(jerry): endianess*/
typedef enum system_endianness{
    SYSTEM_LITTLE_ENDIAN = 1,
    SYSTEM_BIG_ENDIAN = 2
}system_endianness;

static u8 get_system_endianness( void ){
    union{
        u32 word;
        u8 word_bytes[4];
    }endian_union;

    endian_union.word = 1;

    if( endian_union.word_bytes[0] == 1 ){
        return SYSTEM_LITTLE_ENDIAN;
    }else{
        return SYSTEM_BIG_ENDIAN;
    }
}
/*end of endianess*/

// TODO(jerry): use C11 generics?
static i32 i32_min( i32 a, i32 b ){
    if( a < b ){
        return a;
    }else{
        return b;
    }
}

static i32 i32_max( i32 a, i32 b ){
    if( a > b ){
        return a;
    }else{
        return b;
    }
}

static u32 u32_min( u32 a, u32 b ){
    if( a < b ){
        return a;
    }else{
        return b;
    }
}

static u32 u32_max( u32 a, u32 b ){
    if( a > b ){
        return a;
    }else{
        return b;
    }
}

static f32 f32_min( f32 a, f32 b ){
    if( a < b ){
        return a;
    }else{
        return b;
    }
}

static f32 f32_max( f32 a, f32 b ){
    if( a > b ){
        return a;
    }else{
        return b;
    }
}

static f32 f32_clamp( f32 value, f32 a, f32 b ){
    return f32_min( f32_max( value, a ), b );
}

static i32 i32_clamp( i32 value, i32 a, i32 b ){
    return i32_min( i32_max( value, a ), b );
}

static u32 u32_clamp( i32 value, i32 a, i32 b ){
    return u32_min( u32_max( value, a ), b );
}
/*
 * I should probably not be using
 * rand, and probably use a better either
 * higher quality random or a more deterministic
 * random...
 */
static void seed_random(u64 seed) {
    srand(seed);
}

static const i32 random_integer( void ){
    return rand();
}

static const i32 random_integer_ranged( const i32 min, const i32 max ){
    if (min > max) { 
        return 0;
    } else if ((min - max) == 0) {
        return max;
    } else {
        return (random_integer() % (max-min)) + min;
    }
}

#define M_PI 3.141592654

static f32 degrees_to_radians( f32 degrees ){
    return degrees * (M_PI / 180.0f);
}

static f32 radians_to_degrees( f32 radians ){
    return radians * (180.0f / M_PI);
}

static f32 angle_from_direction( f32 dir_x, f32 dir_y ){
    f32 result = radians_to_degrees(atan2(dir_y, dir_x));

    if( result > 0 ){
        result = 360 - result;
    }

    return result;
}

static const f32 wall_thickness_px = 16;
// going to be honest, I don't remember why this worked
// I think it was cause when the game was still orthogonal this was
// the size of everything lol.
// The ingame grid might internally be 128 pixels...
// which is a really bad decision in hindsight.
static const f32 tile_px_size = 128;
static const f32 iso_tile_px_size = 32;

#endif
