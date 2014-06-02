################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/WaterMeterMain.cpp \
../src/pure_virtual.cpp 

OBJS += \
./src/WaterMeterMain.o \
./src/pure_virtual.o 

CPP_DEPS += \
./src/WaterMeterMain.d \
./src/pure_virtual.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: AVR C++ Compiler'
	avr-g++ -I"C:\Dropbox\Work\Programming\Eclipseworkspace\ArduinoUnoCore\src" -I"C:\Dropbox\Work\Programming\Eclipseworkspace\ArduinoWaterMeter\arduinolib" -I"C:\Dropbox\Work\Programming\Eclipseworkspace\ArduinoWaterMeter\lib" -DARDUINO=100 -Wall -g2 -gstabs -Os -ffunction-sections -fdata-sections -fno-exceptions --pedantic -mmcu=atmega328p -DF_CPU=16000000UL -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


