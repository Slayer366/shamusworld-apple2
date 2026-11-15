#ifndef __HARDDRIVE_H__
#define __HARDDRIVE_H__

#include <stdint.h>

//void InstallHardDrive(uint8_t slot);
bool InstallHardDrive(uint8_t slot, const char *imageOverride, bool suppressConfiguredImage);

// Ejecting the image leaves the SCSI card installed in its slot
// and reports NOT READY until another image is mounted.
void EjectHardDrive(void);
bool HardDriveIsMounted(void);
const char *HardDriveGetImageName(void);

// True while the card's firmware is timing out on an empty SCSI ID (boot-time bus scan)
bool HardDriveIsWaiting(void);

#endif	// __HARDDRIVE_H__
