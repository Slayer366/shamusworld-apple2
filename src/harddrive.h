#ifndef __HARDDRIVE_H__
#define __HARDDRIVE_H__

#include <stdint.h>

//void InstallHardDrive(uint8_t slot);
bool InstallHardDrive(uint8_t slot, const char *imageOverride, bool suppressConfiguredImage);

#endif	// __HARDDRIVE_H__

