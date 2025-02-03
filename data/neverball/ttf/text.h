#ifndef TEXT_H_INCLUDED
#define TEXT_H_INCLUDED

#include <stdint.h>
#include <stdbool.h>

#pragma pack(push, 1)
struct InfoBlock
{
	int16_t fontSize;
	int8_t smooth:1;
	int8_t unicode:1;
	int8_t italic:1;
	int8_t bold:1;
	int8_t reserved:4;
	uint8_t charSet;
	uint16_t stretchH;
	int8_t aa;
	uint8_t paddingUp;
	uint8_t paddingRight;
	uint8_t paddingDown;
	uint8_t paddingLeft;
	uint8_t spacingHoriz;
	uint8_t spacingVert;
	uint8_t outline;
};
_Static_assert(sizeof(struct InfoBlock) == 14, "InfoBlock size is not 14");

struct CommonBlock
{
	uint16_t lineHeight;
	uint16_t base;
	uint16_t scaleW;
	uint16_t scaleH;
	uint16_t pages;
	uint8_t reserved:7;
	uint8_t packed:1;
	uint8_t alphaChnl;
	uint8_t redChnl;
	uint8_t greenChnl;
	uint8_t blueChnl;
};
_Static_assert(sizeof(struct CommonBlock) == 15, "CommonBlock size is not 15");

struct CharBlock
{
	uint32_t id;
	uint16_t x;
	uint16_t y;
	uint16_t width;
	uint16_t height;
	int16_t xoffset;
	int16_t yoffset;
	int16_t xadvance;
	uint8_t page;
	uint8_t channel;
};
_Static_assert(sizeof(struct CharBlock) == 20, "CharBlock size is not 20");

struct KerningPairsBlock
{
	uint32_t first;
	uint32_t second;
	int16_t amount;
};
_Static_assert(sizeof(struct KerningPairsBlock) == 10, "KerningPairsBlock size is not 10");
#pragma pack(pop)


struct BMFont
{
	struct InfoBlock info;
	struct CommonBlock common;

	char** pages;

	struct KerningPairsBlock* kernings;
	int kerningCount;

	void* charMap;
};


struct BMFont* font_load(const char* filename);
void font_free(struct BMFont* font);
void font_render(struct BMFont* font, const char* text, float x, float y, float scale, bool center);

#endif // TEXT_H_INCLUDED
