#include <stdio.h>
#include "text.h"

int main(int argc, char** argv)
{
	if (argc > 1)
	{
		struct BMFont* font = font_load(argv[1]);
		font_free(font);
	}
	return 0;
}