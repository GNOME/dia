#ifndef WIDGETS_H
#define WIDGETS_H

#include <gdk/gdk.h>
#include <gtk/gtkmenu.h>
#include <gtk/gtkoptionmenu.h>
#include <gtk/gtkdrawingarea.h>
#include <gtk/gtkcolorsel.h>

#include "font.h"
#include "render.h"
#include "color.h"

/* DiaFontSelector: */
#define DIAFONTSELECTOR(obj)          GTK_CHECK_CAST (obj, dia_font_selector_get_type (), DiaFontSelector)
#define DIAFONTSELECTOR_CLASS(klass)  GTK_CHECK_CLASS_CAST (klass, dia_font_selector_get_type (), DiaFontSelectorClass)
#define IS_DIAFONTSELECTOR(obj)       GTK_CHECK_TYPE (obj, dia_font_selector_get_type ())

typedef struct _DiaFontSelector       DiaFontSelector;
typedef struct _DiaFontSelectorClass  DiaFontSelectorClass;

struct _DiaFontSelector
{
  GtkOptionMenu omenu;

  GtkMenu *font_menu;
};

struct _DiaFontSelectorClass
{
  GtkOptionMenuClass parent_class;
};

guint      dia_font_selector_get_type        (void);
GtkWidget* dia_font_selector_new             (void);
void       dia_font_selector_set_font        (DiaFontSelector *fs, Font *font);
Font *     dia_font_selector_get_font        (DiaFontSelector *fs);

/* DiaAlignmentSelector: */
#define DIAALIGNMENTSELECTOR(obj)          GTK_CHECK_CAST (obj, dia_alignment_selector_get_type (), DiaAlignmentSelector)
#define DIAALIGNMENTSELECTOR_CLASS(klass)  GTK_CHECK_CLASS_CAST (klass, dia_alignment_selector_get_type (), DiaAlignmentSelectorClass)
#define IS_DIAALIGNMENTSELECTOR(obj)       GTK_CHECK_TYPE (obj, dia_alignment_selector_get_type ())

typedef struct _DiaAlignmentSelector       DiaAlignmentSelector;
typedef struct _DiaAlignmentSelectorClass  DiaAlignmentSelectorClass;

struct _DiaAlignmentSelector
{
  GtkOptionMenu omenu;

  GtkMenu *alignment_menu;
};

struct _DiaAlignmentSelectorClass
{
  GtkOptionMenuClass parent_class;
};

guint      dia_alignment_selector_get_type      (void);
GtkWidget* dia_alignment_selector_new           (void);
Alignment  dia_alignment_selector_get_alignment (DiaAlignmentSelector *as);
void       dia_alignment_selector_set_alignment (DiaAlignmentSelector *as,
						 Alignment align);

/* DiaLineStyleSelector: */
#define DIALINESTYLESELECTOR(obj)          GTK_CHECK_CAST (obj, dia_line_style_selector_get_type (), DiaLineStyleSelector)
#define DIALINESTYLESELECTOR_CLASS(klass)  GTK_CHECK_CLASS_CAST (klass, dia_line_style_selector_get_type (), DiaLineStyleSelectorClass)
#define IS_DIALINESTYLESELECTOR(obj)       GTK_CHECK_TYPE (obj, dia_line_style_selector_get_type ())

typedef struct _DiaLineStyleSelector       DiaLineStyleSelector;
typedef struct _DiaLineStyleSelectorClass  DiaLineStyleSelectorClass;

struct _DiaLineStyleSelector
{
  GtkOptionMenu omenu;

  GtkMenu *linestyle_menu;
};

struct _DiaLineStyleSelectorClass
{
  GtkOptionMenuClass parent_class;
};

guint      dia_line_style_selector_get_type      (void);
GtkWidget* dia_line_style_selector_new           (void);
LineStyle  dia_line_style_selector_get_linestyle (DiaLineStyleSelector *as);
void       dia_line_style_selector_set_linestyle (DiaLineStyleSelector *as,
						  LineStyle linestyle);

/* DiaColorSelector: */
#define DIACOLORSELECTOR(obj)          GTK_CHECK_CAST (obj, dia_color_selector_get_type (), DiaColorSelector)
#define DIACOLORSELECTOR_CLASS(klass)  GTK_CHECK_CLASS_CAST (klass, dia_color_selector_get_type (), DiaColorSelectorClass)
#define IS_DIACOLORSELECTOR(obj)       GTK_CHECK_TYPE (obj, dia_color_selector_get_type ())

typedef struct _DiaColorSelector       DiaColorSelector;
typedef struct _DiaColorSelectorClass  DiaColorSelectorClass;

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
					 Color *color);


#endif /* WIDGETS_H */







