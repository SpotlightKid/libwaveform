/*
  copyright (C) 2012-2013 Tim Orford <tim@orford.org>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License version 3
  as published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/time.h>
#include <glib.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glx.h>
#include <GL/glxext.h>
#include "agl/utils.h"
#include "agl/ext.h"
#include "agl/fbo.h"

#define NON_SQUARE

typedef struct { int w, h; } iSize;

//TODO, perhaps just remove custom debugging messages...
extern void wf_debug_printf (const char* func, int level, const char* format, ...);
#define gwarn(A, ...) g_warning("%s(): "A, __func__, ##__VA_ARGS__);
#define dbg(A, B, ...) wf_debug_printf(__func__, A, B, ##__VA_ARGS__)

static GLuint make_fb(AglFBO*);


#ifdef NON_SQUARE
	GLuint make_texture(guint width, guint height)
#else
	GLuint make_texture(guint size)
#endif
	{
					glEnable(GL_TEXTURE_2D);
					glActiveTexture(GL_TEXTURE0);
					glEnable(GL_TEXTURE_2D);
		GLuint texture;
		glGenTextures(1, &texture);
		glBindTexture(GL_TEXTURE_2D, texture);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
#ifdef NON_SQUARE
		glTexImage2D(GL_TEXTURE_2D, 0, 4, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
#else
		glTexImage2D(GL_TEXTURE_2D, 0, 4, size, size, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
#endif
		//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, TextureLevel);
		//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, TextureLevel);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

		return texture;
	}

AglFBO*
agl_fbo_new(int width, int height, GLuint texture)
{
	//if texture is zero, a new texture will be created.

	AglFBO* fbo = g_new0(AglFBO, 1);
	fbo->width = width;
	fbo->height = height;
#ifdef NON_SQUARE
	fbo->texture = texture ? texture : make_texture(agl_power_of_two(width), agl_power_of_two(height));
#else
	fbo->texture = texture ? texture : make_texture(agl_power_of_two(MAX(width, height)));
#endif
	make_fb(fbo);
#ifdef NON_SQUARE
	dbg(1, "fb=%i texture=%i size=%ix%i", fbo->id, fbo->texture, agl_power_of_two(width), agl_power_of_two(height));
#else
	dbg(1, "fb=%i texture=%i size=%i", fbo->id, fbo->texture, agl_power_of_two(MAX(width, height)));
#endif

	/*
	glBindFramebuffer(GL_FRAMEBUFFER_EXT, fbo->id);

	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER_EXT);
	if (status != GL_FRAMEBUFFER_COMPLETE_EXT) gwarn("framebuffer incomplete");

	glPushAttrib(GL_VIEWPORT_BIT);
	glViewport(0, 0, fbo->width, fbo->height);

	//draw something into the fbo
	glClearColor(0.0, 1.0, 0.0, 0.5);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	glColor3f(0, 1, 0);
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);
	float s = 256.0;
	glBegin(GL_POLYGON);
	glVertex2f(-s,  0.0);
	glVertex2f( 0.0, -s);
	glVertex2f( s,  0.0);
	glVertex2f( 0.0,  s);
	glEnd();

	glPopAttrib(); //restore viewport

	glBindFramebuffer(GL_FRAMEBUFFER_EXT, 0); // rebind the normal framebuffer
	*/

	gl_warn("fbo_new");
	return fbo;
}


void
agl_fbo_free(AglFBO* fbo)
{
	g_return_if_fail(fbo);

	glDeleteTextures(1, &fbo->texture);
	glDeleteFramebuffers(1, &fbo->id);
	g_free(fbo);
}


void
agl_fbo_set_size(AglFBO* fbo, int width, int height)
{
	g_return_if_fail(fbo);

#ifdef NON_SQUARE
	iSize current_size = {agl_power_of_two(fbo->width), agl_power_of_two(fbo->height)};
	iSize new_size = {agl_power_of_two(width), agl_power_of_two(height)};
#else
	int current_size = agl_power_of_two(MAX(fbo->width, fbo->height));
	int new_size = agl_power_of_two(MAX(width, height));
#endif
	fbo->width = width;
	fbo->height = height;
#ifdef NON_SQUARE
	if(current_size.w != new_size.w || current_size.h != new_size.h){
		dbg(1, "new size: %i x %i", new_size.w, new_size.h);
#else
	if(current_size != new_size){
		dbg(1, "new size: %i", new_size);
#endif
		glDeleteTextures(1, &fbo->texture);
		glDeleteFramebuffers(1, &fbo->id);

#ifdef NON_SQUARE
		fbo->texture = make_texture(agl_power_of_two(width), agl_power_of_two(height));
#else
		fbo->texture = make_texture(agl_power_of_two(MAX(width, height)));
#endif
		make_fb(fbo);
	}
}


static GLuint DepthRB = 0, StencilRB = 0;
static gboolean UsePackedDepthStencil = FALSE;
static gboolean UsePackedDepthStencilBoth = FALSE;

/**
 * Attach depth and stencil renderbuffer(s) to the given framebuffer object.
 * \param tryDepthStencil  if true, try to use a combined depth+stencil buffer
 * \param bindDepthStencil  if true, and tryDepthStencil is true, bind with the GL_DEPTH_STENCIL_ATTACHMENT target.
 * \return GL_TRUE for success, GL_FALSE for failure
 */
static gboolean
attach_depth_and_stencil_buffers(AglFBO* fbo, GLboolean tryDepthStencil, GLboolean bindDepthStencil, GLuint *depthRbOut, GLuint *stencilRbOut)
{
	GLenum status;

	*depthRbOut = *stencilRbOut = 0;

	glBindFramebuffer(GL_FRAMEBUFFER_EXT, fbo->id);

	if(tryDepthStencil){
		GLuint rb;

		glGenRenderbuffers(1, &rb);
		glBindRenderbuffer(GL_RENDERBUFFER_EXT, rb);
		glRenderbufferStorage(GL_RENDERBUFFER_EXT, GL_DEPTH24_STENCIL8_EXT, agl_power_of_two(fbo->width), agl_power_of_two(fbo->height));
		if (glGetError()) return FALSE;

		if (bindDepthStencil) {
			// attach to both depth and stencil at once 
			glFramebufferRenderbuffer(GL_FRAMEBUFFER_EXT, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER_EXT, rb);
			if (glGetError()) return FALSE;
			//dbg(0, "successfully attached to both depth and stencil buffer");
		} else {
			// attach to depth attachment point
			glFramebufferRenderbuffer(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, rb);
			if (glGetError()) return FALSE;

			// and attach to stencil attachment point
			glFramebufferRenderbuffer(GL_FRAMEBUFFER_EXT, GL_STENCIL_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, rb);
			if (glGetError()) return FALSE;
			//dbg(0, "successfully attached to depth and stencil buffer separately rb=%i", rb);
		}

		status = glCheckFramebufferStatus(GL_FRAMEBUFFER_EXT);
		if (status != GL_FRAMEBUFFER_COMPLETE_EXT) return FALSE;

		*depthRbOut = *stencilRbOut = rb;
		return TRUE;
	}

	// just depth renderbuffer
	{
		GLuint rb;

		glGenRenderbuffers(1, &rb);
		glBindRenderbuffer(GL_RENDERBUFFER_EXT, rb);
		glRenderbufferStorage(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT, agl_power_of_two(fbo->width), agl_power_of_two(fbo->height));
		if (glGetError()) return FALSE;

		// attach to depth attachment point
		glFramebufferRenderbuffer(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, rb);
		if (glGetError()) return FALSE;

		status = glCheckFramebufferStatus(GL_FRAMEBUFFER_EXT);
		if (status != GL_FRAMEBUFFER_COMPLETE_EXT) return FALSE;

		*depthRbOut = rb;
	}

	// just stencil renderbuffer
	{
		GLuint rb;

		glGenRenderbuffers(1, &rb);
		glBindRenderbuffer(GL_RENDERBUFFER_EXT, rb);
		glRenderbufferStorage(GL_RENDERBUFFER_EXT, GL_STENCIL_INDEX, fbo->width, fbo->height);
		if (glGetError()) return FALSE;

		// attach to stencil attachment point
		glFramebufferRenderbuffer(GL_FRAMEBUFFER_EXT, GL_STENCIL_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, rb);
		if (glGetError()) return FALSE;

		status = glCheckFramebufferStatus(GL_FRAMEBUFFER_EXT);
		if (status != GL_FRAMEBUFFER_COMPLETE_EXT) {
			//if(status == GL_FRAMEBUFFER_UNSUPPORTED){ dbg(0, "unsupported"); }
			glDeleteRenderbuffers(1, depthRbOut);
			*depthRbOut = 0;
			glDeleteRenderbuffers(1, &rb);
			return FALSE;
		}

		*stencilRbOut = rb;
	}

	return TRUE;
}


static GLuint
make_fb(AglFBO* fbo)
{
	// create the framebuffer and add a stencil buffer to it.

	dbg(2, "generating framebuffer...");
	g_return_val_if_fail(glGenFramebuffers, 0);
	glGenFramebuffers(1, &fbo->id);
	glBindFramebuffer(GL_FRAMEBUFFER_EXT, fbo->id);

	dbg(2, "attaching texture %u to fbo...", fbo->texture);
	int texture_level = 0;
	glFramebufferTexture2D(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, fbo->texture, texture_level);

	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER_EXT);
	if (status != GL_FRAMEBUFFER_COMPLETE_EXT) gwarn("framebuffer incomplete: 0x%04x", status);

	// Setup depth and stencil buffers - TODO depth buffer not used. remove it?
	{
		gboolean b = attach_depth_and_stencil_buffers(fbo, UsePackedDepthStencil, UsePackedDepthStencilBoth, &DepthRB, &StencilRB);
		if (!b) {
			// try !UsePackedDepthStencil
			b = attach_depth_and_stencil_buffers(fbo, !UsePackedDepthStencil, UsePackedDepthStencilBoth, &DepthRB, &StencilRB);
		}
		if (!b) {
			printf("Unable to create/attach depth and stencil renderbuffers to FBO\n");
		}
	}

   if(FALSE){
      GLint bits, w, h, name;

      glBindRenderbuffer(GL_RENDERBUFFER_EXT, DepthRB);
      glGetRenderbufferParameteriv(GL_RENDERBUFFER_EXT, GL_RENDERBUFFER_WIDTH_EXT, &w);
      glGetRenderbufferParameteriv(GL_RENDERBUFFER_EXT, GL_RENDERBUFFER_HEIGHT_EXT, &h);
      printf("Color/Texture size: %d x %d\n", fbo->width, fbo->height);
      printf("Depth buffer size: %d x %d\n", w, h);

      glGetRenderbufferParameteriv(GL_RENDERBUFFER_EXT, GL_RENDERBUFFER_DEPTH_SIZE_EXT, &bits);
      printf("Depth renderbuffer size = %d bits\n", bits);

      glBindRenderbuffer(GL_RENDERBUFFER_EXT, StencilRB);
      glGetRenderbufferParameteriv(GL_RENDERBUFFER_EXT, GL_RENDERBUFFER_STENCIL_SIZE_EXT, &bits);
      printf("Stencil renderbuffer size = %d bits\n", bits);

      glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME_EXT, &name);
      printf("Render to texture name: %d\n", fbo->texture);
      printf("Color attachment[0] name: %d\n", name);
      g_return_val_if_fail(fbo->texture == name, 0);

      glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER_EXT, GL_STENCIL_ATTACHMENT, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME_EXT, &name);
      printf("Stencil attachment name: %d\n", name);

      glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME_EXT, &name);
      printf("Depth attachment name: %d\n", name);
	}

	glBindFramebuffer(GL_FRAMEBUFFER_EXT, 0); // rebind the normal framebuffer

	return fbo->id;
}


