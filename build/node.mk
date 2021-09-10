# Configuration specific to building nodes (Feather 32u4)

MMCU := atmega32u4

SOURCES := $(wildcard src/node/*.cc)

DEFINES := -DF_CPU=8000000L -DARDUINO_AVR_FEATHER32U4 -DARDUINO_ARCH_AVR
DEFINES += -DUSB_VID=0x239A -DUSB_PID=0x800C "-DUSB_MANUFACTURER=\"Adafruit\""
DEFINES += "-DUSB_PRODUCT=\"Feather 32u4\"" "-DUSB_SERIAL_NUMBER=\"\""

CPPFLAGS := -fpermissive -Wno-error-narrowing
# DEBUG
LDFLAGS := -Wl,-u,vfprintf -lprintf_flt -lm

.PHONY: install
install: $(CONFIG).hex
	@powershell '$$ports = [System.IO.Ports.SerialPort]::GetPortNames(); \
	             if ("$(NODE_BOOT_PORT)" -notin $$ports -and "$(NODE_USER_PORT)" -in $$ports) { \
							   Write-Output "Forcing reset using 1200bps open/close on port $(NODE_USER_PORT)."; \
							   $$user_port = New-Object System.IO.Ports.SerialPort $(NODE_USER_PORT),1200,None,8,1; \
							   $$user_port.Open(); \
							   $$user_port.Close(); \
								 Start-Sleep 1; \
							 }'
	$(AVR_TOOLS)/bin/avrdude -C$(AVR_TOOLS)/etc/avrdude.conf -v \
		-p$(MMCU) -cavr109 -P$(NODE_BOOT_PORT) -b56700 -D -Uflash:w:$(CONFIG).hex:i