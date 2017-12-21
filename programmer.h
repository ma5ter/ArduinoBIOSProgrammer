#ifndef __PROGRAMMER__
#define __PROGRAMMER__

#include <Arduino.h>
#include "winbondflash.h"

#define CLEAR_TERMINAL "\x1B[0m\x1B[2J"

class Programmer
{
private:
	FILE * console;
	bool initialized;
	winbondFlashSPI flash;
	uint8_t page[FLASH_PAGE_SIZE];

public:
	uint32_t size;

	void initialize(void);
	void task(void);
	int message(const __FlashStringHelper * format, ...);
	
	Programmer(FILE * console);
	~Programmer(void);
};

#endif