#pragma once

#include <stdio.h>
#include <stdint.h>
#include "buffer.h"

enum CaptionDataType
{
	OLD_MANAGEMENT = 0x00,
	STATEMENT_1 = 0x01,
	STATEMENT_2 = 0x02,
	STATEMENT_3 = 0x03,
	STATEMENT_4 = 0x04,
	STATEMENT_5 = 0x05,
	STATEMENT_6 = 0x06,
	STATEMENT_7 = 0x07,
	STATEMENT_8 = 0x08,
	NEW_MANAGEMENT
};
typedef enum CaptionDataType CaptionDataType;

enum DataUnitType
{
	STATEMENT_BODY = 0x20,
	GEOMETRIC = 0x28,
	SINTHESIZED_SOUND = 0x2c,
	ONE_BYTE_DRCS = 0x30,
	TWO_BYTE_DRCS = 0x31,
	COLOR_MAP = 0x34,
	BIT_MAP = 0x35
};
typedef enum DataUnitType DataUnitType;

void write_caption_management_data(FILE *out, CaptionDataType cd_type,
	Buffer *data);

void write_data_unit(FILE *out, CaptionDataType cd_type,
	DataUnitType du_type, Buffer *data);
