#include <stdbool.h>
#include <string.h>
#include <arpa/inet.h>
#include <assert.h>

#include "data-group.h"

#include "crc-16.h"
#include "PES-write.h"

static void write_data_group_packet(FILE *out, uint8_t header,
	uint8_t link_number, uint8_t last_link_number,
	uint16_t size, const uint8_t *data)
{
	uint8_t buf[size + 7];

	// data_group_id, data_group_version
	buf[0] = header;

	// data_group_link_number
	buf[1] = link_number;

	// last_data_group_link_number
	buf[2] = last_link_number;

	// data_group_size, in big-endian
	buf[3] = size >> 8;
	buf[4] = size & 0xff;

	// Bytes in data_group_data_byte
	memcpy(&buf[5], data, size);

	// CRC_16
	uint16_t crc = ntohs(gen_crc16(data, size + 5));
	memcpy(&buf[size+5], &crc, sizeof crc);

	PES_write(out, sizeof buf, buf);
}

static void data_group_packetize(FILE *out, CaptionDataType cdt,
	size_t size, const uint8_t *data)
{
	// Assemble data group chain as described in ARIB STD-B24, Chapter 9
	static bool groupB = false;
	static uint8_t version = 0;

	if(cdt == NEW_MANAGEMENT) {
		groupB = !groupB;
		version = (version + 1) & 0b11;
		cdt = OLD_MANAGEMENT;
	}

	uint8_t data_group_id = (uint8_t)cdt;
	if(groupB) {
		data_group_id += 0x20;
	}

	assert(size / UINT16_MAX < 256
			|| (size / UINT16_MAX == 256 && size % UINT16_MAX == 0));

	const uint8_t header = (data_group_id << 2) | version;

	const uint8_t last_piece = size / UINT16_MAX - 1
		+ ((size % UINT16_MAX) != 0);

	for(uint8_t i = 0; size > 0; ++i) {
		const uint16_t piece_size = size > UINT16_MAX ? UINT16_MAX : size;
		write_data_group_packet(out, header, i, last_piece, piece_size, data);
		size -= piece_size;
		data += piece_size;
	}
	// TODO: ensure the 2 CRC bytes in caption data_group is
	// not split by TS packetizer.
}

static void set_3_byte_data(uint8_t *to, uint32_t value)
{
	// Assert we are not discarding non-null bits
	assert(value < (1 << 24));

	// Most significant to least significant byte
	to[0] = (value >> 16) & 0xff;
	to[1] = (value >> 8) & 0xff;
	to[2] = value & 0xff;
}

void write_caption_management_data(FILE *out, CaptionDataType cd_type,
	uint32_t data_size, const uint8_t *data)
{
	// Struct from ARIB STD-B24, Table 9-3
	assert(cd_type == NEW_MANAGEMENT || cd_type == OLD_MANAGEMENT);

	uint8_t buf[data_size + 10];

	// TMD (free), '111111'
	buf[0] = 0b00111111;

	// num_languages
	buf[1] = 1;

	// language_tag (0), '1', DMF (selectable, selectable)
	buf[2] = 0b00011010;

	// ISO_639_language_code
	memcpy(&buf[3], "por", 3);

	// Format (horizontal 960 x 540), TCS (8bit-code), rollup_mode (Roll-up)
	buf[6] = 0b10000001;

	// data_unit_loop_length
	set_3_byte_data(&buf[7], data_size);

	// data_unit bytes
	memcpy(&buf[10], data, data_size);

	data_group_packetize(out, cd_type, sizeof buf, buf);
}

static void write_caption_statement_data(FILE *out, CaptionDataType cd_type,
	uint32_t data_size, const uint8_t *data)
{
	// Struct from ARIB STD-B24, Table 9-10
	assert(cd_type != NEW_MANAGEMENT && cd_type != OLD_MANAGEMENT);
	assert(data_size > 0);

	uint8_t buf[data_size + 4];

	// TMD (free), '111111'
	buf[0] = 0b00111111;

	// data_unit_loop_length
	set_3_byte_data(&buf[1], data_size);

	// data_unit bytes
	memcpy(&buf[4], data, data_size);

	data_group_packetize(out, cd_type, sizeof buf, buf);
}

void write_data_unit(FILE *out, CaptionDataType cd_type,
	DataUnitType du_type, uint32_t data_size, const uint8_t *data)
{
	// Struct from ARIB STD-B24, Table 9-11

	uint8_t buf[data_size + 5];

	// unit_separator
	buf[0] = 0x1f;

	// data_unit_parameter
	buf[1] = du_type;

	// data_unit_size
	set_3_byte_data(&buf[2], data_size);

	// Bytes in data_unit_data_byte
	memcpy(&buf[5], data, data_size);

	if(cd_type == NEW_MANAGEMENT || cd_type == OLD_MANAGEMENT) {
		write_caption_management_data(out, cd_type, sizeof buf, data);
	} else {
		write_caption_statement_data(out, cd_type, sizeof buf, data);
	}
}
