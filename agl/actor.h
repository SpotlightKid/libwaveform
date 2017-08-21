/**
* +----------------------------------------------------------------------+
* | This file is part of the Ayyi project. http://www.ayyi.org           |
* | copyright (C) 2013-2017 Tim Orford <tim@orford.org>                  |
* +----------------------------------------------------------------------+
* | This program is free software; you can redistribute it and/or modify |
* | it under the terms of the GNU General Public License version 3       |
* | as published by the Free Software Foundation.                        |
* +----------------------------------------------------------------------+
*
*/

#ifndef __agl_actor_h__
#define __agl_actor_h__
#include <X11/Xlib.h>
#include <gtkglext-1.0/gdk/gdkgl.h>
#include <gtkglext-1.0/gtk/gtkgl.h>
#ifdef USE_SDL
#  include "SDL2/SDL.h"
#endif
#include "transition/transition.h"
#include "agl/typedefs.h"
#include "agl/utils.h"
#include "gtk/gtk.h"

#undef AGL_DEBUG_ACTOR
#define AGL_ACTOR_RENDER_CACHE

typedef AGlActor* (AGlActorNew)       (GtkWidget*);
typedef void      (*AGlActorSetState) (AGlActor*);
typedef bool      (*AGlActorPaint)    (AGlActor*);
typedef bool      (*AGlActorOnEvent)  (AGlActor*, GdkEvent*, AGliPt xy);
typedef void      (*AGlActorFn)       (AGlActor*);

typedef struct _AGlActorContext AGlActorContext;
typedef int AGlActorType;

typedef enum {
	CONTEXT_TYPE_GTK = 0,
	CONTEXT_TYPE_SDL,
	CONTEXT_TYPE_GLX,
} ContextType;

typedef struct {
	AGlActorType type;
	char*        name;
	AGlActorNew* new;
} AGlActorClass;

struct _AGlActor {
	AGlActorClass*   class;
	char*            name;
	AGlActor*        parent;
	AGlRootActor*    root;

	AGlActorFn       init;            // called once when gl context is available.
	AGlActorFn       set_size;        // called when the parent widget is resized.
	AGlActorSetState set_state;       // called once per expose to set opengl state
	AGlActorFn       invalidate;      // clear fbo caches (and most likely other cached render information too)
	AGlActorPaint    paint;           // called multiple times per expose, once for each object.
	AGlActorOnEvent  on_event;
	AGlActorFn       free;

	AGliRegion       region;          // position and size. {int x1, y1, x2, y2}
	AGliRegion       scrollable;      // larger area within which the actor region is visible {int x1, y1, x2, y2}. optional except for root actor where it is mandatory.
	AGlShader*       program;
	uint32_t         colour;          // rgba
	int              z;               // controls the order objects with the same parent are drawn.
	bool             disabled;        // when disabled, actor and children are greyed-out and are non-interactive.
	GList*           children;        // type AGlActor
	GList*           transitions;     // list of WfAnimation*'s that are currently active.
#ifdef AGL_ACTOR_RENDER_CACHE
	AGlFBO*          fbo;
	struct {
	    bool         enabled;         // if false, caching must be disabled. if true, caching is used only if fbo is present.
	    bool         valid;
	    AGliPt       offset;
	    AGliPt       size_request;    // actors can use this to specify an oversized cache to allow for translations.
	}                cache;
#endif
};


AGlActor* agl_actor__new             ();
AGlActor* agl_actor__new_root        (GtkWidget*);
AGlActor* agl_actor__new_root_       (ContextType);
void      agl_actor__free            (AGlActor*);
AGlActor* agl_actor__add_child       (AGlActor*, AGlActor*);
void      agl_actor__remove_child    (AGlActor*, AGlActor*);
AGlActor* agl_actor__replace_child   (AGlActor*, AGlActor*, AGlActor*);
void      agl_actor__paint           (AGlActor*);
void      agl_actor__set_size        (AGlActor*);
void      agl_actor__set_use_shaders (AGlRootActor*, gboolean);
void      agl_actor__grab            (AGlActor*);
void      agl_actor__invalidate      (AGlActor*);
void      agl_actor__enable_cache    (AGlActor*, bool);
void      agl_actor__start_transition(AGlActor*, GList* animatables, AnimationFn done, gpointer);
bool      agl_actor__is_disabled     (AGlActor*);
bool      agl_actor__on_event        (AGlRootActor*, GdkEvent*);
bool      agl_actor__xevent          (AGlRootActor*, XEvent*);
AGlActor* agl_actor__find_by_name    (AGlActor*, const char*);
AGlActor* agl_actor__find_by_z       (AGlActor*, int);
AGliPt    agl_actor__find_offset     (AGlActor*);
bool      agl_actor__null_painter    (AGlActor*);
bool      agl_actor__on_expose       (GtkWidget*, GdkEventExpose*, gpointer);

#ifdef DEBUG
void      agl_actor__print_tree      (AGlActor*);
#endif

struct _AGlRootActor {
   AGlActor          actor;
   uint32_t          bg_colour;      // rgba
   bool              enable_animations;

   AGlActor*         selected;
   AGlActor*         hovered;

   void              (*draw)(AGlScene*, gpointer); // application callback - called when the application needs to initiate a redraw.

   gpointer          user_data;

   union {
		struct {
			GtkWidget*     widget;
			GdkGLContext*  context;
			GdkGLDrawable* drawable;
		}          gdk;
#ifdef USE_SDL
		struct {
			SDL_GLContext context;
		}          sdl;
#endif
		struct {
		}            glx;
   }                 gl;
   ContextType       type;

#ifdef USE_FRAME_CLOCK
   bool              is_animating;
#endif
};

struct _AGlTextureActor {
   AGlActor          actor;
   guint             texture[1];
};

struct _AGlActorContext {
   AGlActor*         grabbed;         // to enable dragging outside an actors boundaries.
};

#ifdef __agl_actor_c__
AGlActorContext actor_context;
#else
extern AGlActorContext actor_context;
#endif

#define agl_actor__width(A)      ((A)->region.x2 - (A)->region.x1)
#define agl_actor__height(A)     ((A)->region.y2 - (A)->region.y1)
#define agl_actor__is_hovered(A) (A->root->hovered == A)

#define AGL_HANDLED TRUE
#define AGL_NOT_HANDLED FALSE

#ifdef USE_SDL
#  define actor_is_sdl(RA) (RA && RA->type == CONTEXT_TYPE_SDL)
#else
#  define actor_is_sdl(RA) false
#endif
#define actor_not_is_gtk(RA) (RA && RA->type != CONTEXT_TYPE_GTK)

#ifdef __agl_actor_c__
bool __wf_drawing = FALSE;
int __draw_depth = 0;
#else
extern bool __wf_drawing;
extern int __draw_depth;
#endif

#define AGL_ACTOR_START_DRAW(A) \
	if(__wf_drawing){ gwarn("AGL_ACTOR_START_DRAW: already drawing"); } \
	__draw_depth++; \
	__wf_drawing = TRUE; \
	if (actor_is_sdl(((AGlRootActor*)A)) || (__draw_depth > 1) || gdk_gl_drawable_gl_begin (((AGlRootActor*)A)->gl.gdk.drawable, ((AGlRootActor*)A)->gl.gdk.context)) {

#define AGL_ACTOR_END_DRAW(A) \
	__draw_depth--; \
	if(actor_is_sdl(((AGlRootActor*)A))){ \
		if(!__draw_depth) gdk_gl_drawable_gl_end(((AGlRootActor*)A)->gl.gdk.drawable); \
		else { gwarn("!! gl_begin fail"); } \
	} \
	} \
	(__wf_drawing = FALSE);

#endif
