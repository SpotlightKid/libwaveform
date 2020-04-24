/*
  copyright (C) 2012-2017 Tim Orford <tim@orford.org>

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
#ifndef __wf_shader_h__
#define __wf_shader_h__
#include "agl/typedefs.h"
#ifdef __gl_h_
#include "agl/shader.h"
#include "waveform/typedefs.h"

struct _PeakShader {
	AGlShader shader;
	void      (*set_uniforms)(float peaks_per_pixel, float top, float bottom, uint32_t _fg_colour, int n_channels);
	struct {
		float peaks_per_pixel;
		float fg_colour[4];
		int   n_channels;
	}         uniform;
};

typedef struct {
	AGlShader shader;
	struct U {
		uint32_t fg_colour;
		float    top;
		float    bottom;
		int      n_channels;
		float    peaks_per_pixel;
	}         uniform;
} HiResShader;

typedef struct {
	AGlShader shader;
	struct {
		uint32_t fg_colour;
		float    top;
		float    bottom;
		int      n_channels;
		float    v_gain;
		float    tex_width;
		float    tex_height;
		int      mm_level;
	}         uniform;
} HiResNGShader;

typedef struct {
	AGlShader shader;
	struct {
		uint32_t fg_colour;
		float    peaks_per_pixel;
	}         uniform;
} BloomShader;

struct _RulerShader {
	AGlShader    shader;
	struct {
		uint32_t fg_colour;
		float    beats_per_pixel;
		float    samples_per_pixel;
		float    viewport_left;
		int      markers[10];
	}            uniform;
};

typedef struct {
	AGlShader    shader;
	struct {
		uint32_t colour;
		int      n_channels;
		int      texture_width;
	}            uniform;
} LinesShader;

struct _ass_shader {
	AGlShader    shader;
	struct {
		uint32_t colour1;
		uint32_t colour2;
	}            uniform;
};

struct _CursorShader {
	AGlShader    shader;
	struct {
		uint32_t colour;
		float    width;
	}            uniform;
};
#ifndef __wf_shader_c__
extern CursorShader cursor;
#endif

#endif

#endif
