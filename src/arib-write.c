#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <iconv.h>

#include "timer.h"
#include "PES-write.h"
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

static void *caption_writer_thread(void *par)
{
	FILE *out = par;
	for(;;) {
		write_caption_management_data(out);
		sleep(1);
	}
	return NULL;
}

static void subtitle_boilerplate(Buffer *data)
{
	uint8_t *buf = buffer_prepend(data, 50);

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
	/*buf[i++] = 0x1c;
	buf[i++] = 0x4d;
	buf[i++] = 0x40;*/

	assert(i == 50);

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

static size_t getline(char *buf, size_t size, FILE *f)
{
	size_t count = 0;
	while(count < size) {
		const int c = getc(f);
		if(c == EOF) {
			break;
		}

		buf[count++] = c;
		if(c == '\n') {
			break;
		}
	}
	buf[count] = 0;
	return count;
}

static void spawn_caption_writer()
{
	pthread_t cwriter;
	pthread_create(&cwriter, NULL, caption_writer_thread, stdout);
	pthread_detach(cwriter);
	sleep_for(0.5);
}

int main(int argc, char *argv[])
{
	extern SegType seg_type;
	uint8_t debug = 0;
	for(int i = 1; i < argc; ++i) {
		if(strcmp(argv[i], "--one-seg") == 0) {
			seg_type = ONE_SEG;
		} else if(strcmp(argv[i], "-d") == 0) {
			debug = 1;
		}
	}

	fprintf(stderr, "Generating %s-seg PES.\n",
		seg_type == ONE_SEG ? "one" : "full");

	if(debug) {
		fputs("Debug mode.\n", stderr);
	}

	spawn_caption_writer();

	iconv_t cd = iconv_open("l1", "utf8");

	for(;;) {
		char orig[2][4096] = {{0}, {0}};
		char msg[4096];
		uint16_t count = 0;
		uint8_t line_count = 0;
		while(line_count < 2)
		{
			uint16_t ncount = count;
			msg[ncount++] = 0x1c;
			msg[ncount++] = 0x4d + line_count;
			msg[ncount++] = 0x40;
			size_t n;
			{
				size_t remsize = 4095 - ncount;
				char *buf = orig[line_count];
				size_t bn = getline(buf, remsize, stdin);
				if(bn == 0) {
					return 1;
				}

				char *in = buf;
				char *out = &msg[ncount];
				iconv(cd, &in, &bn, &out, &remsize);
				n = out - &msg[ncount];
			}
			if(msg[ncount] == '\n') {
				if(line_count == 0) {
					continue;
				} else {
					break;
				}
			}
			count = ncount + n;
			++line_count;
		}
		if(debug) {
			fprintf(stderr, "Sending subtitle:\n%s%s\n", orig[0], orig[1]);
		}
		write_full_subtitle(stdout, count, msg);
	}
}
