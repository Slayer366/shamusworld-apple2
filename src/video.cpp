//
// VIDEO.CPP: SDL2/local hardware specific video routines
//
// by James Hammons
//
// JLH = James Hammons <jlhamm@acm.org>
//
// WHO  WHEN        WHAT
// ---  ----------  -----------------------------------------------------------
// JLH  01/04/2006  Added changelog ;-)
// JLH  01/20/2006  Cut out unnecessary buffering
//

#include "video.h"
#include <string.h>	// (for memset, etc... Lazy!)
#include <malloc.h>
#include "apple2-icon-64x64.h"
#include "log.h"
#include "settings.h"


// Local vars
static SDL_Window * sdlWindow = NULL;
static SDL_Texture * sdlTexture = NULL;

// Exported vars
SDL_Renderer * sdlRenderer = NULL;
uint32_t scrBuffer[VIRTUAL_SCREEN_WIDTH * VIRTUAL_SCREEN_HEIGHT * sizeof(uint32_t)];


//
// Prime SDL and create surfaces
//
bool InitVideo(void)
{
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_AUDIO | SDL_INIT_TIMER | SDL_INIT_NOPARACHUTE) != 0)
	{
		WriteLog("Video: Could not initialize the SDL library: %s\n", SDL_GetError());
		return false;
	}

	int retVal = SDL_CreateWindowAndRenderer(VIRTUAL_SCREEN_WIDTH * 2, VIRTUAL_SCREEN_HEIGHT * 2, SDL_WINDOW_OPENGL, &sdlWindow, &sdlRenderer);

	if (retVal != 0)
	{
		WriteLog("Video: Could not window and/or renderer: %s\n", SDL_GetError());
		return false;
	}

	// Make the scaled rendering look smoother.
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
//	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");
	SDL_RenderSetLogicalSize(sdlRenderer, VIRTUAL_SCREEN_WIDTH, VIRTUAL_SCREEN_HEIGHT);

	// Set the application's icon & title...
	SDL_Surface * iconSurface = SDL_CreateRGBSurfaceFrom(icon, 64, 64, 32, 64*4, 0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000);
	SDL_SetWindowIcon(sdlWindow, iconSurface);
	SDL_FreeSurface(iconSurface);
	SDL_SetWindowTitle(sdlWindow, "Apple2 Emulator");

	sdlTexture = SDL_CreateTexture(sdlRenderer,
		SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STREAMING,
		VIRTUAL_SCREEN_WIDTH, VIRTUAL_SCREEN_HEIGHT);

	WriteLog("Video: Successfully initialized.\n");
	return true;
}


//
// Free various SDL components
//
void VideoDone(void)
{
	WriteLog("Video: Shutting down SDL...\n");
	SDL_Quit();
	WriteLog("Video: Done.\n");
}


//
// Render the screen buffer to the primary screen surface
//
void RenderScreenBuffer(void)
{
	SDL_UpdateTexture(sdlTexture, NULL, scrBuffer, VIRTUAL_SCREEN_WIDTH * sizeof(Uint32));
	SDL_RenderClear(sdlRenderer);
	SDL_RenderCopy(sdlRenderer, sdlTexture, NULL, NULL);
}


//
// Fullscreen <-> window switching
//
void ToggleFullScreen(void)
{
	settings.fullscreen = !settings.fullscreen;

	int retVal = SDL_SetWindowFullscreen(sdlWindow, (settings.fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0));
	SDL_ShowCursor(settings.fullscreen ? 0 : 1);

	if (retVal != 0)
		WriteLog("Video::ToggleFullScreen: SDL error = %s\n", SDL_GetError());
}


