#include <Arduino.h>
#include <avr/sleep.h>
#include <stdio.h>
#include <SD.h>
#include <EEPROM.h>
#include "ds3234.h"
#include "LowPower.h"
#include "XbeePro.h"

// Define Constants
#define LOG_START_POS		16			// memory position where gallon log starts
#define DEBOUNCE_MS			100			// time constant for debouncing in milliseconds

// Define Pins Used for Operation
#define RADIO_RX_PIN		0			// radio Rx pin
#define RADIO_TX_PIN		1			// radio Tx pin

#define RADIO_SLEEP_PIN     14			// (A0) pin pulled low to wake radio from sleep
#define RADIO_RTS_PIN       15			// (A1) pin pulled high to prevent the radio from transferring data
#define RADIO_CTS_PIN       16			// (A2) pin pulled high by radio to tell the Arduino to stop sending data

#define DS3234_SS_PIN		10			// pin pulled low to allow SPI communication with DS3234 RTC
#define SD_SS_PIN			4			// pin pulled low to allow SPI communication with SD Card
#define MOSI_PIN			11			// SPI MOSI communication pin
#define MISO_PIN			12			// SPI MISO communication pin
#define SPI_CLK_PIN			13			// SPI clock pin

#define ALARM_PIN			2			// pin pulled low when Arduino is woken by the radio
#define METER_PIN			3			// pin pulled low when one gallon has flowed through meter
#define RST_PIN				6			// user reset pin, pulled low to reset

#define VALVE_ENABLE_PIN	7			// pin must be pulled high to enable h bridge controller
#define VALVE_CONTROL_1_PIN 8			// polarity of valve control pins must be reversed to open or close valve
#define VALVE_CONTROL_2_PIN 9			// see above

#define ADD_BYTE			1			// Second parameter for PayloadCreator, adds the byte
#define CLEAR_PAY			2			// Second parameter for PayloadCreator, clears the payload

// Define Enumerations
enum interruptType {NONE, RADIO, METER};
enum SPIType {RTC, SDCard};

// Define Global Variables
File logFile;
static char MessageBuffer[256];
uint8_t leak, timerCount;
uint32_t meterIntTime, lastMeterIntTime;
volatile interruptType lastInt;			// any variables changed by ISRs must be declared volatile
SPIType SPIFunc;
bool isBounce;
XbeePro xbee;

// Define Program Function
static uint8_t openLogFile()						// TODO: set this up to create new logs every month
{
	if(!SD.begin(4))
	{
		return 1;		// SD card error
	}
	logFile = SD.open("log.txt",FILE_WRITE);
	if(logFile)
	{
		return 0;
	}
	else
	{
		return 2;		// file open error
	}
}

static void closeLogFile()
{
	logFile.close();
}

static uint8_t useSDCard()
{
	if(SPIFunc == SDCard)
	{
		return 0;
	}
	else
	{
		DS3234_end();
		SPIFunc = SDCard;
		return openLogFile();
	}
}

static uint8_t useRTC()
{
	if(SPIFunc == RTC)
	{
		return 0;
	}
	else
	{
		closeLogFile();
		SPIFunc = RTC;
		DS3234_init(DS3234_SS_PIN);;
		return 0;
	}
}

static void wakeRadio()
{
	uint32_t tStart = millis();
	digitalWrite(RADIO_SLEEP_PIN,LOW);
	while (digitalRead(RADIO_CTS_PIN))					// wait till radio wakes
	{
		if (millis()-tStart > 1000)
			{
				break;									// error connecting to radio
			}
	}
	delay(1);
}

static void sleepRadio()
{
	digitalWrite(RADIO_SLEEP_PIN,HIGH);
}

static void cycleRadio()
{
	sleepRadio();
	delay(1);
	wakeRadio();
}

/*
static uint8_t printSerial()
{
	if (digitalRead(RADIO_CTS_PIN))
	{
		cycleRadio();
	}
	return Serial.print(MessageBuffer);
}


static void printTime()
{
	// Is this function even needed in the next version?
	useRTC();
	ts time;
	DS3234_get(DS3234_SS_PIN,&time);
	sprintf(MessageBuffer,"%02u/%02u/%4d %02d:%02d:%02d\t",time.mon,time.mday,time.year,time.hour,time.min,time.sec);
	printSerial();
}
*/

static void flushSerial()
{
	if (digitalRead(RADIO_CTS_PIN))
	{
		cycleRadio();
	}
	Serial.flush();
}

static void setValvePos(uint8_t pos)
{
	EEPROM.write(0,pos);
}

static void setLeakCondition(uint8_t cond)
{
	EEPROM.write(1,cond);
}

static uint8_t isValveOpen()
{
	return EEPROM.read(0);
}

static uint8_t wasLeakDetected()
{
	return EEPROM.read(1);
}

static uint8_t getLastLogPos()
{
	return EEPROM.read(2);
}

static void closeValve()
{
	digitalWrite(VALVE_ENABLE_PIN,1);
	digitalWrite(VALVE_CONTROL_1_PIN,0);
	digitalWrite(VALVE_CONTROL_2_PIN,1);
	delay(5000);
	setValvePos(0);
	digitalWrite(VALVE_ENABLE_PIN,0);
	digitalWrite(VALVE_CONTROL_2_PIN,0);
	printTime();
	// sprintf(MessageBuffer,"Valve:\tClosed\n");
	xbee.PayloadCreator(0x12,ADD_BYTE);
	RadioTime();
	xbee.ApiTxRequest();
	// return printSerial();

}

static void openValve()
{
	digitalWrite(VALVE_ENABLE_PIN,1);
	digitalWrite(VALVE_CONTROL_1_PIN,1);
	digitalWrite(VALVE_CONTROL_2_PIN,0);
	delay(5000);
	setValvePos(1);
	digitalWrite(VALVE_ENABLE_PIN,0);
	digitalWrite(VALVE_CONTROL_1_PIN,0);
	printTime();
	// sprintf(MessageBuffer,"Valve:\tOpened\n");
	xbee.PayloadCreator(0x11,ADD_BYTE);
	RadioTime();
	xbee.ApiTxRequest();
	// return printSerial();
}

static uint32_t readLogEntry(uint8_t logStart)
{
	uint32_t t_unix = 0;
	t_unix += (uint32_t)EEPROM.read(logStart)*16777216;
	t_unix += (uint32_t)EEPROM.read(logStart+1)*65536;
	t_unix += (uint32_t)EEPROM.read(logStart+2)*256;
	t_unix += (uint32_t)EEPROM.read(logStart+3);
	return t_unix;
}

static void writeLogEntry(uint8_t startPos, uint32_t t_unix)
{
	// stores t_unix as 4 bytes
	uint8_t splitByte;
	splitByte = t_unix/16777216;
	EEPROM.write(startPos,(char)splitByte);
	t_unix -= (uint32_t)(splitByte)*16777216;
	splitByte = t_unix/65536;
	EEPROM.write(startPos+1,(char)splitByte);
	t_unix -= (uint32_t)(splitByte)*65536;
	splitByte = t_unix/256;
	EEPROM.write(startPos+2,(char)splitByte);
	t_unix -= (uint32_t)(splitByte)*256;
	splitByte = t_unix;
	EEPROM.write(startPos+3,t_unix);
}

static uint16_t getDayGallons()
{
	uint16_t dayGallons = 0;
	dayGallons += (uint16_t)EEPROM.read(3)*256;
	dayGallons += (uint16_t)EEPROM.read(4);
	return dayGallons;
}

static void setDayGallons(uint16_t DayGallons)
{
	uint8_t splitByte;
	splitByte = DayGallons/256;
	EEPROM.write(3,splitByte);
	DayGallons -= (uint32_t)(splitByte)*256;
	splitByte = DayGallons;
	EEPROM.write(4,DayGallons);
}

static uint8_t getConsecGallons()
{
	return EEPROM.read(5);
}

static void setConsecGallons(uint8_t gals)
{
	EEPROM.write(5,gals);
}

static void clearLog()					// TODO: rewrite using SD card
{											// TODO: rewrite for multiple month logs
	uint8_t i;
	if (getLastLogPos()!=LOG_START_POS-1)
	{
		for(i=LOG_START_POS; i<=251; i++)
		{
			EEPROM.write(i,(char)0);
		}
		EEPROM.write(2,(uint8_t)(LOG_START_POS-1));
	}
	printTime();
	// sprintf(MessageBuffer,"Log:\tCleared\n");
	xbee.PayloadCreator(0x05,ADD_BYTE);
	RadioTime();
	xbee.ApiTxRequest();
	// return printSerial();
}

static void resetSystem()
{
	openValve();
	clearLog();
	setLeakCondition(0);
	setDayGallons(0);
	setConsecGallons(0);
	printTime();
	// sprintf(MessageBuffer,"System Reset\n");
	xbee.PayloadCreator(0x51,ADD_BYTE);
	RadioTime();
	xbee.ApiTxRequest();
	// return printSerial();
}

static void radioInterrupt()
{
	lastInt = RADIO;
}

static void meterInterrupt()
{
	lastInt = METER;
}

static void shutdown()
{
	sleep_enable();										// Dont fuck with anything below this point in this function
	attachInterrupt(0,radioInterrupt,LOW);
	attachInterrupt(1,meterInterrupt,CHANGE);
	lastInt = NONE;
	LowPower.powerDown(SLEEP_8S,ADC_OFF,BOD_OFF);
}

static void reportLog()// TODO: rewrite using SD card
{
	uint8_t lastLog = getLastLogPos();
	uint8_t i;
	printTime();
	// sprintf(MessageBuffer,"Gallon Log:\n");
	xbee.PayloadCreator(0x01,ADD_BYTE);
	// printSerial();
	if (lastLog == LOG_START_POS-1)
	{
		// sprintf(MessageBuffer,"Empty\n");
		xbee.PayloadCreator(0x03,ADD_BYTE);
		RadioTime();
		// printSerial();
	}
	else
	{
		for (i=LOG_START_POS;i<lastLog;i++)
		{
			if(i%4 == 0)
			{
				printTime();
				// What format is the log in?
				sprintf(MessageBuffer,"%u\t%lu\n",(i-12)/4,readLogEntry((i)));
				printSerial();
			}

		}
	}
	printTime();
	// sprintf(MessageBuffer,"End Log\n");
	xbee.PayloadCreator(0x02,ADD_BYTE);
	xbee.ApiTxRequest();
	// return printSerial();
}

static void logGallon()// TODO: rewrite using SD card
{
	useRTC();
	uint32_t t_unix = DS3234_get_unix();
	uint8_t lastLog;
	lastLog = getLastLogPos();

	if (lastLog>=95)
	{
		reportLog();
		clearLog();
	}

	lastLog = getLastLogPos();
	writeLogEntry(lastLog+1,t_unix);				// writes gallon to log
	EEPROM.write(2,lastLog+4);						// sets last log position
}

static uint8_t checkForLeaks()											//TODO: rewrite using Sd log
{
	uint16_t dayGallons = getDayGallons();
	uint32_t t_lastLog = readLogEntry(getLastLogPos()-3);
	uint32_t t_prevLog = readLogEntry(getLastLogPos()-7);
	uint32_t t_dayStart = readLogEntry(8);
	uint8_t prevConsMins = getConsecGallons();

	if (t_lastLog - t_dayStart >= 86400)		// full day has passed
	{
		writeLogEntry(8,t_lastLog);				// reset day start time
		setDayGallons(0);						// reset day counter
	}
	else
	{
		setDayGallons(++dayGallons);			// add gallon to daily log
		if (dayGallons >= 1000)
		{
			return 1;							// more than 1000 gallons used in one day
		}
	}

	if (t_lastLog - t_prevLog <= 60)			// check if a minute has passed since last gallon logged
	{
		setConsecGallons(++prevConsMins);		// log consecutive gallon
		if (prevConsMins >= 120)
		{
			return 2;							// flow rate of 1 GPM or greater for 120+ consecutive mins
		}
	}
	else
	{
		setConsecGallons(0);					// reset consecutive gallon counter
	}
	return 0;									// no leak detected
}

static void reportLeak()
{
	printTime();
	switch (wasLeakDetected())
	{
		case 0:
			// sprintf(MessageBuffer,"Leak:\tNo leaks detected.\n");
			xbee.PayloadCreator(0x21,ADD_BYTE);
			RadioTime();
			break;
		case 1:
			// sprintf(MessageBuffer,"Leak:\tPossible leak detected: More than 1000 gallons used in a 24 hour period.\n");
			xbee.PayloadCreator(0x23,ADD_BYTE);
			RadioTime();
			xbee.PayloadCreator(0x00,ADD_BYTE);	// Still no format for leak conditions
			break;
		case 2:
			// sprintf(MessageBuffer,"Leak:\tPossible leak detected: Flow rate >=1 GMP for 120 consecutive minutes.\n");
			xbee.PayloadCreator(0x23,ADD_BYTE);
			RadioTime();
			xbee.PayloadCreator(0x00,ADD_BYTE);	// See leak condition message format
			break;
	}
	// return printSerial();
	xbee.ApiTxRequest();	// Function type fixing after this?
}

static void clearLeak()
{
	setLeakCondition(0);
	printTime();
	// sprintf(MessageBuffer,"Leak:\tCleared\n");
	xbee.PayloadCreator(0x22,ADD_BYTE);
	RadioTime();
	xbee.ApiTxRequest();
	// return printSerial();
}

static void reportValve()
{
	printTime();
	switch (isValveOpen())
	{
	case 0:
		// sprintf(MessageBuffer,"Valve:\tClosed\n");
		xbee.PayloadCreator(0x12,ADD_BYTE);
		RadioTime();
		xbee.ApiTxRequest();
		break;
	case 1:
		// sprintf(MessageBuffer,"Valve:\tOpen\n");
		xbee.PayloadCreator(0x11,ADD_BYTE);
		RadioTime();
		xbee.ApiTxRequest();
		break;
	}
	// return printSerial();
	xbee.ApiTxRequest();
}

static void processRadio(uint8_t Signal)
{
	switch (Signal)
	{
		case 'r':
			resetSystem();
			break;
		case 'c':
			closeValve();
			break;
		case 'o':
			openValve();
			break;
		case 'l':
			reportLeak();
			break;
		case 'v':
			reportValve();
			break;
		case 'h':
			reportLog();
			break;
		case 'q':
			clearLog();
			break;
		case 'k':
			clearLeak();
			break;
		default:
			break;
	}
}

static void checkRadioCommands()
{
	delay(10);										// wait for data to be received
	while(Serial.available()>0)
	{
		processRadio(Serial.read());
	}
}

void RadioTime()
{
	useRTC();
	uint32_t t_unix = DS3234_get_unix();

	// Writes t_unix as 4 bytes to the radio buffer
	uint8_t splitByte;
	splitByte = t_unix/16777216;
	xbee.PayloadCreator(splitByte,ADD_BYTE);
	t_unix -= (uint32_t)(splitByte)*16777216;
	splitByte = t_unix/65536;
	xbee.PayloadCreator(splitByte,ADD_BYTE);
	t_unix -= (uint32_t)(splitByte)*65536;
	splitByte = t_unix/256;
	xbee.PayloadCreator(splitByte,ADD_BYTE);
	t_unix -= (uint32_t)(splitByte)*256;
	splitByte = t_unix;
	xbee.PayloadCreator(splitByte,ADD_BYTE);
}

// Runtime functions
void setup()
{
	// Initialize Pins
	pinMode(VALVE_ENABLE_PIN,OUTPUT);
	pinMode(VALVE_CONTROL_1_PIN,OUTPUT);
	pinMode(VALVE_CONTROL_2_PIN,OUTPUT);
	pinMode(SD_SS_PIN,OUTPUT);
	pinMode(DS3234_SS_PIN,OUTPUT);

	pinMode(ALARM_PIN,INPUT_PULLUP);
	pinMode(METER_PIN,INPUT_PULLUP);

	pinMode(RADIO_SLEEP_PIN,OUTPUT);
	pinMode(RADIO_RTS_PIN,OUTPUT);
	pinMode(RADIO_CTS_PIN,INPUT);

	digitalWrite(VALVE_ENABLE_PIN,0);
	digitalWrite(VALVE_CONTROL_1_PIN,0);
	digitalWrite(VALVE_CONTROL_2_PIN,0);
	digitalWrite(SD_SS_PIN,1);
	digitalWrite(DS3234_SS_PIN,1);

	pinMode(RST_PIN,INPUT_PULLUP);

	// Initialize SPI Communication
	DS3234_init(DS3234_SS_PIN);
	SPIFunc = RTC;

	// Initialize Radio Communication
	Serial.begin(9600,SERIAL_8N1);
	while(!Serial){;}

	// Set Global Variables
	leak = 0;
	timerCount = -1;			// initialize at -1 since the first loop will increment this to 0before time has run
	meterIntTime = 0;
	lastMeterIntTime = 0;
	lastInt = NONE;
	isBounce = false;
}

void loop()
{
	digitalWrite(RADIO_RTS_PIN,LOW);			// tell xBee we are available to receive data
	leak = 0;
	if (!digitalRead(RST_PIN))
	{
		// manually reset system if INPUT 1 is held
		resetSystem();
	}
	else
	{
		switch (lastInt)
		{
		case NONE:
			isBounce = false;						// wait until a few sleeps have happened then transmit data back
			timerCount++;
			if (timerCount >= 10)					// 10x 8 second intervals have passed
			{
				reportLog();
				reportLeak();
				clearLog();
				timerCount = 0;
			}
			break;
		case RADIO:
												// I dont think we are going to implement radio wake yet since we are using AT mode for testing
			break;
		case METER:
			sleep_enable();
			LowPower.powerDown(SLEEP_250MS,ADC_OFF,BOD_OFF);		// wait 250ms before reading pin to avoid bounce
			sleep_disable();
			if (digitalRead(METER_PIN) == LOW)
			{
				isBounce = false;
				logGallon();
				// check if a leak was previously detected
				if (wasLeakDetected()==0)
				{
					// if a new leak is detected, log it, report it, and turn off the valve
					leak = checkForLeaks();
					if (leak!=0)
					{
						setLeakCondition(leak);
						closeValve();
						reportLog();
						reportLeak();
						clearLog();
					}
				}
			}
			else
			{
				isBounce = true;
			}
			break;
		}

		if (!isBounce)
		{
			cycleRadio();
			checkRadioCommands();
			flushSerial();
			digitalWrite(RADIO_RTS_PIN,HIGH);	// tell xbee to stop sending data
			sleepRadio();
		}
	}

	shutdown();							// Do not add or remove any lines below this or I will murder your family
	sleep_disable();
	detachInterrupt(0);
	detachInterrupt(1);
}
