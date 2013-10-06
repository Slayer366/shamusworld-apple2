//
// This file was automagically generated by bin2c (by James Hammons)
//

#include "firmware.h"

uint8_t diskROM[0x100] = {		// Loads at $C600 (slot 6)
	0xA2, 0x20, 0xA0, 0x00, 0xA2, 0x03, 0x86, 0x3C, 0x8A, 0x0A, 0x24, 0x3C, 0xF0, 0x10, 0x05, 0x3C,
	0x49, 0xFF, 0x29, 0x7E, 0xB0, 0x08, 0x4A, 0xD0, 0xFB, 0x98, 0x9D, 0x56, 0x03, 0xC8, 0xE8, 0x10,
	0xE5, 0x20, 0x58, 0xFF, 0xBA, 0xBD, 0x00, 0x01, 0x0A, 0x0A, 0x0A, 0x0A, 0x85, 0x2B, 0xAA, 0xBD,
	0x8E, 0xC0, 0xBD, 0x8C, 0xC0, 0xBD, 0x8A, 0xC0, 0xBD, 0x89, 0xC0, 0xA0, 0x50, 0xBD, 0x80, 0xC0,
	0x98, 0x29, 0x03, 0x0A, 0x05, 0x2B, 0xAA, 0xBD, 0x81, 0xC0, 0xA9, 0x56, 0x20, 0xA8, 0xFC, 0x88,
	0x10, 0xEB, 0x85, 0x26, 0x85, 0x3D, 0x85, 0x41, 0xA9, 0x08, 0x85, 0x27, 0x18, 0x08, 0xBD, 0x8C,
	0xC0, 0x10, 0xFB, 0x49, 0xD5, 0xD0, 0xF7, 0xBD, 0x8C, 0xC0, 0x10, 0xFB, 0xC9, 0xAA, 0xD0, 0xF3,
	0xEA, 0xBD, 0x8C, 0xC0, 0x10, 0xFB, 0xC9, 0x96, 0xF0, 0x09, 0x28, 0x90, 0xDF, 0x49, 0xAD, 0xF0,
	0x25, 0xD0, 0xD9, 0xA0, 0x03, 0x85, 0x40, 0xBD, 0x8C, 0xC0, 0x10, 0xFB, 0x2A, 0x85, 0x3C, 0xBD,
	0x8C, 0xC0, 0x10, 0xFB, 0x25, 0x3C, 0x88, 0xD0, 0xEC, 0x28, 0xC5, 0x3D, 0xD0, 0xBE, 0xA5, 0x40,
	0xC5, 0x41, 0xD0, 0xB8, 0xB0, 0xB7, 0xA0, 0x56, 0x84, 0x3C, 0xBC, 0x8C, 0xC0, 0x10, 0xFB, 0x59,
	0xD6, 0x02, 0xA4, 0x3C, 0x88, 0x99, 0x00, 0x03, 0xD0, 0xEE, 0x84, 0x3C, 0xBC, 0x8C, 0xC0, 0x10,
	0xFB, 0x59, 0xD6, 0x02, 0xA4, 0x3C, 0x91, 0x26, 0xC8, 0xD0, 0xEF, 0xBC, 0x8C, 0xC0, 0x10, 0xFB,
	0x59, 0xD6, 0x02, 0xD0, 0x87, 0xA0, 0x00, 0xA2, 0x56, 0xCA, 0x30, 0xFB, 0xB1, 0x26, 0x5E, 0x00,
	0x03, 0x2A, 0x5E, 0x00, 0x03, 0x2A, 0x91, 0x26, 0xC8, 0xD0, 0xEE, 0xE6, 0x27, 0xE6, 0x3D, 0xA5,
	0x3D, 0xCD, 0x00, 0x08, 0xA6, 0x2B, 0x90, 0xDB, 0x4C, 0x01, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00
};

uint8_t hdROM[0x100] = {		// Loads at $C700 (slot 7)
	0xA9, 0x20, 0xA9, 0x00, 0xA9, 0x03, 0xA9, 0x3C, 0xA9, 0x00, 0x8D, 0xF2, 0xC0, 0xA9, 0x70, 0x8D,
	0xF3, 0xC0, 0xAD, 0xF0, 0xC0, 0x48, 0xAD, 0xF1, 0xC0, 0x18, 0xC9, 0x01, 0xD0, 0x01, 0x38, 0x68,
	0x90, 0x03, 0x4C, 0x00, 0xC6, 0xA9, 0x70, 0x85, 0x43, 0xA9, 0x00, 0x85, 0x44, 0x85, 0x46, 0x85,
	0x47, 0xA9, 0x08, 0x85, 0x45, 0xA9, 0x01, 0x85, 0x42, 0x20, 0x46, 0xC7, 0x90, 0x03, 0x4C, 0x00,
	0xC6, 0xA2, 0x70, 0x4C, 0x01, 0x08, 0x18, 0xA5, 0x42, 0x8D, 0xF2, 0xC0, 0xA5, 0x43, 0x8D, 0xF3,
	0xC0, 0xA5, 0x44, 0x8D, 0xF4, 0xC0, 0xA5, 0x45, 0x8D, 0xF5, 0xC0, 0xA5, 0x46, 0x8D, 0xF6, 0xC0,
	0xA5, 0x47, 0x8D, 0xF7, 0xC0, 0xAd, 0xF0, 0xC0, 0x48, 0xA5, 0x42, 0xC9, 0x01, 0xD0, 0x03, 0x20,
	0x7D, 0xC7, 0xAD, 0xF1, 0xC0, 0x18, 0xC9, 0x01, 0xD0, 0x01, 0x38, 0x68, 0x60, 0x98, 0x48, 0xA0,
	0x00, 0xAD, 0xF8, 0xC0, 0x91, 0x44, 0xC8, 0xD0, 0xF8, 0xE6, 0x45, 0xA0, 0x00, 0xAD, 0xF8, 0xC0,
	0x91, 0x44, 0xC8, 0xD0, 0xF8, 0x68, 0xA8, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x7F, 0xD7, 0x46
};

uint8_t parallelROM[0x100] = {
	0x18, 0xB0, 0x38, 0x48, 0x8A, 0x48, 0x98, 0x48, 0x08, 0x78, 0x20, 0x58, 0xFF, 0xBA, 0x68, 0x68, 
	0x68, 0x68, 0xA8, 0xCA, 0x9A, 0x68, 0x28, 0xAA, 0x90, 0x38, 0xBD, 0xB8, 0x05, 0x10, 0x19, 0x98, 
	0x29, 0x7F, 0x49, 0x30, 0xC9, 0x0A, 0x90, 0x3B, 0xC9, 0x78, 0xB0, 0x29, 0x49, 0x3D, 0xF0, 0x21, 
	0x98, 0x29, 0x9F, 0x9D, 0x38, 0x06, 0x90, 0x7E, 0xBD, 0xB8, 0x06, 0x30, 0x14, 0xA5, 0x24, 0xDD, 
	0x38, 0x07, 0xB0, 0x0D, 0xC9, 0x11, 0xB0, 0x09, 0x09, 0xF0, 0x3D, 0x38, 0x07, 0x65, 0x24, 0x85, 
	0x24, 0x4A, 0x38, 0xB0, 0x6D, 0x18, 0x6A, 0x3D, 0xB8, 0x06, 0x90, 0x02, 0x49, 0x81, 0x9D, 0xB8, 
	0x06, 0xD0, 0x53, 0xA0, 0x0A, 0x7D, 0x38, 0x05, 0x88, 0xD0, 0xFA, 0x9D, 0xB8, 0x04, 0x9D, 0x38, 
	0x05, 0x38, 0xB0, 0x43, 0xC5, 0x24, 0x90, 0x3A, 0x68, 0xA8, 0x68, 0xAA, 0x68, 0x4C, 0xF0, 0xFD, 
	0x90, 0xFE, 0xB0, 0xFE, 0x99, 0x80, 0xC0, 0x90, 0x37, 0x49, 0x07, 0xA8, 0x49, 0x0A, 0x0A, 0xD0, 
	0x06, 0xB8, 0x85, 0x24, 0x9D, 0x38, 0x07, 0xBD, 0xB8, 0x06, 0x4A, 0x70, 0x02, 0xB0, 0x23, 0x0A, 
	0x0A, 0xA9, 0x27, 0xB0, 0xCF, 0xBD, 0x38, 0x07, 0xFD, 0xB8, 0x04, 0xC9, 0xF8, 0x90, 0x03, 0x69, 
	0x27, 0xAC, 0xA9, 0x00, 0x85, 0x24, 0x18, 0x7E, 0xB8, 0x05, 0x68, 0xA8, 0x68, 0xAA, 0x68, 0x60, 
	0x90, 0x27, 0xB0, 0x00, 0x10, 0x11, 0xA9, 0x89, 0x9D, 0x38, 0x06, 0x9D, 0xB8, 0x06, 0xA9, 0x28, 
	0x9D, 0xB8, 0x04, 0xA9, 0x02, 0x85, 0x36, 0x98, 0x5D, 0x38, 0x06, 0x0A, 0xF0, 0x90, 0x5E, 0xB8, 
	0x05, 0x98, 0x48, 0x8A, 0x0A, 0x0A, 0x0A, 0x0A, 0xA8, 0xBD, 0x38, 0x07, 0xC5, 0x24, 0x68, 0xB0, 
	0x05, 0x48, 0x29, 0x80, 0x09, 0x20, 0x2C, 0x58, 0xFF, 0xF0, 0x03, 0xFE, 0x38, 0x07, 0x70, 0x84
};

// Various firmware from the IIe ROM file...

//Not sure what the heck this is...
uint8_t slot2e[0x100] = {
	0x2C, 0x58, 0xFF, 0x70, 0x0C, 0x38, 0x90, 0x18, 0xB8, 0x50, 0x06, 0x01, 0x31, 0x8E, 0x94, 0x97, 
	0x9A, 0x85, 0x27, 0x86, 0x35, 0x8A, 0x48, 0x98, 0x48, 0x08, 0x78, 0x8D, 0xFF, 0xCF, 0x20, 0x58, 
	0xFF, 0xBA, 0xBD, 0x00, 0x01, 0x8D, 0xF8, 0x07, 0xAA, 0x0A, 0x0A, 0x0A, 0x0A, 0x85, 0x26, 0xA8, 
	0x28, 0x50, 0x29, 0x1E, 0x38, 0x05, 0x5E, 0x38, 0x05, 0xB9, 0x8A, 0xC0, 0x29, 0x1F, 0xD0, 0x05, 
	0xA9, 0xEF, 0x20, 0x05, 0xC8, 0xE4, 0x37, 0xD0, 0x0B, 0xA9, 0x07, 0xC5, 0x36, 0xF0, 0x05, 0x85, 
	0x36, 0x18, 0x90, 0x08, 0xE4, 0x39, 0xD0, 0xF9, 0xA9, 0x05, 0x85, 0x38, 0xBD, 0x38, 0x07, 0x29, 
	0x02, 0x08, 0x90, 0x03, 0x4C, 0xBF, 0xC8, 0xBD, 0xB8, 0x04, 0x48, 0x0A, 0x10, 0x0E, 0xA6, 0x35, 
	0xA5, 0x27, 0x09, 0x20, 0x9D, 0x00, 0x02, 0x85, 0x27, 0xAE, 0xF8, 0x07, 0x68, 0x29, 0xBF, 0x9D, 
	0xB8, 0x04, 0x28, 0xF0, 0x06, 0x20, 0x63, 0xCB, 0x4C, 0xB5, 0xC8, 0x4C, 0xFC, 0xC8, 0x20, 0x00, 
	0xC8, 0xA2, 0x00, 0x60, 0x4C, 0x9B, 0xC8, 0x4C, 0xAA, 0xC9, 0x4A, 0x20, 0x9B, 0xC9, 0xB0, 0x08, 
	0x20, 0xF5, 0xCA, 0xF0, 0x06, 0x18, 0x90, 0x03, 0x20, 0xD2, 0xCA, 0xBD, 0xB8, 0x05, 0xAA, 0x60, 
	0xA2, 0x03, 0xB5, 0x36, 0x48, 0xCA, 0x10, 0xFA, 0xAE, 0xF8, 0x07, 0xBD, 0x38, 0x06, 0x85, 0x36, 
	0xBD, 0xB8, 0x04, 0x29, 0x38, 0x4A, 0x4A, 0x4A, 0x09, 0xC0, 0x85, 0x37, 0x8A, 0x48, 0xA5, 0x27, 
	0x48, 0x09, 0x80, 0x20, 0xED, 0xFD, 0x68, 0x85, 0x27, 0x68, 0x8D, 0xF8, 0x07, 0xAA, 0x0A, 0x0A, 
	0x0A, 0x0A, 0x85, 0x26, 0x8D, 0xFF, 0xCF, 0xA5, 0x36, 0x9D, 0x38, 0x06, 0xA2, 0x00, 0x68, 0x95, 
	0x36, 0xE8, 0xE0, 0x04, 0x90, 0xF8, 0xAE, 0xF8, 0x07, 0x60, 0xC1, 0xD0, 0xD0, 0xCC, 0xC5, 0x08
};

//This looks identical to diskROM
uint8_t slot6e[0x100] = {
	0xA2, 0x20, 0xA0, 0x00, 0xA2, 0x03, 0x86, 0x3C, 0x8A, 0x0A, 0x24, 0x3C, 0xF0, 0x10, 0x05, 0x3C, 
	0x49, 0xFF, 0x29, 0x7E, 0xB0, 0x08, 0x4A, 0xD0, 0xFB, 0x98, 0x9D, 0x56, 0x03, 0xC8, 0xE8, 0x10, 
	0xE5, 0x20, 0x58, 0xFF, 0xBA, 0xBD, 0x00, 0x01, 0x0A, 0x0A, 0x0A, 0x0A, 0x85, 0x2B, 0xAA, 0xBD, 
	0x8E, 0xC0, 0xBD, 0x8C, 0xC0, 0xBD, 0x8A, 0xC0, 0xBD, 0x89, 0xC0, 0xA0, 0x50, 0xBD, 0x80, 0xC0, 
	0x98, 0x29, 0x03, 0x0A, 0x05, 0x2B, 0xAA, 0xBD, 0x81, 0xC0, 0xA9, 0x56, 0x20, 0xA8, 0xFC, 0x88, 
	0x10, 0xEB, 0x85, 0x26, 0x85, 0x3D, 0x85, 0x41, 0xA9, 0x08, 0x85, 0x27, 0x18, 0x08, 0xBD, 0x8C, 
	0xC0, 0x10, 0xFB, 0x49, 0xD5, 0xD0, 0xF7, 0xBD, 0x8C, 0xC0, 0x10, 0xFB, 0xC9, 0xAA, 0xD0, 0xF3, 
	0xEA, 0xBD, 0x8C, 0xC0, 0x10, 0xFB, 0xC9, 0x96, 0xF0, 0x09, 0x28, 0x90, 0xDF, 0x49, 0xAD, 0xF0, 
	0x25, 0xD0, 0xD9, 0xA0, 0x03, 0x85, 0x40, 0xBD, 0x8C, 0xC0, 0x10, 0xFB, 0x2A, 0x85, 0x3C, 0xBD, 
	0x8C, 0xC0, 0x10, 0xFB, 0x25, 0x3C, 0x88, 0xD0, 0xEC, 0x28, 0xC5, 0x3D, 0xD0, 0xBE, 0xA5, 0x40, 
	0xC5, 0x41, 0xD0, 0xB8, 0xB0, 0xB7, 0xA0, 0x56, 0x84, 0x3C, 0xBC, 0x8C, 0xC0, 0x10, 0xFB, 0x59, 
	0xD6, 0x02, 0xA4, 0x3C, 0x88, 0x99, 0x00, 0x03, 0xD0, 0xEE, 0x84, 0x3C, 0xBC, 0x8C, 0xC0, 0x10, 
	0xFB, 0x59, 0xD6, 0x02, 0xA4, 0x3C, 0x91, 0x26, 0xC8, 0xD0, 0xEF, 0xBC, 0x8C, 0xC0, 0x10, 0xFB, 
	0x59, 0xD6, 0x02, 0xD0, 0x87, 0xA0, 0x00, 0xA2, 0x56, 0xCA, 0x30, 0xFB, 0xB1, 0x26, 0x5E, 0x00, 
	0x03, 0x2A, 0x5E, 0x00, 0x03, 0x2A, 0x91, 0x26, 0xC8, 0xD0, 0xEE, 0xE6, 0x27, 0xE6, 0x3D, 0xA5, 
	0x3D, 0xCD, 0x00, 0x08, 0xA6, 0x2B, 0x90, 0xDB, 0x4C, 0x01, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00
};

// Various firmware from the IIc

uint8_t slot1[0x100] = {
	0x2C, 0x89, 0xC1, 0x70, 0x0C, 0x38, 0x90, 0x18, 0xB8, 0x50, 0x06, 0x01, 0x31, 0x9E, 0xA8, 0xB4, 
	0xBB, 0xDA, 0xA2, 0xC1, 0x4C, 0x1C, 0xC2, 0x90, 0x03, 0x4C, 0xE5, 0xC7, 0x0A, 0x7A, 0x5A, 0xBD, 
	0xB8, 0x04, 0xF0, 0x42, 0xA5, 0x24, 0xB0, 0x1C, 0xDD, 0xB8, 0x04, 0x90, 0x03, 0xBD, 0x38, 0x07, 
	0xDD, 0x38, 0x07, 0xB0, 0x0B, 0xC9, 0x11, 0xB0, 0x11, 0x09, 0xF0, 0x3D, 0x38, 0x07, 0x65, 0x24, 
	0x85, 0x24, 0x80, 0x06, 0xC5, 0x21, 0x90, 0x02, 0x64, 0x24, 0x7A, 0x5A, 0xBD, 0x38, 0x07, 0xDD, 
	0xB8, 0x04, 0xB0, 0x08, 0xC5, 0x24, 0xB0, 0x0E, 0xA9, 0x40, 0x80, 0x02, 0xA9, 0x1A, 0xC0, 0x80, 
	0x6A, 0x20, 0x9B, 0xC1, 0x80, 0xE4, 0x98, 0x20, 0x8A, 0xC1, 0xBD, 0xB8, 0x04, 0xF0, 0x17, 0x3C, 
	0xB8, 0x06, 0x30, 0x12, 0xBD, 0x38, 0x07, 0xFD, 0xB8, 0x04, 0xC9, 0xF8, 0x90, 0x04, 0x18, 0x65, 
	0x21, 0xAC, 0xA9, 0x00, 0x85, 0x24, 0x68, 0x7A, 0xFA, 0x60, 0x20, 0xA9, 0xC7, 0x90, 0xFA, 0x3C, 
	0xB8, 0x06, 0x10, 0x07, 0xC9, 0x91, 0xF0, 0x03, 0x20, 0xF0, 0xFD, 0x4C, 0xCD, 0xC7, 0x5A, 0x48, 
	0x20, 0xB6, 0xC2, 0x9E, 0xB8, 0x06, 0x80, 0x07, 0x5A, 0x20, 0xD9, 0xC7, 0x90, 0xFA, 0x90, 0x68, 
	0x7A, 0xA2, 0x00, 0x60, 0x5A, 0x48, 0x20, 0x8A, 0xC1, 0x80, 0xF4, 0x5A, 0x48, 0x4A, 0xD0, 0x15, 
	0x08, 0x20, 0xD3, 0xC7, 0x28, 0x90, 0x05, 0x29, 0x28, 0x0A, 0x80, 0x02, 0x29, 0x30, 0xC9, 0x10, 
	0xF0, 0xDD, 0x18, 0x80, 0xDA, 0xA2, 0x40, 0x68, 0x7A, 0x18, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

uint8_t slot2[0x100] = {
	0x2C, 0x89, 0xC1, 0x70, 0x14, 0x38, 0x90, 0x18, 0xB8, 0x50, 0x0E, 0x01, 0x31, 0x11, 0x13, 0x15, 
	0x17, 0x80, 0x8B, 0x80, 0x93, 0x80, 0x9D, 0x80, 0xA2, 0xDA, 0xA2, 0xC2, 0x5A, 0x48, 0x8E, 0xF8, 
	0x07, 0x50, 0x22, 0xA5, 0x36, 0x45, 0x38, 0xF0, 0x06, 0xA5, 0x37, 0xC5, 0x39, 0xF0, 0x03, 0x20, 
	0xB6, 0xC2, 0x8A, 0x45, 0x39, 0x05, 0x38, 0xD0, 0x07, 0xA9, 0x05, 0x85, 0x38, 0x38, 0x80, 0x05, 
	0xA9, 0x07, 0x85, 0x36, 0x18, 0xBD, 0xB8, 0x06, 0x89, 0x01, 0xD0, 0x03, 0x4C, 0x17, 0xC1, 0x90, 
	0xFB, 0x68, 0x80, 0x28, 0x3C, 0xB8, 0x03, 0x50, 0x1C, 0x20, 0x8F, 0xC1, 0x80, 0x1E, 0x68, 0x20, 
	0x70, 0xCC, 0x10, 0x1B, 0x20, 0xA9, 0xC7, 0xB0, 0xEB, 0x29, 0x5F, 0xC9, 0x51, 0xF0, 0x04, 0xC9, 
	0x52, 0xD0, 0x09, 0xA9, 0x98, 0x7A, 0xFA, 0x60, 0x18, 0x20, 0xA3, 0xC7, 0x20, 0x4C, 0xCC, 0x48, 
	0x20, 0xD9, 0xC7, 0xB0, 0x09, 0xBD, 0xB8, 0x06, 0x29, 0x10, 0xF0, 0xD2, 0x80, 0xF2, 0xA8, 0x68, 
	0x5A, 0x20, 0xB8, 0xC3, 0x68, 0xBC, 0x38, 0x06, 0xF0, 0x12, 0x09, 0x80, 0xC9, 0x91, 0xF0, 0xDC, 
	0xC9, 0xFF, 0xF0, 0xD8, 0xC9, 0x92, 0xF0, 0xD0, 0xC9, 0x94, 0xF0, 0xCD, 0x3C, 0xB8, 0x03, 0x50, 
	0xC4, 0x20, 0xED, 0xFD, 0x80, 0xC6, 0x20, 0x9A, 0xCF, 0xBC, 0x29, 0xC2, 0x20, 0x7C, 0xC3, 0x48, 
	0x88, 0x30, 0x04, 0xC0, 0x03, 0xD0, 0xF5, 0x20, 0x9A, 0xCF, 0x68, 0xBC, 0x2B, 0xC2, 0x99, 0xFB, 
	0xBF, 0x68, 0x99, 0xFA, 0xBF, 0x68, 0x9D, 0xB8, 0x06, 0x29, 0x01, 0xD0, 0x02, 0xA9, 0x09, 0x9D, 
	0x38, 0x06, 0x68, 0x9D, 0xB8, 0x04, 0x9E, 0xB8, 0x03, 0x60, 0x03, 0x07, 0xA0, 0xB0, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

uint8_t slot3[0x100] = {
	0x48, 0xDA, 0x5A, 0x80, 0x12, 0x38, 0x90, 0x18, 0x80, 0x1A, 0xEA, 0x01, 0x88, 0x2C, 0x2F, 0x32, 
	0x35, 0x4C, 0xAF, 0xC7, 0x4C, 0xB5, 0xC7, 0x20, 0x20, 0xCE, 0x20, 0xBE, 0xCD, 0x20, 0x58, 0xFC, 
	0x7A, 0xFA, 0x68, 0x18, 0xB0, 0x03, 0x4C, 0xF6, 0xFD, 0x4C, 0x1B, 0xFD, 0x4C, 0x41, 0xCF, 0x4C, 
	0x35, 0xCF, 0x4C, 0xC2, 0xCE, 0x4C, 0xB1, 0xCE, 0xA9, 0x06, 0xCD, 0xB3, 0xFB, 0xF0, 0x3C, 0x20, 
	0x60, 0xC3, 0xA9, 0xF8, 0x85, 0x37, 0x64, 0x36, 0xB2, 0x36, 0x92, 0x36, 0xE6, 0x36, 0xD0, 0xF8, 
	0xE6, 0x37, 0xD0, 0xF4, 0xDA, 0xAE, 0x78, 0x04, 0x3C, 0x81, 0xC0, 0x3C, 0x81, 0xC0, 0xFA, 0x60, 
	0xDA, 0xA2, 0x00, 0x2C, 0x11, 0xC0, 0x30, 0x02, 0xA2, 0x08, 0x2C, 0x12, 0xC0, 0x10, 0x02, 0xE8, 
	0xE8, 0x2C, 0x81, 0xC0, 0x2C, 0x81, 0xC0, 0x8E, 0x78, 0x04, 0xFA, 0x60, 0xAD, 0x13, 0xC0, 0x0A, 
	0xAD, 0x18, 0xC0, 0x08, 0x8D, 0x00, 0xC0, 0x8D, 0x03, 0xC0, 0xB9, 0x78, 0x04, 0x28, 0xB0, 0x03, 
	0x8D, 0x02, 0xC0, 0x10, 0x03, 0x8D, 0x01, 0xC0, 0x60, 0x09, 0x80, 0xC9, 0xFB, 0xB0, 0x06, 0xC9, 
	0xE1, 0x90, 0x02, 0x29, 0xDF, 0x60, 0x48, 0xA9, 0x08, 0x1C, 0xFB, 0x04, 0x68, 0x20, 0xED, 0xFD, 
	0x4C, 0x44, 0xFD, 0x20, 0x9D, 0xCC, 0x80, 0x09, 0x20, 0x9D, 0xCC, 0x24, 0x32, 0x30, 0x02, 0x29, 
	0x7F, 0x5A, 0x09, 0x00, 0x30, 0x15, 0x48, 0xAD, 0xFB, 0x04, 0x6A, 0x68, 0x90, 0x0D, 0x2C, 0x1E, 
	0xC0, 0x10, 0x08, 0x49, 0x40, 0x89, 0x60, 0xF0, 0x02, 0x49, 0x40, 0x2C, 0x1F, 0xC0, 0x10, 0x19, 
	0x48, 0x8D, 0x01, 0xC0, 0x98, 0x45, 0x20, 0x4A, 0xB0, 0x04, 0xAD, 0x55, 0xC0, 0xC8, 0x98, 0x4A, 
	0xA8, 0x68, 0x91, 0x28, 0x2C, 0x54, 0xC0, 0x7A, 0x60, 0x91, 0x28, 0x7A, 0x60, 0x00, 0x00, 0x00
};

uint8_t slot4[0x100] = {
	0x80, 0x05, 0xA2, 0x03, 0x60, 0x38, 0x90, 0x18, 0x4C, 0xCF, 0xC5, 0x01, 0x20, 0x02, 0x02, 0x02, 
	0x02, 0x00, 0x3B, 0xDC, 0x93, 0x82, 0x69, 0xBD, 0x6B, 0x1A, 0x9C, 0x7C, 0x07, 0xA2, 0x80, 0xA0, 
	0x01, 0x9E, 0x7D, 0x04, 0x9E, 0x7D, 0x05, 0xA9, 0xFF, 0x9D, 0x7D, 0x06, 0xA9, 0x03, 0x9D, 0x7D, 
	0x07, 0xA2, 0x00, 0x88, 0x10, 0xEB, 0x20, 0x6B, 0xC4, 0xA9, 0x00, 0xAA, 0x20, 0x9A, 0xCF, 0x8A, 
	0x8D, 0x78, 0x04, 0x4A, 0x0D, 0x78, 0x04, 0xC9, 0x10, 0xB0, 0x1F, 0x29, 0x05, 0xF0, 0x01, 0x58, 
	0x69, 0x55, 0x08, 0x78, 0x8E, 0xFC, 0x07, 0x8D, 0x79, 0xC0, 0xA2, 0x08, 0xCA, 0x0A, 0x90, 0x03, 
	0x9D, 0x58, 0xC0, 0xD0, 0xF7, 0x8D, 0x78, 0xC0, 0x28, 0x18, 0x60, 0xA2, 0x80, 0x80, 0x02, 0xA2, 
	0x00, 0xBD, 0x7D, 0x04, 0x9D, 0x7C, 0x04, 0xBD, 0x7D, 0x05, 0x9D, 0x7C, 0x05, 0xCA, 0x10, 0xEF, 
	0x80, 0x0C, 0x9C, 0x7C, 0x04, 0x9C, 0x7C, 0x05, 0x9C, 0xFC, 0x04, 0x9C, 0xFC, 0x05, 0x9C, 0x7C, 
	0x06, 0x18, 0x60, 0xA9, 0x20, 0x1C, 0x7C, 0x07, 0x2D, 0x7C, 0x06, 0x1C, 0x7C, 0x06, 0x2C, 0xFC, 
	0x07, 0x30, 0x13, 0x2C, 0x63, 0xC0, 0x30, 0x02, 0x09, 0x80, 0x2C, 0x7C, 0x07, 0x10, 0x02, 0x09, 
	0x40, 0x8D, 0x7C, 0x07, 0x18, 0x60, 0x0D, 0x7C, 0x07, 0x29, 0xE0, 0x80, 0xF4, 0x6A, 0x6A, 0x29, 
	0x80, 0xAA, 0xAD, 0x78, 0x04, 0x9D, 0x7D, 0x04, 0xAD, 0x78, 0x05, 0x9D, 0x7D, 0x05, 0xAD, 0xF8, 
	0x04, 0x9D, 0x7D, 0x06, 0xAD, 0xF8, 0x05, 0x9D, 0x7D, 0x07, 0x18, 0x60, 0x48, 0x18, 0xA9, 0x0E, 
	0x2D, 0x7C, 0x07, 0xD0, 0x01, 0x38, 0x68, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xD6, 0x00, 0x00, 0x00, 0x00
};

uint8_t slot5[0x100] = {
	0xA2, 0x20, 0xA2, 0x00, 0xA2, 0x03, 0xC9, 0x00, 0xB0, 0x17, 0x38, 0xB0, 0x01, 0x18, 0xA2, 0x05, 
	0x7E, 0x73, 0x04, 0x18, 0xA2, 0xC5, 0x8E, 0xF8, 0x07, 0xA2, 0x05, 0xAD, 0xFF, 0xCF, 0x4C, 0x97, 
	0xC7, 0xA2, 0x05, 0x86, 0x58, 0xA9, 0xC5, 0x8D, 0xF8, 0x07, 0x20, 0x76, 0xC5, 0xA0, 0x05, 0xB9, 
	0x70, 0xC5, 0x99, 0x42, 0x00, 0x88, 0x10, 0xF7, 0x20, 0x0A, 0xC5, 0xB0, 0x15, 0xAE, 0x00, 0x08, 
	0xCA, 0xD0, 0x0F, 0xAE, 0x01, 0x08, 0xF0, 0x0A, 0xA5, 0x58, 0x0A, 0x0A, 0x0A, 0x0A, 0xAA, 0x4C, 
	0x01, 0x08, 0xA2, 0x10, 0xBD, 0x5F, 0xC5, 0x9D, 0xDB, 0x07, 0xCA, 0x10, 0xF7, 0x80, 0xFE, 0xC3, 
	0xE8, 0xE5, 0xE3, 0xEB, 0xA0, 0xC4, 0xE9, 0xF3, 0xEB, 0xA0, 0xC4, 0xF2, 0xE9, 0xF6, 0xE5, 0xAE, 
	0x01, 0x50, 0x00, 0x08, 0x00, 0x00, 0xA2, 0x08, 0xBD, 0x83, 0xC5, 0x95, 0x00, 0xCA, 0x10, 0xF8, 
	0x4C, 0x00, 0x00, 0x20, 0x0D, 0xC5, 0x05, 0x07, 0x00, 0x60, 0x01, 0x00, 0x00, 0x00, 0xA2, 0x03, 
	0xA0, 0x00, 0x86, 0x3C, 0x8A, 0x0A, 0x24, 0x3C, 0xF0, 0x10, 0x05, 0x3C, 0x49, 0xFF, 0x29, 0x7E, 
	0xB0, 0x08, 0x4A, 0xD0, 0xFB, 0x98, 0x9D, 0x56, 0x03, 0xC8, 0xE8, 0x10, 0xE5, 0xA9, 0x08, 0x85, 
	0x27, 0xA0, 0x7F, 0x60, 0xB9, 0x00, 0x02, 0xC8, 0x4C, 0x99, 0xC3, 0xC1, 0xF0, 0xF0, 0xEC, 0xE5, 
	0xA0, 0xAF, 0xAF, 0xE3, 0x20, 0xD0, 0xF8, 0x20, 0x53, 0xF9, 0x85, 0x3A, 0x84, 0x3B, 0x60, 0x5A, 
	0xB0, 0x1C, 0xA0, 0xC4, 0xC4, 0x39, 0xD0, 0x04, 0xA4, 0x38, 0xF0, 0x12, 0xDA, 0x48, 0x29, 0x7F, 
	0xC9, 0x02, 0xB0, 0x06, 0x20, 0x3B, 0xC4, 0x20, 0x6B, 0xC4, 0x68, 0xFA, 0x7A, 0x60, 0x4C, 0x9D, 
	0xC7, 0x00, 0x00, 0x00, 0x00, 0x4C, 0x52, 0xC5, 0x4C, 0x76, 0xC5, 0x00, 0x00, 0x00, 0xBF, 0x0A
};

uint8_t slot6[0x100] = {
	0xA2, 0x20, 0xA0, 0x00, 0x64, 0x03, 0x64, 0x3C, 0xA9, 0x60, 0xAA, 0x86, 0x2B, 0x85, 0x4F, 0x5A, 
	0xBD, 0x8E, 0xC0, 0xBD, 0x8C, 0xC0, 0x7A, 0xB9, 0xEA, 0xC0, 0xBD, 0x89, 0xC0, 0xA0, 0x50, 0xBD, 
	0x80, 0xC0, 0x98, 0x29, 0x03, 0x0A, 0x05, 0x2B, 0xAA, 0xBD, 0x81, 0xC0, 0xA9, 0x56, 0x20, 0xA8, 
	0xFC, 0x88, 0x10, 0xEB, 0x85, 0x26, 0x85, 0x3D, 0x85, 0x41, 0x20, 0x8E, 0xC5, 0x64, 0x03, 0x18, 
	0x08, 0x28, 0xA6, 0x2B, 0xC6, 0x03, 0xD0, 0x0E, 0xBD, 0x88, 0xC0, 0xA5, 0x01, 0xC9, 0xC6, 0xD0, 
	0xA4, 0x4C, 0x00, 0xC5, 0x00, 0x00, 0x08, 0x88, 0xD0, 0x04, 0xF0, 0xE5, 0x80, 0xDF, 0xBD, 0x8C, 
	0xC0, 0x10, 0xFB, 0x49, 0xD5, 0xD0, 0xF0, 0xBD, 0x8C, 0xC0, 0x10, 0xFB, 0xC9, 0xAA, 0xD0, 0xF3, 
	0xEA, 0xBD, 0x8C, 0xC0, 0x10, 0xFB, 0xC9, 0x96, 0xF0, 0x09, 0x28, 0x90, 0xC2, 0x49, 0xAD, 0xF0, 
	0x25, 0xD0, 0xBC, 0xA0, 0x03, 0x85, 0x40, 0xBD, 0x8C, 0xC0, 0x10, 0xFB, 0x2A, 0x85, 0x3C, 0xBD, 
	0x8C, 0xC0, 0x10, 0xFB, 0x25, 0x3C, 0x88, 0xD0, 0xEC, 0x28, 0xC5, 0x3D, 0xD0, 0xA1, 0xA5, 0x40, 
	0xC5, 0x41, 0xD0, 0x9B, 0xB0, 0x9C, 0xA0, 0x56, 0x84, 0x3C, 0xBC, 0x8C, 0xC0, 0x10, 0xFB, 0x59, 
	0xD6, 0x02, 0xA4, 0x3C, 0x88, 0x99, 0x00, 0x03, 0xD0, 0xEE, 0x84, 0x3C, 0xBC, 0x8C, 0xC0, 0x10, 
	0xFB, 0x59, 0xD6, 0x02, 0xA4, 0x3C, 0x91, 0x26, 0xC8, 0xD0, 0xEF, 0xBC, 0x8C, 0xC0, 0x10, 0xFB, 
	0x59, 0xD6, 0x02, 0xD0, 0xCD, 0xA0, 0x00, 0xA2, 0x56, 0xCA, 0x30, 0xFB, 0xB1, 0x26, 0x5E, 0x00, 
	0x03, 0x2A, 0x5E, 0x00, 0x03, 0x2A, 0x91, 0x26, 0xC8, 0xD0, 0xEE, 0xE6, 0x27, 0xE6, 0x3D, 0xA5, 
	0x3D, 0xCD, 0x00, 0x08, 0xA6, 0x4F, 0x90, 0xDB, 0x4C, 0x01, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00
};

uint8_t slot7[0x100] = {
	0x2C, 0x03, 0xC7, 0x70, 0x1B, 0x38, 0x90, 0x18, 0xB8, 0x50, 0x15, 0x01, 0x9B, 0x1C, 0x1C, 0x1C, 
	0x1C, 0x88, 0x18, 0x80, 0x2A, 0x38, 0x78, 0xA2, 0xFF, 0x9A, 0x80, 0x26, 0x38, 0xA2, 0x03, 0x60, 
	0x8D, 0x78, 0x04, 0xA9, 0xC7, 0x8D, 0xF8, 0x07, 0x08, 0xC5, 0x39, 0xF0, 0xE8, 0x28, 0x4D, 0x7F, 
	0x04, 0xD0, 0x1A, 0xAD, 0xFF, 0x07, 0x48, 0xAD, 0x7F, 0x07, 0x48, 0xAD, 0x78, 0x04, 0x60, 0x8D, 
	0xF8, 0x07, 0x20, 0xD3, 0xC7, 0x70, 0x01, 0x60, 0xB0, 0x03, 0x6C, 0xCB, 0x00, 0xAD, 0x81, 0xC0, 
	0xAD, 0x81, 0xC0, 0x20, 0x84, 0xFE, 0x20, 0x2F, 0xFB, 0x20, 0x58, 0xFC, 0x20, 0x93, 0xFE, 0x20, 
	0x89, 0xFE, 0xA0, 0x10, 0xB9, 0x6F, 0xC7, 0x99, 0xDB, 0x07, 0x88, 0x10, 0xF7, 0x80, 0xFE, 0xC1, 
	0xF0, 0xF0, 0xEC, 0xE5, 0xD4, 0xE1, 0xEC, 0xEB, 0xA0, 0xCF, 0xE6, 0xE6, 0xEC, 0xE9, 0xEE, 0xE5, 
	0x8D, 0x28, 0xC0, 0x40, 0x8D, 0x28, 0xC0, 0x60, 0x8D, 0x28, 0xC0, 0x4C, 0x62, 0xFA, 0x8D, 0x28, 
	0xC0, 0x2C, 0x87, 0xC7, 0x4C, 0x04, 0xC8, 0x8D, 0x28, 0xC0, 0x4C, 0xF1, 0xC7, 0x8D, 0x28, 0xC0, 
	0x4C, 0xF6, 0xC7, 0x8D, 0x28, 0xC0, 0x4C, 0xF1, 0xC7, 0x8D, 0x28, 0xC0, 0x4C, 0x06, 0xC8, 0x8D, 
	0x28, 0xC0, 0x4C, 0x4E, 0xC3, 0x8D, 0x28, 0xC0, 0x4C, 0x97, 0xC3, 0x8D, 0x28, 0xC0, 0x4C, 0x00, 
	0xC1, 0x8D, 0x28, 0xC0, 0x4C, 0xA9, 0xD4, 0x8D, 0x28, 0xC0, 0x4C, 0x80, 0xC5, 0x8D, 0x28, 0xC0, 
	0x4C, 0x4F, 0xC2, 0x8D, 0x28, 0xC0, 0x4C, 0xAC, 0xC2, 0x8D, 0x28, 0xC0, 0x4C, 0xC3, 0xC2, 0x8D, 
	0x28, 0xC0, 0x4C, 0xF7, 0xC2, 0x8D, 0x28, 0xC0, 0x4C, 0xE0, 0xD4, 0x8D, 0x28, 0xC0, 0x6C, 0xED, 
	0x03, 0x20, 0x23, 0xCE, 0x80, 0x8E, 0x20, 0x4D, 0xCE, 0x80, 0x89, 0x00, 0x00, 0x00, 0x00, 0x00
};
