#include <nds.h>
#include <fat.h>

#include "ball/main.h"
#include "putt/main.h"
#include "libadx.h"

#include "bg_selectgame1.h"
#include "bg_selectgame2.h"
#include "bg_selectgame_screenshot1.h"
#include "bg_selectgame_screenshot2.h"

static const unsigned int* bgTiles[] = {
	bg_selectgame1Tiles,
	bg_selectgame2Tiles,
	bg_selectgame_screenshot1Tiles,
	bg_selectgame_screenshot2Tiles
};

static const u16* bgMap[] = {
	bg_selectgame1Map,
	bg_selectgame2Map,
	bg_selectgame_screenshot1Map,
	bg_selectgame_screenshot2Map
};

static const u16* bgPal[] = {
	bg_selectgame1Pal,
	bg_selectgame2Pal,
	bg_selectgame_screenshot1Pal,
	bg_selectgame_screenshot2Pal
};

static const u32 bgTilesLen[] = {
	bg_selectgame1TilesLen,
	bg_selectgame2TilesLen,
	bg_selectgame_screenshot1TilesLen,
	bg_selectgame_screenshot2TilesLen
};

static const u32 bgMapLen[] = {
	bg_selectgame1MapLen,
	bg_selectgame2MapLen,
	bg_selectgame_screenshot1MapLen,
	bg_selectgame_screenshot2MapLen
};


static int bgMain;
static int bgSub;

static void SetBG(int selection)
{
	dmaCopy(bgTiles[selection], bgGetGfxPtr(bgMain), bgTilesLen[selection]);
	dmaCopy(bgMap[selection], bgGetMapPtr(bgMain), bgMapLen[selection]);
	dmaCopy(bgPal[selection], BG_PALETTE, 512);

	dmaCopy(bgTiles[selection+2], bgGetGfxPtr(bgSub), bgTilesLen[selection+2]);
	dmaCopy(bgMap[selection+2], bgGetMapPtr(bgSub), bgMapLen[selection+2]);
	dmaCopy(bgPal[selection+2], BG_PALETTE_SUB, 512);
}

static void FadeFromBlack()
{
	REG_BLDY = 16;
	REG_BLDY_SUB = 16;
	REG_BLDCNT = BLEND_FADE_BLACK | BLEND_SRC_BACKDROP | BLEND_SRC_BG0 | BLEND_SRC_BG1;
	REG_BLDCNT_SUB = BLEND_FADE_BLACK | BLEND_SRC_BACKDROP | BLEND_SRC_BG0 | BLEND_SRC_BG1;

	int alpha = 16;
	while (1)
	{
		swiWaitForVBlank();
		adx_update();

		alpha--;
		REG_BLDY = alpha;
		REG_BLDY_SUB = alpha;
		if (alpha == 0)
			break;
	}

	REG_BLDCNT = BLEND_NONE;
	REG_BLDCNT_SUB = BLEND_NONE;
}

static void FadeToBlack()
{
	REG_BLDY_SUB = 0;
	REG_BLDY = 0;
	REG_BLDCNT = BLEND_FADE_BLACK | BLEND_SRC_BACKDROP | BLEND_SRC_BG0 | BLEND_SRC_BG1;
	REG_BLDCNT_SUB = BLEND_FADE_BLACK | BLEND_SRC_BACKDROP | BLEND_SRC_BG0 | BLEND_SRC_BG1;

	int alpha = 0;
	while (1)
	{
		swiWaitForVBlank();
		adx_update();

		alpha++;
		REG_BLDY = alpha;
		REG_BLDY_SUB = alpha;
		if (alpha == 16)
			break;
	}
}

int main(int argc, char* argv[])
{
	defaultExceptionHandler();

	REG_BLDCNT = BLEND_FADE_BLACK | BLEND_SRC_BACKDROP | BLEND_SRC_BG0 | BLEND_SRC_BG1;
	REG_BLDCNT_SUB = BLEND_FADE_BLACK | BLEND_SRC_BACKDROP | BLEND_SRC_BG0 | BLEND_SRC_BG1;
	REG_BLDY = 16;
	REG_BLDY_SUB = 16;

	videoSetMode(MODE_0_3D);
	videoSetModeSub(MODE_0_2D);

	lcdMainOnBottom();

	if (!fatInitDefault())
	{
		consoleDemoInit();
		printf("fatInitDefault() failed!\ncan't continue loading");
		while (1) swiWaitForVBlank();
	}

	glInit();

	adx_init();

	while (1)
	{
		glClearColor(0,0,0,0);

		vramSetBankA(VRAM_A_MAIN_BG);
		vramSetBankC(VRAM_C_SUB_BG);

		bgMain = bgInit(1, BgType_Text8bpp, BgSize_T_256x256, 0, 1);
		bgSub = bgInitSub(1, BgType_Text8bpp, BgSize_T_256x256, 0, 1);

		SetBG(0);
		int gameSelect = 0;

		adx_play("/data/neverball/bgm/inter.adx", 1);
		adx_set_volume(127);
		FadeFromBlack();

		while (1)
		{
			swiWaitForVBlank();
			adx_update();

			scanKeys();
			u32 down = keysDown();

			if (down & KEY_LEFT)
			{
				gameSelect--;
				if (gameSelect < 0) gameSelect = 1;
				SetBG(gameSelect);
			}
			if (down & KEY_RIGHT)
			{
				gameSelect++;
				if (gameSelect > 1) gameSelect = 0;
				SetBG(gameSelect);
			}
			if (down & KEY_A)
				break;
		}

		FadeToBlack();
		adx_stop();

		(gameSelect == 0) ? ball_main(argc, argv) : putt_main(argc, argv);

		REG_BLDCNT = BLEND_FADE_BLACK | BLEND_SRC_BACKDROP | BLEND_SRC_BG0 | BLEND_SRC_BG1;
		REG_BLDCNT_SUB = BLEND_FADE_BLACK | BLEND_SRC_BACKDROP | BLEND_SRC_BG0 | BLEND_SRC_BG1;
		REG_BLDY = 16;
		REG_BLDY_SUB = 16;
		glFlush(0);
	}

	return 0;
}
