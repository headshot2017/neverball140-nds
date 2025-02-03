#include "text.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <search.h>

// https://github.com/vladimirgamalyan/fontbm

struct IntCharBlockMap
{
	int key;
	struct CharBlock* value;
};

static int compar(const void *l, const void *r)
{
    const struct IntCharBlockMap* lm = (struct IntCharBlockMap*)l;
    const struct IntCharBlockMap* lr = (struct IntCharBlockMap*)r;
    return lm->key - lr->key;
}


struct BMFont* font_load(const char* filename)
{
	FILE* f = fopen(filename, "rb");
	if (!f)
	{
		printf("BMFont: File doesn't exist\n");
		return 0;
	}

	char header[3]; uint8_t version;
	fread(header, 3, 1, f);
	fread(&version, 1, 1, f);
	if (header[0] != 'B' || header[1] != 'M' || header[2] != 'F')
	{
		printf("BMFont: Buffer is not binary BMFont file\n");
		fclose(f);
		return 0;
	}

	struct BMFont* font = (struct BMFont*)malloc(sizeof(struct BMFont));
	memset(font, 0, sizeof(struct BMFont*));

	uint8_t currBlock; int blockSize;

	// Block 1: Info
	fread(&currBlock, 1, 1, f);
	fread(&blockSize, 4, 1, f);

	if (currBlock != 1)
	{
		printf("BMFont: Failed loading font (currBlock: expected 1, got %d)\n", currBlock);
		fclose(f);
		free(font);
		return 0;
	}
	if (blockSize <= 0)
	{
		printf("BMFont: Failed loading font on block %d (block size is %d)\n", currBlock, blockSize);
		fclose(f);
		free(font);
		return 0;
	}

	fread(&font->info, sizeof(struct InfoBlock) - sizeof(char*), 1, f);

	int fontNameSize = blockSize - (sizeof(struct InfoBlock) - sizeof(char*));
	fseek(f, fontNameSize, SEEK_CUR);

	// Block 2: Common
	fread(&currBlock, 1, 1, f);
	fread(&blockSize, 4, 1, f);

	if (currBlock != 2)
	{
		printf("BMFont: Failed loading font (currBlock: expected 2, got %d) (%d)\n", currBlock, blockSize);
		fclose(f);
		free(font);
		return 0;
	}
	if (blockSize <= 0)
	{
		printf("BMFont: Failed loading font on block %d (block size is %d)\n", currBlock, blockSize);
		fclose(f);
		free(font);
		return 0;
	}

	fread(&font->common, sizeof(struct CommonBlock), 1, f);

	// Block 3: Pages
	fread(&currBlock, 1, 1, f);
	fread(&blockSize, 4, 1, f);

	if (currBlock != 3)
	{
		printf("BMFont: Failed loading font (currBlock: expected 2, got %d) (%d)\n", currBlock, blockSize);
		fclose(f);
		free(font);
		return 0;
	}
	if (blockSize <= 0)
	{
		printf("BMFont: Failed loading font on block %d (block size is %d)\n", currBlock, blockSize);
		fclose(f);
		free(font);
		return 0;
	}

	int i = 0;
	font->pages = (char**)malloc(sizeof(char*) * font->common.pages);
	printf("BMFont has %d pages\n", font->common.pages);
	while (blockSize > 0)
	{
		char currChar;

		// find the null terminator
		uint32_t startPos = ftell(f);
		do {
			fread(&currChar, 1, 1, f);
		} while (currChar != 0);
		uint32_t endPos = ftell(f);
		uint32_t pageSize = endPos - startPos;

		char* pageName = (char*)malloc(pageSize);
		fseek(f, startPos, SEEK_SET);
		fread(pageName, pageSize, 1, f);

		printf("BMFont page %d: %s\n", i, pageName);
		font->pages[i++] = pageName;
		blockSize -= pageSize;
	}

	// Block 4: Chars
	fread(&currBlock, 1, 1, f);
	fread(&blockSize, 4, 1, f);

	if (currBlock != 4)
	{
		printf("BMFont: Failed loading font (currBlock: expected 4, got %d)\n", currBlock);
		fclose(f);
		font_free(font);
		return 0;
	}
	if (blockSize <= 0)
	{
		printf("BMFont: Failed loading font on block %d (block size is %d)\n", currBlock, blockSize);
		fclose(f);
		font_free(font);
		return 0;
	}

	int charCount = blockSize / sizeof(struct CharBlock);
	for (int i=0; i<charCount; i++)
	{
		struct CharBlock block = {0};
		fread(&block, sizeof(struct CharBlock), 1, f);
		//font->chars[block.id] = block;
	}

	// Block 5 (optional): Kernings
	if (fread(&currBlock, 1, 1, f) < 1)
	{
		// No kernings. We're done
		printf("BMFont loaded successfully\n");
		fclose(f);
		return font;
	}

	fread(&blockSize, 4, 1, f);

	if (currBlock != 5)
	{
		printf("BMFont: Failed loading font (currBlock: expected 5, got %d)\n", currBlock);
		fclose(f);
		font_free(font);
		return 0;
	}
	if (blockSize <= 0)
	{
		printf("BMFont: Failed loading font on block %d (block size is %d)\n", currBlock, blockSize);
		fclose(f);
		font_free(font);
		return 0;
	}

	font->kerningCount = blockSize / sizeof(struct KerningPairsBlock);
	font->kernings = (struct KerningPairsBlock*)malloc(sizeof(struct KerningPairsBlock) * font->kerningCount);
	for (int i=0; i<font->kerningCount; i++)
	{
		struct KerningPairsBlock block = {0};
		fread(&block, sizeof(struct KerningPairsBlock), 1, f);
		font->kernings[i] = block;
	}

	printf("BMFont loaded successfully with kernings\n");
	fclose(f);
	return font;
}

void font_free(struct BMFont* pFont)
{
	if (!pFont) return;

	if (pFont->pages)
	{
		for (int i=0; i<pFont->common.pages; i++)
			free(pFont->pages[i]);
		free(pFont->pages);
	}

	if (pFont->kernings)
		free(pFont->kernings);

	free(pFont);
}

void font_render(struct BMFont* font, const char* text, float x, float y, float scale, bool center)
{
	/*
	float offsetX = (center) ? GetCenter(text, scale) : 0;
	x -= offsetX;

	float startX = x;
	for (size_t i=0; i<text.size(); i++)
	{
		int c = text.at(i);
		if (c == '\n')
		{
			x = startX;
			y += m_pData->common.lineHeight*scale;
			continue;
		}
		if (!m_pData->chars.count(c))
		{
			x += 32*scale;
			continue;
		}

		m_pEngine->Graphics()->SetTexture(
			m_pTexPages[m_pData->chars[c].page],
			m_pData->chars[c].x,  // U1
			m_pData->chars[c].y,  // V1
			m_pData->chars[c].x + m_pData->chars[c].width, // U2
			m_pData->chars[c].y + m_pData->chars[c].height // V2
		);

		m_pEngine->Graphics()->RenderSquare(
			x + m_pData->chars[c].xoffset*scale,  // X1
			y + m_pData->chars[c].yoffset*scale,  // Y1
			m_pData->chars[c].width*scale,        // X2
			m_pData->chars[c].height*scale        // Y2
		);

		x += m_pData->chars[c].xadvance*scale;
	}
	*/
}

static float font_getcenter(const char* text, float scale)
{
	float maxWidth = 0, x=0;

	for (size_t i=0; i<strlen(text); i++)
	{
		char c = text[i];
		if (c == '\n')
		{
			x = 0;
			continue;
		}
		/*
		if (!m_pData->chars.count(c))
		{
			x += 32*scale;
			continue;
		}

		x += m_pData->chars[c].xadvance*scale;
		if (x > maxWidth) maxWidth = x;
		*/
	}

	return maxWidth/2;
}
