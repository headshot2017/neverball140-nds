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

#include <string.h>
#include <nds.h>

#include "config.h"
#include "audio.h"

#include <libxm7.h>
#include "libadx.h"
#include "wav/wav_nds.h"

// Assign FIFO_USER_07 channel to LibXM7
#define FIFO_LIBXM7 FIFO_USER_07

/*---------------------------------------------------------------------------*/

static int audio_state = 0;

static char        name[MAXSND][MAXSTR];
static int         chan[MAXSND];
static wav_handle *buff[MAXSND];

static char  curr_bgm[MAXSTR];
static char  next_bgm[MAXSTR];
static int   curr_pc;

static float fade_volume = 1.0f;
static float fade_rate   = 0.0f;

static XM7_ModuleManager_Type module;

static void mod_start(XM7_ModuleManager_Type *module)
{
    fifoSendValue32(FIFO_LIBXM7, (u32)module);
}

static void mod_stop(void)
{
    fifoSendValue32(FIFO_LIBXM7, 0);
}

/*---------------------------------------------------------------------------*/

void audio_init(void)
{
	/*
    int r = config_get_d(CONFIG_AUDIO_RATE);
    int b = config_get_d(CONFIG_AUDIO_BUFF);
    int i;

    

    if (audio_state == 0)
    {
        if (Mix_OpenAudio(r, MIX_DEFAULT_FORMAT, 1, b) == 0)
        {
            for (i = 0; i < MAXSND; i++)
                if (chan[i])
                    buff[i] = Mix_LoadWAV(config_data(name[i]));

            audio_state = 1;

            
        }
        else
        {
            fprintf(stderr, "Sound disabled\n");
            audio_state = 0;
        }
    }
	*/

	for (int i = 0; i < MAXSND; i++)
		if (chan[i])
			buff[i] = wav_load_handle(config_data(name[i]));

	memset(curr_bgm, 0, MAXSTR);
	memset(next_bgm, 0, MAXSTR);
	curr_pc = 0;

	adx_init();
	audio_state = 1;

	audio_volume(
		config_get_d(CONFIG_SOUND_VOLUME),
		config_get_d(CONFIG_MUSIC_VOLUME)
	);
}

void audio_free(void)
{
    int i;

    if (audio_state == 1)
    {
        if (curr_pc)
            adx_stop();
        else
            mod_stop();

        for (i = 0; i < MAXSND; i++)
            if (buff[i])
            {
                wav_free_handle(buff[i]);
                buff[i] = NULL;
            }

        audio_state = 0;
    }
}

void audio_bind(int i, int c, const char *filename)
{
    strncpy(name[i], filename, MAXSTR);
    chan[i] = c;
}

void audio_play(int i, float v)
{
    if (audio_state == 1 && buff[i])
    {
        //Mix_VolumeChunk(buff[i], (int) (v * MIX_MAX_VOLUME));
        wav_play(buff[i]);
    }
}

/*---------------------------------------------------------------------------*/

void audio_music_play(char *filename)
{
	int pc = !config_get_d(CONFIG_SOUNDTRACK);

	size_t size = strlen(filename);
	if (strcmp(filename + size-4, ".ogg") == 0)
	{
		filename[size-3] = pc ? 'a' : 'x';
		filename[size-2] = pc ? 'd' : 'm';
		filename[size-1] = pc ? 'x' : 0;
	}

	if (audio_state)
	{
		audio_music_stop();

		if (config_get_d(CONFIG_MUSIC_VOLUME) > 0)
		{
			if (pc)
			{
				adx_play(config_data(filename), 1);
				curr_pc = 1;
			}
			else
			{
				FILE* f = fopen(config_data(filename), "rb");
				fseek(f, 0, SEEK_END);
				size_t size = ftell(f);
				fseek(f, 0, SEEK_SET);
				unsigned char* modBuffer = (unsigned char*)malloc(size);
				fread(modBuffer, size, 1, f);
				fclose(f);
				DC_FlushAll();

				int res = XM7_LoadXM(&module, modBuffer);
				if (res != 0)
					printf("libxm7 error %0x%04x\n", res);
				free(modBuffer);
				mod_start(&module);
				curr_pc = 0;
			}
			strcpy(curr_bgm, filename);
		}
	}
}

void audio_music_queue(char *filename)
{
    if (audio_state)
    {
        if (strlen(curr_bgm) == 0 || strcmp(filename, curr_bgm) != 0)
        {
            audio_music_play(filename);

            if (curr_pc)
                adx_set_volume(0);
            fade_volume = 0.0f;

            strcpy(curr_bgm, filename);

            //adx_pause();
        }
    }
}

void audio_music_stop(void)
{
    if (audio_state)
    {
        if (curr_pc)
            adx_stop();
        else
            mod_stop();
    }
}

/*---------------------------------------------------------------------------*/
/*
 * SDL_mixer already provides music fading.  Unfortunately, it halts playback
 * at the end of a fade.  We need to be able to fade music back in from the
 * point where it stopped.  So, we reinvent this wheel.
 */

void audio_timer(float dt)
{
	if (audio_state)
	{
		float old_volume = fade_volume;

		if (fade_rate > 0.0f || fade_rate < 0.0f)
			fade_volume += dt / fade_rate;

		if (fade_volume < 0.0f)
		{
			fade_volume = 0.0f;

			if (strlen(next_bgm) == 0)
			{
				fade_rate = 0.0f;
				//if (Mix_PlayingMusic())
					//Mix_PauseMusic();
			}
			else
			{
				fade_rate = -fade_rate;
				audio_music_queue(next_bgm);
			}
		}

		if (fade_volume > 1.0f)
		{
			fade_rate   = 0.0f;
			fade_volume = 1.0f;
		}   

		/*
		if (Mix_PausedMusic() && fade_rate > 0.0f)
			Mix_ResumeMusic();
			
		if (Mix_PlayingMusic())
			Mix_VolumeMusic(config_get_d(CONFIG_MUSIC_VOLUME) *
							(int) (fade_volume * MIX_MAX_VOLUME) / 10);
		*/
		if (fade_volume != old_volume)
		{
			if (curr_pc)
				adx_set_volume(config_get_d(CONFIG_MUSIC_VOLUME) * (fade_volume * 127) / 10);
		}
	}
}

void audio_music_fade_out(float t)
{
    fade_rate = -t;
    strcpy(next_bgm, "");
}

void audio_music_fade_in(float t)
{
    fade_rate = +t;
    strcpy(next_bgm, "");
}

void audio_music_fade_to(float t, char *filename)
{
    if (fade_volume > 0)
    {
        if (strlen(curr_bgm) == 0 || strcmp(filename, curr_bgm) != 0)
        {
            strcpy(next_bgm, filename);
            fade_rate = -t;
        }
        else fade_rate = t;
    }
    else
    {
        audio_music_queue(filename);
        audio_music_fade_in(t);
    }
}

void audio_volume(int s, int m)
{
    if (audio_state)
    {
        //Mix_Volume(-1, s * MIX_MAX_VOLUME / 10);
        //Mix_VolumeMusic(m * MIX_MAX_VOLUME / 10);
        if (curr_pc)
            adx_set_volume(m * 127 / 10);
    }
}

/*---------------------------------------------------------------------------*/
