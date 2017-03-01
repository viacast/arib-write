#pragma once

#include <stdio.h>
#include <stdint.h>

enum SegType
{
	FULL_SEG,
	ONE_SEG,
};
typedef enum SegType SegType;
extern SegType seg_type;

void PES_write(FILE *out, size_t payload_size,
	const uint8_t *payload);
