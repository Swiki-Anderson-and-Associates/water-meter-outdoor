#include <Arduino.h>
#include <avr/sleep.h>
#include <stdio.h>
#include <SD.h>
#include <EEPROM.h>
#include "DS1306.h"
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

#define RTC_SS_PIN			10			// pin pulled low to allow SPI communication with DS3234 RTC
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

#define LOG_WRITING			true		// Parameter for openLogFile function
#define LOG_READING			false		// Parameter for openLogFile function

// Define Enumerations
enum interruptType {NONE, RADIO, METER};
enum SPIType {RTC, SDCard};
enum CardFile {Writing, Reading, Closed};

// Define Global Variables
File logFile;
static char MessageBuffer[256];
uint8_t leak, timerCount;
uint32_t meterIntTime, lastMeterIntTime;
volatile interruptType lastInt;			// any variables changed by ISRs must be declared volatile
SPIType SPIFunc;
CardFile FileIs;
bool isBounce;
DS1306 rtc;
XbeePro xbee;

// Define Program Function
static void closeLogFile()
{
	logFile.close();
	FileIs = Closed;
}

static uint8_t openLogFile(bool choice)			// TODO: OpenLogFile - set this up to create new logs every month
{
	if((choice == LOG_WRITING && FileIs == Writing) || (choice == LOG_READING && FileIs == Reading))
	{
		return 3;	// Indicator that nothing was done
	}
	else
	{
		// SD card error check
		if(!SD.begin(4))
		{
			return 1;
		}

		// SD card file open commands
		if(choice)
		{
			if(FileIs == Reading){closeLogFile();}
			logFile = SD.open("log.bmp",FILE_WRITE);
			FileIs = Writing;
		}
		else
		{
			if(FileIs == Writing){closeLogFile();}
			logFile = SD.open("log.bmp",FILE_READ);
			FileIs = Reading;
		}

		// File open error checks
		if(logFile)
		{
			return 0;
		}
		else
		{
			return 2;
		}
	}
}

static uint8_t useSDCard(bool choice)
{
	if(SPIFunc == SDCard)
	{
		return openLogFile(choice);		// TODO: UseSDCard - no longer needs to return openLogFile here
	}
	else
	{
		SPIFunc = SDCard;
		return openLogFile(choice);
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
		rtc.init(RTC_SS_PIN);
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

/*	Function replaced inside of XbeePro class
static uint8_t printSerial()
{
	if (digitalRead(RADIO_CTS_PIN))
	{
		cycleRadio();
	}
	return Serial.print(MessageBuffer);
}
*/

/*	Function replaced with the RadioTime() function
static void printTime()
{
	// Will we need non-log times?
	useRTC();
	ds1306time time;
	rtc.getTime(&time);
	// sprintf(MessageBuffer,"%02u/%02u/%4d %02d:%02d:%02d\t",time.month,time.day,time.year,time.hours,time.minutes,time.seconds);
	// printSerial();
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

static uint16_t getLastLogPos()
{
	return EEPROM.read(2);
}

static void RadioTime()
{
	useRTC();
	uint32_t t_unix = rtc.getTimeUnix();

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

static void writeLogEntry(uint8_t Header, uint32_t t_unix)
{
	useSDCard(LOG_WRITING);
	logFile.write(Header);

	if(Header == 0x04)
	{
		EEPROM.write(7,EEPROM.read(11));
		EEPROM.write(8,EEPROM.read(12));
		EEPROM.write(9,EEPROM.read(13));
		EEPROM.write(10,EEPROM.read(14));
	}

	// stores t_unix as 4 bytes in log (and EEPROM if it is a gallon log)
	uint8_t splitByte;
	splitByte = t_unix/16777216;
	logFile.write(splitByte);
	if(Header == 0x04){EEPROM.write(10,splitByte);}

	t_unix -= (uint32_t)(splitByte)*16777216;
	splitByte = t_unix/65536;
	logFile.write(splitByte);
	if(Header == 0x04){EEPROM.write(11,splitByte);}

	t_unix -= (uint32_t)(splitByte)*65536;
	splitByte = t_unix/256;
	logFile.write(splitByte);
	if(Header == 0x04){EEPROM.write(12,splitByte);}

	t_unix -= (uint32_t)(splitByte)*256;
	splitByte = t_unix;
	logFile.write(splitByte);
	if(Header == 0x04){EEPROM.write(13,splitByte);}
}

static void closeValve()
{
	uint8_t Header = 0x12;
	digitalWrite(VALVE_ENABLE_PIN,1);
	digitalWrite(VALVE_CONTROL_1_PIN,0);
	digitalWrite(VALVE_CONTROL_2_PIN,1);
	delay(5000);
	setValvePos(0);
	digitalWrite(VALVE_ENABLE_PIN,0);
	digitalWrite(VALVE_CONTROL_2_PIN,0);

	// Report event over radio
	// printTime();
	// sprintf(MessageBuffer,"Valve:\tClosed\n");
	xbee.PayloadCreator(Header,ADD_BYTE);
	RadioTime();
	xbee.ApiTxRequest();
	// return printSerial();

	// log event
	uint32_t t_unix = 0;
	useRTC();
	t_unix = rtc.getTimeUnix();
	writeLogEntry(Header,t_unix);
}

static void openValve()
{
	uint8_t Header = 0x11;
	digitalWrite(VALVE_ENABLE_PIN,1);
	digitalWrite(VALVE_CONTROL_1_PIN,1);
	digitalWrite(VALVE_CONTROL_2_PIN,0);
	delay(5000);
	setValvePos(1);
	digitalWrite(VALVE_ENABLE_PIN,0);
	digitalWrite(VALVE_CONTROL_1_PIN,0);

	// Report event over radio
	// printTime();
	// sprintf(MessageBuffer,"Valve:\tOpened\n");
	xbee.PayloadCreator(Header,ADD_BYTE);
	RadioTime();
	xbee.ApiTxRequest();
	// return printSerial();

	// log event
	uint32_t t_unix = 0;
	useRTC();
	t_unix = rtc.getTimeUnix();
	writeLogEntry(Header,t_unix);
}

static uint32_t readLogEntry(uint16_t logStart, bool Certainty)
{
	if(!Certainty)		// Certainty refers to if you already know that the log is open and in the reading format.
	{
		closeLogFile();
		useSDCard(LOG_READING);
	}

	// Sets the read function to read the desired byte
	for(uint8_t i=0; i <= logStart; i++)
	{
		uint8_t junk= logFile.read();	// TODO: ReadLogEntry - system that doesn't require a junk variable.
	}

	// API Header portion of the entry
	uint8_t Header = logFile.read();

	// Unix time portion of the entry
	uint32_t t_unix = 0;
	t_unix += (uint32_t)logFile.read()*16777216;
	t_unix += (uint32_t)logFile.read()*65536;
	t_unix += (uint32_t)logFile.read()*256;
	t_unix += (uint32_t)logFile.read();
	return t_unix;
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

// Needed on SD system? We will need to switch to the SDFat library to be able to delete files.
/*
static void clearLog()						// TODO: ClearLog - rewrite using SD card and API format
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
	// printTime();
	// sprintf(MessageBuffer,"Log:\tCleared\n");
	xbee.PayloadCreator(0x05,ADD_BYTE);
	RadioTime();
	xbee.ApiTxRequest();
	// return printSerial();
}
*/

static void resetSystem()
{
	openValve();
	clearLog();
	setLeakCondition(0);
	setDayGallons(0);
	setConsecGallons(0);
	// printTime();
	// sprintf(MessageBuffer,"System Reset\n");
	xbee.PayloadCreator(0x51,ADD_BYTE);
	RadioTime();
	xbee.ApiTxRequest();
	useRTC();
	uint32_t t_unix= rtc.getTimeUnix();
	writeLogEntry(0x51,t_unix);
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
	sleep_enable();										// Don't fuck with anything below this point in this function
	attachInterrupt(0,radioInterrupt,LOW);
	attachInterrupt(1,meterInterrupt,CHANGE);
	lastInt = NONE;
	LowPower.powerDown(SLEEP_8S,ADC_OFF,BOD_OFF);
}

static void reportLog()
{
	uint16_t logStart = getLastLogPos();
	uint8_t i;
	/* Old version
	printTime();
	sprintf(MessageBuffer,"Gallon Log:\n");
	printSerial();
	API header and time for a log report
	if (lastLog == LOG_START_POS-1)
	{
		sprintf(MessageBuffer,"Empty\n");
		xbee.PayloadCreator(0x03,ADD_BYTE);
		RadioTime();
		printSerial();
	}
	else
	{
		for (i=LOG_START_POS;i<lastLog;i++)
		{
			if(i%4 == 0)
			{
				printTime();
				sprintf(MessageBuffer,"%u\t%lu\n",(i-12)/4,readLogEntry((i,true)));
				printSerial();
			}
		}
	}
	printTime();
	sprintf(MessageBuffer,"End Log\n");
	*/
	closeLogFile();
	useSDCard(LOG_READING);

	// Sets the read function to read the desired byte
	for(uint8_t i=0; i < logStart; i++)
	{
		uint8_t Worthless_Shit_Hole = logFile.read();	// TODO: ReportLog - System that doesn't require a junk variable.
	}

	// Add log bytes to the radio payload
	int counter = 0;
	while(logFile.peek() != -1)
	{
		xbee.PayloadCreator(logFile.read(),ADD_BYTE);
		counter++;
		if(counter == 66)		// Keeps the log from overloading the xbee api frame.
		{
			xbee.ApiTxRequest();
			logStart = logStart + counter;
			counter = 0;
		}
	}
	EEPROM.write(2,logStart+counter);

	xbee.PayloadCreator(0x02,ADD_BYTE);
	xbee.ApiTxRequest();
	// return printSerial();
}

static void logGallon()
{
	uint8_t Header = 0x04;
	uint32_t t_unix = 0;
	useRTC();
	t_unix = rtc.getTimeUnix();
	writeLogEntry(Header,t_unix);
}

static uint32_t readEEGallon(uint8_t start)
{
	uint32_t t_unix = 0;
	t_unix += (uint32_t)EEPROM.read(start)*16777216;
	t_unix += (uint32_t)EEPROM.read(start+1)*65536;
	t_unix += (uint32_t)EEPROM.read(start+2)*256;
	t_unix += (uint32_t)EEPROM.read(start+3);
	return t_unix;
}

static uint8_t checkForLeaks()
{
	useRTC();
	ds1306time t;
	rtc.getTime(&t);
	uint16_t dayGallons = getDayGallons();
	uint32_t t_lastLog = readEEGallon(11);
	uint32_t t_prevLog = readEEGallon(7);
	uint8_t dayNumber = EEPROM.read(6);
	uint8_t prevConsMins = getConsecGallons();

	if (t.day != dayNumber)						// full day has passed
	{
		EEPROM.write(6,t.day);					// reset day in EEPROM
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
	// printTime();
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
	xbee.ApiTxRequest();
}

static void clearLeak()
{
	setLeakCondition(0);
	// printTime();
	// sprintf(MessageBuffer,"Leak:\tCleared\n");
	xbee.PayloadCreator(0x22,ADD_BYTE);
	RadioTime();
	xbee.ApiTxRequest();
	// return printSerial();
}

static void reportValve()
{
	// printTime();
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

static void processRadio(uint8_t Signal[], uint8_t size)
{
	for(uint8_t i=0; i<=size; i++)
	{
		switch (Signal[i])
		{
			case 0x51:
				resetSystem();
				break;
			case 0x12:
				closeValve();
				break;
			case 0x11:
				openValve();
				break;
			case 0x21:
				reportLeak();
				break;
			case 0x13:
				reportValve();
				break;
			case 0x01:
				reportLog();
				break;
			case 0x02:
				clearLog();
				break;
			case 0x22:
				clearLeak();
				break;
			default:
				break;
		}
	}
}

static void checkRadioCommands()
{
	uint8_t Frame_Type, Receive_Options, Checksum;			// Can add functionality to do with these later
	uint8_t Addr_64[8], Addr_16[2], Length[2];

	delay(10);												// wait for data to be received
	if(Serial.available()>0)
	{
		if(Serial.read() == 0x7E)
		{
			Length[0] = Serial.read();
			Length[1] = Serial.read();
			uint8_t Data_array[Length[1] - 12];				// Assuming that the length will never go over one byte.

			Frame_Type = Serial.read();

			for(uint8_t i=0; i<=7; i++)
			{
				Addr_64[i] = Serial.read();
			}

			Addr_16[0] = Serial.read();
			Addr_16[1] = Serial.read();

			Receive_Options = Serial.read();

			for(uint8_t i=0; i <= (Length[1] - 13); i++)
			{
				Data_array[i] = Serial.read();
			}

			processRadio(Data_array, Length[1]-12);
		}
	}
}



// Runtime functions
void setup()
{
	// Initialize Pins
	pinMode(VALVE_ENABLE_PIN,OUTPUT);
	pinMode(VALVE_CONTROL_1_PIN,OUTPUT);
	pinMode(VALVE_CONTROL_2_PIN,OUTPUT);
	pinMode(SD_SS_PIN,OUTPUT);
	pinMode(RTC_SS_PIN,OUTPUT);

	pinMode(ALARM_PIN,INPUT_PULLUP);
	pinMode(METER_PIN,INPUT_PULLUP);

	pinMode(RADIO_SLEEP_PIN,OUTPUT);
	pinMode(RADIO_RTS_PIN,OUTPUT);
	pinMode(RADIO_CTS_PIN,INPUT);

	digitalWrite(VALVE_ENABLE_PIN,0);
	digitalWrite(VALVE_CONTROL_1_PIN,0);
	digitalWrite(VALVE_CONTROL_2_PIN,0);
	digitalWrite(SD_SS_PIN,1);
	digitalWrite(RTC_SS_PIN,1);

	pinMode(RST_PIN,INPUT_PULLUP);

	// Initialize SPI Communication
	rtc.init(RTC_SS_PIN);
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
	FileIs = Closed;
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
												// I don't think we are going to implement radio wake yet since we are using AT mode for testing
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
// TODO: Rearrange functions to remove dependency errors.
// TODO: EEPROM rotation will require using 1 or 2 more EEPROM addresses.
