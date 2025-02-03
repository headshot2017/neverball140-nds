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

#include <math.h>
#include <string.h>

#include "config.h"
#include "glext.h"
#include "vec3.h"
#include "back.h"
#include "image.h"

/*---------------------------------------------------------------------------*/

#define PI 3.1415926535897932

//static GLuint back_list;
static uint32_t* back_list;
static int back_text;
static int back_b;

/*---------------------------------------------------------------------------*/

void back_init(const char *s, int b)
{
	back_b = b;
	int slices = b ? 32 : 16;
	int stacks = b ? 16 :  8;

	back_free();
	back_text = make_image_from_file(NULL, NULL, NULL, NULL, s);

	glTexParameter(0, GL_TEXTURE_WRAP_S | TEXGEN_TEXCOORD | GL_TEXTURE_COLOR0_TRANSPARENT);

	// generate the display list
	back_list = (uint32_t*)malloc(sizeof(uint32_t) * 2048);
	back_list[0] = 0;

	uint32_t S = back_list[0];

	gl_texture_data *tex = (gl_texture_data*)DynamicArrayGet( &glGlob.texturePtrs, back_text );
	u16 palAddr = 0;

	int w = 8 << ((tex->texFormat >> 20 ) & 7 ); // from videoGL.h glGetInt()
	int h = 8 << ((tex->texFormat >> 23 ) & 7 ); // from videoGL.h glGetInt()

	if (tex->palIndex)
	{
		gl_palette_data *pal = (gl_palette_data*)DynamicArrayGet( &glGlob.palettePtrs, tex->palIndex );
		palAddr = pal->addr;
	}

	back_list[++S] = FIFO_COMMAND_PACK(FIFO_TEX_FORMAT, FIFO_PAL_FORMAT, FIFO_SPECULAR_EMISSION, FIFO_DIFFUSE_AMBIENT);
	back_list[++S] = tex->texFormat;
	back_list[++S] = palAddr;
	back_list[++S] = RGB15(0,0,0) + (RGB15(31,31,31) << 16);
	back_list[++S] = RGB15(31,31,31) + (RGB15(31,31,31) << 16);

	for (int i = 0; i < stacks; i++)
	{
		float k0 = (float)  i      / stacks;
		float k1 = (float) (i + 1) / stacks;

		float s0 = fsinf(V_PI * (k0 - 0.5f));
		float c0 = fcosf(V_PI * (k0 - 0.5f));
		float s1 = fsinf(V_PI * (k1 - 0.5f));
		float c1 = fcosf(V_PI * (k1 - 0.5f));

		back_list[++S] = FIFO_COMMAND_PACK(FIFO_BEGIN, FIFO_NOP, FIFO_NOP, FIFO_NOP);
		back_list[++S] = GL_QUAD_STRIP;

		for (int j = 0; j <= slices; j++)
		{
			float k = (float) j / slices;
			float s = fsinf(V_PI * k * 2.0f);
			float c = fcosf(V_PI * k * 2.0f);

			back_list[++S] = FIFO_COMMAND_PACK(FIFO_TEX_COORD, FIFO_NORMAL, FIFO_VERTEX16, FIFO_NOP);
			back_list[++S] = TEXTURE_PACK(floattot16(k * w), floattot16(k1 * h));
			back_list[++S] = NORMAL_PACK(floattov10(s * c1), floattov10(c * c1), floattov10(s1));
			back_list[++S] = VERTEX_PACK(floattov16(s * c1 / SCALE_VERTICES), floattov16(c * c1 / SCALE_VERTICES));
			back_list[++S] = VERTEX_PACK(floattov16(s1 / SCALE_VERTICES), 0);

			back_list[++S] = FIFO_COMMAND_PACK(FIFO_TEX_COORD, FIFO_NORMAL, FIFO_VERTEX16, FIFO_NOP);
			back_list[++S] = TEXTURE_PACK(floattot16(k * w), floattot16(k0 * h));
			back_list[++S] = NORMAL_PACK(floattov10(s * c0), floattov10(c * c0), floattov10(s0));
			back_list[++S] = VERTEX_PACK(floattov16(s * c0 / SCALE_VERTICES), floattov16(c * c0 / SCALE_VERTICES));
			back_list[++S] = VERTEX_PACK(floattov16(s0 / SCALE_VERTICES), 0);
		}

		back_list[++S] = FIFO_COMMAND_PACK(FIFO_END, FIFO_NOP, FIFO_NOP, FIFO_NOP);
	}
	back_list[0] = S;
	printf("back: %d %d %s\n", S, (stacks*3)*(slices*10), s);
}

void back_free(void)
{
	//if (glIsList(back_list))
		//glDeleteLists(back_list, 1);
	free(back_list);

	//if (glIsTexture(back_text))
		glDeleteTextures(1, &back_text);

	back_list = 0;
	back_text = 0;
}

void back_draw(float t)
{
	glPushMatrix();
	//glPushAttrib(GL_LIGHTING_BIT | GL_DEPTH_BUFFER_BIT);
	{
		float dx =  60.f * fsinf(t / 10.f) + 90.f;
		float dz = 180.f * fsinf(t / 12.f);

		//glEnable(GL_TEXTURE_2D);
		//glDisable(GL_LIGHTING);
		//glDepthMask(GL_FALSE);
		glPolyFmt(POLY_ALPHA(31) | POLY_CULL_BACK);
		
		//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		glScalef(BACK_DIST+32, BACK_DIST+32, BACK_DIST+32);
		glRotatef(dz, 0.f, 0.f, 1.f);
		glRotatef(dx, 1.f, 0.f, 0.f);

		glCallList(back_list);

		/*
		glBindTexture(GL_TEXTURE_2D, back_text);

		glMaterialf(GL_AMBIENT, RGB15(31,31,31));
		glMaterialf(GL_DIFFUSE, RGB15(31,31,31));
		glMaterialf(GL_SPECULAR, 0);
		glMaterialf(GL_EMISSION, RGB15(31,31,31));
		
		int slices = back_b ? 32 : 16;
		int stacks = back_b ? 16 :  8;
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

					glTexCoord2f(k, k1);
					glNormal3f(s * c1, c * c1, s1);
					glVertex3f(s * c1 / SCALE_VERTICES, c * c1 / SCALE_VERTICES, s1 / SCALE_VERTICES);

					glTexCoord2f(k, k0);
					glNormal3f(s * c0, c * c0, s0);
					glVertex3f(s * c0 / SCALE_VERTICES, c * c0 / SCALE_VERTICES, s0 / SCALE_VERTICES);
				}
			}
			glEnd();
		}
		*/
	}
	//glPopAttrib();
	glPopMatrix(1);
}

/*---------------------------------------------------------------------------*/
