/*

  Demonstration of the libwaveform WaveformViewPlus widget.
  ---------------------------------------------------------

  It displays a single waveform.
  The keys +- and cursor left/right keys can be used to zoom and in and scroll.

  The two important relevant lines are:
      WaveformView* waveform = waveform_view_plus_new(NULL);
      waveform_view_plus_load_file(waveform, "test/data/mono_1.wav");
  This will create a new Gtk widget that you pack and show as normal.

  In addition to the functions above that are also available in
  the simpler WaveformView widget, additional 'layers' can be added,
  supporting, for example, the display of title text and info text:

    AGlActor* text_layer = waveform_view_plus_add_layer(waveform, text_actor(NULL), 0);
    ((TextActor*)text_layer)->title = g_strdup("Waveform Title");
    text_actor_set_colour((TextActor*)text_layer, 0x33aaffff, 0xffff00ff);

  The SPP layer provides a 'play counter'. To display a cursor and readout
  of the current time, set the time to a non-default value and redraw.

    AGlActor* spp = waveform_view_plus_get_layer(waveform, 5);
    wf_spp_actor_set_time((SppActor*)spp, time_in_milliseconds);

  The WaveformView interface is designed to be easy to use.
  For a more powerful but more complicated interface, see WaveformActor

  --------------------------------------------------------------

  Copyright (C) 2012-2016 Tim Orford <tim@orford.org>

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
#define __wf_private__
#include "config.h"
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <getopt.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include "agl/actor.h"
#include "waveform/view_plus.h"
#include "test/ayyi_utils.h"
#include "common.h"

const char* wavs[] = {
	"test/data/stereo_1.wav",
	"test/data/mono_0:10.wav",
	"test/data/mono_10:00.wav",
//	"test/data/1_block.wav",
//	"test/data/3_blocks.wav",
//	"test/data/2_blocks.wav",
};

KeyHandler
	next_wav,
	toggle_shaders,
	toggle_grid,
	unrealise,
	play,
	stop,
	quit;

extern bool key_down;
extern KeyHold key_hold;

Key keys[] = {
	{GDK_KP_Enter,  NULL},
	{(char)'<',     NULL},
	{(char)'>',     NULL},
	{(char)'n',     next_wav},
	{(char)'s',     toggle_shaders},
	{(char)'g',     toggle_grid},
	{(char)'u',     unrealise},
	{GDK_Delete,    NULL},
	{65438,         stop},
	{65421,         play},
	{113,           quit},
	{0},
};

gpointer tests[] = {};
uint32_t _time = 1000 + 321;
GtkWidget* table = NULL;
struct Layers {
    AGlActor* grid;
    AGlActor* spp;
    AGlActor* spinner;
} layers;


int
main (int argc, char* argv[])
{
	if(sizeof(off_t) != 8){ gerr("sizeof(off_t)=%i\n", sizeof(off_t)); return EXIT_FAILURE; }

	set_log_handlers();

	wf_debug = 0;

	gtk_init(&argc, &argv);
	GtkWidget* window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

	#if 0
	agl_get_instance()->pref_use_shaders = false;
	#endif

	WaveformViewPlus* waveform = waveform_view_plus_new(NULL);
	waveform_view_plus_set_show_rms(waveform, false);
	waveform_view_plus_set_colour(waveform, 0xccccddaa, 0x000000ff);

	char* filename = find_wav(wavs[0]);
	waveform_view_plus_load_file(waveform, filename, NULL, NULL);
	g_free(filename);

#if 0
	waveform_view_plus_set_region(waveform, 0, 32383); // start in hi-res mode
#endif

	waveform_view_plus_add_layer(waveform, background_actor(NULL), 0);

	layers.grid = waveform_view_plus_add_layer(waveform, grid_actor(waveform_view_plus_get_actor(waveform)), 0);

	AGlActor* text_layer = waveform_view_plus_add_layer(waveform, text_actor(NULL), 3);
	((TextActor*)text_layer)->title = g_strdup("Waveform Title");
	((TextActor*)text_layer)->text = g_strdup("Waveform text waveform text");
	text_actor_set_colour((TextActor*)text_layer, 0x33aaffff, 0xffff00ff);

	layers.spp = waveform_view_plus_add_layer(waveform, wf_spp_actor(waveform_view_plus_get_actor(waveform)), 0);
	wf_spp_actor_set_time((SppActor*)layers.spp, (_time += 50, _time));

	layers.spinner = waveform_view_plus_add_layer(waveform, wf_spinner(waveform_view_plus_get_actor(waveform)), 0);

	gtk_widget_set_size_request((GtkWidget*)waveform, 640, 160);

	GtkWidget* scrolledwindow = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledwindow), GTK_POLICY_NEVER, GTK_POLICY_NEVER);
	gtk_container_add((GtkContainer*)window, scrolledwindow);

	table = gtk_table_new(1, 2, false);
	gtk_table_attach(GTK_TABLE(table), (GtkWidget*)waveform, 0, 1, 0, 1, GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);
	gtk_scrolled_window_add_with_viewport((GtkScrolledWindow*)scrolledwindow, table);

	gtk_widget_show_all(window);

	add_key_handlers((GtkWindow*)window, (WaveformView*)waveform, (Key*)&keys);

	bool window_on_delete(GtkWidget* widget, GdkEvent* event, gpointer user_data){
		gtk_main_quit();
		return false;
	}
	g_signal_connect(window, "delete-event", G_CALLBACK(window_on_delete), NULL);

	void on_allocate(GtkWidget* widget, GtkAllocation* allocation, gpointer view)
	{
		static int height = 0;

		if(allocation->height != height){
			gtk_widget_set_size_request((GtkWidget*)view, -1, allocation->height);
			height = allocation->height;
		}
	}
	g_signal_connect(window, "size-allocate", G_CALLBACK(on_allocate), waveform);

	gtk_main();

	return EXIT_SUCCESS;
}


void
quit(WaveformView* waveform)
{
	exit(EXIT_SUCCESS);
}


void next_wav(WaveformView* waveform)
{
	WaveformViewPlus* view = (WaveformViewPlus*)waveform;

	static int i = 0; i = (i + 1) % 3;

	printf("next...\n");

	typedef struct {
		WaveformViewPlus* view;
		float             zoom;
	} C;
	C* c = g_new0(C, 1);
	*c = (C){
		.view = view,
#ifndef USE_CANVAS_SCALING
		.zoom = view->zoom
#endif
	};

	wf_spinner_start((WfSpinner*)layers.spinner);

	void on_loaded_(Waveform* w, gpointer _c)
	{
		wf_spinner_stop((WfSpinner*)layers.spinner);
	}

	char* filename = find_wav(wavs[i]);
	waveform_view_plus_load_file(view, filename, on_loaded_, c);
	g_free(filename);

	// TODO fix widget so that zoom can be set imediately

	bool on_loaded(gpointer _c)
	{
		C* c = _c;
#ifndef USE_CANVAS_SCALING
		waveform_view_plus_set_zoom(c->view, c->zoom);
#endif
		g_free(c);
		return G_SOURCE_REMOVE;
	}
	g_idle_add(on_loaded, c);
}


void
toggle_shaders(WaveformView* view)
{
	printf(">> %s ...\n", __func__);

	agl_actor__set_use_shaders(((AGlActor*)waveform_view_plus_get_actor((WaveformViewPlus*)view))->root, !agl_get_instance()->use_shaders);

	char* filename = find_wav(wavs[0]);
	waveform_view_plus_load_file((WaveformViewPlus*)view, filename, NULL, NULL);
	g_free(filename);
}



void
toggle_grid(WaveformView* view)
{
	static bool visible = true;
	visible = !visible;
	if(visible){
		layers.grid = waveform_view_plus_add_layer((WaveformViewPlus*)view, grid_actor(waveform_view_plus_get_actor((WaveformViewPlus*)view)), 0);
	}else{
		waveform_view_plus_remove_layer((WaveformViewPlus*)view, layers.grid);
	}
}


void
unrealise(WaveformView* view)
{
	bool on_idle(gpointer _view)
	{
		gtk_table_attach(GTK_TABLE(table), (GtkWidget*)_view, 0, 1, 1, 2, GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);
		g_object_unref(_view);
		return G_SOURCE_REMOVE;
	}

	dbg(0, "-----------------------------");
	g_object_ref((GObject*)view);
	gtk_container_remove ((GtkContainer*)table, (GtkWidget*)view);
	g_timeout_add(600, on_idle, view);
}


static guint play_timer = 0;

void
stop(WaveformView* view)
{
	if(play_timer){
		g_source_remove (play_timer);
		play_timer = 0;
	}else{
		wf_spp_actor_set_time((SppActor*)layers.spp, (_time = 0));
	}
}


void
play(WaveformView* view)
{
	bool tick(gpointer view)
	{
		wf_spp_actor_set_time((SppActor*)layers.spp, (_time += 50, _time));
		return true;
	}

	if(!play_timer) play_timer = g_timeout_add(50, tick, view);
}


