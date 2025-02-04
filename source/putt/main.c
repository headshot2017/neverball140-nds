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

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <nds.h>

#include "glext.h"
#include "audio.h"
#include "image.h"
#include "state.h"
#include "config.h"
#include "course.h"
#include "hole.h"
#include "game.h"
#include "gui.h"
#include "timer.h"
#include "libadx.h"

#include "st_conf.h"
#include "st_all.h"

#include "main.h"

#define TITLE "Neverputt"

/*---------------------------------------------------------------------------*/

static int shot(void)
{
    static char filename[MAXSTR];
    static int  num = 0;

    sprintf(filename, "screen%02d.bmp", num++);

    image_snap(filename);

    return 1;
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
						 -e.motion.yrel);
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
				case SDLK_F10: d = shot();                break;
				case SDLK_F9:  config_tgl_d(CONFIG_FPS);  break;
				case SDLK_F8:  config_tgl_d(CONFIG_NICE); break;
				case SDLK_F7:  toggle_wire();             break;
				
				default:
					d = st_keybd(e.key.keysym.sym, 1);
				}
				break;

			case SDL_ACTIVEEVENT:
				if (e.active.state == SDL_APPINPUTFOCUS)
				{
					if (e.active.gain == 0)
						config_set_pause();
				}
				break;
			}
	}
	return d;
	*/
}

int putt_main(int argc, char *argv[])
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

	int camera = 0;

	srand((int) time(NULL));

	if (config_data_path((argc > 1 ? argv[1] : NULL), COURSE_FILE))
	{
		if (config_user_path(NULL))
		{
			//if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) == 0)
			{
				config_init();
				config_load();

				/* Cache Neverball's camera setting. */

				camera = config_get_d(CONFIG_CAMERA);

				/* Initialize the audio. */

				audio_bind(AUD_BIRDIE,  1, "snd/birdie.wav");
				audio_bind(AUD_BOGEY,   1, "snd/bogey.wav");
				audio_bind(AUD_BUMP,    1, "snd/bink.wav");
				audio_bind(AUD_DOUBLE,  1, "snd/double.wav");
				audio_bind(AUD_EAGLE,   1, "snd/eagle.wav");
				audio_bind(AUD_JUMP,    2, "snd/jump.wav");
				audio_bind(AUD_MENU,    2, "snd/menu.wav");
				audio_bind(AUD_ONE,     1, "snd/one.wav");
				audio_bind(AUD_PAR,     1, "snd/par.wav");
				audio_bind(AUD_PENALTY, 1, "snd/penalty.wav");
				audio_bind(AUD_PLAYER1, 1, "snd/player1.wav");
				audio_bind(AUD_PLAYER2, 1, "snd/player2.wav");
				audio_bind(AUD_PLAYER3, 1, "snd/player3.wav");
				audio_bind(AUD_PLAYER4, 1, "snd/player4.wav");
				audio_bind(AUD_SWITCH,  2, "snd/switch.wav");
				audio_bind(AUD_SUCCESS, 1, "snd/success.wav");

				audio_init();
				timer_init();

				/* Initialize the video. */

				if (config_mode(config_get_d(CONFIG_FULLSCREEN),
								256,
								192))
				{
					int t1, t0 = timer_get();

					/* Run the main game loop. */

					init_state(&st_putt_null);
					goto_state(&st_putt_title);

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

				/* Restore Neverball's camera setting. */

				config_set_d(CONFIG_CAMERA, camera);
				config_save();
				audio_free();
				timer_free();

				//SDL_Quit();
			}
			//else fprintf(stderr, "%s: %s\n", argv[0], SDL_GetError());
		}
		else fprintf(stderr, "Failure to establish config directory\n");
	}
	else fprintf(stderr, "Failure to establish game data directory\n");

	goto_state(&st_putt_null);

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

