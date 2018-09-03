/*
  copyright (C) 2013-2018 Tim Orford <tim@orford.org>

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

  ---------------------------------------------------------------

  WaveformView is a Gtk widget based on GtkDrawingArea.
  It displays an audio waveform represented by a Waveform object.

*/
#define __wf_utils_c__
#define __wf_private__
#include "config.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#include <math.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <glib.h>
#include "waveform/waveform.h"

int wf_debug = 0;


#undef SHOW_TIME
#ifdef SHOW_TIME
static uint64_t _get_time()
{
	struct timeval start;
	gettimeofday(&start, NULL);
	return start.tv_sec * 1000 + start.tv_usec / 1000;
}
#endif


void
wf_debug_printf(const char* func, int level, const char* format, ...)
{
    va_list args;

    va_start(args, format);
    if (level <= wf_debug) {
#ifdef SHOW_TIME
		fprintf(stderr, "%Lu %s(): ", _get_time(), func);
#else
		fprintf(stderr, "%s(): ", func);
#endif
        vfprintf(stderr, format, args);
        fprintf(stderr, "\n");
    }
    va_end(args);
}


void
wf_deinterleave(float* src, float** dest, uint64_t n_frames)
{
	int f; for(f=0;f<n_frames;f++){
		int c; for(c=0;c<WF_STEREO;c++){
			dest[c][f] = src[f * WF_STEREO + c];
		}
	}
}


void
wf_deinterleave16(short* src, short** dest, uint64_t n_frames)
{
	int f; for(f=0;f<n_frames;f++){
		int c; for(c=0;c<WF_STEREO;c++){
			dest[c][f] = src[f * WF_STEREO + c];
		}
	}
}


float
wf_int2db(short x)
{
	//converts a signed 16bit int to a dB value.

	float y;

	if(x != 0){
		y = -20.0 * log10(32768.0/ABS(x));
		//printf("int2db: %f\n", 32768.0/abs(x));
	} else {
		y = -100.0;
	}

	return y;
}


//perhaps make all gtk stuff private to the widgets
#ifdef USE_GTK
uint32_t
wf_get_gtk_fg_color(GtkWidget* widget, GtkStateType state)
{
	GtkStyle* style = gtk_widget_get_style(widget);
	//GdkColor fg = style->fg[state];
	return wf_color_gdk_to_rgba(&style->fg[state]);
}


uint32_t
wf_get_gtk_text_color(GtkWidget* widget, GtkStateType state)
{
	GtkStyle* style = gtk_style_copy(gtk_widget_get_style(widget));
	GdkColor c = style->text[state];
	g_object_unref(style);

	return wf_color_gdk_to_rgba(&c);
}


uint32_t
wf_get_gtk_base_color(GtkWidget* widget, GtkStateType state, char alpha)
{
	GtkStyle* style = gtk_style_copy(gtk_widget_get_style(widget));
	GdkColor c = style->base[state];
	g_object_unref(style);

	return (wf_color_gdk_to_rgba(&c) & 0xffffff00) | alpha;
}
#endif


void
wf_colour_rgba_to_float(AGlColourFloat* colour, uint32_t rgba)
{
	//returned values are in the range 0.0 to 1.0;

	g_return_if_fail(colour);

	colour->r = (float)((rgba & 0xff000000) >> 24) / 0xff;
	colour->g = (float)((rgba & 0x00ff0000) >> 16) / 0xff;
	colour->b = (float)((rgba & 0x0000ff00) >>  8) / 0xff;
}


uint32_t
wf_color_gdk_to_rgba(GdkColor* color)
{
	return ((color->red / 0x100) << 24) + ((color->green / 0x100) << 16) + ((color->blue / 0x100) << 8) + 0xff;
}


gboolean
wf_colour_is_dark_rgba(uint32_t colour)
{
	int r = (colour & 0xff000000) >> 24;
	int g = (colour & 0x00ff0000) >> 16;
	int b = (colour & 0x0000ff00) >>  8;

	int average = (r + g + b ) / 3;
	return (average < 0x80);
}


gboolean
wf_get_filename_for_other_channel(const char* filename, char* other, int n_chars)
{
	//return the filename of the other half of a split stereo pair.

	g_strlcpy(other, filename, n_chars);

	gchar* p = g_strrstr(other, "%L.");
	if(p){
		*(p+1) = 'R';
		dbg (3, "pair=%s", other);
		return TRUE;
	}

	p = g_strrstr(other, "%R.");
	if(p){
		*(p+1) = 'L';
		return TRUE;
	}

	p = g_strrstr(other, "-L.");
	if(p){
		*(p+1) = 'R';
		return TRUE;
	}

	p = g_strrstr(other, "-R.");
	if(p){
		*(p+1) = 'L';
		return TRUE;
	}

    other[0] = '\0';
	return FALSE;
}


guint64
wf_get_time()
{
	struct timeval start;
	gettimeofday(&start, NULL);
	return start.tv_sec * 1000 + start.tv_usec / 1000;
}


#if 0
#include "agl/utils.h"
void
fbo_2_png(AGlFBO* fbo)
{
	int width = agl_power_of_two(fbo->width);
	int height = agl_power_of_two(fbo->height);

	unsigned char buffer[width * height * 4];
	glBindTexture(GL_TEXTURE_2D, fbo->texture);
	glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, buffer);

	GdkPixbuf* pixbuf = gdk_pixbuf_new_from_data(buffer, GDK_COLORSPACE_RGB, TRUE, 8, width, height, width*4, NULL, NULL);
	gdk_pixbuf_save(pixbuf, "test.png", "png", NULL, NULL);
	g_object_unref(pixbuf);
}
#endif


#ifndef USE_OPENGL
int
agl_power_of_two(guint a)
{
	// return the next power of two up from the given value.

	int i = 0;
	int orig = a;
	a = MAX(1, a - 1);
	while(a){
		a = a >> 1;
		i++;
	}
	dbg (3, "%i -> %i", orig, 1 << i);
	return 1 << i;
}
#endif


