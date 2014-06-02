################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../lib/LowPower.cpp \
../lib/ds3234.cpp 

OBJS += \
./lib/LowPower.o \
./lib/ds3234.o 

CPP_DEPS += \
./lib/LowPower.d \
./lib/ds3234.d 


# Each subdirectory must supply rules for building sources it contributes
lib/%.o: ../lib/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: AVR C++ Compiler'
	avr-g++ -I"C:\Dropbox\Work\Programming\Eclipseworkspace\ArduinoUnoCore\src" -I"C:\Dropbox\Work\Programming\Eclipseworkspace\ArduinoWaterMeter\arduinolib" -I"C:\Dropbox\Work\Programming\Eclipseworkspace\ArduinoWaterMeter\lib" -DARDUINO=100 -Wall -g2 -gstabs -Os -ffunction-sections -fdata-sections -fno-exceptions --pedantic -mmcu=atmega328p -DF_CPU=16000000UL -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


