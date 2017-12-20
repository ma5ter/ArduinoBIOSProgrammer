#ifndef __XMODEM_H__
#define __XMODEM_H__

#include "Stream.h"

#define XSTART 'C'
#define XMODEM_SOH 0x01
#define XMODEM_ACK 0x06
#define XMODEM_EOT 0x04
#define XMODEM_NACK 0x15
#define XMODEM_ETB 0x17
#define XMODEM_CAN 0x18
#define XMODEM_SUB 0x1A
#define XMODEM_BLOCK_SIZE 128

typedef unsigned short xm_size_t;

extern char xm_preamble;
extern unsigned char xm_blk;
extern char xm_result;

char xm_receive_char(char *dst);
char xm_receive_block(char *dst);
xm_size_t xm_receive(char *dst, xm_size_t size);

class XModem {
private:
	// serial terminal stream
	Stream * terminal;

	uint8_t timeout_flag;
	uint32_t block;
	uint16_t crc;

	void crc_update(uint8_t octet);
	void octet_send(uint8_t octet);
	uint8_t octet_available(void);
	uint8_t octet_receive(void);

public:
	static const uint8_t NACK = XMODEM_NACK;
	static const uint8_t ACK = XMODEM_ACK;
	static const uint8_t SOH = XMODEM_SOH;
	static const uint8_t EOT = XMODEM_EOT;
	static const uint8_t ETB = XMODEM_ETB;
	static const uint8_t CAN = XMODEM_CAN;
	
	enum block_result {
		START,
		END,
		NEXT,
		CANCEL,
		TIMEOUT,
		ERROR
	};

	int retries;

	XModem(Stream & terminal);

	void reset(void);

	enum block_result start_send(void);
	enum block_result finish_send(void);
	enum block_result block_send(uint8_t *dst);
	enum block_result block_receive(uint8_t *dst);
};


#endif
