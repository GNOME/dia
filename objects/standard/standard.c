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
#include "object.h"
#include "sheet.h"

extern ObjectType *_arc_type;
extern ObjectType *_box_type;
extern ObjectType *_ellipse_type;
extern ObjectType *_line_type;
extern ObjectType *_zigzagline_type;
extern ObjectType *_textobj_type;

int get_version(void) {
  return 0;
}

void register_objects(void) {
  object_register_type(_arc_type);
  object_register_type(_box_type);
  object_register_type(_ellipse_type);
  object_register_type(_line_type);
  object_register_type(_zigzagline_type);
  object_register_type(_textobj_type);
}

void register_sheets(void) {
}
