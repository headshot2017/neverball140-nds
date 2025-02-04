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
#include <math.h>

#define STB_IMAGE_IMPLEMENTATION
#define STB_TRUETYPE_IMPLEMENTATION 

#include "glext.h"
#include "image.h"
#include "config.h"

#include "stb_image.h"


/*---------------------------------------------------------------------------*/

void image_snap(char *filename)
{
	/*
    int w = config_get_d(CONFIG_WIDTH);
    int h = config_get_d(CONFIG_HEIGHT);
    int i;

    SDL_Surface *buf = SDL_CreateRGBSurface(SDL_SWSURFACE, w, h, 24,
                                            RMASK, GMASK, BMASK, 0);
    SDL_Surface *img = SDL_CreateRGBSurface(SDL_SWSURFACE, w, h, 24,
                                            RMASK, GMASK, BMASK, 0);

    glReadPixels(0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, buf->pixels);

    for (i = 0; i < h; i++)
        memcpy((GLubyte *) img->pixels + 3 * w * i,
               (GLubyte *) buf->pixels + 3 * w * (h - i), 3 * w);

    SDL_SaveBMP(img, filename);

    SDL_FreeSurface(img);
    SDL_FreeSurface(buf);
	*/
}

void image_size(int *W, int *H, int w, int h)
{
    *W = 1;
    *H = 1;

    while (*W < w) *W *= 2;
    while (*H < h) *H *= 2;
}

#define RGB_to_DS(src) \
	((src[0] & 0xF8) >> 3) | ((src[1] & 0xF8) << 2) | ((src[2] & 0xF8) << 7) | ((255 & 0x80) << 8)

#define RGBA8_to_DS(src) \
	((src[0] & 0xF8) >> 3) | ((src[1] & 0xF8) << 2) | ((src[2] & 0xF8) << 7) | ((src[3] & 0x80) << 8)

static int FindColorInPalette(u16* pal, int pal_size, u16 col)
{
	if ((col >> 15) == 0) return 0;
	
	for (int i = 1; i < pal_size; i++) {
		if(pal[i] == col) return i;
	}
	
	return -1;
}

int image_palettize(int w, int h, int n, unsigned char* data, unsigned short* tmp, unsigned short* tmp_palette)
{
	// transfer to tmp
	for (int y = 0; y < h; y++)
	{
		for (int x = 0; x < w; x++)
		{
			u8* src = data + ((y * w + x) * n);
			u16* dst = tmp + (y * w + x);

			if (n == 3)
				*dst = RGB_to_DS(src);
			else
				*dst = RGBA8_to_DS(src);
		}
	}

	tmp_palette[0] = 0;
	int pal_size = 1;
	for (int i = 0; i < w * h; i++)
	{
		u16 col = tmp[i];
	
		int idx = FindColorInPalette(tmp_palette, pal_size, col);
		
		if (idx == -1) {
			pal_size++;
			if (pal_size > 256) break;
			tmp_palette[pal_size - 1] = col;
		}
	}

	int GLformat = (n == 3) ? GL_RGB : GL_RGBA;
	if(pal_size <= 4) GLformat = GL_RGB4;
	else if(pal_size <= 16) GLformat = GL_RGB16;
	else if(pal_size <= 256) GLformat = GL_RGB256;

	if(GLformat != GL_RGBA && GLformat != GL_RGB)
	{
		char* tmp_chr = (char*) tmp;
		
		for (int i = 0; i < w * h; i++)
		{
			u16 col = tmp[i];
			int idx = FindColorInPalette(tmp_palette, pal_size, col);
			
			if(GLformat == GL_RGB256) {
				tmp_chr[i] = idx;
			} else if(GLformat == GL_RGB16) {
				if((i & 1) == 0) {
					tmp_chr[i >> 1] = idx;
				} else {
					tmp_chr[i >> 1] |= idx << 4;
				}
			} else {
				if((i & 3) == 0) {
					tmp_chr[i >> 2] = idx;
				} else {
					tmp_chr[i >> 2] |= idx << (2 * (i & 3));
				}
			}
		}
	}

	return GLformat;
}

/*---------------------------------------------------------------------------*/

/*
void image_swab(SDL_Surface *src)
{
    int i, j, b = (src->format->BitsPerPixel == 32) ? 4 : 3;
    
    SDL_LockSurface(src);
    {
        unsigned char *s = (unsigned char *) src->pixels;
        unsigned char  t;

        // Iterate over each pixel of the image.

        for (i = 0; i < src->h; i++)
            for (j = 0; j < src->w; j++)
            {
                int k = (i * src->w + j) * b;

                // Swap the red and blue channels of each.

                t        = s[k + 2];
                s[k + 2] = s[k + 0];
                s[k + 0] =        t;
            }
    }
    SDL_UnlockSurface(src);
}

void image_white(SDL_Surface *src)
{
    int i, j, b = (src->format->BitsPerPixel == 32) ? 4 : 3;
    
    SDL_LockSurface(src);
    {
        unsigned char *s = (unsigned char *) src->pixels;

        // Iterate over each pixel of the image.

        for (i = 0; i < src->h; i++)
            for (j = 0; j < src->w; j++)
            {
                int k = (i * src->w + j) * b;

                // Whiten the RGB channels without touching any Alpha.

                s[k + 0] = 0xFF;
                s[k + 1] = 0xFF;
                s[k + 2] = 0xFF;
            }
    }
    SDL_UnlockSurface(src);
}

SDL_Surface *image_scale(SDL_Surface *src, int n)
{
    int si, di;
    int sj, dj;
    int k, b = (src->format->BitsPerPixel == 32) ? 4 : 3;

    SDL_Surface *dst = SDL_CreateRGBSurface(SDL_SWSURFACE,
                                            src->w / n,
                                            src->h / n,
                                            src->format->BitsPerPixel,
                                            src->format->Rmask,
                                            src->format->Gmask,
                                            src->format->Bmask,
                                            src->format->Amask);
    if (dst)
    {
        SDL_LockSurface(src);
        SDL_LockSurface(dst);
        {
            unsigned char *s = (unsigned char *) src->pixels;
            unsigned char *d = (unsigned char *) dst->pixels;

            // Iterate each component of each distination pixel.

            for (di = 0; di < src->h / n; di++)
                for (dj = 0; dj < src->w / n; dj++)
                    for (k = 0; k < b; k++)
                    {
                        int c = 0;

                        // Average the NxN source pixel block for each.

                        for (si = di * n; si < (di + 1) * n; si++)
                            for (sj = dj * n; sj < (dj + 1) * n; sj++)
                                c += s[(si * src->w + sj) * b + k];

                        d[(di * dst->w + dj) * b + k] =
                            (unsigned char) (c / (n * n));
                    }
        }
        SDL_UnlockSurface(dst);
        SDL_UnlockSurface(src);
    }

    return dst;
}
*/

/*---------------------------------------------------------------------------*/

/*
 * Create on  OpenGL texture  object using the  given SDL  surface and
 * format,  scaled  using the  current  scale  factor.  When  scaling,
 * assume dimensions are used only for layout and lie about the size.
 */
/*
GLuint make_image_from_surf(int *w, int *h, SDL_Surface *s)
{
	int    t = config_get_d(CONFIG_TEXTURES);
    GLuint o = 0;

    glGenTextures(1, &o);
    glBindTexture(GL_TEXTURE_2D, o);

    if (t > 1)
    {
        SDL_Surface *d = image_scale(s, t);

        // Load the scaled image.

        if (d->format->BitsPerPixel == 32)
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, d->w, d->h, 0,
                         GL_RGBA, GL_UNSIGNED_BYTE, d->pixels);
        else
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB,  d->w, d->h, 0,
                         GL_RGB,  GL_UNSIGNED_BYTE, d->pixels);

        SDL_FreeSurface(d);
    }
    else
    {
        // Load the unscaled image.

        if (s->format->BitsPerPixel == 32)
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, s->w, s->h, 0,
                         GL_RGBA, GL_UNSIGNED_BYTE, s->pixels);
        else
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB,  s->w, s->h, 0,
                         GL_RGB,  GL_UNSIGNED_BYTE, s->pixels);
    }

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    if (w) *w = s->w;
    if (h) *h = s->h;

    return o;
}
*/

/*---------------------------------------------------------------------------*/

/*
 * Load  an image  from the  named file.   If the  image is  not RGBA,
 * convert it to RGBA.  Return an OpenGL texture object.
 */
int make_image_from_file(int *W, int *H,
                            int *w, int *h, const char *name)
{
	int o = 0;
	int w2, h2, n;
	unsigned char *data = stbi_load(config_data(name), &w2, &h2, &n, 0);

	if (data)
	{
		if (w) *w = w2;
		if (h) *h = h2;
		if (W) *W = w2;
		if (H) *H = h2;

		u16* tmp = (u16*)malloc(w2 * h2 * sizeof(u16));
		if (!tmp)
		{
			printf("FAILED tmp\n");
			stbi_image_free(data);
			return 0;
		}
		u16* tmp_palette = (u16*)malloc(256*sizeof(u16));
		if (!tmp_palette)
		{
			printf("FAILED tmp_palette\n");
			stbi_image_free(data);
			free(tmp);
			return 0;
		}

		int GLformat = image_palettize(w2, h2, n, data, tmp, tmp_palette);
		stbi_image_free(data);

		glGenTextures(1, &o);
		glBindTexture(GL_TEXTURE_2D, o);
		printf("%d %d %d %d '%s'\n", w2, h2, n, GLformat, name);

		if (glTexImage2D(GL_TEXTURE_2D, 0, GLformat, w2, h2, 0, 0, tmp) == 0)
		{
			glDeleteTextures(1, &o);
			free(tmp);
			free(tmp_palette);
			printf("glTexImage2D failed %d %d %d '%s'\n", w2, h2, n, name);
			return 0;
		}

		if (GLformat != GL_RGBA && GLformat != GL_RGB)
		{
			int glPalSize;
			if(GLformat == GL_RGB4) glPalSize = 4;
			else if(GLformat == GL_RGB16) glPalSize = 16;
			else glPalSize = 256;

			glColorTableEXT(0, 0, glPalSize, 0, 0, tmp_palette);
		}
		
		glTexParameter(0, TEXGEN_TEXCOORD | GL_TEXTURE_COLOR0_TRANSPARENT);

		free(tmp);
		free(tmp_palette);

		/*
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		*/
	}
	else
		printf("failed '%s'\n", name);

	return o;
}

/*---------------------------------------------------------------------------*/

static int text_width(stbtt_fontinfo *font, float stbtt_scale, const char *text)
{
    int x = 0;
    for (int i=0; i<strlen(text); i++)
    {
        /* how wide is this character */
        int ax;
	    int lsb;
        stbtt_GetCodepointHMetrics(font, text[i], &ax, &lsb);
        /* (Note that each Codepoint call has an alternative Glyph version which caches the work required to lookup the character word[i].) */

        /* advance x */
        x += roundf(ax * stbtt_scale);
        
        /* add kerning */
        int kern;
        kern = stbtt_GetCodepointKernAdvance(font, text[i], text[i + 1]);
        x += roundf(kern * stbtt_scale);
    }
    return x;
}

/*
 * Render the given  string using the given font.   Transfer the image
 * to a  surface of  power-of-2 size large  enough to fit  the string.
 * Return an OpenGL texture object.
 */
int make_image_from_font(int *W, int *H,
                            int *w, int *h, const char *text, stbtt_fontinfo *font, float stbtt_scale, int h2, int k)
{
    int o = 0;

    if (text && strlen(text) > 0)
    {
        int ascent, descent, lineGap;
        stbtt_GetFontVMetrics(font, &ascent, &descent, &lineGap);
    
        ascent = roundf(ascent * stbtt_scale);

        int w2 = text_width(font, stbtt_scale, text);

        int realW, realH;
		image_size(&realW, &realH, w2, h2);
        unsigned char* bitmap = calloc(1, realW * realH);

        if (w) *w = w2;
        if (h) *h = h2;

        int x = (realW - w2) / 2;
        int startY = (realH - h2) / 2;

        for (int i=0; i<strlen(text); i++)
        {
            /* how wide is this character */
            int ax;
	        int lsb;
            stbtt_GetCodepointHMetrics(font, text[i], &ax, &lsb);
            /* (Note that each Codepoint call has an alternative Glyph version which caches the work required to lookup the character word[i].) */

            /* get bounding box for character (may be offset to account for chars that dip above or below the line) */
            int c_x1, c_y1, c_x2, c_y2;
            stbtt_GetCodepointBitmapBox(font, text[i], stbtt_scale, stbtt_scale, &c_x1, &c_y1, &c_x2, &c_y2);
        
            /* compute y (different characters have different heights) */
            int y = startY + ascent + c_y1;
        
            /* render character (stride and offset is important here) */
            int byteOffset = x + roundf(lsb * stbtt_scale) + (y * realW);
            stbtt_MakeCodepointBitmap(font, bitmap + byteOffset, c_x2 - c_x1, c_y2 - c_y1, realW, stbtt_scale, stbtt_scale, text[i]);

            /* advance x */
            x += roundf(ax * stbtt_scale);
        
            /* add kerning */
            int kern;
            kern = stbtt_GetCodepointKernAdvance(font, text[i], text[i + 1]);
            x += roundf(kern * stbtt_scale);
        }

		u16* tmp = (u16*)malloc(realW * realH * sizeof(u16));
		if (!tmp)
		{
			printf("FAILED tmp\n");
			free(bitmap);
			return 0;
		}
		u16* tmp_palette = (u16*)malloc(256*sizeof(u16));
		if (!tmp_palette)
		{
			printf("FAILED tmp_palette\n");
			free(bitmap);
			free(tmp);
			return 0;
		}

		tmp_palette[0] = 0;
		tmp_palette[1] = RGB15(31,31,31);

		char* tmp_chr = (char*) tmp;
		for (int i = 0; i < realW * realH; i++)
		{
			int idx = (bitmap[i] < 96) ? 0 : 1;

			if((i & 3) == 0)
				tmp_chr[i >> 2] = idx;
			else
				tmp_chr[i >> 2] |= idx << (2 * (i & 3));
		}

		glGenTextures(1, &o);
		glBindTexture(GL_TEXTURE_2D, o);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB4, realW, realH, 0, TEXGEN_TEXCOORD | GL_TEXTURE_COLOR0_TRANSPARENT, tmp);
		glColorTableEXT(0, 0, 4, 0, 0, tmp_palette);

		/*
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		*/

		free(bitmap);
        free(tmp);
        free(tmp_palette);

        if (W) *W *= k;
        if (H) *H *= k;
        if (w) *w *= k;
        if (h) *h *= k;
    }
    else
    {
        if (W) *W = 0;
        if (H) *H = 0;
        if (w) *w = 0;
        if (h) *h = 0;
    }

    return o;
}

/*---------------------------------------------------------------------------*/
