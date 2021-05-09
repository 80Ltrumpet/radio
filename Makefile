# This makefile was created from scratch based on verbose output from the
# Arduino IDE. For now, it is specific to programming the ATmega2560.
# Author: Andrew Lehmer

# Configuration
AVR_TOOLS := "C:/Program Files (x86)/Arduino/hardware/tools/avr"

BINARY := andruio.hex

C_SRC := $(wildcard *.c)
CXX_SRC := $(wildcard *.cc)

COMMON_FLAGS := -Wall -Wextra -Os -flto -mmcu=atmega2560

DEFINES := -DF_CPU=16000000UL -DARDUINO=10813 -DARDUINO_AVR_MEGA2560
DEFINES += -DBAUD=9600

INCDIRS := -I$(AVR_TOOLS)/avr/include

OBJECTS := $(patsubst %.c, %.o, $(C_SRC))
OBJECTS += $(patsubst %.cc, %.o, $(CXX_SRC))

LFLAGS := $(COMMON_FLAGS) -Wl,--gc-sections
EEPFLAGS := -O ihex -j .eeprom --set-section-flags=.eeprom=alloc,load
EEPFLAGS += --no-change-warnings --change-section-lma .eeprom=0
HEXFLAGS := -O ihex -R .eeprom

# For implicit Make rules
CC := $(AVR_TOOLS)/bin/avr-g++
CXX := $(AVR_TOOLS)/bin/avr-g++
CFLAGS := $(COMMON_FLAGS) -w -ffunction-sections -fdata-sections -fno-exceptions
CFLAGS += $(DEFINES) $(INCDIRS)
CXXFLAGS := $(CFLAGS) -std=c++17

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
