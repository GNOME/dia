/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 * 
 * A string pre-renderer. Piggybacks on a normal renderer, and turns 
 * DrawString calls into PreDrawString calls ; all other calls are ignored.
 * Copyright (C) 2001 Cyrille Chepelov <chepelov@calixo.net>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef STRING_PRERENDERER_H
#define STRING_PRERENDERER_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "render.h"

typedef struct _StringPrerenderer StringPrerenderer;

struct _StringPrerenderer {
  Renderer renderer;
  Renderer *org_renderer; /* not owned here */
};

StringPrerenderer *create_string_prerenderer(Renderer *renderer);
void destroy_string_prerenderer(const StringPrerenderer *prerenderer);

#endif /* STRING_PRERENDERER_H */


