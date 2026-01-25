/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * This is code is a ripoff from lib/text.c's text_draw() routine, modified
 * for the GRAFCET action text's strange behaviour.
 * The variations from the original code are Copyright(C) 2000 Cyrille Chepelov
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

#pragma once

#include "dia-text.h"
#include "diarenderer.h"
#include "geometry.h"

G_BEGIN_DECLS

void   action_text_draw             (DiaText *text, DiaRenderer  *renderer);
void   action_text_calc_boundingbox (DiaText *text, DiaRectangle *box);
double action_text_spacewidth       (DiaText *text);

G_END_DECLS
