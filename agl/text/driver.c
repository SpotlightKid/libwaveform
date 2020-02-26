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
#include "config.h"
#include <gdk/gdk.h>
#include "agl/debug.h"
#include "agl/ext.h"
#include "text/driver.h"
#include "text/gdktexture.h"

typedef struct {
   GLuint fbo_id;
   GLuint depth_stencil_id;
} Fbo;

typedef struct {
   GLuint texture_id;
   int width;
   int height;
   GLuint min_filter;
   GLuint mag_filter;
   Fbo fbo;
   GdkTexture* user;
   guint in_use : 1;
   guint permanent : 1;

   /* TODO: Make this optional and not for every texture... */
   TextureSlice *slices;
   guint n_slices;

} Texture;

typedef struct
{
#if 0
   GdkGLContext *gl_context;
   GskProfiler *profiler;
   struct {
      GQuark created_textures;
      GQuark reused_textures;
   } counters;

   Fbo default_fbo;
#endif

   GHashTable* textures;         /* texture_id -> Texture */
   GHashTable* pointer_textures; /* pointer -> texture_id */

   const Texture* bound_source_texture;

   int max_texture_size;

   bool in_frame;
}
GLDriver;

GLDriver driver = {
	.in_frame = 1
};


static Texture*
texture_new (void)
{
	return g_slice_new0(Texture);
}


#if 0
static inline void
fbo_clear (const Fbo *f)
{
  if (f->depth_stencil_id != 0)
    glDeleteRenderbuffers (1, &f->depth_stencil_id);

  glDeleteFramebuffers (1, &f->fbo_id);
}
#endif


static void
texture_free (gpointer data)
{
	Texture* t = data;
	guint i;

#if 0 // --- TODO
	if (t->user)
		gdk_texture_clear_render_data (t->user);
#endif

#if 0
	if (t->fbo.fbo_id != 0)
		fbo_clear (&t->fbo);
#endif

	if (t->texture_id != 0) {
		glDeleteTextures (1, &t->texture_id);
	} else {
		g_assert_cmpint (t->n_slices, >, 0);

		for (i = 0; i < t->n_slices; i ++)
			glDeleteTextures (1, &t->slices[i].texture_id);
	}

	g_slice_free (Texture, t);
}


static void
driver_set_texture_parameters (int min_filter, int mag_filter)
{
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min_filter);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag_filter);

	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}


void
driver_free ()
{
	g_clear_pointer (&driver.textures, g_hash_table_unref);
	g_clear_pointer (&driver.pointer_textures, g_hash_table_unref);
#if 0
	g_clear_object (&self->profiler);
#endif
}


void
driver_init ()
{
	driver.textures = g_hash_table_new_full (NULL, NULL, NULL, texture_free);

	driver.max_texture_size = -1;

#ifdef G_ENABLE_DEBUG
	self->profiler = gsk_profiler_new ();
	self->counters.created_textures = gsk_profiler_add_counter (self->profiler, "created_textures", "Textures created this frame", TRUE);
	self->counters.reused_textures = gsk_profiler_add_counter (self->profiler, "reused_textures", "Textures reused this frame", TRUE);
#endif
}


void
driver_begin_frame ()
{
	g_return_if_fail (!driver.in_frame);

	driver.in_frame = TRUE;

	if (driver.max_texture_size < 0) {
		glGetIntegerv (GL_MAX_TEXTURE_SIZE, (GLint*)&driver.max_texture_size);
#if 0
		GSK_NOTE (OPENGL, g_message ("GL max texture size: %d", self->max_texture_size));
#endif
	}

	glBindFramebuffer (GL_FRAMEBUFFER, 0);

	glActiveTexture (GL_TEXTURE0);
	glBindTexture (GL_TEXTURE_2D, 0);

	glActiveTexture (GL_TEXTURE0 + 1);
	glBindTexture (GL_TEXTURE_2D, 0);

	glBindVertexArray (0);
#if 0
	agl_use_program_id (0);
#endif

	glActiveTexture (GL_TEXTURE0);

#ifdef G_ENABLE_DEBUG
	gsk_profiler_reset (self->profiler);
#endif
}


void
driver_end_frame ()
{
	g_return_if_fail (driver.in_frame);

	driver.bound_source_texture = NULL;

#if 0
	driver.default_fbo.fbo_id = 0;
#endif

#ifdef G_ENABLE_DEBUG
	GSK_NOTE (OPENGL,
		g_message ("Textures created: %" G_GINT64_FORMAT "\n"
			" Textures reused: %" G_GINT64_FORMAT "\n"
			gsk_profiler_counter_get (driver.profiler, driver.counters.created_textures),
			gsk_profiler_counter_get (driver.profiler, driver.counters.reused_textures),
	));
#endif

	dbg (1, "Frame end: textures=%d", g_hash_table_size(driver.textures));

	driver.in_frame = FALSE;
}


bool
driver_in_frame ()
{
	return driver.in_frame;
}


int
driver_collect_textures ()
{
	GHashTableIter iter;
	gpointer value_p = NULL;

#if 0
	g_return_val_if_fail (!driver.in_frame, 0);
#endif

	int old_size = g_hash_table_size (driver.textures);

	g_hash_table_iter_init (&iter, driver.textures);
	while (g_hash_table_iter_next (&iter, NULL, &value_p)) {
		Texture *t = value_p;

		if (t->user || t->permanent)
			continue;

		if (t->in_use) {
			t->in_use = FALSE;

#if 0
			if (t->fbo.fbo_id != 0) {
				fbo_clear (&t->fbo);
				t->fbo.fbo_id = 0;
			}
#endif
		} else {
			/* Remove from self->pointer_textures. */
			/* TODO: Is there a better way for this? */
			if (driver.pointer_textures) {
				GHashTableIter pointer_iter;
				gpointer value;
				gpointer p;

				g_hash_table_iter_init (&pointer_iter, driver.pointer_textures);
				while (g_hash_table_iter_next (&pointer_iter, &p, &value)) {
					if (GPOINTER_TO_INT (value) == t->texture_id) {
						g_hash_table_iter_remove (&pointer_iter);
						break;
					}
				}
			}

			g_hash_table_iter_remove (&iter);
		}
	}

	return old_size - g_hash_table_size (driver.textures);
}


static Texture*
driver_get_texture (int texture_id)
{
	Texture* t;

	if (g_hash_table_lookup_extended(driver.textures, GINT_TO_POINTER (texture_id), NULL, (gpointer *) &t))
		return t;

	return NULL;
}


static Texture*
create_texture (float fwidth, float fheight)
{
	int width = ceilf (fwidth);
	int height = ceilf (fheight);

	g_assert (width > 0);
	g_assert (height > 0);

	if (width > driver.max_texture_size || height > driver.max_texture_size) {
		g_critical ("Texture %d x %d is bigger than supported texture limit of %d; clipping...", width, height, driver.max_texture_size);

		width = MIN (width, driver.max_texture_size);
		height = MIN (height, driver.max_texture_size);
	}

	guint texture_id;
	glGenTextures (1, &texture_id);

	Texture* t = texture_new ();
	t->texture_id = texture_id;
	t->width = width;
	t->height = height;
	t->min_filter = GL_NEAREST;
	t->mag_filter = GL_NEAREST;
	t->in_use = TRUE;
	g_hash_table_insert (driver.textures, GINT_TO_POINTER(texture_id), t);

#ifdef G_ENABLE_DEBUG
	gsk_profiler_counter_inc (driver.profiler, driver.counters.created_textures);
#endif

	return t;
}


#if 0
static void
driver_release_texture (gpointer data)
{
	Texture* t = data;

	t->user = NULL;
}


void
driver_slice_texture (GdkTexture* texture, TextureSlice** out_slices, guint* out_n_slices)
{
  const int max_texture_size = driver_get_max_texture_size (self) / 4; // XXX Too much?
  const int tex_width = texture->width;
  const int tex_height = texture->height;
  const int cols = (texture->width / max_texture_size) + 1;
  const int rows = (texture->height / max_texture_size) + 1;
  int col, row;
  int x = 0, y = 0; /* Position in the texture */
  TextureSlice *slices;
  Texture *tex;

  g_assert (tex_width > max_texture_size || tex_height > max_texture_size);


  tex = gdk_texture_get_render_data (texture, self);

  if (tex != NULL) {
      g_assert (tex->n_slices > 0);
      *out_slices = tex->slices;
      *out_n_slices = tex->n_slices;
      return;
    }

  slices = g_new0 (TextureSlice, cols * rows);

  /* TODO: (Perf):
   *   We still create a surface here, which should obviously be unnecessary
   *   and we should eventually remove it and upload the data directly.
   */
  for (col = 0; col < cols; col ++) {
      const int slice_width = MIN (max_texture_size, texture->width - x);
      const int stride = slice_width * 4;

      for (row = 0; row < rows; row ++) {
          const int slice_height = MIN (max_texture_size, texture->height - y);
          const int slice_index = (col * rows) + row;
          guchar *data;
          guint texture_id;
          cairo_surface_t *surface;

          data = g_malloc (sizeof (guchar) * stride * slice_height);

          gdk_texture_download_area (texture, &(GdkRectangle){x, y, slice_width, slice_height}, data, stride);
          surface = cairo_image_surface_create_for_data (data, CAIRO_FORMAT_ARGB32, slice_width, slice_height, stride);

          glGenTextures (1, &texture_id);

#ifdef G_ENABLE_DEBUG
          gsk_profiler_counter_inc (self->profiler, self->counters.created_textures);
#endif
          glBindTexture (GL_TEXTURE_2D, texture_id);
          driver_set_texture_parameters (GL_NEAREST, GL_NEAREST);
          gdk_cairo_surface_upload_to_gl (surface, GL_TEXTURE_2D, slice_width, slice_height, NULL);

#ifdef G_ENABLE_DEBUG
          gsk_profiler_counter_inc (self->profiler, self->counters.surface_uploads);
#endif

          slices[slice_index].rect = (GdkRectangle){x, y, slice_width, slice_height};
          slices[slice_index].texture_id = texture_id;

          g_free (data);
          cairo_surface_destroy (surface);

          y += slice_height;
        }

      y = 0;
      x += slice_width;
    }

  /* Allocate one Texture for the entire thing. */
  tex = texture_new ();
  tex->width = texture->width;
  tex->height = texture->height;
  tex->min_filter = GL_NEAREST;
  tex->mag_filter = GL_NEAREST;
  tex->in_use = TRUE;
  tex->slices = slices;
  tex->n_slices = cols * rows;

  /* Use texture_free as destroy notify here since we are not inserting this Texture
   * into self->textures! */
  gdk_texture_set_render_data (texture, self, tex, texture_free);

  *out_slices = slices;
  *out_n_slices = cols * rows;
}


int
driver_get_texture_for_texture (GdkTexture* texture, int min_filter, int mag_filter)
{
	Texture* t;
	cairo_surface_t* surface;

	if (GDK_IS_GL_TEXTURE (texture)) {
		GdkGLContext *texture_context = gdk_gl_texture_get_context ((GdkGLTexture *)texture);

		if (texture_context != self->gl_context) {
			/* In this case, we have to temporarily make the texture's context the current one,
			 * download its data into our context and then create a texture from it.
			 */
			if (texture_context)
				gdk_gl_context_make_current (texture_context);

			surface = gdk_texture_download_surface (texture);

			gdk_gl_context_make_current (self->gl_context);
		} else {
			/* A GL texture from the same GL context is a simple task... */
			return gdk_gl_texture_get_id ((GdkGLTexture *)texture);
		}
	} else {
		t = gdk_texture_get_render_data (texture, self);

		if (t) {
			if (t->min_filter == min_filter && t->mag_filter == mag_filter)
				return t->texture_id;
		}

		surface = gdk_texture_download_surface (texture);
	}

	t = create_texture (self, gdk_texture_get_width (texture), gdk_texture_get_height (texture));

	if (gdk_texture_set_render_data (texture, self, t, driver_release_texture))
		t->user = texture;

	driver_bind_source_texture (self, t->texture_id);
	driver_init_texture_with_surface (self, t->texture_id, surface, min_filter, mag_filter);
	gdk_gl_context_label_object_printf (self->gl_context, GL_TEXTURE, t->texture_id, "GdkTexture<%p> %d", texture, t->texture_id);

	cairo_surface_destroy (surface);

	return t->texture_id;
}


int
driver_get_texture_for_pointer (gpointer pointer)
{
	int id = 0;

	if (G_UNLIKELY (self->pointer_textures == NULL))
		self->pointer_textures = g_hash_table_new (NULL, NULL);

	id = GPOINTER_TO_INT (g_hash_table_lookup (self->pointer_textures, pointer));

	if (id != 0) {
		Texture* t = g_hash_table_lookup (self->textures, GINT_TO_POINTER (id));

	if (t != NULL)
		t->in_use = TRUE;
	}

	return id;
}


void
driver_set_texture_for_pointer (gpointer pointer, int texture_id)
{
	if (G_UNLIKELY (self->pointer_textures == NULL))
		self->pointer_textures = g_hash_table_new (NULL, NULL);

	g_hash_table_insert (self->pointer_textures, pointer, GINT_TO_POINTER (texture_id));
}
#endif


int
driver_create_texture (float width, float height)
{
	Texture* t = create_texture (width, height);

	return t->texture_id;
}


#if 0
void
driver_create_render_target (int width, int height, int* out_texture_id, int* out_render_target_id)
{
	GLuint fbo_id;

	g_return_if_fail (self->in_frame);

	Texture* texture = create_texture (self, width, height);
	driver_bind_source_texture (self, texture->texture_id);
	driver_init_texture_empty (self, texture->texture_id, GL_NEAREST, GL_NEAREST);

	glGenFramebuffers (1, &fbo_id);
	glBindFramebuffer (GL_FRAMEBUFFER, fbo_id);
	glFramebufferTexture2D (GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture->texture_id, 0);

#if 0
	if (add_depth_buffer || add_stencil_buffer) {
		glGenRenderbuffersEXT (1, &depth_stencil_buffer_id);
		gdk_gl_context_label_object_printf (self->gl_context, GL_RENDERBUFFER, depth_stencil_buffer_id, "%s buffer for %d", add_depth_buffer ? "Depth" : "Stencil", texture_id);
	}
	else
		depth_stencil_buffer_id = 0;

	glBindRenderbuffer (GL_RENDERBUFFER, depth_stencil_buffer_id);

	if (add_depth_buffer || add_stencil_buffer) {
		if (add_stencil_buffer)
			glRenderbufferStorage (GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, t->width, t->height);
		else
			glRenderbufferStorage (GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, t->width, t->height);

		if (add_depth_buffer)
			glFramebufferRenderbuffer (GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_stencil_buffer_id);

		if (add_stencil_buffer)
			glFramebufferRenderbufferEXT (GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depth_stencil_buffer_id);
		texture->fbo.depth_stencil_id = depth_stencil_buffer_id;
	}
#endif

	texture->fbo.fbo_id = fbo_id;

	g_assert_cmphex (glCheckFramebufferStatus (GL_FRAMEBUFFER), ==, GL_FRAMEBUFFER_COMPLETE);

	glBindFramebuffer (GL_FRAMEBUFFER, self->default_fbo.fbo_id);

	*out_texture_id = texture->texture_id;
	*out_render_target_id = fbo_id;
}
#endif


/*  Mark the texture permanent, meaning it won't be reused by the GLDriver.
 *  E.g. to store it in some other cache.
 */
void
driver_mark_texture_permanent (int texture_id)
{
	Texture* t = driver_get_texture (texture_id);

	g_assert_nonnull (t);

	t->permanent = TRUE;
}


void
driver_bind_source_texture (int texture_id)
{
	g_return_if_fail (driver.in_frame);

	Texture* t = driver_get_texture (texture_id);
	if (t == NULL) {
		g_critical ("No texture %d found.", texture_id);
		return;
	}

	if (driver.bound_source_texture != t) {
		glActiveTexture (GL_TEXTURE0);
		glBindTexture (GL_TEXTURE_2D, t->texture_id);

		driver.bound_source_texture = t;
	}
}


void
driver_destroy_texture (int texture_id)
{
	g_hash_table_remove (driver.textures, GINT_TO_POINTER (texture_id));
}


void
driver_init_texture_empty (int texture_id, int min_filter, int mag_filter)
{

	Texture* t = driver_get_texture(texture_id);
	if (t == NULL) {
		g_critical ("No texture %d found.", texture_id);
		return;
	}

	if (driver.bound_source_texture != t) {
		g_critical("You must bind the texture before initializing it");
		return;
	}

	t->min_filter = min_filter;
	t->mag_filter = mag_filter;

	driver_set_texture_parameters (t->min_filter, t->mag_filter);

	glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA8, t->width, t->height, 0, GL_BGRA, GL_UNSIGNED_BYTE, NULL);

	glBindTexture (GL_TEXTURE_2D, 0);
}


#if 0
static gboolean
filter_uses_mipmaps (int filter)
{
	return filter != GL_NEAREST && filter != GL_LINEAR;
}
#endif
