#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>

#include "timer.h"

#include "data-group.h"

// Encodes a control sequence, which are commands
// preceeded by Control Sequence Introducer (CSI).
static size_t encode_cs(uint8_t *to, uint8_t final,
	const char *data)
{
	size_t i = 0;

	to[i++] = 0x9b;
	for(; *data; ++data) {
		to[i++] = *data;
	}
	to[i++] = 0x20;
	to[i++] = final;

	return i;
}

static void write_caption_management_data(FILE *out)
{
	Buffer data;

	buffer_init(&data, 0);
	caption_management_data(OLD_MANAGEMENT, &data);

	// This packet should have small fixed size below 184 bytes
	// and cause no trouble with divided CRC bytes.
	assert(buffer_get_size(&data) <= 184);
	buffer_write(&data, out);

	buffer_destroy(&data);
}

static void subtitle_boilerplate(Buffer *data)
{
	uint8_t *buf = buffer_prepend(data, 53);

	size_t i = 0;

	// Control codes are defined in both ABNT NBR 15606-1, Tabela 13
	// and ARIB STD-B24, Table 7-14.
	// The semantics are specified in ARIB STD-B24, Table 7-15 and 7-16

	// CS (clear screen)
	buf[i++] = 0x0c;

	// The following are commands started by code
	// CSI (control sequence introducer),
	// and are defined in ARIB STD-B24, Table 7-17.

	// Set Writing Format (SWF):
	i += encode_cs(&buf[i], 0x53, "7");

	// Set Display Position (SDP):
	i += encode_cs(&buf[i], 0x5f, "138;100");

	// Set Display Format (SDF):
	i += encode_cs(&buf[i], 0x56, "684;390");

	// Character composition dot designation (SSM)
	i += encode_cs(&buf[i], 0x57, "36;36");
	
	// Set Horizontal Spacing (SHS):
	i += encode_cs(&buf[i], 0x58, "2");

	// Set Vertical Spacing (SVS):
	i += encode_cs(&buf[i], 0x59, "16");
	
	// Raster Colour Command (RCS):
	i += encode_cs(&buf[i], 0x6e, "8");

	// SSZ (small size)
	buf[i++] = 0x88;

	// WHF (white foreground)
	buf[i++] = 0x87;

	// COL (colour controls), background color - black
	buf[i++] = 0x90;
	buf[i++] = 0x50;

	// APS (active position set), set to (13,0) (0x4d, 0x40)
	buf[i++] = 0x1c;
	buf[i++] = 0x4d;
	buf[i++] = 0x40;

	assert(i == 53);

	data_unit(STATEMENT_1, STATEMENT_BODY, data);
}

static void write_full_subtitle(FILE *out,
	const size_t msg_size, const char *const msg)
{
	Buffer data;

	// Ensures the 2 CRC bytes in caption data_group is
	// not split by the TS packetizer.
	uint8_t padding = 0;
	for(;;) {
		uint8_t *buf = buffer_init(&data, msg_size + padding);
		memcpy(buf, msg, msg_size);
		memset(buf + msg_size, 0, padding);

		subtitle_boilerplate(&data);

		if((buffer_get_size(&data) % 184) != 1) {
			break;
		}

		buffer_destroy(&data);
		++padding;
	}

	// Also according ARIB TR-B14, Fascicle 2, Section 4.2.2,
	// minimum interval between PES packets is 100 ms, 
	// so if last time a PES packet was sent is less than
	// 100 ms, sleep through the time difference.
	static double last_time = 0.0;
	{
		double diff = time_now() - last_time;
		if(diff < 0.100) {
			sleep_for(0.100 - diff);
		}
	}
	buffer_write(&data, out);

	last_time = time_now();

	buffer_destroy(&data);
}

int main()
{
	for(size_t i = 0;; ++i) {
		write_caption_management_data(stdout);
		if(i % 10 == 1) {
			char msg[50];
			snprintf(msg, 50, "\x1d\x21 pele de cobra %zd \x1d\x21", i/10);
			write_full_subtitle(stdout, strlen(msg), msg);
		}
		sleep(1);
	}
}
