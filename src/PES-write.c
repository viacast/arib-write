
#include <math.h>
#include <assert.h>
#include <arpa/inet.h>
#include <string.h>

#include "timer.h"

#include "PES-write.h"

SegType seg_type = FULL_SEG;

static void set_PTS(uint8_t *out)
{
	static double ref_time = 0.0;
	uint64_t pts;
	if(ref_time != 0.0) {
		const double diff = time_now() - ref_time;
		pts = round(diff * 90000);
	} else {
		ref_time = time_now();
		pts = 0;
	}

	out[0] = 0b00100001 | (0b00001110 & (pts >> 29));

	out[1] = 0xff & (pts >> 22);
	out[2] = 1 | (0b11111110 & (pts >> 14));

	out[3] = 0xff & (pts >> 7);
	out[4] = 1 | (0b11111110 & (pts << 1));
}

static void PES_packet(Buffer *data)
{
	const size_t payload_size = buffer_get_size(data);

	// According to operating guidelines ARIB TR-B14, Fascicle 2, Section 4.2.2,
	// PES maximum size must be 32 KB. Since header size must be always 35 bytes
	// (according to ARIB STD-B37, Section 2.2.3.6 (3)), this leaves for payload:
	// 32 KB - 35 bytes = 32733 bytes.
	assert(payload_size <= 32733);

	uint8_t *buf = buffer_prepend(data, 35);

	// From ISO 13818-1, section 2.4.3.6, PES packet:

	// packet_start_code_prefix
	memcpy(&buf[0], "\x00\x00\x01", 3);

	// stream_id == private_stream_1
	// as specified in ARIB STD B24 Volume 3, section 5.1
	buf[3] = 0xBD;

	// PES_packet_length
	const uint16_t length = htons(29 + payload_size);
	memcpy(&buf[4], &length, sizeof length);

	// '10', PES_scrambling_control (not scrambled),
	// PES_priority (normal priority), data_alignment_indicator (no),
	// copyright (no), original_or_copy (copy)
	buf[6] = 0b10000000;

	// PTS_DTS_flags (PTS only), ESCR_flag (no),
	// ES_rate_flag (no), DSM_trick_mode_flag (no),
	// additional_copy_info_flag (no), PES_CRC_flag (no TODO)
	// PES_extension_flag (yes)
	buf[7] = 0b10000001;

	// PES_header_data_length
	// The number of bytes following in PES header,
	// up to stuffing_byte.
	buf[8] = 23;

	// PTS data
	set_PTS(&buf[9]);

	// PES_private_data_flag (yes), pack_header_field_flags (no),
	// program_packet_sequence_counter_flag (no), P-STD_buffer_flag (no),
	// '111', PES_extension_flag_2 (no)
	buf[14] = 0b10001110;

	// PES_private_data, as defined in ABNT NBR 15608-3:2008
	switch(seg_type) {
	case FULL_SEG:
		// Section A.1, unused PES_private_data
		for(uint8_t i = 15; i < 31; ++i) {
			buf[i] = 0xff;
		}
		break;
	case ONE_SEG:
		// Section A.2, PES_private_data as CIS:
		// CCIS_code
		memcpy(&buf[15], "CCIS", 4);

		// Caption_conversion_type (mobile)
		buf[16] = 0x04;

		// DRCS_conversion_type (mobile DRCS), '111111'
		buf[17] = 0b10111111;

		// Unused 10 bytes
		for(uint8_t i = 18; i < 31; ++i) {
			buf[i] = 0xff;
		}
	}

	// stuffing_byte
	buf[31] = 0xff;

	// Multiple PES_packet_data_byte:
	// This the actual PES payload, the following is defined in:
	// - ABNT NBR 15608-3:2008, sections A.1 and A.2
	// - ARIB STD B37, Table 2-25
	
	// Data_identifier:
	buf[32] = 0x80;

	// Private_stream_id:
	buf[33] = 0xff;

	// Reserverd ('1111'), PES_data_packet_header_length ('0000')
	buf[34] = 0b11110000;
}

void PES_packetize(Buffer *data)
{
	/* // TODO fix packetization...
	while(buffer_get_size(data) > 32733) {
		Buffer head;
		buffer_chop_head(data, 32733, &head);
		PES_packet(&head);
		buffer_destroy(&head);
	} */

	assert(buffer_get_size(data) <= 32733);
	PES_packet(data);
}
