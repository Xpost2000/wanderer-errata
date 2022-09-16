#ifndef MAT4_4_H
#define MAT4_4_H

#include "common.h"

void build_identity_matrix( f32* matrix );

void build_scale_matrix( f32* matrix,
                         f32 scale_x,
                         f32 scale_y,
                         f32 scale_z);

void build_translation_matrix( f32* matrix,
                               f32 translation_x,
                               f32 translation_y,
                               f32 translation_z);

void build_orthographic_matrix( f32* matrix,
                                f32 left,
                                f32 top,
                                f32 right,
                                f32 bottom,
                                f32 near_plane,
                                f32 far_plane);
/*NOTE(jerry): end of mat4x4.c*/

#endif
