#pragma once

#include <stdio.h>
#include <stdint.h>

// Linked list of arrays to use as buffer.
struct Buffer
{
	size_t total_size;
	struct BufferLink *head;
	struct BufferLink **last;
};
typedef struct Buffer Buffer;

//! Assumes Buffer is deallocated.
uint8_t *buffer_init(Buffer *buf, const size_t size);

void buffer_destroy(Buffer *buf);

uint8_t *buffer_append(Buffer *buf, size_t size);
uint8_t *buffer_prepend(Buffer *buf, size_t size);

void buffer_write(const Buffer *buf, FILE *out);

void buffer_chop_head(Buffer *buf, size_t size, Buffer *head);

size_t buffer_get_size(Buffer *buf);

uint16_t buffer_CRC16(Buffer *buf);
