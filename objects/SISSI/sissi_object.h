/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1999 Alexander Larsson
 *
 * SISSI diagram -  adapted by Luc Cessieux
 * This class could draw the system security
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
#ifndef GENERIC_H
#define GENERIC_H

#include "diatypes.h"
#include <gdk/gdktypes.h>

#include "geometry.h"
#include "polyshape.h"



#include "sissi.h"
#include "sissi_dialog.h"
#include "pixmaps/sissi_object.xpm"

static PropDescription *sissi_object_describe_props(ObjetSISSI *object_sissi);
static DiaObject *sissi_object_create(Point *startpoint,  void *user_data, Handle **handle1, Handle **handle2);
static DiaObject *sissi_object_load(ObjectNode obj_node, int version, const char *filename);
static void sissi_object_get_props(ObjetSISSI *object_sissi, GPtrArray *props);
static void sissi_object_set_props(ObjetSISSI *object_sissi, GPtrArray *props);

#endif /* GENERIC_H */


