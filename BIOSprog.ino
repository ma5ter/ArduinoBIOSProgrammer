
#include <SPI.h>
#include "winbondflash.h"
#include "XModem.h"

winbondFlashSPI flash;
uint8_t page[FLASH_PAGE_SIZE];
uint8_t initialized = 0;
uint32_t size = 0;

void restart(void)
{
	Serial.println(F("\x1B[0m\x1B[2J\r\nInitializing Chip..."));
	if (flash.begin(winbondFlashClass::autoDetect, SPI, SS)) {
		size = flash.bytes();
		Serial.print(F("Flash size is "));
		Serial.print(size);
		Serial.print(F("b ("));
		Serial.print(size >> 10);
		Serial.print(F("k / "));
		Serial.print(size >> 20);
		Serial.print(F("M)"));
		Serial.println();
		Serial.println(F("CHIP READY"));
		initialized = 1;
	}
	else {
		Serial.println(F("FAILED"));
		initialized = 0;
	}
}

void setup()
{
	Serial.begin(115200);
}

void loop()
{
	// GET COMMAND
	if (Serial.available() > 0) {
		uint8_t octet = Serial.read();

		if ('0' == octet) {
			restart();
		}
		else if (!initialized) {
			Serial.println(F("Chip is not initialized, '0' to initialize"));
		}
		else {
			switch(octet) {
			case 13:
			case 24:
				break;
			//ERASE
			case 'E':
			case 'e':
				{
					Serial.println(F("Erase all"));
					flash.setWriteEnable(1);
					flash.eraseAll();
					Serial.println(F("Done"));
				}
				break;
			case 'W':
			case 'w':
				{
					Serial.println(F("Write chip\r\nReady to receive file using xmodem, please start send"));
					XModem * xmodem = new XModem(Serial);
				
					enum XModem::block_result result = XModem::NEXT;
					for (uint32_t address = 0; (address < size) && (result == XModem::NEXT); address += FLASH_PAGE_SIZE) {
						uint8_t * ptr = page;
						uint8_t count = 2; // FLASH_PAGE_SIZE / XMODEM_BLOCK_SIZE;
						while (count && (result == XModem::NEXT)) {
							result = xmodem->block_receive(ptr);
							ptr += XMODEM_BLOCK_SIZE;
							count--;
						}
						if (result == XModem::NEXT || result == XModem::END) {
							flash.setWriteEnable(1);
							flash.writePage(address, page);
						}
					}

					delay(1000);

					switch (result)
					{
					case XModem::END:
						Serial.println(F("\r\nWrite done"));
						break;
					case XModem::NEXT:
						Serial.println(F("Out of memory"));
						break;
					case XModem::TIMEOUT:
						Serial.println(F("Timeout"));
						break;
					case XModem::CANCEL:
						Serial.println(F("Cancelled by other party"));
						break;
					default:
						Serial.println(F("Unknown result"));
						break;
					}
				}
				break;
			case 'C':
			case 'R':
			case 'r':
				{
					if ('C' != octet) {
						Serial.println(F("Read chip\r\nSending file using xmodem, please start receive"));
					}
					XModem * xmodem = new XModem(Serial);

					enum XModem::block_result result = XModem::START;
					
					if ('C' != octet) {
						result = xmodem->start_send();
					}
				
					if (XModem::START != result) {
						Serial.println(F("Failed to start xmodem"));
					}
					else {
						result = XModem::NEXT;
						for (uint32_t address = 0; (address < size) && (result == XModem::NEXT); address += XMODEM_BLOCK_SIZE) {
							flash.read(address, page, XMODEM_BLOCK_SIZE);
							result = xmodem->block_send(page);
						}

						delay(1000);

						switch (result)
						{
						case XModem::NEXT:
							xmodem->finish_send();
							break;
						case XModem::CANCEL:
							Serial.println(F("Transfer cancelled"));
							break;
						case XModem::ERROR:
							Serial.println(F("Transfer error"));
							break;
						default:
							Serial.println(F("Unknown result"));
							break;
						}
					}
				}
				break;
			default: 
				{
					Serial.print(F("Unknown command code "));
					Serial.println(octet);
				}
				break;
			} 
		}
	}
}



