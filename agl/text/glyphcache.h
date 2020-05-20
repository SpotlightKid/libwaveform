/**
* +----------------------------------------------------------------------+
* | This file is part of the Ayyi project. http://www.ayyi.org           |
* | copyright (C) GTK+ Team and others                                   |
* +----------------------------------------------------------------------+
* | This program is free software; you can redistribute it and/or modify |
* | it under the terms of the GNU General Public License version 3       |
* | as published by the Free Software Foundation.                        |
* +----------------------------------------------------------------------+
*
*/
#ifndef __glyph_cache_h__
#define __glyph_cache_h__

#include <math.h>
#include <pango/pango.h>
#include <gdk/gdk.h>
#include "text/textureatlas.h"

typedef struct
{
  int ref_count;

  GHashTable* hash_table;
  GskGLTextureAtlases* atlases;

  int timestamp;
}
GskGLGlyphCache;

typedef struct
{
  PangoFont* font;
  PangoGlyph glyph;
  guint xshift : 3;
  guint yshift : 3;
  guint scale  : 26; /* times 1024 */
}
CacheKeyData;

typedef struct
{
  CacheKeyData data;
  guint hash;
} GlyphCacheKey;

#define PHASE(x) ((int)(floor (4 * (x + 0.125)) - 4 * floor (x + 0.125)))

static inline void
glyph_cache_key_set_glyph_and_shift (GlyphCacheKey* key, PangoGlyph glyph, float x, float y)
{
	key->data.glyph = glyph;
	key->data.xshift = PHASE (x);
	key->data.yshift = PHASE (y);
	key->hash = GPOINTER_TO_UINT (key->data.font) ^
		key->data.glyph ^
		(key->data.xshift << 24) ^
		(key->data.yshift << 26) ^
		key->data.scale;
}

typedef struct _GskGLCachedGlyph GskGLCachedGlyph;

struct _GskGLCachedGlyph
{
  GskGLTextureAtlas* atlas;
  guint texture_id;

  float tx;
  float ty;
  float tw;
  float th;

  int draw_x;
  int draw_y;
  int draw_width;
  int draw_height;

  guint accessed : 1; /* accessed since last check */
  guint used     : 1; /* accounted as used in the atlas */
};

GskGLGlyphCache* gsk_gl_glyph_cache_new           (GskGLTextureAtlases*);
GskGLGlyphCache* gsk_gl_glyph_cache_ref           (GskGLGlyphCache*);
void             gsk_gl_glyph_cache_unref         (GskGLGlyphCache*);
void             gsk_gl_glyph_cache_begin_frame   (GskGLGlyphCache*, GPtrArray* removed_atlases);
void             gsk_gl_glyph_cache_lookup_or_add (GskGLGlyphCache*, GlyphCacheKey*, const GskGLCachedGlyph**);

#endif
