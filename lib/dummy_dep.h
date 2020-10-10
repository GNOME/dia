/* Dia -- an diagram creation/manipulation program
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
#include <glib.h>
#include "connection.h"
#include "element.h"
#include "font.h"
#include "text.h"
#include "orth_conn.h"
#include "arrows.h"
#include "utils.h"
#include "poly_conn.h"
#include "polyshape.h"
#include "bezier_conn.h"
#include "beziershape.h"
#include "widgets.h"
#include "intl.h"
#include "connpoint_line.h"
#include "properties.h"
#include "dynamic_obj.h"
#include "connectionpoint.h"
#include "diafontselector.h"
#include "dia-state-object-change.h"

/* This is a file with dummy dependencies so that all
   object files will be linked into the app.
*/

#ifndef __sgi
static
#endif
void *dummy_dep[] G_GNUC_UNUSED = {
  connection_move_handle,
  element_update_boundingbox,
  orthconn_update_data,
  polyconn_init,
  polyshape_init,
  bezierconn_init,
  beziershape_init,
  new_text,
  dia_font_new_from_style, /* font.o */
  nearest_pow,
  arrow_draw,
  dia_font_selector_new, /* widgets.o */
  dia_state_object_change_new, /* objchange.o */
  intl_score_locale, /* intl.o */
  connpointline_create, /* connpoint_line.o */
  object_create_props_dialog, /* properties.o */
  dynobj_list_get_dynobj_rate, /* dynamic_obj.o */
  connpoint_update, /* connectionpoint.c */
};
