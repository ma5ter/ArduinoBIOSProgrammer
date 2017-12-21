#include "programmer.h"
#include "XModem.h"

#if defined(__INTELLISENSE__) && defined(F)
#undef F
#define F(string) (__FlashStringHelper *)string
#endif

int inline Programmer::message(const __FlashStringHelper * format, ...)
{
	int result;

	va_list args;
	va_start (args, format);

	result = vfprintf_P(console, (const char*)format, args);

	va_end (args);

	return result;
}

void Programmer::initialize(void)
{
	message(F(CLEAR_TERMINAL "Initializing Chip...\r\n"));
	if (flash.begin(winbondFlashClass::autoDetect, SPI, SS)) {
		initialized = 1;
		size = flash.bytes();
		message(F(
			"CHIP READY\r\n"
			"Type: %s\r\n"
			"Flash size: %lub (%luk / %luM)\r\n"
			), "not implemented", size, size >> 10, size >> 20);
	}
	else {
		initialized = 0;
		message(F("INITIALIZE FAILED\r\n"));
	}
}

void Programmer::task(void)
{
	// GET COMMAND
	int code = fgetc(console);
	if (code < 0) {
		return;
	}
	
	uint8_t octet = code;
	if ('0' == octet) {
		initialize();
	}
	else if (!initialized) {
		message(F("Chip is not initialized, '0' to initialize\r\n"));
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
				message(F("Erase all\r\n"));
				flash.setWriteEnable(1);
				flash.eraseAll();
				do {
					fputc('.', console);
					delay(100);
				} while (flash.busy());
				message(F("\r\nDone\r\n"));
			}
			break;
		case 'W':
		case 'w':
			{
				message(F("Write chip\r\nReady to receive file using xmodem, please start send\r\n"));
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
						while (flash.busy());
					}
				}

				delay(1000);

				switch (result)
				{
				case XModem::END:
					message(F("\r\nWrite done\r\n"));
					break;
				case XModem::NEXT:
					message(F("Out of memory\r\n"));
					break;
				case XModem::TIMEOUT:
					message(F("Timeout\r\n"));
					break;
				case XModem::CANCEL:
					message(F("Cancelled by other party\r\n"));
					break;
				default:
					message(F("Unknown result\r\n"));
					break;
				}
			}
			break;
		case 'C':
		case 'R':
		case 'r':
			{
				if ('C' != octet) {
					message(F("Read chip\r\nSending file using xmodem, please start receive\r\n"));
				}
				XModem * xmodem = new XModem(Serial);

				enum XModem::block_result result = XModem::START;

				if ('C' != octet) {
					result = xmodem->start_send();
				}

				if (XModem::START != result) {
					message(F("Failed to start xmodem\r\n"));
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
						message(F("Transfer cancelled\r\n"));
						break;
					case XModem::ERROR:
						message(F("Transfer error\r\n"));
						break;
					default:
						message(F("Unknown result\r\n"));
						break;
					}
				}
			}
			break;
		default: 
			{
				message(F("Unknown command code '%c'\r\n"), octet);
			}
			break;
		} 
	}
}

Programmer::Programmer(FILE * console)
{
	this->initialized = false;
	this->console = console;
}

Programmer::~Programmer(void)
{
}
