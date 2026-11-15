#ifndef __GAMECONTROLLER_H__
#define __GAMECONTROLLER_H__

#include <SDL2/SDL.h>

void GameControllerInit(void);
void GameControllerDone(void);
void GameControllerHandleEvent(const SDL_Event &event);
void GameControllerUpdate(void);
int GameControllerGetPaddleValue(int paddle);
bool GameControllerGetButton(int button);

#endif	// __GAMECONTROLLER_H__
