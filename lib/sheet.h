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
#ifndef SHEET_H
#define SHEET_H

#include <glib.h>

typedef struct _Sheet Sheet;
typedef struct _SheetObject SheetObject;

#include "object.h"

struct _SheetObject {
  char *object_type;
  char *description;
  char **pixmap; /* in xpm format */

  void *user_data;

  gboolean line_break;

  char *pixmap_file; /* fallback if pixmap is NULL */
};

struct _Sheet {
  char *name;
  char *description;

  GSList *objects; /* list of SheetObject */
};

Sheet *new_sheet(char *name, char *description);
void sheet_prepend_sheet_obj(Sheet *sheet, SheetObject *type);
void sheet_append_sheet_obj(Sheet *sheet, SheetObject *type);
void register_sheet(Sheet *sheet);
GSList *get_sheets_list(void);

void load_all_sheets(void);

#endif /* SHEET_H */
