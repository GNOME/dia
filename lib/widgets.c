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

#include <config.h>
#include <string.h>
#include "intl.h"
#undef GTK_DISABLE_DEPRECATED /* GtkOptionMenu, ... */
#include "widgets.h"
#include "message.h"
#include "dia_dirs.h"
#include "arrows.h"
#include "diaarrowchooser.h"
#include "dialinechooser.h"
#include "persistence.h"

#include <stdlib.h>
#include <glib.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <pango/pango.h>
#include <stdio.h>
#include <time.h>
#include "gdk/gdkkeysyms.h"

#include "diagtkfontsel.h"

/************* DiaSizeSelector: ***************/
/* A widget that selects two sizes, width and height, optionally keeping
 * aspect ration.
 */
struct _DiaSizeSelector
{
  GtkHBox hbox;
  GtkSpinButton *width, *height;
  GtkToggleButton *aspect_locked;
  real ratio;
  GtkAdjustment *last_adjusted;
  GtkWidget *unbroken_link, *broken_link;
};

struct _DiaSizeSelectorClass
{
  GtkHBoxClass parent_class;
};

static void
dia_size_selector_unrealize(GtkWidget *widget)
{
  (* GTK_WIDGET_CLASS (gtk_type_class(gtk_hbox_get_type ()))->unrealize) (widget);
}

static void
dia_size_selector_class_init (DiaSizeSelectorClass *class)
{
  GtkObjectClass *object_class;
  GtkWidgetClass *widget_class;
  
  object_class = (GtkObjectClass*) class;
  widget_class = (GtkWidgetClass*) class;
  widget_class->unrealize = dia_size_selector_unrealize;
}

static void
dia_size_selector_adjust_width(DiaSizeSelector *ss)
{
  real height = 
    gtk_spin_button_get_value_as_float(GTK_SPIN_BUTTON(ss->height));
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(ss->width), height*ss->ratio);
}

static void
dia_size_selector_adjust_height(DiaSizeSelector *ss)
{
  real width = 
    gtk_spin_button_get_value_as_float(GTK_SPIN_BUTTON(ss->width));
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(ss->height), width/ss->ratio);
}

static void
dia_size_selector_ratio_callback(GtkAdjustment *limits, gpointer userdata) 
{
  static gboolean in_progress;
  DiaSizeSelector *ss = DIA_SIZE_SELECTOR(userdata);

  ss->last_adjusted = limits;

  if (!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ss->aspect_locked)))
    return;

  if (in_progress) return;
  in_progress = TRUE;

  if (limits == gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(ss->width))) {
    dia_size_selector_adjust_height(ss);
  } else {
    dia_size_selector_adjust_width(ss);
  }

  in_progress = FALSE;
}

static void
dia_size_selector_destroy_callback(GtkWidget *widget) 
{
  DiaSizeSelector *ss = DIA_SIZE_SELECTOR(widget);

  g_object_unref(ss->broken_link);
  g_object_unref(ss->unbroken_link);
}

static void
dia_size_selector_lock_pressed(GtkWidget *widget, gpointer data)
{
  DiaSizeSelector *ss = DIA_SIZE_SELECTOR(data);

  if (gtk_bin_get_child(GTK_BIN(ss->aspect_locked)))
    gtk_container_remove(GTK_CONTAINER(ss->aspect_locked), 
    gtk_bin_get_child(GTK_BIN(ss->aspect_locked)));

  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ss->aspect_locked))) {
    gtk_container_add(GTK_CONTAINER(ss->aspect_locked), ss->unbroken_link);
    dia_size_selector_ratio_callback(ss->last_adjusted, (gpointer)ss);
  } else {
    gtk_container_add(GTK_CONTAINER(ss->aspect_locked), ss->broken_link);
  }
}

/* Possible args:  Init width, init height, digits */

static void
dia_size_selector_init (DiaSizeSelector *ss)
{
  /* Here's where we set up the real thing */
  GtkAdjustment *adj;
  adj = GTK_ADJUSTMENT(gtk_adjustment_new(1.0, 0, 10,
					  0.1, 1.0, 1.0));
  ss->width = GTK_SPIN_BUTTON(gtk_spin_button_new(adj, 1.0, 2));
  gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(ss->width), TRUE);
  gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(ss->width), TRUE);
  gtk_box_pack_start(GTK_BOX(ss), GTK_WIDGET(ss->width), FALSE, TRUE, 0);
  gtk_widget_show(GTK_WIDGET(ss->width));

  adj = GTK_ADJUSTMENT(gtk_adjustment_new(1.0, 0, 10,
					  0.1, 1.0, 1.0));
  ss->height = GTK_SPIN_BUTTON(gtk_spin_button_new(adj, 1.0, 2));
  gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(ss->height), TRUE);
  gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(ss->height), TRUE);
  gtk_box_pack_start(GTK_BOX(ss), GTK_WIDGET(ss->height), FALSE, TRUE, 0);
  gtk_widget_show(GTK_WIDGET(ss->height));

  /* Replace label with images */
  /* should make sure they're both unallocated when the widget dies. 
  * That should happen in the "destroy" handler, where both should
  * be unref'd */
  ss->broken_link = dia_get_image_from_file("broken-chain.xpm");
  g_object_ref(ss->broken_link);
  gtk_misc_set_padding(GTK_MISC(ss->broken_link), 0, 0);
  gtk_widget_show(ss->broken_link);
  ss->unbroken_link = dia_get_image_from_file("unbroken-chain.xpm");
  g_object_ref(ss->unbroken_link);
  gtk_misc_set_padding(GTK_MISC(ss->unbroken_link), 0, 0);
  gtk_widget_show(ss->unbroken_link);

  ss->aspect_locked = GTK_TOGGLE_BUTTON(gtk_toggle_button_new());
  gtk_container_add(GTK_CONTAINER(ss->aspect_locked), ss->unbroken_link);
  gtk_container_set_border_width(GTK_CONTAINER(ss->aspect_locked), 0);

  gtk_box_pack_start(GTK_BOX(ss), GTK_WIDGET(ss->aspect_locked), FALSE, TRUE, 0); 
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ss->aspect_locked), TRUE);
  gtk_widget_show(GTK_WIDGET(ss->aspect_locked));

  gtk_signal_connect (GTK_OBJECT (ss->aspect_locked), "clicked",
                      (GtkSignalFunc) dia_size_selector_lock_pressed,
                      ss);
  /* Make sure that the aspect ratio stays the same */
  g_signal_connect(GTK_OBJECT(gtk_spin_button_get_adjustment(ss->width)), 
		   "value_changed",
		   G_CALLBACK(dia_size_selector_ratio_callback), (gpointer)ss);
  g_signal_connect(GTK_OBJECT(gtk_spin_button_get_adjustment(ss->height)), 
		   "value_changed",
		   G_CALLBACK(dia_size_selector_ratio_callback), (gpointer)ss);
  g_signal_connect(GTK_OBJECT(ss), "destroy",
		   G_CALLBACK(dia_size_selector_destroy_callback), NULL);
}

GtkType
dia_size_selector_get_type (void)
{
  static GtkType dss_type = 0;

  if (!dss_type) {
    GtkTypeInfo dss_info = {
      "DiaSizeSelector",
      sizeof (DiaSizeSelector),
      sizeof (DiaSizeSelectorClass),
      (GtkClassInitFunc) dia_size_selector_class_init,
      (GtkObjectInitFunc) dia_size_selector_init,
      NULL,
      NULL,
      (GtkClassInitFunc) NULL,
    };
    
    dss_type = gtk_type_unique (gtk_hbox_get_type (), &dss_info);

  }
  
  return dss_type;
}

GtkWidget *
dia_size_selector_new (real width, real height)
{
  GtkWidget *wid;

  wid = GTK_WIDGET ( gtk_type_new (dia_size_selector_get_type ()));
  dia_size_selector_set_size(DIA_SIZE_SELECTOR(wid), width, height);
  return wid;
}

void
dia_size_selector_set_size(DiaSizeSelector *ss, real width, real height) 
{
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(ss->width), width);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(ss->height), height);
  if (height != 0.0) 
    ss->ratio = width/height;
}

void
dia_size_selector_set_locked(DiaSizeSelector *ss, gboolean locked)
{
  if (!ss->aspect_locked && locked) {
    dia_size_selector_adjust_height(ss);
  }
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ss->aspect_locked), locked);
}

gboolean
dia_size_selector_get_size(DiaSizeSelector *ss, real *width, real *height)
{
  *width = gtk_spin_button_get_value_as_float(GTK_SPIN_BUTTON(ss->width));
  *height = gtk_spin_button_get_value_as_float(GTK_SPIN_BUTTON(ss->height));
  return gtk_toggle_button_get_active(ss->aspect_locked);
}

/************* DiaFontSelector: ***************/

struct _DiaFontSelector
{
  GtkHBox hbox;

  GtkOptionMenu *font_omenu;
  GtkOptionMenu *style_omenu;
  GtkMenu *style_menu;
};

struct _DiaFontSelectorClass
{
  GtkHBoxClass parent_class;
};


/* New and improved font selector:  Contains the three standard fonts
 * and an 'Other fonts...' entry that opens the font dialog.  The fonts
 * selected in the font dialog are persistently added to the menu.
 *
 * +----------------+
 * | Sans           |
 * | Serif          |
 * | Monospace      |
 * | -------------- |
 * | Bodini         |
 * | CurlyGothic    |
 * | OldWestern     |
 * | -------------- |
 * | Other fonts... |
 * +----------------+
 */

static void dia_font_selector_fontmenu_callback(DiaDynamicMenu *button,
						gchar *fontname,
						gpointer data);
static void dia_font_selector_set_styles(DiaFontSelector *fs,
					 gchar *name,
					 DiaFontStyle dia_style);
static void dia_font_selector_set_style_menu(DiaFontSelector *fs,
					     PangoFontFamily *pff,
					     DiaFontStyle dia_style);

static void
dia_font_selector_class_init (DiaFontSelectorClass *class)
{
  GtkObjectClass *object_class;
  
  object_class = (GtkObjectClass*) class;
}

static int
dia_font_selector_sort_fonts(const void **p1, const void **p2)
{
  const gchar *n1 = pango_font_family_get_name(PANGO_FONT_FAMILY(*p1));
  const gchar *n2 = pango_font_family_get_name(PANGO_FONT_FAMILY(*p2));
  return g_strcasecmp(n1, n2);
}

static GtkWidget *
dia_font_selector_create_string_item(DiaDynamicMenu *ddm, gchar *string)
{
  GtkWidget *item = gtk_menu_item_new_with_label(string);
  gtk_label_set_markup(GTK_LABEL(gtk_bin_get_child(GTK_BIN(item))),
		       g_strdup_printf("<span face=\"%s\" size=\"medium\">%s</span>",
				       string, string));
  return item;
}

static void
dia_font_selector_init (DiaFontSelector *fs)
{
  GtkWidget *menu;
  GtkWidget *omenu;

  PangoFontFamily **families;
  int n_families,i;
  GList *fontnames = NULL;
    
  pango_context_list_families (dia_font_get_context(),
			       &families, &n_families);
  qsort(families, n_families, sizeof(PangoFontFamily*),
	dia_font_selector_sort_fonts);
  /* Doing it the slow way until I find a better way */
  for (i = 0; i < n_families; i++) {
    fontnames = g_list_append(fontnames, 
			      pango_font_family_get_name(families[i]));
  }

  fs->font_omenu = 
    GTK_OPTION_MENU
    (dia_dynamic_menu_new_listbased(dia_font_selector_create_string_item,
				    dia_font_selector_fontmenu_callback,
				    fs,
				    _("Other fonts"),
				    fontnames,
				    "font-menu"));
  dia_dynamic_menu_add_default_entry(DIA_DYNAMIC_MENU(fs->font_omenu),
				     "sans");
  dia_dynamic_menu_add_default_entry(DIA_DYNAMIC_MENU(fs->font_omenu),
				     "serif");
  dia_dynamic_menu_add_default_entry(DIA_DYNAMIC_MENU(fs->font_omenu),
				     "monospace");
  gtk_widget_show(GTK_WIDGET(fs->font_omenu));

  /* Now build the style menu button */
  omenu = gtk_option_menu_new();
  fs->style_omenu = GTK_OPTION_MENU(omenu);
  menu = gtk_menu_new ();
  fs->style_menu = GTK_MENU(menu);
  gtk_option_menu_set_menu (GTK_OPTION_MENU (fs->style_omenu), menu);

  gtk_widget_show(menu);
  gtk_widget_show(omenu);

  gtk_box_pack_start_defaults(GTK_BOX(fs), GTK_WIDGET(fs->font_omenu));
  gtk_box_pack_start_defaults(GTK_BOX(fs), GTK_WIDGET(fs->style_omenu));
}

GtkType
dia_font_selector_get_type        (void)
{
  static GtkType dfs_type = 0;

  if (!dfs_type) {
    GtkTypeInfo dfs_info = {
      "DiaFontSelector",
      sizeof (DiaFontSelector),
      sizeof (DiaFontSelectorClass),
      (GtkClassInitFunc) dia_font_selector_class_init,
      (GtkObjectInitFunc) dia_font_selector_init,
      NULL,
      NULL,
      (GtkClassInitFunc) NULL
    };
    
    dfs_type = gtk_type_unique (gtk_hbox_get_type (), &dfs_info);
  }
  
  return dfs_type;
}

GtkWidget *
dia_font_selector_new ()
{
  return GTK_WIDGET ( gtk_type_new (dia_font_selector_get_type ()));
}

static PangoFontFamily *
dia_font_selector_get_family_from_name(GtkWidget *widget, const gchar *fontname)
{
  PangoFontFamily **families;
  int n_families,i;
    
  pango_context_list_families (dia_font_get_context(),
			       &families, &n_families);
  /* Doing it the slow way until I find a better way */
  for (i = 0; i < n_families; i++) {
    if (!(g_strcasecmp(pango_font_family_get_name(families[i]), fontname))) {
      PangoFontFamily *fam = families[i];
      g_free(families);
      return fam;
    }
  }
  g_warning(_("Couldn't find font family for %s\n"), fontname);
  g_free(families);
  return NULL;
}

static void
dia_font_selector_fontmenu_callback(DiaDynamicMenu *ddm, gchar *fontname, gpointer data) 
{
  DiaFontSelector *fs = DIAFONTSELECTOR(data);
  dia_font_selector_set_styles(fs, fontname, -1);
}

static char *style_labels[] = {
  "Normal",
  "Oblique",
  "Italic",
  "Ultralight",
  "Ultralight-Oblique",
  "Ultralight-Italic",
  "Light",
  "Light-Oblique",
  "Light-Italic",
  "Medium",
  "Medium-Oblique",
  "Medium-Italic",
  "Demibold",
  "Demibold-Oblique",
  "Demibold-Italic",
  "Bold",
  "Bold-Oblique",
  "Bold-Italic",
  "Ultrabold",
  "Ultrabold-Oblique",
  "Ultrabold-Italic",
  "Heavy",
  "Heavy-Oblique",
  "Heavy-Italic"
};

static void
dia_font_selector_set_style_menu(DiaFontSelector *fs,
				 PangoFontFamily *pff,
				 DiaFontStyle dia_style)
{
  int i=0, select = 0;
  PangoFontFace **faces = NULL;
  int nfaces = 0;
  GtkWidget *menu = NULL;
  long stylebits = 0;
  int menu_item_nr = 0;
  GSList *group = NULL;

  menu = gtk_menu_new ();
  pango_font_family_list_faces(pff, &faces, &nfaces);

  for (i = 0; i < nfaces; i++) {
    PangoFontDescription *pfd = pango_font_face_describe(faces[i]);
    PangoStyle style = pango_font_description_get_style(pfd);
    PangoWeight weight = pango_font_description_get_weight(pfd);
    /**
     * This is a quick and dirty way to pick the styles present,
     * sort them and avoid duplicates.  
     * We set a bit for each style present, bit (weight*3+style)
     * From style_labels, we pick #(weight*3+style)
     * where weight and style are the Dia types.  
     */
    /* Account for DIA_WEIGHT_NORMAL hack */
    int weightnr = (weight-200)/100;
    if (weightnr < 2) weightnr ++;
    else if (weightnr == 2) weightnr = 0;
    stylebits |= 1 << (3*weightnr + style);
    pango_font_description_free(pfd);
  }

  g_free(faces);

  for (i = DIA_FONT_NORMAL; i <= (DIA_FONT_HEAVY | DIA_FONT_ITALIC); i+=4) {
    GtkWidget *menuitem;
    /**
     * bad hack continued ...
     */
    int weight = DIA_FONT_STYLE_GET_WEIGHT(i) >> 4;
    int slant = DIA_FONT_STYLE_GET_SLANT(i) >> 2;
    if (DIA_FONT_STYLE_GET_SLANT(i) > DIA_FONT_ITALIC) continue;
    if (!(stylebits & (1 << (3*weight + slant)))) continue;
    menuitem = gtk_radio_menu_item_new_with_label (group, style_labels[3*weight+slant]);
    group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(menuitem));
    gtk_object_set_user_data(GTK_OBJECT(menuitem), GINT_TO_POINTER(i));
    if (dia_style == i) {
      select = menu_item_nr;
    }
    menu_item_nr++;
    gtk_menu_append (GTK_MENU (menu), menuitem);
    gtk_widget_show (menuitem);
  }
  gtk_widget_show(menu);
  gtk_option_menu_remove_menu(fs->style_omenu);
  gtk_option_menu_set_menu(fs->style_omenu, menu);
  fs->style_menu = GTK_MENU(menu);
  gtk_option_menu_set_history(GTK_OPTION_MENU(fs->style_omenu), select);
  gtk_menu_set_active(fs->style_menu, select);
  gtk_widget_set_sensitive(GTK_WIDGET(fs->style_omenu), menu_item_nr > 1);
  gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_menu_get_active(fs->style_menu)), TRUE);
}

static void
dia_font_selector_set_styles(DiaFontSelector *fs,
			     gchar *name, DiaFontStyle dia_style)
{
  PangoFontFamily *pff;
  pff = dia_font_selector_get_family_from_name(GTK_WIDGET(fs), name);
  dia_font_selector_set_style_menu(fs, pff, dia_style);
} 


/* API functions */
/** Set a string to be used for preview in the GTK font selector dialog.
 * The start of this string will be copied.
 * This function is now obsolete.
 */
void
dia_font_selector_set_preview(DiaFontSelector *fs, gchar *text) {
}

/** Set the current font to be shown in the font selector.
 */
void
dia_font_selector_set_font(DiaFontSelector *fs, DiaFont *font)
{
  gchar *fontname = dia_font_get_family(font);
  dia_dynamic_menu_add_entry(DIA_DYNAMIC_MENU(fs->font_omenu), fontname);
}

DiaFont *
dia_font_selector_get_font(DiaFontSelector *fs)
{
  GtkWidget *menuitem;
  char *fontname;
  DiaFontStyle style;
  fontname = dia_dynamic_menu_get_entry(DIA_DYNAMIC_MENU(fs->font_omenu));
  menuitem = gtk_menu_get_active(fs->style_menu);
  style = GPOINTER_TO_INT(gtk_object_get_user_data(GTK_OBJECT(menuitem)));
  return dia_font_new(fontname,style,1.0);
}

/************* DiaAlignmentSelector: ***************/
struct _DiaAlignmentSelector
{
  GtkOptionMenu omenu;

  GtkMenu *alignment_menu;
};

struct _DiaAlignmentSelectorClass
{
  GtkOptionMenuClass parent_class;
};

static void
dia_alignment_selector_class_init (DiaAlignmentSelectorClass *class)
{
  GtkObjectClass *object_class;
  
  object_class = (GtkObjectClass*) class;
}

static void
dia_alignment_selector_init (DiaAlignmentSelector *fs)
{
  GtkWidget *menu;
  GtkWidget *submenu;
  GtkWidget *menuitem;
  GSList *group;
  
  menu = gtk_menu_new ();
  fs->alignment_menu = GTK_MENU(menu);
  submenu = NULL;
  group = NULL;

  menuitem = gtk_radio_menu_item_new_with_label (group, _("Left"));
  gtk_object_set_user_data(GTK_OBJECT(menuitem), GINT_TO_POINTER(ALIGN_LEFT));
  group = gtk_radio_menu_item_group (GTK_RADIO_MENU_ITEM (menuitem));
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
  gtk_widget_show (menuitem);

  menuitem = gtk_radio_menu_item_new_with_label (group, _("Center"));
  gtk_object_set_user_data(GTK_OBJECT(menuitem), GINT_TO_POINTER(ALIGN_CENTER));
  group = gtk_radio_menu_item_group (GTK_RADIO_MENU_ITEM (menuitem));
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
  gtk_widget_show (menuitem);

  menuitem = gtk_radio_menu_item_new_with_label (group, _("Right"));
  gtk_object_set_user_data(GTK_OBJECT(menuitem), GINT_TO_POINTER(ALIGN_RIGHT));
  group = gtk_radio_menu_item_group (GTK_RADIO_MENU_ITEM (menuitem));
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
  gtk_widget_show (menuitem);
  
  gtk_menu_set_active(GTK_MENU (menu), DEFAULT_ALIGNMENT);
  gtk_option_menu_set_menu (GTK_OPTION_MENU (fs), menu);
}

GtkType
dia_alignment_selector_get_type        (void)
{
  static GtkType dfs_type = 0;

  if (!dfs_type) {
    GtkTypeInfo dfs_info = {
      "DiaAlignmentSelector",
      sizeof (DiaAlignmentSelector),
      sizeof (DiaAlignmentSelectorClass),
      (GtkClassInitFunc) dia_alignment_selector_class_init,
      (GtkObjectInitFunc) dia_alignment_selector_init,
      NULL,
      NULL,
      (GtkClassInitFunc) NULL,
    };
    
    dfs_type = gtk_type_unique (gtk_option_menu_get_type (), &dfs_info);
  }
  
  return dfs_type;
}

GtkWidget *
dia_alignment_selector_new ()
{
  return GTK_WIDGET ( gtk_type_new (dia_alignment_selector_get_type ()));
}


Alignment 
dia_alignment_selector_get_alignment(DiaAlignmentSelector *fs)
{
  GtkWidget *menuitem;
  void *align;
  
  menuitem = gtk_menu_get_active(fs->alignment_menu);
  align = gtk_object_get_user_data(GTK_OBJECT(menuitem));

  return GPOINTER_TO_INT(align);
}

void
dia_alignment_selector_set_alignment (DiaAlignmentSelector *as,
				      Alignment align)
{
  gtk_menu_set_active(GTK_MENU (as->alignment_menu), align);
  gtk_option_menu_set_history (GTK_OPTION_MENU(as), align);
}

/************* DiaLineStyleSelector: ***************/
struct _DiaLineStyleSelector
{
  GtkVBox vbox;

  GtkOptionMenu *omenu;
  GtkMenu *linestyle_menu;
  GtkLabel *lengthlabel;
  GtkSpinButton *dashlength;
    
};

struct _DiaLineStyleSelectorClass
{
  GtkVBoxClass parent_class;
};

static void
dia_line_style_selector_class_init (DiaLineStyleSelectorClass *class)
{
  GtkObjectClass *object_class;
  
  object_class = (GtkObjectClass*) class;
}

static void
set_linestyle_sensitivity(DiaLineStyleSelector *fs)
{
  int state;
  GtkWidget *menuitem;
  if (!fs->linestyle_menu) return;
  menuitem = gtk_menu_get_active(fs->linestyle_menu);
  state = (GPOINTER_TO_INT(gtk_object_get_user_data(GTK_OBJECT(menuitem)))
	   != LINESTYLE_SOLID);

  gtk_widget_set_sensitive(GTK_WIDGET(fs->lengthlabel), state);
  gtk_widget_set_sensitive(GTK_WIDGET(fs->dashlength), state);
}

static void
linestyle_type_change_callback(GtkObject *as, gboolean arg1, gpointer data)
{
  set_linestyle_sensitivity(DIALINESTYLESELECTOR(as));
}

static void
dia_line_style_selector_init (DiaLineStyleSelector *fs)
{
  GtkWidget *menu;
  GtkWidget *submenu;
  GtkWidget *menuitem, *ln;
  GtkWidget *label;
  GtkWidget *length;
  GtkWidget *box;
  GSList *group;
  GtkAdjustment *adj;
  gint i;
  
  menu = gtk_option_menu_new();
  fs->omenu = GTK_OPTION_MENU(menu);

  menu = gtk_menu_new ();
  fs->linestyle_menu = GTK_MENU(menu);
  submenu = NULL;
  group = NULL;

  for (i = 0; i <= LINESTYLE_DOTTED; i++) {
    menuitem = gtk_menu_item_new();
    gtk_object_set_user_data(GTK_OBJECT(menuitem), GINT_TO_POINTER(i));
    ln = dia_line_preview_new(i);
    gtk_container_add(GTK_CONTAINER(menuitem), ln);
    gtk_widget_show(ln);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
    gtk_widget_show(menuitem);
  }
#if 0
  menuitem = gtk_radio_menu_item_new_with_label (group, _("Solid"));
  gtk_object_set_user_data(GTK_OBJECT(menuitem), GINT_TO_POINTER(LINESTYLE_SOLID));
  group = gtk_radio_menu_item_group (GTK_RADIO_MENU_ITEM (menuitem));
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
  gtk_widget_show (menuitem);

  menuitem = gtk_radio_menu_item_new_with_label (group, _("Dashed"));
  gtk_object_set_user_data(GTK_OBJECT(menuitem), GINT_TO_POINTER(LINESTYLE_DASHED));
  group = gtk_radio_menu_item_group (GTK_RADIO_MENU_ITEM (menuitem));
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
  gtk_widget_show (menuitem);

  menuitem = gtk_radio_menu_item_new_with_label (group, _("Dash-Dot"));
  gtk_object_set_user_data(GTK_OBJECT(menuitem), GINT_TO_POINTER(LINESTYLE_DASH_DOT));
  group = gtk_radio_menu_item_group (GTK_RADIO_MENU_ITEM (menuitem));
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
  gtk_widget_show (menuitem);

  menuitem = gtk_radio_menu_item_new_with_label (group, _("Dash-Dot-Dot"));
  gtk_object_set_user_data(GTK_OBJECT(menuitem), GINT_TO_POINTER(LINESTYLE_DASH_DOT_DOT));
  group = gtk_radio_menu_item_group (GTK_RADIO_MENU_ITEM (menuitem));
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
  gtk_widget_show (menuitem);
  
  menuitem = gtk_radio_menu_item_new_with_label (group, _("Dotted"));
  gtk_object_set_user_data(GTK_OBJECT(menuitem), GINT_TO_POINTER(LINESTYLE_DOTTED));
  group = gtk_radio_menu_item_group (GTK_RADIO_MENU_ITEM (menuitem));
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
  gtk_widget_show (menuitem);
#endif
  
  gtk_menu_set_active(GTK_MENU (menu), DEFAULT_LINESTYLE);
  gtk_option_menu_set_menu (GTK_OPTION_MENU (fs->omenu), menu);
  gtk_signal_connect_object(GTK_OBJECT(menu), "selection-done", 
			    GTK_SIGNAL_FUNC(linestyle_type_change_callback), 
			    (gpointer)fs);
 
  gtk_box_pack_start(GTK_BOX(fs), GTK_WIDGET(fs->omenu), FALSE, TRUE, 0);
  gtk_widget_show(GTK_WIDGET(fs->omenu));

  box = gtk_hbox_new(FALSE,0);
  /*  fs->sizebox = GTK_HBOX(box); */

  label = gtk_label_new(_("Dash length: "));
  fs->lengthlabel = GTK_LABEL(label);
  gtk_box_pack_start_defaults(GTK_BOX(box), label);
  gtk_widget_show(label);

  adj = (GtkAdjustment *)gtk_adjustment_new(0.1, 0.00, 10.0, 0.1, 1.0, 1.0);
  length = gtk_spin_button_new(adj, DEFAULT_LINESTYLE_DASHLEN, 2);
  gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(length), TRUE);
  gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(length), TRUE);
  fs->dashlength = GTK_SPIN_BUTTON(length);
  gtk_box_pack_start_defaults(GTK_BOX (box), length);
  gtk_widget_show (length);

  set_linestyle_sensitivity(fs);
  gtk_box_pack_start_defaults(GTK_BOX(fs), box);
  gtk_widget_show(box);
  
}

GtkType
dia_line_style_selector_get_type        (void)
{
  static GtkType dfs_type = 0;

  if (!dfs_type) {
    GtkTypeInfo dfs_info = {
      "DiaLineStyleSelector",
      sizeof (DiaLineStyleSelector),
      sizeof (DiaLineStyleSelectorClass),
      (GtkClassInitFunc) dia_line_style_selector_class_init,
      (GtkObjectInitFunc) dia_line_style_selector_init,
      NULL,
      NULL,
      (GtkClassInitFunc) NULL,
    };
    
    dfs_type = gtk_type_unique (gtk_vbox_get_type (), &dfs_info);
  }
  
  return dfs_type;
}

GtkWidget *
dia_line_style_selector_new ()
{
  return GTK_WIDGET ( gtk_type_new (dia_line_style_selector_get_type ()));
}


void 
dia_line_style_selector_get_linestyle(DiaLineStyleSelector *fs, 
				      LineStyle *ls, real *dl)
{
  GtkWidget *menuitem;
  void *align;
  
  menuitem = gtk_menu_get_active(fs->linestyle_menu);
  align = gtk_object_get_user_data(GTK_OBJECT(menuitem));
  *ls = GPOINTER_TO_INT(align);
  if (dl!=NULL) {
    *dl = gtk_spin_button_get_value(fs->dashlength);
  }
}

void
dia_line_style_selector_set_linestyle (DiaLineStyleSelector *as,
				       LineStyle linestyle, real dashlength)
{
  gtk_menu_set_active(GTK_MENU (as->linestyle_menu), linestyle);
  gtk_option_menu_set_history (GTK_OPTION_MENU(as->omenu), linestyle);
/* TODO restore this later */
/*  gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM(gtk_menu_get_active(GTK_MENU(as->linestyle_menu))), TRUE);*/
  set_linestyle_sensitivity(DIALINESTYLESELECTOR(as));
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(as->dashlength), dashlength);
}



/************* DiaColorSelector: ***************/

static GtkWidget *
dia_color_selector_create_string_item(DiaDynamicMenu *ddm, gchar *string)
{
  GtkWidget *item = gtk_menu_item_new_with_label(string);
  gint r, g, b;
  sscanf(string, "#%2x%2x%2x", &r, &g, &b);
  
  /* See http://web.umr.edu/~rhall/commentary/color_readability.htm for
   * explanation of this formula */
  if (r*299+g*587+b*114 > 500 * 256) {
    gtk_label_set_markup(GTK_LABEL(gtk_bin_get_child(GTK_BIN(item))),
			 g_strdup_printf("<span foreground=\"black\" background=\"%s\">%s</span>", string, string));
  } else {
    gtk_label_set_markup(GTK_LABEL(gtk_bin_get_child(GTK_BIN(item))),
			 g_strdup_printf("<span foreground=\"white\" background=\"%s\">%s</span>", string, string));
  }
  
  return item;
}

static void
dia_color_selector_more_ok(GtkWidget *ok, gpointer userdata)
{
  DiaDynamicMenu *ddm = g_object_get_data(G_OBJECT(userdata), "ddm");
  GtkWidget *colorsel = GTK_COLOR_SELECTION_DIALOG(userdata);
  GdkColor gcol;
  
  gtk_color_selection_get_current_color(
	GTK_COLOR_SELECTION(
	    GTK_COLOR_SELECTION_DIALOG(colorsel)->colorsel),
	&gcol);

  dia_dynamic_menu_select_entry(ddm,
				g_strdup_printf("#%02X%02X%02X",
						gcol.red/256, 
						gcol.green/256,
						gcol.blue/256));
  gtk_widget_destroy(colorsel);
}

static void
dia_color_selector_activate(DiaDynamicMenu *ddm, gchar *entry, gpointer data)
{
}

static void
dia_color_selector_more_callback(GtkWidget *widget, gpointer userdata)
{
  GtkColorSelectionDialog *dialog = gtk_color_selection_dialog_new(_("Select color"));
  DiaDynamicMenu *ddm = DIA_DYNAMIC_MENU(userdata);

  gchar *old_color = dia_dynamic_menu_get_entry(ddm);
  /* Force history to the old place */
  dia_dynamic_menu_select_entry(ddm, old_color);

  gtk_color_selection_set_has_palette 
    (GTK_COLOR_SELECTION (GTK_COLOR_SELECTION_DIALOG (dialog)->colorsel),
     FALSE);
  
  gtk_widget_hide(dialog->help_button);
  
  gtk_signal_connect (GTK_OBJECT (dialog->ok_button), "clicked",
		      (GtkSignalFunc) dia_color_selector_more_ok,
		      dialog);  
  gtk_signal_connect_object(GTK_OBJECT (dialog->cancel_button), "clicked",
			    (GtkSignalFunc) gtk_widget_destroy,
			    GTK_OBJECT(dialog));
  g_object_set_data(G_OBJECT(dialog), "ddm", ddm);

  gtk_widget_show(GTK_WIDGET(dialog));
}

GtkWidget *
dia_color_selector_new ()
{
  GtkWidget *otheritem = gtk_menu_item_new_with_label(_("More colors..."));
  GtkWidget *ddm = dia_dynamic_menu_new(dia_color_selector_create_string_item,
					dia_color_selector_activate,
					NULL,
					otheritem,
					"color-menu");
  dia_dynamic_menu_add_default_entry(DIA_DYNAMIC_MENU(ddm),
				     "#000000");
  dia_dynamic_menu_add_default_entry(DIA_DYNAMIC_MENU(ddm),
				     "#FFFFFF");
  dia_dynamic_menu_add_default_entry(DIA_DYNAMIC_MENU(ddm),
				     "#FF0000");
  dia_dynamic_menu_add_default_entry(DIA_DYNAMIC_MENU(ddm),
				     "#00FF00");
  dia_dynamic_menu_add_default_entry(DIA_DYNAMIC_MENU(ddm),
				     "#0000FF");
  g_signal_connect(G_OBJECT(otheritem), "activate",
		   dia_color_selector_more_callback, ddm);
  gtk_widget_show(otheritem);
  return ddm;
}


void
dia_color_selector_get_color(GtkWidget *widget, Color *color)
{
  gchar *entry = dia_dynamic_menu_get_entry(DIA_DYNAMIC_MENU(widget));
  gint r, g, b;

  sscanf(entry, "#%2x%2x%2x", &r, &g, &b);
  color->red = r / 255.0;
  color->green = g / 255.0;
  color->blue = b / 255.0;
}

void
dia_color_selector_set_color (GtkWidget *widget,
			      const Color *color)
{
  GdkColor col;
  
  gint red, green, blue;
  gchar *entry;
  red = color->red * 255;
  green = color->green * 255;
  blue = color->blue * 255;
  entry = g_strdup_printf("#%02X%02X%02X", red, green, blue);
  dia_dynamic_menu_select_entry(DIA_DYNAMIC_MENU(widget), entry);
}


/************* DiaArrowSelector: ***************/
struct _DiaArrowSelector
{
  GtkVBox vbox;

  GtkHBox *sizebox;
  GtkLabel *sizelabel;
  DiaSizeSelector *size;
  
  GtkOptionMenu *omenu;
};

struct _DiaArrowSelectorClass
{
  GtkVBoxClass parent_class;
};

static void
dia_arrow_selector_class_init (DiaArrowSelectorClass *class)
{
}
  
static void
set_size_sensitivity(DiaArrowSelector *as)
{
  int state;
  gchar *entryname = dia_dynamic_menu_get_entry(DIA_DYNAMIC_MENU(as->omenu));

  state = (entryname != NULL) && (!g_strcasecmp(entryname, "None"));

  gtk_widget_set_sensitive(GTK_WIDGET(as->sizelabel), state);
  gtk_widget_set_sensitive(GTK_WIDGET(as->size), state);
}

static void
arrow_type_change_callback(DiaDynamicMenu *ddm, gchar *name, gpointer userdata)
{
  set_size_sensitivity(DIA_ARROW_SELECTOR(userdata));
}

static GtkWidget *
create_arrow_menu_item(DiaDynamicMenu *ddm, gchar *name)
{
  ArrowType atype = arrow_type_from_name(name);
  GtkWidget *item = gtk_menu_item_new();
  GtkWidget *preview = dia_arrow_preview_new(atype, FALSE);
  
  gtk_widget_show(preview);
  gtk_container_add(GTK_CONTAINER(item), preview);
  gtk_widget_show(item);
  return item;
}

static void
dia_arrow_selector_init (DiaArrowSelector *as,
			 gpointer g_class)
{
  GtkWidget *omenu;
  GtkWidget *box;
  GtkWidget *label;
  GtkWidget *size;
  
  GList *arrow_names = get_arrow_names();
  omenu = dia_dynamic_menu_new_listbased(create_arrow_menu_item,
					 arrow_type_change_callback, as,
					 _("More arrows"),
					 arrow_names,
					 "arrow-menu");
  dia_dynamic_menu_add_default_entry(DIA_DYNAMIC_MENU(omenu), "None");
  dia_dynamic_menu_add_default_entry(DIA_DYNAMIC_MENU(omenu), "Lines");
  dia_dynamic_menu_add_default_entry(DIA_DYNAMIC_MENU(omenu), "Filled Concave");
  as->omenu = GTK_OPTION_MENU(omenu);
  gtk_box_pack_start(GTK_BOX(as), omenu, FALSE, TRUE, 0);
  gtk_widget_show(omenu);

  box = gtk_hbox_new(FALSE,0);
  as->sizebox = GTK_HBOX(box);

  label = gtk_label_new(_("Size: "));
  as->sizelabel = GTK_LABEL(label);
  gtk_box_pack_start_defaults(GTK_BOX(box), label);
  gtk_widget_show(label);

  size = dia_size_selector_new(DEFAULT_ARROW_WIDTH, DEFAULT_ARROW_LENGTH);
  as->size = DIA_SIZE_SELECTOR(size);
  gtk_box_pack_start_defaults(GTK_BOX(box), size);
  gtk_widget_show(size);  

  set_size_sensitivity(as);
  gtk_box_pack_start_defaults(GTK_BOX(as), box);

  gtk_widget_show(box);

}

GType
dia_arrow_selector_get_type        (void)
{
  static GType dfs_type = 0;

  if (!dfs_type) {
    static const GTypeInfo dfs_info = {
      /*      sizeof (DiaArrowSelector),*/
      sizeof (DiaArrowSelectorClass),
      (GBaseInitFunc) NULL,
      (GBaseFinalizeFunc) NULL,
      (GClassInitFunc) dia_arrow_selector_class_init,
      NULL, /* class_finalize */
      NULL, /* class_data */
      sizeof (DiaArrowSelector),
      0,    /* n_preallocs */
      (GInstanceInitFunc)dia_arrow_selector_init,  /* init */
      /*
      (GtkObjectInitFunc) dia_arrow_selector_init,
      NULL,
      NULL,
      (GtkClassInitFunc) NULL,
      */
    };
    
    dfs_type = g_type_register_static (GTK_TYPE_VBOX,
				       "DiaArrowSelector",
				       &dfs_info, 0);
  }
  
  return dfs_type;
}

GtkWidget *
dia_arrow_selector_new ()
{
  return GTK_WIDGET ( g_object_new (DIA_TYPE_ARROW_SELECTOR, NULL));
}


Arrow 
dia_arrow_selector_get_arrow(DiaArrowSelector *as)
{
  Arrow at;

  at.type = arrow_type_from_name(dia_dynamic_menu_get_entry(DIA_DYNAMIC_MENU(as->omenu)));
  dia_size_selector_get_size(as->size, &at.width, &at.length);
  return at;
}

void
dia_arrow_selector_set_arrow (DiaArrowSelector *as,
			      Arrow arrow)
{
  dia_dynamic_menu_select_entry(DIA_DYNAMIC_MENU(as->omenu),
				arrow_types[arrow_index_from_type(arrow.type)].name);
  set_size_sensitivity(as);
  dia_size_selector_set_size(DIA_SIZE_SELECTOR(as->size), arrow.width, arrow.length);
}

/************* DiaFileSelector: ***************/
struct _DiaFileSelector
{
  GtkHBox hbox;
  GtkEntry *entry;
  GtkButton *browse;
  GtkFileSelection *dialog;
  gchar *sys_filename;
};

struct _DiaFileSelectorClass
{
  GtkHBoxClass parent_class;
};

static void
dia_file_selector_unrealize(GtkWidget *widget)
{
  DiaFileSelector *fs = DIAFILESELECTOR(widget);

  if (fs->dialog != NULL) {
    gtk_widget_destroy(GTK_WIDGET(fs->dialog));
    fs->dialog = NULL;
  }
  if (fs->sys_filename) {
    g_free(fs->sys_filename);
    fs->sys_filename = NULL;
  }

  (* GTK_WIDGET_CLASS (gtk_type_class(gtk_hbox_get_type ()))->unrealize) (widget);
}

static void
dia_file_selector_class_init (DiaFileSelectorClass *class)
{
  GtkObjectClass *object_class;
  GtkWidgetClass *widget_class;
  
  object_class = (GtkObjectClass*) class;
  widget_class = (GtkWidgetClass*) class;
  widget_class->unrealize = dia_file_selector_unrealize;
}

static void
dia_file_selector_ok(GtkWidget *widget, gpointer data)
{
  gchar *utf8;
  GtkFileSelection *dialog = GTK_FILE_SELECTION(data);
  DiaFileSelector *fs =
    DIAFILESELECTOR(gtk_object_get_user_data(GTK_OBJECT(dialog)));
  utf8 = g_filename_to_utf8(gtk_file_selection_get_filename(dialog), 
                            -1, NULL, NULL, NULL);
  gtk_entry_set_text(GTK_ENTRY(fs->entry), utf8);
  g_free(utf8);
  gtk_widget_hide(GTK_WIDGET(dialog));
}

static void
dia_file_selector_browse_pressed(GtkWidget *widget, gpointer data)
{
  GtkFileSelection *dialog;
  DiaFileSelector *fs = DIAFILESELECTOR(data);
  gchar *filename;
  
  if (fs->dialog == NULL) {
    dialog = fs->dialog =
      GTK_FILE_SELECTION(gtk_file_selection_new(_("Select image file")));

    if (dialog->help_button != NULL)
      gtk_widget_hide(dialog->help_button);
    
    gtk_signal_connect (GTK_OBJECT (dialog->ok_button), "clicked",
			(GtkSignalFunc) dia_file_selector_ok,
			dialog);
    
    gtk_signal_connect (GTK_OBJECT (fs->dialog), "destroy",
			GTK_SIGNAL_FUNC (gtk_widget_destroyed),
			&fs->dialog);
    
    gtk_signal_connect_object(GTK_OBJECT (dialog->cancel_button), "clicked",
			      (GtkSignalFunc) gtk_widget_hide,
			      GTK_OBJECT(dialog));
    
    gtk_object_set_user_data(GTK_OBJECT(dialog), fs);
  }

  filename = g_filename_from_utf8(gtk_entry_get_text(fs->entry), -1, NULL, NULL, NULL);
  gtk_file_selection_set_filename(fs->dialog, filename);
  g_free(filename);
  
  gtk_widget_show(GTK_WIDGET(fs->dialog));
}

static void
dia_file_selector_init (DiaFileSelector *fs)
{
  /* Here's where we set up the real thing */
  fs->dialog = NULL;
  fs->sys_filename = NULL;  
  fs->entry = GTK_ENTRY(gtk_entry_new());
  gtk_box_pack_start(GTK_BOX(fs), GTK_WIDGET(fs->entry), FALSE, TRUE, 0);
  gtk_widget_show(GTK_WIDGET(fs->entry));
  fs->browse = GTK_BUTTON(gtk_button_new_with_label(_("Browse")));
  gtk_box_pack_start(GTK_BOX(fs), GTK_WIDGET(fs->browse), FALSE, TRUE, 0);
  gtk_signal_connect (GTK_OBJECT (fs->browse), "clicked",
                      (GtkSignalFunc) dia_file_selector_browse_pressed,
                      fs);
  gtk_widget_show(GTK_WIDGET(fs->browse));
}


GtkType
dia_file_selector_get_type (void)
{
  static GtkType dfs_type = 0;

  if (!dfs_type) {
    GtkTypeInfo dfs_info = {
      "DiaFileSelector",
      sizeof (DiaFileSelector),
      sizeof (DiaFileSelectorClass),
      (GtkClassInitFunc) dia_file_selector_class_init,
      (GtkObjectInitFunc) dia_file_selector_init,
      NULL,
      NULL,
      (GtkClassInitFunc) NULL,
    };
    
    dfs_type = gtk_type_unique (gtk_hbox_get_type (), &dfs_info);

  }
  
  return dfs_type;
}

GtkWidget *
dia_file_selector_new ()
{
  return GTK_WIDGET ( gtk_type_new (dia_file_selector_get_type ()));
}

void
dia_file_selector_set_file(DiaFileSelector *fs, gchar *file)
{
  /* filename is in system encoding */
  gchar *utf8 = g_filename_to_utf8(file, -1, NULL, NULL, NULL);
  gtk_entry_set_text(GTK_ENTRY(fs->entry), utf8);
  g_free(utf8);
}

const gchar *
dia_file_selector_get_file(DiaFileSelector *fs)
{
  /* let it behave like gtk_file_selector_get_file */
  g_free(fs->sys_filename);
  fs->sys_filename = g_filename_from_utf8(gtk_entry_get_text(GTK_ENTRY(fs->entry)),
                                          -1, NULL, NULL, NULL);
  return fs->sys_filename;
}

/************* DiaUnitSpinner: ***************/

/** A Spinner that allows a 'favored' unit to display in.  External access
 *  to the value still happens in cm, but display is in the favored unit.
 *  Internally, the value is kept in the favored unit to a) allow proper
 *  limits, and b) avoid rounding problems while editing.
 */

typedef struct _DiaUnitDef DiaUnitDef;
struct _DiaUnitDef {
  char* name;
  char* unit;
  float factor;
};

/* from gnome-libs/libgnome/gnome-paper.c */
static const DiaUnitDef units[] =
{
  /* XXX does anyone *really* measure paper size in feet?  meters? */

  /* human name, abreviation, points per unit */
  { "Feet",       "ft", 864 },
  { "Meter",      "m",  2834.6457 },
  { "Decimeter",  "dm", 283.46457 },
  { "Millimeter", "mm", 2.8346457 },
  { "Point",      "pt", 1. },
  { "Centimeter", "cm", 28.346457 },
  { "Inch",       "in", 72 },
  { "Pica",       "pi", 12 },
  { 0 }
};

static GtkObjectClass *parent_class;
static GtkObjectClass *entry_class;

static void dia_unit_spinner_class_init(DiaUnitSpinnerClass *class);
static void dia_unit_spinner_init(DiaUnitSpinner *self);

GtkType
dia_unit_spinner_get_type(void)
{
  static GtkType us_type = 0;

  if (!us_type) {
    GtkTypeInfo us_info = {
      "DiaUnitSpinner",
      sizeof(DiaUnitSpinner),
      sizeof(DiaUnitSpinnerClass),
      (GtkClassInitFunc) dia_unit_spinner_class_init,
      (GtkObjectInitFunc) dia_unit_spinner_init,
      NULL,
      NULL,
      (GtkClassInitFunc) NULL,
    };
    us_type = gtk_type_unique(gtk_spin_button_get_type(), &us_info);
  }
  return us_type;
}

/** Updates the spinner display to show digits and units */
static void
dia_unit_spinner_value_changed(GtkAdjustment *adjustment,
			       DiaUnitSpinner *spinner)
{
  char buf[256];
  GtkSpinButton *sbutton = GTK_SPIN_BUTTON(spinner);

  g_snprintf(buf, sizeof(buf), "%0.*f%s", sbutton->digits, adjustment->value,
	     units[spinner->unit_num].unit);
  gtk_entry_set_text(GTK_ENTRY(spinner), buf);
}

static void dia_unit_spinner_set_value_direct(DiaUnitSpinner *self, gfloat val);
static gint dia_unit_spinner_focus_out(GtkWidget *widget, GdkEventFocus *ev);
static gint dia_unit_spinner_button_press(GtkWidget *widget,GdkEventButton*ev);
static gint dia_unit_spinner_key_press(GtkWidget *widget, GdkEventKey *event);
static void dia_unit_spinner_activate(GtkEntry *editable);

static void
dia_unit_spinner_class_init(DiaUnitSpinnerClass *class)
{
  GtkObjectClass *object_class;
  GtkWidgetClass *widget_class;
  GtkEntryClass  *editable_class;

  object_class = (GtkObjectClass *)class;
  widget_class = (GtkWidgetClass *)class;
  editable_class = (GtkEntryClass *)class;

  widget_class->focus_out_event    = dia_unit_spinner_focus_out;
  widget_class->button_press_event = dia_unit_spinner_button_press;
  widget_class->key_press_event    = dia_unit_spinner_key_press;
  editable_class->activate         = dia_unit_spinner_activate;

  parent_class = gtk_type_class(GTK_TYPE_SPIN_BUTTON);
  entry_class  = gtk_type_class(GTK_TYPE_ENTRY);
}

static void
dia_unit_spinner_init(DiaUnitSpinner *self)
{
  /* change over to our own print function that appends the unit name on the
   * end */
  if (self->parent.adjustment) {
    gtk_signal_disconnect_by_data(GTK_OBJECT(self->parent.adjustment),
				  (gpointer) self);
    g_signal_connect(GTK_OBJECT(self->parent.adjustment), "value_changed",
		      G_CALLBACK(dia_unit_spinner_value_changed),
		       (gpointer) self);
  }

  self->unit_num = DIA_UNIT_CENTIMETER;
}

GtkWidget *
dia_unit_spinner_new(GtkAdjustment *adjustment, guint digits, DiaUnit adj_unit)
{
  DiaUnitSpinner *self = gtk_type_new(dia_unit_spinner_get_type());

  self->unit_num = adj_unit;

  gtk_spin_button_configure(GTK_SPIN_BUTTON(self), adjustment, 0.0, digits);

  if (adjustment) {
    gtk_signal_disconnect_by_data(GTK_OBJECT(adjustment),
				  (gpointer) self);
    g_signal_connect(GTK_OBJECT(adjustment), "value_changed",
		     G_CALLBACK(dia_unit_spinner_value_changed),
		       (gpointer) self);
  }
  dia_unit_spinner_set_value(self, adjustment->value);

  return GTK_WIDGET(self);
}

/** Set the value (in cm).
 * */
void
dia_unit_spinner_set_value(DiaUnitSpinner *self, gfloat val)
{
  dia_unit_spinner_set_value_direct(self, val /
				    (units[self->unit_num].factor / units[DIA_UNIT_CENTIMETER].factor));
}

/** Set the value (in preferred units) */
static void
dia_unit_spinner_set_value_direct(DiaUnitSpinner *self, gfloat val)
{
  GtkSpinButton *sbutton = GTK_SPIN_BUTTON(self);

  if (val < sbutton->adjustment->lower)
    val = sbutton->adjustment->lower;
  else if (val > sbutton->adjustment->upper)
    val = sbutton->adjustment->upper;
  sbutton->adjustment->value = val;
  dia_unit_spinner_value_changed(sbutton->adjustment, self);
}

/** Get the value (in cm) */
gfloat
dia_unit_spinner_get_value(DiaUnitSpinner *self)
{
  GtkSpinButton *sbutton = GTK_SPIN_BUTTON(self);

  return sbutton->adjustment->value *
    (units[self->unit_num].factor / units[DIA_UNIT_CENTIMETER].factor);
}

static void
dia_unit_spinner_update(DiaUnitSpinner *self)
{
  GtkSpinButton *sbutton = GTK_SPIN_BUTTON(self);
  gfloat val, factor = 1.0;
  gchar *extra = NULL;

  val = g_strtod(gtk_entry_get_text(GTK_ENTRY(self)), &extra);

  /* get rid of extra white space after number */
  while (*extra && g_ascii_isspace(*extra)) extra++;
  if (*extra) {
    int i;

    for (i = 0; units[i].name != NULL; i++)
      if (!g_strcasecmp(units[i].unit, extra)) {
	factor = units[i].factor / units[self->unit_num].factor;
	break;
      }
  }
  /* convert to prefered units */
  val *= factor;
  dia_unit_spinner_set_value_direct(self, val);
}

static gint
dia_unit_spinner_focus_out(GtkWidget *widget, GdkEventFocus *event)
{
  if (GTK_ENTRY (widget)->editable)
    dia_unit_spinner_update(DIA_UNIT_SPINNER(widget));
  return GTK_WIDGET_CLASS(entry_class)->focus_out_event(widget, event);
}

static gint
dia_unit_spinner_button_press(GtkWidget *widget, GdkEventButton *event)
{
  dia_unit_spinner_update(DIA_UNIT_SPINNER(widget));
  return GTK_WIDGET_CLASS(parent_class)->button_press_event(widget, event);
}

static gint
dia_unit_spinner_key_press(GtkWidget *widget, GdkEventKey *event)
{
  gint key = event->keyval;

  if (GTK_ENTRY (widget)->editable &&
      (key == GDK_Up || key == GDK_Down || 
       key == GDK_Page_Up || key == GDK_Page_Down))
    dia_unit_spinner_update (DIA_UNIT_SPINNER(widget));
  return GTK_WIDGET_CLASS(parent_class)->key_press_event(widget, event);
}

static void
dia_unit_spinner_activate(GtkEntry *editable)
{
  if (editable->editable)
    dia_unit_spinner_update(DIA_UNIT_SPINNER(editable));
}


/* ************************ Dynamic menus ************************ */

static void dia_dynamic_menu_class_init(DiaDynamicMenuClass *class);
static void dia_dynamic_menu_init(DiaDynamicMenu *self);
static void dia_dynamic_menu_create_sublist(DiaDynamicMenu *ddm,
					    GList *items, 
					    DDMCreateItemFunc create);
static void dia_dynamic_menu_create_menu(DiaDynamicMenu *ddm);

GtkType
dia_dynamic_menu_get_type(void)
{
  static GtkType us_type = 0;

  if (!us_type) {
    GtkTypeInfo us_info = {
      "DiaDynamicMenu",
      sizeof(DiaDynamicMenu),
      sizeof(DiaDynamicMenuClass),
      (GtkClassInitFunc) dia_dynamic_menu_class_init,
      (GtkObjectInitFunc) dia_dynamic_menu_init,
      NULL,
      NULL,
      (GtkClassInitFunc) NULL,
    };
    us_type = gtk_type_unique(gtk_option_menu_get_type(), &us_info);
  }
  return us_type;
}

static void
dia_dynamic_menu_class_init(DiaDynamicMenuClass *class)
{
}

static void
dia_dynamic_menu_init(DiaDynamicMenu *self)
{
}

/** Create a new dynamic menu.  The entries are represented with
 * gpointers.
 * @param create A function that creates menuitems from gpointers.
 * @param otheritem A menuitem that can be selected by the user to
 * add more entries, for instance making a dialog box or a submenu.
 * @param persist A string naming this menu for persistence purposes, or NULL.

 * @return A new menu
 */
GtkWidget *
dia_dynamic_menu_new(DDMCreateItemFunc create, 
		     DDMCallbackFunc activate, gpointer userdata,
		     GtkMenuItem *otheritem, gchar *persist)
{
  DiaDynamicMenu *ddm;

  g_assert(persist != NULL);

  ddm = DIA_DYNAMIC_MENU ( gtk_type_new (dia_dynamic_menu_get_type ()));
  
  ddm->create_func = create;
  ddm->activate_func = activate;
  ddm->userdata = userdata;
  ddm->other_item = otheritem;
  ddm->persistent_name = persist;
  ddm->cols = 1;

  persistence_register_list(persist);

  dia_dynamic_menu_create_menu(ddm);

  return GTK_WIDGET(ddm);
}

/** Select the given entry, adding it if necessary */
void
dia_dynamic_menu_select_entry(DiaDynamicMenu *ddm, gchar *name)
{
  gint add_result = dia_dynamic_menu_add_entry(ddm, name);
  if (add_result == 0) {
      GList *tmp;
      int i = 0;
      for (tmp = ddm->default_entries; tmp != NULL;
	   tmp = g_list_next(tmp), i++) {
	if (!g_strcasecmp(tmp->data, name))
	  gtk_option_menu_set_history(GTK_OPTION_MENU(ddm), i);
      }
      /* Not there after all? */
  } else {
    if (ddm->default_entries != NULL)
      gtk_option_menu_set_history(GTK_OPTION_MENU(ddm),
				  g_list_length(ddm->default_entries)+1);
    else
      gtk_option_menu_set_history(GTK_OPTION_MENU(ddm), 0);
  }
  if (ddm->activate_func != NULL) {
    (ddm->activate_func)(ddm, name, ddm->userdata);
  }
}

static void
dia_dynamic_menu_activate(GtkWidget *item, gpointer userdata)
{
  DiaDynamicMenu *ddm = DIA_DYNAMIC_MENU(userdata);
  gchar *name = g_object_get_data(G_OBJECT(item), "ddm_name");
  dia_dynamic_menu_select_entry(ddm, name);
}

static GtkWidget *
dia_dynamic_menu_create_string_item(DiaDynamicMenu *ddm, gchar *string)
{
  GtkWidget *item = gtk_menu_item_new_with_label(string);
  return item;
}

/** Utility function for dynamic menus that are entirely based on the
 * labels in the menu.
 */
GtkWidget *
dia_dynamic_menu_new_stringbased(GtkMenuItem *otheritem, 
				 DDMCallbackFunc activate,
				 gpointer userdata,
				 gchar *persist)
{
  GtkWidget *ddm = dia_dynamic_menu_new(dia_dynamic_menu_create_string_item,
					activate, userdata,
					otheritem, persist);
  return ddm;
}

/** Utility function for dynamic menus that are based on a submenu with
 * many entries.  This is useful for allowing the user to get a smaller 
 * subset menu out of a set too large to be easily handled by a menu.
 */
GtkWidget *
dia_dynamic_menu_new_listbased(DDMCreateItemFunc create,
			       DDMCallbackFunc activate,
			       gpointer userdata,
			       gchar *other_label, GList *items, 
			       gchar *persist)
{
  GtkWidget *item = gtk_menu_item_new_with_label(other_label);
  GtkWidget *ddm = dia_dynamic_menu_new(create, activate, userdata,
					GTK_MENU_ITEM(item), persist);
  dia_dynamic_menu_create_sublist(DIA_DYNAMIC_MENU(ddm), items,  create);

  gtk_widget_show(item);
  return ddm;
}

/** Utility function for dynamic menus that allow selection from a large
 * number of strings.
 */
GtkWidget *
dia_dynamic_menu_new_stringlistbased(gchar *other_label, 
				     GList *items,
				     DDMCallbackFunc activate,
				     gpointer userdata,
				     gchar *persist)
{
  return dia_dynamic_menu_new_listbased(dia_dynamic_menu_create_string_item,
					activate, userdata,
					other_label, items, persist);
}

static void
dia_dynamic_menu_create_sublist(DiaDynamicMenu *ddm,
				GList *items, DDMCreateItemFunc create)
{
  GtkWidget *item = GTK_WIDGET(ddm->other_item);

  GtkWidget *submenu = gtk_menu_new();

  for (; items != NULL; items = g_list_next(items)) {
    GtkWidget *submenuitem = (create)(ddm, items->data);
    /* Set callback function to cause addition of item */
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), submenuitem);
    g_object_set_data(G_OBJECT(submenuitem), "ddm_name", items->data);
    g_signal_connect(submenuitem, "activate",
		     G_CALLBACK(dia_dynamic_menu_activate), ddm);
    gtk_widget_show(submenuitem);
  }

  gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), submenu);
  gtk_widget_show(submenu);
}

/** Add a new default entry to this menu.
 * The default entries are always shown, also after resets, above the
 * other entries.  Possible uses are standard fonts or common colors.
 * The entry is added at the end of the default entries section.
 * Do not add too many default entries.
 *
 * @param ddm A dynamic menu to add the entry to.
 * @param entry An entry for the menu.
 */
void
dia_dynamic_menu_add_default_entry(DiaDynamicMenu *ddm, gchar *entry)
{
  ddm->default_entries = g_list_append(ddm->default_entries, entry);

  dia_dynamic_menu_create_menu(ddm);
}

/** Set the number of columns this menu uses (default 1)
 * @param cols Desired # of columns (>= 1)
 */
void
dia_dynamic_menu_set_columns(DiaDynamicMenu *ddm, gint cols)
{
  ddm->cols = cols;

  dia_dynamic_menu_create_menu(ddm);
}

/** Add a new entry to this menu.  The placement depends on what sorting
 * system has been chosen with dia_dynamic_menu_set_sorting_method().
 *
 * @param ddm A dynamic menu to add the entry to.
 * @param entry An entry for the menu.
 *
 * @returns 0 if the entry was one of the default entries.
 * 1 if the entry was already there.
 * 2 if the entry got added.
 */
gint
dia_dynamic_menu_add_entry(DiaDynamicMenu *ddm, gchar *entry)
{
  GList *tmp;
  gboolean existed;

  ddm->active = entry;

  for (tmp = ddm->default_entries; tmp != NULL; tmp = g_list_next(tmp)) {
    if (!g_strcasecmp(tmp->data, entry))
      return 0;
  }
  existed = persistent_list_add(ddm->persistent_name, entry);

  dia_dynamic_menu_create_menu(ddm);
  
  return existed?1:2;
}

/** Returns the currently selected entry. */
gchar *
dia_dynamic_menu_get_entry(DiaDynamicMenu *ddm)
{
  return ddm->active;
}

/** Rebuild the actual menu of a DDM.
 * Ignores columns for now.
 */
static void
dia_dynamic_menu_create_menu(DiaDynamicMenu *ddm)
{
  GtkWidget *sep;
  GList *tmplist;
  GtkWidget *menu;
  GtkWidget *item;

  g_object_ref(G_OBJECT(ddm->other_item));
  menu = gtk_option_menu_get_menu(GTK_OPTION_MENU(ddm));
  if (menu != NULL) {
    gtk_container_remove(GTK_CONTAINER(menu), GTK_WIDGET(ddm->other_item));
    gtk_container_foreach(GTK_CONTAINER(menu),
			  (GtkCallback)gtk_widget_destroy, NULL);
    gtk_option_menu_remove_menu(GTK_OPTION_MENU(ddm));
  }

  menu = gtk_menu_new();

  if (ddm->default_entries != NULL) {
    for (tmplist = ddm->default_entries; tmplist != NULL; tmplist = g_list_next(tmplist)) {
      GtkWidget *item =  (ddm->create_func)(ddm, tmplist->data);
      g_object_set_data(G_OBJECT(item), "ddm_name", tmplist->data);
      g_signal_connect(G_OBJECT(item), "activate", 
		       dia_dynamic_menu_activate, ddm);
      gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
      gtk_widget_show(item);
    }
    sep = gtk_separator_menu_item_new();
    gtk_widget_show(sep);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), sep);
  }

  for (tmplist = persistent_list_get_glist(ddm->persistent_name);
       tmplist != NULL; tmplist = g_list_next(tmplist)) {
    GtkWidget *item = (ddm->create_func)(ddm, tmplist->data);
    g_object_set_data(G_OBJECT(item), "ddm_name", tmplist->data);
    g_signal_connect(G_OBJECT(item), "activate", 
		     dia_dynamic_menu_activate, ddm);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
    gtk_widget_show(item);
  }
  sep = gtk_separator_menu_item_new();
  gtk_widget_show(sep);
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), sep);

  gtk_menu_shell_append(GTK_MENU_SHELL(menu), GTK_WIDGET(ddm->other_item));
  g_object_unref(G_OBJECT(ddm->other_item));
  /* Eventually reset item here */
  gtk_widget_show(menu);

  item = gtk_menu_item_new_with_label(_("Reset menu"));
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
  g_signal_connect(G_OBJECT(item), "activate", dia_dynamic_menu_reset, ddm);
  gtk_widget_show(item);

  gtk_option_menu_set_menu(GTK_OPTION_MENU(ddm), menu);

  gtk_option_menu_set_history(GTK_OPTION_MENU(ddm), 0);
}

/** Select the method used for sorting the non-default entries.
 * @param ddm A dynamic menu
 * @param sort The way the non-default entries in the menu should be sorted.
 */
void
dia_dynamic_menu_set_sorting_method(DiaDynamicMenu *ddm, DdmSortType sort)
{
}

/** Reset the non-default entries of a menu
 */
void
dia_dynamic_menu_reset(GtkWidget *item, gpointer userdata)
{
  DiaDynamicMenu *ddm = DIA_DYNAMIC_MENU(userdata);
  PersistentList *plist = persistent_list_get(ddm->persistent_name);
  g_list_free(plist->glist);
  plist->glist = NULL;
  dia_dynamic_menu_create_menu(ddm);
  dia_dynamic_menu_select_entry(ddm, ddm->active);
}

/** Set the maximum number of non-default entries.
 * If more than this number of entries are added, the least recently
 * selected ones are removed. */
void
dia_dynamic_menu_set_max_entries(DiaDynamicMenu *ddm, gint max)
{
}

/* ************************ Misc. util functions ************************ */
/** Get a GtkImage from the data in Dia data file filename.
 * On Unix, this could be /usr/local/share/dia/image/<filename>
 */
GtkWidget *
dia_get_image_from_file(gchar *filename)
{
  gchar *datadir = dia_get_data_directory("images");
  gchar *imagefile = g_strconcat(datadir, G_DIR_SEPARATOR_S, filename, NULL);
  GtkWidget *image = gtk_image_new_from_file(imagefile);
  g_free(imagefile);
  g_free(datadir);
  return image;
}

struct image_pair { GtkWidget *on; GtkWidget *off; };

static void
dia_toggle_button_swap_images(GtkToggleButton *widget,
			      gpointer data) 
{
  struct image_pair *images = (struct image_pair *)data;
  if (gtk_toggle_button_get_active(widget)) {
    gtk_container_remove(GTK_CONTAINER(widget), 
			 gtk_bin_get_child(GTK_BIN(widget)));
    gtk_container_add(GTK_CONTAINER(widget),
		      images->on);
    
  } else {
    gtk_container_remove(GTK_CONTAINER(widget), 
			 gtk_bin_get_child(GTK_BIN(widget)));
    gtk_container_add(GTK_CONTAINER(widget),
		      images->off);
  }
}

static void
dia_toggle_button_destroy(GtkWidget *widget, gpointer data)
{
  struct image_pair *images = (struct image_pair *)data;
  g_object_unref(images->on);
  g_object_unref(images->off);
  g_free(images);
}

/** Create a toggle button with two images switching (on and off) */
GtkWidget *
dia_toggle_button_new_with_images(gchar *on_image, gchar *off_image)
{
  GtkWidget *button = gtk_toggle_button_new();
  struct image_pair *images = g_new0(struct image_pair, 1);
  GtkRcStyle *rcstyle;
  GtkStyle *style;
  GValue *prop;
  gint i;

  /* Since these may not be added at any point, make sure to
   * sink them. */
  images->on = dia_get_image_from_file(on_image);
  g_object_ref(G_OBJECT(images->on));
  gtk_object_sink(GTK_OBJECT(images->on));
  gtk_widget_show(images->on);

  images->off = dia_get_image_from_file(off_image);
  g_object_ref(G_OBJECT(images->off));
  gtk_object_sink(GTK_OBJECT(images->off));
  gtk_widget_show(images->off);

  /* Make border as small as possible */
  gtk_misc_set_padding(GTK_MISC(images->on), 0, 0);
  gtk_misc_set_padding(GTK_MISC(images->off), 0, 0);
  GTK_WIDGET_UNSET_FLAGS(GTK_WIDGET(button), GTK_CAN_FOCUS);
  GTK_WIDGET_UNSET_FLAGS(GTK_WIDGET(button), GTK_CAN_DEFAULT);

  rcstyle = gtk_rc_style_new ();  
  rcstyle->xthickness = rcstyle->ythickness = 0;       
  gtk_widget_modify_style (button, rcstyle);
  gtk_rc_style_unref (rcstyle);

  prop = g_new0(GValue, 1);
  g_value_init(prop, G_TYPE_INT);
  gtk_widget_style_get_property(GTK_WIDGET(button), "focus-padding", prop);
  i = g_value_get_int(prop);
  g_value_set_int(prop, 0);

  gtk_button_set_relief(GTK_BUTTON(button), GTK_RELIEF_NONE);
  /*  gtk_button_set_focus_on_click(GTK_BUTTON(button), FALSE);*/
  gtk_container_set_border_width(GTK_CONTAINER(button), 0);

  gtk_container_add(GTK_CONTAINER(button), images->off);

  g_signal_connect(G_OBJECT(button), "toggled", 
		   dia_toggle_button_swap_images, images);
  g_signal_connect(G_OBJECT(button), "destroy",
		   dia_toggle_button_destroy, images);

  return button;
}
