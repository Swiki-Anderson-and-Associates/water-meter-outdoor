################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include src/subdir.mk
-include lib/subdir.mk
-include arduinolib/utility/subdir.mk
-include arduinolib/subdir.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C++_DEPS)),)
-include $(C++_DEPS)
endif
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
ifneq ($(strip $(ASM_DEPS)),)
-include $(ASM_DEPS)
endif
ifneq ($(strip $(CC_DEPS)),)
-include $(CC_DEPS)
endif
ifneq ($(strip $(CPP_DEPS)),)
-include $(CPP_DEPS)
endif
ifneq ($(strip $(S_DEPS)),)
-include $(S_DEPS)
endif
ifneq ($(strip $(CXX_DEPS)),)
-include $(CXX_DEPS)
endif
ifneq ($(strip $(C_UPPER_DEPS)),)
-include $(C_UPPER_DEPS)
endif
ifneq ($(strip $(S_UPPER_DEPS)),)
-include $(S_UPPER_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 
LSS += \
ArduinoWaterMeter.lss \

FLASH_IMAGE += \
ArduinoWaterMeter.hex \

SIZEDUMMY += \
sizedummy \


# All Target
all: ArduinoWaterMeter.elf secondary-outputs

# Tool invocations
ArduinoWaterMeter.elf: $(OBJS) $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: AVR C++ Linker'
	avr-g++ -Wl,-Map,ArduinoWaterMeter.map,--cref -mrelax -Wl,--gc-sections -L"C:\Dropbox\Work\Programming\Eclipseworkspace\ArduinoUnoCore\328P_16MHz" -mmcu=atmega328p -o "ArduinoWaterMeter.elf" $(OBJS) $(USER_OBJS) $(LIBS); avr-nm -C -n "ArduinoWaterMeter.elf" >ArduinoWaterMeter.symbol
	@echo 'Finished building target: $@'
	@echo ' '

ArduinoWaterMeter.lss: ArduinoWaterMeter.elf
	@echo 'Invoking: AVR Create Extended Listing'
	-avr-objdump -h -S ArduinoWaterMeter.elf  >"ArduinoWaterMeter.lss"
	@echo 'Finished building: $@'
	@echo ' '

ArduinoWaterMeter.hex: ArduinoWaterMeter.elf
	@echo 'Create Flash image (ihex format)'
	-avr-objcopy -R .eeprom -R .fuse -R .lock -R .signature -O ihex ArduinoWaterMeter.elf  "ArduinoWaterMeter.hex"
	@echo 'Finished building: $@'
	@echo ' '

sizedummy: ArduinoWaterMeter.elf
	@echo 'Invoking: Print Size'
	-avr-size --format=avr --mcu=atmega328p ArduinoWaterMeter.elf
	@echo 'Finished building: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(OBJS)$(C_DEPS)$(FLASH_IMAGE)$(ELFS)$(LSS)$(S_DEPS)$(CXX_DEPS)$(S_UPPER_DEPS)$(C++_DEPS)$(ASM_DEPS)$(CC_DEPS)$(CPP_DEPS)$(C_UPPER_DEPS)$(SIZEDUMMY) ArduinoWaterMeter.elf
	-@echo ' '

secondary-outputs: $(LSS) $(FLASH_IMAGE) $(SIZEDUMMY)

.PHONY: all clean dependents
.SECONDARY:

-include ../makefile.targets
