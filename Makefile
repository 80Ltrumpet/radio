# This makefile was created from scratch based on verbose output from the
# Arduino IDE.

.DEFAULT_GOAL := all

AVR_TOOLS := "C:/Arduino/hardware/tools/avr"

include build/local.mk

# Configuration makefiles must include the following:
# - Define MMCU.
# - Define the "install" target.
# - (Optional) Additional SOURCES, DEFINES, or CPPFLAGS
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

OUTPUT := $(CONFIG).hex

SOURCES := $(wildcard src/*.cc) $(SOURCES)

OBJTOP := obj
OBJDIR := $(OBJTOP)/$(CONFIG)
OBJECTS := $(addprefix $(OBJDIR)/,$(SOURCES:.cc=.o))
ELF := $(addprefix $(OBJDIR)/,$(OUTPUT:.hex=.elf))

# Pre-compute static provisioning for tasks and commands.
WINSRC := $(subst /,\,$(SOURCES))
NUM_TASKS := $(shell findstr "::AddTask.*;" $(WINSRC) | find /c /v "")
NUM_COMMANDS := $(shell findstr "::RegisterCommand<" $(WINSRC) | find /c /v "")

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

CXX := $(AVR_TOOLS)/bin/avr-g++
CXXFLAGS := -std=c++17 -MMD -MP
CPPFLAGS := $(CPPFLAGS) $(COMMON_FLAGS) -w -ffunction-sections -fdata-sections
CPPFLAGS += -fno-exceptions -fno-threadsafe-statics $(DEFINES) $(INCDIRS)
LDFLAGS := $(LDFLAGS) $(COMMON_FLAGS) -fuse-linker-plugin -Wl,--gc-sections

.PHONY: all
all: $(OUTPUT)

$(OBJECTS): $(OBJDIR)/%.o: %.cc
	@if not exist $(@D) mkdir $(subst /,\,$(@D))
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $< -o $@

-include $(OBJECTS:.o=.d)

$(ELF): $(OBJECTS)
	$(CXX) $(LDFLAGS) -o $@ $^

%.eep: %.elf
	$(AVR_TOOLS)/bin/avr-objcopy $(EEPFLAGS) $^ $@

$(OUTPUT): $(ELF)
	$(AVR_TOOLS)/bin/avr-objcopy $(HEXFLAGS) $^ $@

.PHONY: clean
clean:
	-rmdir /S /Q $(OBJTOP)
	-del *.hex

.PHONY: size
size: $(ELF)
	$(AVR_TOOLS)/bin/avr-size -A $^