/**
* +----------------------------------------------------------------------+
* | This file is part of the Ayyi project. http://ayyi.org               |
* | copyright (C) GTK+ Team and others                                   |
* +----------------------------------------------------------------------+
* | This program is free software; you can redistribute it and/or modify |
* | it under the terms of the GNU General Public License version 3       |
* | as published by the Free Software Foundation.                        |
* +----------------------------------------------------------------------+
*
*/
#include "agl/debug.h"
#include "text/opbuffer.h"

static guint op_sizes[OP_LAST] = {
  0,
  sizeof (OpOpacity),
  sizeof (OpColor),
  sizeof (OpMatrix),
  sizeof (OpMatrix),
  sizeof (OpProgram),
  sizeof (OpRenderTarget),
  sizeof (OpClip),
  sizeof (OpViewport),
  sizeof (OpTexture),
  sizeof (OpRepeat),
  sizeof (OpLinearGradient),
  sizeof (OpColorMatrix),
  sizeof (OpBlur),
  sizeof (OpShadow),
  sizeof (OpShadow),
  sizeof (OpBorder),
  sizeof (OpBorder),
  sizeof (OpBorder),
  sizeof (OpCrossFade),
  sizeof (OpShadow),
  0,
  sizeof (OpDraw),
  sizeof (OpDumpFrameBuffer),
  sizeof (OpDebugGroup),
  0,
  sizeof (OpBlend),
};


void
op_buffer_init (OpBuffer* buffer)
{
	static gsize initialized = FALSE;

	if (g_once_init_enter (&initialized)) {
		for (int i = 0; i < G_N_ELEMENTS (op_sizes); i++) {
			guint size = op_sizes[i];

			if (size > 0) {
				/* Round all op entry sizes to the nearest 16 to ensure
				 * that we guarantee proper alignments for all op entries.
				 * This is only done once on first use.
				 */
				#define CHECK_SIZE(s) else if (size < (s)) { size = s; }
				if (0) {}
				CHECK_SIZE (16)
				CHECK_SIZE (32)
				CHECK_SIZE (48)
				CHECK_SIZE (64)
				CHECK_SIZE (80)
				CHECK_SIZE (96)
				CHECK_SIZE (112)
				CHECK_SIZE (128)
				CHECK_SIZE (144)
				CHECK_SIZE (160)
				CHECK_SIZE (176)
				CHECK_SIZE (192)
				else g_assert_not_reached ();
				#undef CHECK_SIZE

				op_sizes[i] = size;
			}
		}

		g_once_init_leave (&initialized, TRUE);
	}

	#define BUFLEN 4096

	*buffer = (OpBuffer){
		.len = BUFLEN,
		.pos = 0,
		.buf = g_malloc(BUFLEN),
		.index = g_array_new(FALSE, FALSE, sizeof(OpBufferEntry))
	};

	/* Add dummy entry to guarantee non-empty index */
	op_buffer_add (buffer, OP_NONE);
}


void
op_buffer_destroy (OpBuffer* buffer)
{
	g_free (buffer->buf);
	g_array_unref (buffer->index);
}


void
op_buffer_clear (OpBuffer* buffer)
{
	if (buffer->index->len > 1)
		g_array_remove_range (buffer->index, 1, buffer->index->len - 1);
	buffer->pos = 0;
}


static inline void
ensure_buffer_space_for (OpBuffer* buffer, guint size)
{
	if G_UNLIKELY (buffer->pos + size >= buffer->len) {
		buffer->len *= 2;
		buffer->buf = g_realloc (buffer->buf, buffer->len);
	}
}


gpointer
op_buffer_add (OpBuffer* buffer, OpKind kind)
{
	guint size = op_sizes[kind];
	OpBufferEntry entry;

	entry.pos = buffer->pos;
	entry.kind = kind;

	if (size > 0)
		ensure_buffer_space_for (buffer, size);

	g_array_append_val (buffer->index, entry);

	buffer->pos += size;

	return &buffer->buf[entry.pos];
}
