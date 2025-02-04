/*   
 * Copyright (C) 2003 Robert Kooima
 *
 * NEVERBALL is  free software; you can redistribute  it and/or modify
 * it under the  terms of the GNU General  Public License as published
 * by the Free  Software Foundation; either version 2  of the License,
 * or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT  ANY  WARRANTY;  without   even  the  implied  warranty  of
 * MERCHANTABILITY or  FITNESS FOR A PARTICULAR PURPOSE.   See the GNU
 * General Public License for more details.
 */

#ifndef VEC_H
#define VEC_H

#include <math.h>
#include <nds/arm9/math.h>

#define V_PI 3.1415927f

#define V_RAD(d) (d * V_PI / 180.f)
#define V_DEG(r) (r * 180.f / V_PI)

#define fsinf(a)      (sinf(a))
#define fcosf(a)      (cosf(a))
#define ftanf(a)      (tanf(a))
//#define fabsf(a)      ((float) fabs((double) a))
#define fsqrtf(a)     (sqrtf(a))
//#define fmodf(x,y)    ((float) fmod((double) x, (double) y))
#define fatan2f(x, y) (atan2f(x, y))

/*---------------------------------------------------------------------------*/

#define vf_dot(u, v)  (mulf32(u[0], v[0]) + mulf32(u[1], v[1]) + mulf32(u[2], v[2]))
#define vf_len(u)     sqrtf32(vf_dot(u, u))

#define vf_cpy(u, v) { \
    (u)[0] = (v)[0];  \
    (u)[1] = (v)[1];  \
    (u)[2] = (v)[2];  \
}

#define vf_inv(u, v) { \
    (u)[0] = -(v)[0]; \
    (u)[1] = -(v)[1]; \
    (u)[2] = -(v)[2]; \
}

#define vf_scl(u, v, k) {   \
    (u)[0] = (v)[0] * (k); \
    (u)[1] = (v)[1] * (k); \
    (u)[2] = (v)[2] * (k); \
}

#define vf_add(u, v, w) {      \
    (u)[0] = (v)[0] + (w)[0]; \
    (u)[1] = (v)[1] + (w)[1]; \
    (u)[2] = (v)[2] + (w)[2]; \
}

#define vf_sub(u, v, w) {      \
    (u)[0] = (v)[0] - (w)[0]; \
    (u)[1] = (v)[1] - (w)[1]; \
    (u)[2] = (v)[2] - (w)[2]; \
}

#define vf_mid(u, v, w) {              \
    (u)[0] = divf32((v)[0] + (w)[0], inttof32(2)); \
    (u)[1] = divf32((v)[1] + (w)[1], inttof32(2)); \
    (u)[2] = divf32((v)[2] + (w)[2], inttof32(2)); \
}

#define vf_mad(u, p, v, t) {         \
    (u)[0] = (p)[0] + (v)[0] * (t); \
    (u)[1] = (p)[1] + (v)[1] * (t); \
    (u)[2] = (p)[2] + (v)[2] * (t); \
}

/*---------------------------------------------------------------------------*/


void   vf_nrm(int *, const int *);
void   vf_crs(int *, const int *, const int *);

void   mf_cpy(int *, const int *);
void   mf_xps(int *, const int *);
int    mf_inv(int *, const int *);

void   mf_ident(int *);
void   mf_basis(int *, const int *,
                        const int *,
                        const int *);

void   mf_xlt(int *, const int *);
void   mf_scl(int *, const int *);
void   mf_rot(int *, const int *, int);

void   mf_mult(int *, const int *, const int *);
void   mf_pxfm(int *, const int *, const int *);
void   mf_vxfm(int *, const int *, const int *);

#endif
