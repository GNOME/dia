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
#include <gtk/gtkmenuitem.h>

#include "diatypes.h"

#include "font.h"
#include "color.h"
#include "arrows.h"

/* DiaFontSelector: */
#define DIAFONTSELECTOR(obj)          GTK_CHECK_CAST (obj, dia_font_selector_get_type (), DiaFontSelector)
#define DIAFONTSELECTOR_CLASS(klass)  GTK_CHECK_CLASS_CAST (klass, dia_font_selector_get_type (), DiaFontSelectorClass)
#define IS_DIAFONTSELECTOR(obj)       GTK_CHECK_TYPE (obj, dia_font_selector_get_type ())


GtkType    dia_font_selector_get_type        (void);
GtkWidget* dia_font_selector_new             (void);
void       dia_font_selector_set_font        (DiaFontSelector *fs, DiaFont *font);
void       dia_font_selector_set_preview     (DiaFontSelector *fs, gchar *text);
DiaFont *     dia_font_selector_get_font        (DiaFontSelector *fs);

/* DiaAlignmentSelector: */
#define DIAALIGNMENTSELECTOR(obj)          GTK_CHECK_CAST (obj, dia_alignment_selector_get_type (), DiaAlignmentSelector)
#define DIAALIGNMENTSELECTOR_CLASS(klass)  GTK_CHECK_CLASS_CAST (klass, dia_alignment_selector_get_type (), DiaAlignmentSelectorClass)
#define IS_DIAALIGNMENTSELECTOR(obj)       GTK_CHECK_TYPE (obj, dia_alignment_selector_get_type ())


GtkType    dia_alignment_selector_get_type      (void);
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


GtkType    dia_line_style_selector_get_type      (void);
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

GtkType    dia_color_selector_get_type  (void);
GtkWidget* dia_color_selector_new       (void);
void       dia_color_selector_get_color (DiaColorSelector *cs, Color *color);
void       dia_color_selector_set_color (DiaColorSelector *cs,
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
#define DIAFILESELECTOR(obj)          GTK_CHECK_CAST (obj, dia_file_selector_get_type (), DiaFileSelector)
#define DIAFILESELECTOR_CLASS(klass)  GTK_CHECK_CLASS_CAST (klass, dia_file_selector_get_type (), DiaFileSelectorClass)
#define IS_DIAFILESELECTOR(obj)       GTK_CHECK_TYPE (obj, dia_file_selector_get_type ())


GtkType    dia_file_selector_get_type        (void);
GtkWidget* dia_file_selector_new             (void);
void       dia_file_selector_set_file        (DiaFileSelector *fs, char *file);
const gchar *dia_file_selector_get_file        (DiaFileSelector *fs);

/* DiaSizeSelector: */
#define DIA_SIZE_SELECTOR(obj)          GTK_CHECK_CAST (obj, dia_size_selector_get_type (), DiaSizeSelector)
#define DIA_SIZE_SELECTOR_CLASS(klass)  GTK_CHECK_CLASS_CAST (klass, dia_size_selector_get_type (), DiaSizeSelectorClass)
#define IS_DIA_SIZE_SELECTOR(obj)       GTK_CHECK_TYPE (obj, dia_size_selector_get_type ())


GtkType    dia_size_selector_get_type        (void);
GtkWidget* dia_size_selector_new             (real width, real height);
void       dia_size_selector_set_locked(DiaSizeSelector *ss, gboolean locked);
void       dia_size_selector_set_size        (DiaSizeSelector *ss, real width, real height);
gboolean dia_size_selector_get_size        (DiaSizeSelector *ss, real *width, real *height);


/* DiaUnitSpinner */
#define DIA_UNIT_SPINNER(obj) GTK_CHECK_CAST(obj, dia_unit_spinner_get_type(), DiaUnitSpinner)
#define DIA_UNIT_SPINNER_CLASS(klass) GTK_CHECK_CLASS_CAST(klass, dia_unit_spinner_get_type(), DiaUnitSpinnerClass)
#define DIA_IS_UNIT_SPINNER(obj) GTK_CHECK_TYPE(obj, dia_unit_spinner_get_type())

typedef struct _DiaUnitSpinner DiaUnitSpinner;
typedef struct _DiaUnitSpinnerClass DiaUnitSpinnerClass;

typedef enum {
  DIA_UNIT_FEET,
  DIA_UNIT_METER,
  DIA_UNIT_DECIMETER,
  DIA_UNIT_MILLIMETER,
  DIA_UNIT_POINT,
  DIA_UNIT_CENTIMETER,
  DIA_UNIT_INCH,
} DiaUnit;

struct _DiaUnitSpinner {
  GtkSpinButton parent;

  DiaUnit unit_num;
};

struct _DiaUnitSpinnerClass {
  GtkSpinButtonClass parent_class;

};

GtkType    dia_unit_spinner_get_type  (void);
GtkWidget *dia_unit_spinner_new       (GtkAdjustment *adjustment,
				       guint digits,
				       DiaUnit adj_unit);
void       dia_unit_spinner_set_value (DiaUnitSpinner *self, gfloat val);
gfloat     dia_unit_spinner_get_value (DiaUnitSpinner *self);

/* DiaDynamicMenu */

#define DIA_DYNAMIC_MENU(obj) GTK_CHECK_CAST(obj, dia_dynamic_menu_get_type(), DiaDynamicMenu)
#define DIA_DYNAMIC_MENU_CLASS(klass) GTK_CHECK_CLASS_CAST(klass, dia_dynamic_menu_get_type(), DiaDynamicMenuClass)
#define DIA_IS_DYNAMIC_MENU(obj) GTK_CHECK_TYPE(obj, dia_dynamic_menu_get_type())

typedef struct _DiaDynamicMenu DiaDynamicMenu;
typedef struct _DiaDynamicMenuClass DiaDynamicMenuClass;

/** The ways the non-default entries in the menu can be sorted:
 * DDM_SORT_TOP: Just add new ones at the top, removing them from the middle.
 * Not currently implemented.
 * DDM_SORT_NEWEST:  Add new ones to the top, and move selected ones to the
 * top as well.
 * DDM_SORT_SORT:  Sort the entries according to the CompareFunc order.
 */
typedef enum { DDM_SORT_TOP, DDM_SORT_NEWEST, DDM_SORT_SORT } DdmSortType;

typedef GtkWidget *(* DDMCreateItemFunc)(DiaDynamicMenu *, gchar *);
typedef void (* DDMCallbackFunc)(DiaDynamicMenu *, gchar *);

struct _DiaDynamicMenu {
  GtkOptionMenu parent;

  GList *default_entries;

  GCompareFunc compare_func;
  DDMCreateItemFunc create_func;

  GtkMenuItem *other_item;

  gchar *persistent_name;
  gint cols;

  /** For the list-based versions, these are the options */
  GList *options;

  /** For the string-based versions, this is the activate function */
  DDMCallbackFunc activate_func;
};

struct _DiaDynamicMenuClass {
  GtkOptionMenuClass parent_class;
};

GtkType    dia_dynamic_menu_get_type  (void);

GtkWidget *dia_dynamic_menu_new(GCompareFunc comp, DDMCreateItemFunc create,
				DDMCallbackFunc activate,
				GtkMenuItem *otheritem, gchar *persist);
GtkWidget *dia_dynamic_menu_new_stringbased(GtkMenuItem *otheritem, 
					    DDMCallbackFunc activate,
					    gchar *persist);
GtkWidget *dia_dynamic_menu_new_listbased(GCompareFunc comp, 
					  DDMCreateItemFunc create,
					  DDMCallbackFunc activate,
					  gchar *other_label,
					  GList *items, gchar *persist);
GtkWidget *dia_dynamic_menu_new_stringlistbased(gchar *other_label,
						GList *items, 
						DDMCallbackFunc activate,
						gchar *persist);
void dia_dynamic_menu_add_default_entry(DiaDynamicMenu *ddm, gchar *entry);
gint dia_dynamic_menu_add_entry(DiaDynamicMenu *ddm, gchar *entry);
void dia_dynamic_menu_set_sorting_method(DiaDynamicMenu *ddm, DdmSortType sort);
void dia_dynamic_menu_reset(DiaDynamicMenu *ddm);
void dia_dynamic_menu_set_max_entries(DiaDynamicMenu *ddm, gint max);
void dia_dynamic_menu_set_columns(DiaDynamicMenu *ddm, gint cols);


/* **** Util functions for Gtk stuff **** */
/** Gets the image name 'filename' out of the shared data dir 
 * To see how to get the image installed, check app/Makefile.am
 */
GtkWidget *dia_get_image_from_file(gchar *filename);

/** Create a toggle button with two different images, one for when on and
 * one for when off.  Images are taken from standard image directory.
 */
GtkWidget *dia_toggle_button_new_with_images(gchar *on_file, gchar *off_file);


/* Other common defaults */

#define DEFAULT_ALIGNMENT ALIGN_LEFT
/* This is defined in app/linewidth_area.c.  Aw, bummer */
#define DEFAULT_LINE_WIDTH 2*0.05

#endif /* WIDGETS_H */
