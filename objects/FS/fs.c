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
#include <string.h>
#include <stdio.h>
#include <assert.h>

#include "config.h"
#include "intl.h"
#include "object.h"
#include "sheet.h"

extern ObjectType function_type;
extern ObjectType flow_type;
extern ObjectType orthflow_type;

void register_objects(void) {
  object_register_type(&function_type);
  object_register_type(&flow_type);  
  object_register_type(&orthflow_type);  
}

extern SheetObject function_sheetobj;
extern SheetObject flow_sheetobj;
extern SheetObject orthflow_sheetobj;

int get_version(void) {
  return 0;
}

void register_sheets(void) {
  Sheet *sheet;
  
  sheet = new_sheet(_("FS"),
		    _("Editor for Function Structure Diagrams."));
  sheet_append_sheet_obj(sheet, &function_sheetobj);
  sheet_append_sheet_obj(sheet, &flow_sheetobj);
  sheet_append_sheet_obj(sheet, &orthflow_sheetobj);

  register_sheet(sheet);
}
