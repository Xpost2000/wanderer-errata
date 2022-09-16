#ifndef COLOR_H
#define COLOR_H

// TODO(jerry): not used yet.
typedef struct color{
    union{
        struct {
            u8 r;
            u8 g;
            u8 b;
            u8 a;
        };
    };
}color;

typedef struct colorf{
    union{
        struct {
            f32 r;
            f32 g;
            f32 b;
            f32 a;
        };
    };
}colorf;

#endif
