/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * diarrowchooser.h -- Copyright (C) 1999 James Henstridge.
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

#ifndef _DIAARROWCHOOSER_H_
#define _DIAARROWCHOOSER_H_

#include <gtk/gtk.h>

#include "arrows.h"
#include "diatypes.h"


/* --------------- DiaArrowPreview -------------------------------- */
GType dia_arrow_preview_get_type (void);

#define DIA_TYPE_ARROW_PREVIEW            (dia_arrow_preview_get_type ())
#define DIA_ARROW_PREVIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), DIA_TYPE_ARROW_PREVIEW, DiaArrowPreview))
#define DIA_ARROW_PREVIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), DIA_TYPE_ARROW_PREVIEW, DiaArrowPreviewClass))
#define DIA_IS_ARROW_PREVIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DIA_TYPE_ARROW_PREVIEW))
#define DIA_IS_ARROW_PREVIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), DIA_TYPE_ARROW_PREVIEW))
#define DIA_ARROW_PREVIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), DIA_TYPE_ARROW_PREVIEW, DiaArrowPreviewClass))

typedef struct _DiaArrowPreview DiaArrowPreview;
typedef struct _DiaArrowPreviewClass DiaArrowPreviewClass;

struct _DiaArrowPreview
{
  GtkMisc misc;
  ArrowType atype;
  gboolean left;
};

struct _DiaArrowPreviewClass
{
  GtkMiscClass parent_class;
};

GtkWidget *dia_arrow_preview_new (ArrowType atype, gboolean left);


/* ------- Code for DiaArrowChooser ----------------------- */
GType dia_arrow_chooser_get_type (void);

#define DIA_TYPE_ARROW_CHOOSER            (dia_arrow_chooser_get_type ())
#define DIA_ARROW_CHOOSER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), DIA_TYPE_ARROW_CHOOSER, DiaArrowChooser))
#define DIA_ARROW_CHOOSER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), DIA_TYPE_ARROW_CHOOSER, DiaArrowChooserClass))
#define DIA_IS_ARROW_CHOOSER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DIA_TYPE_ARROW_CHOOSER))
#define DIA_IS_ARROW_CHOOSER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), DIA_TYPE_ARROW_CHOOSER))
#define DIA_ARROW_CHOOSER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), DIA_TYPE_ARROW_CHOOSER, DiaArrowChooserClass))

typedef struct _DiaArrowChooser DiaArrowChooser;
typedef struct _DiaArrowChooserClass DiaArrowChooserClass;

typedef void (*DiaChangeArrowCallback) (Arrow atype, gpointer user_data);

struct _DiaArrowChooser
{
  GtkButton button;
  DiaArrowPreview *preview;
  Arrow arrow;
  gboolean left;

  DiaChangeArrowCallback callback;
  gpointer user_data;

  GtkWidget *dialog;
  DiaArrowSelector *selector;
};

struct _DiaArrowChooserClass
{
  GtkButtonClass parent_class;
};

GtkWidget *dia_arrow_chooser_new (gboolean left,
				  DiaChangeArrowCallback callback,
				  gpointer user_data);
void dia_arrow_chooser_set_arrow(DiaArrowChooser *chooser, Arrow *arrow);
ArrowType dia_arrow_chooser_get_arrow_type(DiaArrowChooser *chooser);

#endif /* _DIAARROWCHOOSER_H_ */
