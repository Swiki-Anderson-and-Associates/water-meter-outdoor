/*
 * XbeePro.h
 *
 *  Created on: Jun 4, 2014
 *      Author: Andrew
 */

#ifndef XBEEPRO_H_
#define XBEEPRO_H_

#include "XBee.h"

class XbeePro: public XBee {
public:
	virtual ~XbeePro();
	XbeePro();
	void BootloaderBypass();
	void PayloadCreator();
	void ApiTxRequest();
private:
	uint8_t Payload[66];
	uint8_t PayloadSize;
};

XbeePro::~XbeePro() {
	// TODO Auto-generated destructor stub
}

XbeePro::XbeePro() {
	// TODO Auto-generated constructor stub
	PayloadSize = 0;
}

void XbeePro::BootloaderBypass() {

}

void XbeePro::PayloadCreator() {

}

void XbeePro::TxRequest() {

}

#endif /* XBEEPRO_H_ */
