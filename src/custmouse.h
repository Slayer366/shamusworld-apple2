#ifndef __CUSTMOUSE_H__
#define __CUSTMOUSE_H__

#include <SDL2/SDL.h>

class CustMouse
{
public:
		CustMouse();

		void setPosition(int x, int y);
		void draw(SDL_Renderer * sdlRenderer) const;

	private:
		int x;
		int y;
};

#endif /* __CUSTMOUSE_H__ */
