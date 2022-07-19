/* Dia -- a diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * sheets.h : a sheets and objects dialog
 * Copyright (C) 2002 M.C. Nelson
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
 *
 */

#pragma once

#include <gtk/gtk.h>

#include "../lib/sheet.h"

#include "sheets_dialog_callbacks.h"


enum {
  SO_COL_NAME,
  SO_COL_LOCATION,
  SO_COL_MOD,
  SO_N_COL,
};

/* The theory behind these structures is simple.  Sheets and SheetObjects
   are wrapped in SheetMod's and SheetObjectMod's respectively.  Any changes
   made by the user to the Sheet or SheetObject is reflected in the *Mod
   subclass.  When the user commits, the information is written back to
   the datastore and the *Mod lists discarded. */

typedef struct _SheetObjectMod SheetObjectMod;
typedef struct _SheetMod       SheetMod;

struct _SheetObjectMod
{
  SheetObject sheet_object;

  enum { SHEET_OBJECT_MOD_NONE,
         SHEET_OBJECT_MOD_NEW,
         SHEET_OBJECT_MOD_CHANGED,
         SHEET_OBJECT_MOD_DELETED } mod;

  gchar *svg_filename; /* For a new sheet_object */
};

struct _SheetMod
{
  Sheet sheet;
  Sheet *original;

  enum { SHEETMOD_MOD_NONE,
         SHEETMOD_MOD_NEW,
         SHEETMOD_MOD_CHANGED,
         SHEETMOD_MOD_DELETED } mod;
};

extern GtkWidget *sheets_dialog;
extern GtkWidget *sheets_dialog_optionmenu_menu;

SheetObjectMod *sheets_append_sheet_object_mod   (SheetObject     *so,
                                                  SheetMod        *sm);
SheetMod       *sheets_append_sheet_mods         (Sheet           *sheet);
gboolean        sheets_dialog_create             (void);
GtkWidget      *lookup_widget                    (GtkWidget       *widget,
                                                  const char      *widget_name);
void            populate_store                   (GtkListStore    *store);
void            select_sheet                     (GtkWidget       *combo,
                                                  char            *sheet_name);
