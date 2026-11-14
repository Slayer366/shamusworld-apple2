//
// SOUND.H
//
// by James Hammons
// (C) 2004-2018 Underground Software
//

#ifndef __SOUND_H__
#define __SOUND_H__

#include <stdint.h>

#define SAMPLE_RATE		(48000.0)

#define APPLE2_CPU_CLOCK_HZ (1020484.32)
#define APPLE2_FRAME_RATE   (APPLE2_CPU_CLOCK_HZ / (65.0 * 262.0))

// Global variables (exported)


// Exported functions

void SoundInit(void);
void SoundDone(void);
void SoundPause(void);
void SoundResume(void);
void ToggleSpeaker(void);
void WriteSampleToBuffer(bool discardOutput);
bool SpeakerIsActive(void);
void VolumeUp(void);
void VolumeDown(void);
uint8_t GetVolume(void);

#endif	// __SOUND_H__

