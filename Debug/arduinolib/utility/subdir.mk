################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../arduinolib/utility/Sd2Card.cpp \
../arduinolib/utility/SdFile.cpp \
../arduinolib/utility/SdVolume.cpp 

OBJS += \
./arduinolib/utility/Sd2Card.o \
./arduinolib/utility/SdFile.o \
./arduinolib/utility/SdVolume.o 

CPP_DEPS += \
./arduinolib/utility/Sd2Card.d \
./arduinolib/utility/SdFile.d \
./arduinolib/utility/SdVolume.d 


# Each subdirectory must supply rules for building sources it contributes
arduinolib/utility/%.o: ../arduinolib/utility/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: AVR C++ Compiler'
	avr-g++ -I"C:\Dropbox\Work\Programming\Eclipseworkspace\ArduinoUnoCore\src" -I"C:\Dropbox\Work\Programming\Eclipseworkspace\ArduinoWaterMeter\arduinolib" -I"C:\Dropbox\Work\Programming\Eclipseworkspace\ArduinoWaterMeter\lib" -DARDUINO=100 -Wall -g2 -gstabs -Os -ffunction-sections -fdata-sections -fno-exceptions --pedantic -mmcu=atmega328p -DF_CPU=16000000UL -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


