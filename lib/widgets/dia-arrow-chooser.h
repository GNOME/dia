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

#define DIA_TYPE_ARROW_PREVIEW (dia_arrow_preview_get_type ())
G_DECLARE_FINAL_TYPE (DiaArrowPreview, dia_arrow_preview, DIA, ARROW_PREVIEW, GtkWidget)

struct _DiaArrowPreview
{
  GtkWidget misc;
  ArrowType atype;
  gboolean left;
};

GtkWidget *dia_arrow_preview_new (ArrowType atype, gboolean left);


/* ------- Code for DiaArrowChooser ----------------------- */

#define DIA_TYPE_ARROW_CHOOSER_POPOVER (dia_arrow_chooser_popover_get_type ())
G_DECLARE_DERIVABLE_TYPE (DiaArrowChooserPopover, dia_arrow_chooser_popover, DIA, ARROW_CHOOSER_POPOVER, GtkPopover)

struct _DiaArrowChooserPopoverClass {
  GtkPopoverClass parent_class;
};

GtkWidget *dia_arrow_chooser_popover_new                  (gboolean                left);
void       dia_arrow_chooser_popover_set_arrow            (DiaArrowChooserPopover *chooser,
                                                           Arrow                  *arrow);
Arrow      dia_arrow_chooser_popover_get_arrow            (DiaArrowChooserPopover *chooser);


#define DIA_TYPE_ARROW_CHOOSER (dia_arrow_chooser_get_type ())
G_DECLARE_DERIVABLE_TYPE (DiaArrowChooser, dia_arrow_chooser, DIA, ARROW_CHOOSER, GtkMenuButton)

struct _DiaArrowChooserClass
{
  GtkMenuButtonClass parent_class;
};

GtkWidget *dia_arrow_chooser_new            (gboolean         left);
void       dia_arrow_chooser_set_arrow      (DiaArrowChooser *chooser,
                                             Arrow           *arrow);
Arrow      dia_arrow_chooser_get_arrow      (DiaArrowChooser *chooser);
ArrowType  dia_arrow_chooser_get_arrow_type (DiaArrowChooser *chooser);

#endif /* _DIAARROWCHOOSER_H_ */
