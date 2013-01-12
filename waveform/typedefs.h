/*
  copyright (C) 2012 Tim Orford <tim@orford.org>

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
#ifndef __waveform_typedefs_h__
#define __waveform_typedefs_h__

typedef struct _wf              WF;
typedef struct _Waveform        Waveform;
typedef struct _peakbuf         Peakbuf;
typedef struct _alpha_buf       AlphaBuf;
typedef struct _peakbuf1        WfPeakBuf;
typedef struct _buf             RmsBuf;
typedef struct _waveform_canvas WaveformCanvas;
typedef struct _waveform_actor  WaveformActor;
typedef struct _wf_texture_list WfGlBlock;
typedef struct _textures_hi     WfTexturesHi;
typedef struct _texture_hi      WfTextureHi;
typedef struct _waveform_priv   WaveformPriv;
typedef struct _audio_data      WfAudioData;
typedef struct _vp              WfViewPort; 
typedef struct _texture_unit    TextureUnit; 
typedef struct _colour_float    WfColourFloat;
typedef struct _WaveformView    WaveformView;
typedef struct _WaveformViewPlus WaveformViewPlus;
typedef struct _wf_shaders       WfShaders;
typedef struct _ass_shader       AssShader;


#endif //__waveform_typedefs_h__
