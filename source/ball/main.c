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

/*---------------------------------------------------------------------------*/

#ifdef WIN32
#pragma comment(lib, "SDL_ttf.lib")
#pragma comment(lib, "SDL_mixer.lib")
#pragma comment(lib, "SDL_image.lib")
#pragma comment(lib, "SDL.lib")
#pragma comment(lib, "SDLmain.lib")
#pragma comment(lib, "opengl32.lib")
#endif

/*---------------------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>
#include <nds.h>

#include "glext.h"
#include "config.h"
#include "image.h"
#include "audio.h"
#include "demo.h"
#include "game.h"
#include "gui.h"
#include "set.h"
#include "timer.h"
#include "libadx.h"

#include "st_conf.h"
#include "st_title.h"

#include "main.h"

#define TITLE "Neverball"

/*---------------------------------------------------------------------------*/

static void shot(void)
{
    static char filename[MAXSTR];
    static int  num = 0;

    sprintf(filename, "screen%02d.bmp", num++);

    image_snap(filename);
}

/*---------------------------------------------------------------------------*/

static void toggle_wire(void)
{
	/*
    static int wire = 0;

    if (wire)
    {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glEnable(GL_TEXTURE_2D);
        glEnable(GL_LIGHTING);
        wire = 0;
    }
    else
    {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glDisable(GL_TEXTURE_2D);
        glDisable(GL_LIGHTING);
        wire = 1;
    }
	*/
}

/*---------------------------------------------------------------------------*/

static int loop(void)
{
	adx_update();

	scanKeys();

	int d = 1;

	u32 held = keysHeld();
	u32 down = keysDown();
	u32 up = keysUp();

	if (down & KEY_SELECT)
		config_tgl_pause();

	if (!config_get_pause())
	{
		touchPosition touch;

		if (down)
			d = st_keybd(down, 1);
		if (down & KEY_A)
			d = st_buttn(config_get_d(CONFIG_JOYSTICK_BUTTON_A), 1);
		if (down & KEY_LEFT)
			st_stick(config_get_d(CONFIG_JOYSTICK_AXIS_X), -JOY_MAX);
		if (down & KEY_RIGHT)
			st_stick(config_get_d(CONFIG_JOYSTICK_AXIS_X), +JOY_MAX);
		if (down & KEY_UP)
			st_stick(config_get_d(CONFIG_JOYSTICK_AXIS_Y), -JOY_MAX);
		if (down & KEY_DOWN)
			st_stick(config_get_d(CONFIG_JOYSTICK_AXIS_Y), +JOY_MAX);
		if (held & KEY_TOUCH || down & KEY_TOUCH)
		{
			touchRead(&touch);
			st_point(touch.px, -touch.py + config_get_d(CONFIG_HEIGHT), 0, 0);
			if (down & KEY_TOUCH)
				d = st_click(-1, 1);
		}

		if (up)
			d = st_keybd(up, 0);
		if (up & KEY_A)
			d = st_buttn(config_get_d(CONFIG_JOYSTICK_BUTTON_A), 0);
		if (up & KEY_LEFT || up & KEY_RIGHT)
			st_stick(config_get_d(CONFIG_JOYSTICK_AXIS_X), 1);
		if (up & KEY_DOWN || up & KEY_UP)
			st_stick(config_get_d(CONFIG_JOYSTICK_AXIS_Y), 1);
		if (up & KEY_TOUCH)
			d = st_click(-1, 0);
	}

	return d;

	/*
    SDL_Event e;
    int d = 1;

    while (d && SDL_PollEvent(&e))
    {
        if (e.type == SDL_QUIT)
            return 0;

        if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_SPACE)
            config_tgl_pause();

        if (!config_get_pause())
            switch (e.type)
            {
            case SDL_MOUSEMOTION:
                st_point(+e.motion.x,
#ifdef __APPLE__
                         +e.motion.y,
#else
                         -e.motion.y + config_get_d(CONFIG_HEIGHT),
#endif
                         +e.motion.xrel,
                         config_get_d(CONFIG_MOUSE_INVERT)
                         ? +e.motion.yrel : -e.motion.yrel);
                break;

            case SDL_MOUSEBUTTONDOWN:
                d = st_click((e.button.button == SDL_BUTTON_LEFT) ? -1 : 1, 1);
                break;
                
            case SDL_MOUSEBUTTONUP:
                d = st_click((e.button.button == SDL_BUTTON_LEFT) ? -1 : 1, 0);
                break;

            case SDL_KEYDOWN:
                
                switch (e.key.keysym.sym)
                {
                case SDLK_F10:   shot();                    break;
                case SDLK_F9:    config_tgl_d(CONFIG_FPS);  break;
                case SDLK_F8:    config_tgl_d(CONFIG_NICE); break;
                case SDLK_F7:    toggle_wire();             break;
                
                case SDLK_RETURN:
                    d = st_buttn(config_get_d(CONFIG_JOYSTICK_BUTTON_A), 1);
                    break;
                case SDLK_LEFT:
                    st_stick(config_get_d(CONFIG_JOYSTICK_AXIS_X), -JOY_MAX);
                    break;
                case SDLK_RIGHT:
                    st_stick(config_get_d(CONFIG_JOYSTICK_AXIS_X), +JOY_MAX);
                    break;
                case SDLK_UP:
                    st_stick(config_get_d(CONFIG_JOYSTICK_AXIS_Y), -JOY_MAX);
                    break;
                case SDLK_DOWN:
                    st_stick(config_get_d(CONFIG_JOYSTICK_AXIS_Y), +JOY_MAX);
                    break;
                             
                default: 
                    d = st_keybd(e.key.keysym.sym, 1);
                }
                break;

            case SDL_KEYUP:

                switch (e.key.keysym.sym)
                {
                case SDLK_RETURN:
                    d = st_buttn(config_get_d(CONFIG_JOYSTICK_BUTTON_A), 0);
                    break;
                case SDLK_LEFT:
                case SDLK_RIGHT:
                    st_stick(config_get_d(CONFIG_JOYSTICK_AXIS_X), 1);
                    break;
                case SDLK_DOWN:
                case SDLK_UP:
                    st_stick(config_get_d(CONFIG_JOYSTICK_AXIS_Y), 1);
                    break;

                default:
                    d = st_keybd(e.key.keysym.sym, 0);
                }

                break;

            case SDL_ACTIVEEVENT:
                if (e.active.state == SDL_APPINPUTFOCUS)
                    if (e.active.gain == 0 && config_get_grab())
                        config_set_pause();
                break;

            case SDL_JOYAXISMOTION:
                st_stick(e.jaxis.axis, e.jaxis.value);
                break;

            case SDL_JOYBUTTONDOWN:
                d = st_buttn(e.jbutton.button, 1);
                break;

            case SDL_JOYBUTTONUP:
                d = st_buttn(e.jbutton.button, 0);
                break;
            }
    }
    return d;
	*/
}

int ball_main(int argc, char *argv[])
{
	vramSetPrimaryBanks(VRAM_A_LCD, VRAM_B_LCD, VRAM_C_LCD, VRAM_D_LCD);
	dmaFillHalfWords(0, VRAM_A, 131072);
	dmaFillHalfWords(0, VRAM_B, 131072);
	dmaFillHalfWords(0, VRAM_C, 131072);
	dmaFillHalfWords(0, VRAM_D, 131072);
	vramSetPrimaryBanks(VRAM_A_TEXTURE, VRAM_B_TEXTURE, VRAM_C_TEXTURE, VRAM_D_TEXTURE);

	vramSetBankE(VRAM_E_TEX_PALETTE);
	vramSetBankF(VRAM_F_TEX_PALETTE);
	vramSetBankG(VRAM_G_TEX_PALETTE);
	vramSetBankH(VRAM_H_SUB_BG);

	consoleInit(NULL, 1, BgType_Text4bpp, BgSize_T_256x256, 14, 0, false, true);
	consoleDebugInit(DebugDevice_NOCASH);

	REG_BLDCNT = BLEND_NONE;
	REG_BLDCNT_SUB = BLEND_NONE;


	if (config_data_path((argc > 1 ? argv[1] : NULL), SET_FILE))
	{
		if (config_user_path(NULL))
		{
			//if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK))
			{
				//SDL_Joystick *joy = NULL;

				printf("config init\n");
				config_init();
				printf("config load\n");
				config_load();
				printf("config done\n");

				/* Initialize the joystick. */

				/*
				if (SDL_NumJoysticks() > 0)
				{
					joy=SDL_JoystickOpen(config_get_d(CONFIG_JOYSTICK_DEVICE));
					if (joy)
						SDL_JoystickEventState(SDL_ENABLE);
				}
				*/

				/* Initialize the audio. */

				audio_bind(AUD_MENU,   3, "snd/menu.wav");
				audio_bind(AUD_START,  1, "snd/select.wav");
				audio_bind(AUD_READY,  1, "snd/ready.wav");
				audio_bind(AUD_SET,    1, "snd/set.wav");
				audio_bind(AUD_GO,     1, "snd/go.wav");
				audio_bind(AUD_BALL,   2, "snd/ball.wav");
				audio_bind(AUD_BUMP,   3, "snd/bump.wav");
				audio_bind(AUD_COIN,   2, "snd/coin.wav");
				audio_bind(AUD_TICK,   4, "snd/tick.wav");
				audio_bind(AUD_TOCK,   4, "snd/tock.wav");
				audio_bind(AUD_SWITCH, 5, "snd/switch.wav");
				audio_bind(AUD_JUMP,   5, "snd/jump.wav");
				audio_bind(AUD_GOAL,   5, "snd/goal.wav");
				audio_bind(AUD_SCORE,  1, "snd/record.wav");
				audio_bind(AUD_FALL,   1, "snd/fall.wav");
				audio_bind(AUD_TIME,   1, "snd/time.wav");
				audio_bind(AUD_OVER,   1, "snd/over.wav");

				printf("audio init\n");
				audio_init();
				printf("audio done\n");

				timer_init();

				/* Initialize the video. */

				if (config_mode(config_get_d(CONFIG_FULLSCREEN),
								256,
								192))
				{
					printf("video inited\n");
					int t1, t0 = timer_get();

					/* Initialize the run state and the title display. */

					init_state(&st_null);
					goto_state(&st_title);

					/* Run the main game loop. */

					while (loop())
						//if ((t1 = timer_get()) > t0)
						{
							t1 = timer_get();
							if (config_get_pause())
							{
								st_paint();
								gui_blank();
							}
							else
							{
								st_timer((t1 - t0) / 1000.f);
								st_paint();
							}
							adx_update();
							glFlush(0);

							t0 = t1;

							//if (config_get_d(CONFIG_NICE))
								//SDL_Delay(1);
						}
				}
				//else fprintf(stderr, "%s: %s\n", argv[0], SDL_GetError());

				audio_free();
				config_save();
				timer_free();

				//if (SDL_JoystickOpened(0))
					//SDL_JoystickClose(joy);

				//SDL_Quit();
			}
			//else fprintf(stderr, "%s: %s\n", argv[0], SDL_GetError());
		}
		else fprintf(stderr, "Failure to establish config directory\n");
	}
	else fprintf(stderr, "Failure to establish game data directory\n");

	goto_state(&st_null);

	consoleClear();
	consoleSelect(NULL);
	vramSetPrimaryBanks(VRAM_A_LCD, VRAM_B_LCD, VRAM_C_LCD, VRAM_D_LCD);
	vramSetBankH(VRAM_H_LCD);
	dmaFillHalfWords(0, VRAM_A, 131072);
	dmaFillHalfWords(0, VRAM_B, 131072);
	dmaFillHalfWords(0, VRAM_C, 131072);
	dmaFillHalfWords(0, VRAM_D, 131072);
	dmaFillHalfWords(0, VRAM_H, 32768);

	return 0;
}

/*---------------------------------------------------------------------------*/

