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
GType dia_line_preview_get_type (void);

#define DIA_TYPE_LINE_PREVIEW            (dia_line_preview_get_type ())
#define DIA_LINE_PREVIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), DIA_TYPE_LINE_PREVIEW, DiaLinePreview))
#define DIA_LINE_PREVIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), DIA_TYPE_LINE_PREVIEW, DiaLinePreviewClass))
#define DIA_IS_LINE_PREVIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DIA_TYPE_LINE_PREVIEW))
#define DIA_IS_LINE_PREVIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), DIA_TYPE_LINE_PREVIEW))
#define DIA_LINE_PREVIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), DIA_TYPE_LINE_PREVIEW, DiaLinePreviewClass))

typedef struct _DiaLinePreview DiaLinePreview;
typedef struct _DiaLinePreviewClass DiaLinePreviewClass;

struct _DiaLinePreview
{
  GtkMisc misc;
  LineStyle lstyle;
};

struct _DiaLinePreviewClass
{
  GtkMiscClass parent_class;
};

GtkWidget *dia_line_preview_new (LineStyle lstyle);


/* ------- Code for DiaLineChooser ---------------------- */
GType dia_line_chooser_get_type (void);

#define DIA_TYPE_LINE_CHOOSER            (dia_line_chooser_get_type ())
#define DIA_LINE_CHOOSER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), DIA_TYPE_LINE_CHOOSER, DiaLineChooser))
#define DIA_LINE_CHOOSER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), DIA_TYPE_LINE_CHOOSER, DiaLineChooserClass))
#define DIA_IS_LINE_CHOOSER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DIA_TYPE_LINE_CHOOSER))
#define DIA_IS_LINE_CHOOSER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), DIA_TYPE_LINE_CHOOSER))
#define DIA_LINE_CHOOSER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), DIA_TYPE_LINE_CHOOSER, DiaLineChooserClass))

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

void dia_line_chooser_set_line_style(DiaLineChooser *lchooser, 
				     LineStyle style,
				     real dashlength);
GtkWidget *dia_line_chooser_new  (DiaChangeLineCallback callback,
				  gpointer user_data);


#endif /* DIALINECHOOSER_H */
