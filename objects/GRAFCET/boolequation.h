/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * This code renders boolean equations, as needed by the transitions'
 * receptivities and the conditional actions.
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


#ifndef __BOOLEQUATION_H
#define __BOOLEQUATION_H

#include <glib.h>
#include <libxml/tree.h>
#include "dia_xml.h"
#include "geometry.h"
#include "diarenderer.h"

typedef struct _Block Block;
typedef struct {
  DiaFont *font;
  real fontheight;
  Color color;

  Point pos;

  char *value;

  Block *rootblock;

  real width;
  real height;
        /*
  real ascent;
  real descent;
        */
} Boolequation;


extern Boolequation *boolequation_create(const gchar *value, DiaFont *font,
				       real fontheight, Color *color);
extern void boolequation_destroy(Boolequation *rcep);
extern void boolequation_set_value(Boolequation *rcep, const gchar *value);

extern void save_boolequation(ObjectNode obj_node, const gchar *attrname,
			     Boolequation *rcep, DiaContext *ctx);

extern void boolequation_draw(Boolequation *rcep, DiaRenderer *renderer);
extern void boolequation_calc_boundingbox(Boolequation *rcep, DiaRectangle *box);

#endif /* __BOOLEQUATION_H */
