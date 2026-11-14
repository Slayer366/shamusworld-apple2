//
// Sound Interface
//
// by James Hammons
// (C) 2005-2018 Underground Software
//
// JLH = James Hammons <jlhamm@acm.org>
//
// WHO  WHEN        WHAT
// ---  ----------  -----------------------------------------------------------
// JLH  12/02/2005  Fixed a problem with sound callback thread signaling the
//                  main thread
// JLH  12/03/2005  Fixed sound callback dropping samples when the sample
//                  buffer is shorter than the callback sample buffer
//

// STILL TO DO:
//
// - Figure out why it's losing samples (Bard's Tale) [DONE]
// - Figure out why it's playing too fast [DONE]
//

#include "sound.h"
#include "apple2.h"

#include <string.h>			// For memset, memcpy
#include <SDL2/SDL.h>
#include "log.h"
#include "mockingboard.h"


// Useful defines

//#define DEBUG

#define SAMPLES_PER_FRAME	(SAMPLE_RATE / APPLE2_FRAME_RATE)
#define CYCLES_PER_SAMPLE	(APPLE2_CPU_CLOCK_HZ / SAMPLE_RATE)
// 32K ought to be enough for anybody
#define SOUND_BUFFER_SIZE	(32768)

// Global variables


// Local variables

static SDL_AudioSpec desired, obtained;
static SDL_AudioDeviceID device;
static bool soundInitialized = false;
static bool speakerState = false;
static uint16_t soundBuffer[SOUND_BUFFER_SIZE];
static uint32_t soundBufferRead;
static uint32_t soundBufferWrite;
static uint32_t soundBufferPos;
static uint16_t sample;
static uint8_t ampPtr = 12;						// Start with -2047 - +2047

// The emulated machine produces samples from its CPU clock, while the host
// audio device consumes them using its own clock. A small adaptive resampler
// keeps the source queue near a short target latency without changing emulated
// CPU timing. resampleStep is source samples consumed per output sample.
static double resamplePhase = 0.0;
static double resampleIntegral = 0.0;
static double resampleStep = 1.0;
static const double AUDIO_QUEUE_TARGET = 2048.0;
static const double RESAMPLE_KP = 0.0000005;
static const double RESAMPLE_KI = 0.00000002;
static const double RESAMPLE_MAX_CORRECTION = 0.005;

static int16_t amplitude[17] = { 0, 1, 2, 3, 7, 15, 31, 63, 127, 255,
	511, 1023, 2047, 4095, 8191, 16383, 32767 };

// Private function prototypes

static void SDLSoundCallback(void * userdata, Uint8 * buffer, int length);


/*
N.B: We can convert this from the current callback model to a push model by using SDL_QueueAudio(SDL_AudioDeviceID id, const void * data, Uint32 len) where id is the audio device ID, data is a pointer to the sound buffer, and len is the size of the buffer in *bytes* (not samples!).  To use this method, we need to set up things as usual but instead of putting the callback function pointer in desired.callback, we put a NULL there.  The downside is that we can't tell if the buffer is being starved or not, which is why we haven't kicked it to the curb just yet--we want to know why we're still getting buffer starvation even if it's not as frequent as it used to be.  :-/
You can get the size of the audio already queued with SDL_GetQueuedAudioSize(SDL_AudioDeviceID id), which will return the size of the buffer in bytes (again, *not* samples!).
*/
//
// Initialize the SDL sound system
//
void SoundInit(void)
{
	SDL_zero(desired);
	desired.freq = SAMPLE_RATE;		// SDL will do conversion on the fly, if it can't get the exact rate. Nice!
	desired.format = AUDIO_U16SYS;	// This uses the native endian (for portability)...
	desired.channels = 1;
	desired.samples = 512;			// Let's try a 1/2K buffer
	desired.callback = SDLSoundCallback;

	device = SDL_OpenAudioDevice(NULL, 0, &desired, &obtained, 0);

	if (device == 0)
	{
		WriteLog("Sound: Failed to initialize SDL sound.\n");
		WriteLog("SDL sez: %s\n", SDL_GetError());
		return;
	}

	soundBufferRead = 0;
	soundBufferWrite = 0;
	soundBufferPos = 0;
	resamplePhase = 0.0;
	resampleIntegral = 0.0;
	resampleStep = 1.0;
	sample = desired.silence;		// ? wilwok ? yes

	SDL_PauseAudioDevice(device, 0);// Start playback!
	soundInitialized = true;
	WriteLog("Sound: Successfully initialized.\n");
	WriteLog("Sound: requested %d Hz, format 0x%04X, %u channel(s), %u samples; obtained %d Hz, format 0x%04X, %u channel(s), %u samples.\n",
		desired.freq, desired.format, desired.channels, desired.samples,
		obtained.freq, obtained.format, obtained.channels, obtained.samples);
}


//
// Close down the SDL sound subsystem
//
void SoundDone(void)
{
	if (soundInitialized)
	{
		SDL_PauseAudioDevice(device, 1);
		SDL_CloseAudioDevice(device);
		WriteLog("Sound: Done.\n");
	}
}


void SoundPause(void)
{
	if (soundInitialized)
		SDL_PauseAudioDevice(device, 1);
}


void SoundResume(void)
{
	if (soundInitialized)
		SDL_PauseAudioDevice(device, 0);
}


//
// Sound card callback handler
//
static void SDLSoundCallback(void * /*userdata*/, Uint8 * buffer8, int length8)
{
	// Recast this as a 16-bit type...
	uint16_t * buffer = (uint16_t *)buffer8;

	uint32_t outputCount = (uint32_t)length8 / sizeof(uint16_t);

	uint32_t available = soundBufferPos;
	uint32_t copied = 0;
	double elapsed = (obtained.freq > 0) ? (double)outputCount / (double)obtained.freq : 0.0106666667;

	// Positive queue error means the producer is getting ahead, so consume
	// source samples slightly faster. The integral term corrects small,
	// persistent clock differences.
	double error = (double)available - AUDIO_QUEUE_TARGET;
	resampleIntegral += error * RESAMPLE_KI * elapsed;
	if (resampleIntegral > RESAMPLE_MAX_CORRECTION) {
		resampleIntegral = RESAMPLE_MAX_CORRECTION;
	}
	else if (resampleIntegral < -RESAMPLE_MAX_CORRECTION) {
		resampleIntegral = -RESAMPLE_MAX_CORRECTION;
	}

	double correction = RESAMPLE_KP * error + resampleIntegral;
	if (correction > RESAMPLE_MAX_CORRECTION) {
		correction = RESAMPLE_MAX_CORRECTION;
	}
	else if (correction < -RESAMPLE_MAX_CORRECTION) {
		correction = -RESAMPLE_MAX_CORRECTION;
	}
	resampleStep = 1.0 + correction;

	// Linear interpolation is sufficient for this very small clock correction
	// and avoids abrupt sample drops when the queue grows.
	for (uint32_t i = 0; i < outputCount; i++)
	{
		uint32_t index = (uint32_t)resamplePhase;
		if (index + 1 >= available) {
			break;
		}

		uint32_t firstIndex = (soundBufferRead + index) % SOUND_BUFFER_SIZE;
		uint32_t secondIndex = (firstIndex + 1) % SOUND_BUFFER_SIZE;
		uint16_t first = soundBuffer[firstIndex];
		uint16_t second = soundBuffer[secondIndex];
		double fraction = resamplePhase - (double)index;
		int32_t delta = (int32_t)second - (int32_t)first;
		buffer[i] = (uint16_t)((double)first + (double)delta * fraction);

		resamplePhase += resampleStep;
		copied++;
	}

	// Retain one source sample for interpolation across callback boundaries.
	uint32_t consumed = (uint32_t)resamplePhase;
	if (available > 0 && consumed >= available) {
		consumed = available - 1;
	}
	soundBufferRead = (soundBufferRead + consumed) % SOUND_BUFFER_SIZE;
	soundBufferPos -= consumed;
	resamplePhase -= (double)consumed;

	// If the producer has not supplied enough source samples, silence only the
	// unfilled tail. Do not discard the samples that were available.
	for (uint32_t i = copied; i < outputCount; i++) {
		buffer[i] = obtained.silence;
	}
}


//
// This is called by the main CPU thread every ~21.666 cycles.
//

// Speaker activity is tracked (in emulated CPU cycles) so disk turbo can stay
// off while a program is making noise.
static uint64_t lastSpeakerActivityCycle = 0;
static bool speakerActivitySeen = false;
static const uint64_t SPEAKER_ACTIVITY_TIMEOUT = 204097;	// 0.2 seconds at 1.0204832 MHz

// If 'discardOutput' is true (disk turbo), the AY generators are still stepped
// so they stay in sync, but nothing is queued: samples made faster than real
// time would only overflow the audio buffer.
void WriteSampleToBuffer(bool discardOutput)
{
//	uint16_t s1 = AYGetSample(0);
//	uint16_t s2 = AYGetSample(1);
	uint16_t s1 = mb[0].ay[0].GetSample();
	uint16_t s2 = mb[0].ay[1].GetSample();

	if (discardOutput) {
		return;
	}

	if (!soundInitialized) {
		return;
	}

	SDL_LockAudioDevice(device);
//	soundBuffer[soundBufferPos++] = sample + s1 + s2;
	if (soundBufferPos < SOUND_BUFFER_SIZE - 1)
	{
		soundBuffer[soundBufferWrite] = sample + s1 + s2;
		soundBufferWrite = (soundBufferWrite + 1) % SOUND_BUFFER_SIZE;
		soundBufferPos++;
	}

	// A full queue should be exceptionally rare once adaptive resampling has
	// settled. Never stall the emulated CPU waiting for the host audio thread.
	SDL_UnlockAudioDevice(device);
}


bool SpeakerIsActive(void)
{
	return speakerActivitySeen && ((mainCPU.clock - lastSpeakerActivityCycle) < SPEAKER_ACTIVITY_TIMEOUT);
}


void ToggleSpeaker(void)
{
	lastSpeakerActivityCycle = mainCPU.clock;
	speakerActivitySeen = true;

	if (!soundInitialized)
		return;

	speakerState = !speakerState;
	sample = (speakerState ? amplitude[ampPtr] : 0);
}


void VolumeUp(void)
{
	// Currently set for 16-bit samples
	if (ampPtr < 16)
		ampPtr++;
}


void VolumeDown(void)
{
	if (ampPtr > 0)
		ampPtr--;
}


uint8_t GetVolume(void)
{
	return ampPtr;
}

