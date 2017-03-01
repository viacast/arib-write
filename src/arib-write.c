#include <assert.h>
#include <string.h>
#include <unistd.h>
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

void write_full_subtitle(FILE *out,
	size_t msg_size, const uint8_t *msg)
{
	uint8_t buf[msg_size + 53];
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

	memcpy(&buf[i], msg, msg_size);

	write_data_unit(out, STATEMENT_1, STATEMENT_BODY, sizeof buf, buf);
}

int main()
{
	for(size_t i = 0;; ++i) {
		write_caption_management_data(stdout, OLD_MANAGEMENT, 0, NULL);
		if(i % 10 == 1) {
			char msg[50];
			snprintf(msg, 50, "\x1d\x21 pele de cobra %zd \x1d\x21\n", i/10);
			write_full_subtitle(stdout, strlen(msg), msg);
		}
		sleep(1);
	}	
}
