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

#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "glext.h"
#include "geom.h"
#include "part.h"
#include "vec3.h"
#include "solid.h"
#include "image.h"
#include "config.h"

#define PI 3.1415926535897932f

/*---------------------------------------------------------------------------*/

static uint32_t* ball_list;
static int ball_text;

void ball_init(int b)
{
    char name[MAXSTR];
	int slices = b ? 32 : 16;
    int stacks = b ? 16 :  8;

    config_get_s(CONFIG_BALL, name, MAXSTR);

    ball_text = make_image_from_file(NULL, NULL, NULL, NULL, name);

    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameter(0, GL_TEXTURE_WRAP_S | GL_TEXTURE_WRAP_T | TEXGEN_TEXCOORD);

	ball_list = (uint32_t*)malloc(sizeof(uint32_t) * 8192);
	ball_list[0] = 0;

	uint32_t S = ball_list[0];

	gl_texture_data *tex = (gl_texture_data*)DynamicArrayGet( &glGlob.texturePtrs, ball_text );
	u16 palAddr = 0;

	int w = 8 << ((tex->texFormat >> 20 ) & 7 ); // from videoGL.h glGetInt()
	int h = 8 << ((tex->texFormat >> 23 ) & 7 ); // from videoGL.h glGetInt()

	if (tex->palIndex)
	{
		gl_palette_data *pal = (gl_palette_data*)DynamicArrayGet( &glGlob.palettePtrs, tex->palIndex );
		palAddr = pal->addr;
	}

	ball_list[++S] = FIFO_COMMAND_PACK(FIFO_TEX_FORMAT, FIFO_PAL_FORMAT, FIFO_SPECULAR_EMISSION, FIFO_DIFFUSE_AMBIENT);
	ball_list[++S] = tex->texFormat;
	ball_list[++S] = palAddr;
	ball_list[++S] = RGB15(6,6,6) + (RGB15(31,31,31) << 16);
	ball_list[++S] = RGB15(31,31,31) + (RGB15(31,31,31) << 16);
	ball_list[++S] = FIFO_COMMAND_PACK(FIFO_COLOR, FIFO_NOP, FIFO_NOP, FIFO_NOP);
	ball_list[++S] = RGB15(31,31,31);

	for (int i = 0; i < stacks; i++)
	{
		float k0 = (float)  i      / stacks;
		float k1 = (float) (i + 1) / stacks;

		float s0 = fsinf(V_PI * (k0 - 0.5f));
		float c0 = fcosf(V_PI * (k0 - 0.5f));
		float s1 = fsinf(V_PI * (k1 - 0.5f));
		float c1 = fcosf(V_PI * (k1 - 0.5f));

		ball_list[++S] = FIFO_COMMAND_PACK(FIFO_BEGIN, FIFO_NOP, FIFO_NOP, FIFO_NOP);
		ball_list[++S] = GL_QUAD_STRIP;

		for (int j = 0; j <= slices; j++)
		{
			float k = (float) j / slices;
			float s = fsinf(V_PI * k * 2.0f);
			float c = fcosf(V_PI * k * 2.0f);

			ball_list[++S] = FIFO_COMMAND_PACK(FIFO_TEX_COORD, FIFO_NORMAL, FIFO_VERTEX16, FIFO_NOP);
			ball_list[++S] = TEXTURE_PACK(floattot16(k * w), floattot16(k0 * h));
			ball_list[++S] = NORMAL_PACK(floattov10(s * c0), floattov10(c * c0), floattov10(s0));
			ball_list[++S] = VERTEX_PACK(floattov16(s * c0 / SCALE_VERTICES), floattov16(c * c0 / SCALE_VERTICES));
			ball_list[++S] = VERTEX_PACK(floattov16(s0 / SCALE_VERTICES), 0);

			ball_list[++S] = FIFO_COMMAND_PACK(FIFO_TEX_COORD, FIFO_NORMAL, FIFO_VERTEX16, FIFO_NOP);
			ball_list[++S] = TEXTURE_PACK(floattot16(k * w), floattot16(k1 * h));
			ball_list[++S] = NORMAL_PACK(floattov10(s * c1), floattov10(c * c1), floattov10(s1));
			ball_list[++S] = VERTEX_PACK(floattov16(s * c1 / SCALE_VERTICES), floattov16(c * c1 / SCALE_VERTICES));
			ball_list[++S] = VERTEX_PACK(floattov16(s1 / SCALE_VERTICES), 0);
		}

		ball_list[++S] = FIFO_COMMAND_PACK(FIFO_END, FIFO_NOP, FIFO_NOP, FIFO_NOP);
	}

	ball_list[0] = S;
	printf("ball: %d %d %d '%s'\n", S, ball_text, palAddr, name);
}

void ball_free(void)
{
    //if (glIsList(ball_list))
        //glDeleteLists(ball_list, 1);
	free(ball_list);

    //if (glIsTexture(ball_text))
        glDeleteTextures(1, &ball_text);

    ball_list = 0;
    ball_text = 0;
}

void ball_draw(void)
{
	/*
	static const float  s[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
	static const float  e[4] = { 0.2f, 0.2f, 0.2f, 1.0f };
	static const float  h[1] = { 64.0f };

	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR,  s);
	glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION,  e);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, h);
	*/
	
	//glMaterialf(GL_AMBIENT, RGB15(31,31,31));
	//glMaterialf(GL_DIFFUSE, RGB15(31,31,31));
    //glMaterialf(GL_SPECULAR, RGB15(31, 31, 31));
    //glMaterialf(GL_EMISSION, RGB15(6, 6, 6));
    //glMaterialf(GL_SHININESS, RGB15(16, 16, 16));

	//glEnable(GL_COLOR_MATERIAL);
	//glEnable(GL_TEXTURE_2D);

	glPolyFmt(POLY_ID(60) | POLY_ALPHA(25) | POLY_CULL_BACK);
	glCallList(ball_list);
	glPolyFmt(POLY_ALPHA(31) | POLY_CULL_BACK);

	/*
	glBindTexture(GL_TEXTURE_2D, ball_text);
	glColor3b(255,255,255);

	int slices = _b ? 32 : 16;
    int stacks = _b ? 16 :  8;

	for (int i = 0; i < stacks; i++)
	{
		float k0 = (float)  i      / stacks;
		float k1 = (float) (i + 1) / stacks;

		float s0 = fsinf(V_PI * (k0 - 0.5f));
		float c0 = fcosf(V_PI * (k0 - 0.5f));
		float s1 = fsinf(V_PI * (k1 - 0.5f));
		float c1 = fcosf(V_PI * (k1 - 0.5f));

		glBegin(GL_QUAD_STRIP);
		{
			for (int j = 0; j <= slices; j++)
			{
				float k = (float) j / slices;
				float s = fsinf(V_PI * k * 2.0f);
				float c = fcosf(V_PI * k * 2.0f);

				glTexCoord2f(k, k0);
				glNormal3f(s * c0, c * c0, s0);
				glVertex3f(s * c0 / SCALE_VERTICES, c * c0 / SCALE_VERTICES, s0 / SCALE_VERTICES);

				glTexCoord2f(k, k1);
				glNormal3f(s * c1, c * c1, s1);
				glVertex3f(s * c1 / SCALE_VERTICES, c * c1 / SCALE_VERTICES, s1 / SCALE_VERTICES);
			}
		}
		glEnd();
	}
	*/

	// Render the ball back to front in case it is translucent.

	/*
	//glDepthMask(GL_FALSE);

	glCullFace(GL_FRONT);
	glCallList(ball_list);
	glCullFace(GL_BACK);
	glCallList(ball_list);

	// Render the ball into the depth buffer.

	glDepthMask(GL_TRUE);
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
	{
		glCallList(ball_list);
	}
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

	// Ensure the ball is visible even when obscured by geometry.

	glDisable(GL_DEPTH_TEST);

	glColor4f(1.0f, 1.0f, 1.0f, 0.1f);
	glCallList(ball_list);
	*/
}

/*---------------------------------------------------------------------------*/

//static GLuint mark_list;
//static int mark_b;

void mark_init(int b)
{
	//mark_b = b;
	/*
	int i, slices = b ? 32 : 16;

	//mark_list = glGenLists(1);

	glNewList(mark_list, GL_COMPILE);
	{
		glBegin(GL_TRIANGLE_FAN);
		{
			glNormal3f(0.f, 1.f, 0.f);

			for (i = 0; i < slices; i++)
			{
				float x = fcosf(-2.f * PI * i / slices);
				float y = fsinf(-2.f * PI * i / slices);

				glVertex3f(x, 0, y);
			}
		}
		glEnd();
	}
	glEndList();
	*/
}

void mark_draw(void)
{
	//glPushAttrib(GL_TEXTURE_BIT);
	//glPushAttrib(GL_LIGHTING_BIT);
	//glPushAttrib(GL_DEPTH_BUFFER_BIT);
	{
		//glEnable(GL_COLOR_MATERIAL);
		//glDisable(GL_TEXTURE_2D);
		//glDepthMask(GL_FALSE);
		/*
		int i, slices = mark_b ? 32 : 16;
		glBindTexture(0, 0);
		glBegin(GL_TRIANGLE_FAN);
		{
			glNormal3f(0.f, 1.f, 0.f);

			for (i = 0; i < slices; i++)
			{
				float x = fcosf(-2.f * PI * i / slices);
				float y = fsinf(-2.f * PI * i / slices);

				glVertex3f(x / SCALE_VERTICES, 0, y / SCALE_VERTICES);
			}
		}
		glEnd();
		*/

		//glCallList(mark_list);
	}
	//glPopAttrib();
	//glPopAttrib();
	//glPopAttrib();
}

void mark_free(void)
{
    //if (glIsList(mark_list))
        //glDeleteLists(mark_list, 1);

    //mark_list = 0;
}

/*---------------------------------------------------------------------------*/

static int coin_text;
static uint32_t* coin_list;

static uint32_t coin_head(int n, float radius, float thick, uint32_t* list, int w, int h)
{
	uint32_t S = list[0];

	list[++S] = FIFO_COMMAND_PACK(FIFO_BEGIN, FIFO_NORMAL, FIFO_NOP, FIFO_NOP);
	list[++S] = GL_QUADS;
	list[++S] = NORMAL_PACK(0, 0, inttov10(1));

	list[++S] = FIFO_COMMAND_PACK(FIFO_TEX_COORD, FIFO_VERTEX16, FIFO_TEX_COORD, FIFO_VERTEX_XY);
	list[++S] = TEXTURE_PACK(0, 0);
	list[++S] = VERTEX_PACK(floattov16(-radius / SCALE_VERTICES), floattov16(+radius / SCALE_VERTICES));
	list[++S] = VERTEX_PACK(floattov16(+thick / SCALE_VERTICES), 0);
	list[++S] = TEXTURE_PACK(0, inttot16(h));
	list[++S] = VERTEX_PACK(floattov16(-radius / SCALE_VERTICES), floattov16(-radius / SCALE_VERTICES));

	list[++S] = FIFO_COMMAND_PACK(FIFO_TEX_COORD, FIFO_VERTEX_XY, FIFO_TEX_COORD, FIFO_VERTEX_XY);
	list[++S] = TEXTURE_PACK(inttot16(w), inttot16(h));
	list[++S] = VERTEX_PACK(floattov16(+radius / SCALE_VERTICES), floattov16(-radius / SCALE_VERTICES));
	list[++S] = TEXTURE_PACK(inttot16(w), 0);
	list[++S] = VERTEX_PACK(floattov16(+radius / SCALE_VERTICES), floattov16(+radius / SCALE_VERTICES));

	list[++S] = FIFO_COMMAND_PACK(FIFO_END, FIFO_NOP, FIFO_NOP, FIFO_NOP);

	list[0] = S;

	/*
	glBegin(GL_QUADS);
	{
        glNormal3f(0.f, 0.f, +1.f);
		glTexCoord2f(0.0f, 0.0f); glVertex3f(-radius / SCALE_VERTICES, +radius / SCALE_VERTICES, +thick / SCALE_VERTICES);
		glTexCoord2f(0.0f, 1.0f); glVertex3f(-radius / SCALE_VERTICES, -radius / SCALE_VERTICES, +thick / SCALE_VERTICES);
		glTexCoord2f(1.0f, 1.0f); glVertex3f(+radius / SCALE_VERTICES, -radius / SCALE_VERTICES, +thick / SCALE_VERTICES);
		glTexCoord2f(1.0f, 0.0f); glVertex3f(+radius / SCALE_VERTICES, +radius / SCALE_VERTICES, +thick / SCALE_VERTICES);
    }
	glEnd();
	*/

	/*
	int i;

	glBegin(GL_TRIANGLE_FAN);
	{
		glNormal3f(0.f, 0.f, +1.f);

		for (i = 0; i < n; i++)
		{
			float x = fcosf(+2.f * PI * i / n);
			float y = fsinf(+2.f * PI * i / n);

			glTexCoord2f(-x * 0.5f + 0.5f, +y * 0.5f + 0.5f);
			glVertex3f(radius * x, radius * y, +thick);
		}
	}
	glEnd();
	*/

	return S;
}

static uint32_t coin_tail(int n, float radius, float thick, uint32_t* list, int w, int h)
{
	uint32_t S = list[0];

	list[++S] = FIFO_COMMAND_PACK(FIFO_BEGIN, FIFO_NORMAL, FIFO_NOP, FIFO_NOP);
	list[++S] = GL_QUADS;
	list[++S] = NORMAL_PACK(0, 0, inttov10(-1));

	list[++S] = FIFO_COMMAND_PACK(FIFO_TEX_COORD, FIFO_VERTEX16, FIFO_TEX_COORD, FIFO_VERTEX_XY);
	list[++S] = TEXTURE_PACK(0, 0);
	list[++S] = VERTEX_PACK(floattov16(-radius / SCALE_VERTICES), floattov16(-radius / SCALE_VERTICES));
	list[++S] = VERTEX_PACK(floattov16(-thick / SCALE_VERTICES), 0);
	list[++S] = TEXTURE_PACK(0, inttot16(h));
	list[++S] = VERTEX_PACK(floattov16(-radius / SCALE_VERTICES), floattov16(+radius / SCALE_VERTICES));

	list[++S] = FIFO_COMMAND_PACK(FIFO_TEX_COORD, FIFO_VERTEX_XY, FIFO_TEX_COORD, FIFO_VERTEX_XY);
	list[++S] = TEXTURE_PACK(inttot16(w), inttot16(h));
	list[++S] = VERTEX_PACK(floattov16(+radius / SCALE_VERTICES), floattov16(+radius / SCALE_VERTICES));
	list[++S] = TEXTURE_PACK(inttot16(w), 0);
	list[++S] = VERTEX_PACK(floattov16(+radius / SCALE_VERTICES), floattov16(-radius / SCALE_VERTICES));

	list[++S] = FIFO_COMMAND_PACK(FIFO_END, FIFO_NOP, FIFO_NOP, FIFO_NOP);

	list[0] = S;

	/*
	glBegin(GL_QUADS);
	{
		glNormal3f(0.f, 0.f, -1.f);
		glTexCoord2f(0.0f, 0.0f); glVertex3f(-radius / SCALE_VERTICES, -radius / SCALE_VERTICES, -thick / SCALE_VERTICES);
		glTexCoord2f(0.0f, 1.0f); glVertex3f(-radius / SCALE_VERTICES, +radius / SCALE_VERTICES, -thick / SCALE_VERTICES);
		glTexCoord2f(1.0f, 1.0f); glVertex3f(+radius / SCALE_VERTICES, +radius / SCALE_VERTICES, -thick / SCALE_VERTICES);
		glTexCoord2f(1.0f, 0.0f); glVertex3f(+radius / SCALE_VERTICES, -radius / SCALE_VERTICES, -thick / SCALE_VERTICES);
	}
	glEnd();
	*/

	/*
	int i;

	glBegin(GL_TRIANGLE_FAN);
	{
		glNormal3f(0.f, 0.f, -1.f);

		for (i = 0; i < n; i++)
		{
			float x = fcosf(-2.f * PI * i / n);
			float y = fsinf(-2.f * PI * i / n);

			glTexCoord2f(+x * 0.5f + 0.5f, +y * 0.5f + 0.5f);
			glVertex3f(radius * x, radius * y, -thick);
		}
	}
	glEnd();
	*/

	return S;
}

static uint32_t coin_edge(int n, float radius, float thick, uint32_t* list)
{
    int i;
    uint32_t S = list[0];

	list[++S] = FIFO_COMMAND_PACK(FIFO_BEGIN, FIFO_NOP, FIFO_NOP, FIFO_NOP);
	list[++S] = GL_QUAD_STRIP;
    {
        for (i = 0; i <= n; i++)
        {
            float x = fcosf(2.f * PI * i / n);
            float y = fsinf(2.f * PI * i / n);

			list[++S] = FIFO_COMMAND_PACK(FIFO_NORMAL, FIFO_VERTEX16, FIFO_VERTEX_XZ, FIFO_NOP);
			list[++S] = NORMAL_PACK(floattov10(x), floattov10(y), 0);
			list[++S] = VERTEX_PACK(floattov16(radius * x / SCALE_VERTICES), floattov16(radius * y / SCALE_VERTICES));
			list[++S] = VERTEX_PACK(floattov16(+thick / SCALE_VERTICES), 0);
			list[++S] = VERTEX_PACK(floattov16(radius * x / SCALE_VERTICES), floattov16(-thick / SCALE_VERTICES));

            //glNormal3f(x, y, 0.f);
            //glVertex3f(radius * x / SCALE_VERTICES, radius * y / SCALE_VERTICES, +thick / SCALE_VERTICES);
            //glVertex3f(radius * x / SCALE_VERTICES, radius * y / SCALE_VERTICES, -thick / SCALE_VERTICES);
        }
    }
    list[++S] = FIFO_COMMAND_PACK(FIFO_END, FIFO_NOP, FIFO_NOP, FIFO_NOP);
	list[0] = S;

	return S;
}

void coin_color(uint8_t* c, int n)
{
    if (n >= 1)
    {
        c[0] = 255;
        c[1] = 255;
        c[2] = 51;
    }
    if (n >= 5)
    {
        c[0] = 255;
        c[1] = 51;
        c[2] = 51;
    }
    if (n >= 10)
    {
        c[0] = 51;
        c[1] = 51;
        c[2] = 255;
    }
}

void coin_init(int b)
{
	char name[MAXSTR];
	int n = b ? 32 : 8;

	config_get_s(CONFIG_COIN, name, MAXSTR);

	coin_text = make_image_from_file(NULL, NULL, NULL, NULL, name);
	coin_list = (uint32_t*)malloc(sizeof(uint32_t) * 128);
	coin_list[0] = 0;
	uint32_t S = coin_list[0];

	int w, h;
	glGetInt(GL_GET_TEXTURE_WIDTH, &w);
	glGetInt(GL_GET_TEXTURE_HEIGHT, &h);

	S = coin_edge(n, COIN_RADIUS, COIN_THICK, coin_list);
	S = coin_head(n, COIN_RADIUS, COIN_THICK, coin_list, w, h);
	S = coin_tail(n, COIN_RADIUS, COIN_THICK, coin_list, w, h);

	coin_list[0] = S;
}

void coin_free(void)
{
    //if (glIsList(coin_list))
        //glDeleteLists(coin_list, 1);
    free(coin_list);

    //if (glIsTexture(coin_text))
        glDeleteTextures(1, &coin_text);

    coin_list = 0;
    coin_text = 0;
}

void coin_push(void)
{
	/*
	static const float  a[4] = { 0.2f, 0.2f, 0.2f, 1.0f };
	static const float  s[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
	static const float  e[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	static const float  h[1] = { 32.0f };

	glPushAttrib(GL_LIGHTING_BIT);

	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT,   a);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR,  s);
	glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION,  e);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, h);
	*/
	glMaterialf(GL_AMBIENT, RGB15(31,31,31));
	glMaterialf(GL_DIFFUSE, RGB15(31,31,31));

	//glEnable(GL_COLOR_MATERIAL);
	//glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, coin_text);
}

void coin_draw(int n, float r)
{
	uint8_t c[3] = {0, 0, 0};

	coin_color(c, n);

	//glColor3b(c[0], c[1], c[2]);
	glMaterialf(GL_EMISSION, RGB15(c[0]*31/255, c[1]*31/255, c[2]*31/255));

	glPolyFmt(POLY_ALPHA(31) | POLY_CULL_BACK);
	glCallList(coin_list);
}

void coin_pull(void)
{
    //glPopAttrib();
}

/*---------------------------------------------------------------------------*/

static uint32_t* goal_list;

void goal_init(int b)
{
	int n = b ? 32 : 8;

	goal_list = (uint32_t*)malloc(sizeof(uint32_t) * 128);
	goal_list[0] = 0;
	uint32_t S = goal_list[0];

	goal_list[++S] = FIFO_COMMAND_PACK(FIFO_TEX_FORMAT, FIFO_PAL_FORMAT, FIFO_POLY_FORMAT, FIFO_BEGIN);
	goal_list[++S] = 0;
	goal_list[++S] = 0;
	goal_list[++S] = POLY_ALPHA(16) | POLY_CULL_BACK;
	goal_list[++S] = GL_QUAD_STRIP;

	for (int i = 0; i <= n; i++)
	{
		float x = fcosf(2.f * PI * i / n);
		float y = fsinf(2.f * PI * i / n);

		goal_list[++S] = FIFO_COMMAND_PACK(FIFO_COLOR, FIFO_VERTEX16, FIFO_VERTEX_XY, FIFO_NOP);
		goal_list[++S] = RGB15(31, 31, 0);
		goal_list[++S] = VERTEX_PACK(floattov16(x / SCALE_VERTICES), 0);
		goal_list[++S] = VERTEX_PACK(floattov16(y / SCALE_VERTICES), 0);
		goal_list[++S] = VERTEX_PACK(floattov16(x / SCALE_VERTICES), floattov16(GOAL_HEIGHT / SCALE_VERTICES));
	}

	goal_list[++S] = FIFO_COMMAND_PACK(FIFO_END, FIFO_POLY_FORMAT, FIFO_NOP, FIFO_NOP);
	goal_list[++S] = POLY_ALPHA(31) | POLY_CULL_BACK;

	goal_list[0] = S;
}

void goal_free(void)
{
	//if (glIsList(goal_list))
		//glDeleteLists(goal_list, 1);

	free(goal_list);
	goal_list = 0;
}

void goal_draw(void)
{
	glCallList(goal_list);
	/*
	glEnable(GL_COLOR_MATERIAL);
	glDisable(GL_LIGHTING);
	glDisable(GL_TEXTURE_2D);
	glDepthMask(GL_FALSE);
	*/

	/*
	int n = goal_b;
	glBindTexture(0, 0);
	glPolyFmt(POLY_ID(61) | POLY_ALPHA(16) | POLY_CULL_BACK);

	glBegin(GL_QUAD_STRIP);
	{
		for (int i = 0; i <= n; i++)
		{
			float x = fcosf(2.f * PI * i / n);
			float y = fsinf(2.f * PI * i / n);
	
			//glColor4f(1.0f, 1.0f, 0.0f, 0.5f);
			glColor3b(255, 255, 0);
			glVertex3f(x / SCALE_VERTICES, 0, y / SCALE_VERTICES);

			//glColor4f(1.0f, 1.0f, 0.0f, 0.0f);
			glColor3b(255, 255, 0);
			glVertex3f(x / SCALE_VERTICES, GOAL_HEIGHT / SCALE_VERTICES, y / SCALE_VERTICES);
		}
	}
	glEnd();

	glPolyFmt(POLY_ALPHA(31) | POLY_CULL_BACK);
	*/
}

/*---------------------------------------------------------------------------*/

static uint32_t* jump_list;

void jump_init(int b)
{
	int n = b ? 32 : 8;

	jump_list = (uint32_t*)malloc(sizeof(uint32_t) * 128);
	jump_list[0] = 0;
	uint32_t S = jump_list[0];

	jump_list[++S] = FIFO_COMMAND_PACK(FIFO_TEX_FORMAT, FIFO_PAL_FORMAT, FIFO_POLY_FORMAT, FIFO_BEGIN);
	jump_list[++S] = 0;
	jump_list[++S] = 0;
	jump_list[++S] = POLY_ALPHA(16) | POLY_CULL_BACK;
	jump_list[++S] = GL_QUAD_STRIP;

	for (int i = 0; i <= n; i++)
	{
		float x = fcosf(2.f * PI * i / n);
		float y = fsinf(2.f * PI * i / n);

		jump_list[++S] = FIFO_COMMAND_PACK(FIFO_COLOR, FIFO_VERTEX16, FIFO_VERTEX_XY, FIFO_NOP);
		jump_list[++S] = RGB15(31, 31, 31);
		jump_list[++S] = VERTEX_PACK(floattov16(x / SCALE_VERTICES), 0);
		jump_list[++S] = VERTEX_PACK(floattov16(y / SCALE_VERTICES), 0);
		jump_list[++S] = VERTEX_PACK(floattov16(x / SCALE_VERTICES), floattov16(JUMP_HEIGHT / SCALE_VERTICES));
	}

	jump_list[++S] = FIFO_COMMAND_PACK(FIFO_END, FIFO_POLY_FORMAT, FIFO_NOP, FIFO_NOP);
	jump_list[++S] = POLY_ALPHA(31) | POLY_CULL_BACK;

	jump_list[0] = S;
}

void jump_free(void)
{
	//if (glIsList(jump_list))
		//glDeleteLists(jump_list, 1);

	free(jump_list);
	jump_list = 0;
}

void jump_draw(void)
{
	glCallList(jump_list);

	//glEnable(GL_COLOR_MATERIAL);
	//glDisable(GL_LIGHTING);
	//glDisable(GL_TEXTURE_2D);
	//glDepthMask(GL_FALSE);
	/*
	glPolyFmt(POLY_ALPHA(16) | POLY_CULL_BACK);

	int n = jump_b;
	glBindTexture(0, 0);

	glBegin(GL_QUAD_STRIP);
	{
		for (int i = 0; i <= n; i++)
		{
			float x = fcosf(2.f * PI * i / n);
			float y = fsinf(2.f * PI * i / n);

			//glColor4f(1.0f, 1.0f, 1.0f, 0.5f);
			glColor3b(255, 255, 255);
			glVertex3f(x / SCALE_VERTICES, 0, y / SCALE_VERTICES);

			//glColor4f(1.0f, 1.0f, 1.0f, 0.0f);
			glColor3b(255, 255, 255);
			glVertex3f(x / SCALE_VERTICES, JUMP_HEIGHT / SCALE_VERTICES, y / SCALE_VERTICES);
		}
	}
	glEnd();

	glPolyFmt(POLY_ALPHA(31) | POLY_CULL_BACK);
	*/
}

/*---------------------------------------------------------------------------*/

static uint32_t* swch_list_off;
static uint32_t* swch_list_on;

void swch_init(int b)
{
    int n = b ? 32 : 8;

	// Create the ON display list.

	swch_list_on = (uint32_t*)malloc(sizeof(uint32_t) * 128);
	swch_list_on[0] = 0;
	uint32_t S = swch_list_on[0];

	swch_list_on[++S] = FIFO_COMMAND_PACK(FIFO_TEX_FORMAT, FIFO_PAL_FORMAT, FIFO_POLY_FORMAT, FIFO_BEGIN);
	swch_list_on[++S] = 0;
	swch_list_on[++S] = 0;
	swch_list_on[++S] = POLY_ID(61) | POLY_ALPHA(16) | POLY_CULL_BACK;
	swch_list_on[++S] = GL_QUAD_STRIP;

	for (int i = 0; i <= n; i++)
	{
		float x = fcosf(2.f * PI * i / n);
		float y = fsinf(2.f * PI * i / n);

		swch_list_on[++S] = FIFO_COMMAND_PACK(FIFO_COLOR, FIFO_VERTEX16, FIFO_VERTEX_XY, FIFO_NOP);
		swch_list_on[++S] = RGB15(0, 31, 0);
		swch_list_on[++S] = VERTEX_PACK(floattov16(x / SCALE_VERTICES), 0);
		swch_list_on[++S] = VERTEX_PACK(floattov16(y / SCALE_VERTICES), 0);
		swch_list_on[++S] = VERTEX_PACK(floattov16(x / SCALE_VERTICES), floattov16(SWCH_HEIGHT / SCALE_VERTICES));
	}

	swch_list_on[++S] = FIFO_COMMAND_PACK(FIFO_END, FIFO_POLY_FORMAT, FIFO_NOP, FIFO_NOP);
	swch_list_on[++S] = POLY_ALPHA(31) | POLY_CULL_BACK;

	swch_list_on[0] = S;

	// Create the OFF display list.

	swch_list_off = (uint32_t*)malloc(sizeof(uint32_t) * 128);
	swch_list_off[0] = 0;
	S = swch_list_off[0];

	swch_list_off[++S] = FIFO_COMMAND_PACK(FIFO_TEX_FORMAT, FIFO_PAL_FORMAT, FIFO_POLY_FORMAT, FIFO_BEGIN);
	swch_list_off[++S] = 0;
	swch_list_off[++S] = 0;
	swch_list_off[++S] = POLY_ALPHA(16) | POLY_CULL_BACK;
	swch_list_off[++S] = GL_QUAD_STRIP;

	for (int i = 0; i <= n; i++)
	{
		float x = fcosf(2.f * PI * i / n);
		float y = fsinf(2.f * PI * i / n);

		swch_list_off[++S] = FIFO_COMMAND_PACK(FIFO_COLOR, FIFO_VERTEX16, FIFO_VERTEX_XY, FIFO_NOP);
		swch_list_off[++S] = RGB15(31, 0, 0);
		swch_list_off[++S] = VERTEX_PACK(floattov16(x / SCALE_VERTICES), 0);
		swch_list_off[++S] = VERTEX_PACK(floattov16(y / SCALE_VERTICES), 0);
		swch_list_off[++S] = VERTEX_PACK(floattov16(x / SCALE_VERTICES), floattov16(SWCH_HEIGHT / SCALE_VERTICES));
	}

	swch_list_off[++S] = FIFO_COMMAND_PACK(FIFO_END, FIFO_POLY_FORMAT, FIFO_NOP, FIFO_NOP);
	swch_list_off[++S] = POLY_ALPHA(31) | POLY_CULL_BACK;

	swch_list_off[0] = S;
}

void swch_free(void)
{
	/*
	if (glIsList(swch_list))
		glDeleteLists(swch_list, 2);

	swch_list = 0;
	*/
	free(swch_list_off);
	free(swch_list_on);
	swch_list_off = 0;
	swch_list_on = 0;
}

void swch_draw(int b)
{
	if (b)
	{
		// ON
        glCallList(swch_list_on);
	}
    else
	{
		// OFF
        glCallList(swch_list_off);
	}
}

/*---------------------------------------------------------------------------*/

static uint32_t* flag_list;

void flag_init(int b)
{
	int n = b ? 8 : 4;

	flag_list = (uint32_t*)malloc(sizeof(uint32_t) * 128);
	flag_list[0] = 0;
	uint32_t S = flag_list[0];

	flag_list[++S] = FIFO_COMMAND_PACK(FIFO_TEX_FORMAT, FIFO_PAL_FORMAT, FIFO_BEGIN, FIFO_NOP);
	flag_list[++S] = 0;
	flag_list[++S] = 0;
	flag_list[++S] = GL_QUAD_STRIP;

	for (int i = 0; i <= n; i++)
	{
		float x = fcosf(2.f * PI * i / n) * 0.01f;
		float y = fsinf(2.f * PI * i / n) * 0.01f;

		flag_list[++S] = FIFO_COMMAND_PACK(FIFO_COLOR, FIFO_VERTEX16, FIFO_VERTEX_XY, FIFO_NOP);
		flag_list[++S] = RGB15(31, 31, 31);
		flag_list[++S] = VERTEX_PACK(floattov16(x / SCALE_VERTICES), 0);
		flag_list[++S] = VERTEX_PACK(floattov16(y / SCALE_VERTICES), 0);
		flag_list[++S] = VERTEX_PACK(floattov16(x / SCALE_VERTICES), floattov16(GOAL_HEIGHT / SCALE_VERTICES));
	}

	flag_list[++S] = FIFO_COMMAND_PACK(FIFO_END, FIFO_BEGIN, FIFO_COLOR, FIFO_NOP);
	flag_list[++S] = GL_TRIANGLES;
	flag_list[++S] = RGB15(31, 0, 0);

	flag_list[++S] = FIFO_COMMAND_PACK(FIFO_VERTEX16, FIFO_VERTEX_XY, FIFO_VERTEX_XY, FIFO_NOP);
	flag_list[++S] = VERTEX_PACK(0,                                               floattov16(GOAL_HEIGHT / SCALE_VERTICES));
	flag_list[++S] = VERTEX_PACK(0,                                               0);
	flag_list[++S] = VERTEX_PACK(floattov16(GOAL_HEIGHT * 0.2f / SCALE_VERTICES), floattov16(GOAL_HEIGHT * 0.9f / SCALE_VERTICES));
	flag_list[++S] = VERTEX_PACK(0,                                               floattov16(GOAL_HEIGHT * 0.8f / SCALE_VERTICES));

	flag_list[++S] = FIFO_COMMAND_PACK(FIFO_VERTEX_XY, FIFO_VERTEX_XY, FIFO_VERTEX_XY, FIFO_END);
	flag_list[++S] = VERTEX_PACK(0,                                               floattov16(GOAL_HEIGHT / SCALE_VERTICES));
	flag_list[++S] = VERTEX_PACK(0,                                               floattov16(GOAL_HEIGHT * 0.8f / SCALE_VERTICES));
	flag_list[++S] = VERTEX_PACK(floattov16(GOAL_HEIGHT * 0.2f / SCALE_VERTICES), floattov16(GOAL_HEIGHT * 0.9f / SCALE_VERTICES));

	flag_list[0] = S;
}

void flag_free(void)
{
	/*
	if (glIsList(flag_list))
		glDeleteLists(flag_list, 1);

	*/
	free(flag_list);
	flag_list = 0;
}

void flag_draw(void)
{
    glCallList(flag_list);
}

/*---------------------------------------------------------------------------*/
/*
 * A note about lighting and shadow: technically speaking, it's wrong.
 * The  light  position  and   shadow  projection  behave  as  if  the
 * light-source rotates with the  floor.  However, the skybox does not
 * rotate, thus the light should also remain stationary.
 *
 * The  correct behavior  would eliminate  a significant  3D  cue: the
 * shadow of  the ball indicates  the ball's position relative  to the
 * floor even  when the ball is  in the air.  This  was the motivating
 * idea  behind the  shadow  in  the first  place,  so correct  shadow
 * projection would only magnify the problem.
 */

static int shad_text;

void shad_init(void)
{
    shad_text = make_image_from_file(NULL, NULL, NULL, NULL, IMG_SHAD);

    /*
    if (config_get_d(CONFIG_SHADOW) == 2)
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    }
    */
}

void shad_free(void)
{
    //if (glIsTexture(shad_text))
        glDeleteTextures(1, &shad_text);
}

void shad_draw_set(const float *p, float r)
{
    glMatrixMode(GL_TEXTURE);
    {
        float k = 0.25f / r;

        glBindTexture(GL_TEXTURE_2D, shad_text);

        glLoadIdentity();
        glTranslatef((0.5f - k * p[0]) / SCALE_VERTICES,
                     (0.5f - k * p[2]) / SCALE_VERTICES, 0.f);
        glScalef(k, k, 1.0f);
    }
    glMatrixMode(GL_MODELVIEW);
}

void shad_draw_clr(void)
{
    glMatrixMode(GL_TEXTURE);
    {
        glLoadIdentity();
    }
    glMatrixMode(GL_MODELVIEW);
}

/*---------------------------------------------------------------------------*/

void fade_draw(float k)
{
	int w = config_get_d(CONFIG_WIDTH);
	int h = config_get_d(CONFIG_HEIGHT);

	if (k > 0.0f)
	{
		config_push_ortho();
		{
			glBindTexture(0, 0);
			
			glColor3b(0, 0, 0);
			int a = k*31;
			glPolyFmt(POLY_ID(62) | POLY_ALPHA(a) | POLY_CULL_NONE);

			glBegin(GL_QUADS);
			{
				int fw = divf32(inttof32(w), floattof32(SCALE_VERTICES));
				int fh = divf32(inttof32(h), floattof32(SCALE_VERTICES));
				glVertex3v16(           0,            0, 0);
				glVertex3v16(f32tov16(fw),            0, 0);
				glVertex3v16(f32tov16(fw), f32tov16(fh), 0);
				glVertex3v16(           0, f32tov16(fh), 0);
			}
			glEnd();

			glPolyFmt(POLY_ALPHA(31) | POLY_CULL_BACK);
		}
		config_pop_matrix();
	}
}

/*---------------------------------------------------------------------------*/
