/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * This code renders boolean equations, as needed by the transitions' 
 * receptivities.
 *
 * Copyright (C) 2000 Cyrille Chepelov
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


#ifndef __RECEPTIVITY_TEXT_DRAW_H
#define __RECEPTIVITY_TEXT_DRAW_H

#include <glib.h>
#include "geometry.h"
#include "render.h"
#include "lazyprops.h"

typedef struct _Block Block;
typedef struct {
  Font *font;
  real fontheight;
  Color color;

  Point pos;

  const gchar *value;
  
  Block *rootblock;

  real width;
  real height;
  real ascent;
  real descent;
} Receptivity;

  
extern Receptivity *receptivity_create(const gchar *value, Font *font, 
				       real fontheight, Color *color);
extern void receptivity_destroy(Receptivity *rcep);
extern void receptivity_set_value(Receptivity *rcep, const gchar *value);
extern void save_receptivity(ObjectNode *obj_node, const gchar *attrname,
			     Receptivity *rcep);
extern Receptivity *load_receptivity(ObjectNode *obj_node,
				     const gchar *attrname,
				     const gchar *defaultvalue,
				     Font *font,
				     real fontheight,
				     Color *color);
extern void receptivity_set_pos(Receptivity *rcep, Point *pos);
extern void receptivity_draw(Receptivity *rcep, Renderer *renderer);
extern void receptivity_calc_boundingbox(Receptivity *rcep, Rectangle *box);
 
#endif __RECEPTIVITY_TEXT_DRAW_H
