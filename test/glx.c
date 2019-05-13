/**
* +----------------------------------------------------------------------+
* | This file is part of libwaveform                                     |
* | https://github.com/ayyi/libwaveform                                  |
* | copyright (C) 2012-2019 Tim Orford <tim@orford.org>                  |
* +----------------------------------------------------------------------+
* | This program is free software; you can redistribute it and/or modify |
* | it under the terms of the GNU General Public License version 3       |
* | as published by the Free Software Foundation.                        |
* +----------------------------------------------------------------------+
*
* *********** TODO only redraw if something has changed (currently redrawing at 60fps) **********
*/
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
# define GLX_GLXEXT_PROTOTYPES
#include <GL/glx.h>
#include "gdk/gdk.h"
#include "agl/ext.h"
#define __wf_private__
#include "waveform/waveform.h"
#include "agl/actor.h"
#include "waveform/actors/background.h"
#define __glx_test__
#include "test/common2.h"

static GLboolean print_info = GL_FALSE;

static void on_window_resize       (int, int);
static void _add_key_handlers      ();

#define BENCHMARK
#define NUL '\0'

extern PFNGLXGETFRAMEUSAGEMESAPROC get_frame_usage;

static AGlRootActor* scene = NULL;
struct {
	AGlActor*      bg;
	WaveformActor* wa;
} layers = {0,};

static GHashTable* key_handlers = NULL;

static KeyHandler
	nav_up,
	nav_down,
	zoom_in,
	zoom_out;

Key keys[] = {
	{XK_Up,          nav_up},
	{XK_Down,        nav_down},
	{XK_equal,       zoom_in},
	{XK_KP_Add,      zoom_in},
	{XK_minus,       zoom_out},
	{XK_KP_Subtract, zoom_out},
};

static const struct option long_options[] = {
	{ "non-interactive",  0, NULL, 'n' },
};

static const char* const short_options = "n";


int
main(int argc, char *argv[])
{
	int width = 300, height = 300;

	int i; for (i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-info") == 0) {
			print_info = GL_TRUE;
		}
		else if (strcmp(argv[i], "-swap") == 0 && i + 1 < argc) {
			//swap_interval = atoi( argv[i+1] );
			//do_swap_interval = GL_TRUE;
			i++;
		}
		else if (strcmp(argv[i], "-forcegetrate") == 0) {
			/* This option was put in because some DRI drivers don't support the
			 * full GLX_OML_sync_control extension, but they do support
			 * glXGetMscRateOML.
			 */
			//force_get_rate = GL_TRUE;
		}
		else if (strcmp(argv[i], "-help") == 0) {
			printf("Usage:\n");
			printf("  glx [options]\n");
			printf("Options:\n");
			printf("  -help                   Print this information\n");
			printf("  -info                   Display GL information\n");
			printf("  -swap N                 Swap no more than once per N vertical refreshes\n");
			printf("  -forcegetrate           Try to use glXGetMscRateOML function\n");
			return 0;
		}
	}

	int opt;
	while((opt = getopt_long (argc, argv, short_options, long_options, NULL)) != -1) {
		switch(opt) {
			case 'n':
				g_timeout_add(3000, (gpointer)exit, NULL);
				break;
		}
	}

	Display* dpy = XOpenDisplay(NULL);
	if (!dpy) {
		printf("Error: couldn't open display %s\n", XDisplayName(NULL));
		return -1;
	}

	scene = (AGlRootActor*)agl_actor__new_root_(CONTEXT_TYPE_GLX);

	AGlWindow* window = agl_make_window(dpy, "waveformglxtest", width, height, scene);
	XMapWindow(dpy, window->window);

	// -----------------------------------------------------------

	g_main_loop_new(NULL, true);

	agl_actor__add_child((AGlActor*)scene, layers.bg = background_actor(NULL));

	char* filename = find_wav("mono_0:10.wav");
	Waveform* w = waveform_load_new(filename);
	g_free(filename);

	WaveformContext* wfc = wf_context_new(scene);

	agl_actor__add_child((AGlActor*)scene, (AGlActor*)(layers.wa = wf_canvas_add_new_actor(wfc, w)));

	wf_actor_set_region(layers.wa, &(WfSampleRegion){0, 44100});

	wf_actor_set_rect(layers.wa, &(WfRectangle){
		0.0,
		0.0,
		width,
		height
	});

	// -----------------------------------------------------------

	on_window_resize(width, height);

	_add_key_handlers();

	event_loop(dpy);

	agl_window_destroy(dpy, &window);
	XCloseDisplay(dpy);

	return 0;
}


static void
on_window_resize (int width, int height)
{
	#define HBORDER 0
	#define VBORDER 0
	int vx = 0;
	int vy = 0;
	glViewport(vx, vy, width, height);
	dbg (2, "viewport: %i %i %i %i", vx, vy, width, height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	double left   = -HBORDER;
	double right  = width + HBORDER;
	double bottom = height + VBORDER;
	double top    = -VBORDER;
	glOrtho (left, right, bottom, top, 1.0, -1.0);

	((AGlActor*)scene)->region = (AGliRegion){
		.x2 = width,
		.y2 = height,
	};

	layers.wa->canvas->samples_per_pixel = 0.1 * ((float)waveform_get_n_frames(layers.wa->waveform)) / width;
	WaveformContext* wfc = layers.wa->canvas;
	wf_context_set_zoom(wfc, wf_context_get_zoom(wfc) ? wf_context_get_zoom(wfc) : 1.0);
}


static void
_add_key_handlers ()
{
	if(!key_handlers){
		key_handlers = g_hash_table_new(g_int_hash, g_int_equal);

		int i = 0; while(true){
			Key* key = &keys[i];
			if(i > 100 || !key->key) break;
			g_hash_table_insert(key_handlers, &key->key, key->handler);
			i++;
		}
	}
}


static void
nav_up (gpointer user_data)
{
	PF0;
}


static void
nav_down (gpointer user_data)
{
	PF0;
}


static void
zoom_in (gpointer user_data)
{
	PF0;
	WaveformContext* wfc = layers.wa->canvas;
	wf_context_set_zoom(wfc, wf_context_get_zoom(wfc) * 1.5);
}


static void
zoom_out (gpointer user_data)
{
	PF0;
	WaveformContext* wfc = layers.wa->canvas;
	wf_context_set_zoom(wfc, wf_context_get_zoom(wfc) / 1.5);
}


