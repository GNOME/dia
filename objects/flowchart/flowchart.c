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
#include "intl.h"
#include "object.h"
#include "custom.h"

extern ObjectType fc_box_type;
extern ObjectType pgram_type;
extern ObjectType diamond_type;
extern ObjectType fc_ellipse_type;

int get_version(void) {
  return 0;
}

void register_objects(void)
{
  object_register_type(&fc_box_type);
  object_register_type(&pgram_type);
  object_register_type(&diamond_type);
  object_register_type(&fc_ellipse_type);
}

void register_sheets(void)
{
}
