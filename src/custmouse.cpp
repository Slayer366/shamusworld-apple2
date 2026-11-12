#include "custmouse.h"

namespace
{

enum
{
    CursorWidth = 12,
    CursorHeight = 20
};

static const char custmouseico[(CursorWidth * CursorHeight) + 1] =
  	"@           "    //  	"@           "  Original cursor art
  	"@@          "    //   	"@@          "
  	"@.@         "    //   	"@.@         "
  	"@..@        "    //   	"@..@        "
  	"@...@       "    //   	"@...@       "
  	"@....@      "    //   	"@....@      "
  	"@.. ..@     "    //   	"@.....@     "
  	"@..  ..@    "    //   	"@......@    "
  	"@..  ...@   "    //   	"@.......@   "
  	"@....@@@@@  "    //   	"@........@  "
  	"@...@       "    //   	"@.....@@@@@ "
  	"@..@        "    //   	"@..@..@     "
  	"@.@         "    //   	"@.@ @..@    "
  	"@@          "    //   	"@@  @..@    "
  	"@           "    //   	"@    @..@   "
  	"            "    //   	"     @..@   "
  	"            "    //   	"      @..@  "
  	"            "    //   	"      @..@  "
  	"            "    //   	"       @@   "
  	"            ";   //   	"            ";
}

CustMouse::CustMouse() : x(0), y(0)
{
}

void CustMouse::setPosition(int mouseX, int mouseY)
{
    x = mouseX;
    y = mouseY;
}

void CustMouse::draw(SDL_Renderer *sdlRenderer) const
{
    Uint8 cmR, cmG, cmB, cmA;
    SDL_GetRenderDrawColor(sdlRenderer, &cmR, &cmG, &cmB, &cmA);
    SDL_SetRenderDrawColor(sdlRenderer, 0x87, 0xCE, 0xFF, 0xFF);

    for (int row = 0; row < CursorHeight; ++row)
    {
        for (int col = 0; col < CursorWidth; ++col)
        {
            if (custmouseico[row * CursorWidth + col] != ' ')
                SDL_RenderDrawPoint(sdlRenderer, x + col, y + row);
        }
    }

    SDL_SetRenderDrawColor(sdlRenderer, cmR, cmG, cmB, cmA);
}
