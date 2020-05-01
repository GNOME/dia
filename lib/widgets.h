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

#include <gdk/gdk.h>
#include <gtk/gtk.h>

#include "diatypes.h"
#include "font.h"
#include "color.h"
#include "arrows.h"
#include "units.h"
#include "dia-autoptr.h"

/* DiaAlignmentSelector: */
GtkWidget* dia_alignment_selector_new           (void);
Alignment  dia_alignment_selector_get_alignment (GtkWidget *as);
void       dia_alignment_selector_set_alignment (GtkWidget *as, Alignment align);

/* DiaLineStyleSelector: */
#define DIALINESTYLESELECTOR(obj)          G_TYPE_CHECK_INSTANCE_CAST (obj, dia_line_style_selector_get_type (), DiaLineStyleSelector)
#define DIALINESTYLESELECTOR_CLASS(klass)  G_TYPE_CHECK_CLASS_CAST (klass, dia_line_style_selector_get_type (), DiaLineStyleSelectorClass)
#define IS_DIALINESTYLESELECTOR(obj)       G_TYPE_CHECK_INSTANCE_TYPE (obj, dia_line_style_selector_get_type ())

GType      dia_line_style_selector_get_type      (void);
GtkWidget* dia_line_style_selector_new           (void);
void       dia_line_style_selector_get_linestyle (DiaLineStyleSelector *as,
						  LineStyle *linestyle,
						  real *dashlength);
void       dia_line_style_selector_set_linestyle (DiaLineStyleSelector *as,
						  LineStyle linestyle,
						  real dashlength);

/* DiaColorSelector: */
#define DIACOLORSELECTOR(obj)          G_TYPE_CHECK_INSTANCE_CAST (obj, dia_color_selector_get_type (), DiaColorSelector)
#define DIACOLORSELECTOR_CLASS(klass)  G_TYPE_CHECK_CLASS_CAST (klass, dia_color_selector_get_type (), DiaColorSelectorClass)
#define IS_DIACOLORSELECTOR(obj)       G_TYPE_CHECK_INSTANCE_TYPE (obj, dia_color_selector_get_type ())

GType      dia_color_selector_get_type  (void);
GtkWidget* dia_color_selector_new       (void);
void       dia_color_selector_set_use_alpha (GtkWidget *cs, gboolean use_alpha);
void       dia_color_selector_get_color (GtkWidget *cs, Color *color);
void       dia_color_selector_set_color (GtkWidget *cs,
					 const Color *color);


/* DiaArrowSelector */
#define DIA_TYPE_ARROW_SELECTOR           (dia_arrow_selector_get_type())
#define DIA_ARROW_SELECTOR(obj)           (G_TYPE_CHECK_INSTANCE_CAST (obj, dia_arrow_selector_get_type (), DiaArrowSelector))
#define DIA_ARROW_SELECTOR_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST (klass, dia_arrow_selector_get_type (), DiaArrowSelectorClass))
#define DIA_IS_ARROW_SELECTOR(obj)        (G_TYPE_CHECK_TYPE (obj, dia_arrow_selector_get_type ()))
#define DIA_ARROW_SELECTOR_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), DIA_TYPE_ARROW_SELECTOR, DiaArrowSelectorClass))

#define DEFAULT_ARROW ARROW_NONE
#define DEFAULT_ARROW_LENGTH DEFAULT_ARROW_SIZE
#define DEFAULT_ARROW_WIDTH DEFAULT_ARROW_SIZE


GType    dia_arrow_selector_get_type        (void);
GtkWidget* dia_arrow_selector_new           (void);
Arrow      dia_arrow_selector_get_arrow     (DiaArrowSelector *as);
void       dia_arrow_selector_set_arrow     (DiaArrowSelector *as,
					     Arrow arrow);


/* DiaFileSelector: */
#define DIAFILESELECTOR(obj)          G_TYPE_CHECK_INSTANCE_CAST (obj, dia_file_selector_get_type (), DiaFileSelector)
#define DIAFILESELECTOR_CLASS(klass)  G_TYPE_CHECK_CLASS_CAST (klass, dia_file_selector_get_type (), DiaFileSelectorClass)
#define IS_DIAFILESELECTOR(obj)       G_TYPE_CHECK_INSTANCE_TYPE (obj, dia_file_selector_get_type ())


GType      dia_file_selector_get_type        (void);
GtkWidget* dia_file_selector_new             (void);
void       dia_file_selector_set_extensions  (DiaFileSelector *fs, const gchar **exts);
void       dia_file_selector_set_file        (DiaFileSelector *fs, char *file);
const gchar *dia_file_selector_get_file        (DiaFileSelector *fs);

/* DiaSizeSelector: */
#define DIA_SIZE_SELECTOR(obj)          G_TYPE_CHECK_INSTANCE_CAST (obj, dia_size_selector_get_type (), DiaSizeSelector)
#define DIA_SIZE_SELECTOR_CLASS(klass)  G_TYPE_CHECK_CLASS_CAST (klass, dia_size_selector_get_type (), DiaSizeSelectorClass)
#define IS_DIA_SIZE_SELECTOR(obj)       G_TYPE_CHECK_INSTANCE_TYPE (obj, dia_size_selector_get_type ())


GType      dia_size_selector_get_type        (void);
GtkWidget* dia_size_selector_new             (real width, real height);
void       dia_size_selector_set_locked(DiaSizeSelector *ss, gboolean locked);
void       dia_size_selector_set_size        (DiaSizeSelector *ss, real width, real height);
gboolean   dia_size_selector_get_size        (DiaSizeSelector *ss, real *width, real *height);


/* DiaUnitSpinner */

struct _DiaUnitSpinner {
  GtkSpinButton parent;

  DiaUnit unit_num;
};


#define DIA_TYPE_UNIT_SPINNER dia_unit_spinner_get_type ()
G_DECLARE_FINAL_TYPE (DiaUnitSpinner, dia_unit_spinner, DIA, UNIT_SPINNER, GtkSpinButton)


GtkWidget *dia_unit_spinner_new       (GtkAdjustment  *adjustment,
                                       DiaUnit         adj_unit);
void       dia_unit_spinner_set_value (DiaUnitSpinner *self,
                                       double          val);
double     dia_unit_spinner_get_value (DiaUnitSpinner *self);
void       dia_unit_spinner_set_upper (DiaUnitSpinner *self,
                                       double          val);

/* **** Util functions for Gtk stuff **** */

GtkWidget *dia_toggle_button_new_with_icon_names (const gchar *on,
                                                  const gchar *off);

GdkPixbuf *pixbuf_from_resource (const gchar *path);

/* Other common defaults */

#define DEFAULT_ALIGNMENT ALIGN_LEFT
/* This is defined in app/linewidth_area.c.  Aw, bummer */
#define DEFAULT_LINE_WIDTH 2*0.05

#endif /* WIDGETS_H */
