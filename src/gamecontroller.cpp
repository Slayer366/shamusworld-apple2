// SDL2 GameController input for the Apple II paddles and pushbuttons.

#include "gamecontroller.h"

#include "log.h"
#include "settings.h"

static SDL_GameController *controller = NULL;
static SDL_atomic_t paddleValues[4];
static SDL_atomic_t buttonStates[2];
static SDL_atomic_t controllerConnected;
static bool controllerEnabled = false;
static bool hasLeftStick = false;

static int AxisToPaddle(Sint16 axis)
{
	const int deadzone = 3932; // 12% of 32767
	const int value = (int)axis;

	if (value >= -deadzone && value <= deadzone) {
		return 128;
	}

	if (value < -deadzone) {
		return 128 - ((-value - deadzone) * 128 / (32768 - deadzone));
	}

	return 128 + ((value - deadzone) * 127 / (32767 - deadzone));
}

static int DPadToPaddle(bool negative, bool positive)
{
	if (negative == positive) {
		return 128;
	}

	return negative ? 0 : 255;
}

static void ClearControllerState(void)
{
	for (int i = 0; i < 4; i++) {
		SDL_AtomicSet(&paddleValues[i], 128);
	}

	for (int i = 0; i < 2; i++) {
		SDL_AtomicSet(&buttonStates[i], 0);
	}
}

static bool OpenController(int deviceIndex)
{
	if (!SDL_IsGameController(deviceIndex)) {
		return false;
	}

	controller = SDL_GameControllerOpen(deviceIndex);
	if (controller == NULL)
	{
		WriteLog("GameController: Could not open controller %d: %s\n", deviceIndex, SDL_GetError());
		return false;
	}

	hasLeftStick = SDL_GameControllerHasAxis(controller, SDL_CONTROLLER_AXIS_LEFTX)
		&& SDL_GameControllerHasAxis(controller, SDL_CONTROLLER_AXIS_LEFTY);

	const char *name = SDL_GameControllerName(controller);
	if (hasLeftStick) {
		WriteLog("GameController: Opened %s (left thumbstick)\n", name != NULL ? name : "Unknown controller");
	} else {
		WriteLog("GameController: Opened %s (using D-pad for joystick)\n", name != NULL ? name : "Unknown controller");
	}
	SDL_AtomicSet(&controllerConnected, 1);
	return true;
}

static void FindController(void)
{
	if (!controllerEnabled || controller != NULL) {
		return;
	}

	for (int i = 0; i < SDL_NumJoysticks(); i++)
	{
		if (OpenController(i)) {
			break;
		}
	}
}

void GameControllerUpdate(void)
{
	if (!controllerEnabled || controller == NULL || !SDL_GameControllerGetAttached(controller)) {
		return;
	}

	SDL_GameControllerUpdate();

	if (hasLeftStick)
	{
		SDL_AtomicSet(&paddleValues[0], AxisToPaddle(SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_LEFTX)));
		SDL_AtomicSet(&paddleValues[1], AxisToPaddle(SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_LEFTY)));
	}
	else
	{
		SDL_AtomicSet(&paddleValues[0], DPadToPaddle(
			SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_LEFT),
			SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_RIGHT)));
		SDL_AtomicSet(&paddleValues[1], DPadToPaddle(
			SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_UP),
			SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_DOWN)));
	}

	SDL_AtomicSet(&paddleValues[2], AxisToPaddle(SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_RIGHTX)));
	SDL_AtomicSet(&paddleValues[3], AxisToPaddle(SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_RIGHTY)));

	SDL_AtomicSet(&buttonStates[0], SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_A));
	SDL_AtomicSet(&buttonStates[1], SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_B));
}

void GameControllerInit(void)
{
	controllerEnabled = settings.useJoystick;
	if (!controllerEnabled)
	{
		WriteLog("GameController: Disabled in configuration (useJoystick = 0)\n");
		return;
	}

	SDL_GameControllerAddMappingsFromFile("gamecontrollerdb.txt");
	SDL_GameControllerEventState(SDL_ENABLE);
	FindController();
	GameControllerUpdate();
}

void GameControllerDone(void)
{
	controllerEnabled = false;
	SDL_AtomicSet(&controllerConnected, 0);
	if (controller != NULL)
	{
		SDL_GameControllerClose(controller);
		controller = NULL;
	}

	ClearControllerState();
}

void GameControllerHandleEvent(const SDL_Event &event)
{
	if (!controllerEnabled) {
		return;
	}

	if (event.type == SDL_CONTROLLERDEVICEADDED)
	{
		FindController();
	}
	else if (event.type == SDL_CONTROLLERDEVICEREMOVED && controller != NULL)
	{
		SDL_Joystick *joystick = SDL_GameControllerGetJoystick(controller);
		if (SDL_JoystickInstanceID(joystick) == event.cdevice.which)
		{
			WriteLog("GameController: Controller disconnected\n");
			SDL_AtomicSet(&controllerConnected, 0);
			SDL_GameControllerClose(controller);
			controller = NULL;
			hasLeftStick = false;
			ClearControllerState();
			FindController();
		}
	}
}

int GameControllerGetPaddleValue(int paddle)
{
	if (paddle < 0 || paddle >= 4 || !SDL_AtomicGet(&controllerConnected)) {
		return -1;
	}

	return SDL_AtomicGet(&paddleValues[paddle]);
}

bool GameControllerGetButton(int button)
{
	if (button < 0 || button >= 2 || !SDL_AtomicGet(&controllerConnected)) {
		return false;
	}

	return SDL_AtomicGet(&buttonStates[button]) != 0;
}