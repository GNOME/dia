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
#include "network.h"

Color computer_color = { 0.7, 0.7, 0.7 };

extern ObjectType computer_type;
extern ObjectType monitor_type;
extern ObjectType disc_type;
extern ObjectType bus_type;
extern ObjectType bus_type_std;
extern ObjectType printer_type;
extern ObjectType flash_type;
extern ObjectType hub_type;
extern ObjectType modularswitch_type;
extern ObjectType rj45plug_type;
extern ObjectType sceadplug_type;
extern ObjectType modem_type;
extern ObjectType antenna_type;

int get_version(void) {
  return 0;
}

void register_objects(void) {
  object_register_type(&computer_type);
  object_register_type(&monitor_type);
  object_register_type(&disc_type);
  object_register_type(&bus_type_std);
  object_register_type(&bus_type);
  object_register_type(&printer_type);
  object_register_type(&flash_type);
  object_register_type(&hub_type);
  object_register_type(&modularswitch_type);
  object_register_type(&rj45plug_type);
  object_register_type(&sceadplug_type);
  object_register_type(&modem_type);
  object_register_type(&antenna_type);
}

extern SheetObject computer_sheetobj;
extern SheetObject monitor_sheetobj;
extern SheetObject disc_sheetobj;
extern SheetObject bus_sheetobj;
extern SheetObject printer_sheetobj;
extern SheetObject flash_sheetobj;
extern SheetObject hub_sheetobj;
extern SheetObject modularswitch_sheetobj;
extern SheetObject rj45plug_sheetobj;
extern SheetObject sceadplug_sheetobj;
extern SheetObject modem_sheetobj;
extern SheetObject antenna_sheetobj;

void register_sheets(void) {
  Sheet *sheet;

  sheet = new_sheet(_("Network"),
		    _("Objects to design network diagrams with"));
  sheet_append_sheet_obj(sheet, &computer_sheetobj);
  sheet_append_sheet_obj(sheet, &monitor_sheetobj);
  sheet_append_sheet_obj(sheet, &disc_sheetobj);
  sheet_append_sheet_obj(sheet, &bus_sheetobj);
  sheet_append_sheet_obj(sheet, &printer_sheetobj);
  sheet_append_sheet_obj(sheet, &flash_sheetobj);
  sheet_append_sheet_obj(sheet, &hub_sheetobj);
  sheet_append_sheet_obj(sheet, &modularswitch_sheetobj);
  sheet_append_sheet_obj(sheet, &rj45plug_sheetobj);
  sheet_append_sheet_obj(sheet, &sceadplug_sheetobj);
  sheet_append_sheet_obj(sheet, &modem_sheetobj);
  sheet_append_sheet_obj(sheet, &antenna_sheetobj);

  register_sheet(sheet);

}

