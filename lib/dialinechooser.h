/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * dialinechooser.h -- Copyright (C) 1999 James Henstridge.
 *                     Copyright (C) 2004 Hubert Figuiere
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
#ifndef DIALINECHOOSER_H
#define DIALINECHOOSER_H

#include "arrows.h"

#include <gtk/gtk.h>

/* --------------- DiaLinePreview -------------------------------- */
typedef struct _DiaLinePreview DiaLinePreview;
typedef struct _DiaLinePreviewClass DiaLinePreviewClass;


#define DIA_LINE_PREVIEW(obj) (GTK_CHECK_CAST((obj),dia_line_preview_get_type(), DiaLinePreview))
#define DIA_LINE_PREVIEW_CLASS(obj) (GTK_CHECK_CLASS_CAST((obj), dia_line_preview_get_type(), DiaLinePreviewClass))

GtkWidget *dia_line_preview_new (LineStyle lstyle);

struct _DiaLinePreview
{
  GtkMisc misc;
  LineStyle lstyle;
};
struct _DiaLinePreviewClass
{
  GtkMiscClass parent_class;
};

/* ------- Code for DiaLineChooser ---------------------- */
GtkType dia_line_chooser_get_type (void);

#define DIA_LINE_CHOOSER(obj) (GTK_CHECK_CAST((obj),dia_line_chooser_get_type(), DiaLineChooser))
#define DIA_LINE_CHOOSER_CLASS(obj) (GTK_CHECK_CLASS_CAST((obj), dia_line_chooser_get_type(), DiaLineChooserClass))

typedef struct _DiaLineChooser DiaLineChooser;
typedef struct _DiaLineChooserClass DiaLineChooserClass;

typedef void (*DiaChangeLineCallback) (LineStyle lstyle, real dash_length,
                                       gpointer user_data);

struct _DiaLineChooser
{
  GtkButton button;
  DiaLinePreview *preview;
  LineStyle lstyle;
  real dash_length;

  DiaChangeLineCallback callback;
  gpointer user_data;

  GtkWidget *dialog;
  DiaLineStyleSelector *selector;
};
struct _DiaLineChooserClass
{
  GtkButtonClass parent_class;
};


GtkWidget *dia_line_chooser_new  (DiaChangeLineCallback callback,
				  gpointer user_data);

#endif
