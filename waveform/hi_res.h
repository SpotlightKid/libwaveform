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
#include "waveform/actor.h"

void     draw_wave_buffer_hi   (Waveform*, WfSampleRegion, WfRectangle*, Peakbuf*, int chan, float v_gain, uint32_t rgba);
void     draw_wave_buffer_v_hi (WaveformActor*, WfSampleRegion, WfSampleRegion, WfRectangle*, WfViewPort*, WfBuf16*, float v_gain, uint32_t rgba, bool is_first, double x_block0);
