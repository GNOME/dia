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
#include "sheet.h"

#include "shape_info.h"

void
custom_object_new(ShapeInfo *info, ObjectType **otype, SheetObject **sheetobj);

int get_version(void) {
  return 0;
}

GList *sheet_objs;

void register_objects(void) {
  ObjectType  *obj_type;
  SheetObject *sheet_obj;
  ShapeInfo *info;

  sheet_objs = NULL;

  info = shape_info_load("/home/james/gnomecvs/dia/objects/custom/test.xml");
  shape_info_print(info);
  custom_object_new(info, &obj_type, &sheet_obj);
  sheet_objs = g_list_append(sheet_objs, sheet_obj);

  object_register_type(obj_type);
}

void register_sheets(void) {
  GList *tmp;
  Sheet *sheet;

  sheet = new_sheet(_("Custom"),
		    _("Custom obj test"));
  for (tmp = sheet_objs; tmp; tmp = tmp->next)
    sheet_append_sheet_obj(sheet, (SheetObject *)tmp->data);

  register_sheet(sheet);
}
