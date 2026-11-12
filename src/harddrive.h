#ifndef __HARDDRIVE_H__
#define __HARDDRIVE_H__

#include <stdint.h>

//void InstallHardDrive(uint8_t slot);
bool InstallHardDrive(uint8_t slot, const char *imageOverride, bool suppressConfiguredImage);

// True while the card's firmware is timing out on an empty SCSI ID (boot-time bus scan)
bool HardDriveIsWaiting(void);

#endif	// __HARDDRIVE_H__
