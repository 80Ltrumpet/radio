# Configuration specific to building nodes (Feather 32u4)

MMCU := atmega32u4

DEFINES := -DF_CPU=8000000L -DARDUINO_AVR_FEATHER32U4 -DARDUINO_ARCH_AVR
DEFINES += -DUSB_VID=0x239A -DUSB_PID=0x800C "-DUSB_MANUFACTURER=\"Adafruit\""
DEFINES += "-DUSB_PRODUCT=\"Feather 32u4\"" "-DUSB_SERIAL_NUMBER=\"\""

CPPFLAGS := -fpermissive -Wno-error-narrowing

USER_COM := COM7
BOOT_COM := COM5

.PHONY: install
install: $(CONFIG).hex
	@powershell "$$ports = [System.IO.Ports.SerialPort]::GetPortNames(); \
	             if (\"$(BOOT_COM)\" -notin $$ports -and \"$(USER_COM)\" -in $$ports) { \
							   Write-Output \"Forcing reset using 1200bps open/close on port $(USER_COM).\"; \
							   $$user_com = New-Object System.IO.Ports.SerialPort $(USER_COM),1200,None,8,1; \
							   $$user_com.Open(); \
							   $$user_com.Close(); \
								 Start-Sleep 1; \
							 }"
	$(AVR_TOOLS)/bin/avrdude -C$(AVR_TOOLS)/etc/avrdude.conf -v \
		-p$(MMCU) -cavr109 -PCOM5 -b56700 -D -Uflash:w:$(CONFIG).hex:i