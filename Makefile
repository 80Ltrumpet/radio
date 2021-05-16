# This makefile was created from scratch based on verbose output from the
# Arduino IDE. For now, it is specific to programming the ATmega2560.
# Author: Andrew Lehmer

AVR_TOOLS := "C:/Arduino/hardware/tools/avr"

BINARY := andruio.hex

SOURCES := $(wildcard *.cc)
OBJECTS := $(patsubst %.cc, %.o, $(SOURCES))

# Pre-compute static provisioning for tasks and commands.
NUM_TASKS := $(shell findstr "::AddTask.*;" $(SOURCES) | find /c /v "")
NUM_COMMANDS := $(shell findstr "::RegisterCommand<" $(SOURCES) | find /c /v "")

DEFINES := -DF_CPU=16000000UL -DARDUINO=10813 -DARDUINO_AVR_MEGA2560
DEFINES += -DBAUD=9600
# Custom definitions
DEFINES += -DANDRUIO_MAX_TASKS=$(NUM_TASKS)
DEFINES += -DANDRUIO_MAX_COMMANDS=$(NUM_COMMANDS)

INCDIRS := -I$(AVR_TOOLS)/avr/include
INCDIRS += -I$(AVR_TOOLS)/lib/gcc/avr/7.3.0/include

COMMON_FLAGS := -Wall -Wextra -Os -flto -mmcu=atmega2560 -w -ffunction-sections
COMMON_FLAGS += -fdata-sections -fno-exceptions $(DEFINES) $(INCDIRS)

LFLAGS := $(COMMON_FLAGS) -Wl,--gc-sections
EEPFLAGS := -O ihex -j .eeprom --set-section-flags=.eeprom=alloc,load
EEPFLAGS += --no-change-warnings --change-section-lma .eeprom=0
HEXFLAGS := -O ihex -R .eeprom

# For implicit Make rules
CC := $(AVR_TOOLS)/bin/avr-g++
CXX := $(AVR_TOOLS)/bin/avr-g++
CFLAGS := $(COMMON_FLAGS) -std=c11
CXXFLAGS := $(COMMON_FLAGS) -std=c++17

all: $(BINARY)

%.elf: $(OBJECTS)
	$(CXX) $(LFLAGS) -o $@ $^

%.eep: %.elf
	$(AVR_TOOLS)/bin/avr-objcopy $(EEPFLAGS) $^ $@

%.hex: %.elf
	$(AVR_TOOLS)/bin/avr-objcopy $(HEXFLAGS) $^ $@

clean:
	@del $(BINARY)

install: all
	$(AVR_TOOLS)/bin/avrdude -C$(AVR_TOOLS)/etc/avrdude.conf -v \
		-patmega2560 -cwiring -PCOM3 -b115200 -D -Uflash:w:$(BINARY):i
