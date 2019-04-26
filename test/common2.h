
/**
* +----------------------------------------------------------------------+
* | This file is part of the Ayyi project. http://ayyi.org               |
* | copyright (C) 2013-2018 Tim Orford <tim@orford.org>                  |
* +----------------------------------------------------------------------+
* | This program is free software; you can redistribute it and/or modify |
* | it under the terms of the GNU General Public License version 3       |
* | as published by the Free Software Foundation.                        |
* +----------------------------------------------------------------------+
*
*/

/*
 *  Common code for all tests
 */

void  set_log_handlers   ();
char* find_wav           (const char*);
const char* find_data_dir();

typedef void (KeyHandler)(gpointer);

typedef struct
{
	int         key;
	KeyHandler* handler;
} Key;

typedef struct
{
	guint          timer;
	KeyHandler*    handler;
} KeyHold;

#ifdef __glx_test__
static GLboolean has_OML_sync_control = GL_FALSE;
static GLboolean has_SGI_swap_control = GL_FALSE;
static GLboolean has_MESA_swap_control = GL_FALSE;
static GLboolean has_MESA_swap_frame_usage = GL_FALSE;

#ifndef GLX_MESA_swap_control
typedef GLint (*PFNGLXSWAPINTERVALMESAPROC)    (unsigned interval);
typedef GLint (*PFNGLXGETSWAPINTERVALMESAPROC) (void);
#endif

#if !defined( GLX_OML_sync_control ) && defined( _STDINT_H )
#define GLX_OML_sync_control 1
typedef Bool (*PFNGLXGETMSCRATEOMLPROC) (Display*, GLXDrawable, int32_t* numerator, int32_t* denominator);
#endif

#ifndef GLX_MESA_swap_frame_usage
#define GLX_MESA_swap_frame_usage 1
typedef int (*PFNGLXGETFRAMEUSAGEMESAPROC) (Display*, GLXDrawable, float* usage);
#endif

#endif

#ifdef __common2_c__
char grey     [16] = "\x1b[2;39m"; // 2 = dim
char yellow   [16] = "\x1b[1;33m";
char yellow_r [16] = "\x1b[30;43m";
char white__r [16] = "\x1b[30;47m";
char cyan___r [16] = "\x1b[30;46m";
char magent_r [16] = "\x1b[30;45m";
char blue_r   [16] = "\x1b[30;44m";
char red      [16] = "\x1b[1;31m";
char red_r    [16] = "\x1b[30;41m";
char green    [16] = "\x1b[1;32m";
char green_r  [16] = "\x1b[30;42m";
char ayyi_warn[32] = "\x1b[1;33mwarning:\x1b[0;39m";
char ayyi_err [32] = "\x1b[1;31merror!\x1b[0;39m";
char go_rhs   [32] = "\x1b[A\x1b[50C"; //go up one line, then goto column 60
char ok       [32] = " [ \x1b[1;32mok\x1b[0;39m ]";
char fail     [32] = " [\x1b[1;31mFAIL\x1b[0;39m]";
#else
extern char grey     [16];
extern char yellow   [16];
extern char yellow_r [16];
extern char white__r [16];
extern char cyan___r [16];
extern char magent_r [16];
extern char blue_r   [16];
extern char red      [16];
extern char red_r    [16];
extern char green    [16];
extern char green_r  [16];
extern char ayyi_warn[32];
extern char ayyi_err [32];
extern char go_rhs   [32];
extern char ok       [];
extern char fail     [];
#endif

#ifdef GLX_H
void make_window        (Display*, const char*, int width, int height, Window*, GLXContext*);
#endif

