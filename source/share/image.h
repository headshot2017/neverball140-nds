#ifndef IMAGE_H
#define IMAGE_H

#include "glext.h"
#include "stb_truetype.h"

/*---------------------------------------------------------------------------*/

void   image_snap(char *);
void   image_size(int *, int *, int, int);

int    image_palettize(int, int, int, unsigned char*, unsigned short*, unsigned short*);

//void         image_swab (SDL_Surface *);
//void         image_white(SDL_Surface *);
//SDL_Surface *image_scale(SDL_Surface *, int);

//GLuint make_image_from_surf(int *, int *, SDL_Surface *);
int make_image_from_file(int *, int *,
                            int *, int *, const char *);
int make_image_from_font(int *, int *,
                            int *, int *, const char *, stbtt_fontinfo *, float, int, int);

/*---------------------------------------------------------------------------*/

#endif
