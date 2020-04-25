/* Dia -- an diagram creation/manipulation program -*- c -*-
 * Copyright (C) 1998 Alexander Larsson
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
#ifndef HIGHLIGHT_H
#define HIGHLIGHT_H

#include "glib.h"

#include "diatypes.h"
#include "diagram.h"
#include "color.h"

/* Each object holds the color it is highlighted with.
 */

/*
 * Set an object to be highlighted with a color border..
 * If color is NULL, a standard #FF0000 color (red) is used.
 * The exact method used for highlighting depends on the renderer.
 */
void highlight_object(DiaObject *obj, DiaHighlightType type, Diagram *dia);
/*
 * Remove highlighting from an object.
 */
void highlight_object_off(DiaObject *obj, Diagram *dia);
/** Reset a diagram to have no highlighted objects */
void highlight_reset_all(Diagram *dia);

#endif
