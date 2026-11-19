// SDL2 GameController input for the Apple II paddles and pushbuttons.

#include "gamecontroller.h"

#include "log.h"
#include "settings.h"

struct ControllerState
{
	SDL_GameController *handle;
	bool hasLeftStick;
};

static ControllerState controllers[2] = {{NULL, false}, {NULL, false}};
static SDL_atomic_t paddleValues[4];
static SDL_atomic_t buttonStates[2];
static SDL_atomic_t controllerConnected;
static bool controllerEnabled = false;
static bool twoJoysticksEnabled = false;

enum KeypadKey
{
	KEYPAD_1,
	KEYPAD_2,
	KEYPAD_3,
	KEYPAD_4,
	KEYPAD_5,
	KEYPAD_6,
	KEYPAD_7,
	KEYPAD_8,
	KEYPAD_9,
	KEYPAD_0,
	KEYPAD_PERIOD,
	KEYPAD_KEY_COUNT
};

static SDL_atomic_t keypadKeys[KEYPAD_KEY_COUNT];
static bool keypadFallbackActive = false;

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

static int KeypadKeyIndex(const SDL_Keysym &key)
{
	switch (key.scancode)
	{
	case SDL_SCANCODE_KP_1: return KEYPAD_1;
	case SDL_SCANCODE_KP_2: return KEYPAD_2;
	case SDL_SCANCODE_KP_3: return KEYPAD_3;
	case SDL_SCANCODE_KP_4: return KEYPAD_4;
	case SDL_SCANCODE_KP_5: return KEYPAD_5;
	case SDL_SCANCODE_KP_6: return KEYPAD_6;
	case SDL_SCANCODE_KP_7: return KEYPAD_7;
	case SDL_SCANCODE_KP_8: return KEYPAD_8;
	case SDL_SCANCODE_KP_9: return KEYPAD_9;
	case SDL_SCANCODE_KP_0: return KEYPAD_0;
	case SDL_SCANCODE_KP_PERIOD: return KEYPAD_PERIOD;
	default:
		break;
	}

	// Scancodes are normally available, but retain a keycode fallback for
	// platforms/backends that report SDL_SCANCODE_UNKNOWN.
	switch (key.sym)
	{
	case SDLK_KP_1: return KEYPAD_1;
	case SDLK_KP_2: return KEYPAD_2;
	case SDLK_KP_3: return KEYPAD_3;
	case SDLK_KP_4: return KEYPAD_4;
	case SDLK_KP_5: return KEYPAD_5;
	case SDLK_KP_6: return KEYPAD_6;
	case SDLK_KP_7: return KEYPAD_7;
	case SDLK_KP_8: return KEYPAD_8;
	case SDLK_KP_9: return KEYPAD_9;
	case SDLK_KP_0: return KEYPAD_0;
	case SDLK_KP_PERIOD: return KEYPAD_PERIOD;
	default: return -1;
	}
}

static void UpdateKeypadJoystick(void)
{
	const bool left = SDL_AtomicGet(&keypadKeys[KEYPAD_1])
		|| SDL_AtomicGet(&keypadKeys[KEYPAD_4])
		|| SDL_AtomicGet(&keypadKeys[KEYPAD_7]);
	const bool right = SDL_AtomicGet(&keypadKeys[KEYPAD_3])
		|| SDL_AtomicGet(&keypadKeys[KEYPAD_6])
		|| SDL_AtomicGet(&keypadKeys[KEYPAD_9]);
	const bool up = SDL_AtomicGet(&keypadKeys[KEYPAD_7])
		|| SDL_AtomicGet(&keypadKeys[KEYPAD_8])
		|| SDL_AtomicGet(&keypadKeys[KEYPAD_9]);
	const bool down = SDL_AtomicGet(&keypadKeys[KEYPAD_1])
		|| SDL_AtomicGet(&keypadKeys[KEYPAD_2])
		|| SDL_AtomicGet(&keypadKeys[KEYPAD_3]);

	// Like LinApple's centered keypad mode, released directions return to
	// the neutral paddle position instead of remaining at an endpoint.
	SDL_AtomicSet(&paddleValues[0], DPadToPaddle(left, right));
	SDL_AtomicSet(&paddleValues[1], DPadToPaddle(up, down));
	SDL_AtomicSet(&paddleValues[2], 128);
	SDL_AtomicSet(&paddleValues[3], 128);
	SDL_AtomicSet(&buttonStates[0], SDL_AtomicGet(&keypadKeys[KEYPAD_0]));
	SDL_AtomicSet(&buttonStates[1], SDL_AtomicGet(&keypadKeys[KEYPAD_PERIOD]));
}

static bool IsAlreadyOpen(SDL_JoystickID instanceID)
{
	for (int i = 0; i < 2; i++)
	{
		if (controllers[i].handle != NULL)
		{
			SDL_Joystick *joystick = SDL_GameControllerGetJoystick(controllers[i].handle);
			if (SDL_JoystickInstanceID(joystick) == instanceID) {
				return true;
			}
		}
	}
	return false;
}

static bool OpenController(int deviceIndex, int player)
{
	if (!SDL_IsGameController(deviceIndex)) {
		return false;
	}

	SDL_GameController *handle = SDL_GameControllerOpen(deviceIndex);
	if (handle == NULL)
	{
		WriteLog("GameController: Could not open controller %d: %s\n", deviceIndex, SDL_GetError());
		return false;
	}

	controllers[player].handle = handle;
	controllers[player].hasLeftStick = SDL_GameControllerHasAxis(handle, SDL_CONTROLLER_AXIS_LEFTX)
		&& SDL_GameControllerHasAxis(handle, SDL_CONTROLLER_AXIS_LEFTY);

	const char *name = SDL_GameControllerName(handle);
	if (controllers[player].hasLeftStick) {
		WriteLog("GameController: Opened player %d controller %s (left thumbstick)\n", player + 1, name != NULL ? name : "Unknown controller");
	} else {
		WriteLog("GameController: Opened player %d controller %s (using D-pad for joystick)\n", player + 1, name != NULL ? name : "Unknown controller");
	}
	return true;
}

static void FindControllers(void)
{
	if (!controllerEnabled) {
		return;
	}

	const int maxControllers = twoJoysticksEnabled ? 2 : 1;
	for (int i = 0; i < SDL_NumJoysticks(); i++)
	{
		if (controllers[0].handle != NULL && controllers[1].handle != NULL) {
			break;
		}

		if (!SDL_IsGameController(i)) {
			continue;
		}

		SDL_JoystickID instanceID = SDL_JoystickGetDeviceInstanceID(i);
		if (instanceID < 0 || IsAlreadyOpen(instanceID)) {
			continue;
		}

		int player = controllers[0].handle == NULL ? 0 : 1;
		if (player < maxControllers) {
			OpenController(i, player);
		}
	}
	//SDL_AtomicSet(&controllerConnected, controllers[0].handle != NULL ? 1 : 0);
	// Joystick input remains available through the keypad when no controller is open.
	SDL_AtomicSet(&controllerConnected, 1);
}

static void UpdateControllerPaddles(SDL_GameController *handle, bool hasLeftStick, int firstPaddle)
{
	if (hasLeftStick)
	{
		SDL_AtomicSet(&paddleValues[firstPaddle], AxisToPaddle(SDL_GameControllerGetAxis(handle, SDL_CONTROLLER_AXIS_LEFTX)));
		SDL_AtomicSet(&paddleValues[firstPaddle + 1], AxisToPaddle(SDL_GameControllerGetAxis(handle, SDL_CONTROLLER_AXIS_LEFTY)));
	}
	else
	{
		SDL_AtomicSet(&paddleValues[firstPaddle], DPadToPaddle(
			SDL_GameControllerGetButton(handle, SDL_CONTROLLER_BUTTON_DPAD_LEFT),
			SDL_GameControllerGetButton(handle, SDL_CONTROLLER_BUTTON_DPAD_RIGHT)));
		SDL_AtomicSet(&paddleValues[firstPaddle + 1], DPadToPaddle(
			SDL_GameControllerGetButton(handle, SDL_CONTROLLER_BUTTON_DPAD_UP),
			SDL_GameControllerGetButton(handle, SDL_CONTROLLER_BUTTON_DPAD_DOWN)));
	}
}

void GameControllerUpdate(void)
{
//	if (!controllerEnabled || controllers[0].handle == NULL || !SDL_GameControllerGetAttached(controllers[0].handle)) {
	if (!controllerEnabled) {
		return;
	}

	SDL_GameControllerUpdate();

	if (controllers[0].handle == NULL || !SDL_GameControllerGetAttached(controllers[0].handle)) {
		if (!keypadFallbackActive) {
			WriteLog("GameController: No usable controller; numeric keypad fallback active (1-9, 0, .)\n");
			keypadFallbackActive = true;
		}
		UpdateKeypadJoystick();
		return;
	}

	if (keypadFallbackActive) {
		WriteLog("GameController: Controller detected; numeric keypad fallback inactive\n");
		keypadFallbackActive = false;
	}

	const bool twoPlayerActive = twoJoysticksEnabled && controllers[1].handle != NULL
		&& SDL_GameControllerGetAttached(controllers[1].handle);

	if (twoPlayerActive)
	{
		// In two-player mode each controller becomes one of two Apple II joysticks (no duh! :-):
		// left stick (or D-pad) plus its A button.
		UpdateControllerPaddles(controllers[0].handle, controllers[0].hasLeftStick, 0);
		UpdateControllerPaddles(controllers[1].handle, controllers[1].hasLeftStick, 2);
		SDL_AtomicSet(&buttonStates[0], SDL_GameControllerGetButton(controllers[0].handle, SDL_CONTROLLER_BUTTON_A));
		SDL_AtomicSet(&buttonStates[1], SDL_GameControllerGetButton(controllers[1].handle, SDL_CONTROLLER_BUTTON_A));
	}
	else
	{
		// Preserve the existing four-paddle two-button mapping when only one controller
		// is detected or available even if two-player mode is enabled.
		UpdateControllerPaddles(controllers[0].handle, controllers[0].hasLeftStick, 0);
		SDL_AtomicSet(&paddleValues[2], AxisToPaddle(SDL_GameControllerGetAxis(controllers[0].handle, SDL_CONTROLLER_AXIS_RIGHTX)));
		SDL_AtomicSet(&paddleValues[3], AxisToPaddle(SDL_GameControllerGetAxis(controllers[0].handle, SDL_CONTROLLER_AXIS_RIGHTY)));
		SDL_AtomicSet(&buttonStates[0], SDL_GameControllerGetButton(controllers[0].handle, SDL_CONTROLLER_BUTTON_A));
		SDL_AtomicSet(&buttonStates[1], SDL_GameControllerGetButton(controllers[0].handle, SDL_CONTROLLER_BUTTON_B));
	}
}

void GameControllerInit(void)
{
	controllerEnabled = settings.useJoystick;
	twoJoysticksEnabled = settings.twoJoysticks;
	ClearControllerState();

	for (int i = 0; i < KEYPAD_KEY_COUNT; i++) {
		SDL_AtomicSet(&keypadKeys[i], 0);
	}
	SDL_AtomicSet(&controllerConnected, controllerEnabled ? 1 : 0);
	keypadFallbackActive = false;

	if (!controllerEnabled)
	{
		WriteLog("GameController: Disabled in configuration (useJoystick = 0)\n");
		return;
	}

	SDL_GameControllerAddMappingsFromFile("gamecontrollerdb.txt");
	SDL_GameControllerEventState(SDL_ENABLE);
	FindControllers();
	GameControllerUpdate();
}

void GameControllerDone(void)
{
	controllerEnabled = false;
	keypadFallbackActive = false;
	SDL_AtomicSet(&controllerConnected, 0);
	for (int i = 0; i < 2; i++)
	{
		if (controllers[i].handle != NULL)
		{
			SDL_GameControllerClose(controllers[i].handle);
			controllers[i].handle = NULL;
			controllers[i].hasLeftStick = false;
		}
	}

	ClearControllerState();
}

void GameControllerHandleEvent(const SDL_Event &event)
{
	if (!controllerEnabled) {
		return;
	}

	if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP)
	{
		const int keypadKey = KeypadKeyIndex(event.key.keysym);
		if (keypadKey >= 0)
			SDL_AtomicSet(&keypadKeys[keypadKey], event.type == SDL_KEYDOWN ? 1 : 0);
		return;
	}
	else if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_FOCUS_LOST)
	{
		// Do not leave a virtual joystick direction or button stuck if focus is lost
		// before SDL delivers the corresponding key-up event.
		for (int i = 0; i < KEYPAD_KEY_COUNT; i++) {
			SDL_AtomicSet(&keypadKeys[i], 0);
		}
		if (controllers[0].handle == NULL || !SDL_GameControllerGetAttached(controllers[0].handle)) {
			UpdateKeypadJoystick();
		}
		return;
	}

	if (event.type == SDL_CONTROLLERDEVICEADDED)
	{
		FindControllers();
		GameControllerUpdate();
	}
	else if (event.type == SDL_CONTROLLERDEVICEREMOVED)
	{
		for (int i = 0; i < 2; i++)
		{
			if (controllers[i].handle == NULL)
				continue;

			SDL_Joystick *joystick = SDL_GameControllerGetJoystick(controllers[i].handle);
			if (SDL_JoystickInstanceID(joystick) == event.cdevice.which)
			{
				WriteLog("GameController: Player %d controller disconnected\n", i + 1);
				SDL_GameControllerClose(controllers[i].handle);
				controllers[i].handle = NULL;
				controllers[i].hasLeftStick = false;

				// Keep the remaining controller in player 1's position.
				if (i == 0 && controllers[1].handle != NULL)
				{
					controllers[0] = controllers[1];
					controllers[1].handle = NULL;
					controllers[1].hasLeftStick = false;
				}

				ClearControllerState();
				FindControllers();
				GameControllerUpdate();
				return;
			}
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