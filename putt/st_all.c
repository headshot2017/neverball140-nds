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

#include "hud.h"
#include "back.h"
#include "geom.h"
#include "gui.h"
#include "vec3.h"
#include "game.h"
#include "hole.h"
#include "audio.h"
#include "course.h"
#include "config.h"

#include "st_all.h"
#include "st_conf.h"

/*---------------------------------------------------------------------------*/

static char *number(int i)
{
    static char str[MAXSTR];

    sprintf(str, "%02d", i);

    return str;
}

static int score_card(const char  *title,
                      const float *c0,
                      const float *c1)
{
    int id, jd, kd, ld;

    int p1 = (curr_party() >= 1) ? 1 : 0, l1 = (curr_party() == 1) ? 1 : 0;
    int p2 = (curr_party() >= 2) ? 1 : 0, l2 = (curr_party() == 2) ? 1 : 0;
    int p3 = (curr_party() >= 3) ? 1 : 0, l3 = (curr_party() == 3) ? 1 : 0;
    int p4 = (curr_party() >= 4) ? 1 : 0, l4 = (curr_party() == 4) ? 1 : 0;

    int i;
    int n = curr_count() - 1;
    int m = curr_count() / 2;

    if ((id = gui_vstack(0)))
    {
        gui_label(id, title, GUI_MED, GUI_ALL, c0, c1);
        gui_space(id);

        if ((jd = gui_hstack(id)))
        {
            if ((kd = gui_varray(jd)))
            { 
                if (p1) gui_label(kd, "O",         0, GUI_NE, 0, 0);
                if (p1) gui_label(kd, hole_out(0), 0, 0,           gui_wht, gui_wht);
                if (p1) gui_label(kd, hole_out(1), 0, GUI_SE * l1, gui_red, gui_wht);
                if (p2) gui_label(kd, hole_out(2), 0, GUI_SE * l2, gui_grn, gui_wht);
                if (p3) gui_label(kd, hole_out(3), 0, GUI_SE * l3, gui_blu, gui_wht);
                if (p4) gui_label(kd, hole_out(4), 0, GUI_SE * l4, gui_yel, gui_wht);
            }

            if ((kd = gui_harray(jd)))
                for (i = m; i > 0; i--)
                    if ((ld = gui_varray(kd)))
                    {
                        if (p1) gui_label(ld, number(i), 0, (i == 1) ? GUI_NW : 0, 0, 0);
                        if (p1) gui_label(ld, hole_score(i, 0), 0, 0, gui_wht, gui_wht);
                        if (p1) gui_label(ld, hole_score(i, 1), 0, 0, gui_red, gui_wht);
                        if (p2) gui_label(ld, hole_score(i, 2), 0, 0, gui_grn, gui_wht);
                        if (p3) gui_label(ld, hole_score(i, 3), 0, 0, gui_blu, gui_wht);
                        if (p4) gui_label(ld, hole_score(i, 4), 0, 0, gui_yel, gui_wht);
                    }
            if ((kd = gui_varray(jd)))
            {
                gui_filler(kd);
                if (p1) gui_label(kd, "Par", 0, GUI_NW,      gui_wht, gui_wht);
                if (p1) gui_label(kd, "P1",  0, GUI_SW * l1, gui_red, gui_wht);
                if (p2) gui_label(kd, "P2",  0, GUI_SW * l2, gui_grn, gui_wht);
                if (p3) gui_label(kd, "P3",  0, GUI_SW * l3, gui_blu, gui_wht);
                if (p4) gui_label(kd, "P4",  0, GUI_SW * l4, gui_yel, gui_wht);
            }
        }

        gui_space(id);

        if ((jd = gui_hstack(id)))
        {
            if ((kd = gui_varray(jd)))
            {
                if (p1) gui_label(kd, "Tot",       0, GUI_TOP, 0, 0);
                if (p1) gui_label(kd, hole_tot(0), 0, 0,           gui_wht, gui_wht);
                if (p1) gui_label(kd, hole_tot(1), 0, GUI_BOT * l1, gui_red, gui_wht);
                if (p2) gui_label(kd, hole_tot(2), 0, GUI_BOT * l2, gui_grn, gui_wht);
                if (p3) gui_label(kd, hole_tot(3), 0, GUI_BOT * l3, gui_blu, gui_wht);
                if (p4) gui_label(kd, hole_tot(4), 0, GUI_BOT * l4, gui_yel, gui_wht);
            }
            if ((kd = gui_varray(jd)))
            {
                if (p1) gui_label(kd, "I",        0, GUI_NE, 0, 0);
                if (p1) gui_label(kd, hole_in(0), 0, 0,           gui_wht, gui_wht);
                if (p1) gui_label(kd, hole_in(1), 0, GUI_SE * l1, gui_red, gui_wht);
                if (p2) gui_label(kd, hole_in(2), 0, GUI_SE * l2, gui_grn, gui_wht);
                if (p3) gui_label(kd, hole_in(3), 0, GUI_SE * l3, gui_blu, gui_wht);
                if (p4) gui_label(kd, hole_in(4), 0, GUI_SE * l4, gui_yel, gui_wht);
            }
            if ((kd = gui_harray(jd)))
                for (i = n; i > m; i--)
                    if ((ld = gui_varray(kd)))
                    {
                        if (p1) gui_label(ld, number(i), 0, (i == m+1) ? GUI_NW : 0, 0, 0);
                        if (p1) gui_label(ld, hole_score(i, 0), 0, 0, gui_wht, gui_wht);
                        if (p1) gui_label(ld, hole_score(i, 1), 0, 0, gui_red, gui_wht);
                        if (p2) gui_label(ld, hole_score(i, 2), 0, 0, gui_grn, gui_wht);
                        if (p3) gui_label(ld, hole_score(i, 3), 0, 0, gui_blu, gui_wht);
                        if (p4) gui_label(ld, hole_score(i, 4), 0, 0, gui_yel, gui_wht);
                    }
            if ((kd = gui_varray(jd)))
            {
                gui_filler(kd);
                if (p1) gui_label(kd, "Par", 0, GUI_NW,      gui_wht, gui_wht);
                if (p1) gui_label(kd, "P1",  0, GUI_SW * l1, gui_red, gui_wht);
                if (p2) gui_label(kd, "P2",  0, GUI_SW * l2, gui_grn, gui_wht);
                if (p3) gui_label(kd, "P3",  0, GUI_SW * l3, gui_blu, gui_wht);
                if (p4) gui_label(kd, "P4",  0, GUI_SW * l4, gui_yel, gui_wht);
            }
        }

        gui_layout(id, 0, 0);
    }

    return id;
}

/*---------------------------------------------------------------------------*/

#define TITLE_PLAY 1
#define TITLE_CONF 2
#define TITLE_EXIT 3

static int title_action(int i)
{
    audio_play(AUD_MENU, 1.0f);

    switch (i)
    {
    case TITLE_PLAY: return goto_state(&st_course);
    case TITLE_CONF: return goto_state(&st_conf);
    case TITLE_EXIT: return 0;
    }
    return 1;
}

static int title_enter(void)
{
    int id, jd, kd;

    /* Build the title GUI. */

    if ((id = gui_vstack(0)))
    {
        gui_label(id, "Neverputt", GUI_LRG, GUI_ALL, 0, 0);
        gui_space(id);

        if ((jd = gui_harray(id)))
        {
            gui_filler(jd);

            if ((kd = gui_varray(jd)))
            {
                gui_start(kd, "Play",    GUI_MED, TITLE_PLAY, 1);
                gui_state(kd, "Options", GUI_MED, TITLE_CONF, 0);
                gui_state(kd, "Exit",    GUI_MED, TITLE_EXIT, 0);
            }

            gui_filler(jd);
        }
        gui_layout(id, 0, 0);
    }

    course_init();
    course_rand();

    return id;
}

static void title_leave(int id)
{
    gui_delete(id);
}

static void title_paint(int id, float st)
{
    game_draw(0);
    gui_paint(id);
}

static void title_timer(int id, float dt)
{
    float g[3] = { 0.f, 0.f, 0.f };

    game_step(g, dt);
    game_set_fly(fcosf(time_state() / 10.f));

    gui_timer(id, dt);
    audio_timer(dt);
}

static void title_point(int id, int x, int y, int dx, int dy)
{
    gui_pulse(gui_point(id, x, y), 1.2f);
}

static int title_click(int b, int d)
{
    return (d && b < 0) ? title_action(gui_token(gui_click())) : 1;
}

static int title_keybd(int c, int d)
{
    return (d && c == SDLK_ESCAPE) ? 0 : 1;
}

/*---------------------------------------------------------------------------*/

static int desc_id;

static int course_action(int i)
{
    if (course_exists(i))
    {
        course_goto(i);
        goto_state(&st_party);
    }
    if (i < 0)
        goto_state(&st_title);

    return 1;
}

static int course_enter(void)
{
    int w = config_get_d(CONFIG_WIDTH);
    int h = config_get_d(CONFIG_HEIGHT);

    int id, jd, i, n = course_count(), m = n + 2;

    if ((id = gui_vstack(0)))
    {
        gui_label(id, "Select Course", GUI_MED, GUI_ALL, 0, 0);
        gui_space(id);

        if ((jd = gui_hstack(id)))
        {
            gui_filler(jd);
            for (i = n - 1; i >= 0; i--)
                gui_active(gui_image(jd, course_shot(i), w / m, h / m), i, 0);
            gui_filler(jd);
        }

        gui_space(id);
        desc_id = gui_multi(id, course_desc(0), GUI_SML, GUI_ALL, gui_yel, gui_wht);
        gui_space(id);

        if ((jd = gui_hstack(id)))
        {
            gui_filler(jd);
            gui_state(jd, " Back ", GUI_SML, -1, 0);
        }

        gui_layout(id, 0, 0);
    }

    audio_music_fade_to(0.5f, "bgm/inter.ogg");

    return id;
}

static void course_leave(int id)
{
    gui_delete(id);
}

static void course_paint(int id, float st)
{
    game_draw(0);
    gui_paint(id);
}

static void course_timer(int id, float dt)
{
    gui_timer(id, dt);
    audio_timer(dt);
}

static void course_point(int id, int x, int y, int dx, int dy)
{
    int jd;

    if ((jd = gui_point(id, x, y)))
    {
        int i = gui_token(jd);

        gui_set_multi(desc_id, course_desc(i));
        gui_pulse(jd, 1.2f);
    }
}

static int course_click(int b, int d)
{
    return (d && b < 0) ? course_action(gui_token(gui_click())) : 1;
}

static int course_keybd(int c, int d)
{
    return (d && c == SDLK_ESCAPE) ? goto_state(&st_title) : 1;
}

/*---------------------------------------------------------------------------*/

#define PARTY_T 0
#define PARTY_1 1
#define PARTY_2 2
#define PARTY_3 3
#define PARTY_4 4
#define PARTY_B 5

static int party_action(int i)
{
    switch (i)
    {
    case PARTY_1:
        audio_play(AUD_MENU, 1.f);
        hole_goto(1, 1);
        goto_state(&st_next);
        break;
    case PARTY_2:
        audio_play(AUD_MENU, 1.f);
        hole_goto(1, 2);
        goto_state(&st_next);
        break;
    case PARTY_3:
        audio_play(AUD_MENU, 1.f);
        hole_goto(1, 3);
        goto_state(&st_next);
        break;
    case PARTY_4:
        audio_play(AUD_MENU, 1.f);
        hole_goto(1, 4);
        goto_state(&st_next);
        break;
    case PARTY_B:
        audio_play(AUD_MENU, 1.f);
        goto_state(&st_course);
        break;
    }
    return 1;
}

static int party_enter(void)
{
    int id, jd;

    if ((id = gui_vstack(0)))
    {
        gui_label(id, "Players?", GUI_MED, GUI_ALL, 0, 0);
        gui_space(id);

        if ((jd = gui_harray(id)))
        {
            int p4 = gui_state(jd, "4", GUI_LRG, PARTY_4, 0);
            int p3 = gui_state(jd, "3", GUI_LRG, PARTY_3, 0);
            int p2 = gui_state(jd, "2", GUI_LRG, PARTY_2, 0);
            int p1 = gui_state(jd, "1", GUI_LRG, PARTY_1, 0);

            gui_set_color(p1, gui_red, gui_wht);
            gui_set_color(p2, gui_grn, gui_wht);
            gui_set_color(p3, gui_blu, gui_wht);
            gui_set_color(p4, gui_yel, gui_wht);
        }

        gui_space(id);

        if ((jd = gui_hstack(id)))
        {
            gui_filler(jd);
            gui_state(jd, " Back ", GUI_SML, PARTY_B, 0);
        }

        gui_layout(id, 0, 0);
    }

    return id;
}

static void party_leave(int id)
{
    gui_delete(id);
}

static void party_paint(int id, float st)
{
    game_draw(0);
    gui_paint(id);
}

static void party_timer(int id, float dt)
{
    gui_timer(id, dt);
    audio_timer(dt);
}

static void party_point(int id, int x, int y, int dx, int dy)
{
    gui_pulse(gui_point(id, x, y), 1.2f);
}

static int party_click(int b, int d)
{
    return (d && b < 0) ? party_action(gui_token(gui_click())) : 1;
}

static int party_keybd(int c, int d)
{
    return (d && c == SDLK_ESCAPE) ? goto_state(&st_course) : 1;
}

/*---------------------------------------------------------------------------*/

static int num = 0;

static int next_enter(void)
{
    int id;
    char str[MAXSTR];

    sprintf(str, "Hole %02d", curr_hole());

    if ((id = gui_vstack(0)))
    {
        gui_label(id, str, GUI_MED, GUI_ALL, 0, 0);
        gui_space(id);

        gui_label(id, "Player", GUI_SML, GUI_TOP, 0, 0);

        switch (curr_player())
        {
        case 1:
            gui_label(id, "1", GUI_LRG, GUI_BOT, gui_red, gui_wht);
            if (curr_party() > 1) audio_play(AUD_PLAYER1, 1.f);
            break;
        case 2:
            gui_label(id, "2", GUI_LRG, GUI_BOT, gui_grn, gui_wht);
            if (curr_party() > 1) audio_play(AUD_PLAYER2, 1.f);
            break;
        case 3:
            gui_label(id, "3", GUI_LRG, GUI_BOT, gui_blu, gui_wht);
            if (curr_party() > 1) audio_play(AUD_PLAYER3, 1.f);
            break;
        case 4:
            gui_label(id, "4", GUI_LRG, GUI_BOT, gui_yel, gui_wht);
            if (curr_party() > 1) audio_play(AUD_PLAYER4, 1.f);
            break;
        }
        gui_layout(id, 0, 0);
    }

    hud_init();
    game_set_fly(1.f);

    return id;
}

static void next_leave(int id)
{
    hud_free();
    gui_delete(id);
}

static void next_paint(int id, float st)
{
    game_draw(0);
    hud_paint();
    gui_paint(id);
}

static void next_timer(int id, float dt)
{
    gui_timer(id, dt);
    audio_timer(dt);
}

static void next_point(int id, int x, int y, int dx, int dy)
{
    gui_pulse(gui_point(id, x, y), 1.2f);
}

static int next_click(int b, int d)
{
    return (d && b < 0) ? goto_state(&st_flyby) : 1;
}

static int next_keybd(int c, int d)
{
    if (d)
    {
        if (c == SDLK_F12)
            return goto_state(&st_poser);
        if (c == SDLK_ESCAPE)
            return goto_state(&st_over);
        if (c == SDLK_RETURN)
        {
            hole_goto(num, -1);
            num = 0;
            return goto_state(&st_next);
        }
        if ('0' <= c && c <= '9')
            num = num * 10 + c - '0';
    }
    return 1;
}

/*---------------------------------------------------------------------------*/

static int poser_enter(void)
{
    game_set_fly(-1.f);
    return 0;
}

static void poser_paint(int id, float st)
{
    game_draw(1);
}

static int poser_keybd(int c, int d)
{
    return (d && c == SDLK_ESCAPE) ? goto_state(&st_next) : 1;
}

/*---------------------------------------------------------------------------*/

static int flyby_enter(void)
{
    hud_init();
    return 0;
}

static void flyby_leave(int id)
{
    hud_free();
}

static void flyby_paint(int id, float st)
{
    game_draw(0);
    hud_paint();
}

static void flyby_timer(int id, float dt)
{
    float t = time_state();

    if (dt > 0.f && t > 1.f)
        goto_state(&st_stroke);
    else
        game_set_fly(1.f - t);

    gui_timer(id, dt);
    audio_timer(dt);
}

static int flyby_click(int b, int d)
{
    if (d && b < 0)
    {
        game_set_fly(0.f);
        return goto_state(&st_stroke);
    }
    return 1;
}

static int flyby_keybd(int c, int d)
{
    return (d && c == SDLK_ESCAPE) ? goto_state(&st_over) : 1;
}

/*---------------------------------------------------------------------------*/

static int stroke_enter(void)
{
    hud_init();
    game_clr_mag();
    config_set_d(CONFIG_CAMERA, 2);
    config_set_grab();
    return 0;
}

static void stroke_leave(int id)
{
    hud_free();
    config_clr_grab();
    config_set_d(CONFIG_CAMERA, 0);
}

static void stroke_paint(int id, float st)
{
    game_draw(0);
    hud_paint();
}

static void stroke_timer(int id, float dt)
{
    float g[3] = { 0.f, 0.f, 0.f };

    game_update_view(dt);
    game_step(g, dt);
    audio_timer(dt);
}

static void stroke_point(int id, int x, int y, int dx, int dy)
{
    game_set_rot(dx);
    game_set_mag(dy);
}

static int stroke_click(int b, int d)
{
    return (d && b < 0) ? goto_state(&st_roll) : 1;
}

static int stroke_keybd(int c, int d)
{
    return (d && c == SDLK_ESCAPE) ? goto_state(&st_over) : 1;
}

/*---------------------------------------------------------------------------*/

static int roll_enter(void)
{
    hud_init();
    game_putt();
    return 0;
}

static void roll_leave(int id)
{
    hud_free();
}

static void roll_paint(int id, float st)
{
    game_draw(0);
    hud_paint();
}

static void roll_timer(int id, float dt)
{
    float g[3] = { 0.0f, -9.8f, 0.0f };

    switch (game_step(g, dt))
    {
    case GAME_STOP: goto_state(&st_stop); break;
    case GAME_GOAL: goto_state(&st_goal); break;
    case GAME_FALL: goto_state(&st_fall); break;
    }
    audio_timer(dt);
}

static int roll_keybd(int c, int d)
{
    return (d && c == SDLK_ESCAPE) ? goto_state(&st_over) : 1;
}

/*---------------------------------------------------------------------------*/

static int goal_enter(void)
{
    int id;
    
    if ((id = gui_label(0, "It's In!", GUI_MED, GUI_ALL, gui_grn, gui_grn)))
        gui_layout(id, 0, 0);

    hole_goal();
    hud_init();

    return id;
}

static void goal_leave(int id)
{
    gui_delete(id);
    hud_free();
}

static void goal_paint(int id, float st)
{
    game_draw(0);
    gui_paint(id);
    hud_paint();
}

static void goal_timer(int id, float dt)
{
    if (time_state() > 3)
    {
        if (hole_next())
            goto_state(&st_next);
        else
            goto_state(&st_score);
    }
    audio_timer(dt);
}

static int goal_click(int b, int d)
{
    if (b < 0 && d == 1)
    {
        if (hole_next())
            goto_state(&st_next);
        else
            goto_state(&st_score);
    }
    return 1;
}

static int goal_keybd(int c, int d)
{
    return (d && c == SDLK_ESCAPE) ? goto_state(&st_over) : 1;
}

/*---------------------------------------------------------------------------*/

static int stop_enter(void)
{
    hole_stop();
    hud_init();
    return 0;
}

static void stop_leave(int id)
{
    hud_free();
}

static void stop_paint(int id, float st)
{
    game_draw(0);
    hud_paint();
}

static void stop_timer(int id, float dt)
{
    float g[3] = { 0.f, 0.f, 0.f };

    game_update_view(dt);
    game_step(g, dt);
    audio_timer(dt);

    if (time_state() > 1)
    {
        if (hole_next())
            goto_state(&st_next);
        else
            goto_state(&st_score);
    }
}

static int stop_click(int b, int d)
{
    if (b < 0 && d == 1)
    {
        if (hole_next())
            goto_state(&st_next);
        else
            goto_state(&st_score);
    }
    return 1;
}

static int stop_keybd(int c, int d)
{
    return (d && c == SDLK_ESCAPE) ? goto_state(&st_over) : 1;
}

/*---------------------------------------------------------------------------*/

static int fall_enter(void)
{
    int id;
    
    if ((id = gui_label(0, "1 Stroke Penalty", GUI_MED, GUI_ALL, gui_blk, gui_red)))
        gui_layout(id, 0, 0);

    hole_fall();
    hud_init();

    return id;
}

static void fall_leave(int id)
{
    gui_delete(id);
    hud_free();
}

static void fall_paint(int id, float st)
{
    game_draw(0);
    gui_paint(id);
    hud_paint();
}

static void fall_timer(int id, float dt)
{
    if (time_state() > 3)
    {
        if (hole_next())
            goto_state(&st_next);
        else
            goto_state(&st_score);
    }
    audio_timer(dt);
}

static int fall_click(int b, int d)
{
    if (b < 0 && d == 1)
    {
        if (hole_next())
            goto_state(&st_next);
        else
            goto_state(&st_score);
    }
    return 1;
}

static int fall_keybd(int c, int d)
{
    return (d && c == SDLK_ESCAPE) ? goto_state(&st_over) : 1;
}

/*---------------------------------------------------------------------------*/

static int score_enter(void)
{
    audio_music_fade_out(2.f);
    return score_card("Scores", gui_yel, gui_red);
}

static void score_leave(int id)
{
    gui_delete(id);
}

static void score_paint(int id, float st)
{
    game_draw(0);
    gui_paint(id);
}

static void score_timer(int id, float dt)
{
    gui_timer(id, dt);
    audio_timer(dt);
}

static int score_click(int b, int d)
{
    if (b < 0 && d == 1)
    {
        if (hole_move())
            return goto_state(&st_next);
        else
            return goto_state(&st_title);
    }
    return 1;
}

static int score_keybd(int c, int d)
{
    return (d && c == SDLK_ESCAPE) ? goto_state(&st_title) : 1;
}

/*---------------------------------------------------------------------------*/

static int over_enter(void)
{
    audio_music_fade_out(2.f);
    return score_card("Final Scores", gui_yel, gui_red);
}

static void over_leave(int id)
{
    gui_delete(id);
}

static void over_paint(int id, float st)
{
    game_draw(0);
    gui_paint(id);
}

static void over_timer(int id, float dt)
{
    gui_timer(id, dt);
    audio_timer(dt);
}

static int over_click(int b, int d)
{
    return (d && b < 0) ? goto_state(&st_title) : 1;
}

static int over_keybd(int c, int d)
{
    return (d && c == SDLK_ESCAPE) ? goto_state(&st_title) : 1;
}

/*---------------------------------------------------------------------------*/

struct state st_title = {
    title_enter,
    title_leave,
    title_paint,
    title_timer,
    title_point,
    NULL,
    title_click,
    title_keybd,
    NULL,
    1, 0
};

struct state st_course = {
    course_enter,
    course_leave,
    course_paint,
    course_timer,
    course_point,
    NULL,
    course_click,
    course_keybd,
    NULL,
    1, 0
};

struct state st_party = {
    party_enter,
    party_leave,
    party_paint,
    party_timer,
    party_point,
    NULL,
    party_click,
    party_keybd,
    NULL,
    1, 0
};

struct state st_next = {
    next_enter,
    next_leave,
    next_paint,
    next_timer,
    next_point,
    NULL,
    next_click,
    next_keybd,
    NULL,
    1, 0
};

struct state st_poser = {
    poser_enter,
    NULL,
    poser_paint,
    NULL,
    NULL,
    NULL,
    NULL,
    poser_keybd,
    NULL,
    1, 0
};

struct state st_flyby = {
    flyby_enter,
    flyby_leave,
    flyby_paint,
    flyby_timer,
    NULL,
    NULL,
    flyby_click,
    flyby_keybd,
    NULL,
    1, 0
};

struct state st_stroke = {
    stroke_enter,
    stroke_leave,
    stroke_paint,
    stroke_timer,
    stroke_point,
    NULL,
    stroke_click,
    stroke_keybd,
    NULL,
    0, 0
};

struct state st_roll = {
    roll_enter,
    roll_leave,
    roll_paint,
    roll_timer,
    NULL,
    NULL,
    NULL,
    roll_keybd,
    NULL,
    0, 0
};

struct state st_goal = {
    goal_enter,
    goal_leave,
    goal_paint,
    goal_timer,
    NULL,
    NULL,
    goal_click,
    goal_keybd,
    NULL,
    0, 0
};

struct state st_stop = {
    stop_enter,
    stop_leave,
    stop_paint,
    stop_timer,
    NULL,
    NULL,
    stop_click,
    stop_keybd,
    NULL,
    0, 0
};

struct state st_fall = {
    fall_enter,
    fall_leave,
    fall_paint,
    fall_timer,
    NULL,
    NULL,
    fall_click,
    fall_keybd,
    NULL,
    0, 0
};

struct state st_score = {
    score_enter,
    score_leave,
    score_paint,
    score_timer,
    NULL,
    NULL,
    score_click,
    score_keybd,
    NULL,
    0, 0
};

struct state st_over = {
    over_enter,
    over_leave,
    over_paint,
    over_timer,
    NULL,
    NULL,
    over_click,
    over_keybd,
    NULL,
    1, 0
};
