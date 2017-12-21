#include "xmodem.h"

XModem::XModem(Stream & terminal)
{
	this->retries = 10;
	this->terminal = &terminal;
	reset();
}

void XModem::reset(void)
{
	this->block = 0;
}

uint8_t XModem::octet_available(void) 
{
	return terminal->available() > 0;
}

void XModem::crc_update(uint8_t octet)
{
	crc ^= (uint16_t)octet << 8;
	for (uint8_t i = 0; i < 8; i++) {
		if (crc & 0x8000) {
			crc = crc << 1 ^ 0x1021;
		}
		else {
			crc <<= 1;
		}
	}
}

void XModem::octet_send(uint8_t octet)
{
	terminal->write(octet);
}

uint8_t XModem::octet_receive(void)
{
	#define DELAY_TIMEOUT ((16000000 / (100)) * 7) // 7 seconds timeout
	#if DELAY_TIMEOUT > 0xFFFFFFFFull
		#error TIMEOUT overflow
	#endif
	uint32_t timeout = DELAY_TIMEOUT; 
	#undef DELAY_TIMEOUT
	
	while (--timeout) {
		if (octet_available()) {
			uint8_t octet = terminal->read();
			timeout_flag = 0;
			return octet;
		}
	}
	
	timeout_flag = 1;
	return 0;
}

enum XModem::block_result XModem::start_send(void)
{
	int last = retries;

	reset();

	do {
		uint8_t octet = octet_receive();
		if (!timeout_flag) {
			if (XSTART == octet) {
				return START;
			}
			else {
				return CANCEL;
			}
		}
	} while (--last);

	return TIMEOUT;
}

enum XModem::block_result XModem::finish_send(void)
{
	uint8_t octet;

	octet_send(EOT);

	octet = octet_receive();
	if (timeout_flag) {
		return TIMEOUT;
	}
	if (ACK != octet) {
		return CANCEL;
	}

	octet_send(ETB);
	octet_receive();

	return END;
}

enum XModem::block_result XModem::block_send(uint8_t *dst)
{
	int last = retries;
	uint8_t i, octet;

start:
	crc = 0;
	octet_send(SOH);
	// octets are numbered from 1
	octet = (uint8_t)block + 1;
	octet_send(octet);
	octet_send(255 - octet);

	for (i = 0; i < XMODEM_BLOCK_SIZE; i++) {
		uint8_t octet = *dst++;
		octet_send(octet);
		crc_update(octet);
		if (octet_available()) {
			if (terminal->read() == CAN) {
				return CANCEL;
			}
		}
	}

	octet_send(crc >> 8);
	octet_send(crc);

	octet = octet_receive();
	if ((octet == NACK) && (--last)) {
		goto start;
	}
	if (octet != ACK) {
		if (octet == CAN) {
			return CANCEL;
		}
		return ERROR;
	}

	block++;
	return NEXT;
}

enum XModem::block_result XModem::block_receive(uint8_t *dst)
{
	int last = retries;
	uint8_t i, octet, receive;
	uint8_t preamble = block ? ACK : XSTART;

start:
	// for the first block send 'C', for others send ACK
	octet_send(preamble);

	crc = 0;

	octet = octet_receive();
	if (timeout_flag) {
		if (--last) {
			// wait for sender/request resend
			goto start;
		}
		return TIMEOUT;
	}

	// if done
	if (EOT == octet) {
		octet_send(ACK);
		return END;
	}

	// if cancelled
	if (CAN == octet) {
		return CANCEL;
	}

	// skip spoiled packet
	if (SOH != octet) {
skip_nak:
		preamble = NACK;
		do {
			octet_receive();
		} while (!timeout_flag);
		goto start;
	}

	// get block number
	i = octet_receive();
	if (timeout_flag) {
restart:
		if (--last) {
			// wait for sender/request resend
			preamble = NACK;
			goto start;
		}
		return TIMEOUT;
	}

	// get block number complement
	octet = octet_receive();
	if (timeout_flag) {
		goto restart;
	}

	// check valid value
	if ((uint8_t)(255 - octet) != i) {
		goto skip_nak;
	}

	// check block number
	if (i == (uint8_t)block) {
		receive = 0;
	}
	else if (i == (uint8_t)(block + 1)) {
		receive = 1;
	}
	else {
		return CANCEL;
	}

	// receive data
	for (i = 0; i < XMODEM_BLOCK_SIZE; i++) {
		octet = octet_receive();
		crc_update(octet);
		if (timeout_flag) {
			goto restart;
		}
		if (receive) {
			*dst++ = octet;
		}
	}

	// receive checksum
	uint16_t line_crc = octet_receive() << 8;
	if (timeout_flag) {
		goto restart;
	}
	line_crc |= octet_receive();
	if (timeout_flag) {
		goto restart;
	}

	// compare checksum
	if (crc != line_crc) {
		goto skip_nak;
	}

	preamble = ACK;

	if (!receive) {
		goto start;
	}

	block++;
	last = retries;

	return NEXT;
}
