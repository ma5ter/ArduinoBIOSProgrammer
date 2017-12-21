#include "programmer.h"

Programmer * programmer;

// file structure (initialized with zeros)
static FILE uart = { 0 };

// output function
static int uart_putchar(char c, FILE *file)
{
    Serial.write(c);
    return 0 ;
}

// input function
static int uart_getchar(FILE *file)
{
	return Serial.available() ? Serial.read() : -1;
}

void setup()
{
	Serial.begin(115200);
	Serial.println(F(CLEAR_TERMINAL "UART READY"));
	// fill in the UART file descriptor with pointer to writer
	fdev_setup_stream(&uart, uart_putchar, uart_getchar, _FDEV_SETUP_RW);

	programmer = new Programmer(&uart);
}

void loop()
{
	programmer->task();
}



