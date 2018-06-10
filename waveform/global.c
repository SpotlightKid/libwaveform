/*
  copyright (C) 2012-2018 Tim Orford <tim@orford.org>

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
#define __waveform_peak_c__
#define __wf_private__
#include "config.h"
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <stdint.h>
#include <gtk/gtk.h>
#include "waveform/waveform.h"
#include "waveform/loaders/riff.h"
#include "waveform/texture_cache.h"

WF* wf = NULL;


																												extern int n_loads[4096];
#if defined (WF_USE_TEXTURE_CACHE) && defined (USE_OPENGL)
static void
on_steal (Texture* tex)
{
	WaveformBlock* wb = &tex->wb;

	if(wb->block & WF_TEXTURE_CACHE_HIRES_NG_MASK){
		extern void hi_gl2_on_steal(WaveformBlock*, guint);
		hi_gl2_on_steal(wb, tex->id);
	}else{
		extern void med_lo_on_steal(WaveformBlock*, guint);
		med_lo_on_steal(wb, tex->id);
	}
}
#endif


WF*
wf_get_instance()
{
	if(!wf){
		wf = g_new0(WF, 1);
		wf->domain = "Libwaveform";
		wf->peak_cache = g_hash_table_new(g_direct_hash, g_direct_equal);
		wf->audio.cache = g_hash_table_new(g_direct_hash, g_direct_equal);
																												memset(n_loads, 0, 4096);
		wf->load_peak = wf_load_riff_peak; //set the default loader

#if defined (WF_USE_TEXTURE_CACHE) && defined (USE_OPENGL)
		texture_cache_init();
		texture_cache_set_on_steal(on_steal);
#endif
	}
	return wf;
}


