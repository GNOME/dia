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
#ifndef WIDGETS_H
#define WIDGETS_H

#include <config.h>

#include <gdk/gdk.h>
#include <gtk/gtkmenu.h>
#include <gtk/gtkoptionmenu.h>
#include <gtk/gtkdrawingarea.h>
#include <gtk/gtkcolorsel.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtkfilesel.h>
#include <gtk/gtkspinbutton.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkcolorseldialog.h>

#include "diatypes.h"

#include "font.h"
#include "color.h"
#include "arrows.h"

/* DiaFontSelector: */
#define DIAFONTSELECTOR(obj)          GTK_CHECK_CAST (obj, dia_font_selector_get_type (), DiaFontSelector)
#define DIAFONTSELECTOR_CLASS(klass)  GTK_CHECK_CLASS_CAST (klass, dia_font_selector_get_type (), DiaFontSelectorClass)
#define IS_DIAFONTSELECTOR(obj)       GTK_CHECK_TYPE (obj, dia_font_selector_get_type ())


guint      dia_font_selector_get_type        (void);
GtkWidget* dia_font_selector_new             (void);
void       dia_font_selector_set_font        (DiaFontSelector *fs, DiaFont *font);
void       dia_font_selector_set_preview     (DiaFontSelector *fs, gchar *text);
DiaFont *     dia_font_selector_get_font        (DiaFontSelector *fs);

/* DiaAlignmentSelector: */
#define DIAALIGNMENTSELECTOR(obj)          GTK_CHECK_CAST (obj, dia_alignment_selector_get_type (), DiaAlignmentSelector)
#define DIAALIGNMENTSELECTOR_CLASS(klass)  GTK_CHECK_CLASS_CAST (klass, dia_alignment_selector_get_type (), DiaAlignmentSelectorClass)
#define IS_DIAALIGNMENTSELECTOR(obj)       GTK_CHECK_TYPE (obj, dia_alignment_selector_get_type ())


guint      dia_alignment_selector_get_type      (void);
GtkWidget* dia_alignment_selector_new           (void);
Alignment  dia_alignment_selector_get_alignment (DiaAlignmentSelector *as);
void       dia_alignment_selector_set_alignment (DiaAlignmentSelector *as,
						 Alignment align);

/* DiaLineStyleSelector: */
#define DIALINESTYLESELECTOR(obj)          GTK_CHECK_CAST (obj, dia_line_style_selector_get_type (), DiaLineStyleSelector)
#define DIALINESTYLESELECTOR_CLASS(klass)  GTK_CHECK_CLASS_CAST (klass, dia_line_style_selector_get_type (), DiaLineStyleSelectorClass)
#define IS_DIALINESTYLESELECTOR(obj)       GTK_CHECK_TYPE (obj, dia_line_style_selector_get_type ())

#define DEFAULT_LINESTYLE LINESTYLE_SOLID
#define DEFAULT_LINESTYLE_DASHLEN 1.0


guint      dia_line_style_selector_get_type      (void);
GtkWidget* dia_line_style_selector_new           (void);
void       dia_line_style_selector_get_linestyle (DiaLineStyleSelector *as,
						  LineStyle *linestyle, 
						  real *dashlength);
void       dia_line_style_selector_set_linestyle (DiaLineStyleSelector *as,
						  LineStyle linestyle,
						  real dashlength);

/* DiaColorSelector: */
#define DIACOLORSELECTOR(obj)          GTK_CHECK_CAST (obj, dia_color_selector_get_type (), DiaColorSelector)
#define DIACOLORSELECTOR_CLASS(klass)  GTK_CHECK_CLASS_CAST (klass, dia_color_selector_get_type (), DiaColorSelectorClass)
#define IS_DIACOLORSELECTOR(obj)       GTK_CHECK_TYPE (obj, dia_color_selector_get_type ())

#define DEFAULT_FG_COLOR color_black
#define DEFAULT_BG_COLOR color_white
#define DEFAULT_COLOR color_white


struct _DiaColorSelector
{
  GtkButton button;

  GtkWidget *area;
  GdkGC *gc;
  Color col;

  GtkWidget *col_sel;
  
};

struct _DiaColorSelectorClass
{
  GtkButtonClass parent_class;
};

guint      dia_color_selector_get_type  (void);
GtkWidget* dia_color_selector_new       (void);
void       dia_color_selector_get_color (DiaColorSelector *cs, Color *color);
void       dia_color_selector_set_color (DiaColorSelector *cs,
					 const Color *color);


/* DiaArrowSelector */
#define DIAARROWSELECTOR(obj)          GTK_CHECK_CAST (obj, dia_arrow_selector_get_type (), DiaArrowSelector)
#define DIAARROWSELECTOR_CLASS(klass)  GTK_CHECK_CLASS_CAST (klass, dia_arrow_selector_get_type (), DiaArrowSelectorClass)
#define IS_DIAARROWSELECTOR(obj)       GTK_CHECK_TYPE (obj, dia_arrow_selector_get_type ())

#define DEFAULT_ARROW ARROW_NONE
#define DEFAULT_ARROW_LENGTH 0.8 
#define DEFAULT_ARROW_WIDTH 0.8


guint      dia_arrow_selector_get_type      (void);
GtkWidget* dia_arrow_selector_new           (void);
Arrow      dia_arrow_selector_get_arrow     (DiaArrowSelector *as);
void       dia_arrow_selector_set_arrow     (DiaArrowSelector *as,
					     Arrow arrow);


/* DiaFileSelector: */
#define DIAFILESELECTOR(obj)          GTK_CHECK_CAST (obj, dia_file_selector_get_type (), DiaFileSelector)
#define DIAFILESELECTOR_CLASS(klass)  GTK_CHECK_CLASS_CAST (klass, dia_file_selector_get_type (), DiaFileSelectorClass)
#define IS_DIAFILESELECTOR(obj)       GTK_CHECK_TYPE (obj, dia_file_selector_get_type ())


struct _DiaFileSelector
{
  GtkHBox hbox;
  GtkEntry *entry;
  GtkButton *browse;
  GtkFileSelection *dialog;
};

struct _DiaFileSelectorClass
{
  GtkHBoxClass parent_class;
};

guint      dia_file_selector_get_type        (void);
GtkWidget* dia_file_selector_new             (void);
void       dia_file_selector_set_file        (DiaFileSelector *fs, char *file);
const gchar *dia_file_selector_get_file        (DiaFileSelector *fs);

/* Other common defaults */

#define DEFAULT_ALIGNMENT ALIGN_LEFT
/* This is defined in app/linewidth_area.c.  Aw, bummer */
#define DEFAULT_LINE_WIDTH 2*0.05

#endif /* WIDGETS_H */
