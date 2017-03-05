#pragma once

#include <stdio.h>
#include <stdint.h>

#include "buffer.h"

enum SegType
{
	FULL_SEG,
	ONE_SEG,
};
typedef enum SegType SegType;
extern SegType seg_type;

void PES_write(FILE *out, Buffer *data);
