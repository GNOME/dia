/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 2007 Lars Clausen
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

#include <config.h>
#include <string.h>

#include "prefs.h"

DiaUnit length_unit = DIA_UNIT_CENTIMETER;
DiaUnit fontsize_unit = DIA_UNIT_POINT;


void
prefs_set_length_unit (DiaUnit unit) {
  length_unit = unit;
}

void
prefs_set_fontsize_unit (DiaUnit unit) {
  fontsize_unit = unit;
}


DiaUnit
prefs_get_length_unit(void)
{
  return length_unit;
}


DiaUnit
prefs_get_fontsize_unit(void)
{
  return fontsize_unit;
}

