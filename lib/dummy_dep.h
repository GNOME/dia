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
#include "connection.h"
#include "element.h"
#include "font.h"
#include "text.h"
#include "render_store.h"
#include "render_object.h"
#include "orth_conn.h"
#include "arrows.h"
#include "utils.h"
#include "widgets.h"

/* This is a file with dummy dependencies so that all
   object files will be linked into the app.
*/

static void *dummy_dep[] = {
  connection_move_handle,
  element_update_boundingbox,
  orthconn_update_data,
  new_text,
  font_getfont,
  new_render_store,
  new_render_object,
  nearest_pow,
  arrow_draw,
  dia_font_selector_new /* widgets.o */
};
