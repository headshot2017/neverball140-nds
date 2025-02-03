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
#include <stdio.h>

#include "config.h"
#include "glext.h"
#include "image.h"
#include "vec3.h"
#include "gui.h"

#include "stb_truetype.h"

/*---------------------------------------------------------------------------*/

#define MAXWIDGET 256

#define GUI_TYPE 0xFFFE

#define GUI_FREE   0
#define GUI_STATE  1
#define GUI_HARRAY 2
#define GUI_VARRAY 4
#define GUI_HSTACK 6
#define GUI_VSTACK 8
#define GUI_FILLER 10
#define GUI_IMAGE  12
#define GUI_LABEL  14
#define GUI_COUNT  16
#define GUI_CLOCK  18
#define GUI_SPACE  20
#define GUI_PAUSE  22

struct widget
{
    int     type;
    int     token;
    int     value;
    int     size;
    int     rect;

    int     x, y;
    int     w, h;
    int     car;
    int     cdr;

    int     text_img;
    uint32_t*  text_obj;
    uint32_t*  rect_obj;

    const GLfloat *color0;
    const GLfloat *color1;

    GLfloat  scale;
};

/*---------------------------------------------------------------------------*/

const GLfloat gui_wht[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
const GLfloat gui_yel[4] = { 1.0f, 1.0f, 0.0f, 1.0f };
const GLfloat gui_red[4] = { 1.0f, 0.0f, 0.0f, 1.0f };
const GLfloat gui_grn[4] = { 0.0f, 1.0f, 0.0f, 1.0f };
const GLfloat gui_blu[4] = { 0.0f, 0.0f, 1.0f, 1.0f };
const GLfloat gui_blk[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
const GLfloat gui_gry[4] = { 0.3f, 0.3f, 0.3f, 1.0f };

/*---------------------------------------------------------------------------*/

struct FontScale
{
    int lineHeight;
    float scale;
};

static struct widget  widget[MAXWIDGET];
static int            active;
static int            radius;
//static TTF_Font     *font[3] = { NULL, NULL, NULL };
static stbtt_fontinfo font;
static unsigned char *font_buffer = 0;
static struct FontScale font_scales[3] = { {0}, {0}, {0} };
static int            scale[3] = { 1, 1, 1 };

static int digit_text[3][11];
static uint32_t* digit_list[3][11];
static int    digit_w[3][11];
static int    digit_h[3][11];

static int pause_id;

/*---------------------------------------------------------------------------*/

static int gui_hot(int id)
{
    return (widget[id].type & GUI_STATE);
}

/*---------------------------------------------------------------------------*/
/*
 * Initialize a  display list  containing a  rectangle (x, y, w, h) to
 * which a  rendered-font texture  may be applied.   Colors  c0 and c1
 * determine the top-to-bottom color gradiant of the text.
 */

static uint32_t* gui_list(int x, int y,
                          int w, int h, const float *c0, const float *c1)
{
    uint32_t* list = (uint32_t*)malloc(sizeof(uint32_t) * 512);

    GLfloat s0, t0;
    GLfloat s1, t1;

    int W, H, d = h / 16;

    // Assume the applied texture size is rect size rounded to power-of-two.

    image_size(&W, &H, w, h);

    s0 = 0.5f * (W - w) / W;
    t0 = 0.5f * (H - h) / H;
    s1 = 1.0f - s0;
    t1 = 1.0f - t0;

    list[0] = 0;
	uint32_t S = list[0];
    {
        list[++S] = FIFO_COMMAND_PACK(FIFO_POLY_FORMAT, FIFO_BEGIN, FIFO_NOP, FIFO_NOP);
        list[++S] = POLY_ID(63) | POLY_ALPHA(31) | POLY_CULL_NONE;
        list[++S] = GL_QUADS;
        {
            list[++S] = FIFO_COMMAND_PACK(FIFO_COLOR, FIFO_NOP, FIFO_NOP, FIFO_NOP);
            list[++S] = 0;

            list[++S] = FIFO_COMMAND_PACK(FIFO_TEX_COORD, FIFO_VERTEX16, FIFO_TEX_COORD, FIFO_VERTEX_XY);
            list[++S] = TEXTURE_PACK(floattot16(s0 * W), floattot16(t1 * H));
            list[++S] = VERTEX_PACK(floattov16((x + d) / SCALE_VERTICES), floattov16((y - d) / SCALE_VERTICES));
            list[++S] = VERTEX_PACK(0, 0);
            list[++S] = TEXTURE_PACK(floattot16(s1 * W), floattot16(t1 * H));
            list[++S] = VERTEX_PACK(floattov16((x + w + d) / SCALE_VERTICES), floattov16((y - d) / SCALE_VERTICES));

            list[++S] = FIFO_COMMAND_PACK(FIFO_TEX_COORD, FIFO_VERTEX_XY, FIFO_TEX_COORD, FIFO_VERTEX_XY);
            list[++S] = TEXTURE_PACK(floattot16(s1 * W), floattot16(t0 * H));
            list[++S] = VERTEX_PACK(floattov16((x + w + d) / SCALE_VERTICES), floattov16((y + h - d) / SCALE_VERTICES));
            list[++S] = TEXTURE_PACK(floattot16(s0 * W), floattot16(t0 * H));
            list[++S] = VERTEX_PACK(floattov16((x + d) / SCALE_VERTICES), floattov16((y + h - d) / SCALE_VERTICES));

            list[++S] = FIFO_COMMAND_PACK(FIFO_COLOR, FIFO_NOP, FIFO_NOP, FIFO_NOP);
            list[++S] = RGB15((int)(c0[0] * 31), (int)(c0[1] * 31), (int)(c0[2] * 31));
            //list[++S] = POLY_ID(63) | POLY_ALPHA((int)(c0[3] * 31)) | POLY_CULL_NONE;

            list[++S] = FIFO_COMMAND_PACK(FIFO_TEX_COORD, FIFO_VERTEX16, FIFO_TEX_COORD, FIFO_VERTEX_XY);
            list[++S] = TEXTURE_PACK(floattot16(s0 * W), floattot16(t1 * H));
            list[++S] = VERTEX_PACK(floattov16((x) / SCALE_VERTICES), floattov16((y) / SCALE_VERTICES));
            list[++S] = VERTEX_PACK(floattov16(8 / SCALE_VERTICES), 0);
            list[++S] = TEXTURE_PACK(floattot16(s1 * W), floattot16(t1 * H));
            list[++S] = VERTEX_PACK(floattov16((x + w) / SCALE_VERTICES), floattov16((y) / SCALE_VERTICES));

            list[++S] = FIFO_COMMAND_PACK(FIFO_COLOR, FIFO_NOP, FIFO_NOP, FIFO_NOP);
            list[++S] = RGB15((int)(c1[0] * 31), (int)(c1[1] * 31), (int)(c1[2] * 31));
            //list[++S] = POLY_ID(63) | POLY_ALPHA((int)(c1[3] * 31)) | POLY_CULL_NONE;

            list[++S] = FIFO_COMMAND_PACK(FIFO_TEX_COORD, FIFO_VERTEX_XY, FIFO_TEX_COORD, FIFO_VERTEX_XY);
            list[++S] = TEXTURE_PACK(floattot16(s1 * W), floattot16(t0 * H));
            list[++S] = VERTEX_PACK(floattov16((x + w) / SCALE_VERTICES), floattov16((y + h) / SCALE_VERTICES));
            list[++S] = TEXTURE_PACK(floattot16(s0 * W), floattot16(t0 * H));
            list[++S] = VERTEX_PACK(floattov16((x) / SCALE_VERTICES), floattov16((y + h) / SCALE_VERTICES));

            /*
            glColor4f(0.0f, 0.0f, 0.0f, 0.5f);
            glTexCoord2f(s0, t1); glVertex2i(x     + d, y     - d);
            glTexCoord2f(s1, t1); glVertex2i(x + w + d, y     - d);
            glTexCoord2f(s1, t0); glVertex2i(x + w + d, y + h - d);
            glTexCoord2f(s0, t0); glVertex2i(x     + d, y + h - d);

            glColor4fv(c0);
            glTexCoord2f(s0, t1); glVertex2i(x,     y);
            glTexCoord2f(s1, t1); glVertex2i(x + w, y);

            glColor4fv(c1);
            glTexCoord2f(s1, t0); glVertex2i(x + w, y + h);
            glTexCoord2f(s0, t0); glVertex2i(x,     y + h);
            */
        }
        list[++S] = FIFO_COMMAND_PACK(FIFO_END, FIFO_NOP, FIFO_NOP, FIFO_NOP);
    }
    list[0] = S;

    return list;
}

/*
 * Initialize a display list containing a rounded-corner rectangle (x,
 * y, w, h).  Generate texture coordinates to properly apply a texture
 * map to the rectangle as though the corners were not rounded.
 */

static uint32_t* gui_rect(int x, int y, int w, int h, int f, int r)
{
    uint32_t* list = (uint32_t*)malloc(sizeof(uint32_t) * 512);
    int n = 8;
    int i;

	list[0] = 0;
	uint32_t S = list[0];
    {
        list[++S] = FIFO_COMMAND_PACK(FIFO_BEGIN, FIFO_NOP, FIFO_NOP, FIFO_NOP);
        list[++S] = GL_QUAD_STRIP;
        {
            // Left side...

            for (i = 0; i <= n; i++)
            {
                float a = 0.5f * V_PI * (float) i / (float) n;
                float s = r * fsinf(a);
                float c = r * fcosf(a);

                float X  = x     + r - c;
                float Ya = y + h + ((f & GUI_NW) ? (s - r) : 0);
                float Yb = y     + ((f & GUI_SW) ? (r - s) : 0);

                list[++S] = FIFO_COMMAND_PACK(FIFO_TEX_COORD, FIFO_VERTEX16, FIFO_TEX_COORD, FIFO_VERTEX_YZ);

                list[++S] = TEXTURE_PACK(floattot16(0), floattot16(0));
                list[++S] = VERTEX_PACK(floattov16(X / SCALE_VERTICES), floattov16(Ya / SCALE_VERTICES));
                list[++S] = VERTEX_PACK(0,0);

                list[++S] = TEXTURE_PACK(floattot16(0), floattot16(h));
                list[++S] = VERTEX_PACK(floattov16(Yb / SCALE_VERTICES), 0);
                //glTexCoord2f((X - x) / w, 1 - (Ya - y) / h);
                //glVertex2f(X, Ya);

                //glTexCoord2f((X - x) / w, 1 - (Yb - y) / h);
                //glVertex2f(X, Yb);
            }

            // ... Right side.

            for (i = 0; i <= n; i++)
            {
                float a = 0.5f * V_PI * (float) i / (float) n;
                float s = r * fsinf(a);
                float c = r * fcosf(a);

                float X  = x + w - r + s;
                float Ya = y + h + ((f & GUI_NE) ? (c - r) : 0);
                float Yb = y     + ((f & GUI_SE) ? (r - c) : 0);

                list[++S] = FIFO_COMMAND_PACK(FIFO_TEX_COORD, FIFO_VERTEX16, FIFO_TEX_COORD, FIFO_VERTEX_YZ);

                list[++S] = TEXTURE_PACK(floattot16(X - x), floattot16(h - (Ya - y)));
                list[++S] = VERTEX_PACK(floattov16(X / SCALE_VERTICES), floattov16(Ya / SCALE_VERTICES));
                list[++S] = VERTEX_PACK(0,0);

                list[++S] = TEXTURE_PACK(floattot16(X - x), floattot16(h - (Yb - y)));
                list[++S] = VERTEX_PACK(floattov16(Yb / SCALE_VERTICES), 0);

                //glTexCoord2f((X - x) / w, 1 - (Ya - y) / h);
                //glVertex2f(X, Ya);

                //glTexCoord2f((X - x) / w, 1 - (Yb - y) / h);
                //glVertex2f(X, Yb);
            }
        }
        list[++S] = FIFO_COMMAND_PACK(FIFO_END, FIFO_NOP, FIFO_NOP, FIFO_NOP);
    }
    list[0] = S;
    if (S > 512) printf("gui rect exceed %d\n", S);

    return list;
}

/*---------------------------------------------------------------------------*/

void gui_init(void)
{
    const float *c0 = gui_yel;
    const float *c1 = gui_red;

    int w = config_get_d(CONFIG_WIDTH);
    int h = config_get_d(CONFIG_HEIGHT);
    int i, j, s = (h < w) ? h : w;

    /* Initialize font rendering. */

    //if (TTF_Init() == 0)
    {
        int s0 = s / 18;
        int s1 = s / 12;
        int s2 = s /  6;
        int m;

        /* Make sure text size doesn't exceed the maximum texture size. */

        //glGetIntegerv(GL_MAX_TEXTURE_SIZE, &m);
        m = 512;

        scale[0] = 1;
        scale[1] = 1;
        scale[2] = 1;

        while (s0 > m) { s0 /= 2; scale[0] *= 2; }
        while (s1 > m) { s1 /= 2; scale[1] *= 2; }
        while (s2 > m) { s2 /= 2; scale[2] *= 2; }

        memset(widget, 0, sizeof (struct widget) * MAXWIDGET);

        /* Load small, medium, and large typefaces. */

        /*
        font[GUI_SML] = TTF_OpenFont(config_data(GUI_FACE), s0);
        font[GUI_MED] = TTF_OpenFont(config_data(GUI_FACE), s1);
        font[GUI_LRG] = TTF_OpenFont(config_data(GUI_FACE), s2);
        */

        if (!font_buffer)
        {
            FILE* f = fopen(config_data(GUI_FACE), "rb");
            fseek(f, 0, SEEK_END);
            size_t size = ftell(f);
            fseek(f, 0, SEEK_SET);
            font_buffer = (unsigned char*)malloc(size);
            fread(font_buffer, size, 1, f);
            fclose(f);
            stbtt_InitFont(&font, font_buffer, 0);
        }

		font_scales[GUI_SML] = (struct FontScale){s0, stbtt_ScaleForPixelHeight(&font, s0)};
		font_scales[GUI_MED] = (struct FontScale){s1, stbtt_ScaleForPixelHeight(&font, s1)};
		font_scales[GUI_LRG] = (struct FontScale){s2, stbtt_ScaleForPixelHeight(&font, s2)};

        radius = s / 60;

        /* Initialize the global pause GUI. */

        if ((pause_id = gui_pause(0)))
            gui_layout(pause_id, 0, 0);

        /* Initialize digit glyphs and lists for counters and clocks. */

        for (i = 0; i < 3; i++)
        {
            char text[2];

            /* Draw digits 0 throught 9. */

            for (j = 0; j < 10; j++)
            {
                text[0] = '0' + (char) j;
                text[1] =  0;

                digit_text[i][j] = make_image_from_font(NULL, NULL,
                                                        &digit_w[i][j],
                                                        &digit_h[i][j],
                                                        text, &font, font_scales[i].scale, font_scales[i].lineHeight, scale[i]);
                digit_list[i][j] = gui_list(-digit_w[i][j] / 2,
                                            -digit_h[i][j] / 2,
                                            +digit_w[i][j],
                                            +digit_h[i][j], c0, c1);
            }

            /* Draw the colon for the clock. */

            digit_text[i][j] = make_image_from_font(NULL, NULL,
                                                    &digit_w[i][10],
                                                    &digit_h[i][10],
                                                    ":", &font, font_scales[i].scale, font_scales[i].lineHeight, scale[i]);
            digit_list[i][j] = gui_list(-digit_w[i][10] / 2,
                                        -digit_h[i][10] / 2,
                                        +digit_w[i][10],
                                        +digit_h[i][10], c0, c1);
        }
    }

    active = 0;
}

void gui_free(void)
{
    int i, j, id;

    /* Release any remaining widget texture and display list indices. */

    for (id = 1; id < MAXWIDGET; id++)
    {
        //if (glIsTexture(widget[id].text_img))
            glDeleteTextures(1, &widget[id].text_img);

        //if (glIsList(widget[id].text_obj))
            //glDeleteLists(widget[id].text_obj, 1);
        //if (glIsList(widget[id].rect_obj))
            //glDeleteLists(widget[id].rect_obj, 1);

        if (widget[id].text_obj)
            free(widget[id].text_obj);
        if (widget[id].rect_obj)
            free(widget[id].rect_obj);

        widget[id].type     = GUI_FREE;
        widget[id].text_img = 0;
        widget[id].text_obj = 0;
        widget[id].rect_obj = 0;
        widget[id].cdr      = 0;
        widget[id].car      = 0;
    }

    /* Release all digit textures and display lists. */

    for (i = 0; i < 3; i++)
        for (j = 0; j < 11; j++)
        {
            //if (glIsTexture(digit_text[i][j]))
                glDeleteTextures(1, &digit_text[i][j]);

            //if (glIsList(digit_list[i][j]))
                //glDeleteLists(digit_list[i][j], 1);
            if (digit_list[i][j])
                free(digit_list[i][j]);
            digit_list[i][j] = 0;
        }

    /* Release all loaded fonts and finalize font rendering. */

    /*
    if (font[GUI_LRG]) TTF_CloseFont(font[GUI_LRG]);
    if (font[GUI_MED]) TTF_CloseFont(font[GUI_MED]);
    if (font[GUI_SML]) TTF_CloseFont(font[GUI_SML]);

    TTF_Quit();
    */

    free(font_buffer);
	font_buffer = 0;
}

/*---------------------------------------------------------------------------*/

static int gui_widget(int pd, int type)
{
    int id;

    /* Find an unused entry in the widget table. */

    for (id = 1; id < MAXWIDGET; id++)
        if (widget[id].type == GUI_FREE)
        {
            /* Set the type and default properties. */

            widget[id].type     = type;
            widget[id].token    = 0;
            widget[id].value    = 0;
            widget[id].size     = 0;
            widget[id].rect     = GUI_NW | GUI_SW | GUI_NE | GUI_SE;
            widget[id].w        = 0;
            widget[id].h        = 0;
            widget[id].text_img = 0;
            widget[id].text_obj = 0;
            widget[id].rect_obj = 0;
            widget[id].color0   = gui_wht;
            widget[id].color1   = gui_wht;
            widget[id].scale    = 1.0f;

            /* Insert the new widget into the parents's widget list. */

            if (pd)
            {
                widget[id].car = 0;
                widget[id].cdr = widget[pd].car;
                widget[pd].car = id;
            }
            else
            {
                widget[id].car = 0;
                widget[id].cdr = 0;
            }

            return id;
        }

    fprintf(stderr, "Out of widget IDs\n");

    return 0;
}

int gui_harray(int pd) { return gui_widget(pd, GUI_HARRAY); }
int gui_varray(int pd) { return gui_widget(pd, GUI_VARRAY); }
int gui_hstack(int pd) { return gui_widget(pd, GUI_HSTACK); }
int gui_vstack(int pd) { return gui_widget(pd, GUI_VSTACK); }
int gui_filler(int pd) { return gui_widget(pd, GUI_FILLER); }

/*---------------------------------------------------------------------------*/

void gui_set_image(int id, const char *file)
{
    //if (glIsTexture(widget[id].text_img))
        glDeleteTextures(1, &widget[id].text_img);

    widget[id].text_img = make_image_from_file(NULL, NULL, NULL, NULL, file);
}

void gui_set_label(int id, const char *text)
{
    int w, h;

    //if (glIsTexture(widget[id].text_img))
        glDeleteTextures(1, &widget[id].text_img);
    //if (glIsList(widget[id].text_obj))
        //glDeleteLists(widget[id].text_obj, 1);
    if (widget[id].text_obj)
        free(widget[id].text_obj);

    widget[id].text_img = make_image_from_font(NULL, NULL, &w, &h,
                                               text, &font, font_scales[widget[id].size].scale, font_scales[widget[id].size].lineHeight,
                                                    scale[widget[id].size]);
    widget[id].text_obj = gui_list(-w / 2, -h / 2, w, h,
                                   widget[id].color0, widget[id].color1);
}

void gui_set_count(int id, int value)
{
    widget[id].value = value;
}

void gui_set_clock(int id, int value)
{
    widget[id].value = value;
}

void gui_set_color(int id, const float *c0,
                           const float *c1)
{
    widget[id].color0 = c0 ? c0 : gui_yel;
    widget[id].color1 = c1 ? c1 : gui_red;
}

void gui_set_multi(int id, const char *text)
{
    const char *p;

    char s[8][MAXSTR];
    int  i, j, jd;

    size_t n = 0;

    /* Copy each delimited string to a line buffer. */

    for (p = text, j = 0; *p && j < 8; j++)
    {
        strncpy(s[j], p, (n = strcspn(p, "\\")));
        s[j][n] = 0;

        if (*(p += n) == '\\') p++;
    }

    /* Set the label value for each line. */

    for (i = j - 1, jd = widget[id].car; i >= 0 && jd; i--, jd = widget[jd].cdr)
        gui_set_label(jd, s[i]);
}

/*---------------------------------------------------------------------------*/

int gui_image(int pd, const char *file, int w, int h)
{
    int id;

    if ((id = gui_widget(pd, GUI_IMAGE)))
    {
        widget[id].text_img = make_image_from_file(NULL, NULL,
                                                   NULL, NULL, file);
        widget[id].w     = w;
        widget[id].h     = h;
    }
    return id;
}

int gui_start(int pd, const char *text, int size, int token, int value)
{
    int id;

    if ((id = gui_state(pd, text, size, token, value)))
        active = id;

    return id;
}

int gui_state(int pd, const char *text, int size, int token, int value)
{
    int id;

    if ((id = gui_widget(pd, GUI_STATE)))
    {
        widget[id].text_img = make_image_from_font(NULL, NULL,
                                                   &widget[id].w,
                                                   &widget[id].h,
                                                   text, &font, font_scales[size].scale, font_scales[size].lineHeight,
                                                        scale[size]);
        widget[id].size  = size;
        widget[id].token = token;
        widget[id].value = value;
    }
    return id;
}

int gui_label(int pd, const char *text, int size, int rect, const float *c0,
                                                            const float *c1)
{
    int id;

    if ((id = gui_widget(pd, GUI_LABEL)))
    {
        widget[id].text_img = make_image_from_font(NULL, NULL,
                                                   &widget[id].w,
                                                   &widget[id].h,
                                                   text, &font, font_scales[size].scale, font_scales[size].lineHeight,
                                                        scale[size]);
        widget[id].size   = size;
        widget[id].color0 = c0 ? c0 : gui_yel;
        widget[id].color1 = c1 ? c1 : gui_red;
        widget[id].rect   = rect;
    }
    return id;
}

int gui_count(int pd, int value, int size, int rect)
{
    int i, id;

    if ((id = gui_widget(pd, GUI_COUNT)))
    {
        for (i = value; i; i /= 10)
            widget[id].w += digit_w[size][0];

        widget[id].h      = digit_h[size][0];
        widget[id].value  = value;
        widget[id].size   = size;
        widget[id].color0 = gui_yel;
        widget[id].color1 = gui_red;
        widget[id].rect   = rect;
    }
    return id;
}

int gui_clock(int pd, int value, int size, int rect)
{
    int id;

    if ((id = gui_widget(pd, GUI_CLOCK)))
    {
        widget[id].w      = digit_w[size][0] * 6;
        widget[id].h      = digit_h[size][0];
        widget[id].value  = value;
        widget[id].size   = size;
        widget[id].color0 = gui_yel;
        widget[id].color1 = gui_red;
        widget[id].rect   = rect;
    }
    return id;
}

int gui_space(int pd)
{
    int id;

    if ((id = gui_widget(pd, GUI_SPACE)))
    {
        widget[id].w = 0;
        widget[id].h = 0;
    }
    return id;
}

int gui_pause(int pd)
{
    const char *text = "Paused";
    int id;

    if ((id = gui_widget(pd, GUI_PAUSE)))
    {
        widget[id].text_img = make_image_from_font(NULL, NULL,
                                                   &widget[id].w,
                                                   &widget[id].h,
                                                   text, &font, font_scales[GUI_LRG].scale, font_scales[GUI_LRG].lineHeight,
                                                        scale[GUI_LRG]);
        widget[id].color0 = gui_wht;
        widget[id].color1 = gui_wht;
        widget[id].value  = 0;
        widget[id].size   = GUI_LRG;
        widget[id].rect   = GUI_ALL;
    }
    return id;
}

/*---------------------------------------------------------------------------*/
/*
 * Create  a multi-line  text box  using a  vertical array  of labels.
 * Parse the  text for '\'  characters and treat them  as line-breaks.
 * Preserve the rect specifation across the entire array.
 */

int gui_multi(int pd, const char *text, int size, int rect, const float *c0,
                                                            const float *c1)
{
    int id = 0;

    if (text && (id = gui_varray(pd)))
    {
        const char *p;

        char s[8][MAXSTR];
        int  r[8];
        int  i, j;

        size_t n = 0;

        /* Copy each delimited string to a line buffer. */

        for (p = text, j = 0; *p && j < 8; j++)
        {
            strncpy(s[j], p, (n = strcspn(p, "\\")));
            s[j][n] = 0;
            r[j]    = 0;

            if (*(p += n) == '\\') p++;
        }

        /* Set the curves for the first and last lines. */

        if (j > 0)
        {
            r[0]     |= rect & (GUI_NW | GUI_NE);
            r[j - 1] |= rect & (GUI_SW | GUI_SE);
        }

        /* Create a label widget for each line. */

        for (i = 0; i < j; i++)
            gui_label(id, s[i], size, r[i], c0, c1);
    }
    return id;
}

/*---------------------------------------------------------------------------*/
/*
 * The bottom-up pass determines the area of all widgets.  The minimum
 * width  and height of  a leaf  widget is  given by  the size  of its
 * contents.   Array  and  stack   widths  and  heights  are  computed
 * recursively from these.
 */

static void gui_widget_up(int id);

static void gui_harray_up(int id)
{
    int jd, c = 0;

    /* Find the widest child width and the highest child height. */

    for (jd = widget[id].car; jd; jd = widget[jd].cdr)
    {
        gui_widget_up(jd);

        if (widget[id].h < widget[jd].h)
            widget[id].h = widget[jd].h;
        if (widget[id].w < widget[jd].w)
            widget[id].w = widget[jd].w;

        c++;
    }

    /* Total width is the widest child width times the child count. */

    widget[id].w *= c;
}

static void gui_varray_up(int id)
{
    int jd, c = 0;

    /* Find the widest child width and the highest child height. */

    for (jd = widget[id].car; jd; jd = widget[jd].cdr)
    {
        gui_widget_up(jd);

        if (widget[id].h < widget[jd].h)
            widget[id].h = widget[jd].h;
        if (widget[id].w < widget[jd].w)
            widget[id].w = widget[jd].w;

        c++;
    }

    /* Total height is the highest child height times the child count. */

    widget[id].h *= c;
}

static void gui_hstack_up(int id)
{
    int jd;

    /* Find the highest child height.  Sum the child widths. */

    for (jd = widget[id].car; jd; jd = widget[jd].cdr)
    {
        gui_widget_up(jd);

        if (widget[id].h < widget[jd].h)
            widget[id].h = widget[jd].h;

        widget[id].w += widget[jd].w;
    }
}

static void gui_vstack_up(int id)
{
    int jd;

    /* Find the widest child width.  Sum the child heights. */

    for (jd = widget[id].car; jd; jd = widget[jd].cdr)
    {
        gui_widget_up(jd);

        if (widget[id].w < widget[jd].w)
            widget[id].w = widget[jd].w;

        widget[id].h += widget[jd].h;
    }
}

static void gui_paused_up(int id)
{
    /* Store width and height for later use in text rendering. */

    widget[id].x = widget[id].w;
    widget[id].y = widget[id].h;

    /* The pause widget fills the screen. */

    widget[id].w = config_get_d(CONFIG_WIDTH);
    widget[id].h = config_get_d(CONFIG_HEIGHT);
}

static void gui_button_up(int id)
{
    /* Store width and height for later use in text rendering. */

    widget[id].x = widget[id].w;
    widget[id].y = widget[id].h;

    if (widget[id].w < widget[id].h && widget[id].w > 0)
        widget[id].w = widget[id].h;


    /* Padded text elements look a little nicer. */

    if (widget[id].w < config_get_d(CONFIG_WIDTH))
        widget[id].w += radius;
    if (widget[id].h < config_get_d(CONFIG_HEIGHT))
        widget[id].h += radius;
}

static void gui_widget_up(int id)
{
    if (id)
        switch (widget[id].type & GUI_TYPE)
        {
        case GUI_HARRAY: gui_harray_up(id); break;
        case GUI_VARRAY: gui_varray_up(id); break;
        case GUI_HSTACK: gui_hstack_up(id); break;
        case GUI_VSTACK: gui_vstack_up(id); break;
        case GUI_PAUSE:  gui_paused_up(id); break;
        default:         gui_button_up(id); break;
        }
}

/*---------------------------------------------------------------------------*/
/*
 * The  top-down layout  pass distributes  available area  as computed
 * during the bottom-up pass.  Widgets  use their area and position to
 * initialize rendering state.
 */

static void gui_widget_dn(int id, int x, int y, int w, int h);

static void gui_harray_dn(int id, int x, int y, int w, int h)
{
    int jd, i = 0, c = 0;

    widget[id].x = x;
    widget[id].y = y;
    widget[id].w = w;
    widget[id].h = h;

    /* Count children. */

    for (jd = widget[id].car; jd; jd = widget[jd].cdr)
        c += 1;

    /* Distribute horizontal space evenly to all children. */

    for (jd = widget[id].car; jd; jd = widget[jd].cdr, i++)
    {
        int x0 = x +  i      * w / c;
        int x1 = x + (i + 1) * w / c;

        gui_widget_dn(jd, x0, y, x1 - x0, h);
    }
}

static void gui_varray_dn(int id, int x, int y, int w, int h)
{
    int jd, i = 0, c = 0;

    widget[id].x = x;
    widget[id].y = y;
    widget[id].w = w;
    widget[id].h = h;

    /* Count children. */

    for (jd = widget[id].car; jd; jd = widget[jd].cdr)
        c += 1;

    /* Distribute vertical space evenly to all children. */

    for (jd = widget[id].car; jd; jd = widget[jd].cdr, i++)
    {
        int y0 = y +  i      * h / c;
        int y1 = y + (i + 1) * h / c;

        gui_widget_dn(jd, x, y0, w, y1 - y0);
    }
}

static void gui_hstack_dn(int id, int x, int y, int w, int h)
{
    int jd, jx = x, jw = 0, c = 0;

    widget[id].x = x;
    widget[id].y = y;
    widget[id].w = w;
    widget[id].h = h;

    /* Measure the total width requested by non-filler children. */

    for (jd = widget[id].car; jd; jd = widget[jd].cdr)
        if ((widget[jd].type & GUI_TYPE) == GUI_FILLER)
            c += 1;
        else
            jw += widget[jd].w;

    /* Give non-filler children their requested space.   */
    /* Distribute the rest evenly among filler children. */

    for (jd = widget[id].car; jd; jd = widget[jd].cdr)
    {
        if ((widget[jd].type & GUI_TYPE) == GUI_FILLER)
            gui_widget_dn(jd, jx, y, (w - jw) / c, h);
        else
            gui_widget_dn(jd, jx, y, widget[jd].w, h);

        jx += widget[jd].w;
    }
}

static void gui_vstack_dn(int id, int x, int y, int w, int h)
{
    int jd, jy = y, jh = 0, c = 0;

    widget[id].x = x;
    widget[id].y = y;
    widget[id].w = w;
    widget[id].h = h;

    /* Measure the total height requested by non-filler children. */

    for (jd = widget[id].car; jd; jd = widget[jd].cdr)
        if ((widget[jd].type & GUI_TYPE) == GUI_FILLER)
            c += 1;
        else
            jh += widget[jd].h;

    /* Give non-filler children their requested space.   */
    /* Distribute the rest evenly among filler children. */

    for (jd = widget[id].car; jd; jd = widget[jd].cdr)
    {
        if ((widget[jd].type & GUI_TYPE) == GUI_FILLER)
            gui_widget_dn(jd, x, jy, w, (h - jh) / c);
        else
            gui_widget_dn(jd, x, jy, w, widget[jd].h);

        jy += widget[jd].h;
    }
}

static void gui_filler_dn(int id, int x, int y, int w, int h)
{
    /* Filler expands to whatever size it is given. */

    widget[id].x = x;
    widget[id].y = y;
    widget[id].w = w;
    widget[id].h = h;
}

static void gui_button_dn(int id, int x, int y, int w, int h)
{
    /* Recall stored width and height for text rendering. */

    int W = widget[id].x;
    int H = widget[id].y;
    int R = widget[id].rect;
    int r = ((widget[id].type & GUI_TYPE) == GUI_PAUSE ? radius * 4 : radius);

    const float *c0 = widget[id].color0;
    const float *c1 = widget[id].color1;

    widget[id].x = x;
    widget[id].y = y;
    widget[id].w = w;
    widget[id].h = h;

    /* Create display lists for the text area and rounded rectangle. */

    widget[id].text_obj = gui_list(-W / 2, -H / 2, W, H, c0, c1);
    widget[id].rect_obj = gui_rect(-w / 2, -h / 2, w, h, R, r);
}

static void gui_widget_dn(int id, int x, int y, int w, int h)
{
    if (id)
        switch (widget[id].type & GUI_TYPE)
        {
        case GUI_HARRAY: gui_harray_dn(id, x, y, w, h); break;
        case GUI_VARRAY: gui_varray_dn(id, x, y, w, h); break;
        case GUI_HSTACK: gui_hstack_dn(id, x, y, w, h); break;
        case GUI_VSTACK: gui_vstack_dn(id, x, y, w, h); break;
        case GUI_FILLER: gui_filler_dn(id, x, y, w, h); break;
        case GUI_SPACE:  gui_filler_dn(id, x, y, w, h); break;
        default:         gui_button_dn(id, x, y, w, h); break;
        }
}

/*---------------------------------------------------------------------------*/
/*
 * During GUI layout, we make a bottom-up pass to determine total area
 * requirements for  the widget  tree.  We position  this area  to the
 * sides or center of the screen.  Finally, we make a top-down pass to
 * distribute this area to each widget.
 */

void gui_layout(int id, int xd, int yd)
{
    int x, y;

    int w, W = config_get_d(CONFIG_WIDTH);
    int h, H = config_get_d(CONFIG_HEIGHT);

    gui_widget_up(id);

    w = widget[id].w;
    h = widget[id].h;

    if      (xd < 0) x = 0;
    else if (xd > 0) x = (W - w);
    else             x = (W - w) / 2;

    if      (yd < 0) y = 0;
    else if (yd > 0) y = (H - h);
    else             y = (H - h) / 2;

    gui_widget_dn(id, x, y, w, h);

    /* Hilite the widget under the cursor, if any. */

    gui_point(id, -1, -1);
}

int gui_search(int id, int x, int y)
{
    int jd, kd;

    /* Search the hierarchy for the widget containing the given point. */

    if (id && (widget[id].x <= x && x < widget[id].x + widget[id].w &&
               widget[id].y <= y && y < widget[id].y + widget[id].h))
    {
        if (gui_hot(id))
            return id;

        for (jd = widget[id].car; jd; jd = widget[jd].cdr)
            if ((kd = gui_search(jd, x, y)))
                return kd;
    }
    return 0;
}

/*
 * Activate a widget, allowing it  to behave as a normal state widget.
 * This may  be used  to create  image buttons, or  cause an  array of
 * widgets to behave as a single state widget.
 */
int gui_active(int id, int token, int value)
{
    widget[id].type |= GUI_STATE;
    widget[id].token = token;
    widget[id].value = value;

    return id;
}

int gui_delete(int id)
{
    if (id)
    {
        /* Recursively delete all subwidgets. */

        gui_delete(widget[id].cdr);
        gui_delete(widget[id].car);

        /* Release any GL resources held by this widget. */

        //if (glIsTexture(widget[id].text_img))
            glDeleteTextures(1, &widget[id].text_img);

		/*
        if (glIsList(widget[id].text_obj))
            glDeleteLists(widget[id].text_obj, 1);
        if (glIsList(widget[id].rect_obj))
            glDeleteLists(widget[id].rect_obj, 1);
		*/

        if (widget[id].text_obj)
            free(widget[id].text_obj);
        if (widget[id].rect_obj)
            free(widget[id].rect_obj);

        /* Mark this widget unused. */

        widget[id].type     = GUI_FREE;
        widget[id].text_img = 0;
        widget[id].text_obj = 0;
        widget[id].rect_obj = 0;
        widget[id].cdr      = 0;
        widget[id].car      = 0;
    }
    return 0;
}

/*---------------------------------------------------------------------------*/

static void gui_paint_rect(int id, int st)
{
    static const uint8_t back[4][4] = {
        {  26,  26,  26, 128 },             /* off and inactive    */
        { 128, 128, 128, 205 },             /* off and   active    */
        { 255, 179,  77, 128 },             /* on  and inactive    */
        { 255, 179,  77, 205 },             /* on  and   active    */
    };

    int jd, i = 0;

    /* Use the widget status to determine the background color. */

    if (gui_hot(id))
        i = st | (((widget[id].value) ? 2 : 0) |
                  ((id == active)     ? 1 : 0));

    switch (widget[id].type & GUI_TYPE)
    {
    case GUI_IMAGE:
    case GUI_SPACE:
    case GUI_FILLER:
        break;

    case GUI_HARRAY:
    case GUI_VARRAY:
    case GUI_HSTACK:
    case GUI_VSTACK:

        /* Recursively paint all subwidgets. */

        for (jd = widget[id].car; jd; jd = widget[jd].cdr)
            gui_paint_rect(jd, i);

        break;

    default:

        /* Draw a leaf's background, colored by widget state. */

        glPushMatrix();
        {
            glTranslatef((GLfloat) (widget[id].x + widget[id].w / 2) / SCALE_VERTICES,
                         (GLfloat) (widget[id].y + widget[id].h / 2) / SCALE_VERTICES, 0.f);

            glColor3b(back[i][0], back[i][1], back[i][2]);
            glPolyFmt(POLY_ID(63) | POLY_ALPHA(back[i][3] * 31 / 255) | POLY_CULL_NONE);
            glCallList(widget[id].rect_obj);
	        glPolyFmt(POLY_ALPHA(31) | POLY_CULL_BACK);
        }
        glPopMatrix(1);

        break;
    }
}

/*---------------------------------------------------------------------------*/

static void gui_paint_text(int id);

static void gui_paint_array(int id)
{
    int jd;

    glPushMatrix();
    {
        GLfloat cx = widget[id].x + widget[id].w / 2.0f;
        GLfloat cy = widget[id].y + widget[id].h / 2.0f;
        GLfloat ck = widget[id].scale;

        glTranslatef(+cx / SCALE_VERTICES, +cy / SCALE_VERTICES, 0.0f);
        glScalef(ck, ck, ck);
        glTranslatef(-cx / SCALE_VERTICES, -cy / SCALE_VERTICES, 0.0f);

        /* Recursively paint all subwidgets. */

        for (jd = widget[id].car; jd; jd = widget[jd].cdr)
            gui_paint_text(jd);
    }
    glPopMatrix(1);
}

static void gui_paint_image(int id)
{
    /* Draw the widget rect, textured using the image. */

    glPushMatrix();
    {
        glTranslatef((GLfloat) (widget[id].x + widget[id].w / 2) / SCALE_VERTICES,
                     (GLfloat) (widget[id].y + widget[id].h / 2) / SCALE_VERTICES, 0.f);

        glScalef(widget[id].scale,
                 widget[id].scale,
                 widget[id].scale);

        glBindTexture(GL_TEXTURE_2D, widget[id].text_img);
        glColor3b(255, 255, 255);
        glPolyFmt(POLY_ALPHA(31) | POLY_CULL_NONE);
        glCallList(widget[id].rect_obj);
	    glPolyFmt(POLY_ALPHA(31) | POLY_CULL_BACK);
    }
    glPopMatrix(1);
}

static void gui_paint_count(int id)
{
    int j, i = widget[id].size;

    glPushMatrix();
    {
        glColor3b(255, 255, 255);
        glPolyFmt(POLY_ALPHA(31) | POLY_CULL_NONE);

        /* Translate to the widget center, and apply the pulse scale. */

        glTranslatef((GLfloat) (widget[id].x + widget[id].w / 2) / SCALE_VERTICES,
                     (GLfloat) (widget[id].y + widget[id].h / 2) / SCALE_VERTICES, 0.f);

        glScalef(widget[id].scale,
                 widget[id].scale,
                 widget[id].scale);

        if (widget[id].value)
        {
            /* Translate left by half the total width of the rendered value. */

            for (j = widget[id].value; j; j /= 10)
                glTranslatef((GLfloat) +digit_w[i][j % 10] / 2.0f / SCALE_VERTICES, 0.0f, 0.0f);

            glTranslatef((GLfloat) -digit_w[i][0] / 2.0f / SCALE_VERTICES, 0.0f, 0.0f);

            /* Render each digit, moving right after each. */

            for (j = widget[id].value; j; j /= 10)
            {
                glBindTexture(GL_TEXTURE_2D, digit_text[i][j % 10]);
                glCallList(digit_list[i][j % 10]);
                glTranslatef((GLfloat) -digit_w[i][j % 10] / SCALE_VERTICES, 0.0f, 0.0f);
            }
        }
        else
        {
            /* If the value is zero, just display a zero in place. */

            glBindTexture(GL_TEXTURE_2D, digit_text[i][0]);
            glCallList(digit_list[i][0]);
        }

        glPolyFmt(POLY_ALPHA(31) | POLY_CULL_BACK);
    }
    glPopMatrix(1);
}

static void gui_paint_clock(int id)
{
    int i  =   widget[id].size;
    int mt =  (widget[id].value / 6000) / 10;
    int mo =  (widget[id].value / 6000) % 10;
    int st = ((widget[id].value % 6000) / 100) / 10;
    int so = ((widget[id].value % 6000) / 100) % 10;
    int ht = ((widget[id].value % 6000) % 100) / 10;
    int ho = ((widget[id].value % 6000) % 100) % 10;

    GLfloat dx_large = (GLfloat) digit_w[i][0];
    GLfloat dx_small = (GLfloat) digit_w[i][0] * 0.75f;

    glPushMatrix();
    {
		glColor3b(255, 255, 255);
        glPolyFmt(POLY_ALPHA(31) | POLY_CULL_NONE);

        /* Translate to the widget center, and apply the pulse scale. */

        glTranslatef((GLfloat) (widget[id].x + widget[id].w / 2) / SCALE_VERTICES,
                     (GLfloat) (widget[id].y + widget[id].h / 2) / SCALE_VERTICES, 0.f);

        glScalef(widget[id].scale,
                 widget[id].scale,
                 widget[id].scale);

        /* Translate left by half the total width of the rendered value. */

        if (mt > 0)
            glTranslatef(-2.25f * dx_large / SCALE_VERTICES, 0.0f, 0.0f);
        else
            glTranslatef(-1.75f * dx_large / SCALE_VERTICES, 0.0f, 0.0f);

        /* Render the minutes counter. */

        if (mt > 0)
        {
            glBindTexture(GL_TEXTURE_2D, digit_text[i][mt]);
            glCallList(digit_list[i][mt]);
            glTranslatef(dx_large / SCALE_VERTICES, 0.0f, 0.0f);
        }

        glBindTexture(GL_TEXTURE_2D, digit_text[i][mo]);
        glCallList(digit_list[i][mo]);
        glTranslatef(dx_small / SCALE_VERTICES, 0.0f, 0.0f);

        /* Render the colon. */

        glBindTexture(GL_TEXTURE_2D, digit_text[i][10]);
        glCallList(digit_list[i][10]);
        glTranslatef(dx_small / SCALE_VERTICES, 0.0f, 0.0f);

        /* Render the seconds counter. */

        glBindTexture(GL_TEXTURE_2D, digit_text[i][st]);
        glCallList(digit_list[i][st]);
        glTranslatef(dx_large / SCALE_VERTICES, 0.0f, 0.0f);

        glBindTexture(GL_TEXTURE_2D, digit_text[i][so]);
        glCallList(digit_list[i][so]);
        glTranslatef(dx_small / SCALE_VERTICES, 0.0f, 0.0f);

        /* Render hundredths counter half size. */

        glScalef(0.5f, 0.5f, 1.0f);

        glBindTexture(GL_TEXTURE_2D, digit_text[i][ht]);
        glCallList(digit_list[i][ht]);
        glTranslatef(dx_large / SCALE_VERTICES, 0.0f, 0.0f);

        glBindTexture(GL_TEXTURE_2D, digit_text[i][ho]);
        glCallList(digit_list[i][ho]);

        glPolyFmt(POLY_ALPHA(31) | POLY_CULL_BACK);
    }
    glPopMatrix(1);
}

static void gui_paint_label(int id)
{
    /* Draw the widget text box, textured using the glyph. */

    glPushMatrix();
    {
		glEnable(GL_TEXTURE_2D);
        glTranslatef((GLfloat) (widget[id].x + widget[id].w / 2) / SCALE_VERTICES,
                     (GLfloat) (widget[id].y + widget[id].h / 2) / SCALE_VERTICES, 0.f);

        glScalef(widget[id].scale,
                 widget[id].scale,
                 widget[id].scale);

        glBindTexture(GL_TEXTURE_2D, widget[id].text_img);
        glCallList(widget[id].text_obj);
    }
    glPopMatrix(1);
}

static void gui_paint_text(int id)
{
    switch (widget[id].type & GUI_TYPE)
    {
    case GUI_SPACE:  break;
    case GUI_FILLER: break;
    case GUI_HARRAY: gui_paint_array(id); break;
    case GUI_VARRAY: gui_paint_array(id); break;
    case GUI_HSTACK: gui_paint_array(id); break;
    case GUI_VSTACK: gui_paint_array(id); break;
    case GUI_IMAGE:  gui_paint_image(id); break;
    case GUI_COUNT:  gui_paint_count(id); break;
    case GUI_CLOCK:  gui_paint_clock(id); break;
    default:         gui_paint_label(id); break;
    }
}

void gui_paint(int id)
{
    if (id)
    {
        /*
        glPushAttrib(GL_LIGHTING_BIT     |
                     GL_COLOR_BUFFER_BIT |
                     GL_DEPTH_BUFFER_BIT);
        */
        config_push_ortho();
        {
            glEnable(GL_BLEND);
            /*
            glEnable(GL_COLOR_MATERIAL);
            glDisable(GL_LIGHTING);
            glDisable(GL_DEPTH_TEST);
            */

            //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            //glPushAttrib(GL_TEXTURE_BIT);
            {
                //glDisable(GL_TEXTURE_2D);
                glBindTexture(0, 0);
                gui_paint_rect(id, 0);
            }
            //glPopAttrib();

            gui_paint_text(id);
        }
        config_pop_matrix();
        //glPopAttrib();
    }
}

void gui_blank(void)
{
    gui_paint(pause_id);
}

/*---------------------------------------------------------------------------*/

void gui_dump(int id, int d)
{
    int jd, i;

    if (id)
    {
        char *type = "?";

        switch (widget[id].type & GUI_TYPE)
        {
        case GUI_HARRAY: type = "harray"; break;
        case GUI_VARRAY: type = "varray"; break;
        case GUI_HSTACK: type = "hstack"; break;
        case GUI_VSTACK: type = "vstack"; break;
        case GUI_FILLER: type = "filler"; break;
        case GUI_IMAGE:  type = "image";  break;
        case GUI_LABEL:  type = "label";  break;
        case GUI_COUNT:  type = "count";  break;
        case GUI_CLOCK:  type = "clock";  break;
        }

        for (i = 0; i < d; i++)
            printf("    ");

        printf("%04d %s\n", id, type);

        for (jd = widget[id].car; jd; jd = widget[jd].cdr)
            gui_dump(jd, d + 1);
    }
}

void gui_pulse(int id, float k)
{
    if (id) widget[id].scale = k;
}

void gui_timer(int id, float dt)
{
    int jd;

    if (id)
    {
        for (jd = widget[id].car; jd; jd = widget[jd].cdr)
            gui_timer(jd, dt);

        if (widget[id].scale - 1.0f < dt)
            widget[id].scale = 1.0f;
        else
            widget[id].scale -= dt;
    }
}

int gui_point(int id, int x, int y)
{
    static int x_cache = 0;
    static int y_cache = 0;

    int jd;

    /* Reuse the last coordinates if (x,y) == (-1,-1) */

    if (x < 0 && y < 0)
        return gui_point(id, x_cache, y_cache);

    x_cache = x;
    y_cache = y;

    /* Short-circuit check the current active widget. */

    jd = gui_search(active, x, y);

    /* If not still active, search the hierarchy for a new active widget. */

    if (jd == 0)
        jd = gui_search(id, x, y);

    /* If the active widget has changed, return the new active id. */

    if (jd == 0 || jd == active)
        return 0;
    else
        return active = jd;
}

int gui_click(void)
{
    return active;
}

int gui_token(int id)
{
    return id ? widget[id].token : 0;
}

int gui_value(int id)
{
    return id ? widget[id].value : 0;
}

void gui_toggle(int id)
{
    widget[id].value = widget[id].value ? 0 : 1;
}

/*---------------------------------------------------------------------------*/

static int gui_vert_test(int id, int jd)
{
    /* Determine whether widget id is in vertical contact with widget jd. */

    if (id && gui_hot(id) && jd && gui_hot(jd))
    {
        int i0 = widget[id].x;
        int i1 = widget[id].x + widget[id].w;
        int j0 = widget[jd].x;
        int j1 = widget[jd].x + widget[jd].w;

        /* Is widget id's top edge is in contact with jd's bottom edge? */

        if (widget[id].y + widget[id].h == widget[jd].y)
        {
            /* Do widgets id and jd overlap horizontally? */

            if (j0 <= i0 && i0 <  j1) return 1;
            if (j0 <  i1 && i1 <= j1) return 1;
            if (i0 <= j0 && j0 <  i1) return 1;
            if (i0 <  j1 && j1 <= i1) return 1;
        }
    }
    return 0;
}

static int gui_horz_test(int id, int jd)
{
    /* Determine whether widget id is in horizontal contact with widget jd. */

    if (id && gui_hot(id) && jd && gui_hot(jd))
    {
        int i0 = widget[id].y;
        int i1 = widget[id].y + widget[id].h;
        int j0 = widget[jd].y;
        int j1 = widget[jd].y + widget[jd].h;

        /* Is widget id's right edge in contact with jd's left edge? */

        if (widget[id].x + widget[id].w == widget[jd].x)
        {
            /* Do widgets id and jd overlap vertically? */

            if (j0 <= i0 && i0 <  j1) return 1;
            if (j0 <  i1 && i1 <= j1) return 1;
            if (i0 <= j0 && j0 <  i1) return 1;
            if (i0 <  j1 && j1 <= i1) return 1;
        }
    }
    return 0;
}

/*---------------------------------------------------------------------------*/

static int gui_stick_L(int id, int dd)
{
    int jd, kd;

    /* Find a widget to the left of widget dd. */

    if (gui_horz_test(id, dd))
        return id;

    for (jd = widget[id].car; jd; jd = widget[jd].cdr)
        if ((kd = gui_stick_L(jd, dd)))
            return kd;

    return 0;
}

static int gui_stick_R(int id, int dd)
{
    int jd, kd;

    /* Find a widget to the right of widget dd. */

    if (gui_horz_test(dd, id))
        return id;

    for (jd = widget[id].car; jd; jd = widget[jd].cdr)
        if ((kd = gui_stick_R(jd, dd)))
            return kd;

    return 0;
}

static int gui_stick_D(int id, int dd)
{
    int jd, kd;

    /* Find a widget below widget dd. */

    if (gui_vert_test(id, dd))
        return id;

    for (jd = widget[id].car; jd; jd = widget[jd].cdr)
        if ((kd = gui_stick_D(jd, dd)))
            return kd;

    return 0;
}

static int gui_stick_U(int id, int dd)
{
    int jd, kd;

    /* Find a widget above widget dd. */

    if (gui_vert_test(dd, id))
        return id;

    for (jd = widget[id].car; jd; jd = widget[jd].cdr)
        if ((kd = gui_stick_U(jd, dd)))
            return kd;

    return 0;
}

/*---------------------------------------------------------------------------*/

static int gui_wrap_L(int id, int dd)
{
    int jd, kd;

    if ((jd = gui_stick_L(id, dd)) == 0)
        for (jd = dd; (kd = gui_stick_R(id, jd)); jd = kd)
            ;

    return jd;
}

static int gui_wrap_R(int id, int dd)
{
    int jd, kd;

    if ((jd = gui_stick_R(id, dd)) == 0)
        for (jd = dd; (kd = gui_stick_L(id, jd)); jd = kd)
            ;

    return jd;
}

static int gui_wrap_U(int id, int dd)
{
    int jd, kd;

    if ((jd = gui_stick_U(id, dd)) == 0)
        for (jd = dd; (kd = gui_stick_D(id, jd)); jd = kd)
            ;

    return jd;
}

static int gui_wrap_D(int id, int dd)
{
    int jd, kd;

    if ((jd = gui_stick_D(id, dd)) == 0)
        for (jd = dd; (kd = gui_stick_U(id, jd)); jd = kd)
            ;

    return jd;
}

/*---------------------------------------------------------------------------*/

int gui_stick(int id, int x, int y)
{
    /* Flag the axes to prevent uncontrolled scrolling. */

    static int xflag = 1;
    static int yflag = 1;

    int jd = 0;

    /* Find a new active widget in the direction of joystick motion. */

    if (x && -JOY_MID <= x && x <= +JOY_MID)
        xflag = 1;
    else if (x < -JOY_MID && xflag && (jd = gui_wrap_L(id, active)))
        xflag = 0;
    else if (x > +JOY_MID && xflag && (jd = gui_wrap_R(id, active)))
        xflag = 0;

    if (y && -JOY_MID <= y && y <= +JOY_MID)
        yflag = 1;
    else if (y < -JOY_MID && yflag && (jd = gui_wrap_U(id, active)))
        yflag = 0;
    else if (y > +JOY_MID && yflag && (jd = gui_wrap_D(id, active)))
        yflag = 0;

    /* If the active widget has changed, return the new active id. */

    if (jd == 0 || jd == active)
        return 0;
    else
        return active = jd;
}

/*---------------------------------------------------------------------------*/
