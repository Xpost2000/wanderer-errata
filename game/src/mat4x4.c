#include "mat4x4.h"
/*NOTE(jerry): mat4x4.c*/
// for 4x4 matrices
// might be problematic but for now it works so eh.
void build_identity_matrix( f32* matrix ){
    matrix[0] = 1;
    matrix[1] = 0;
    matrix[2] = 0;
    matrix[3] = 0;

    matrix[4] = 0;
    matrix[5] = 1;
    matrix[6] = 0;
    matrix[7] = 0;

    matrix[8] = 0;
    matrix[9] = 0;
    matrix[10] = 1;
    matrix[11] = 0;

    matrix[12] = 0;
    matrix[13] = 0;
    matrix[14] = 0;
    matrix[15] = 1;
}

void build_scale_matrix( f32* matrix,
                         f32 scale_x,
                         f32 scale_y,
                         f32 scale_z){
    matrix[0] = scale_x;
    matrix[5] = scale_y;
    matrix[10] = scale_z;
}

void build_translation_matrix( f32* matrix,
                               f32 translation_x,
                               f32 translation_y,
                               f32 translation_z){
    matrix[12] = translation_x;
    matrix[13] = translation_y;
    matrix[14] = translation_z;
}

void build_orthographic_matrix( f32* matrix,
                                f32 left,
                                f32 top,
                                f32 right,
                                f32 bottom,
                                f32 near_plane,
                                f32 far_plane){
    f32 scale_x = (2.0f/(right-left));
    f32 scale_y = (2.0f/(top-bottom));
    f32 scale_z = (-2.0f/(far_plane-near_plane));

    f32 translate_x = -((right+left)/(right-left));
    f32 translate_y = -((top+bottom)/(top-bottom));
    f32 translate_z = -((far_plane+near_plane)/(far_plane-near_plane));

    build_scale_matrix( matrix, scale_x, scale_y, scale_z );
    build_translation_matrix( matrix, translate_x, translate_y, translate_z );
}
/*NOTE(jerry): end of mat4x4.c*/
