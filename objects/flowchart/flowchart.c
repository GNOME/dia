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
#include "custom.h"

#include "pixmaps/display.xpm"
#include "pixmaps/transaction.xpm"
#include "pixmaps/offpageconn.xpm"
#include "pixmaps/document.xpm"
#include "pixmaps/manualop.xpm"
#include "pixmaps/preparation.xpm"
#include "pixmaps/manualinput.xpm"
#include "pixmaps/predefdproc.xpm"
#include "pixmaps/terminal.xpm"
#include "pixmaps/magdisk.xpm"
#include "pixmaps/magtape.xpm"
#include "pixmaps/intstorage.xpm"
#include "pixmaps/merge.xpm"
#include "pixmaps/extract.xpm"
#include "pixmaps/delay.xpm"
#include "pixmaps/sumjunction.xpm"
#include "pixmaps/collate.xpm"
#include "pixmaps/sort.xpm"
#include "pixmaps/or.xpm"

extern ObjectType fc_box_type;
extern ObjectType pgram_type;
extern ObjectType diamond_type;
extern ObjectType fc_ellipse_type;
static ObjectType *display_type = NULL;
static ObjectType *transaction_type = NULL;
static ObjectType *offpageconn_type = NULL;
static ObjectType *doc_type = NULL;
static ObjectType *manualop_type = NULL;
static ObjectType *preparation_type = NULL;
static ObjectType *manualinput_type = NULL;
static ObjectType *predefdproc_type = NULL;
static ObjectType *terminal_type = NULL;
static ObjectType *collate_type = NULL;
static ObjectType *delay_type = NULL;
static ObjectType *extract_type = NULL;
static ObjectType *intstorage_type = NULL;
static ObjectType *magdisk_type = NULL;
static ObjectType *magtape_type = NULL;
static ObjectType *merge_type = NULL;
static ObjectType *or_type = NULL;
static ObjectType *sort_type = NULL;
static ObjectType *sumjunction_type = NULL;

extern SheetObject box_sheetobj;
extern SheetObject pgram_sheetobj;
extern SheetObject diamond_sheetobj;
extern SheetObject ellipse_sheetobj;
static SheetObject *display_sheetobj = NULL;
static SheetObject *transaction_sheetobj = NULL;
static SheetObject *offpageconn_sheetobj = NULL;
static SheetObject *doc_sheetobj = NULL;
static SheetObject *manualop_sheetobj = NULL;
static SheetObject *preparation_sheetobj = NULL;
static SheetObject *manualinput_sheetobj = NULL;
static SheetObject *predefdproc_sheetobj = NULL;
static SheetObject *terminal_sheetobj = NULL;
static SheetObject *collate_sheetobj = NULL;
static SheetObject *delay_sheetobj = NULL;
static SheetObject *extract_sheetobj = NULL;
static SheetObject *intstorage_sheetobj = NULL;
static SheetObject *magdisk_sheetobj = NULL;
static SheetObject *magtape_sheetobj = NULL;
static SheetObject *merge_sheetobj = NULL;
static SheetObject *or_sheetobj = NULL;
static SheetObject *sort_sheetobj = NULL;
static SheetObject *sumjunction_sheetobj = NULL;

int get_version(void) {
  return 0;
}

void register_objects(void) {
  object_register_type(&fc_box_type);
  object_register_type(&pgram_type);
  object_register_type(&diamond_type);
  object_register_type(&fc_ellipse_type);

  custom_object_load(SHAPE_DIR "/display.shape", &display_type,
		     &display_sheetobj);
  display_type->pixmap = display_xpm;
  display_sheetobj->pixmap = display_xpm;
  object_register_type(display_type);

  custom_object_load(SHAPE_DIR "/transaction.shape", &transaction_type,
		     &transaction_sheetobj);
  transaction_type->pixmap = transaction_xpm;
  transaction_sheetobj->pixmap = transaction_xpm;
  object_register_type(transaction_type);

  custom_object_load(SHAPE_DIR "/offpageconn.shape", &offpageconn_type,
		     &offpageconn_sheetobj);
  offpageconn_type->pixmap = offpageconn_xpm;
  offpageconn_sheetobj->pixmap = offpageconn_xpm;
  object_register_type(offpageconn_type);

  custom_object_load(SHAPE_DIR "/document.shape", &doc_type, &doc_sheetobj);
  doc_type->pixmap = document_xpm;
  doc_sheetobj->pixmap = document_xpm;
  object_register_type(doc_type);

  custom_object_load(SHAPE_DIR "/manualop.shape", &manualop_type,
		     &manualop_sheetobj);
  manualop_type->pixmap = manualop_xpm;
  manualop_sheetobj->pixmap = manualop_xpm;
  object_register_type(manualop_type);

  custom_object_load(SHAPE_DIR "/preparation.shape", &preparation_type,
		     &preparation_sheetobj);
  preparation_type->pixmap = preparation_xpm;
  preparation_sheetobj->pixmap = preparation_xpm;
  object_register_type(preparation_type);

  custom_object_load(SHAPE_DIR "/manualinput.shape", &manualinput_type,
		     &manualinput_sheetobj);
  manualinput_type->pixmap = manualinput_xpm;
  manualinput_sheetobj->pixmap = manualinput_xpm;
  object_register_type(manualinput_type);

  custom_object_load(SHAPE_DIR "/predefdproc.shape", &predefdproc_type,
		     &predefdproc_sheetobj);
  predefdproc_type->pixmap = predefdproc_xpm;
  predefdproc_sheetobj->pixmap = predefdproc_xpm;
  object_register_type(predefdproc_type);

  custom_object_load(SHAPE_DIR "/terminal.shape", &terminal_type,
		     &terminal_sheetobj);
  terminal_type->pixmap = terminal_xpm;
  terminal_sheetobj->pixmap = terminal_xpm;
  object_register_type(terminal_type);

  custom_object_load(SHAPE_DIR "/magdisk.shape", &magdisk_type,
		     &magdisk_sheetobj);
  magdisk_type->pixmap = magdisk_xpm;
  magdisk_sheetobj->pixmap = magdisk_xpm;
  object_register_type(magdisk_type);

  custom_object_load(SHAPE_DIR "/magtape.shape", &magtape_type,
		     &magtape_sheetobj);
  magtape_type->pixmap = magtape_xpm;
  magtape_sheetobj->pixmap = magtape_xpm;
  object_register_type(magtape_type);

  custom_object_load(SHAPE_DIR "/intstorage.shape", &intstorage_type,
		     &intstorage_sheetobj);
  intstorage_type->pixmap = intstorage_xpm;
  intstorage_sheetobj->pixmap = intstorage_xpm;
  object_register_type(intstorage_type);

  custom_object_load(SHAPE_DIR "/merge.shape", &merge_type,
		     &merge_sheetobj);
  merge_type->pixmap = merge_xpm;
  merge_sheetobj->pixmap = merge_xpm;
  object_register_type(merge_type);

  custom_object_load(SHAPE_DIR "/extract.shape", &extract_type,
		     &extract_sheetobj);
  extract_type->pixmap = extract_xpm;
  extract_sheetobj->pixmap = extract_xpm;
  object_register_type(extract_type);

  custom_object_load(SHAPE_DIR "/delay.shape", &delay_type,
		     &delay_sheetobj);
  delay_type->pixmap = delay_xpm;
  delay_sheetobj->pixmap = delay_xpm;
  object_register_type(delay_type);

  custom_object_load(SHAPE_DIR "/sumjunction.shape", &sumjunction_type,
		     &sumjunction_sheetobj);
  sumjunction_type->pixmap = sumjunction_xpm;
  sumjunction_sheetobj->pixmap = sumjunction_xpm;
  object_register_type(sumjunction_type);

  custom_object_load(SHAPE_DIR "/collate.shape", &collate_type,
		     &collate_sheetobj);
  collate_type->pixmap = collate_xpm;
  collate_sheetobj->pixmap = collate_xpm;
  object_register_type(collate_type);

  custom_object_load(SHAPE_DIR "/sort.shape", &sort_type,
		     &sort_sheetobj);
  sort_type->pixmap = sort_xpm;
  sort_sheetobj->pixmap = sort_xpm;
  object_register_type(sort_type);

  custom_object_load(SHAPE_DIR "/or.shape", &or_type,
		     &or_sheetobj);
  or_type->pixmap = or_xpm;
  or_sheetobj->pixmap = or_xpm;
  object_register_type(or_type);

}

void register_sheets(void) {
  Sheet *sheet;

  sheet = new_sheet(_("Flowchart"),
		    _("Objects to draw flowcharts"));
  sheet_append_sheet_obj(sheet, &box_sheetobj);
  sheet_append_sheet_obj(sheet, &pgram_sheetobj);
  sheet_append_sheet_obj(sheet, &diamond_sheetobj);
  sheet_append_sheet_obj(sheet, &ellipse_sheetobj);
  sheet_append_sheet_obj(sheet, display_sheetobj);
  sheet_append_sheet_obj(sheet, transaction_sheetobj);
  sheet_append_sheet_obj(sheet, offpageconn_sheetobj);
  sheet_append_sheet_obj(sheet, doc_sheetobj);
  sheet_append_sheet_obj(sheet, manualop_sheetobj);
  sheet_append_sheet_obj(sheet, preparation_sheetobj);
  sheet_append_sheet_obj(sheet, manualinput_sheetobj);
  sheet_append_sheet_obj(sheet, predefdproc_sheetobj);
  sheet_append_sheet_obj(sheet, terminal_sheetobj);
  sheet_append_sheet_obj(sheet, magdisk_sheetobj);
  sheet_append_sheet_obj(sheet, magtape_sheetobj);
  sheet_append_sheet_obj(sheet, intstorage_sheetobj);
  sheet_append_sheet_obj(sheet, merge_sheetobj);
  sheet_append_sheet_obj(sheet, extract_sheetobj);
  sheet_append_sheet_obj(sheet, delay_sheetobj);
  sheet_append_sheet_obj(sheet, sumjunction_sheetobj);
  sheet_append_sheet_obj(sheet, collate_sheetobj);
  sheet_append_sheet_obj(sheet, sort_sheetobj);
  sheet_append_sheet_obj(sheet, or_sheetobj);

  register_sheet(sheet);
}
