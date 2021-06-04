# This makefile was created from scratch based on verbose output from the
# Arduino IDE. For now, it is specific to programming the ATmega2560.
# Author: Andrew Lehmer

AVR_TOOLS := "C:/Arduino/hardware/tools/avr"

MODULE := feather
MMCU := atmega32u4

USER_COM := COM7
BOOT_COM := COM5

COMMON_FLAGS := -Wall -Wextra -Os -flto -mmcu=$(MMCU)

SOURCES := $(wildcard *.cc)
OBJECTS := $(SOURCES:%.cc=%.o)

# Pre-compute static provisioning for tasks and commands.
NUM_TASKS := $(shell findstr "::AddTask.*;" $(SOURCES) | find /c /v "")
NUM_COMMANDS := $(shell findstr "::RegisterCommand<" $(SOURCES) | find /c /v "")

DEFINES := -DF_CPU=8000000L -DARDUINO=10813 -DARDUINO_AVR_FEATHER32U4
DEFINES += -DARDUINO_ARCH_AVR -DBAUD=9600
DEFINES += -DUSB_VID=0x239A -DUSB_PID=0x800C "-DUSB_MANUFACTURER=\"Adafruit\""
DEFINES += "-DUSB_PRODUCT=\"Feather 32u4\"" "-DUSB_SERIAL_NUMBER=\"\""
# Custom definitions
DEFINES += -DANDRUIO_MAX_TASKS=$(NUM_TASKS)
DEFINES += -DANDRUIO_MAX_COMMANDS=$(NUM_COMMANDS)
DEFINES += "-DRADIO_SYNC_WORDS=0x54, 0x2c, 0xab, 0xd3"
DEFINES += -DRADIO_BROADCAST_ADDR=0xbc

INCDIRS := -I$(AVR_TOOLS)/avr/include
INCDIRS += -I$(AVR_TOOLS)/lib/gcc/avr/7.3.0/include

EEPFLAGS := -O ihex -j .eeprom --set-section-flags=.eeprom=alloc,load
EEPFLAGS += --no-change-warnings --change-section-lma .eeprom=0
HEXFLAGS := -O ihex -R .eeprom

# For implicit Make rules
CC := $(AVR_TOOLS)/bin/avr-g++
CXX := $(AVR_TOOLS)/bin/avr-g++
CFLAGS := -std=c11
CXXFLAGS := -std=c++17
CPPFLAGS := $(COMMON_FLAGS) -w -ffunction-sections -fdata-sections
CPPFLAGS += -fno-exceptions -fpermissive -fno-threadsafe-statics
CPPFLAGS += -Wno-error-narrowing $(DEFINES) $(INCDIRS)
LDFLAGS := $(COMMON_FLAGS) -fuse-linker-plugin -Wl,--gc-sections

.PHONY: all
all: $(MODULE).hex

%.elf: $(OBJECTS)
	$(CXX) $(LDFLAGS) -o $@ $^

%.eep: %.elf
	$(AVR_TOOLS)/bin/avr-objcopy $(EEPFLAGS) $^ $@

%.hex: %.elf
	$(AVR_TOOLS)/bin/avr-objcopy $(HEXFLAGS) $^ $@

.PHONY: clean
clean:
	@del $(MODULE).*

.PHONY: install
install: all
	@powershell "$$ports = [System.IO.Ports.SerialPort]::GetPortNames(); \
	             if (\"$(BOOT_COM)\" -notin $$ports -and \"$(USER_COM)\" -in $$ports) { \
							   Write-Output \"Forcing reset using 1200bps open/close on port $(USER_COM).\"; \
							   $$user_com = New-Object System.IO.Ports.SerialPort $(USER_COM),1200,None,8,1; \
							   $$user_com.Open(); \
							   $$user_com.Close(); \
								 Start-Sleep 1; \
							 }"
	$(AVR_TOOLS)/bin/avrdude -C$(AVR_TOOLS)/etc/avrdude.conf -v \
		-p$(MMCU) -cavr109 -PCOM5 -b56700 -D -Uflash:w:$(MODULE).hex:i

.PHONY: size
size: $(MODULE).elf
	$(AVR_TOOLS)/bin/avr-size -A $^