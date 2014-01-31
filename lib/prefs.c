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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <string.h>

#include "prefs.h"
#include "widgets.h"

DiaUnit length_unit = DIA_UNIT_CENTIMETER;
DiaUnit fontsize_unit = DIA_UNIT_POINT;

void 
prefs_set_length_unit(gchar* unit) {
  GList *name_list = get_units_name_list();
  int i;

  for (i = 0; name_list != NULL; name_list = g_list_next(name_list), i++) {
    if (!strcmp(unit, (gchar*) name_list->data)) {
      length_unit = i;
      return;
    }
  }
  length_unit = DIA_UNIT_CENTIMETER;
}

void
prefs_set_fontsize_unit(gchar* unit) {
  GList *name_list = get_units_name_list();
  int i;

  for (i = 0; name_list != NULL; name_list = g_list_next(name_list), i++) {
    if (!strcmp(unit, (gchar*) name_list->data)) {
      fontsize_unit = i;
      return;
    }
  }
  fontsize_unit = DIA_UNIT_POINT;
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

