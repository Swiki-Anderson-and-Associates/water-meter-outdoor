/*
 * XbeePro.cpp
 *
 *  Created on: Jun 4, 2014
 *      Author: Andrew
 */

#include "XbeePro.h"

XbeePro::~XbeePro() {
	// TODO Auto-generated destructor stub
}

XbeePro::XbeePro() {
	// TODO Auto-generated constructor stub

}

void XbeePro::BootloaderBypass()
{
	char BootloaderBuffer[72];
	// Connect to serial port

	// Send carriage return until the arduino receives the bootloader menu
	// Compare input to bootloader menu, expected bootloader menu:
	// B-Bypass Mode
	// F-Update App
	// T-Timeout
	// V-BL Version
	// A-App Version
	// R-Reset
	// "B-Bypass Mode\rF-Update App\rT-Timeout\rV-BL Version\rA-App Version\rR-Reset\r" which is 72 bytes long
	Serial.setTimeout(250);
	Serial.println("");
	while(Serial.readBytesUntil(0x7A,BootloaderBuffer,72) != 72)
	{
		// This loop assumes that if it receives 72 bytes, then that is the bootloader menu.
		Serial.println("");
	}
	Serial.print("B");

	// The radio should then be ready to receive API frames
	// sfakljskljfdsalkjfdasljf;a
}
