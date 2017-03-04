#include <stdlib.h>
#include <assert.h>
#include "crc-16.h"

#define           poly     0x1021

/* based on CRC code found at
http://srecord.sourceforge.net/crc16-ccitt.html
*/
uint16_t gen_crc16(const uint8_t *data, uint16_t len)
{
	//uint16_t crc = (((uint16_t)data[0]) << 8) | data[1];
	uint16_t crc = 0x0000;
	unsigned short i, v, xor_flag;

	for(size_t c = 0; c < len; ++c) 
	{
		uint8_t ch = data[c];

		/*
		Align test bit with leftmost bit of the message byte.
		*/
		v = 0x80;

		for (i=0; i<8; i++)
		{
			if (crc & 0x8000)
			{
				xor_flag= 1;
			}
			else
			{
				xor_flag= 0;
			}
			crc = crc << 1;

			if (ch & v)
			{
				/*
				Append next bit of message to end of CRC if it is not zero.
				The zero bit placed there by the shift above need not be
				changed if the next bit of the message is zero.
				*/
				crc = crc + 1;
			}

			if (xor_flag)
			{
				crc = crc ^ poly;
			}

			/*
			Align test bit with next bit of the message byte.
			*/
			v = v >> 1;
		}
	}

	for (i=0; i<16; i++)
    {
        if (crc & 0x8000)
        {
            xor_flag= 1;
        }
        else
        {
            xor_flag= 0;
        }
        crc = crc << 1;

        if (xor_flag)
        {
            crc = crc ^ poly;
        }
    }

	return crc;
}
