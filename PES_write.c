#include <time.h>
#include <math.h>
#include <assert.h>
#include <arpa/inet.h>

#include "PES_write.h"

SegType seg_type = FULL_SEG;

static double time_now()
{
	struct timespec t;
	clock_gettime(CLOCK_MONOTONIC_RAW, &t);

	return t.tv_sec + (1e-9 * t.tv_nsec);
}

static void sleep_for(const double duration)
{
	struct timespec req;
	struct timespec rem;

	double full_secs;
	req.tv_nsec = (long)(modf(duration, &full_secs) * 1e9);
	req.tv_sec = (time_t)full_secs;

	while(nanosleep(&req, &rem) != 0) {
		req = rem;
	}
}

static void write_PTS(FILE *out)
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

	putc( 0b00100001 |
		 (0b00001110 & (pts >> 29)),
		 out);

	putc(0xff & (pts >> 22), out);
	putc(1 | (0b11111110 & (pts >> 14)), out);

	putc(0xff & (pts >> 7), out);
	putc(1 | (0b11111110 & (pts << 1)), out);
}

static void write_single_PES(FILE *out, uint16_t payload_size, const uint8_t *payload)
{
	// According to operating guidelines ARIB TR-B14, Fascicle 2, Section 4.2.2,
	// PES maximum size must be 32 KB. Since header size must be always 35 bytes
	// (according to ARIB STD-B37, Section 2.2.3.6 (3)), this leaves for payload:
	// 32 KB - 35 bytes = 32733 bytes.
	assert(payload_size <= 32733);

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

	// From ISO 13818-1, section 2.4.3.6, PES packet:

	// packet_start_code_prefix
	fwrite("\x00\x00\x01", 1, 3, out);

	// stream_id == private_stream_1
	// as specified in ARIB STD B24 Volume 3, section 5.1
	putc(0xBD, out);

	// PES_packet_length
	const uint16_t length = htons((3 + 23 + 3) + payload_size);
	fwrite(&length, 2, 1, out);

	// '10', PES_scrambling_control (not scrambled),
	// PES_priority (normal priority), data_alignment_indicator (no),
	// copyright (no), original_or_copy (copy)
	putc(0b10000000, out);

	// PTS_DTS_flags (PTS only), ESCR_flag (no),
	// ES_rate_flag (no), DSM_trick_mode_flag (no),
	// additional_copy_info_flag (no), PES_CRC_flag (no TODO)
	// PES_extension_flag (yes)
	putc(0b10000001, out);

	// PES_header_data_length
	// The number of bytes following in PES header,
	// up to stuffing_byte.
	putc(23, out);

	// PTS data
	write_PTS(out);

	// PES_private_data_flag (yes), pack_header_field_flags (no),
	// program_packet_sequence_counter_flag (no), P-STD_buffer_flag (no),
	// '111', PES_extension_flag_2 (no)
	putc(0b10001110, out);

	// PES_private_data, as defined in ABNT NBR 15608-3:2008
	switch(seg_type) {
	case FULL_SEG:
		// Section A.1, unused PES_private_data
		for(uint8_t i = 0; i < 16; ++i) {
			putc(0xff, out);
		}
		break;
	case ONE_SEG:
		// Section A.2, PES_private_data as CIS:
		// CCIS_code
		fwrite("CCIS", 1, 4, out);

		// Caption_conversion_type (mobile)
		putc(0x04, out);

		// DRCS_conversion_type (mobile DRCS), '111111'
		putc(0b10111111, out);

		// Unused 10 bytes
		for(uint8_t i = 0; i < 10; ++i) {
			putc(0xff, out);
		}
	}

	// stuffing_byte
	putc(0xff, out);

	// Multiple PES_packet_data_byte:
	// This the actual PES payload, the following is defined in:
	// - ABNT NBR 15608-3:2008, sections A.1 and A.2
	// - ARIB STD B37, Table 2-25
	
	// Data_identifier:
	putc(0x80, out);

	// Private_stream_id:
	putc(0xff, out);

	// Reserverd ('1111'), PES_data_packet_header_length ('0000')
	putc(0b11110000, out);

	// Copy the payload
	fwrite(payload, 1, payload_size, out);

	fflush(out);
	last_time = time_now();
}

void PES_write(FILE *out, size_t payload_size,
	const uint8_t *payload)
{
	while(payload_size > 0) {
		const uint16_t sendsize = (payload_size > 32733)
			? 32733 : payload_size;
		write_single_PES(out, sendsize, payload);
		payload += sendsize;
		payload_size -= sendsize;
	}
}
