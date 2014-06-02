################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../arduinolib/EEPROM.cpp \
../arduinolib/File.cpp \
../arduinolib/SD.cpp \
../arduinolib/SPI.cpp 

OBJS += \
./arduinolib/EEPROM.o \
./arduinolib/File.o \
./arduinolib/SD.o \
./arduinolib/SPI.o 

CPP_DEPS += \
./arduinolib/EEPROM.d \
./arduinolib/File.d \
./arduinolib/SD.d \
./arduinolib/SPI.d 


# Each subdirectory must supply rules for building sources it contributes
arduinolib/%.o: ../arduinolib/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: AVR C++ Compiler'
	avr-g++ -I"C:\Dropbox\Work\Programming\Eclipseworkspace\ArduinoUnoCore\src" -I"C:\Dropbox\Work\Programming\Eclipseworkspace\ArduinoWaterMeter\arduinolib" -I"C:\Dropbox\Work\Programming\Eclipseworkspace\ArduinoWaterMeter\lib" -DARDUINO=100 -Wall -g2 -gstabs -Os -ffunction-sections -fdata-sections -fno-exceptions --pedantic -mmcu=atmega328p -DF_CPU=16000000UL -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


