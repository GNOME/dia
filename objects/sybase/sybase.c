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

#include "config.h"
#include "intl.h"
#include "sybase.h"

Color computer_color = { 0.7, 0.7, 0.7 };

extern ObjectType dataserver_type;
extern ObjectType repserver_type;
extern ObjectType ltm_type;
extern ObjectType stableq_type;
extern ObjectType client_type;
extern ObjectType rsm_type;

int get_version(void) {
  return 0;
}

void register_objects(void) {
  object_register_type(&dataserver_type);
  object_register_type(&repserver_type);
  object_register_type(&ltm_type);
  object_register_type(&stableq_type);
  object_register_type(&client_type);
  object_register_type(&rsm_type);
}

extern SheetObject dataserver_sheetobj;
extern SheetObject repserver_sheetobj;
extern SheetObject ltm_sheetobj;
extern SheetObject stableq_sheetobj;
extern SheetObject client_sheetobj;
extern SheetObject rsm_sheetobj;

void register_sheets(void) {
  Sheet *sheet;

  sheet = new_sheet(_("Sybase"),
		    _("Objects to design Sybase replication domain diagrams with"));
  sheet_append_sheet_obj(sheet, &dataserver_sheetobj);
  sheet_append_sheet_obj(sheet, &repserver_sheetobj);
  sheet_append_sheet_obj(sheet, &ltm_sheetobj);
  sheet_append_sheet_obj(sheet, &stableq_sheetobj);
  sheet_append_sheet_obj(sheet, &client_sheetobj);
  sheet_append_sheet_obj(sheet, &rsm_sheetobj);

  register_sheet(sheet);

}

