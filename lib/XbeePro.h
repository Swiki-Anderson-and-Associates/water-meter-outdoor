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
};

XbeePro::~XbeePro() {
	// TODO Auto-generated destructor stub
}

XbeePro::XbeePro() {
	// TODO Auto-generated constructor stub

}

void XbeePro::BootloaderBypass()
{

}

#endif /* XBEEPRO_H_ */
