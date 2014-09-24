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
	void PayloadCreator(uint8_t,uint8_t);
	void ApiTxRequest();
private:
	uint8_t Payload[66];
	uint8_t PayloadSize;
};

#endif /* XBEEPRO_H_ */
