/* xxxxxx -- an diagram creation/manipulation program
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
#include "sheet.h"

static GSList *sheets = NULL;

Sheet *
new_sheet(char *name, char *description)
{
  Sheet *sheet;

  sheet = g_new(Sheet, 1);
  sheet->name = name;
  sheet->description = description;

  sheet->objects = NULL;
  return sheet;
}

void
sheet_prepend_sheet_obj(Sheet *sheet, SheetObject *obj)
{
  sheet->objects = g_slist_prepend( sheet->objects, (gpointer) obj);
}

void
sheet_append_sheet_obj(Sheet *sheet, SheetObject *obj)
{
  sheet->objects = g_slist_append( sheet->objects, (gpointer) obj);
}

void
register_sheet(Sheet *sheet)
{
  sheets = g_slist_append(sheets, (gpointer) sheet);
}

GSList *
get_sheets_list(void)
{
  return sheets;
}
