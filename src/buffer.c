#include "buffer.h"

#include <assert.h>
#include <string.h>
#include <stdlib.h>

struct BufferLink
{
	struct BufferLink *next;
	size_t size;
	uint8_t data[];
};
typedef struct BufferLink BLink;

static BLink *alloc_blink(const size_t size)
{
	BLink *ret = malloc((sizeof *ret) + size);
	ret->size = size;
	ret->next = NULL;

	return ret;
}

uint8_t *buffer_init(Buffer *const buf, const size_t size)
{
	buf->total_size = size;

	BLink *blink = alloc_blink(size);

	buf->head = blink;
	buf->last = &blink->next;

	return blink->data;
}

void buffer_destroy(Buffer *const buf)
{
	BLink *l = buf->head;
	while(l) {
		BLink *next = l->next;
		free(l);
		l = next;
	}
	memset(buf, 0, sizeof *buf);
}

uint8_t *buffer_append(Buffer *const buf, const size_t size)
{
	BLink *l = alloc_blink(size);
	*buf->last = l;
	buf->last = &l->next;

	buf->total_size += size;

	return l->data;
}

uint8_t *buffer_prepend(Buffer *const buf, const size_t size)
{
	BLink *l = alloc_blink(size);
	l->next = buf->head;
	buf->head = l;

	buf->total_size += size;

	return l->data;
}

void buffer_write(const Buffer *const buf, FILE *out)
{
	BLink *l = buf->head;
	while(l) {
		fwrite(l->data, 1, l->size, out);
		l = l->next;
	}

	fflush(out);
}

void buffer_chop_head(Buffer *buf, size_t size, Buffer *head)
{
	assert(buf->total_size <= size);

	head->total_size = size;
	buf->total_size -= size;

	BLink *l = head->head = buf->head;
	size_t count = l->size;
	
	while(count < size) {
		l = l->next;
		count += l->size;
	}

	const size_t diff = count - size;

	if(diff || !buf->total_size) {
		BLink *nl = alloc_blink(diff);
		l->size -= diff;
		memcpy(nl->data, l->data + l->size, diff);
		realloc(l, l->size + sizeof *l);

		nl->next = l->next;
		buf->head = nl;
		buf->last = &nl->next;
	} else {
		assert(l->next);
		buf->head = l->next;
	}

	l->next = NULL;
	head->last = &l->next;
}

size_t buffer_get_size(Buffer *buf)
{
	return buf->total_size;
}

uint16_t buffer_CRC16(Buffer *buf)
{
	const uint16_t poly = 0x1021;

	//uint16_t crc = (((uint16_t)data[0]) << 8) | data[1];
	uint16_t crc = 0x0000;
	unsigned short i, v, xor_flag;

	BLink *l = buf->head;

	while(l) {
		for(size_t c = 0; c < l->size; ++c) 
		{
			uint8_t ch = l->data[c];

			/*
			Align test bit with leftmost bit of the message byte.
			*/
			v = 0x80;

			for (i=0; i<8; i++)
			{
				if (crc & 0x8000)
				{
					xor_flag= 1;
				}
				else
				{
					xor_flag= 0;
				}
				crc = crc << 1;

				if (ch & v)
				{
					/*
					Append next bit of message to end of CRC if it is not zero.
					The zero bit placed there by the shift above need not be
					changed if the next bit of the message is zero.
					*/
					crc = crc + 1;
				}

				if (xor_flag)
				{
					crc = crc ^ poly;
				}

				/*
				Align test bit with next bit of the message byte.
				*/
				v = v >> 1;
			}
		}
		l = l->next;
	}

	for (i=0; i<16; i++)
    {
        if (crc & 0x8000)
        {
            xor_flag= 1;
        }
        else
        {
            xor_flag= 0;
        }
        crc = crc << 1;

        if (xor_flag)
        {
            crc = crc ^ poly;
        }
    }

	return crc;

}
