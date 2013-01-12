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
#define __waveform_peak_c__
#define __wf_private__
#include "config.h"
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <string.h>
#include <sys/stat.h>
#include <math.h>
#include <stdint.h>
#include <sys/time.h>
#include <sndfile.h>
#include <gtk/gtk.h>
#include "waveform/utils.h"
#include "waveform/peak.h"
#include "waveform/audio.h"

typedef struct {
	Waveform*        waveform;
	int              block_num;
	int              min_output_tiers;
}
PeakbufQueueItem;

typedef void   (*WfCallback)    (gpointer user_data);

typedef struct _queue_item
{
	Waveform*        waveform;
	WfCallback       callback;
	void*            user_data;
	gboolean         cancelled;
} QueueItem;

#define MAX_AUDIO_CACHE_SIZE (1 << 23) // words, NOT bytes.

static short*      audio_cache_malloc (Waveform*, int);
static void        audio_cache_free   (Waveform*, int block);
static void        audio_cache_print  ();


void
waveform_audio_free(Waveform* waveform)
{
	PF;
	g_return_if_fail(waveform);

	WfAudioData* audio = waveform->priv->audio_data;
	if(audio && audio->buf16){
		int b; for(b=0;b<audio->n_blocks;b++){
			WfBuf16* buf16 = audio->buf16[b];
			if(buf16){
				audio_cache_free(waveform, b);
				g_free0(buf16);
			}
		}
		g_free(audio->buf16);
		g_free0(waveform->priv->audio_data);
	}
}


gboolean
waveform_load_audio_block(Waveform* waveform, int block_num)
{
	//load a single audio block for the case where the audio is on a local filesystem.

	//TODO handle split stereo files

	g_return_val_if_fail(waveform, false);
	WfAudioData* audio = waveform->priv->audio_data;
	g_return_val_if_fail(audio && audio->buf16, false);

	//int n_frames = sfinfo.frames;
	//guint n_peaks = ((n_frames * 1 ) / WF_PEAK_RATIO) << audio->n_tiers_present;

	// which parts of the audio file are present?
	//  tier 1:  0,             128
	//  tier 2:  0,     64,     128,     196
	//  tier 3:  0, 32, 64, 96, 128, ... 196
	//  tier 4:  0, 16, ...
	//  tier 5:  0,  8, ...
	//  tier 6:  0,  4, ...
	//  tier 7:  0,  2, ...
	//  tier 8:  0,  1, ...

	//int spacing = WF_PEAK_RATIO >> (audio->n_tiers_present - 1);

	uint64_t start_pos = block_num * (WF_PEAK_BLOCK_SIZE - 2.0 * TEX_BORDER * 256.0);
	uint64_t end_pos   = start_pos + WF_PEAK_BLOCK_SIZE;

	int n_chans = waveform_get_n_channels(waveform);
	g_return_val_if_fail(n_chans, false);

	SF_INFO sfinfo;
	SNDFILE* sffile;
	sfinfo.format = 0;

	if(!(sffile = sf_open(waveform->filename, SFM_READ, &sfinfo))){
		gwarn ("not able to open input file %s.", waveform->filename);
		puts(sf_strerror(NULL));
		return false;
	}

	if(start_pos > sfinfo.frames){ gerr("startpos too too high. %Li > %Li block=%i", start_pos, sfinfo.frames, block_num); return false; }
	if(end_pos > sfinfo.frames){ dbg(1, "*** last block: end_pos=%Lu max=%Lu", end_pos, sfinfo.frames); end_pos = sfinfo.frames; }
	sf_seek(sffile, start_pos, SEEK_SET);
	dbg(1, "block=%i/%i start=%Li end=%Li", block_num, waveform_get_n_audio_blocks(waveform), start_pos, end_pos);

	sf_count_t n_frames = MIN(audio->buf16[block_num]->size, end_pos - start_pos); //1st of these isnt needed?
	WfBuf16* buf = audio->buf16[block_num];
	g_return_val_if_fail(buf && buf->buf[WF_LEFT], false);

	gboolean sf_read_float_to_short(SNDFILE* sffile, WfBuf16* buf, int ch, sf_count_t n_frames)
	{
		float readbuf[buf->size];
		sf_count_t readcount;
		if((readcount = sf_readf_float(sffile, readbuf, n_frames)) < n_frames){
			gwarn("unexpected EOF: %s", waveform->filename);
			gwarn("                start_frame=%Li n_frames=%Lu/%Lu read=%Li", start_pos, n_frames, sfinfo.frames, readcount);
			return false;
		}

		//convert to short
		int j; for(j=0;j<readcount;j++){
			buf->buf[ch][j] = readbuf[j] * (1 << 15);
		}

		return true;
	}

	sf_count_t readcount;
	switch(sfinfo.channels){
		case WF_MONO:
			;gboolean is_float = ((sfinfo.format & SF_FORMAT_SUBMASK) == SF_FORMAT_FLOAT);
			if(is_float){
				//FIXME temporary? sndfile is supposed to automatically convert between formats??!

				sf_read_float_to_short(sffile, buf, WF_LEFT, n_frames);

				if(waveform->is_split){
					dbg(2, "is_split! file=%s", waveform->filename);
					char rhs[256];
					if(wf_get_filename_for_other_channel(waveform->filename, rhs, 256)){
						dbg(3, "  %s", rhs);

						if(sf_close(sffile)) gwarn ("bad file close.");
						if(!(sffile = sf_open(rhs, SFM_READ, &sfinfo))){
							gwarn ("not able to open input file %s.", rhs);
							puts(sf_strerror(NULL));
							return false;
						}
						sf_seek(sffile, start_pos, SEEK_SET);

						sf_read_float_to_short(sffile, buf, WF_RIGHT, n_frames);
					}
				}
			}else{
				if((readcount = sf_readf_short(sffile, buf->buf[WF_LEFT], n_frames)) < n_frames){
					gwarn("unexpected EOF: %s", waveform->filename);
					gwarn("                start_frame=%Li n_frames=%Lu/%Lu read=%Li", start_pos, n_frames, sfinfo.frames, readcount);
				}
			}
			/*
			int i; for(i=0;i<10;i++){
				printf("  %i\n", buf->buf[WF_LEFT][i]);
			}
			*/
			break;
		case WF_STEREO:
			{
#if 0
			float read_buf[n_frames * WF_STEREO];

			if((readcount = sf_readf_float(sffile, read_buf, n_frames)) < n_frames){
#else
			short read_buf[n_frames * WF_STEREO];
			if((readcount = sf_readf_short(sffile, read_buf, n_frames)) < n_frames){
#endif
				gwarn("unexpected EOF: %s", waveform->filename);
				gwarn("                STEREO start_frame=%Lu n_frames=%Lu/%Lu read=%Lu", start_pos, n_frames, sfinfo.frames, readcount);
			}

			#if 0 //only useful for testing.
			memset(w->cache->buf->buf[0], 0, WF_CACHE_BUF_SIZE);
			memset(w->cache->buf->buf[1], 0, WF_CACHE_BUF_SIZE);
			#endif

			wf_deinterleave16(read_buf, buf->buf, n_frames);
			}
			break;
		default:
			break;
	}
	dbg(2, "read %Lu frames", n_frames);
	if(sf_error(sffile)) gwarn("read error");
	if(sf_close(sffile)) gwarn ("bad file close.");

	//buffer size is the allocation size. To check if it is full, use w->samplecount
	//buf->size = readcount; X

	return true;
}


static gboolean
peakbuf_is_present(Waveform* waveform, int block_num)
{
	Peakbuf* peakbuf = waveform_get_peakbuf_n(waveform, block_num);
	if(!peakbuf){ dbg(2, "no"); return FALSE; }
	dbg(2, "%i: %s", block_num, peakbuf->buf[0] ? "yes" : "no");
	return (gboolean)peakbuf->buf[0];
}


gpointer
file_load_thread(gpointer data)
{
	//TODO as we use a blocking call on the async queue (g_async_queue_pop) we may no longer need a main loop.

	dbg(2, "new file load thread.");
	WF* wf = wf_get_instance();

	if(!wf->msg_queue){ perr("no msg_queue!\n"); return NULL; }

	g_async_queue_ref(wf->msg_queue);

	gboolean worker_timeout(gpointer data)
	{
		static GList* jobs = NULL;

		WF* wf = wf_get_instance();

		PeakbufQueueItem* peakbuf_queue_find(GList* jobs, PeakbufQueueItem* target)
		{
			GList* l = jobs;//wf_get_instance()->work_queue;
			for(;l;l=l->next){
				PeakbufQueueItem* item = l->data;
				if(item->waveform == target->waveform && item->block_num == target->block_num) return item;
			}
			return NULL;
		}

		gboolean do_callback(gpointer _item)
		{
			QueueItem* item = _item;
			if(!item->cancelled){
				call(item->callback, item->user_data);
			}else{
				dbg(0, "job cancelled. not calling callback");
				g_free(item->user_data);
			}
			WF* wf = wf_get_instance();
			wf->jobs = g_list_remove(wf->jobs, item);
			g_free(item);
			return IDLE_STOP;
		}

		//check for new work
		while(g_async_queue_length(wf->msg_queue)){
			QueueItem* message = g_async_queue_pop(wf->msg_queue); // blocks
			if(!peakbuf_queue_find(jobs, message->user_data)){
				jobs = g_list_append(jobs, message);
				dbg(2, "new message! %p", message);
			}
			else g_free(message->user_data); //not ideal.
		}

		while(jobs){
			dbg(1, "%i jobs remaining", g_list_length(jobs));
			QueueItem* item = g_list_first(jobs)->data;
			jobs = g_list_remove(jobs, item);

			g_idle_add(do_callback, item);
		}

		//TODO stop timer if no work?
		return TIMER_CONTINUE;
	}

	GMainContext* context = g_main_context_new();

	GSource* source = g_timeout_source_new(100);
	gpointer _data = NULL;
	g_source_set_callback(source, worker_timeout, _data, NULL);
	g_source_attach(source, context);

	g_main_loop_run (g_main_loop_new (context, 0));
	return NULL;
}


/*
 * load part of the audio into a ram buffer.
 * -a signal will be emitted once the load is complete.
 *
 * warning: a vary large file cannot be loaded into ram all at once.
 * if the audio file is too big for the cache, then multiple parallel calls may fail.
 * -requests should be done sequentially to avoid this.
 * TODO this would be helped if the buffer was not allocated until queue processing.
 *
 */
WfBuf16*
waveform_load_audio_async(Waveform* waveform, int block_num, int n_tiers_needed)
{
	//if the audio is already loaded, it will be returned imediately
	//and nothing further done.
	// *** Having two return paths is bad and is subject to change.

	//if the file is local we access it directly, otherwise send a msg.
	//-for now, we assume the file is local.

	//TODO should use same api as g_file_read_async ? uses GAsyncReadyCallback

	PF;
	g_return_val_if_fail(waveform->priv->audio_data, NULL);
	g_return_val_if_fail(block_num < waveform_get_n_audio_blocks(waveform), NULL);
	WfAudioData* audio = waveform->priv->audio_data;

	if(audio->buf16){
		WfBuf16* buf = audio->buf16[block_num];
		if(buf) return buf;
	}

	waveform->priv->audio_data->n_tiers_present = MAX_TIERS;

	static gboolean have_thread = false;
	GError* error = NULL;
	if(!have_thread && !g_thread_create(file_load_thread, NULL, false, &error)){
		perr("error creating thread: %s\n", error->message);
		g_error_free(error);
		return NULL;
	}
	have_thread = true;

	gboolean is_queued(Waveform* waveform, int block_num)
	{
		WF* wf = wf_get_instance();
		GList* l = wf->jobs;
		for(;l;l=l->next){
			QueueItem* i = l->data;
			PeakbufQueueItem* item = i->user_data;
			if(item->waveform == waveform && item->block_num == block_num){
				dbg(1, "already queued");
				//gwarn("never get here");
				//actually it is possible to get here using test_view while zooming in/out
				//or when there are lots of views of the same waveform
				return true;
			}
		}
		return false;
	}

	void _queue_work(Waveform* waveform, WfCallback callback, gpointer user_data)
	{
		QueueItem* item = g_new0(QueueItem, 1);
		item->waveform = waveform;
		item->callback = callback;
		item->user_data = user_data;

		wf_push_job(item);
	}

	void wf_peakbuf_queue_for_regen(Waveform* waveform, int block_num, int min_output_tiers)
	{
		PF;

		if(is_queued(waveform, block_num)) return;

		PeakbufQueueItem* item = g_new0(PeakbufQueueItem, 1);
		item->waveform         = waveform;
		item->block_num        = block_num;
		item->min_output_tiers = min_output_tiers;

		WfAudioData* audio = waveform->priv->audio_data;
		if(!audio->buf16){
			audio->buf16 = g_malloc0(sizeof(void*) * waveform_get_n_audio_blocks(waveform));
			//the fact that this is now allocated indicates that a request has been initiated.
			//(for this waveform, but not necesarily for this block)
		}

		void run_queue_item(gpointer item)
		{
			PeakbufQueueItem* peak = item;
			WF* wf = wf_get_instance();
			Waveform* waveform = peak->waveform;
			WfAudioData* audio = peak->waveform->priv->audio_data;
			int block_num = peak->block_num;

			if(!audio->buf16[block_num]){
				audio->buf16[block_num] = g_new0(WfBuf16, 1); // this is used as a flag to indicate that the audio is now valid.
				audio->buf16[block_num]->size = WF_PEAK_BLOCK_SIZE;
				int c; for(c=0;c<waveform_get_n_channels(waveform);c++){
					audio->buf16[block_num]->buf[c] = audio_cache_malloc(waveform, block_num);
					audio->buf16[block_num]->stamp = ++wf->audio.access_counter;
				}
			}
			dbg(1, "block=%i tot_audio_mem=%ukB", block_num, wf->audio.mem_size / 1024);

			if(waveform_load_audio_block(peak->waveform, peak->block_num)){
				waveform_peakbuf_regen(peak->waveform, peak->block_num, peak->min_output_tiers);

				dbg(1, "--->");
				g_signal_emit_by_name(peak->waveform, "peakdata-ready", peak->block_num);
			}

			g_free(item);
		}
		_queue_work(waveform, run_queue_item, item);
	}

	if(!peakbuf_is_present(waveform, block_num)) wf_peakbuf_queue_for_regen(waveform, block_num, n_tiers_needed);

	return NULL;
}


void
wf_cancel_jobs(Waveform* waveform)
{
	WF* wf = wf_get_instance();
	GList* l = wf->jobs;
	for(;l;l=l->next){
		QueueItem* i = l->data;
		if(i->waveform == waveform) i->cancelled = true;
	}
	int n_jobs = g_list_length(wf->jobs);
	dbg(n_jobs ? 1 : 2, "n_jobs=%i", n_jobs);
}


static int
wf_block_lookup_by_audio_buf(Waveform* w, WfBuf16* buf)
{
	g_return_val_if_fail(w && buf, -1);

	int b; for(b=0;b<waveform_get_n_audio_blocks(w);b++){
		if(w->priv->audio_data->buf16[b] == buf) return b;
	}
	return -1;
}


static short*
audio_cache_malloc(Waveform* w, int b)
{
	int size = WF_PEAK_BLOCK_SIZE;

	//dbg(1, "cache_size=%ik", MAX_AUDIO_CACHE_SIZE / 1024);
	WF* wf = wf_get_instance();
	if(wf->audio.mem_size + size > MAX_AUDIO_CACHE_SIZE){
		dbg(1, "**** cache full. looking for audio block to delete...");
		//what to delete?
		// - audio cache items can be in use, but only if we are at high zoom.
		//   But as there are many views, none of which we have knowledge of, we can't use this information.
		// Therefore we base decisions on 'least recently used'.
		WfBuf16* oldest = NULL;
		Waveform* oldest_waveform = NULL;
		GHashTableIter iter;
		gpointer key, value;
		g_hash_table_iter_init (&iter, wf->audio.cache);
		while (g_hash_table_iter_next (&iter, &key, &value)){ //need to check the stamp of each block and associate it with the waveform
			WfBuf16* buf = key;
			Waveform* w = value;
			//dbg(1, "  stamp=%u", buf->stamp);
			if(!oldest || buf->stamp < oldest->stamp){
				oldest = buf;
				oldest_waveform = w;
			}
		}
		if(oldest){
			dbg(1, "clearing buf with stamp=%i ...", oldest->stamp);
			audio_cache_free(oldest_waveform, wf_block_lookup_by_audio_buf(oldest_waveform, oldest));
		}
		if(wf->audio.mem_size + size > MAX_AUDIO_CACHE_SIZE){
			gerr("cant free space in audio cache");
		}
	}

	short* buf = g_malloc(sizeof(short) * size);
	wf->audio.mem_size += size;
	w->priv->audio_data->buf16[b]->size = size;
	dbg(1, "b=%i inserting: %p", b, buf);
	g_hash_table_insert(wf->audio.cache, w->priv->audio_data->buf16[b], w); //each channel has its own entry. however as channels are always accessed together, it might be better to have one entry per Buf16*
	audio_cache_print();
	return buf;
}


static void
audio_cache_free(Waveform* w, int block)
{
	//currently this ONLY frees the audio data, not the structs that contain it.

	//dbg(1, "b=%i", block);

	WF* wf = wf_get_instance();
	WfAudioData* audio = w->priv->audio_data;
	WfBuf16* buf16 = audio->buf16[block];
	if(buf16){
		// the cache may already have been cleared if the cache is full
		if(!g_hash_table_remove(wf->audio.cache, buf16)) dbg(2, "%i: failed to remove waveform block from audio_cache", block);
		if(buf16->buf[WF_LEFT]){
			wf->audio.mem_size -= buf16->size;
			//dbg(1, "b=%i clearing left... size=%i", block, buf16->size);
			g_free0(buf16->buf[WF_LEFT]);
		}
		else { dbg(2, "%i: left buffer empty", block); }

		if(buf16->buf[WF_RIGHT]){
			wf->audio.mem_size -= buf16->size;
			dbg(2, "b=%i clearing right...", block);
			g_free0(buf16->buf[WF_RIGHT]);
		}
	}
	//audio_cache_print();
}


static void
audio_cache_print()
{
	WF* wf = wf_get_instance();

	#define STRLEN 64 // this is not long enough to show the whole cache.
	char str[STRLEN];
	memset(str, ' ', STRLEN - 1);
	str[STRLEN - 1] = '\0';

	int total_size = 0;
	int total_mem = 0;
	GHashTableIter iter;
	gpointer key, value;
	int i = 0;
	g_hash_table_iter_init (&iter, wf->audio.cache);
	while (g_hash_table_iter_next (&iter, &key, &value)){
		Waveform* w = value;
		WfBuf16* buf = key;
		//	printf("  -\n");
		g_return_if_fail(w && buf);
		int c; for(c=0;c<WF_STEREO;c++){
			//if(buf->buf[c]) printf("    %i %p\n", c, buf->buf[c]);
			if(buf->buf[c]) total_mem += WF_PEAK_BLOCK_SIZE;
		}
		if(i < STRLEN / 2 - 1){
			if(buf->buf[WF_LEFT])  str[2*i    ] = 'L';
			if(buf->buf[WF_RIGHT]) str[2*i + 1] = 'R';
		}

		total_size++;
		i++;
	}
	dbg(1, "size=%i mem=%ikB=%ikB %s", total_size, total_mem * sizeof(short) / 1024, wf->audio.mem_size * sizeof(short) / 1024, str);
}


#if UNUSED
static float
int2db(short x) // only used for debug output
{
	//converts a signed 16bit int to a dB value.

	float y;

	if(x != 0){
		y = -20.0 * log10(32768.0/abs(x));
		//printf("int2db: %f\n", 32768.0/abs(x));
	} else {
		y = -100.0;
	}

	return y;    
}
#endif


