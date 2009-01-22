//
// GUIMISC.CPP
//
// Graphical User Interface support functions
// by James L. Hammons
//
// JLH = James L. Hammons <jlhamm@acm.org>
//
// WHO  WHEN        WHAT
// ---  ----------  ------------------------------------------------------------
// JLH  02/02/2006  Created this file
// JLH  03/13/2006  Abstracted out font to allow external fonts
//

#include "guimisc.h"
#include "font14pt.h"
//Can't do this...!
//#include "charset.h"	// For Apple II font (small)
#include <string>
#include <vector>


#if SDL_BYTEORDER == SDL_BIG_ENDIAN
#define MASK_R 0xFF000000
#define MASK_G 0x00FF0000
#define MASK_B 0x0000FF00
#define MASK_A 0x000000FF
#else
#define MASK_R 0x000000FF
#define MASK_G 0x0000FF00
#define MASK_B 0x00FF0000
#define MASK_A 0xFF000000
#endif

// Local variables
// This will enable us to set up any font without having it embedded here...

static Font font((uint8 *)font2, FONT_WIDTH, FONT_HEIGHT);
static std::vector<Font> oldFontList;


void SetNewFont(Font newFont)
{
	oldFontList.push_back(font);
	font.data = newFont.data, font.width = newFont.width, font.height = newFont.height;
}

void RestoreOldFont(void)
{
	if (oldFontList.size() == 0)
		return;

	font = oldFontList.back();
	oldFontList.pop_back();
}

uint32 GetFontWidth(void)
{
	return font.width;
}

uint32 GetFontHeight(void)
{
	return font.height;
}

//
// Draw text at the given x/y coordinates with transparency (255 is fully opaque, 0 is fully transparent).
//
void DrawStringTrans(SDL_Surface * screen, uint32 x, uint32 y, uint32 color, const char * text, ...)
{
	char string[4096];
	va_list arg;

	va_start(arg, text);
	vsprintf(string, text, arg);
	va_end(arg);

	uint8 * esColor = (uint8 *)&color;			// Do things endian safe...!
	uint8 trans = esColor[3];
	uint32 length = strlen(string);

	// Make a "stamp" surface (with built in alpha!) for constructing our font chars...
	SDL_Surface * chr = SDL_CreateRGBSurface(SDL_SWSURFACE, font.width, font.height, 32,
		MASK_R, MASK_G, MASK_B, MASK_A);
	SDL_Rect rect;
	rect.x = x, rect.y = y;

	for(uint32 i=0; i<length; i++)
	{
		uint8 c = string[i];
		uint32 fontAddr = (uint32)(c < 32 ? 0 : c - 32) * font.width * font.height;

		for(uint32 yy=0; yy<font.height; yy++)
		{
			for(uint32 xx=0; xx<font.width; xx++)
			{
				esColor[3] = (font.data[fontAddr++] * trans) / 255;
				((uint32 *)chr->pixels)[xx + (yy * (chr->pitch / 4))] = color;
			}
		}

		SDL_BlitSurface(chr, NULL, screen, &rect);
		rect.x += font.width;
	}

	SDL_FreeSurface(chr);
}

//
// Draw text at given x/y coords using foreground/background color combination
//
void DrawStringOpaque(SDL_Surface * screen, uint32 x, uint32 y, uint32 fg, uint32 bg, const char * text, ...)
{
	char string[4096];
	va_list arg;

	va_start(arg, text);
	vsprintf(string, text, arg);
	va_end(arg);

	uint8 * esColor = (uint8 *)&fg;				// Do things endian safe...!
	uint32 length = strlen(string);

	SDL_Rect destRect;

	destRect.x = x, destRect.y = y;
	destRect.w = length * font.width, destRect.h = font.height;

	SDL_FillRect(screen, &destRect, bg);

	// Make a "stamp" surface (with built in alpha!) for constructing our font chars...
	SDL_Surface * chr = SDL_CreateRGBSurface(SDL_SWSURFACE, font.width, font.height, 32,
		MASK_R, MASK_G, MASK_B, MASK_A);
	SDL_Rect rect;
	rect.x = x, rect.y = y;

	for(uint32 i=0; i<length; i++)
	{
		uint8 c = string[i];
		uint32 fontAddr = (uint32)(c < 32 ? 0 : c - 32) * font.width * font.height;

		for(uint32 yy=0; yy<font.height; yy++)
		{
			for(uint32 xx=0; xx<font.width; xx++)
			{
				esColor[3] = font.data[fontAddr++];
				((uint32 *)chr->pixels)[xx + (yy * (chr->pitch / 4))] = fg;
			}
		}

		SDL_BlitSurface(chr, NULL, screen, &rect);
		rect.x += font.width;
	}

	SDL_FreeSurface(chr);
}

bool RectanglesIntersect(SDL_Rect r1, SDL_Rect r2)
{
	// The strategy here is to see if any of the sides of the smaller rect
	// fall within the larger.

/*
    +-----------------+ r1
    |                 |
    |   +------+ r2   |
    |   |      |      |
    |   |      |      |
    |   +------+      |
    |                 |
    +-----------------+

*/

//This approach fails if r2 is inside of r1. !!! FIX !!! [DONE]
	if (RectangleFirstInsideSecond(r2, r1))
		return true;

	if ((r1.x > r2.x && r1.x < (r2.x + r2.w))
		|| ((r1.x + r1.w) > r2.x && (r1.x + r1.w) < (r2.x + r2.w))
		|| (r1.y > r2.y && r1.y < (r2.y + r2.h))
		|| ((r1.y + r1.h) > r2.y && (r1.y + r1.h) < (r2.y + r2.h)))
		return true;

	return false;
}

bool RectangleFirstInsideSecond(SDL_Rect r1, SDL_Rect r2)
{
	if ((r1.x > r2.x             && (r1.x + r1.w) > r2.x)
		&& (r1.x < (r2.x + r2.w) && (r1.x + r1.w) < (r2.x + r2.w))
		&& (r1.y > r2.y          && (r1.y + r1.h) > r2.y)
		&& (r1.y < (r2.y + r2.h) && (r1.y + r1.h) < (r2.y + r2.h)))
		return true;

	return false;
}


//
// Various GUI bitmaps
//

// These representations *should* be endian safe.

uint8 closeBox[] = {
	15 / 256, 15 % 256,		// width (HI byte, LO byte)
	15 / 256, 15 % 256,		// height (HI byte, LO byte)

	0x00, 0x00, 0x00, 0x00, 0xED, 0x38, 0x38, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0x00, 0x00, 0x00, 0x00,
	0xED, 0x38, 0x38, 0xFF, 0xE7, 0x58, 0x58, 0xFF, 0xFB, 0x6B, 0x6B, 0xFF, 0xFB, 0x6B, 0x6B, 0xFF, 0xFB, 0x6B, 0x6B, 0xFF, 0xFB, 0x6B, 0x6B, 0xFF, 0xFB, 0x6B, 0x6B, 0xFF, 0xFB, 0x6B, 0x6B, 0xFF, 0xFB, 0x6B, 0x6B, 0xFF, 0xFB, 0x6B, 0x6B, 0xFF, 0xFB, 0x6B, 0x6B, 0xFF, 0xFB, 0x6B, 0x6B, 0xFF, 0xFB, 0x6B, 0x6B, 0xFF, 0xE7, 0x58, 0x58, 0xFF, 0xED, 0x38, 0x38, 0xFF,
	0xED, 0x38, 0x38, 0xFF, 0xFB, 0x6B, 0x6B, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0xFB, 0x6B, 0x6B, 0xFF, 0xED, 0x38, 0x38, 0xFF,
	0xED, 0x38, 0x38, 0xFF, 0xFB, 0x6B, 0x6B, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0xFB, 0x6B, 0x6B, 0xFF, 0xED, 0x38, 0x38, 0xFF,
	0xED, 0x38, 0x38, 0xFF, 0xFB, 0x6B, 0x6B, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xFB, 0x6B, 0x6B, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xFB, 0x6B, 0x6B, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0xFB, 0x6B, 0x6B, 0xFF, 0xED, 0x38, 0x38, 0xFF,
	0xED, 0x38, 0x38, 0xFF, 0xFB, 0x6B, 0x6B, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xFB, 0x6B, 0x6B, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xFB, 0x6B, 0x6B, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0xFB, 0x6B, 0x6B, 0xFF, 0xED, 0x38, 0x38, 0xFF,
	0xED, 0x38, 0x38, 0xFF, 0xFB, 0x6B, 0x6B, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xFB, 0x6B, 0x6B, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xFB, 0x6B, 0x6B, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0xFB, 0x6B, 0x6B, 0xFF, 0xED, 0x38, 0x38, 0xFF,
	0xED, 0x38, 0x38, 0xFF, 0xFB, 0x6B, 0x6B, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xFB, 0x6B, 0x6B, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0xFB, 0x6B, 0x6B, 0xFF, 0xED, 0x38, 0x38, 0xFF,
	0xED, 0x38, 0x38, 0xFF, 0xFB, 0x6B, 0x6B, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xFB, 0x6B, 0x6B, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xFB, 0x6B, 0x6B, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0xFB, 0x6B, 0x6B, 0xFF, 0xED, 0x38, 0x38, 0xFF,
	0xED, 0x38, 0x38, 0xFF, 0xFB, 0x6B, 0x6B, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xFB, 0x6B, 0x6B, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xFB, 0x6B, 0x6B, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0xFB, 0x6B, 0x6B, 0xFF, 0xED, 0x38, 0x38, 0xFF,
	0xED, 0x38, 0x38, 0xFF, 0xFB, 0x6B, 0x6B, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xFB, 0x6B, 0x6B, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xFB, 0x6B, 0x6B, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0xFB, 0x6B, 0x6B, 0xFF, 0xED, 0x38, 0x38, 0xFF,
	0xED, 0x38, 0x38, 0xFF, 0xFB, 0x6B, 0x6B, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0xFB, 0x6B, 0x6B, 0xFF, 0xED, 0x38, 0x38, 0xFF,
	0xED, 0x38, 0x38, 0xFF, 0xFB, 0x6B, 0x6B, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0xFB, 0x6B, 0x6B, 0xFF, 0xED, 0x38, 0x38, 0xFF,
	0xED, 0x38, 0x38, 0xFF, 0xE7, 0x58, 0x58, 0xFF, 0xFB, 0x6B, 0x6B, 0xFF, 0xFB, 0x6B, 0x6B, 0xFF, 0xFB, 0x6B, 0x6B, 0xFF, 0xFB, 0x6B, 0x6B, 0xFF, 0xFB, 0x6B, 0x6B, 0xFF, 0xFB, 0x6B, 0x6B, 0xFF, 0xFB, 0x6B, 0x6B, 0xFF, 0xFB, 0x6B, 0x6B, 0xFF, 0xFB, 0x6B, 0x6B, 0xFF, 0xFB, 0x6B, 0x6B, 0xFF, 0xFB, 0x6B, 0x6B, 0xFF, 0xE7, 0x58, 0x58, 0xFF, 0xED, 0x38, 0x38, 0xFF,
	0x00, 0x00, 0x00, 0x00, 0xED, 0x38, 0x38, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0x00, 0x00, 0x00, 0x00
};

uint8 closeBoxHover[] = {
	15 / 256, 15 % 256,		// width (HI byte, LO byte)
	15 / 256, 15 % 256,		// height (HI byte, LO byte)

	0x00, 0x00, 0x00, 0x00, 0xED, 0x38, 0x38, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0x00, 0x00, 0x00, 0x00,
	0xED, 0x38, 0x38, 0xFF, 0xE7, 0x58, 0x58, 0xFF, 0xFB, 0x6B, 0x6B, 0xFF, 0xFB, 0x6B, 0x6B, 0xFF, 0xFB, 0x6B, 0x6B, 0xFF, 0xFB, 0x6B, 0x6B, 0xFF, 0xFB, 0x6B, 0x6B, 0xFF, 0xFB, 0x6B, 0x6B, 0xFF, 0xFB, 0x6B, 0x6B, 0xFF, 0xFB, 0x6B, 0x6B, 0xFF, 0xFB, 0x6B, 0x6B, 0xFF, 0xFB, 0x6B, 0x6B, 0xFF, 0xFB, 0x6B, 0x6B, 0xFF, 0xE7, 0x58, 0x58, 0xFF, 0xED, 0x38, 0x38, 0xFF,
	0xED, 0x38, 0x38, 0xFF, 0xFB, 0x6B, 0x6B, 0xFF, 0xFF, 0xA2, 0x20, 0xFF, 0xFF, 0xA2, 0x20, 0xFF, 0xFF, 0xA2, 0x20, 0xFF, 0xFF, 0xA2, 0x20, 0xFF, 0xFF, 0xA2, 0x20, 0xFF, 0xFF, 0xA2, 0x20, 0xFF, 0xFF, 0xA2, 0x20, 0xFF, 0xFF, 0xA2, 0x20, 0xFF, 0xFF, 0xA2, 0x20, 0xFF, 0xFF, 0xA2, 0x20, 0xFF, 0xFF, 0xA2, 0x20, 0xFF, 0xFB, 0x6B, 0x6B, 0xFF, 0xED, 0x38, 0x38, 0xFF,
	0xED, 0x38, 0x38, 0xFF, 0xFB, 0x6B, 0x6B, 0xFF, 0xFF, 0xA2, 0x20, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xFF, 0xAF, 0x40, 0xFF, 0xFF, 0xAF, 0x40, 0xFF, 0xFF, 0xAF, 0x40, 0xFF, 0xFF, 0xAF, 0x40, 0xFF, 0xFF, 0xAF, 0x40, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xFF, 0xA2, 0x20, 0xFF, 0xFB, 0x6B, 0x6B, 0xFF, 0xED, 0x38, 0x38, 0xFF,
	0xED, 0x38, 0x38, 0xFF, 0xFB, 0x6B, 0x6B, 0xFF, 0xFF, 0xA2, 0x20, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xFB, 0x6B, 0x6B, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xFF, 0xB7, 0x52, 0xFF, 0xFF, 0xB7, 0x52, 0xFF, 0xFF, 0xB7, 0x52, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xFB, 0x6B, 0x6B, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xFF, 0xA2, 0x20, 0xFF, 0xFB, 0x6B, 0x6B, 0xFF, 0xED, 0x38, 0x38, 0xFF,
	0xED, 0x38, 0x38, 0xFF, 0xFB, 0x6B, 0x6B, 0xFF, 0xFF, 0xA2, 0x20, 0xFF, 0xFF, 0xAF, 0x40, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xFB, 0x6B, 0x6B, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xFF, 0xBE, 0x63, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xFB, 0x6B, 0x6B, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xFF, 0xAF, 0x40, 0xFF, 0xFF, 0xA2, 0x20, 0xFF, 0xFB, 0x6B, 0x6B, 0xFF, 0xED, 0x38, 0x38, 0xFF,
	0xED, 0x38, 0x38, 0xFF, 0xFB, 0x6B, 0x6B, 0xFF, 0xFF, 0xA2, 0x20, 0xFF, 0xFF, 0xAF, 0x40, 0xFF, 0xFF, 0xB7, 0x52, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xFB, 0x6B, 0x6B, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xFB, 0x6B, 0x6B, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xFF, 0xB7, 0x52, 0xFF, 0xFF, 0xAF, 0x40, 0xFF, 0xFF, 0xA2, 0x20, 0xFF, 0xFB, 0x6B, 0x6B, 0xFF, 0xED, 0x38, 0x38, 0xFF,
	0xED, 0x38, 0x38, 0xFF, 0xFB, 0x6B, 0x6B, 0xFF, 0xFF, 0xA2, 0x20, 0xFF, 0xFF, 0xAF, 0x40, 0xFF, 0xFF, 0xB7, 0x52, 0xFF, 0xFF, 0xBE, 0x63, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xFB, 0x6B, 0x6B, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xFF, 0xBE, 0x63, 0xFF, 0xFF, 0xB7, 0x52, 0xFF, 0xFF, 0xAF, 0x40, 0xFF, 0xFF, 0xA2, 0x20, 0xFF, 0xFB, 0x6B, 0x6B, 0xFF, 0xED, 0x38, 0x38, 0xFF,
	0xED, 0x38, 0x38, 0xFF, 0xFB, 0x6B, 0x6B, 0xFF, 0xFF, 0xA2, 0x20, 0xFF, 0xFF, 0xAF, 0x40, 0xFF, 0xFF, 0xB7, 0x52, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xFB, 0x6B, 0x6B, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xFB, 0x6B, 0x6B, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xFF, 0xB7, 0x52, 0xFF, 0xFF, 0xAF, 0x40, 0xFF, 0xFF, 0xA2, 0x20, 0xFF, 0xFB, 0x6B, 0x6B, 0xFF, 0xED, 0x38, 0x38, 0xFF,
	0xED, 0x38, 0x38, 0xFF, 0xFB, 0x6B, 0x6B, 0xFF, 0xFF, 0xA2, 0x20, 0xFF, 0xFF, 0xAF, 0x40, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xFB, 0x6B, 0x6B, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xFF, 0xBE, 0x63, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xFB, 0x6B, 0x6B, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xFF, 0xAF, 0x40, 0xFF, 0xFF, 0xA2, 0x20, 0xFF, 0xFB, 0x6B, 0x6B, 0xFF, 0xED, 0x38, 0x38, 0xFF,
	0xED, 0x38, 0x38, 0xFF, 0xFB, 0x6B, 0x6B, 0xFF, 0xFF, 0xA2, 0x20, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xFB, 0x6B, 0x6B, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xFF, 0xB7, 0x52, 0xFF, 0xFF, 0xB7, 0x52, 0xFF, 0xFF, 0xB7, 0x52, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xFB, 0x6B, 0x6B, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xFF, 0xA2, 0x20, 0xFF, 0xFB, 0x6B, 0x6B, 0xFF, 0xED, 0x38, 0x38, 0xFF,
	0xED, 0x38, 0x38, 0xFF, 0xFB, 0x6B, 0x6B, 0xFF, 0xFF, 0xA2, 0x20, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xFF, 0xAF, 0x40, 0xFF, 0xFF, 0xAF, 0x40, 0xFF, 0xFF, 0xAF, 0x40, 0xFF, 0xFF, 0xAF, 0x40, 0xFF, 0xFF, 0xAF, 0x40, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xFF, 0xA2, 0x20, 0xFF, 0xFB, 0x6B, 0x6B, 0xFF, 0xED, 0x38, 0x38, 0xFF,
	0xED, 0x38, 0x38, 0xFF, 0xFB, 0x6B, 0x6B, 0xFF, 0xFF, 0xA2, 0x20, 0xFF, 0xFF, 0xA2, 0x20, 0xFF, 0xFF, 0xA2, 0x20, 0xFF, 0xFF, 0xA2, 0x20, 0xFF, 0xFF, 0xA2, 0x20, 0xFF, 0xFF, 0xA2, 0x20, 0xFF, 0xFF, 0xA2, 0x20, 0xFF, 0xFF, 0xA2, 0x20, 0xFF, 0xFF, 0xA2, 0x20, 0xFF, 0xFF, 0xA2, 0x20, 0xFF, 0xFF, 0xA2, 0x20, 0xFF, 0xFB, 0x6B, 0x6B, 0xFF, 0xED, 0x38, 0x38, 0xFF,
	0xED, 0x38, 0x38, 0xFF, 0xE7, 0x58, 0x58, 0xFF, 0xFB, 0x6B, 0x6B, 0xFF, 0xFB, 0x6B, 0x6B, 0xFF, 0xFB, 0x6B, 0x6B, 0xFF, 0xFB, 0x6B, 0x6B, 0xFF, 0xFB, 0x6B, 0x6B, 0xFF, 0xFB, 0x6B, 0x6B, 0xFF, 0xFB, 0x6B, 0x6B, 0xFF, 0xFB, 0x6B, 0x6B, 0xFF, 0xFB, 0x6B, 0x6B, 0xFF, 0xFB, 0x6B, 0x6B, 0xFF, 0xFB, 0x6B, 0x6B, 0xFF, 0xE7, 0x58, 0x58, 0xFF, 0xED, 0x38, 0x38, 0xFF,
	0x00, 0x00, 0x00, 0x00, 0xED, 0x38, 0x38, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0x00, 0x00, 0x00, 0x00
};

uint8 closeBoxDown[] = {
	15 / 256, 15 % 256,		// width (HI byte, LO byte)
	15 / 256, 15 % 256,		// height (HI byte, LO byte)

	0x00, 0x00, 0x00, 0x00, 0xED, 0x38, 0x38, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0x00, 0x00, 0x00, 0x00,
	0xED, 0x38, 0x38, 0xFF, 0xE2, 0x1D, 0x1D, 0xFF, 0xC0, 0x2B, 0x2B, 0xFF, 0xC0, 0x2B, 0x2B, 0xFF, 0xC0, 0x2B, 0x2B, 0xFF, 0xC0, 0x2B, 0x2B, 0xFF, 0xC0, 0x2B, 0x2B, 0xFF, 0xC0, 0x2B, 0x2B, 0xFF, 0xC0, 0x2B, 0x2B, 0xFF, 0xC0, 0x2B, 0x2B, 0xFF, 0xC0, 0x2B, 0x2B, 0xFF, 0xC0, 0x2B, 0x2B, 0xFF, 0xC0, 0x2B, 0x2B, 0xFF, 0xE2, 0x1D, 0x1D, 0xFF, 0xED, 0x38, 0x38, 0xFF,
	0xED, 0x38, 0x38, 0xFF, 0xC0, 0x2B, 0x2B, 0xFF, 0xF7, 0x9D, 0x1F, 0xFF, 0xF7, 0x9D, 0x1F, 0xFF, 0xF7, 0x9D, 0x1F, 0xFF, 0xF7, 0x9D, 0x1F, 0xFF, 0xF7, 0x9D, 0x1F, 0xFF, 0xF7, 0x9D, 0x1F, 0xFF, 0xF7, 0x9D, 0x1F, 0xFF, 0xF7, 0x9D, 0x1F, 0xFF, 0xF7, 0x9D, 0x1F, 0xFF, 0xF7, 0x9D, 0x1F, 0xFF, 0xF7, 0x9D, 0x1F, 0xFF, 0xC0, 0x2B, 0x2B, 0xFF, 0xED, 0x38, 0x38, 0xFF,
	0xED, 0x38, 0x38, 0xFF, 0xC0, 0x2B, 0x2B, 0xFF, 0xF7, 0x9D, 0x1F, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xE2, 0x86, 0x07, 0xFF, 0xE2, 0x86, 0x07, 0xFF, 0xE2, 0x86, 0x07, 0xFF, 0xE2, 0x86, 0x07, 0xFF, 0xE2, 0x86, 0x07, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xF7, 0x9D, 0x1F, 0xFF, 0xC0, 0x2B, 0x2B, 0xFF, 0xED, 0x38, 0x38, 0xFF,
	0xED, 0x38, 0x38, 0xFF, 0xC0, 0x2B, 0x2B, 0xFF, 0xF7, 0x9D, 0x1F, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xC0, 0x2B, 0x2B, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xCC, 0x77, 0x00, 0xFF, 0xCC, 0x77, 0x00, 0xFF, 0xCC, 0x77, 0x00, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xC0, 0x2B, 0x2B, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xF7, 0x9D, 0x1F, 0xFF, 0xC0, 0x2B, 0x2B, 0xFF, 0xED, 0x38, 0x38, 0xFF,
	0xED, 0x38, 0x38, 0xFF, 0xC0, 0x2B, 0x2B, 0xFF, 0xF7, 0x9D, 0x1F, 0xFF, 0xE2, 0x86, 0x07, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xC0, 0x2B, 0x2B, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xBD, 0x6E, 0x00, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xC0, 0x2B, 0x2B, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xE2, 0x86, 0x07, 0xFF, 0xF7, 0x9D, 0x1F, 0xFF, 0xC0, 0x2B, 0x2B, 0xFF, 0xED, 0x38, 0x38, 0xFF,
	0xED, 0x38, 0x38, 0xFF, 0xC0, 0x2B, 0x2B, 0xFF, 0xF7, 0x9D, 0x1F, 0xFF, 0xE2, 0x86, 0x07, 0xFF, 0xCC, 0x77, 0x00, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xC0, 0x2B, 0x2B, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xC0, 0x2B, 0x2B, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xCC, 0x77, 0x00, 0xFF, 0xE2, 0x86, 0x07, 0xFF, 0xF7, 0x9D, 0x1F, 0xFF, 0xC0, 0x2B, 0x2B, 0xFF, 0xED, 0x38, 0x38, 0xFF,
	0xED, 0x38, 0x38, 0xFF, 0xC0, 0x2B, 0x2B, 0xFF, 0xF7, 0x9D, 0x1F, 0xFF, 0xE2, 0x86, 0x07, 0xFF, 0xCC, 0x77, 0x00, 0xFF, 0xBD, 0x6E, 0x00, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xC0, 0x2B, 0x2B, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xBD, 0x6E, 0x00, 0xFF, 0xCC, 0x77, 0x00, 0xFF, 0xE2, 0x86, 0x07, 0xFF, 0xF7, 0x9D, 0x1F, 0xFF, 0xC0, 0x2B, 0x2B, 0xFF, 0xED, 0x38, 0x38, 0xFF,
	0xED, 0x38, 0x38, 0xFF, 0xC0, 0x2B, 0x2B, 0xFF, 0xF7, 0x9D, 0x1F, 0xFF, 0xE2, 0x86, 0x07, 0xFF, 0xCC, 0x77, 0x00, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xC0, 0x2B, 0x2B, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xC0, 0x2B, 0x2B, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xCC, 0x77, 0x00, 0xFF, 0xE2, 0x86, 0x07, 0xFF, 0xF7, 0x9D, 0x1F, 0xFF, 0xC0, 0x2B, 0x2B, 0xFF, 0xED, 0x38, 0x38, 0xFF,
	0xED, 0x38, 0x38, 0xFF, 0xC0, 0x2B, 0x2B, 0xFF, 0xF7, 0x9D, 0x1F, 0xFF, 0xE2, 0x86, 0x07, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xC0, 0x2B, 0x2B, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xBD, 0x6E, 0x00, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xC0, 0x2B, 0x2B, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xE2, 0x86, 0x07, 0xFF, 0xF7, 0x9D, 0x1F, 0xFF, 0xC0, 0x2B, 0x2B, 0xFF, 0xED, 0x38, 0x38, 0xFF,
	0xED, 0x38, 0x38, 0xFF, 0xC0, 0x2B, 0x2B, 0xFF, 0xF7, 0x9D, 0x1F, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xC0, 0x2B, 0x2B, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xCC, 0x77, 0x00, 0xFF, 0xCC, 0x77, 0x00, 0xFF, 0xCC, 0x77, 0x00, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xC0, 0x2B, 0x2B, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xF7, 0x9D, 0x1F, 0xFF, 0xC0, 0x2B, 0x2B, 0xFF, 0xED, 0x38, 0x38, 0xFF,
	0xED, 0x38, 0x38, 0xFF, 0xC0, 0x2B, 0x2B, 0xFF, 0xF7, 0x9D, 0x1F, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xE2, 0x86, 0x07, 0xFF, 0xE2, 0x86, 0x07, 0xFF, 0xE2, 0x86, 0x07, 0xFF, 0xE2, 0x86, 0x07, 0xFF, 0xE2, 0x86, 0x07, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xF7, 0x9D, 0x1F, 0xFF, 0xC0, 0x2B, 0x2B, 0xFF, 0xED, 0x38, 0x38, 0xFF,
	0xED, 0x38, 0x38, 0xFF, 0xC0, 0x2B, 0x2B, 0xFF, 0xF7, 0x9D, 0x1F, 0xFF, 0xF7, 0x9D, 0x1F, 0xFF, 0xF7, 0x9D, 0x1F, 0xFF, 0xF7, 0x9D, 0x1F, 0xFF, 0xF7, 0x9D, 0x1F, 0xFF, 0xF7, 0x9D, 0x1F, 0xFF, 0xF7, 0x9D, 0x1F, 0xFF, 0xF7, 0x9D, 0x1F, 0xFF, 0xF7, 0x9D, 0x1F, 0xFF, 0xF7, 0x9D, 0x1F, 0xFF, 0xF7, 0x9D, 0x1F, 0xFF, 0xC0, 0x2B, 0x2B, 0xFF, 0xED, 0x38, 0x38, 0xFF,
	0xED, 0x38, 0x38, 0xFF, 0xE2, 0x1D, 0x1D, 0xFF, 0xC0, 0x2B, 0x2B, 0xFF, 0xC0, 0x2B, 0x2B, 0xFF, 0xC0, 0x2B, 0x2B, 0xFF, 0xC0, 0x2B, 0x2B, 0xFF, 0xC0, 0x2B, 0x2B, 0xFF, 0xC0, 0x2B, 0x2B, 0xFF, 0xC0, 0x2B, 0x2B, 0xFF, 0xC0, 0x2B, 0x2B, 0xFF, 0xC0, 0x2B, 0x2B, 0xFF, 0xC0, 0x2B, 0x2B, 0xFF, 0xC0, 0x2B, 0x2B, 0xFF, 0xE2, 0x1D, 0x1D, 0xFF, 0xED, 0x38, 0x38, 0xFF,
	0x00, 0x00, 0x00, 0x00, 0xED, 0x38, 0x38, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0xED, 0x38, 0x38, 0xFF, 0x00, 0x00, 0x00, 0x00
};

#include "fd-img-128x128.c"


#if 0
#include <stdarg.h>
#include <sys/types.h>								// For MacOS <dirent.h> dependency
#include <dirent.h>
#include <SDL.h>
#include <string>
#include <vector>
#include <algorithm>
#include <ctype.h>									// For toupper()
#include "settings.h"
#include "tom.h"
#include "video.h"
#include "clock.h"
#include "font1.h"
#include "font14pt.h"								// Also 15, 16, 17, 18
#include "guielements.h"
#include "crc32.h"
#include "zlib.h"
#include "unzip.h"
#include "sdlemu_opengl.h"
#include "gui.h"

using namespace std;								// For STL stuff

// Private function prototypes

class Window;										// Forward declaration...

//void DrawTransparentBitmap(uint32 * screen, uint32 x, uint32 y, uint32 * bitmap, uint8 * alpha = NULL);
void DrawTransparentBitmapDeprecated(uint32 * screen, uint32 x, uint32 y, uint32 * bitmap);
void DrawTransparentBitmap(uint32 * screen, uint32 x, uint32 y, const void * bitmap);
void DrawBitmap(uint32 * screen, uint32 x, uint32 y, const void * bitmap);
//Should call this FillScreenRectangle with a number representing the RGBA value to fill. !!! FIX !!!
//void ClearScreenRectangle(uint32 * screen, uint32 x, uint32 y, uint32 w, uint32 h);
void FillScreenRectangle(uint32 * screen, uint32 x, uint32 y, uint32 w, uint32 h, uint32 color);
void DrawStringTrans(uint32 * screen, uint32 x, uint32 y, uint32 color, uint8 opacity, const char * text, ...);
void DrawStringOpaque(uint32 * screen, uint32 x, uint32 y, uint32 color1, uint32 color2, const char * text, ...);
void DrawString(uint32 * screen, uint32 x, uint32 y, bool invert, const char * text, ...);
void DrawString2(uint32 * screen, uint32 x, uint32 y, uint32 color, uint8 transparency, const char * text, ...);
Window * LoadROM(void);
Window * ResetJaguar(void);
Window * ResetJaguarCD(void);
Window * RunEmu(void);
Window * Quit(void);
Window * About(void);
Window * MiscOptions(void);

int gzfilelength(gzFile gd);

// External variables

extern uint8 * jaguar_mainRam;
extern uint8 * jaguar_mainRom;
extern uint8 * jaguar_bootRom;
extern uint8 * jaguar_CDBootROM;
extern bool BIOSLoaded;
extern bool CDBIOSLoaded;

// Local global variables

bool exitGUI = false;								// GUI (emulator) done variable
int mouseX = 0, mouseY = 0;
uint32 background[1280 * 256];						// GUI background buffer

char separator[] = "--------------------------------------------------------";

//
// Case insensitive string compare function
// Taken straight out of Thinking In C++ by Bruce Eckel. Thanks Bruce!
//

int stringCmpi(const string &s1, const string &s2)
{
	// Select the first element of each string:
	string::const_iterator p1 = s1.begin(), p2 = s2.begin();

	while (p1 != s1.end() && p2 != s2.end())		// Don�t run past the end
	{
		if (toupper(*p1) != toupper(*p2))			// Compare upper-cased chars
			return (toupper(*p1) < toupper(*p2) ? -1 : 1);// Report which was lexically greater

		p1++;
		p2++;
	}

	// If they match up to the detected eos, say which was longer. Return 0 if the same.
	return s2.size() - s1.size();
}

//
// Local GUI classes
//

enum { WINDOW_CLOSE, MENU_ITEM_CHOSEN };

class Element
{
	public:
		Element(uint32 x = 0, uint32 y = 0, uint32 w = 0, uint32 h = 0)
			{ extents.x = x, extents.y = y, extents.w = w, extents.h = h; }
		virtual void HandleKey(SDLKey key) = 0;		// These are "pure" virtual functions...
		virtual void HandleMouseMove(uint32 x, uint32 y) = 0;
		virtual void HandleMouseButton(uint32 x, uint32 y, bool mouseDown) = 0;
		virtual void Draw(uint32, uint32) = 0;
		virtual void Notify(Element *) = 0;
//Needed?		virtual ~Element() = 0;
//We're not allocating anything in the base class, so the answer would be NO.
		bool Inside(uint32 x, uint32 y);
		// Class method
//		static void SetScreenAndPitch(int16 * s, uint32 p) { screenBuffer = s, pitch = p; }
		static void SetScreenAndPitch(uint32 * s, uint32 p) { screenBuffer = s, pitch = p; }

	protected:
		SDL_Rect extents;
		uint32 state;
		// Class variables...
//		static int16 * screenBuffer;
		static uint32 * screenBuffer;
		static uint32 pitch;
};

// Initialize class variables (Element)
//int16 * Element::screenBuffer = NULL;
uint32 * Element::screenBuffer = NULL;
uint32 Element::pitch = 0;

bool Element::Inside(uint32 x, uint32 y)
{
	return (x >= (uint32)extents.x && x < (uint32)(extents.x + extents.w)
		&& y >= (uint32)extents.y && y < (uint32)(extents.y + extents.h) ? true : false);
}


//
// Button class
//

class Button: public Element
{
	public:
		Button(uint32 x = 0, uint32 y = 0, uint32 w = 0, uint32 h = 0): Element(x, y, w, h),
			activated(false), clicked(false), inside(false), fgColor(0xFFFFFFFF),
			bgColor(0xFF00FF00), pic(NULL), elementToTell(NULL) {}
		Button(uint32 x, uint32 y, uint32 w, uint32 h, uint32 * p): Element(x, y, w, h),
			activated(false), clicked(false), inside(false), fgColor(0xFFFFFFFF),
			bgColor(0xFF00FF00), pic(p), elementToTell(NULL) {}
//		Button(uint32 x, uint32 y, uint32 * p): Element(x, y, 0, 0),
		Button(uint32 x, uint32 y, uint32 * p, uint32 * pH = NULL, uint32 * pD = NULL): Element(x, y, 0, 0),
			activated(false), clicked(false), inside(false), fgColor(0xFFFFFFFF),
			bgColor(0xFF00FF00), pic(p), picHover(pH), picDown(pD), elementToTell(NULL)
			{ if (pic) extents.w = pic[0], extents.h = pic[1]; }
		Button(uint32 x, uint32 y, uint32 w, uint32 h, string s): Element(x, y, w, h),
			activated(false), clicked(false), inside(false), fgColor(0xFFFFFFFF),
			bgColor(0xFF00FF00), pic(NULL), text(s), elementToTell(NULL) {}
		Button(uint32 x, uint32 y, string s): Element(x, y, 0, FONT_HEIGHT),
			activated(false), clicked(false), inside(false), fgColor(0xFFFFFFFF),
			bgColor(0xFF00FF00), pic(NULL), text(s), elementToTell(NULL)
			{ extents.w = s.length() * FONT_WIDTH; }
		virtual void HandleKey(SDLKey key) {}
		virtual void HandleMouseMove(uint32 x, uint32 y);
		virtual void HandleMouseButton(uint32 x, uint32 y, bool mouseDown);
		virtual void Draw(uint32 offsetX = 0, uint32 offsetY = 0);
		virtual void Notify(Element *) {}
		bool ButtonClicked(void) { return activated; }
		void SetNotificationElement(Element * e) { elementToTell = e; }

	protected:
		bool activated, clicked, inside;
		uint32 fgColor, bgColor;
		uint32 * pic, * picHover, * picDown;
		string text;
		Element * elementToTell;
};

void Button::HandleMouseMove(uint32 x, uint32 y)
{
	inside = Inside(x, y);
}

void Button::HandleMouseButton(uint32 x, uint32 y, bool mouseDown)
{
	if (inside)
	{
		if (mouseDown)
			clicked = true;

		if (clicked && !mouseDown)
		{
			clicked = false, activated = true;

			// Send a message that we're activated (if there's someone to tell, that is)
			if (elementToTell)
				elementToTell->Notify(this);
		}
	}
	else
		clicked = activated = false;
}

void Button::Draw(uint32 offsetX/*= 0*/, uint32 offsetY/*= 0*/)
{
	uint32 addr = (extents.x + offsetX) + ((extents.y + offsetY) * pitch);

	if (text.length() > 0)							// Simple text button
//	if (pic == NULL)
	{
		for(uint32 y=0; y<extents.h; y++)
		{
			for(uint32 x=0; x<extents.w; x++)
			{
				// Doesn't clip in y axis! !!! FIX !!!
				if (extents.x + x < pitch)
					screenBuffer[addr + x + (y * pitch)]
//					= (clicked && inside ? fgColor : (inside ? 0x43F0 : bgColor));
//43F0 -> 010000 11111 10000 -> 0100 0001 1111 1111 1000 0100 -> 41 FF 84
						= (clicked && inside ? fgColor : (inside ? 0xFF84FF41 : bgColor));
			}
		}

		DrawString(screenBuffer, extents.x + offsetX, extents.y + offsetY, false, "%s", text.c_str());
	}
	else											// Graphical button
	{
		uint32 * picToShow = pic;

		if (picHover != NULL && inside && !clicked)
			picToShow = picHover;

		if (picDown != NULL && inside && clicked)
			picToShow = picDown;

		DrawTransparentBitmapDeprecated(screenBuffer, extents.x + offsetX, extents.y + offsetY, picToShow);
	}
}


//
// PushButton class
//

class PushButton: public Element
{
// How to handle?
// Save state externally?
//We pass in a state variable if we want to track it externally, otherwise we use our own
//internal state var. Still need to do some kind of callback for pushbuttons that do things
//like change from fullscreen to windowed... !!! FIX !!!

	public:
//		PushButton(uint32 x = 0, uint32 y = 0, uint32 w = 0, uint32 h = 0): Element(x, y, w, h),
//			activated(false), clicked(false), inside(false), fgColor(0xFFFF),
//			bgColor(0x03E0), pic(NULL), elementToTell(NULL) {}
//		PushButton(uint32 x, uint32 y, bool * st, string s): Element(x, y, 8, 8), state(st),
//			inside(false), text(s) { if (st == NULL) state = &internalState; }
		PushButton(uint32 x, uint32 y, bool * st, string s): Element(x, y, 16, 16), state(st),
			inside(false), text(s) { if (st == NULL) state = &internalState; }
/*		Button(uint32 x, uint32 y, uint32 w, uint32 h, uint32 * p): Element(x, y, w, h),
			activated(false), clicked(false), inside(false), fgColor(0xFFFF),
			bgColor(0x03E0), pic(p), elementToTell(NULL) {}
		Button(uint32 x, uint32 y, uint32 * p): Element(x, y, 0, 0),
			activated(false), clicked(false), inside(false), fgColor(0xFFFF),
			bgColor(0x03E0), pic(p), elementToTell(NULL)
			{ if (pic) extents.w = pic[0], extents.h = pic[1]; }
		Button(uint32 x, uint32 y, uint32 w, uint32 h, string s): Element(x, y, w, h),
			activated(false), clicked(false), inside(false), fgColor(0xFFFF),
			bgColor(0x03E0), pic(NULL), text(s), elementToTell(NULL) {}
		PushButton(uint32 x, uint32 y, string s): Element(x, y, 0, 8),
			activated(false), clicked(false), inside(false), fgColor(0xFFFF),
			bgColor(0x03E0), pic(NULL), text(s), elementToTell(NULL)
			{ extents.w = s.length() * 8; }*/
		virtual void HandleKey(SDLKey key) {}
		virtual void HandleMouseMove(uint32 x, uint32 y);
		virtual void HandleMouseButton(uint32 x, uint32 y, bool mouseDown);
		virtual void Draw(uint32 offsetX = 0, uint32 offsetY = 0);
		virtual void Notify(Element *) {}
//		bool ButtonClicked(void) { return activated; }
//		void SetNotificationElement(Element * e) { elementToTell = e; }

	protected:
		bool * state;
		bool inside;
//		bool activated, clicked, inside;
//		uint16 fgColor, bgColor;
//		uint32 * pic;
		string text;
//		Element * elementToTell;
		bool internalState;
};

void PushButton::HandleMouseMove(uint32 x, uint32 y)
{
	inside = Inside(x, y);
}

void PushButton::HandleMouseButton(uint32 x, uint32 y, bool mouseDown)
{
	if (inside && mouseDown)
	{
/*		if (mouseDown)
			clicked = true;

		if (clicked && !mouseDown)
		{
			clicked = false, activated = true;

			// Send a message that we're activated (if there's someone to tell, that is)
			if (elementToTell)
				elementToTell->Notify(this);
		}*/
		*state = !(*state);
	}
//	else
//		clicked = activated = false;
}

void PushButton::Draw(uint32 offsetX/*= 0*/, uint32 offsetY/*= 0*/)
{
/*	uint32 addr = (extents.x + offsetX) + ((extents.y + offsetY) * pitch);

	for(uint32 y=0; y<extents.h; y++)
	{
		for(uint32 x=0; x<extents.w; x++)
		{
			// Doesn't clip in y axis! !!! FIX !!!
			if (extents.x + x < pitch)
				screenBuffer[addr + x + (y * pitch)]
					= (clicked && inside ? fgColor : (inside ? 0x43F0 : bgColor));
		}
	}*/

	if (*state)
		DrawTransparentBitmap(screenBuffer, extents.x + offsetX, extents.y + offsetY, &pbDown);
	else
		DrawTransparentBitmap(screenBuffer, extents.x + offsetX, extents.y + offsetY, &pbUp);

	if (text.length() > 0)
		DrawString(screenBuffer, extents.x + offsetX + 24, extents.y + offsetY, false, "%s", text.c_str());
}


//
// SlideSwitch class
//

class SlideSwitch: public Element
{
// How to handle?
// Save state externally?
//Seems to be handled the same as PushButton, but without sanity checks. !!! FIX !!!

	public:
		SlideSwitch(uint32 x, uint32 y, bool * st, string s1, string s2): Element(x, y, 16, 32), state(st),
			inside(false), text1(s1), text2(s2) {}
		virtual void HandleKey(SDLKey key) {}
		virtual void HandleMouseMove(uint32 x, uint32 y);
		virtual void HandleMouseButton(uint32 x, uint32 y, bool mouseDown);
		virtual void Draw(uint32 offsetX = 0, uint32 offsetY = 0);
		virtual void Notify(Element *) {}
//		bool ButtonClicked(void) { return activated; }
//		void SetNotificationElement(Element * e) { elementToTell = e; }

	protected:
		bool * state;
		bool inside;
//		bool activated, clicked, inside;
//		uint16 fgColor, bgColor;
//		uint32 * pic;
		string text1, text2;
//		Element * elementToTell;
};

void SlideSwitch::HandleMouseMove(uint32 x, uint32 y)
{
	inside = Inside(x, y);
}

void SlideSwitch::HandleMouseButton(uint32 x, uint32 y, bool mouseDown)
{
	if (inside && mouseDown)
	{
/*		if (mouseDown)
			clicked = true;

		if (clicked && !mouseDown)
		{
			clicked = false, activated = true;

			// Send a message that we're activated (if there's someone to tell, that is)
			if (elementToTell)
				elementToTell->Notify(this);
		}*/
		*state = !(*state);
	}
//	else
//		clicked = activated = false;
}

void SlideSwitch::Draw(uint32 offsetX/*= 0*/, uint32 offsetY/*= 0*/)
{
	DrawTransparentBitmapDeprecated(screenBuffer, extents.x + offsetX, extents.y + offsetY, (*state ? slideSwitchDown : slideSwitchUp));

	if (text1.length() > 0)
		DrawString(screenBuffer, extents.x + offsetX + 24, extents.y + offsetY, false, "%s", text1.c_str());

	if (text2.length() > 0)
		DrawString(screenBuffer, extents.x + offsetX + 24, extents.y + offsetY + 16, false, "%s", text2.c_str());
}


//
// Window class
//

class Window: public Element
{
	public:
/*		Window(uint32 x = 0, uint32 y = 0, uint32 w = 0, uint32 h = 0):	Element(x, y, w, h),
			fgColor(0x4FF0), bgColor(0xFE10)
			{ close = new Button(w - 8, 1, closeBox); list.push_back(close); }*/
		Window(uint32 x = 0, uint32 y = 0, uint32 w = 0, uint32 h = 0,
			void (* f)(Element *) = NULL): Element(x, y, w, h),
//			/*clicked(false), inside(false),*/ fgColor(0x4FF0), bgColor(0x1E10),
//4FF0 -> 010011 11111 10000 -> 0100 1101 1111 1111 1000 0100 -> 4D FF 84
//1E10 -> 000111 10000 10000 -> 0001 1111 1000 0100 1000 0100 -> 1F 84 84
			/*clicked(false), inside(false),*/ fgColor(0xFF84FF4D), bgColor(0xFF84841F),
			handler(f)
			{ close = new Button(w - (CLOSEBOX_WIDTH + 1), 1, closeBox, closeBoxHover, closeBoxDown);
			  list.push_back(close);
			  close->SetNotificationElement(this); }
		virtual ~Window();
		virtual void HandleKey(SDLKey key);
		virtual void HandleMouseMove(uint32 x, uint32 y);
		virtual void HandleMouseButton(uint32 x, uint32 y, bool mouseDown);
		virtual void Draw(uint32 offsetX = 0, uint32 offsetY = 0);
		virtual void Notify(Element * e);
		void AddElement(Element * e);
//		bool WindowActive(void) { return true; }//return !close->ButtonClicked(); }

	protected:
//		bool clicked, inside;
		uint32 fgColor, bgColor;
		void (* handler)(Element *);
		Button * close;
//We have to use a list of Element *pointers* because we can't make a list that will hold
//all the different object types in the same list...
		vector<Element *> list;
};

Window::~Window()
{
	for(uint32 i=0; i<list.size(); i++)
		if (list[i])
			delete list[i];
}

void Window::HandleKey(SDLKey key)
{
	if (key == SDLK_ESCAPE)
	{
		SDL_Event event;
		event.type = SDL_USEREVENT, event.user.code = WINDOW_CLOSE;
		SDL_PushEvent(&event);
	}

	// Handle the items this window contains...
	for(uint32 i=0; i<list.size(); i++)
		// Make coords relative to upper right corner of this window...
		list[i]->HandleKey(key);
}

void Window::HandleMouseMove(uint32 x, uint32 y)
{
	// Handle the items this window contains...
	for(uint32 i=0; i<list.size(); i++)
		// Make coords relative to upper right corner of this window...
		list[i]->HandleMouseMove(x - extents.x, y - extents.y);
}

void Window::HandleMouseButton(uint32 x, uint32 y, bool mouseDown)
{
	// Handle the items this window contains...
	for(uint32 i=0; i<list.size(); i++)
		// Make coords relative to upper right corner of this window...
		list[i]->HandleMouseButton(x - extents.x, y - extents.y, mouseDown);
}

void Window::Draw(uint32 offsetX/*= 0*/, uint32 offsetY/*= 0*/)
{
	uint32 addr = (extents.x + offsetX) + ((extents.y + offsetY) * pitch);

	for(uint32 y=0; y<extents.h; y++)
	{
		for(uint32 x=0; x<extents.w; x++)
		{
			// Doesn't clip in y axis! !!! FIX !!!
			if (extents.x + x < pitch)
				screenBuffer[addr + x + (y * pitch)] = bgColor;
		}
	}

	// Handle the items this window contains...
	for(uint32 i=0; i<list.size(); i++)
		list[i]->Draw(extents.x, extents.y);
}

void Window::AddElement(Element * e)
{
	list.push_back(e);
}

void Window::Notify(Element * e)
{
	if (e == close)
	{
		SDL_Event event;
		event.type = SDL_USEREVENT, event.user.code = WINDOW_CLOSE;
		SDL_PushEvent(&event);
	}
}


//
// Static text class
//

class Text: public Element
{
	public:
//		Text(uint32 x = 0, uint32 y = 0, uint32 w = 0, uint32 h = 0): Element(x, y, w, h),
//			fgColor(0x4FF0), bgColor(0xFE10) {}
//		Text(uint32 x, uint32 y, string s, uint16 fg = 0x4FF0, uint16 bg = 0xFE10): Element(x, y, 0, 0),
//			fgColor(fg), bgColor(bg), text(s) {}
//4FF0 -> 010011 11111 10000 -> 0100 1101 1111 1111 1000 0100 -> 4D FF 84
//FE10 -> 111111 10000 10000 -> 1111 1111 1000 0100 1000 0100 -> FF 84 84
		Text(uint32 x = 0, uint32 y = 0, uint32 w = 0, uint32 h = 0): Element(x, y, w, h),
			fgColor(0xFF8484FF), bgColor(0xFF84FF4D) {}
		Text(uint32 x, uint32 y, string s, uint32 fg = 0xFF8484FF, uint32 bg = 0xFF84FF4D):
			Element(x, y, 0, 0), fgColor(fg), bgColor(bg), text(s) {}
		virtual void HandleKey(SDLKey key) {}
		virtual void HandleMouseMove(uint32 x, uint32 y) {}
		virtual void HandleMouseButton(uint32 x, uint32 y, bool mouseDown) {}
		virtual void Draw(uint32 offsetX = 0, uint32 offsetY = 0);
		virtual void Notify(Element *) {}

	protected:
		uint32 fgColor, bgColor;
		string text;
};

void Text::Draw(uint32 offsetX/*= 0*/, uint32 offsetY/*= 0*/)
{
	if (text.length() > 0)
//		DrawString(screenBuffer, extents.x + offsetX, extents.y + offsetY, false, "%s", text.c_str());
		DrawStringOpaque(screenBuffer, extents.x + offsetX, extents.y + offsetY, fgColor, bgColor, "%s", text.c_str());
}


//
// Static image class
//

class Image: public Element
{
	public:
		Image(uint32 x, uint32 y, const void * img): Element(x, y, 0, 0), image(img) {}
		virtual void HandleKey(SDLKey key) {}
		virtual void HandleMouseMove(uint32 x, uint32 y) {}
		virtual void HandleMouseButton(uint32 x, uint32 y, bool mouseDown) {}
		virtual void Draw(uint32 offsetX = 0, uint32 offsetY = 0);
		virtual void Notify(Element *) {}

	protected:
		uint32 fgColor, bgColor;
		const void * image;
};

void Image::Draw(uint32 offsetX/*= 0*/, uint32 offsetY/*= 0*/)
{
	if (image != NULL)
		DrawTransparentBitmap(screenBuffer, extents.x + offsetX, extents.y + offsetY, image);
}


//
// TextEdit class
//

class TextEdit: public Element
{
	public:
		TextEdit(uint32 x = 0, uint32 y = 0, uint32 w = 0, uint32 h = 0): Element(x, y, w, h),
			fgColor(0xFF8484FF), bgColor(0xFF84FF4D), text(""), caretPos(0),
			maxScreenSize(10) {}
		TextEdit(uint32 x, uint32 y, string s, uint32 mss = 10, uint32 fg = 0xFF8484FF,
			uint32 bg = 0xFF84FF4D): Element(x, y, 0, 0), fgColor(fg), bgColor(bg), text(s),
			caretPos(0), maxScreenSize(mss) {}
		virtual void HandleKey(SDLKey key);
		virtual void HandleMouseMove(uint32 x, uint32 y) {}
		virtual void HandleMouseButton(uint32 x, uint32 y, bool mouseDown) {}
		virtual void Draw(uint32 offsetX = 0, uint32 offsetY = 0);
		virtual void Notify(Element *) {}

	protected:
		uint32 fgColor, bgColor;
		string text;
		uint32 caretPos;
		uint32 maxScreenSize;
};

//Set different filters depending on type passed in on construction, e.g., filename, amount, etc...?
void TextEdit::HandleKey(SDLKey key)
{
	if ((key >= SDLK_a && key <= SDLK_z) || (key >= SDLK_0 && key <= SDLK_9) || key == SDLK_PERIOD
		|| key == SDLK_SLASH)
	{
		//Need to handle shift key as well...
		text[caretPos++] = key;
		Draw();
	}
	else if (key == SDLK_BACKSPACE)
	{

	}
	else if (key == SDLK_DELETE)
	{
	}
//left, right arrow
}

void TextEdit::Draw(uint32 offsetX/*= 0*/, uint32 offsetY/*= 0*/)
{
	if (text.length() > 0)
	{
		FillScreenRectangle(screenBuffer, extents.x + offsetX, extents.y + offsetY, FONT_WIDTH * maxScreenSize, FONT_HEIGHT, bgColor);
//		DrawString(screenBuffer, extents.x + offsetX, extents.y + offsetY, false, "%s", text.c_str());
		DrawStringOpaque(screenBuffer, extents.x + offsetX, extents.y + offsetY, fgColor, bgColor, "%s", text.c_str());
	}

	// Draw the caret (underscore? or vertical line?)
}


//
// ListBox class
//

class ListBox: public Element
//class ListBox: public Window
{
	public:
//		ListBox(uint32 x = 0, uint32 y = 0, uint32 w = 0, uint32 h = 0): Element(x, y, w, h),
		ListBox(uint32 x = 0, uint32 y = 0, uint32 w = 0, uint32 h = 0);//: Window(x, y, w, h),
//		windowPtr(0), cursor(0), limit(0), charWidth((w / 8) - 1), charHeight(h / 8),
//		elementToTell(NULL), upArrow(w - 8, 0, upArrowBox),
//		downArrow(w - 8, h - 8, downArrowBox), upArrow2(w - 8, h - 16, upArrowBox) {}
		virtual void HandleKey(SDLKey key);
		virtual void HandleMouseMove(uint32 x, uint32 y);
		virtual void HandleMouseButton(uint32 x, uint32 y, bool mouseDown);
		virtual void Draw(uint32 offsetX = 0, uint32 offsetY = 0);
		virtual void Notify(Element * e);
		void SetNotificationElement(Element * e) { elementToTell = e; }
		void AddItem(string s);
		string GetSelectedItem(void);

	protected:
		bool thumbClicked;
		uint32 windowPtr, cursor, limit;
		uint32 charWidth, charHeight;				// Box width/height in characters
		Element * elementToTell;
		Button upArrow, downArrow, upArrow2;
		vector<string> item;

	private:
		uint32 yRelativePoint;
};

ListBox::ListBox(uint32 x, uint32 y, uint32 w, uint32 h): Element(x, y, w, h),
	thumbClicked(false), windowPtr(0), cursor(0), limit(0), charWidth((w / FONT_WIDTH) - 1),
	charHeight(h / FONT_HEIGHT), elementToTell(NULL), upArrow(w - 8, 0, upArrowBox),
	downArrow(w - 8, h - 8, downArrowBox), upArrow2(w - 8, h - 16, upArrowBox)
{
	upArrow.SetNotificationElement(this);
	downArrow.SetNotificationElement(this);
	upArrow2.SetNotificationElement(this);
	extents.w -= 8;									// Make room for scrollbar...
}

void ListBox::HandleKey(SDLKey key)
{
	if (key == SDLK_DOWN)
	{
		if (cursor != limit - 1)	// Cursor is within its window
			cursor++;
		else						// Otherwise, scroll the window...
		{
			if (cursor + windowPtr != item.size() - 1)
				windowPtr++;
		}
	}
	else if (key == SDLK_UP)
	{
		if (cursor != 0)
			cursor--;
		else
		{
			if (windowPtr != 0)
				windowPtr--;
		}
	}
	else if (key == SDLK_PAGEDOWN)
	{
		if (cursor != limit - 1)
			cursor = limit - 1;
		else
		{
			windowPtr += limit;
			if (windowPtr > item.size() - limit)
				windowPtr = item.size() - limit;
		}
	}
	else if (key == SDLK_PAGEUP)
	{
		if (cursor != 0)
			cursor = 0;
		else
		{
			if (windowPtr < limit)
				windowPtr = 0;
			else
				windowPtr -= limit;
		}
	}
	else if (key >= SDLK_a && key <= SDLK_z)
	{
		// Advance cursor to filename with first letter pressed...
		uint8 which = (key - SDLK_a) + 65;	// Convert key to A-Z char

		for(uint32 i=0; i<item.size(); i++)
		{
			if ((item[i][0] & 0xDF) == which)
			{
				cursor = i - windowPtr;
				if (i > windowPtr + limit - 1)
					windowPtr = i - limit + 1, cursor = limit - 1;
				if (i < windowPtr)
					windowPtr = i, cursor = 0;
				break;
			}
		}
	}
}

void ListBox::HandleMouseMove(uint32 x, uint32 y)
{
	upArrow.HandleMouseMove(x - extents.x, y - extents.y);
	downArrow.HandleMouseMove(x - extents.x, y - extents.y);
	upArrow2.HandleMouseMove(x - extents.x, y - extents.y);

	if (thumbClicked)
	{
		uint32 sbHeight = extents.h - 24,
			thumb = (uint32)(((float)limit / (float)item.size()) * (float)sbHeight);

//yRelativePoint is the spot on the thumb where we clicked...
		int32 newThumbStart = y - yRelativePoint;

		if (newThumbStart < 0)
			newThumbStart = 0;

		if ((uint32)newThumbStart > sbHeight - thumb)
			newThumbStart = sbHeight - thumb;

		windowPtr = (uint32)(((float)newThumbStart / (float)sbHeight) * (float)item.size());
//Check for cursor bounds as well... Or do we need to???
//Actually, we don't...!
	}
}

void ListBox::HandleMouseButton(uint32 x, uint32 y, bool mouseDown)
{
	if (Inside(x, y) && mouseDown)
	{
		// Why do we have to do this??? (- extents.y?)
		// I guess it's because only the Window class has offsetting implemented... !!! FIX !!!
//		cursor = (y - extents.y) / 8;
		cursor = (y - extents.y) / FONT_HEIGHT;
	}

	// Check for a hit on the scrollbar...
	if (x > (uint32)(extents.x + extents.w) && x <= (uint32)(extents.x + extents.w + 8)
		&& y > (uint32)(extents.y + 8) && y <= (uint32)(extents.y + extents.h - 16))
	{
		if (mouseDown)
		{
// This shiaut should be calculated in AddItem(), not here... (or in Draw() for that matter)
			uint32 sbHeight = extents.h - 24,
				thumb = (uint32)(((float)limit / (float)item.size()) * (float)sbHeight),
				thumbStart = (uint32)(((float)windowPtr / (float)item.size()) * (float)sbHeight);

			// Did we hit the thumb?
			if (y >= (extents.y + 8 + thumbStart) && y < (extents.y + 8 + thumbStart + thumb))
				thumbClicked = true, yRelativePoint = y - thumbStart;
		}
//Seems that this is useless--never reached except in rare cases and that the code outside is
//more effective...
//		else
//			thumbClicked = false;
	}

	if (!mouseDown)
		thumbClicked = false;

	upArrow.HandleMouseButton(x - extents.x, y - extents.y, mouseDown);
	downArrow.HandleMouseButton(x - extents.x, y - extents.y, mouseDown);
	upArrow2.HandleMouseButton(x - extents.x, y - extents.y, mouseDown);
}

void ListBox::Draw(uint32 offsetX/*= 0*/, uint32 offsetY/*= 0*/)
{
	for(uint32 i=0; i<limit; i++)
	{
		// Strip off the extension
		// (extension stripping should be an option, not default!)
		string s(item[windowPtr + i], 0, item[windowPtr + i].length() - 4);
//		DrawString(screenBuffer, extents.x + offsetX, extents.y + offsetY + i*8,
		DrawString(screenBuffer, extents.x + offsetX, extents.y + offsetY + i*FONT_HEIGHT,
			(cursor == i ? true : false), "%-*.*s", charWidth, charWidth, s.c_str());
	}

	upArrow.Draw(extents.x + offsetX, extents.y + offsetY);
	downArrow.Draw(extents.x + offsetX, extents.y + offsetY);
	upArrow2.Draw(extents.x + offsetX, extents.y + offsetY);

	uint32 sbHeight = extents.h - 24,
		thumb = (uint32)(((float)limit / (float)item.size()) * (float)sbHeight),
		thumbStart = (uint32)(((float)windowPtr / (float)item.size()) * (float)sbHeight);

	for(uint32 y=extents.y+offsetY+8; y<extents.y+offsetY+extents.h-16; y++)
	{
//		for(uint32 x=extents.x+offsetX+extents.w-8; x<extents.x+offsetX+extents.w; x++)
		for(uint32 x=extents.x+offsetX+extents.w; x<extents.x+offsetX+extents.w+8; x++)
		{
			if (y >= thumbStart + (extents.y+offsetY+8) && y < thumbStart + thumb + (extents.y+offsetY+8))
//				screenBuffer[x + (y * pitch)] = (thumbClicked ? 0x458E : 0xFFFF);
//458E -> 01 0001  0 1100  0 1110 -> 0100 0101  0110 0011  0111 0011 -> 45 63 73
				screenBuffer[x + (y * pitch)] = (thumbClicked ? 0xFF736345 : 0xFFFFFFFF);
			else
//				screenBuffer[x + (y * pitch)] = 0x0200;
//0200 -> 000000 10000 00000 -> 00 1000 0100 00
				screenBuffer[x + (y * pitch)] = 0xFF008400;
		}
	}
}

void ListBox::Notify(Element * e)
{
	if (e == &upArrow || e == &upArrow2)
	{
		if (windowPtr != 0)
		{
			windowPtr--;

			if (cursor < limit - 1)
				cursor++;
		}
	}
	else if (e == &downArrow)
	{
		if (windowPtr < item.size() - limit)
		{
			windowPtr++;

			if (cursor != 0)
				cursor--;
		}
	}
}

void ListBox::AddItem(string s)
{
	// Do a simple insertion sort
	bool inserted = false;

	for(vector<string>::iterator i=item.begin(); i<item.end(); i++)
	{
		if (stringCmpi(s, *i) == -1)
		{
			item.insert(i, s);
			inserted = true;
			break;
		}
	}

	if (!inserted)
		item.push_back(s);

	limit = (item.size() > charHeight ? charHeight : item.size());
}

string ListBox::GetSelectedItem(void)
{
	return item[windowPtr + cursor];
}


//
// FileList class
//

class FileList: public Window
{
	public:
		FileList(uint32 x = 0, uint32 y = 0, uint32 w = 0, uint32 h = 0);
		virtual ~FileList() {}
		virtual void HandleKey(SDLKey key);
		virtual void HandleMouseMove(uint32 x, uint32 y) { Window::HandleMouseMove(x, y); }
		virtual void HandleMouseButton(uint32 x, uint32 y, bool mouseDown) { Window::HandleMouseButton(x, y, mouseDown); }
		virtual void Draw(uint32 offsetX = 0, uint32 offsetY = 0) { Window::Draw(offsetX, offsetY); }
		virtual void Notify(Element * e);

	protected:
		ListBox * files;
		Button * load;
};

//Need 4 buttons, one scrollbar...
FileList::FileList(uint32 x, uint32 y, uint32 w, uint32 h): Window(x, y, w, h)
{
	files = new ListBox(8, 8, w - 16, h - 32);
	AddElement(files);
	load = new Button(8, h - 16, " Load ");
	AddElement(load);
	load->SetNotificationElement(this);

//!!! FIX !!! Directory might not exist--this shouldn't cause VJ to crash!
	DIR * dp = opendir(vjs.ROMPath);
	dirent * de;

	if (dp != NULL)
	{
		while ((de = readdir(dp)) != NULL)
		{
			char * ext = strrchr(de->d_name, '.');

			if (ext != NULL)
				if (strcasecmp(ext, ".zip") == 0 || strcasecmp(ext, ".j64") == 0
					|| strcasecmp(ext, ".abs") == 0 || strcasecmp(ext, ".jag") == 0
					|| strcasecmp(ext, ".rom") == 0)
					files->AddItem(string(de->d_name));
		}

		closedir(dp);
	}
	else
	{
//Give a diagnostic message here so that the (l)user can figure out what went wrong. !!! FIX !!!
	}
}

void FileList::HandleKey(SDLKey key)
{
	if (key == SDLK_RETURN)
		Notify(load);
	else
		Window::HandleKey(key);
}

void FileList::Notify(Element * e)
{
	if (e == load)
	{
		char filename[MAX_PATH];
		strcpy(filename, vjs.ROMPath);

		if (strlen(filename) > 0)
			if (filename[strlen(filename) - 1] != '/')
				strcat(filename, "/");

		strcat(filename, files->GetSelectedItem().c_str());

//		uint32 romSize = JaguarLoadROM(jaguar_mainRom, filename);
//		JaguarLoadCart(jaguar_mainRom, filename);
		if (JaguarLoadFile(filename))
		{
			SDL_Event event;
			event.type = SDL_USEREVENT, event.user.code = WINDOW_CLOSE;
			SDL_PushEvent(&event);

			event.type = SDL_USEREVENT, event.user.code = MENU_ITEM_CHOSEN;
			event.user.data1 = (void *)ResetJaguar;
	    	SDL_PushEvent(&event);
		}
		else
		{
			SDL_Event event;
			event.type = SDL_USEREVENT, event.user.code = WINDOW_CLOSE;
			SDL_PushEvent(&event);

			// Handle the error, but don't run...
			// Tell the user that we couldn't run their file for some reason... !!! FIX !!!
//how to kludge: Make a function like ResetJaguar which creates the dialog window
		}
	}
	else
		Window::Notify(e);
}


//
// Menu class & supporting structs/classes
//

struct NameAction
{
	string name;
	Window * (* action)(void);
	SDLKey hotKey;

	NameAction(string n, Window * (* a)(void) = NULL, SDLKey k = SDLK_UNKNOWN): name(n),
		action(a), hotKey(k) {}
};

class MenuItems
{
	public:
		MenuItems(): charLength(0) {}
		bool Inside(uint32 x, uint32 y)
		{ return (x >= (uint32)extents.x && x < (uint32)(extents.x + extents.w)
		&& y >= (uint32)extents.y && y < (uint32)(extents.y + extents.h) ? true : false); }

		string title;
		vector<NameAction> item;
		uint32 charLength;
		SDL_Rect extents;
};

class Menu: public Element
{
	public:
// 1CFF -> 0 001 11 00  111 1 1111
// 421F -> 0 100 00 10  000 1 1111
		Menu(uint32 x = 0, uint32 y = 0, uint32 w = 0, uint32 h = FONT_HEIGHT,
/*			uint16 fgc = 0x1CFF, uint16 bgc = 0x000F, uint16 fgch = 0x421F,
			uint16 bgch = 0x1CFF): Element(x, y, w, h), activated(false), clicked(false),*/
/*			uint32 fgc = 0xFF3F3F00, uint32 bgc = 0x7F000000, uint32 fgch = 0xFF878700,
			uint32 bgch = 0xFF3F3F00): Element(x, y, w, h), activated(false), clicked(false),*/
/*			uint32 fgc = 0xFFFF3F3F, uint32 bgc = 0xFF7F0000, uint32 fgch = 0xFFFF8787,
			uint32 bgch = 0xFFFF3F3F): Element(x, y, w, h), activated(false), clicked(false),*/
			uint32 fgc = 0xFF7F0000, uint32 bgc = 0xFFFF3F3F, uint32 fgch = 0xFFFF3F3F,
			uint32 bgch = 0xFFFF8787): Element(x, y, w, h), activated(false), clicked(false),
			inside(0), insidePopup(0), fgColor(fgc), bgColor(bgc), fgColorHL(fgch),
			bgColorHL(bgch), menuChosen(-1), menuItemChosen(-1) {}
		virtual void HandleKey(SDLKey key);
		virtual void HandleMouseMove(uint32 x, uint32 y);
		virtual void HandleMouseButton(uint32 x, uint32 y, bool mouseDown);
		virtual void Draw(uint32 offsetX = 0, uint32 offsetY = 0);
		virtual void Notify(Element *) {}
		void Add(MenuItems mi);

	protected:
		bool activated, clicked;
		uint32 inside, insidePopup;
//		uint16 fgColor, bgColor, fgColorHL, bgColorHL;
		uint32 fgColor, bgColor, fgColorHL, bgColorHL;
		int menuChosen, menuItemChosen;

	private:
		vector<MenuItems> itemList;
};

void Menu::HandleKey(SDLKey key)
{
	for(uint32 i=0; i<itemList.size(); i++)
	{
		for(uint32 j=0; j<itemList[i].item.size(); j++)
		{
			if (itemList[i].item[j].hotKey == key)
			{
				SDL_Event event;
				event.type = SDL_USEREVENT;
				event.user.code = MENU_ITEM_CHOSEN;
				event.user.data1 = (void *)itemList[i].item[j].action;
	    		SDL_PushEvent(&event);

				clicked = false, menuChosen = menuItemChosen = -1;
				break;
			}
		}
	}
}

void Menu::HandleMouseMove(uint32 x, uint32 y)
{
	inside = insidePopup = 0;

	if (Inside(x, y))
	{
		// Find out *where* we are inside the menu bar
		uint32 xpos = extents.x;

		for(uint32 i=0; i<itemList.size(); i++)
		{
			uint32 width = (itemList[i].title.length() + 2) * FONT_WIDTH;

			if (x >= xpos && x < xpos + width)
			{
				inside = i + 1;
				menuChosen = i;
				break;
			}

			xpos += width;
		}
	}

	if (!Inside(x, y) && !clicked)
	{
		menuChosen = -1;
	}

	if (itemList[menuChosen].Inside(x, y) && clicked)
	{
		insidePopup = ((y - itemList[menuChosen].extents.y) / FONT_HEIGHT) + 1;
		menuItemChosen = insidePopup - 1;
	}
}

void Menu::HandleMouseButton(uint32 x, uint32 y, bool mouseDown)
{
	if (!clicked)
	{
		if (mouseDown)
		{
			if (inside)
				clicked = true;
			else
				menuChosen = -1;					// clicked is already false...!
		}
	}
	else											// clicked == true
	{
		if (insidePopup && !mouseDown)				// I.e., mouse-button-up
		{
			activated = true;
			if (itemList[menuChosen].item[menuItemChosen].action != NULL)
			{
//				itemList[menuChosen].item[menuItemChosen].action();
				SDL_Event event;
				event.type = SDL_USEREVENT;
				event.user.code = MENU_ITEM_CHOSEN;
				event.user.data1 = (void *)itemList[menuChosen].item[menuItemChosen].action;
			    SDL_PushEvent(&event);

				clicked = false, menuChosen = menuItemChosen = -1;

/*				SDL_Event event;
				while (SDL_PollEvent(&event));		// Flush the event queue...
				event.type = SDL_MOUSEMOTION;
				int mx, my;
				SDL_GetMouseState(&mx, &my);
				event.motion.x = mx, event.motion.y = my;
			    SDL_PushEvent(&event);				// & update mouse position...!
*/			}
		}

		if (!inside && !insidePopup && mouseDown)
			clicked = false, menuChosen = menuItemChosen = -1;
	}
}

void Menu::Draw(uint32 offsetX/*= 0*/, uint32 offsetY/*= 0*/)
{
	uint32 xpos = extents.x + offsetX;

	for(uint32 i=0; i<itemList.size(); i++)
	{
//		uint16 color1 = fgColor, color2 = bgColor;
		uint32 color1 = fgColor, color2 = bgColor;
		if (inside == (i + 1) || (menuChosen != -1 && (uint32)menuChosen == i))
			color1 = fgColorHL, color2 = bgColorHL;

		DrawStringOpaque(screenBuffer, xpos, extents.y + offsetY, color1, color2,
			" %s ", itemList[i].title.c_str());
		xpos += (itemList[i].title.length() + 2) * FONT_WIDTH;
	}

	// Draw sub menu (but only if active)
	if (clicked)
	{
		uint32 ypos = extents.y + FONT_HEIGHT + 1;

		for(uint32 i=0; i<itemList[menuChosen].item.size(); i++)
		{
//			uint16 color1 = fgColor, color2 = bgColor;
			uint32 color1 = fgColor, color2 = bgColor;

			if (insidePopup == i + 1)
				color1 = fgColorHL, color2 = bgColorHL, menuItemChosen = i;

			if (itemList[menuChosen].item[i].name.length() > 0)
				DrawStringOpaque(screenBuffer, itemList[menuChosen].extents.x, ypos,
					color1, color2, " %-*.*s ", itemList[menuChosen].charLength,
					itemList[menuChosen].charLength, itemList[menuChosen].item[i].name.c_str());
			else
				DrawStringOpaque(screenBuffer, itemList[menuChosen].extents.x, ypos,
					fgColor, bgColor, "%.*s", itemList[menuChosen].charLength + 2, separator);

			ypos += FONT_HEIGHT;
		}
	}
}

void Menu::Add(MenuItems mi)
{
	for(uint32 i=0; i<mi.item.size(); i++)
		if (mi.item[i].name.length() > mi.charLength)
			mi.charLength = mi.item[i].name.length();

	// Set extents here as well...
	mi.extents.x = extents.x + extents.w, mi.extents.y = extents.y + FONT_HEIGHT + 1;
	mi.extents.w = (mi.charLength + 2) * FONT_WIDTH, mi.extents.h = mi.item.size() * FONT_HEIGHT;

	itemList.push_back(mi);
	extents.w += (mi.title.length() + 2) * FONT_WIDTH;
}


//Do we even *need* this?
//Doesn't seem like it...
/*class RootWindow: public Window
{
	public:
		RootWindow(Menu * m, Window * w = NULL): menu(m), window(w) {}
//Do we even need to care about this crap?
//			{ extents.x = extents.y = 0, extents.w = 320, extents.h = 240; }
		virtual void HandleKey(SDLKey key) {}
		virtual void HandleMouseMove(uint32 x, uint32 y) {}
		virtual void HandleMouseButton(uint32 x, uint32 y, bool mouseDown) {}
		virtual void Draw(uint32 offsetX = 0, uint32 offsetY = 0) {}
		virtual void Notify(Element *) {}

	private:
		Menu * menu;
		Window * window;
		int16 * rootImage[1280 * 240 * 2];
};//*/


//
// Draw text at the given x/y coordinates. Can invert text as well.
//
void DrawString(uint32 * screen, uint32 x, uint32 y, bool invert, const char * text, ...)
{
	char string[4096];
	va_list arg;

	va_start(arg, text);
	vsprintf(string, text, arg);
	va_end(arg);

	uint32 pitch = sdlemuGetOverlayWidthInPixels();//GetSDLScreenWidthInPixels();
	uint32 length = strlen(string), address = x + (y * pitch);

	uint32 color1 = 0x0080FF;
	uint8 nBlue = (color1 >> 16) & 0xFF, nGreen = (color1 >> 8) & 0xFF, nRed = color1 & 0xFF;
	uint8 xorMask = (invert ? 0xFF : 0x00);

	for(uint32 i=0; i<length; i++)
	{
		uint8 c = string[i];
		uint32 fontAddr = (uint32)(c < 32 ? 0 : c - 32) * FONT_WIDTH * FONT_HEIGHT;

		for(uint32 yy=0; yy<FONT_HEIGHT; yy++)
		{
			for(uint32 xx=0; xx<FONT_WIDTH; xx++)
			{
				uint32 existingColor = *(screen + address + xx + (yy * pitch));

				uint8 eBlue = (existingColor >> 16) & 0xFF,
					eGreen = (existingColor >> 8) & 0xFF,
					eRed = existingColor & 0xFF;

				uint8 trans = font2[fontAddr] ^ xorMask;
				uint8 invTrans = trans ^ 0xFF;

				uint32 bRed = (eRed * invTrans + nRed * trans) / 255,
					bGreen = (eGreen * invTrans + nGreen * trans) / 255,
					bBlue = (eBlue * invTrans + nBlue * trans) / 255;

				*(screen + address + xx + (yy * pitch)) = 0xFF000000 | (bBlue << 16) | (bGreen << 8) | bRed;
				fontAddr++;
			}
		}

		address += FONT_WIDTH;
	}
}

//
// Draw text at the given x/y coordinates, using FG/BG colors.
//
void DrawStringOpaque(uint32 * screen, uint32 x, uint32 y, uint32 color1, uint32 color2, const char * text, ...)
{
	char string[4096];
	va_list arg;

	va_start(arg, text);
	vsprintf(string, text, arg);
	va_end(arg);

	uint32 pitch = sdlemuGetOverlayWidthInPixels();
	uint32 length = strlen(string), address = x + (y * pitch);

	uint8 eBlue = (color2 >> 16) & 0xFF, eGreen = (color2 >> 8) & 0xFF, eRed = color2 & 0xFF,
		nBlue = (color1 >> 16) & 0xFF, nGreen = (color1 >> 8) & 0xFF, nRed = color1 & 0xFF;

	for(uint32 i=0; i<length; i++)
	{
		uint8 c = string[i];
		c = (c < 32 ? 0 : c - 32);
		uint32 fontAddr = (uint32)c * FONT_WIDTH * FONT_HEIGHT;

		for(uint32 yy=0; yy<FONT_HEIGHT; yy++)
		{
			for(uint32 xx=0; xx<FONT_WIDTH; xx++)
			{
				uint8 trans = font2[fontAddr++];
				uint8 invTrans = trans ^ 0xFF;

				uint32 bRed   = (eRed   * invTrans + nRed   * trans) / 255;
				uint32 bGreen = (eGreen * invTrans + nGreen * trans) / 255;
				uint32 bBlue  = (eBlue  * invTrans + nBlue  * trans) / 255;

				*(screen + address + xx + (yy * pitch)) = 0xFF000000 | (bBlue << 16) | (bGreen << 8) | bRed;
			}
		}

		address += FONT_WIDTH;
	}
}

//
// Draw text at the given x/y coordinates with transparency (0 is fully opaque, 32 is fully transparent).
//
void DrawStringTrans(uint32 * screen, uint32 x, uint32 y, uint32 color, uint8 trans, const char * text, ...)
{
	char string[4096];
	va_list arg;

	va_start(arg, text);
	vsprintf(string, text, arg);
	va_end(arg);

	uint32 pitch = sdlemuGetOverlayWidthInPixels();//GetSDLScreenWidthInPixels();
	uint32 length = strlen(string), address = x + (y * pitch);

	for(uint32 i=0; i<length; i++)
	{
		uint32 fontAddr = (uint32)string[i] * 64;

		for(uint32 yy=0; yy<8; yy++)
		{
			for(uint32 xx=0; xx<8; xx++)
			{
				if (font1[fontAddr])
				{
					uint32 existingColor = *(screen + address + xx + (yy * pitch));

					uint8 eBlue = (existingColor >> 16) & 0xFF,
						eGreen = (existingColor >> 8) & 0xFF,
						eRed = existingColor & 0xFF,
//This could be done ahead of time, instead of on each pixel...
						nBlue = (color >> 16) & 0xFF,
						nGreen = (color >> 8) & 0xFF,
						nRed = color & 0xFF;

//This could be sped up by using a table of 5 + 5 + 5 bits (32 levels transparency -> 32768 entries)
//Here we've modified it to have 33 levels of transparency (could have any # we want!)
//because dividing by 32 is faster than dividing by 31...!
					uint8 invTrans = 32 - trans;

					uint32 bRed = (eRed * trans + nRed * invTrans) / 32;
					uint32 bGreen = (eGreen * trans + nGreen * invTrans) / 32;
					uint32 bBlue = (eBlue * trans + nBlue * invTrans) / 32;

					*(screen + address + xx + (yy * pitch)) = 0xFF000000 | (bBlue << 16) | (bGreen << 8) | bRed;
				}

				fontAddr++;
			}
		}

		address += 8;
	}
}

//
// Draw text at the given x/y coordinates, using FG color and overlay alpha blending.
//
void DrawString2(uint32 * screen, uint32 x, uint32 y, uint32 color, uint8 transparency, const char * text, ...)
{
	char string[4096];
	va_list arg;

	va_start(arg, text);
	vsprintf(string, text, arg);
	va_end(arg);

	uint32 pitch = sdlemuGetOverlayWidthInPixels();
	uint32 length = strlen(string), address = x + (y * pitch);

	color &= 0x00FFFFFF;						// Just in case alpha was passed in...

	for(uint32 i=0; i<length; i++)
	{
		uint8 c = string[i];
		c = (c < 32 ? 0 : c - 32);
		uint32 fontAddr = (uint32)c * FONT_WIDTH * FONT_HEIGHT;

		for(uint32 yy=0; yy<FONT_HEIGHT; yy++)
		{
			for(uint32 xx=0; xx<FONT_WIDTH; xx++)
			{
				uint8 fontTrans = font2[fontAddr++];
				uint32 newTrans = (fontTrans * transparency / 255) << 24;
				uint32 pixel = newTrans | color;

				*(screen + address + xx + (yy * pitch)) = pixel;
			}
		}

		address += FONT_WIDTH;
	}
}

//
// Draw "picture"
// Uses zero as transparent color
// Can also use an optional alpha channel
// Alpha channel is now mandatory! ;-)
//
//void DrawTransparentBitmap(int16 * screen, uint32 x, uint32 y, uint16 * bitmap, uint8 * alpha/*=NULL*/)
/*void DrawTransparentBitmap(uint32 * screen, uint32 x, uint32 y, uint32 * bitmap, uint8 * alpha)
{
	uint32 width = bitmap[0], height = bitmap[1];
	bitmap += 2;

//	uint32 pitch = GetSDLScreenPitch() / 2;			// Returns pitch in bytes but we need words...
	uint32 pitch = sdlemuGetOverlayWidthInPixels();//GetSDLScreenWidthInPixels();
	uint32 address = x + (y * pitch);

	for(uint32 yy=0; yy<height; yy++)
	{
		for(uint32 xx=0; xx<width; xx++)
		{
			if (alpha == NULL)
			{
				if (*bitmap && x + xx < pitch)			// NOTE: Still doesn't clip the Y val...
					*(screen + address + xx + (yy * pitch)) = *bitmap;
			}
			else
			{
				uint8 trans = *alpha;
				uint32 color = *bitmap;
				uint32 existingColor = *(screen + address + xx + (yy * pitch));

				uint8 eRed = existingColor & 0xFF,
					eGreen = (existingColor >> 8) & 0xFF,
					eBlue = (existingColor >> 16) & 0xFF,

					nRed = color & 0xFF,
					nGreen = (color >> 8) & 0xFF,
					nBlue = (color >> 16) & 0xFF;

				uint8 invTrans = 255 - trans;
				uint32 bRed = (eRed * trans + nRed * invTrans) / 255;
				uint32 bGreen = (eGreen * trans + nGreen * invTrans) / 255;
				uint32 bBlue = (eBlue * trans + nBlue * invTrans) / 255;

				uint32 blendedColor = 0xFF000000 | bRed | (bGreen << 8) | (bBlue << 16);

				*(screen + address + xx + (yy * pitch)) = blendedColor;

				alpha++;
			}

			bitmap++;
		}
	}
}*/
void DrawTransparentBitmapDeprecated(uint32 * screen, uint32 x, uint32 y, uint32 * bitmap)
{
	uint32 width = bitmap[0], height = bitmap[1];
	bitmap += 2;

	uint32 pitch = sdlemuGetOverlayWidthInPixels();//GetSDLScreenWidthInPixels();
	uint32 address = x + (y * pitch);

	for(uint32 yy=0; yy<height; yy++)
	{
		for(uint32 xx=0; xx<width; xx++)
		{
			uint32 color = *bitmap;
			uint32 blendedColor = color;
			uint32 existingColor = *(screen + address + xx + (yy * pitch));

			if (existingColor >> 24 != 0x00)		// Pixel needs blending
			{
				uint8 trans = color >> 24;
				uint8 invTrans = trans ^ 0xFF;//255 - trans;

				uint8 eRed = existingColor & 0xFF,
					eGreen = (existingColor >> 8) & 0xFF,
					eBlue = (existingColor >> 16) & 0xFF,

					nRed = color & 0xFF,
					nGreen = (color >> 8) & 0xFF,
					nBlue = (color >> 16) & 0xFF;

				uint32 bRed = (eRed * invTrans + nRed * trans) / 255;
				uint32 bGreen = (eGreen * invTrans + nGreen * trans) / 255;
				uint32 bBlue = (eBlue * invTrans + nBlue * trans) / 255;

				blendedColor = 0xFF000000 | bRed | (bGreen << 8) | (bBlue << 16);
			}

			*(screen + address + xx + (yy * pitch)) = blendedColor;
			bitmap++;
		}
	}
}

void DrawTransparentBitmap(uint32 * screen, uint32 x, uint32 y, const void * bitmap)
{
	uint32 pitch = sdlemuGetOverlayWidthInPixels();
	uint32 address = x + (y * pitch);
	uint32 count = 0;

	for(uint32 yy=0; yy<((Bitmap *)bitmap)->height; yy++)
	{
		for(uint32 xx=0; xx<((Bitmap *)bitmap)->width; xx++)
		{
			uint32 color = ((uint32 *)((Bitmap *)bitmap)->pixelData)[count];
			uint32 blendedColor = color;
			uint32 existingColor = *(screen + address + xx + (yy * pitch));

			if (existingColor >> 24 != 0x00)	// Pixel needs blending
			{
				uint8 trans = color >> 24;
				uint8 invTrans = trans ^ 0xFF;

				uint8 eRed = existingColor & 0xFF,
					eGreen = (existingColor >> 8) & 0xFF,
					eBlue = (existingColor >> 16) & 0xFF,

					nRed = color & 0xFF,
					nGreen = (color >> 8) & 0xFF,
					nBlue = (color >> 16) & 0xFF;

				uint32 bRed = (eRed * invTrans + nRed * trans) / 255;
				uint32 bGreen = (eGreen * invTrans + nGreen * trans) / 255;
				uint32 bBlue = (eBlue * invTrans + nBlue * trans) / 255;

// Instead of $FF, should use the alpha from the destination pixel as the final alpha value...
				blendedColor = 0xFF000000 | bRed | (bGreen << 8) | (bBlue << 16);
			}

			*(screen + address + xx + (yy * pitch)) = blendedColor;
			count++;
		}
	}
}

//
// Draw a bitmap without using blending
//
void DrawBitmap(uint32 * screen, uint32 x, uint32 y, const void * bitmap)
{
	uint32 pitch = sdlemuGetOverlayWidthInPixels();
	uint32 address = x + (y * pitch);
	uint32 count = 0;

	for(uint32 yy=0; yy<((Bitmap *)bitmap)->height; yy++)
	{
		for(uint32 xx=0; xx<((Bitmap *)bitmap)->width; xx++)
		{
			*(screen + address + xx + (yy * pitch)) = ((uint32 *)((Bitmap *)bitmap)->pixelData)[count];
			count++;
		}
	}
}

//
// Fill a portion of the screen with the passed in color
//
void FillScreenRectangle(uint32 * screen, uint32 x, uint32 y, uint32 w, uint32 h, uint32 color)
//void ClearScreenRectangle(uint32 * screen, uint32 x, uint32 y, uint32 w, uint32 h)
{
	uint32 pitch = sdlemuGetOverlayWidthInPixels();
	uint32 address = x + (y * pitch);

	for(uint32 yy=0; yy<h; yy++)
		for(uint32 xx=0; xx<w; xx++)
			*(screen + address + xx + (yy * pitch)) = color;
}


//
// GUI stuff--it's not crunchy, it's GUI! ;-)
//

void InitGUI(void)
{
	SDL_ShowCursor(SDL_DISABLE);
	SDL_GetMouseState(&mouseX, &mouseY);
}

void GUIDone(void)
{
}

//
// GUI main loop
//
//bool GUIMain(void)
bool GUIMain(char * filename)
{
WriteLog("GUI: Inside GUIMain...\n");

	uint32 pointerBGSave[6 * 8 + 2];
	pointerBGSave[0] = 6;
	pointerBGSave[1] = 8;

// Need to set things up so that it loads and runs a file if given on the command line. !!! FIX !!! [DONE]
	extern uint32 * backbuffer;
//	bool done = false;
	SDL_Event event;
	Window * mainWindow = NULL;

	// Set up the GUI classes...
//	Element::SetScreenAndPitch(backbuffer, GetSDLScreenWidthInPixels());
	Element::SetScreenAndPitch((uint32 *)sdlemuGetOverlayPixels(), sdlemuGetOverlayWidthInPixels());
	sdlemuEnableOverlay();

	Menu mainMenu;
	MenuItems mi;
	mi.title = "Jaguar";
	mi.item.push_back(NameAction("Load...", LoadROM, SDLK_l));
	mi.item.push_back(NameAction("Reset", ResetJaguar, SDLK_r));
	if (CDBIOSLoaded)
		mi.item.push_back(NameAction("Reset CD", ResetJaguarCD, SDLK_c));
	mi.item.push_back(NameAction("Run", RunEmu, SDLK_ESCAPE));
	mi.item.push_back(NameAction(""));
	mi.item.push_back(NameAction("Quit", Quit, SDLK_q));
	mainMenu.Add(mi);
	mi.title = "Settings";
	mi.item.clear();
	mi.item.push_back(NameAction("Video..."));
	mi.item.push_back(NameAction("Audio..."));
	mi.item.push_back(NameAction("Misc...", MiscOptions, SDLK_m));
	mainMenu.Add(mi);
	mi.title = "Info";
	mi.item.clear();
	mi.item.push_back(NameAction("About...", About));
	mainMenu.Add(mi);

	bool showMouse = true;

	// Grab the BG where the mouse will be painted (prime the backstore)

/*
DISNOWOK
Bitmap ptr = { 6, 8, 4,
""//"000011112222333344445555"
//"000011112222333344445555"
//"000011112222333344445555"
//"000011112222333344445555"
//"000011112222333344445555"
//"000011112222333344445555"
//"000011112222333344445555"
//"000011112222333344445555"
};//*/
	uint32 * overlayPixels = (uint32 *)sdlemuGetOverlayPixels();
	uint32 count = 2;

	for(uint32 y=0; y<pointerBGSave[1]; y++)
		for(uint32 x=0; x<pointerBGSave[0]; x++)
			pointerBGSave[count++] = overlayPixels[((mouseY + y) * sdlemuGetOverlayWidthInPixels()) + (mouseX + x)];

	uint32 oldMouseX = mouseX, oldMouseY = mouseY;

//This is crappy!!! !!! FIX !!!
//Is this even needed any more? Hmm. Maybe. Dunno.
WriteLog("GUI: Resetting Jaguar...\n");
	jaguar_reset();

WriteLog("GUI: Clearing BG save...\n");
	// Set up our background save...
//	memset(background, 0x11, tom_getVideoModeWidth() * 240 * 2);
//1111 -> 000100 01000 10001 -> 0001 0000 0100 0010 1000 1100 -> 10 42 8C
	for(uint32 i=0; i<tom_getVideoModeWidth()*240; i++)
//		background[i] = 0xFF8C4210;
		backbuffer[i] = 0xFF8C4210;

/*	uint32 * overlayPix = (uint32 *)sdlemuGetOverlayPixels();
	for(uint32 i=0; i<sdlemuGetOverlayWidthInPixels()*480; i++)
		overlayPix[i] = 0x00000000;*/

	// Handle loading file passed in on the command line...! [DONE]

	if (filename)
	{
		if (JaguarLoadFile(filename))
		{
//			event.type = SDL_USEREVENT, event.user.code = MENU_ITEM_CHOSEN;
//			event.user.data1 = (void *)ResetJaguar;
//	    	SDL_PushEvent(&event);
			// Make it so that if passed in on the command line, we quit right
			// away when pressing ESC
WriteLog("GUI: Bypassing GUI since ROM passed in on command line...\n");
			ResetJaguar();
			return true;
		}
		else
		{
			// Create error dialog...
			char errText[1024];
			sprintf(errText, "The file %40s could not be loaded.", filename);

			mainWindow = new Window(8, 16, 304, 160);
			mainWindow->AddElement(new Text(8, 8, "Error!"));
			mainWindow->AddElement(new Text(8, 24, errText));
		}
	}

WriteLog("GUI: Entering main loop...\n");
	while (!exitGUI)
	{
		if (SDL_PollEvent(&event))
		{
			if (event.type == SDL_USEREVENT)
			{
				if (event.user.code == WINDOW_CLOSE)
				{
					delete mainWindow;
					mainWindow = NULL;
				}
				else if (event.user.code == MENU_ITEM_CHOSEN)
				{
					// Confused? Let me enlighten... What we're doing here is casting
					// data1 as a pointer to a function which returns a Window pointer and
					// which takes no parameters (the "(Window *(*)(void))" part), then
					// derefencing it (the "*" in front of that) in order to call the
					// function that it points to. Clear as mud? Yeah, I hate function
					// pointers too, but what else are you gonna do?
					mainWindow = (*(Window *(*)(void))event.user.data1)();

					while (SDL_PollEvent(&event));	// Flush the event queue...
					event.type = SDL_MOUSEMOTION;
					int mx, my;
					SDL_GetMouseState(&mx, &my);
					event.motion.x = mx, event.motion.y = my;
				    SDL_PushEvent(&event);			// & update mouse position...!

					oldMouseX = mouseX, oldMouseY = mouseY;
					mouseX = mx, mouseY = my;		// This prevents "mouse flash"...
				}
			}
			else if (event.type == SDL_ACTIVEEVENT)
			{
				if (event.active.state == SDL_APPMOUSEFOCUS)
					showMouse = (event.active.gain ? true : false);
			}
			else if (event.type == SDL_KEYDOWN)
			{
				if (mainWindow)
					mainWindow->HandleKey(event.key.keysym.sym);
				else
					mainMenu.HandleKey(event.key.keysym.sym);
			}
			else if (event.type == SDL_MOUSEMOTION)
			{
				oldMouseX = mouseX, oldMouseY = mouseY;
				mouseX = event.motion.x, mouseY = event.motion.y;

				if (mainWindow)
					mainWindow->HandleMouseMove(mouseX, mouseY);
				else
					mainMenu.HandleMouseMove(mouseX, mouseY);
			}
			else if (event.type == SDL_MOUSEBUTTONDOWN)
			{
				uint32 mx = event.button.x, my = event.button.y;

				if (mainWindow)
					mainWindow->HandleMouseButton(mx, my, true);
				else
					mainMenu.HandleMouseButton(mx, my, true);
			}
			else if (event.type == SDL_MOUSEBUTTONUP)
			{
				uint32 mx = event.button.x, my = event.button.y;

				if (mainWindow)
					mainWindow->HandleMouseButton(mx, my, false);
				else
					mainMenu.HandleMouseButton(mx, my, false);
			}

//PROBLEM: In order to use the dirty rectangle approach here, we need some way of
//         handling it in mainMenu.Draw() and mainWindow->Draw(). !!! FIX !!!
//POSSIBLE SOLUTION:
// When mouse is moving and not on menu or window, can do straight dirty rect.
// When mouse is on menu, need to update screen. Same for buttons on windows...
// What the menu & windows should do is only redraw on a state change. IOW, they
// should call their own/child window's Draw() function instead of doing it top
// level.
//#define NEW_BACKSTORE_METHOD

			// Draw the GUI...
// The way we do things here is kinda stupid (redrawing the screen every frame), but
// it's simple. Perhaps there may be a reason down the road to be more selective with
// our clearing, but for now, this will suffice.
//			memset(backbuffer, 0x11, tom_getVideoModeWidth() * 240 * 2);
//			memcpy(backbuffer, background, tom_getVideoModeWidth() * 256 * 2);
//			memcpy(backbuffer, background, tom_getVideoModeWidth() * 256 * 4);
#ifndef NEW_BACKSTORE_METHOD
			memset(sdlemuGetOverlayPixels(), 0, sdlemuGetOverlayWidthInPixels() * 480 * 4);

			mainMenu.Draw();
//Could do multiple windows here by using a vector + priority info...
//Though the way ZSNES does it seems to be by a bool (i.e., they're always active, just not shown)
			if (mainWindow)
				mainWindow->Draw();
#endif

/*uint32 pBGS[6 * 8 + 3] = { 6, 8, 4,
	0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0
};*/
//This isn't working... Why????
//It's because DrawTransparentBitmap does alpha blending if it detects zero in the alpha channel.
//So why do it that way? Hm.
			overlayPixels = (uint32 *)sdlemuGetOverlayPixels();

#ifdef NEW_BACKSTORE_METHOD
//			DrawTransparentBitmapDeprecated(overlayPixels, oldMouseX, oldMouseY, pointerBGSave);
//			DrawTransparentBitmap(overlayPixels, oldMouseX, oldMouseY, pBGS);
			for(uint32 y=0; y<pointerBGSave[1]; y++)
				for(uint32 x=0; x<pointerBGSave[0]; x++)
					overlayPixels[((oldMouseY + y) * sdlemuGetOverlayWidthInPixels()) + (oldMouseX + x)] = 0x00000000;

			count = 2;

			for(uint32 y=0; y<pointerBGSave[1]; y++)
				for(uint32 x=0; x<pointerBGSave[0]; x++)
					pointerBGSave[count++] = overlayPixels[((mouseY + y) * sdlemuGetOverlayWidthInPixels()) + (mouseX + x)];
#endif

			if (showMouse)
//				DrawTransparentBitmapDeprecated(backbuffer, mouseX, mouseY, mousePic);
				DrawTransparentBitmapDeprecated(overlayPixels, mouseX, mouseY, mousePic);

			RenderBackbuffer();
		}
	}

	return true;
}

//
// GUI "action" functions
//

Window * LoadROM(void)
{
	FileList * fileList = new FileList(20, 20, 600, 440);

	return (Window *)fileList;
}

Window * ResetJaguar(void)
{
	jaguar_reset();

	return RunEmu();
}

Window * ResetJaguarCD(void)
{
	memcpy(jaguar_mainRom, jaguar_CDBootROM, 0x40000);
	jaguarRunAddress = 0x802000;
	jaguar_mainRom_crc32 = crc32_calcCheckSum(jaguar_mainRom, 0x40000);
	jaguar_reset();
//This is a quick kludge to get the CDBIOS to boot properly...
//Wild speculation: It could be that this memory location is wired into the CD unit
//somehow, which lets it know whether or not a cart is present in the unit...
	jaguar_mainRom[0x0040B] = 0x03;

	return RunEmu();
}


#if 0

bool debounceRunKey = true;
Window * RunEmu(void)
{
	extern uint32 * backbuffer;
//Temporary, to test the new timer based code...
sdlemuDisableOverlay();
JaguarExecuteNew();
sdlemuEnableOverlay();
	// Save the background for the GUI...
	// In this case, we squash the color to monochrome, then force it to blue + green...
	for(uint32 i=0; i<tom_getVideoModeWidth() * 256; i++)
	{
		uint32 pixel = backbuffer[i];
		uint8 b = (pixel >> 16) & 0xFF, g = (pixel >> 8) & 0xFF, r = pixel & 0xFF;
		pixel = ((r + g + b) / 3) & 0x00FF;
		backbuffer[i] = 0xFF000000 | (pixel << 16) | (pixel << 8);
	}
return NULL;//*/

//This is crappy... !!! FIX !!!
	extern bool finished, showGUI;

//	uint32 nFrame = 0, nFrameskip = 0;
	uint32 totalFrames = 0;
	finished = false;
	bool showMessage = true;
	uint32 showMsgFrames = 120;
	uint8 transparency = 0;
	// Pass a message to the "joystick" code to debounce the ESC key...
	debounceRunKey = true;

	uint32 cartType = 4;
	if (jaguarRomSize == 0x200000)
		cartType = 0;
	else if (jaguarRomSize == 0x400000)
		cartType = 1;
	else if (jaguar_mainRom_crc32 == 0x687068D5)
		cartType = 2;
	else if (jaguar_mainRom_crc32 == 0x55A0669C)
		cartType = 3;

	char * cartTypeName[5] = { "2M Cartridge", "4M Cartridge", "CD BIOS", "CD Dev BIOS", "Homebrew" };
	uint32 elapsedTicks = SDL_GetTicks(), frameCount = 0, framesPerSecond = 0;

	while (true)
	{
		// Set up new backbuffer with new pixels and data
		JaguarExecute(backbuffer, true);
//		JaguarExecuteNew();
		totalFrames++;
//WriteLog("Frame #%u...\n", totalFrames);
//extern bool doDSPDis;
//if (totalFrames == 373)
//	doDSPDis = true;

//This sucks... !!! FIX !!!
		joystick_exec();
//This is done here so that the crud below doesn't get on our GUI background...
		if (finished)
			break;

		// Some QnD GUI stuff here...
		if (showGUI)
		{
			extern uint32 gpu_pc, dsp_pc;
			DrawString(backbuffer, 8, 8, false, "GPU PC: %08X", gpu_pc);
			DrawString(backbuffer, 8, 16, false, "DSP PC: %08X", dsp_pc);
			DrawString(backbuffer, 8, 32, false, "%u FPS", framesPerSecond);
		}

		if (showMessage)
		{
// FF0F -> 1111 11 11  000 0 1111 -> 3F 18 0F
// 3FE3 -> 0011 11 11  111 0 0011 -> 0F 3F 03
/*			DrawStringTrans((uint32 *)backbuffer, 8, 24*8, 0xFF0F, transparency, "Running...");
			DrawStringTrans((uint32 *)backbuffer, 8, 26*8, 0x3FE3, transparency, "%s, run address: %06X", cartTypeName[cartType], jaguarRunAddress);
			DrawStringTrans((uint32 *)backbuffer, 8, 27*8, 0x3FE3, transparency, "CRC: %08X", jaguar_mainRom_crc32);//*/
//first has wrong color. !!! FIX !!!
			DrawStringTrans(backbuffer, 8, 24*8, 0xFF7F63FF, transparency, "Running...");
			DrawStringTrans(backbuffer, 8, 26*8, 0xFF1FFF3F, transparency, "%s, run address: %06X", cartTypeName[cartType], jaguarRunAddress);
			DrawStringTrans(backbuffer, 8, 27*8, 0xFF1FFF3F, transparency, "CRC: %08X", jaguar_mainRom_crc32);

			if (showMsgFrames == 0)
			{
				transparency++;

				if (transparency == 33)
{
					showMessage = false;
/*extern bool doGPUDis;
doGPUDis = true;//*/
}

			}
			else
				showMsgFrames--;
		}

		RenderBackbuffer();
		frameCount++;

		if (SDL_GetTicks() - elapsedTicks > 250)
			elapsedTicks += 250, framesPerSecond = frameCount * 4, frameCount = 0;
	}

	// Reset the pitch, since it may have been changed in-game...
	Element::SetScreenAndPitch((uint32 *)backbuffer, GetSDLScreenWidthInPixels());

	// Save the background for the GUI...
//	memcpy(background, backbuffer, tom_getVideoModeWidth() * 240 * 2);
	// In this case, we squash the color to monochrome, then force it to blue + green...
	for(uint32 i=0; i<tom_getVideoModeWidth() * 256; i++)
	{
		uint32 pixel = backbuffer[i];
		uint8 b = (pixel >> 16) & 0xFF, g = (pixel >> 8) & 0xFF, r = pixel & 0xFF;
		pixel = ((r + g + b) / 3) & 0x00FF;
		background[i] = 0xFF000000 | (pixel << 16) | (pixel << 8);
	}

	return NULL;
}

#else

bool debounceRunKey = true;
Window * RunEmu(void)
{
	extern uint32 * backbuffer;
	uint32 * overlayPixels = (uint32 *)sdlemuGetOverlayPixels();
	memset(overlayPixels, 0x00, 640 * 480 * 4);			// Clear out overlay...

//This is crappy... !!! FIX !!!
	extern bool finished, showGUI;

	sdlemuDisableOverlay();

//	uint32 nFrame = 0, nFrameskip = 0;
	uint32 totalFrames = 0;
	finished = false;
	bool showMessage = true;
	uint32 showMsgFrames = 120;
	uint8 transparency = 0xFF;
	// Pass a message to the "joystick" code to debounce the ESC key...
	debounceRunKey = true;

	uint32 cartType = 4;
	if (jaguarRomSize == 0x200000)
		cartType = 0;
	else if (jaguarRomSize == 0x400000)
		cartType = 1;
	else if (jaguar_mainRom_crc32 == 0x687068D5)
		cartType = 2;
	else if (jaguar_mainRom_crc32 == 0x55A0669C)
		cartType = 3;

	char * cartTypeName[5] = { "2M Cartridge", "4M Cartridge", "CD BIOS", "CD Dev BIOS", "Homebrew" };
	uint32 elapsedTicks = SDL_GetTicks(), frameCount = 0, framesPerSecond = 0;

	while (!finished)
	{
		// Set up new backbuffer with new pixels and data
		JaguarExecuteNew();
		totalFrames++;
//WriteLog("Frame #%u...\n", totalFrames);
//extern bool doDSPDis;
//if (totalFrames == 373)
//	doDSPDis = true;

//Problem: Need to do this *only* when the state changes from visible to not...
//Also, need to clear out the GUI when not on (when showMessage is active...)
if (showGUI || showMessage)
	sdlemuEnableOverlay();
else
	sdlemuDisableOverlay();

//Add in a new function for clearing patches of screen (ClearOverlayRect)

		// Some QnD GUI stuff here...
		if (showGUI)
		{
			FillScreenRectangle(overlayPixels, 8, 1*FONT_HEIGHT, 128, 4*FONT_HEIGHT, 0x00000000);
			extern uint32 gpu_pc, dsp_pc;
			DrawString(overlayPixels, 8, 1*FONT_HEIGHT, false, "GPU PC: %08X", gpu_pc);
			DrawString(overlayPixels, 8, 2*FONT_HEIGHT, false, "DSP PC: %08X", dsp_pc);
			DrawString(overlayPixels, 8, 4*FONT_HEIGHT, false, "%u FPS", framesPerSecond);
		}

		if (showMessage)
		{
			DrawString2(overlayPixels, 8, 24*FONT_HEIGHT, 0x007F63FF, transparency, "Running...");
			DrawString2(overlayPixels, 8, 26*FONT_HEIGHT, 0x001FFF3F, transparency, "%s, run address: %06X", cartTypeName[cartType], jaguarRunAddress);
			DrawString2(overlayPixels, 8, 27*FONT_HEIGHT, 0x001FFF3F, transparency, "CRC: %08X", jaguar_mainRom_crc32);

			if (showMsgFrames == 0)
			{
				transparency--;

				if (transparency == 0)
{
					showMessage = false;
/*extern bool doGPUDis;
doGPUDis = true;//*/
}

			}
			else
				showMsgFrames--;
		}

		frameCount++;

		if (SDL_GetTicks() - elapsedTicks > 250)
			elapsedTicks += 250, framesPerSecond = frameCount * 4, frameCount = 0;
	}

	// Save the background for the GUI...
	// In this case, we squash the color to monochrome, then force it to blue + green...
	for(uint32 i=0; i<tom_getVideoModeWidth() * 256; i++)
	{
		uint32 pixel = backbuffer[i];
		uint8 b = (pixel >> 16) & 0xFF, g = (pixel >> 8) & 0xFF, r = pixel & 0xFF;
		pixel = ((r + g + b) / 3) & 0x00FF;
		backbuffer[i] = 0xFF000000 | (pixel << 16) | (pixel << 8);
	}

	sdlemuEnableOverlay();

	return NULL;
}

#endif


Window * Quit(void)
{
	WriteLog("GUI: Quitting due to user request.\n");
	exitGUI = true;

	return NULL;
}

Window * About(void)
{
	char buf[512];
//	sprintf(buf, "Virtual Jaguar CVS %s", __DATE__);
	sprintf(buf, "CVS %s", __DATE__);
//fprintf(fp, "VirtualJaguar v1.0.8 (Last full build was on %s %s)\n", __DATE__, __TIME__);
//VirtualJaguar v1.0.8 (Last full build was on Dec 30 2004 20:01:31)
//Hardwired, bleh... !!! FIX !!!
uint32 width = 55 * FONT_WIDTH, height = 18 * FONT_HEIGHT;
uint32 xpos = (640 - width) / 2, ypos = (480 - height) / 2;
//	Window * window = new Window(8, 16, 50 * FONT_WIDTH, 21 * FONT_HEIGHT);
	Window * window = new Window(xpos, ypos, width, height);
//	window->AddElement(new Text(8, 8, "Virtual Jaguar 1.0.8"));
//	window->AddElement(new Text(8, 8, "Virtual Jaguar CVS 20050110", 0xFF3030FF, 0xFF000000));
//	window->AddElement(new Text(208, 8+0*FONT_HEIGHT, buf, 0xFF3030FF, 0xFF000000));
	window->AddElement(new Text(248, 8+4*FONT_HEIGHT+5, buf, 0xFF3030FF, 0xFF000000));
	window->AddElement(new Text(8, 8+0*FONT_HEIGHT, "Coders:"));
	window->AddElement(new Text(16, 8+1*FONT_HEIGHT, "James L. Hammons (shamus)"));
	window->AddElement(new Text(16, 8+2*FONT_HEIGHT, "Niels Wagenaar (nwagenaar)"));
	window->AddElement(new Text(16, 8+3*FONT_HEIGHT, "Carwin Jones (Caz)"));
	window->AddElement(new Text(16, 8+4*FONT_HEIGHT, "Adam Green"));
	window->AddElement(new Text(8, 8+6*FONT_HEIGHT, "Testers:"));
	window->AddElement(new Text(16, 8+7*FONT_HEIGHT, "Guruma"));
	window->AddElement(new Text(8, 8+9*FONT_HEIGHT, "Thanks go out to:"));
	window->AddElement(new Text(16, 8+10*FONT_HEIGHT, "Aaron Giles for the original CoJag"));
	window->AddElement(new Text(16, 8+11*FONT_HEIGHT, "David Raingeard for the original VJ"));
	window->AddElement(new Text(16, 8+12*FONT_HEIGHT, "Karl Stenerud for his Musashi 68K emu"));
	window->AddElement(new Text(16, 8+13*FONT_HEIGHT, "Sam Lantinga for his amazing SDL libs"));
	window->AddElement(new Text(16, 8+14*FONT_HEIGHT, "Ryan C. Gordon for VJ's web presence"));
	window->AddElement(new Text(16, 8+15*FONT_HEIGHT, "Curt Vendel for various Jaguar goodies"));
	window->AddElement(new Text(16, 8+16*FONT_HEIGHT, "The guys over at Atari Age ;-)"));
//	window->AddElement(new Image(8, 8, &vj_title_small));
	window->AddElement(new Image(width - (vj_title_small.width + 8), 8, &vj_title_small));

	return window;
}

Window * MiscOptions(void)
{
	Window * window = new Window(8, 16, 304, 192);
	window->AddElement(new PushButton(8, 8, &vjs.useJaguarBIOS, "BIOS"));
	window->AddElement(new SlideSwitch(8, 32, &vjs.hardwareTypeNTSC, "PAL", "NTSC"));
	window->AddElement(new PushButton(8, 64, &vjs.DSPEnabled, "DSP"));
	window->AddElement(new SlideSwitch(24, 88, &vjs.usePipelinedDSP, "Original", "Pipelined"));
	window->AddElement(new SlideSwitch(8, 120, (bool *)&vjs.glFilter, "Sharp", "Blurry"));
	window->AddElement(new SlideSwitch(8, 152, (bool *)&vjs.renderType, "Normal render", "TV style"));

	window->AddElement(new TextEdit(88, 8, vjs.ROMPath, 20, 0xFF8484FF, 0xFF000000));

/*TextEdit(uint32 x, uint32 y, string s, uint32 mss = 10, uint32 fg = 0xFF8484FF,
	uint32 bg = 0xFF84FF4D): Element(x, y, 0, 0), fgColor(fg), bgColor(bg), text(s),
	caretPos(0), maxScreenSize(mss) {}*/
// Missing:
// * BIOS path
// * ROM path
// * EEPROM path
// * joystick
// * joystick port
// * OpenGL?
// * GL Filter type
// * Window/fullscreen
// * Key definitions

	return window;
}


//
// Generic ROM loading
//
uint32 JaguarLoadROM(uint8 * rom, char * path)
{
// We really should have some kind of sanity checking for the ROM size here to prevent
// a buffer overflow... !!! FIX !!!
	uint32 romSize = 0;

WriteLog("JaguarLoadROM: Attempting to load file '%s'...", path);
	char * ext = strrchr(path, '.');
if (ext == NULL)
	WriteLog("FAILED!\n");
else
	WriteLog("Succeeded in finding extension (%s)!\n", ext);

	if (ext != NULL)
	{
		WriteLog("VJ: Loading \"%s\"...", path);

		if (strcasecmp(ext, ".zip") == 0)
		{
			// Handle ZIP file loading here...
			WriteLog("(ZIPped)...");

			if (load_zipped_file(0, 0, path, NULL, &rom, &romSize) == -1)
			{
				WriteLog("Failed!\n");
				return 0;
			}
		}
		else
		{
/*			FILE * fp = fopen(path, "rb");

			if (fp == NULL)
			{
				WriteLog("Failed!\n");
				return 0;
			}

			fseek(fp, 0, SEEK_END);
			romSize = ftell(fp);
			fseek(fp, 0, SEEK_SET);
			fread(rom, 1, romSize, fp);
			fclose(fp);*/

			// Handle gzipped files transparently [Adam Green]...

			gzFile fp = gzopen(path, "rb");

			if (fp == NULL)
			{
				WriteLog("Failed!\n");
				return 0;
			}

			romSize = gzfilelength(fp);
			gzseek(fp, 0, SEEK_SET);
			gzread(fp, rom, romSize);
			gzclose(fp);
		}

		WriteLog("OK (%i bytes)\n", romSize);
	}

	return romSize;
}

//
// Jaguar file loading
//
bool JaguarLoadFile(char * path)
{
//	jaguarRomSize = JaguarLoadROM(mem, path);
	jaguarRomSize = JaguarLoadROM(jaguar_mainRom, path);

/*//This is not *nix friendly for some reason...
//		if (!UserSelectFile(path, newPath))
	if (!UserSelectFile((strlen(path) == 0 ? (char *)"." : path), newPath))
	{
		WriteLog("VJ: Could not find valid ROM in directory \"%s\"...\nAborting!\n", path);
		log_done();
		exit(0);
	}*/

	if (jaguarRomSize == 0)
	{
//			WriteLog("VJ: Could not load ROM from file \"%s\"...\nAborting!\n", newPath);
		WriteLog("GUI: Could not load ROM from file \"%s\"...\nAborting load!\n", path);
// Need to do something else here, like throw up an error dialog instead of aborting. !!! FIX !!!
//		log_done();
//		exit(0);
		return false;								// This is a start...
	}

	jaguar_mainRom_crc32 = crc32_calcCheckSum(jaguar_mainRom, jaguarRomSize);
	WriteLog("CRC: %08X\n", (unsigned int)jaguar_mainRom_crc32);
	eeprom_init();

	jaguarRunAddress = 0x802000;

	char * ext = strrchr(path, '.');				// Get the file's extension for non-cartridge checking

//NOTE: Should fix JaguarLoadROM() to replace .zip with what's *in* the zip (.abs, .j64, etc.)
	if (strcasecmp(ext, ".rom") == 0)
	{
		// File extension ".ROM": Alpine image that loads/runs at $802000
		WriteLog("GUI: Setting up homebrew (ROM)... Run address: 00802000, length: %08X\n", jaguarRomSize);

		for(int i=jaguarRomSize-1; i>=0; i--)
			jaguar_mainRom[0x2000 + i] = jaguar_mainRom[i];

		memset(jaguar_mainRom, 0xFF, 0x2000);
/*		memcpy(jaguar_mainRam, jaguar_mainRom, jaguarRomSize);
		memset(jaguar_mainRom, 0xFF, 0x600000);
		memcpy(jaguar_mainRom + 0x2000, jaguar_mainRam, jaguarRomSize);
		memset(jaguar_mainRam, 0x00, 0x400000);*/

/*
Stubulator ROM vectors...
handler 001 at $00E00008
handler 002 at $00E008DE
handler 003 at $00E008E2
handler 004 at $00E008E6
handler 005 at $00E008EA
handler 006 at $00E008EE
handler 007 at $00E008F2
handler 008 at $00E0054A
handler 009 at $00E008FA
handler 010 at $00000000
handler 011 at $00000000
handler 012 at $00E008FE
handler 013 at $00E00902
handler 014 at $00E00906
handler 015 at $00E0090A
handler 016 at $00E0090E
handler 017 at $00E00912
handler 018 at $00E00916
handler 019 at $00E0091A
handler 020 at $00E0091E
handler 021 at $00E00922
handler 022 at $00E00926
handler 023 at $00E0092A
handler 024 at $00E0092E
handler 025 at $00E0107A
handler 026 at $00E0107A
handler 027 at $00E0107A
handler 028 at $00E008DA
handler 029 at $00E0107A
handler 030 at $00E0107A
handler 031 at $00E0107A
handler 032 at $00000000

Let's try setting up the illegal instruction vector for a stubulated jaguar...
*/
/*		SET32(jaguar_mainRam, 0x08, 0x00E008DE);
		SET32(jaguar_mainRam, 0x0C, 0x00E008E2);
		SET32(jaguar_mainRam, 0x10, 0x00E008E6);	// <-- Should be here (it is)...
		SET32(jaguar_mainRam, 0x14, 0x00E008EA);//*/

		// Try setting the vector to say, $1000 and putting an instruction there that loops forever:
		// This kludge works! Yeah!
		SET32(jaguar_mainRam, 0x10, 0x00001000);
		SET16(jaguar_mainRam, 0x1000, 0x60FE);		// Here: bra Here
	}
	else if (strcasecmp(ext, ".abs") == 0)
	{
		// File extension ".ABS": Atari linker output file with header (w/o is useless to us here)

/*
ABS Format sleuthing (LBUGDEMO.ABS):

000000  60 1B 00 00 05 0C 00 04 62 C0 00 00 04 28 00 00
000010  12 A6 00 00 00 00 00 80 20 00 FF FF 00 80 25 0C
000020  00 00 40 00

DRI-format file detected...
Text segment size = 0x0000050c bytes
Data segment size = 0x000462c0 bytes
BSS Segment size = 0x00000428 bytes
Symbol Table size = 0x000012a6 bytes
Absolute Address for text segment = 0x00802000
Absolute Address for data segment = 0x0080250c
Absolute Address for BSS segment = 0x00004000

(CRZDEMO.ABS):
000000  01 50 00 03 00 00 00 00 00 03 83 10 00 00 05 3b
000010  00 1c 00 03 00 00 01 07 00 00 1d d0 00 03 64 98
000020  00 06 8b 80 00 80 20 00 00 80 20 00 00 80 3d d0

000030  2e 74 78 74 00 00 00 00 00 80 20 00 00 80 20 00 .txt (+36 bytes)
000040  00 00 1d d0 00 00 00 a8 00 00 00 00 00 00 00 00
000050  00 00 00 00 00 00 00 20
000058  2e 64 74 61 00 00 00 00 00 80 3d d0 00 80 3d d0 .dta (+36 bytes)
000068  00 03 64 98 00 00 1e 78 00 00 00 00 00 00 00 00
000078  00 00 00 00 00 00 00 40
000080  2e 62 73 73 00 00 00 00 00 00 50 00 00 00 50 00 .bss (+36 bytes)
000090  00 06 8b 80 00 03 83 10 00 00 00 00 00 00 00 00
0000a0  00 00 00 00 00 00 00 80

Header size is $A8 bytes...

BSD/COFF format file detected...
3 sections specified
Symbol Table offset = 230160				($00038310)
Symbol Table contains 1339 symbol entries	($0000053B)
The additional header size is 28 bytes		($001C)
Magic Number for RUN_HDR = 0x00000107
Text Segment Size = 7632					($00001DD0)
Data Segment Size = 222360					($00036498)
BSS Segment Size = 428928					($00068B80)
Starting Address for executable = 0x00802000
Start of Text Segment = 0x00802000
Start of Data Segment = 0x00803dd0
*/
		if (jaguar_mainRom[0] == 0x60 && jaguar_mainRom[1] == 0x1B)
		{
			uint32 loadAddress = GET32(jaguar_mainRom, 0x16), //runAddress = GET32(jaguar_mainRom, 0x2A),
				codeSize = GET32(jaguar_mainRom, 0x02) + GET32(jaguar_mainRom, 0x06);
			WriteLog("GUI: Setting up homebrew (ABS-1)... Run address: %08X, length: %08X\n", loadAddress, codeSize);

			if (loadAddress < 0x800000)
				memcpy(jaguar_mainRam + loadAddress, jaguar_mainRom + 0x24, codeSize);
			else
			{
				for(int i=codeSize-1; i>=0; i--)
					jaguar_mainRom[(loadAddress - 0x800000) + i] = jaguar_mainRom[i + 0x24];
/*				memcpy(jaguar_mainRam, jaguar_mainRom + 0x24, codeSize);
				memset(jaguar_mainRom, 0xFF, 0x600000);
				memcpy(jaguar_mainRom + (loadAddress - 0x800000), jaguar_mainRam, codeSize);
				memset(jaguar_mainRam, 0x00, 0x400000);*/
			}

			jaguarRunAddress = loadAddress;
		}
		else if (jaguar_mainRom[0] == 0x01 && jaguar_mainRom[1] == 0x50)
		{
			uint32 loadAddress = GET32(jaguar_mainRom, 0x28), runAddress = GET32(jaguar_mainRom, 0x24),
				codeSize = GET32(jaguar_mainRom, 0x18) + GET32(jaguar_mainRom, 0x1C);
			WriteLog("GUI: Setting up homebrew (ABS-2)... Run address: %08X, length: %08X\n", runAddress, codeSize);

			if (loadAddress < 0x800000)
				memcpy(jaguar_mainRam + loadAddress, jaguar_mainRom + 0xA8, codeSize);
			else
			{
				for(int i=codeSize-1; i>=0; i--)
					jaguar_mainRom[(loadAddress - 0x800000) + i] = jaguar_mainRom[i + 0xA8];
/*				memcpy(jaguar_mainRam, jaguar_mainRom + 0xA8, codeSize);
				memset(jaguar_mainRom, 0xFF, 0x600000);
				memcpy(jaguar_mainRom + (loadAddress - 0x800000), jaguar_mainRam, codeSize);
				memset(jaguar_mainRam, 0x00, 0x400000);*/
			}

			jaguarRunAddress = runAddress;
		}
		else
		{
			WriteLog("GUI: Couldn't find correct ABS format: %02X %02X\n", jaguar_mainRom[0], jaguar_mainRom[1]);
			return false;
		}
	}
	else if (strcasecmp(ext, ".jag") == 0)
	{
		// File extension ".JAG": Atari server file with header
//NOTE: The bytes 'JAGR' should also be at position $1C...
//      Also, there's *always* a $601A header at position $00...
		if (jaguar_mainRom[0] == 0x60 && jaguar_mainRom[1] == 0x1A)
		{
			uint32 loadAddress = GET32(jaguar_mainRom, 0x22), runAddress = GET32(jaguar_mainRom, 0x2A);
//This is not always right! Especially when converted via bin2jag1!!!
//We should have access to the length of the furshlumiger file that was loaded anyway!
//Now, we do! ;-)
//			uint32 progLength = GET32(jaguar_mainRom, 0x02);
//jaguarRomSize
//jaguarRunAddress
//			WriteLog("Jaguar: Setting up PD ROM... Run address: %08X, length: %08X\n", runAddress, progLength);
//			memcpy(jaguar_mainRam + loadAddress, jaguar_mainRom + 0x2E, progLength);
			WriteLog("GUI: Setting up homebrew (JAG)... Run address: %08X, length: %08X\n", runAddress, jaguarRomSize - 0x2E);
			memcpy(jaguar_mainRam + loadAddress, jaguar_mainRom + 0x2E, jaguarRomSize - 0x2E);
//		SET32(jaguar_mainRam, 4, runAddress);
			jaguarRunAddress = runAddress;
		}
		else
			return false;
	}
	// .J64 (Jaguar cartridge ROM image) is implied by the FileList object...

	return true;
}

//
// Get the length of a (possibly) gzipped file
//
int gzfilelength(gzFile gd)
{
   int size = 0, length = 0;
   unsigned char buffer[0x10000];

   gzrewind(gd);

   do
   {
      // Read in chunks until EOF
      size = gzread(gd, buffer, 0x10000);

      if (size <= 0)
      	break;

      length += size;
   }
   while (!gzeof(gd));

   gzrewind(gd);
   return length;
}
#endif
