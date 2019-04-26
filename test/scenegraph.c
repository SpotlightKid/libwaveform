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
*/
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
# define GLX_GLXEXT_PROTOTYPES
#include "gdk/gdk.h"
#include "agl/ext.h"
#define __wf_private__ // for dbg
#include "waveform/waveform.h"
#include "agl/actor.h"
#include "waveform/actors/background.h"
#include "waveform/actors/plain.h"
#define __glx_test__
#include "test/common2.h"

extern char* basename(const char*);

static GLboolean print_info = GL_FALSE;

static int  current_time           ();
static void make_extension_table   (const char*);
static bool is_extension_supported (const char*);
static void show_refresh_rate      (Display*);
static void on_window_resize       (int, int);
static void event_loop             (Display*, Window);
static void _add_key_handlers      ();

static AGlActor* cache_actor       (void*);

static void scene_needs_redraw (AGlScene* scene, gpointer _){ scene->gl.glx.needs_draw = True; }

#define BENCHMARK
#define NUL '\0'

PFNGLXGETFRAMEUSAGEMESAPROC get_frame_usage = NULL;


static char** extension_table = NULL;
static unsigned num_extensions;

static AGlRootActor* scene = NULL;
struct {
	AGlActor *bg, *l1, *l2, *l3, *l4, *l5;
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
	/*
	XK_Left
	XK_Right
	*/
};

static const struct option long_options[] = {
	{ "help",             0, NULL, 'h' },
	{ "non-interactive",  0, NULL, 'n' },
};

static const char* const short_options = "nh";

static const char* const usage =
	"Usage: %s [OPTIONS]\n\n"
	"\n"
	"Options:\n"
	"  --help\n"
	"  -info                   Display GL information\n"
	"  -swap N                 Swap no more than once per N vertical refreshes\n"
	"  -forcegetrate           Try to use glXGetMscRateOML function\n"
	"\n";


int
main (int argc, char *argv[])
{
	Window win;
	GLXContext ctx;
	int swap_interval = 1;
	GLboolean do_swap_interval = GL_FALSE;
	GLboolean force_get_rate = GL_FALSE;
	PFNGLXSWAPINTERVALMESAPROC set_swap_interval = NULL;
	PFNGLXGETSWAPINTERVALMESAPROC get_swap_interval = NULL;
	int width = 300, height = 300;

	int i; for (i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-info") == 0) {
			print_info = GL_TRUE;
		}
		else if (strcmp(argv[i], "-swap") == 0 && i + 1 < argc) {
			swap_interval = atoi( argv[i+1] );
			do_swap_interval = GL_TRUE;
			i++;
		}
		else if (strcmp(argv[i], "-forcegetrate") == 0) {
			/* This option was put in because some DRI drivers don't support the
			 * full GLX_OML_sync_control extension, but they do support
			 * glXGetMscRateOML.
			 */
			force_get_rate = GL_TRUE;
		}
	}

	int opt;
	while((opt = getopt_long (argc, argv, short_options, long_options, NULL)) != -1) {
		switch(opt) {
			case 'h':
				printf(usage, basename(argv[0]));
				exit(EXIT_SUCCESS);
				break;
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

	make_window(dpy, "waveformscenegraphtest", width, height, &win, &ctx);
	XMapWindow(dpy, win);
	glXMakeCurrent(dpy, win, ctx);

	// TODO is make_extension_table needed as well as agl_get_extensions ?
	make_extension_table((char*)glXQueryExtensionsString(dpy, DefaultScreen(dpy)));
	has_OML_sync_control = is_extension_supported("GLX_OML_sync_control");
	has_SGI_swap_control = is_extension_supported("GLX_SGI_swap_control");
	has_MESA_swap_control = is_extension_supported("GLX_MESA_swap_control");
	has_MESA_swap_frame_usage = is_extension_supported("GLX_MESA_swap_frame_usage");

	agl_gl_init();

	if (has_MESA_swap_control) {
		set_swap_interval = (PFNGLXSWAPINTERVALMESAPROC) glXGetProcAddressARB((const GLubyte*) "glXSwapIntervalMESA");
		get_swap_interval = (PFNGLXGETSWAPINTERVALMESAPROC) glXGetProcAddressARB((const GLubyte*) "glXGetSwapIntervalMESA");
	}
	else if (has_SGI_swap_control) {
		set_swap_interval = (PFNGLXSWAPINTERVALMESAPROC) glXGetProcAddressARB((const GLubyte*) "glXSwapIntervalSGI");
	}

	if (has_MESA_swap_frame_usage) {
		get_frame_usage = (PFNGLXGETFRAMEUSAGEMESAPROC)  glXGetProcAddressARB((const GLubyte*) "glXGetFrameUsageMESA");
	}

	if(print_info){
		printf("GL_RENDERER   = %s\n", (char*)glGetString(GL_RENDERER));
		printf("GL_VERSION    = %s\n", (char*)glGetString(GL_VERSION));
		printf("GL_VENDOR     = %s\n", (char*)glGetString(GL_VENDOR));
		printf("GL_EXTENSIONS = %s\n", (char*)glGetString(GL_EXTENSIONS));
		if(has_OML_sync_control || force_get_rate){
			show_refresh_rate(dpy);
		}

		if(get_swap_interval){
			printf("Default swap interval = %d\n", (*get_swap_interval)());
		}
	}

	if(do_swap_interval){
		if(set_swap_interval){
			if(((swap_interval == 0) && !has_MESA_swap_control) || (swap_interval < 0)){
				printf( "Swap interval must be non-negative or greater than zero "
					"if GLX_MESA_swap_control is not supported.\n" );
			}
			else {
				(*set_swap_interval)(swap_interval);
			}

			if (print_info && (get_swap_interval != NULL)){
				printf("Current swap interval = %d\n", (*get_swap_interval)());
			}
		}
		else {
			printf("Unable to set swap-interval. Neither GLX_SGI_swap_control "
				"nor GLX_MESA_swap_control are supported.\n" );
		}
	}

	// -----------------------------------------------------------

	g_main_loop_new(NULL, true);

	scene = (AGlRootActor*)agl_actor__new_root_(CONTEXT_TYPE_GLX);

	scene->draw = scene_needs_redraw;

	agl_actor__add_child((AGlActor*)scene, layers.bg = background_actor(NULL));

	agl_actor__add_child((AGlActor*)scene, layers.l1 = plain_actor(NULL));
	layers.l1->colour = 0x99ff9999;
	layers.l1->region = (AGliRegion){10, 15, 60, 65};

	agl_actor__add_child((AGlActor*)scene, layers.l2 = plain_actor(NULL));
	layers.l2->colour = 0x9999ff99;
	layers.l2->region = (AGliRegion){40, 45, 90, 95};

	// now show the same 2 squares again, but wrapped in a caching actor

	agl_actor__add_child((AGlActor*)scene, layers.l3 = cache_actor(NULL));
	layers.l3->region = (AGliRegion){10, 135, 90, 215};

	agl_actor__add_child(layers.l3, layers.l4 = plain_actor(NULL));
	layers.l4->colour = 0x99ff9999;
	layers.l4->region = (AGliRegion){0, 0, 50, 50};

	agl_actor__add_child(layers.l3, layers.l5 = plain_actor(NULL));
	layers.l5->colour = 0x9999ff99;
	layers.l5->region = (AGliRegion){30, 30, 80, 80};

	// -----------------------------------------------------------

	on_window_resize(width, height);

	_add_key_handlers();

	event_loop(dpy, win);

	glXDestroyContext(dpy, ctx);
	XDestroyWindow(dpy, win);
	XCloseDisplay(dpy);

	return 0;
}


static void
draw (void)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	agl_actor__paint((AGlActor*)scene);

	agl_set_font_string("Roboto 8");
	agl_print(10, 2, 0, 0xffffffff, "You should see 2 overlapping squares");
	agl_print(10, 106, 0, 0xffffffff, "The two squares below are from an fbo cache.");
	agl_print(10, 120, 0, 0xffffffff, "They should be the same as above");
}


static void
on_window_resize (int width, int height)
{
	// FIXME adding border messes up 1:1 ratio
	#define HBORDER 0 //50
	#define VBORDER 0 //50
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

	agl_actor__set_size((AGlActor*)scene);
}


static void
event_loop (Display* dpy, Window win)
{
	float frame_usage = 0.0;

	while (1) {
		while (XPending(dpy) > 0) {
			XEvent event;
			XNextEvent(dpy, &event);
			switch (event.type) {
				case Expose:
					scene->gl.glx.needs_draw = True;
					break;
				case ConfigureNotify:
					on_window_resize(event.xconfigure.width, event.xconfigure.height);
					scene->gl.glx.needs_draw = True;
					break;
				case KeyPress: {
					int code = XLookupKeysym(&event.xkey, 0);

					KeyHandler* handler = g_hash_table_lookup(key_handlers, &code);
					if(handler){
						handler(NULL);
					}else{
						char buffer[10];
						XLookupString(&event.xkey, buffer, sizeof(buffer), NULL, NULL);
						if (buffer[0] == 27 || buffer[0] == 'q') {
							/* escape */
							return;
						}
					}
				}
			}
		}

		if(scene->gl.glx.needs_draw){
			draw();
			glXSwapBuffers(dpy, win);
			scene->gl.glx.needs_draw = false;
		}

		if (get_frame_usage) {
			GLfloat temp;

			(*get_frame_usage)(dpy, win, & temp);
			frame_usage += temp;
		}

		/* calc framerate */
		if(print_info){
			static int t0 = -1;
			static int frames = 0;
			int t = current_time();

			if (t0 < 0) t0 = t;

			frames++;

			if (t - t0 >= 5.0) {
				GLfloat seconds = t - t0;
				GLfloat fps = frames / seconds;
				if (get_frame_usage) {
					printf("%d frames in %3.1f seconds = %6.3f FPS (%3.1f%% usage)\n", frames, seconds, fps, (frame_usage * 100.0) / (float) frames );
				}
				else {
					printf("%d frames in %3.1f seconds = %6.3f FPS\n", frames, seconds, fps);
				}
				fflush(stdout);

				t0 = t;
				frames = 0;
				frame_usage = 0.0;
			}
		}

		g_main_context_iteration(NULL, false); // update animations
	}
}


/**
 * Display the refresh rate of the display using the GLX_OML_sync_control
 * extension.
 */
static void
show_refresh_rate (Display* dpy)
{
#if defined(GLX_OML_sync_control) && defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L)
   int32_t  n;
   int32_t  d;

   PFNGLXGETMSCRATEOMLPROC get_msc_rate = (PFNGLXGETMSCRATEOMLPROC)glXGetProcAddressARB((const GLubyte*) "glXGetMscRateOML");
   if (get_msc_rate != NULL) {
      (*get_msc_rate)(dpy, glXGetCurrentDrawable(), &n, &d);
      printf( "refresh rate: %.1fHz\n", (float) n / d);
      return;
   }
#endif
   printf( "glXGetMscRateOML not supported.\n" );
}


/**
 * Fill in the table of extension strings from a supplied extensions string
 * (as returned by glXQueryExtensionsString).
 *
 * \param string   String of GLX extensions.
 * \sa is_extension_supported
 */
static void
make_extension_table (const char* string)
{
	char** string_tab;
	unsigned  num_strings;
	unsigned  base;
	unsigned  idx;
	unsigned  i;

	/* Count the number of spaces in the string.  That gives a base-line
	 * figure for the number of extension in the string.
	 */

	num_strings = 1;
	for (i = 0; string[i] != NUL; i++) {
		if (string[i] == ' ') {
			num_strings++;
		}
	}

	string_tab = (char**)malloc(sizeof(char*) * num_strings);
	if (string_tab == NULL) {
		return;
	}

	base = 0;
	idx = 0;

	while (string[base] != NUL) {
		// Determine the length of the next extension string.

		for (i = 0; (string[base + i] != NUL) && (string[base + i] != ' '); i++) {
			/* empty */ ;
		}

		if(i > 0){
			/* If the string was non-zero length, add it to the table.  We
			 * can get zero length strings if there is a space at the end of
			 * the string or if there are two (or more) spaces next to each
			 * other in the string.
			 */

			string_tab[ idx ] = malloc(sizeof(char) * (i + 1));
			if(string_tab[idx] == NULL){
				unsigned j = 0;

				for(j = 0; j < idx; j++){
					free(string_tab[j]);
				}

				free(string_tab);

				return;
			}

			(void) memcpy(string_tab[idx], & string[base], i);
			string_tab[idx][i] = NUL;
			idx++;
		}

		// Skip to the start of the next extension string.
		for (base += i; (string[ base ] == ' ') && (string[ base ] != NUL); base++ ) {
			/* empty */;
		}
	}

	extension_table = string_tab;
	num_extensions = idx;
}


/**
 * Determine of an extension is supported.  The extension string table
 * must have already be initialized by calling \c make_extension_table.
 *
 * \praram ext  Extension to be tested.
 * \return GL_TRUE of the extension is supported, GL_FALSE otherwise.
 * \sa make_extension_table
 */
static bool
is_extension_supported(const char* ext)
{
	unsigned i;

	for (i = 0; i < num_extensions; i++) {
		if (strcmp(ext, extension_table[i]) == 0) {
			return GL_TRUE;
		}
	}

	return false;
}


#ifdef BENCHMARK
#include <sys/time.h>
#include <unistd.h>

/* return current time (in seconds) */
static int
current_time(void)
{
   struct timeval tv;
#ifdef __VMS
   (void) gettimeofday(&tv, NULL);
#else
   struct timezone tz;
   (void) gettimeofday(&tv, &tz);
#endif
   return (int) tv.tv_sec;
}

#else /*BENCHMARK*/

/* dummy */
static int
current_time(void)
{
   return 0;
}

#endif /*BENCHMARK*/


static void
_add_key_handlers()
{
	if(!key_handlers){
		key_handlers = g_hash_table_new(g_int_hash, g_int_equal);

		int i = 0; while(true) {
			Key* key = &keys[i];
			if(i > 100 || !key->key) break;
			g_hash_table_insert(key_handlers, &key->key, key->handler);
			i++;
		}
	}
}


static AGlActor*
cache_actor (void* _)
{
	void cache_set_state (AGlActor* actor)
	{
	}

	bool cache_paint (AGlActor* actor)
	{
		return true;
	}

	void cache_init (AGlActor* actor)
	{
#ifdef AGL_ACTOR_RENDER_CACHE
		actor->fbo = agl_fbo_new(agl_actor__width(actor), agl_actor__height(actor), 0, 0);
		actor->cache.enabled = true;
#endif
	}

	return AGL_NEW(AGlActor,
		.name = "Cache",
		.init = cache_init,
		.paint = cache_paint,
		.set_state = cache_set_state
	);
}


static void
nav_up(gpointer user_data)
{
	PF0;
}


static void
nav_down(gpointer user_data)
{
	PF0;
}


static void
zoom_in(gpointer user_data)
{
}


static void
zoom_out(gpointer user_data)
{
}


