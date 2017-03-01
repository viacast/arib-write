#include <stdlib.h>
#include "crc-16.h"

//Tested
#define CRC16_DNP	0x3D65		// DNP, IEC 870, M-BUS, wM-BUS, ...
#define CRC16_CCITT	0x1021		// X.25, V.41, HDLC FCS, Bluetooth, ...

//Other polynoms not tested
#define CRC16_IBM	0x8005		// ModBus, USB, Bisync, CRC-16, CRC-16-ANSI, ...
#define CRC16_T10_DIF	0x8BB7		// SCSI DIF
#define CRC16_DECT	0x0589		// Cordeless Telephones
#define CRC16_ARINC	0xA02B		// ACARS Aplications

#define POLYNOM		CRC16_CCITT   // Define the used polynom from one of the aboves

static uint16_t crc16(uint16_t crcValue, uint8_t newByte) 
{
	for (uint8_t i = 0; i < 8; i++) {

		if (((crcValue & 0x8000) >> 8) ^ (newByte & 0x80)){
			crcValue = (crcValue << 1)  ^ POLYNOM;
		}else{
			crcValue = (crcValue << 1);
		}

		newByte <<= 1;
	}
  
	return crcValue;
}

uint16_t gen_crc16(const uint8_t *Data, uint16_t len)
{
	unsigned int crc;
	unsigned char aux = 0;

	crc = 0x0000; //Initialization of crc to 0x0000 for DNP
	//crc = 0xFFFF; //Initialization of crc to 0xFFFF for CCITT

	while (aux < len){
		crc = crc16(crc,Data[aux]);
		aux++;
	}

	//return (~crc); //The crc value for DNP it is obtained by NOT operation

	return crc; //The crc value for CCITT
}
