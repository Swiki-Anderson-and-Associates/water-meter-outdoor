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
	PayloadSize = 0;
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
}

void XbeePro::PayloadCreator(uint8_t DataByte, uint8_t Option)
{
	if (Option == 1)
	{
		// Write to payload and increment size
		Payload[PayloadSize] = DataByte;
		PayloadSize++;
	}
	else	// Just input option as 2
	{
		// Payload array does not need to be rewritten, because that will happen as this function
		//	is used and extra crap at the end will be ignored on the next transmission.
		PayloadSize = 0;
	}
}

void XbeePro::ApiTxRequest()
{
	// SH + SL Address of receiving XBee
	XBeeAddress64 addr64 = XBeeAddress64(0x00000000, 0x00000000);
	ZBTxRequest zbTx = ZBTxRequest(addr64, Payload, PayloadSize);
	ZBTxStatusResponse txStatus = ZBTxStatusResponse();

	// Message transmit
	send(zbTx);
	// Clear buffer & size
	PayloadSize = 0;
}
