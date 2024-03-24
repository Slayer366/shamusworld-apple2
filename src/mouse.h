
#ifndef __CUSTMOUSE_H__
#define __CUSTMOUSE_H__

#include <SDL2/SDL.h>

int mouseX;
int mouseY;

class CustMouse{
	public:

const char custmouseico[(12 * 20) + 1] =
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

	SDL_Rect rect;
	SDL_Rect pointer;

	void DrawCharArray(SDL_Renderer * sdlRenderer, const char * array, int x,
	int y, int w, int h, int r, int g, int b){
	SDL_SetRenderDrawColor(sdlRenderer, r, g, b, 0xFF);

	for(int j=0; j<h; j++)
	{
		for(int i=0; i<w; i++)
		{
			if (array[(j * w) + i] != ' ')
				SDL_RenderDrawPoint(sdlRenderer, x + i, y + j);
		}
	}

	SDL_SetRenderDrawColor(sdlRenderer, 0x00, 0x00, 0x00, 0x00);
	}

	CustMouse(){
		rect.w = 12;
		rect.h = 20;
		pointer.w = 1;
		pointer.h = 1;
	}

	void update(SDL_Renderer * sdlRenderer) {
		pointer.x = mouseX;
		pointer.y = mouseY;
	}

	void draw(SDL_Renderer * sdlRenderer){
		int r = 0x87, g = 0xCE, b = 0xFF;
		DrawCharArray(sdlRenderer, custmouseico, pointer.x, pointer.y, rect.w, rect.h, r, g, b);
	}

};

#endif /* __CUSTMOUSE_H__ */
