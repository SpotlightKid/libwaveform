/**
* +----------------------------------------------------------------------+
* | This file is part of the Ayyi project. http://ayyi.org               |
* | copyright (C) 2013-2020 Tim Orford <tim@orford.org>                  |
* +----------------------------------------------------------------------+
* | This program is free software; you can redistribute it and/or modify |
* | it under the terms of the GNU General Public License version 3       |
* | as published by the Free Software Foundation.                        |
* +----------------------------------------------------------------------+
*
*/
/*

  WaveformGrid draws timeline markers onto a shared opengl drawable.

*/
#define __wf_private__
#include "config.h"
#include "wf/waveform.h"
#include "waveform/actor.h"
#include "waveform/context.h"
#include "waveform/grid.h"

typedef struct {
    AGlActor         actor;
    WaveformContext* context;
} RulerActor;

static AGl* agl = NULL;

static bool ruler_actor_paint(AGlActor*);


	static void ruler_actor_size(AGlActor* actor)
	{
		actor->region.x2 = agl_actor__width(actor->parent);
	}

	static void ruler_init(AGlActor* actor)
	{
		RulerActor* ruler = (RulerActor*)actor;
		if(agl->use_shaders){
			if(!ruler->context->shaders.ruler->shader.program)
				agl_create_program(&ruler->context->shaders.ruler->shader);
		}
#ifdef AGL_ACTOR_RENDER_CACHE
		actor->fbo = agl_fbo_new(actor->region.x2 - actor->region.x1, actor->region.y2 - actor->region.y1, 0, 0);
#endif
	}

	static void ruler_set_state(AGlActor* actor)
	{
		if(!agl->use_shaders) return;

		#define samples_per_beat(C) (C->sample_rate / (C->bpm / 60.0))

		RulerActor* ruler = (RulerActor*)actor;
		RulerShader* shader = ruler->context->shaders.ruler;
		WaveformContext* context = ruler->context;

		shader->uniform.fg_colour = 0xffffff7f;
		shader->uniform.beats_per_pixel = context->samples_per_pixel / (samples_per_beat(context) * context->zoom->value.f);
		shader->uniform.samples_per_pixel = context->samples_per_pixel;

		agl_use_program((AGlShader*)shader);
	}

AGlActor*
ruler_actor (WaveformActor* wf_actor)
{
	g_return_val_if_fail(wf_actor, NULL);

	agl = agl_get_instance();

	return (AGlActor*)AGL_NEW(RulerActor,
		.actor = {
			.name = "ruler",
			.init = ruler_init,
			.set_state = ruler_set_state,
			.paint = ruler_actor_paint,
			.set_size = ruler_actor_size
		},
		.context = wf_actor->context
	);
}


static bool
ruler_actor_paint(AGlActor* actor)
{
	if(!agl->use_shaders) return false;

#if 0 //shader debugging
	{
		float smoothstep(float edge0, float edge1, float x)
		{
			float t = CLAMP((x - edge0) / (edge1 - edge0), 0.0, 1.0);
			return t * t * (3.0 - 2.0 * t);
		}

		float pixels_per_beat = 1.0 / wfc->priv->shaders.ruler->uniform.beats_per_pixel;
		dbg(0, "ppb=%.2f", pixels_per_beat);
		int x; for(x=0;x<30;x++){
			float m = (x * 100) % ((int)pixels_per_beat * 100);
			float m_ = x - pixels_per_beat * floor(x / pixels_per_beat);
			printf("  %.2f %.2f %.2f\n", m / 100, m_, smoothstep(0.0, 0.5, m_));
		}
	}
#endif

	glPushMatrix();
	glScalef(1.0, -1.0, 1.0);           // inverted vertically to make alignment of marks to bottom easier in the shader
	glTranslatef(0.0, -agl_actor__height(actor), 0.0); // making more negative moves downward
	glRecti(actor->region.x1, 0, actor->region.x2, agl_actor__height(actor));
	glPopMatrix();

	return true;
}


