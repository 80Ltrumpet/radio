# Configuration specific to building the root (access point) (Arduino MEGA 2560)

MMCU := atmega2560

DEFINES := -DANDRUIO_CONFIG_ROOT
DEFINES += -DF_CPU=16000000UL -DARDUINO_AVR_MEGA2560 -DBAUD=9600

.PHONY: install
install: $(CONFIG).hex
	$(AVR_TOOLS)/bin/avrdude -C$(AVR_TOOLS)/etc/avrdude.conf -v \
		-p$(MMCU) -cwiring -PCOM3 -b115200 -D -Uflash:w:$(CONFIG).hex:i