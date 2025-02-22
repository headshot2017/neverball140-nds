/*   
 * Copyright (C) 2003 Robert Kooima
 *
 * NEVERPUTT is  free software; you can redistribute  it and/or modify
 * it under the  terms of the GNU General  Public License as published
 * by the Free  Software Foundation; either version 2  of the License,
 * or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT  ANY  WARRANTY;  without   even  the  implied  warranty  of
 * MERCHANTABILITY or  FITNESS FOR A PARTICULAR PURPOSE.   See the GNU
 * General Public License for more details.
 */

#include <math.h>

#include "glext.h"
#include "game.h"
#include "vec3.h"
#include "geom.h"
#include "back.h"
#include "hole.h"
#include "hud.h"
#include "image.h"
#include "audio.h"
#include "solid.h"
#include "config.h"

/*---------------------------------------------------------------------------*/

static struct s_file file;
static short         ball;

static float view_a;                    /* Ideal view rotation about Y axis  */
static float view_m;
static float view_ry;                   /* Angular velocity about Y axis     */
static float view_dy;                   /* Ideal view distance above ball    */
static float view_dz;                   /* Ideal view distance behind ball   */

static float view_c[3];                 /* Current view center               */
static float view_v[3];                 /* Current view vector               */
static float view_p[3];                 /* Current view position             */
static float view_e[3][3];              /* Current view orientation          */

static int   swch_e = 1;                /* Switching enabled flag            */
static float jump_e = 1;                /* Jumping enabled flag              */
static float jump_b = 0;                /* Jump-in-progress flag             */
static float jump_dt;                   /* Jump duration                     */
static float jump_p[3];                 /* Jump destination                  */

/*---------------------------------------------------------------------------*/

static void view_init(void)
{
    view_a  = 0.f;
    view_m  = 0.f;
    view_ry = 0.f;
    view_dy = 3.f;
    view_dz = 5.f;

    view_c[0] = 0.f;
    view_c[1] = 0.f;
    view_c[2] = 0.f;

    view_p[0] =     0.f;
    view_p[1] = view_dy;
    view_p[2] = view_dz;

    view_e[0][0] = 1.f;
    view_e[0][1] = 0.f;
    view_e[0][2] = 0.f;
    view_e[1][0] = 0.f;
    view_e[1][1] = 1.f;
    view_e[1][2] = 0.f;
    view_e[2][0] = 0.f;
    view_e[2][1] = 0.f;
    view_e[2][2] = 1.f;
}

void putt_game_init(const char *s)
{
    jump_e = 1;
    jump_b = 0;

    view_init();
    sol_load(&file, config_data(s), config_get_d(CONFIG_TEXTURES),
                                    config_get_d(CONFIG_SHADOW));
}

void putt_game_free(void)
{
    sol_free(&file);
}

/*---------------------------------------------------------------------------*/

static void putt_game_draw_vect_prim(const struct s_file *fp, int mode)
{
    float p[3];
    float x[3];
    float z[3];
    float r;

    v_cpy(p, fp->uv[ball].p);
    v_cpy(x, view_e[0]);
    v_cpy(z, view_e[2]);

    r = fp->uv[ball].r;

    glPolyFmt(POLY_ALPHA(16) | POLY_CULL_BACK);
    glBegin(mode);
    {
        //glColor4f(1.0f, 1.0f, 0.5f, 0.5f);
        glColor3b(255, 255, 128);
        glVertex3f((p[0] - x[0] * r) / SCALE_VERTICES,
                   (p[1] - x[1] * r) / SCALE_VERTICES,
                   (p[2] - x[2] * r) / SCALE_VERTICES);

        //glColor4f(1.0f, 0.0f, 0.0f, 0.5f);
        glColor3b(255, 0, 0);
        glVertex3f((p[0] + z[0] * view_m) / SCALE_VERTICES,
                   (p[1] + z[1] * view_m) / SCALE_VERTICES,
                   (p[2] + z[2] * view_m) / SCALE_VERTICES);

        //glColor4f(1.0f, 1.0f, 0.0f, 0.5f);
        glColor3b(255, 255, 0);
        glVertex3f((p[0] + x[0] * r) / SCALE_VERTICES,
                   (p[1] + x[1] * r) / SCALE_VERTICES,
                   (p[2] + x[2] * r) / SCALE_VERTICES);
    }
    glEnd();
    glPolyFmt(POLY_ALPHA(31) | POLY_CULL_BACK);
}

static void putt_game_draw_vect(const struct s_file *fp)
{
    if (view_m > 0.f)
    {
        //glPushAttrib(GL_TEXTURE_BIT);
        //glPushAttrib(GL_POLYGON_BIT);
        //glPushAttrib(GL_LIGHTING_BIT);
        //glPushAttrib(GL_DEPTH_BUFFER_BIT);
        {
            //glEnable(GL_COLOR_MATERIAL);
            //glDisable(GL_LIGHTING);
            //glDisable(GL_TEXTURE_2D);
            //glDepthMask(GL_FALSE);
            glBindTexture(0, 0);

            //glEnable(GL_DEPTH_TEST);
            putt_game_draw_vect_prim(fp, GL_TRIANGLES);

            //glDisable(GL_DEPTH_TEST);
            //putt_game_draw_vect_prim(fp, GL_LINE_STRIP);
        }
        //glPopAttrib();
        //glPopAttrib();
        //glPopAttrib();
        //glPopAttrib();
    }
}

static void putt_game_draw_balls(const struct s_file *fp)
{
    /*
    static const GLfloat color[5][4] = {
        { 1.0f, 1.0f, 1.0f, 0.7f },
        { 1.0f, 0.0f, 0.0f, 1.0f },
        { 0.0f, 1.0f, 0.0f, 1.0f },
        { 0.0f, 0.0f, 1.0f, 1.0f },
        { 1.0f, 1.0f, 0.0f, 1.0f },
    };
    */
    static const uint8_t color[5][4] = {
        { 255, 255, 255, 179 },
        { 255, 0,   0,   255 },
        { 0,   255, 0,   255 },
        { 0,   0,   255, 255 },
        { 255, 255, 0,   255 },
    };

    float M[16];
    m4x4 Mf32;
    int ui;

    for (ui = curr_party(); ui > 0; ui--)
    {
        if (ui == ball)
        {
            glPushMatrix();
            {
                m_basis(M, fp->uv[ui].e[0], fp->uv[ui].e[1], fp->uv[ui].e[2]);
                for (int i=0; i<16; i++)
                    Mf32.m[i] = floattof32(M[i]);

                glTranslatef(fp->uv[ui].p[0] / SCALE_VERTICES,
                             (fp->uv[ui].p[1] + BALL_FUDGE) / SCALE_VERTICES,
                             fp->uv[ui].p[2] / SCALE_VERTICES);
                glMultMatrix4x4(&Mf32);
                glScalef(fp->uv[ui].r,
                         fp->uv[ui].r,
                         fp->uv[ui].r);

                //glColor4fv(color[ui]);
                ball_set_color(color[ui][0], color[ui][1], color[ui][2]);

                ball_draw();
            }
            glPopMatrix(1);
        }
        else
        {
            glPushMatrix();
            {
                glTranslatef(fp->uv[ui].p[0] / SCALE_VERTICES,
                             (fp->uv[ui].p[1] - fp->uv[ui].r + BALL_FUDGE) / SCALE_VERTICES,
                             fp->uv[ui].p[2] / SCALE_VERTICES);
                glScalef(fp->uv[ui].r,
                         fp->uv[ui].r,
                         fp->uv[ui].r);

                //glColor4f(color[ui][0],
                //          color[ui][1],
                //          color[ui][2], 0.5f);
                glColor3b(color[ui][0], color[ui][1], color[ui][2]);

                mark_draw();
            }
            glPopMatrix(1);
        }
    }
}

static void putt_game_draw_goals(const struct s_file *fp, float rx, float ry)
{
    int zi;

    for (zi = 0; zi < fp->zc; zi++)
    {
        glPushMatrix();
        {
            glTranslatef(fp->zv[zi].p[0] / SCALE_VERTICES,
                         fp->zv[zi].p[1] / SCALE_VERTICES,
                         fp->zv[zi].p[2] / SCALE_VERTICES);
            flag_draw();
        }
        glPopMatrix(1);
    }
}

static void putt_game_draw_jumps(const struct s_file *fp)
{
    int ji;

    for (ji = 0; ji < fp->jc; ji++)
    {
        glPushMatrix();
        {
            glTranslatef(fp->jv[ji].p[0] / SCALE_VERTICES,
                         fp->jv[ji].p[1] / SCALE_VERTICES,
                         fp->jv[ji].p[2] / SCALE_VERTICES);

            glScalef(fp->jv[ji].r, 1.f, fp->jv[ji].r);
            jump_draw();
        }
        glPopMatrix(1);
    }
}

static void putt_game_draw_swchs(const struct s_file *fp)
{
    int xi;

    for (xi = 0; xi < fp->xc; xi++)
    {
        glPushMatrix();
        {
            glTranslatef(fp->xv[xi].p[0] / SCALE_VERTICES,
                         fp->xv[xi].p[1] / SCALE_VERTICES,
                         fp->xv[xi].p[2] / SCALE_VERTICES);

            glScalef(fp->xv[xi].r, 1.f, fp->xv[xi].r);
            swch_draw(fp->xv[xi].f);
        }
        glPopMatrix(1);
    }
}

/*---------------------------------------------------------------------------*/

void putt_game_draw(int pose)
{
    //const float light_p[4] = { 8.f, 32.f, 8.f, 1.f };

    const struct s_file *fp = &file;
    
    float fov = FOV;

    if (jump_b) fov *= 2.0f * fabsf(jump_dt - 0.5f);

    config_push_persp(fov, 0.1f, FAR_DIST);
    //glPushAttrib(GL_LIGHTING_BIT);
    glPushMatrix();
    {
        float v[3], rx, ry;

        v_sub(v, view_c, view_p);

        rx = V_DEG(fatan2f(-v[1], fsqrtf(v[0] * v[0] + v[2] * v[2])));
        ry = V_DEG(fatan2f(+v[0], -v[2]));

        glTranslatef(0.f, 0.f, -v_len(v) / SCALE_VERTICES);
        glRotatef(rx, 1.f, 0.f, 0.f);
        glRotatef(ry, 0.f, 1.f, 0.f);
        glTranslatef(-view_c[0] / SCALE_VERTICES, -view_c[1] / SCALE_VERTICES, -view_c[2] / SCALE_VERTICES);

        /* Center the skybox about the position of the camera. */

        glPushMatrix();
        {
            glTranslatef(view_p[0] / SCALE_VERTICES, view_p[1] / SCALE_VERTICES, view_p[2] / SCALE_VERTICES);
            back_draw(0);
        }
        glPopMatrix(1);

        //glEnable(GL_LIGHT0);
        //glLightfv(GL_LIGHT0, GL_POSITION, light_p);

        /* Draw the floor. */

        sol_draw(fp);

        if (config_get_d(CONFIG_SHADOW) && !pose)
        {
            shad_draw_set(fp->uv[ball].p, fp->uv[ball].r);
            sol_shad(fp);
            shad_draw_clr();
        }

        /* Draw the game elements. */

        glEnable(GL_BLEND);
        //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        if (pose == 0)
        {
            putt_game_draw_balls(fp);
            putt_game_draw_vect(fp);
        }

        putt_game_draw_goals(fp, -rx, -ry);
        putt_game_draw_jumps(fp);
        putt_game_draw_swchs(fp);
    }
    glPopMatrix(1);
    //glPopAttrib();
    config_pop_matrix();
}

/*---------------------------------------------------------------------------*/

void putt_game_update_view(float dt)
{
    const float y[3] = { 0.f, 1.f, 0.f };

    float dy;
    float dz;
    float k;
    float e[3];
    float d[3];
    float s = 2.f * dt;

    /* Center the view about the ball. */

    v_cpy(view_c, file.uv[ball].p);
    v_inv(view_v, file.uv[ball].v);

    switch (config_get_d(CONFIG_CAMERA))
    {
    case 2:
        /* Camera 2: View vector is given by view angle. */

        view_e[2][0] = fsinf(V_RAD(view_a));
        view_e[2][1] = 0.f;
        view_e[2][2] = fcosf(V_RAD(view_a));

        s = 1.f;
        break;

    default:
        /* View vector approaches the ball velocity vector. */

        v_mad(e, view_v, y, v_dot(view_v, y));
        v_inv(e, e);

        k = v_dot(view_v, view_v);

        v_sub(view_e[2], view_p, view_c);
        v_mad(view_e[2], view_e[2], view_v, k * dt * 0.1f);
    }

    /* Orthonormalize the basis of the view in its new position. */

    v_crs(view_e[0], view_e[1], view_e[2]);
    v_crs(view_e[2], view_e[0], view_e[1]);
    v_nrm(view_e[0], view_e[0]);
    v_nrm(view_e[2], view_e[2]);

    /* The current view (dy, dz) approaches the ideal (view_dy, view_dz). */

    v_sub(d, view_p, view_c);

    dy = v_dot(view_e[1], d);
    dz = v_dot(view_e[2], d);

    dy += (view_dy - dy) * s;
    dz += (view_dz - dz) * s;

    /* Compute the new view position. */

    view_p[0] = view_p[1] = view_p[2] = 0.f;

    v_mad(view_p, view_c, view_e[1], dy);
    v_mad(view_p, view_p, view_e[2], dz);

    view_a = V_DEG(fatan2f(view_e[2][0], view_e[2][2]));
}

static int putt_game_update_state(float dt)
{
    static float t = 0.f;

    struct s_file *fp = &file;
    float p[3];
    int e = swch_e;

    if (dt > 0.f)
        t += dt;
    else
        t = 0.f;

    /* Test for a switch. */

    if ((swch_e = sol_swch_test(fp, swch_e, ball)) != e && e)
        audio_play(AUD_SWITCH, 1.f);

    /* Test for a jump. */

    if (jump_e == 1 && jump_b == 0 && sol_jump_test(fp, jump_p, ball) == 1)
    {
        jump_b  = 1;
        jump_e  = 0;
        jump_dt = 0.f;
        
        audio_play(AUD_JUMP, 1.f);
    }
    if (jump_e == 0 && jump_b == 0 &&  sol_jump_test(fp, jump_p, ball) == 0)
        jump_e = 1;

    /* Test for fall-out. */

    if (fp->uv[ball].p[1] < -10.f)
        return GAME_FALL;

    /* Test for a goal or stop. */

    if (t > 1.f)
    {
        t = 0.f;

        if (sol_goal_test(fp, p, ball))
            return GAME_GOAL;
        else
            return GAME_STOP;
    }

    return GAME_NONE;
}

/*
 * On  most  hardware, rendering  requires  much  more  computing power  than
 * physics.  Since  physics takes less time  than graphics, it  make sense to
 * detach  the physics update  time step  from the  graphics frame  rate.  By
 * performing multiple physics updates for  each graphics update, we get away
 * with higher quality physics with little impact on overall performance.
 *
 * Toward this  end, we establish a  baseline maximum physics  time step.  If
 * the measured  frame time  exceeds this  maximum, we cut  the time  step in
 * half, and  do two updates.  If THIS  time step exceeds the  maximum, we do
 * four updates.  And  so on.  In this way, the physics  system is allowed to
 * seek an optimal update rate independant of, yet in integral sync with, the
 * graphics frame rate.
 */

int putt_game_step(const float g[3], float dt)
{
    struct s_file *fp = &file;

    static float s = 0.f;
    static float t = 0.f;

    float d = 0.f;
    float b = 0.f;
    float st = 0.f;
    int i, n = 1, m = 0;

    s = (7.f * s + dt) / 8.f;
    t = s;

    if (jump_b)
    {
        jump_dt += dt;

        /* Handle a jump. */

        if (0.5 < jump_dt)
        {
            fp->uv[ball].p[0] = jump_p[0];
            fp->uv[ball].p[1] = jump_p[1];
            fp->uv[ball].p[2] = jump_p[2];
        }
        if (1.f < jump_dt)
            jump_b = 0;
    }
    else
    {
        /* Run the sim. */

        while (t > MAX_DT && n < MAX_DN)
        {
            t /= 2;
            n *= 2;
        }

        for (i = 0; i < n; i++)
        {
            d = sol_step(fp, g, t, ball, &m);
            
            if (b < d)
                b = d;
            if (m)
                st += t;
        }

        /* Mix the sound of a ball bounce. */

        if (b > 0.5)
            audio_play(AUD_BUMP, (float) (b - 0.5) * 2.0f);
    }

    putt_game_update_view(dt);
    return putt_game_update_state(st);
}

void putt_game_putt(void)
{
    /*
     * HACK: The BALL_FUDGE here  guarantees that a putt doesn't drive
     * the ball  too directly down  toward a lump,  triggering rolling
     * friction too early and stopping the ball prematurely.
     */

    file.uv[ball].v[0] = -4.f * view_e[2][0] * view_m;
    file.uv[ball].v[1] = -4.f * view_e[2][1] * view_m + BALL_FUDGE;
    file.uv[ball].v[2] = -4.f * view_e[2][2] * view_m;

    view_m = 0.f;
}

/*---------------------------------------------------------------------------*/

void putt_game_set_rot(int d)
{
    view_a += (float) (30.f * d) / config_get_d(CONFIG_MOUSE_SENSE);
}

void putt_game_clr_mag(void)
{
    view_m = 1.f;
}

void putt_game_set_mag(int d)
{
    view_m -= (float) (1.f * d) / config_get_d(CONFIG_MOUSE_SENSE);

    if (view_m < 0.25)
        view_m = 0.25;
}

void putt_game_set_fly(float k)
{
    struct s_file *fp = &file;

    float  x[3] = { 1.f, 0.f, 0.f };
    float  y[3] = { 0.f, 1.f, 0.f };
    float  z[3] = { 0.f, 0.f, 1.f };
    float c0[3] = { 0.f, 0.f, 0.f };
    float p0[3] = { 0.f, 0.f, 0.f };
    float c1[3] = { 0.f, 0.f, 0.f };
    float p1[3] = { 0.f, 0.f, 0.f };
    float  v[3];

    v_cpy(view_e[0], x);
    v_cpy(view_e[1], y);
    v_sub(view_e[2], fp->uv[ball].p, fp->zv[0].p);

    if (fabsf(v_dot(view_e[1], view_e[2])) > 0.999f)
        v_cpy(view_e[2], z);

    v_crs(view_e[0], view_e[1], view_e[2]);
    v_crs(view_e[2], view_e[0], view_e[1]);

    v_nrm(view_e[0], view_e[0]);
    v_nrm(view_e[2], view_e[2]);

    /* k = 0.0 view is at the ball. */

    if (fp->uc > 0)
    {
        v_cpy(c0, fp->uv[ball].p);
        v_cpy(p0, fp->uv[ball].p);
    }

    v_mad(p0, p0, view_e[1], view_dy);
    v_mad(p0, p0, view_e[2], view_dz);

    /* k = +1.0 view is s_view 0 */

    if (k >= 0 && fp->wc > 0)
    {
        v_cpy(p1, fp->wv[0].p);
        v_cpy(c1, fp->wv[0].q);
    }

    /* k = -1.0 view is s_view 1 */

    if (k <= 0 && fp->wc > 1)
    {
        v_cpy(p1, fp->wv[1].p);
        v_cpy(c1, fp->wv[1].q);
    }

    /* Interpolate the views. */

    v_sub(v, p1, p0);
    v_mad(view_p, p0, v, k * k);

    v_sub(v, c1, c0);
    v_mad(view_c, c0, v, k * k);

    /* Orthonormalize the view basis. */

    v_sub(view_e[2], view_p, view_c);
    v_crs(view_e[0], view_e[1], view_e[2]);
    v_crs(view_e[2], view_e[0], view_e[1]);
    v_nrm(view_e[0], view_e[0]);
    v_nrm(view_e[2], view_e[2]);

    view_a = V_DEG(fatan2f(view_e[2][0], view_e[2][2]));
}

void putt_game_ball(int i)
{
    int ui;

    ball = i;

    jump_e = 1;
    jump_b = 0;

    for (ui = 0; ui < file.uc; ui++)
    {
        file.uv[ui].v[0] = 0.f;
        file.uv[ui].v[1] = 0.f;
        file.uv[ui].v[2] = 0.f;

        file.uv[ui].w[0] = 0.f;
        file.uv[ui].w[1] = 0.f;
        file.uv[ui].w[2] = 0.f;
    }
}

void putt_game_get_pos(float p[3], float e[3][3])
{
    v_cpy(p,    file.uv[ball].p);
    v_cpy(e[0], file.uv[ball].e[0]);
    v_cpy(e[1], file.uv[ball].e[1]);
    v_cpy(e[2], file.uv[ball].e[2]);
}

void putt_game_set_pos(float p[3], float e[3][3])
{
    v_cpy(file.uv[ball].p,    p);
    v_cpy(file.uv[ball].e[0], e[0]);
    v_cpy(file.uv[ball].e[1], e[1]);
    v_cpy(file.uv[ball].e[2], e[2]);
}

/*---------------------------------------------------------------------------*/

