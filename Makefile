# This makefile was created from scratch based on verbose output from the
# Arduino IDE.

.DEFAULT_GOAL := all

AVR_TOOLS := "C:/Arduino/hardware/tools/avr"

# Configuration makefiles must include the following:
# - Define MMCU.
# - Define the "install" target.
ifndef CONFIG
ifneq ($(MAKECMDGOALS),clean)
$(warning CONFIG is undefined. Defaulting to "node".)
CONFIG := node
endif
endif

ifeq ($(CONFIG), node)
include build/node.mk
else ifeq ($(CONFIG), root)
include build/root.mk
else ifneq ($(CONFIG),)
$(error Invalid CONFIG)
endif

SOURCES := $(subst /,\,$(wildcard src/*.cc))
OBJECTS := $(SOURCES:.cc=.o)

# Pre-compute static provisioning for tasks and commands.
NUM_TASKS := $(shell findstr "::AddTask.*;" $(SOURCES) | find /c /v "")
NUM_COMMANDS := $(shell findstr "::RegisterCommand<" $(SOURCES) | find /c /v "")

DEFINES := -DARDUINO=10813 $(DEFINES)
# Custom definitions
DEFINES += -DANDRUIO_MAX_TASKS=$(NUM_TASKS)
DEFINES += -DANDRUIO_MAX_COMMANDS=$(NUM_COMMANDS)
DEFINES += "-DRADIO_SYNC_WORDS=0x54, 0x2c, 0xab, 0xd3"
DEFINES += -DRADIO_BROADCAST_ADDR=0xbc

INCDIRS := -Iinclude
INCDIRS += -I$(AVR_TOOLS)/avr/include
INCDIRS += -I$(AVR_TOOLS)/lib/gcc/avr/7.3.0/include

EEPFLAGS := -O ihex -j .eeprom --set-section-flags=.eeprom=alloc,load
EEPFLAGS += --no-change-warnings --change-section-lma .eeprom=0

HEXFLAGS := -O ihex -R .eeprom

COMMON_FLAGS := -Wall -Wextra -Os -flto -mmcu=$(MMCU)

# For implicit Make rules
CC := $(AVR_TOOLS)/bin/avr-g++
CXX := $(AVR_TOOLS)/bin/avr-g++
CFLAGS := -std=c11
CXXFLAGS := -std=c++17
CPPFLAGS := $(CPPFLAGS) $(COMMON_FLAGS) -w -ffunction-sections -fdata-sections
CPPFLAGS += -fno-exceptions $(DEFINES) $(INCDIRS)
LDFLAGS := $(LDFLAGS) $(COMMON_FLAGS) -fuse-linker-plugin -Wl,--gc-sections

.PHONY: all
all: $(CONFIG).hex

%.elf: $(OBJECTS)
	$(CXX) $(LDFLAGS) -o $@ $^

%.eep: %.elf
	$(AVR_TOOLS)/bin/avr-objcopy $(EEPFLAGS) $^ $@

%.hex: %.elf
	$(AVR_TOOLS)/bin/avr-objcopy $(HEXFLAGS) $^ $@

.PHONY: clean
clean:
	@del *.elf *.hex

.PHONY: size
size: $(CONFIG).elf
	$(AVR_TOOLS)/bin/avr-size -A $^