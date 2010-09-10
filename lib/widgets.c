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
#include "units.h"
#include "message.h"
#include "dia_dirs.h"
#include "arrows.h"
#include "diaarrowchooser.h"
#include "dialinechooser.h"
#include "persistence.h"
#include "dia-lib-icons.h"

#include <stdlib.h>
#include <glib.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <pango/pango.h>
#include <stdio.h>
#include <time.h>
#include <gdk/gdkkeysyms.h>

/* hidden internals for two reasosn: 
 * - noone is supposed to mess with the internals 
 * - it uses deprecated stuff
 */
struct _DiaDynamicMenu {
  GtkOptionMenu parent;

  GList *default_entries;

  DDMCreateItemFunc create_func;
  DDMCallbackFunc activate_func;
  gpointer userdata;

  GtkMenuItem *other_item;

  gchar *persistent_name;
  gint cols;

  gchar *active;
  /** For the list-based versions, these are the options */
  GList *options;

};

struct _DiaDynamicMenuClass {
  GtkOptionMenuClass parent_class;
};


/************* DiaSizeSelector: ***************/
/* A widget that selects two sizes, width and height, optionally keeping
 * aspect ratio.  When created, aspect ratio is locked, but the user can
 * unlock it.  The current users do not store aspect ratio, so we have
 * to give a good default.  
 */
struct _DiaSizeSelector
{
  GtkHBox hbox;
  GtkSpinButton *width, *height;
  GtkToggleButton *aspect_locked;
  real ratio;
  GtkAdjustment *last_adjusted;
};

struct _DiaSizeSelectorClass
{
  GtkHBoxClass parent_class;
};

enum {
    DSS_VALUE_CHANGED,
    DSS_LAST_SIGNAL
};

static guint dss_signals[DSS_LAST_SIGNAL] = { 0 };

static void
dia_size_selector_unrealize(GtkWidget *widget)
{
  (* GTK_WIDGET_CLASS (gtk_type_class(gtk_hbox_get_type ()))->unrealize) (widget);
}

static void
dia_size_selector_class_init (DiaSizeSelectorClass *class)
{
  GtkWidgetClass *widget_class;
  
  widget_class = (GtkWidgetClass*) class;
  widget_class->unrealize = dia_size_selector_unrealize;

  dss_signals[DSS_VALUE_CHANGED]
      = g_signal_new("value-changed",
		     G_TYPE_FROM_CLASS(class),
		     G_SIGNAL_RUN_FIRST,
		     0, NULL, NULL,
		     g_cclosure_marshal_VOID__VOID,
		     G_TYPE_NONE, 0);
}

static void
dia_size_selector_adjust_width(DiaSizeSelector *ss)
{
  real height = 
    gtk_spin_button_get_value(GTK_SPIN_BUTTON(ss->height));
  if (fabs(ss->ratio) > 1e-6)
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(ss->width), height*ss->ratio);
}

static void
dia_size_selector_adjust_height(DiaSizeSelector *ss)
{
  real width = 
    gtk_spin_button_get_value(GTK_SPIN_BUTTON(ss->width));
  if (fabs(ss->ratio) > 1e-6)
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(ss->height), width/ss->ratio);
}

static void
dia_size_selector_ratio_callback(GtkAdjustment *limits, gpointer userdata) 
{
  static gboolean in_progress = FALSE;
  DiaSizeSelector *ss = DIA_SIZE_SELECTOR(userdata);

  ss->last_adjusted = limits;

  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ss->aspect_locked)) 
      && ss->ratio != 0.0) {

    if (in_progress) 
      return;
    in_progress = TRUE;

    if (limits == gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(ss->width))) {
      dia_size_selector_adjust_height(ss);
    } else {
      dia_size_selector_adjust_width(ss);
    }

    in_progress = FALSE;
  }

  g_signal_emit(ss, dss_signals[DSS_VALUE_CHANGED], 0);

}

/** Update the ratio of this DSS to be the ratio of width to height.
 * If height is 0, ratio becomes 0.0.
 */
static void
dia_size_selector_set_ratio(DiaSizeSelector *ss, real width, real height) 
{
  if (height > 0.0) 
    ss->ratio = width/height;
  else 
    ss->ratio = 0.0;
}

static void
dia_size_selector_lock_pressed(GtkWidget *widget, gpointer data)
{
  DiaSizeSelector *ss = DIA_SIZE_SELECTOR(data);

  dia_size_selector_set_ratio(ss, 
			      gtk_spin_button_get_value(GTK_SPIN_BUTTON(ss->width)),
			      gtk_spin_button_get_value(GTK_SPIN_BUTTON(ss->height)));
}

/* Possible args:  Init width, init height, digits */

static void
dia_size_selector_init (DiaSizeSelector *ss)
{
  GtkAdjustment *adj;

  ss->ratio = 0.0;
  /* Here's where we set up the real thing */
  adj = GTK_ADJUSTMENT(gtk_adjustment_new(1.0, 0.01, 10,
					  0.1, 1.0, 0));
  ss->width = GTK_SPIN_BUTTON(gtk_spin_button_new(adj, 1.0, 2));
  gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(ss->width), TRUE);
  gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(ss->width), TRUE);
  gtk_box_pack_start(GTK_BOX(ss), GTK_WIDGET(ss->width), FALSE, TRUE, 0);
  gtk_widget_show(GTK_WIDGET(ss->width));

  adj = GTK_ADJUSTMENT(gtk_adjustment_new(1.0, 0.01, 10,
					  0.1, 1.0, 0));
  ss->height = GTK_SPIN_BUTTON(gtk_spin_button_new(adj, 1.0, 2));
  gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(ss->height), TRUE);
  gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(ss->height), TRUE);
  gtk_box_pack_start(GTK_BOX(ss), GTK_WIDGET(ss->height), FALSE, TRUE, 0);
  gtk_widget_show(GTK_WIDGET(ss->height));

  /* Replace label with images */
  /* should make sure they're both unallocated when the widget dies. 
  * That should happen in the "destroy" handler, where both should
  * be unref'd */
  ss->aspect_locked = 
    GTK_TOGGLE_BUTTON(dia_toggle_button_new_with_icons
		      (dia_unbroken_chain_icon,
		       dia_broken_chain_icon));

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
}

GtkType
dia_size_selector_get_type (void)
{
  static GtkType dss_type = 0;

  if (!dss_type) {
    static const GtkTypeInfo dss_info = {
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
  /*
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ss->aspect_locked),
			       fabs(width - height) < 0.000001);
  */
  dia_size_selector_set_ratio(ss, width, height);
}

void
dia_size_selector_set_locked(DiaSizeSelector *ss, gboolean locked)
{
  if (!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ss->aspect_locked))
      && locked) {
    dia_size_selector_set_ratio(ss, 
				gtk_spin_button_get_value(GTK_SPIN_BUTTON(ss->width)),
				gtk_spin_button_get_value(GTK_SPIN_BUTTON(ss->height)));
  }
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ss->aspect_locked), locked);
}

gboolean
dia_size_selector_get_size(DiaSizeSelector *ss, real *width, real *height)
{
  *width = gtk_spin_button_get_value(GTK_SPIN_BUTTON(ss->width));
  *height = gtk_spin_button_get_value(GTK_SPIN_BUTTON(ss->height));
  return gtk_toggle_button_get_active(ss->aspect_locked);
}

/************* DiaFontSelector: ***************/

/* Should these structs be in widgets.h instead? : 
 * _no_ they should not, noone is supposed to mess with the internals, ever heard of information hiding? 
 */

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

enum {
    DFONTSEL_VALUE_CHANGED,
    DFONTSEL_LAST_SIGNAL
};

static guint dfontsel_signals[DFONTSEL_LAST_SIGNAL] = { 0 };


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

static void dia_font_selector_fontmenu_callback(DiaDynamicMenu *ddm,
						gpointer data);
static void dia_font_selector_stylemenu_callback(GtkMenu *menu,
						gpointer data);
static void dia_font_selector_set_styles(DiaFontSelector *fs,
					 const gchar *name,
					 DiaFontStyle dia_style);
static void dia_font_selector_set_style_menu(DiaFontSelector *fs,
					     PangoFontFamily *pff,
					     DiaFontStyle dia_style);

static void
dia_font_selector_class_init (DiaFontSelectorClass *class)
{
  dfontsel_signals[DFONTSEL_VALUE_CHANGED]
      = g_signal_new("value_changed",
		     G_TYPE_FROM_CLASS(class),
		     G_SIGNAL_RUN_FIRST,
		     0, NULL, NULL,
		     g_cclosure_marshal_VOID__VOID,
		     G_TYPE_NONE, 0);
}

static int
dia_font_selector_sort_fonts(const void *p1, const void *p2)
{
  const gchar *n1 = pango_font_family_get_name(PANGO_FONT_FAMILY(*(void**)p1));
  const gchar *n2 = pango_font_family_get_name(PANGO_FONT_FAMILY(*(void**)p2));
  return g_ascii_strcasecmp(n1, n2);
}

static gchar*
replace_ampersands(gchar* string)
{
  gchar** pieces = g_strsplit(string, "&", -1);
  gchar* escaped = g_strjoinv("&amp;", pieces);
  g_strfreev(pieces);
  return escaped;
}

static GtkWidget *
dia_font_selector_create_string_item(DiaDynamicMenu *ddm, gchar *string)
{
  GtkWidget *item = gtk_menu_item_new_with_label(string);
  if (strchr(string, '&')) {
    gchar *escaped = replace_ampersands(string);
    gchar *label = g_strdup_printf("<span face=\"%s,sans\" size=\"medium\">%s</span>",
				   escaped, escaped);
    gtk_label_set_markup(GTK_LABEL(gtk_bin_get_child(GTK_BIN(item))), label);
    g_free(label);
    g_free(escaped);
  } else {
    gchar *label = g_strdup_printf("<span face=\"%s,sans\" size=\"medium\">%s</span>",
				   string, string);
    gtk_label_set_markup(GTK_LABEL(gtk_bin_get_child(GTK_BIN(item))), label);
    g_free(label);
  }
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
			      g_strdup(pango_font_family_get_name(families[i])));
  }
  g_free (families);
  
  fs->font_omenu =
      GTK_OPTION_MENU(dia_dynamic_menu_new_listbased(dia_font_selector_create_string_item,
				     fs, _("Other fonts"),
				     fontnames, "font-menu"));
  g_signal_connect(DIA_DYNAMIC_MENU(fs->font_omenu), "value-changed",
		   G_CALLBACK(dia_font_selector_fontmenu_callback), fs);
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
  /* No callback needed since fs->style_menu keeps getting replaced. */
  fs->style_menu = GTK_MENU(menu);
  gtk_option_menu_set_menu (GTK_OPTION_MENU (fs->style_omenu), menu);

  gtk_widget_show(menu);
  gtk_widget_show(omenu);

  gtk_box_pack_start(GTK_BOX(fs), GTK_WIDGET(fs->font_omenu), TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(fs), GTK_WIDGET(fs->style_omenu), TRUE, TRUE, 0);
}

GtkType
dia_font_selector_get_type        (void)
{
  static GtkType dfs_type = 0;

  if (!dfs_type) {
    static const GtkTypeInfo dfs_info = {
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
    if (!(g_ascii_strcasecmp(pango_font_family_get_name(families[i]), fontname))) {
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
dia_font_selector_fontmenu_callback(DiaDynamicMenu *ddm, gpointer data) 
{
  DiaFontSelector *fs = DIAFONTSELECTOR(data);
  char *fontname = dia_dynamic_menu_get_entry(ddm);
  dia_font_selector_set_styles(fs, fontname, -1);
  g_signal_emit(GTK_OBJECT(fs),
		dfontsel_signals[DFONTSEL_VALUE_CHANGED], 0);
  g_free(fontname);
}

static void
dia_font_selector_stylemenu_callback(GtkMenu *menu, gpointer data)
{
  DiaFontSelector *fs = DIAFONTSELECTOR(data);
  g_signal_emit(GTK_OBJECT(fs),
		dfontsel_signals[DFONTSEL_VALUE_CHANGED], 0);
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
  int select = 0;
  PangoFontFace **faces = NULL;
  guint nfaces = 0;
  guint i=0;
  GtkWidget *menu = NULL;
  long stylebits = 0;
  int menu_item_nr = 0;
  GSList *group = NULL;

  menu = gtk_menu_new ();
  g_signal_connect(menu, "selection-done",
		   G_CALLBACK(dia_font_selector_stylemenu_callback), fs);
  
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

  if (stylebits == 0) {
    g_warning ("'%s' has no style!", 
               pango_font_family_get_name (pff) ? pango_font_family_get_name (pff) : "(null font)");
  }

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
    g_object_set_data(G_OBJECT(menuitem), "user_data", GINT_TO_POINTER(i));
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
			     const gchar *name, DiaFontStyle dia_style)
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
  const gchar *fontname = dia_font_get_family(font);
  /* side effect: adds fontname to presistence list */
  dia_dynamic_menu_select_entry(DIA_DYNAMIC_MENU(fs->font_omenu), fontname);
  g_signal_emit(GTK_OBJECT(fs),
		dfontsel_signals[DFONTSEL_VALUE_CHANGED], 0);
  dia_font_selector_set_styles(fs, fontname, dia_font_get_style (font));
}

DiaFont *
dia_font_selector_get_font(DiaFontSelector *fs)
{
  GtkWidget *menuitem;
  char *fontname;
  DiaFontStyle style;
  DiaFont *font;

  fontname = dia_dynamic_menu_get_entry(DIA_DYNAMIC_MENU(fs->font_omenu));
  menuitem = gtk_menu_get_active(fs->style_menu);
  if (!menuitem) /* FIXME: should not happen ??? (but does if we don't have added a style) */
    style = 0;
  else
    style = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(menuitem), "user_data"));
  font = dia_font_new(fontname, style, 1.0);
  g_free(fontname);
  return font;
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
  g_object_set_data(G_OBJECT(menuitem), "user_data", GINT_TO_POINTER(ALIGN_LEFT));
  group = gtk_radio_menu_item_get_group (GTK_RADIO_MENU_ITEM (menuitem));
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
  gtk_widget_show (menuitem);

  menuitem = gtk_radio_menu_item_new_with_label (group, _("Center"));
  g_object_set_data(G_OBJECT(menuitem), "user_data", GINT_TO_POINTER(ALIGN_CENTER));
  group = gtk_radio_menu_item_get_group (GTK_RADIO_MENU_ITEM (menuitem));
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
  gtk_widget_show (menuitem);

  menuitem = gtk_radio_menu_item_new_with_label (group, _("Right"));
  g_object_set_data(G_OBJECT(menuitem), "user_data", GINT_TO_POINTER(ALIGN_RIGHT));
  group = gtk_radio_menu_item_get_group (GTK_RADIO_MENU_ITEM (menuitem));
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
    static const GtkTypeInfo dfs_info = {
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
  align = g_object_get_data(G_OBJECT(menuitem), "user_item");

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

enum {
    DLS_VALUE_CHANGED,
    DLS_LAST_SIGNAL
};

static guint dls_signals[DLS_LAST_SIGNAL] = { 0 };

static void
dia_line_style_selector_class_init (DiaLineStyleSelectorClass *class)
{
  dls_signals[DLS_VALUE_CHANGED]
      = g_signal_new("value-changed",
		     G_TYPE_FROM_CLASS(class),
		     G_SIGNAL_RUN_FIRST,
		     0, NULL, NULL,
		     g_cclosure_marshal_VOID__VOID,
		     G_TYPE_NONE, 0);
}

static void
set_linestyle_sensitivity(DiaLineStyleSelector *fs)
{
  int state;
  GtkWidget *menuitem;
  if (!fs->linestyle_menu) return;
  menuitem = gtk_menu_get_active(fs->linestyle_menu);
  state = (GPOINTER_TO_INT(g_object_get_data(G_OBJECT(menuitem), "user_data"))
	   != LINESTYLE_SOLID);

  gtk_widget_set_sensitive(GTK_WIDGET(fs->lengthlabel), state);
  gtk_widget_set_sensitive(GTK_WIDGET(fs->dashlength), state);
}

static void
linestyle_type_change_callback(GtkMenu *menu, gpointer data)
{
  set_linestyle_sensitivity(DIALINESTYLESELECTOR(data));
  g_signal_emit(DIALINESTYLESELECTOR(data),
		dls_signals[DLS_VALUE_CHANGED], 0);
}

static void
linestyle_dashlength_change_callback(GtkSpinButton *sb, gpointer data)
{
  g_signal_emit(DIALINESTYLESELECTOR(data),
		dls_signals[DLS_VALUE_CHANGED], 0);
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
    g_object_set_data(G_OBJECT(menuitem), "user_data", GINT_TO_POINTER(i));
    ln = dia_line_preview_new(i);
    gtk_container_add(GTK_CONTAINER(menuitem), ln);
    gtk_widget_show(ln);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
    gtk_widget_show(menuitem);
  }
#if 0
  menuitem = gtk_radio_menu_item_new_with_label (group, _("Solid"));
  g_object_set_data(G_OBJECT(menuitem), "user_data", GINT_TO_POINTER(LINESTYLE_SOLID));
  group = gtk_radio_menu_item_get_group (GTK_RADIO_MENU_ITEM (menuitem));
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
  gtk_widget_show (menuitem);

  menuitem = gtk_radio_menu_item_new_with_label (group, _("Dashed"));
  g_object_set_data(G_OBJECT(menuitem), "user_data", GINT_TO_POINTER(LINESTYLE_DASHED));
  group = gtk_radio_menu_item_get_group (GTK_RADIO_MENU_ITEM (menuitem));
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
  gtk_widget_show (menuitem);

  menuitem = gtk_radio_menu_item_new_with_label (group, _("Dash-Dot"));
  g_object_set_data(G_OBJECT(menuitem), "user_data", GINT_TO_POINTER(LINESTYLE_DASH_DOT));
  group = gtk_radio_menu_item_get_group (GTK_RADIO_MENU_ITEM (menuitem));
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
  gtk_widget_show (menuitem);

  menuitem = gtk_radio_menu_item_new_with_label (group, _("Dash-Dot-Dot"));
  g_object_set_data(G_OBJECT(menuitem), "user_data", GINT_TO_POINTER(LINESTYLE_DASH_DOT_DOT));
  group = gtk_radio_menu_item_get_group (GTK_RADIO_MENU_ITEM (menuitem));
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
  gtk_widget_show (menuitem);
  
  menuitem = gtk_radio_menu_item_new_with_label (group, _("Dotted"));
  g_object_set_data(G_OBJECT(menuitem), "user_data", GINT_TO_POINTER(LINESTYLE_DOTTED));
  group = gtk_radio_menu_item_get_group (GTK_RADIO_MENU_ITEM (menuitem));
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
  gtk_widget_show (menuitem);
#endif
  
  gtk_menu_set_active(GTK_MENU (menu), DEFAULT_LINESTYLE);
  gtk_option_menu_set_menu (GTK_OPTION_MENU (fs->omenu), menu);
  g_signal_connect(GTK_OBJECT(menu), "selection-done", 
		   G_CALLBACK(linestyle_type_change_callback), fs);
 
  gtk_box_pack_start(GTK_BOX(fs), GTK_WIDGET(fs->omenu), FALSE, TRUE, 0);
  gtk_widget_show(GTK_WIDGET(fs->omenu));

  box = gtk_hbox_new(FALSE,0);
  /*  fs->sizebox = GTK_HBOX(box); */

  label = gtk_label_new(_("Dash length: "));
  fs->lengthlabel = GTK_LABEL(label);
  gtk_box_pack_start(GTK_BOX(box), label, TRUE, TRUE, 0);
  gtk_widget_show(label);

  adj = (GtkAdjustment *)gtk_adjustment_new(0.1, 0.00, 10.0, 0.1, 1.0, 0);
  length = gtk_spin_button_new(adj, DEFAULT_LINESTYLE_DASHLEN, 2);
  gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(length), TRUE);
  gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(length), TRUE);
  fs->dashlength = GTK_SPIN_BUTTON(length);
  gtk_box_pack_start(GTK_BOX (box), length, TRUE, TRUE, 0);
  gtk_widget_show (length);

  g_signal_connect(GTK_OBJECT(length), "changed", 
		   G_CALLBACK(linestyle_dashlength_change_callback), fs);

  set_linestyle_sensitivity(fs);
  gtk_box_pack_start(GTK_BOX(fs), box, TRUE, TRUE, 0);
  gtk_widget_show(box);
  
}

GtkType
dia_line_style_selector_get_type        (void)
{
  static GtkType dfs_type = 0;

  if (!dfs_type) {
    static const GtkTypeInfo dfs_info = {
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
  align = g_object_get_data(G_OBJECT(menuitem), "user_data");
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
    gchar *label = g_strdup_printf("<span foreground=\"black\" background=\"%s\">%s</span>", string, string);
    gtk_label_set_markup(GTK_LABEL(gtk_bin_get_child(GTK_BIN(item))), label);
    g_free(label);
  } else {
    gchar *label = g_strdup_printf("<span foreground=\"white\" background=\"%s\">%s</span>", string, string);
    gtk_label_set_markup(GTK_LABEL(gtk_bin_get_child(GTK_BIN(item))), label);
    g_free(label);
  }
  
  return item;
}

static void
dia_color_selector_more_ok(GtkWidget *ok, gpointer userdata)
{
  DiaDynamicMenu *ddm = g_object_get_data(G_OBJECT(userdata), "ddm");
  GtkWidget *colorsel = GTK_WIDGET(userdata);
  GdkColor gcol;
  gchar *entry;

  gtk_color_selection_get_current_color(
	GTK_COLOR_SELECTION(
	    GTK_COLOR_SELECTION_DIALOG(colorsel)->colorsel),
	&gcol);

  entry = g_strdup_printf("#%02X%02X%02X", gcol.red/256, gcol.green/256, gcol.blue/256);
  dia_dynamic_menu_select_entry(ddm, entry);
  g_free(entry);

  gtk_widget_destroy(colorsel);
}

static void
dia_color_selector_more_callback(GtkWidget *widget, gpointer userdata)
{
  GtkColorSelectionDialog *dialog = GTK_COLOR_SELECTION_DIALOG (gtk_color_selection_dialog_new(_("Select color")));
  DiaDynamicMenu *ddm = DIA_DYNAMIC_MENU(userdata);
  GtkColorSelection *colorsel = GTK_COLOR_SELECTION(dialog->colorsel);
  GString *palette = g_string_new ("");

  gchar *old_color = dia_dynamic_menu_get_entry(ddm);
  GtkWidget *parent;

  /* Force history to the old place */
  dia_dynamic_menu_select_entry(ddm, old_color);

  /* avoid crashing if the property dialog is closed before the color dialog */
  parent = widget;
  while (parent && !GTK_IS_WINDOW (parent))
    parent = gtk_widget_get_parent (parent);
  if (parent) {
    gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW(parent));
    gtk_window_set_destroy_with_parent (GTK_WINDOW (dialog), TRUE);
  }

  if (ddm->default_entries != NULL) {
    GList *tmplist;
    int index = 0;
    gboolean advance = TRUE;

    for (tmplist = ddm->default_entries; 
         tmplist != NULL || advance; 
         tmplist = g_list_next(tmplist)) {
      const gchar* spec;
      GdkColor color;

      /* handle both lists */
      if (!tmplist && advance) {
        advance = FALSE;
        tmplist = persistent_list_get_glist(ddm->persistent_name);
        if (!tmplist)
          break;
      }
      spec = (gchar *)tmplist->data;

      gdk_color_parse (spec, &color);
#if 0
      /* the easy way if the Gtk Team would decide to make it public */
      gtk_color_selection_set_palette_color (colorsel, index, &color);
#else
      g_string_append (palette, spec);
      g_string_append (palette, ":");
#endif
      if (0 == strcmp (spec, old_color)) {
        gtk_color_selection_set_previous_color (colorsel, &color);
        gtk_color_selection_set_current_color (colorsel, &color);
      }
      index++;
    }
  }

  g_object_set (gtk_widget_get_settings (GTK_WIDGET (colorsel)), "gtk-color-palette", palette->str, NULL);
  gtk_color_selection_set_has_palette (colorsel, TRUE);
  g_string_free (palette, TRUE);
  g_free(old_color);
  
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
  GtkWidget *otheritem = gtk_menu_item_new_with_label(_("More colors…"));
  GtkWidget *ddm = dia_dynamic_menu_new(dia_color_selector_create_string_item,
					NULL,
					GTK_MENU_ITEM(otheritem),
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
		   G_CALLBACK(dia_color_selector_more_callback), ddm);
  gtk_widget_show(otheritem);
  return ddm;
}


void
dia_color_selector_get_color(GtkWidget *widget, Color *color)
{
  gchar *entry = dia_dynamic_menu_get_entry(DIA_DYNAMIC_MENU(widget));
  gint r, g, b;

  sscanf(entry, "#%2x%2x%2x", &r, &g, &b);
  g_free(entry);
  color->red = r / 255.0;
  color->green = g / 255.0;
  color->blue = b / 255.0;
}

void
dia_color_selector_set_color (GtkWidget *widget,
			      const Color *color)
{
  gint red, green, blue;
  gchar *entry;
  red = color->red * 255;
  green = color->green * 255;
  blue = color->blue * 255;
  if (color->red > 1.0 || color->green > 1.0 || color->blue > 1.0) {
    printf("Color out of range: r %f, g %f, b %f\n",
	   color->red, color->green, color->blue);
    red = MIN(red, 255);
    green = MIN(green, 255);
    blue = MIN(blue, 255);
  }
  entry = g_strdup_printf("#%02X%02X%02X", red, green, blue);
  dia_dynamic_menu_select_entry(DIA_DYNAMIC_MENU(widget), entry);
  g_free (entry);
}


/************* DiaArrowSelector: ***************/

/* FIXME: Should these structs be in widgets.h instead? */
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

enum {
    DAS_VALUE_CHANGED,
    DAS_LAST_SIGNAL
};

static guint das_signals[DAS_LAST_SIGNAL] = {0};

static void
dia_arrow_selector_class_init (DiaArrowSelectorClass *class)
{
  das_signals[DAS_VALUE_CHANGED]
      = g_signal_new("value_changed",
		     G_TYPE_FROM_CLASS(class),
		     G_SIGNAL_RUN_FIRST,
		     0, NULL, NULL,
		     g_cclosure_marshal_VOID__VOID,
		     G_TYPE_NONE, 0);
}
  
static void
set_size_sensitivity(DiaArrowSelector *as)
{
  int state;
  gchar *entryname = dia_dynamic_menu_get_entry(DIA_DYNAMIC_MENU(as->omenu));

  state = (entryname != NULL) && (0 != g_ascii_strcasecmp(entryname, "None"));
  g_free(entryname);

  gtk_widget_set_sensitive(GTK_WIDGET(as->sizelabel), state);
  gtk_widget_set_sensitive(GTK_WIDGET(as->size), state);
}

static void
arrow_type_change_callback(DiaDynamicMenu *ddm, gpointer userdata)
{
  set_size_sensitivity(DIA_ARROW_SELECTOR(userdata));
  g_signal_emit(DIA_ARROW_SELECTOR(userdata),
		das_signals[DAS_VALUE_CHANGED], 0);
}

static void
arrow_size_change_callback(DiaSizeSelector *size, gpointer userdata)
{
  g_signal_emit(DIA_ARROW_SELECTOR(userdata),
		das_signals[DAS_VALUE_CHANGED], 0);
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
					 as,
					 _("More arrows"),
					 arrow_names,
					 "arrow-menu");
  dia_dynamic_menu_add_default_entry(DIA_DYNAMIC_MENU(omenu), "None");
  dia_dynamic_menu_add_default_entry(DIA_DYNAMIC_MENU(omenu), "Lines");
  dia_dynamic_menu_add_default_entry(DIA_DYNAMIC_MENU(omenu), "Filled Concave");
  as->omenu = GTK_OPTION_MENU(omenu);
  gtk_box_pack_start(GTK_BOX(as), omenu, FALSE, TRUE, 0);
  gtk_widget_show(omenu);

  g_signal_connect(DIA_DYNAMIC_MENU(omenu),
		   "value-changed", G_CALLBACK(arrow_type_change_callback),
		   as);

  box = gtk_hbox_new(FALSE,0);
  as->sizebox = GTK_HBOX(box);

  label = gtk_label_new(_("Size: "));
  as->sizelabel = GTK_LABEL(label);
  gtk_box_pack_start(GTK_BOX(box), label, TRUE, TRUE, 0);
  gtk_widget_show(label);

  size = dia_size_selector_new(0.0, 0.0);
  as->size = DIA_SIZE_SELECTOR(size);
  gtk_box_pack_start(GTK_BOX(box), size, TRUE, TRUE, 0);
  gtk_widget_show(size);
  g_signal_connect(size, "value-changed",
		   G_CALLBACK(arrow_size_change_callback), as);

  set_size_sensitivity(as);
  gtk_box_pack_start(GTK_BOX(as), box, TRUE, TRUE, 0);

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
  gchar *arrowname = dia_dynamic_menu_get_entry(DIA_DYNAMIC_MENU(as->omenu));

  at.type = arrow_type_from_name(arrowname);
  g_free(arrowname);
  dia_size_selector_get_size(as->size, &at.width, &at.length);
  return at;
}

void
dia_arrow_selector_set_arrow (DiaArrowSelector *as,
			      Arrow arrow)
{
  dia_dynamic_menu_select_entry(DIA_DYNAMIC_MENU(as->omenu),
				arrow_get_name_from_type(arrow.type));
  set_size_sensitivity(as);
  dia_size_selector_set_size(DIA_SIZE_SELECTOR(as->size), arrow.width, arrow.length);
}

/************* DiaFileSelector: ***************/
struct _DiaFileSelector
{
  GtkHBox hbox;
  GtkEntry *entry;
  GtkButton *browse;
  GtkWidget *dialog;
  gchar *sys_filename;
  gchar *pattern; /* for supported formats */
};

struct _DiaFileSelectorClass
{
  GtkHBoxClass parent_class;
};

enum {
    DFILE_VALUE_CHANGED,
    DFILE_LAST_SIGNAL
};

static guint dfile_signals[DFILE_LAST_SIGNAL] = { 0 };

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
  if (fs->pattern) {
    g_free (fs->pattern);
    fs->pattern = NULL;
  }

  (* GTK_WIDGET_CLASS (gtk_type_class(gtk_hbox_get_type ()))->unrealize) (widget);
}

static void
dia_file_selector_class_init (DiaFileSelectorClass *class)
{
  GtkWidgetClass *widget_class;
  
  widget_class = (GtkWidgetClass*) class;
  widget_class->unrealize = dia_file_selector_unrealize;

  dfile_signals[DFILE_VALUE_CHANGED]
      = g_signal_new("value-changed",
		     G_TYPE_FROM_CLASS(class),
		     G_SIGNAL_RUN_FIRST,
		     0, NULL, NULL,
		     g_cclosure_marshal_VOID__VOID,
		     G_TYPE_NONE, 0);
}

static void
dia_file_selector_entry_changed(GtkEditable *editable
				, gpointer data)
{
  DiaFileSelector *fs = DIAFILESELECTOR(data);
  g_signal_emit(fs, dfile_signals[DFILE_VALUE_CHANGED], 0);
}

static void
file_open_response_callback(GtkWidget *dialog, 
                            gint       response, 
                            gpointer   user_data)
{
  DiaFileSelector *fs =
    DIAFILESELECTOR(g_object_get_data(G_OBJECT(dialog), "user_data"));

  if (response == GTK_RESPONSE_ACCEPT || response == GTK_RESPONSE_OK) { 
    gchar *utf8 = g_filename_to_utf8(gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog)), 
                            -1, NULL, NULL, NULL);
    gtk_entry_set_text(GTK_ENTRY(fs->entry), utf8);
    g_free(utf8);
  }
  gtk_widget_destroy(GTK_WIDGET(dialog));
}

static void
dia_file_selector_browse_pressed(GtkWidget *widget, gpointer data)
{
  GtkWidget *dialog;
  DiaFileSelector *fs = DIAFILESELECTOR(data);
  gchar *filename;
  GtkWidget *toplevel = gtk_widget_get_toplevel (widget);

  if (!GTK_WIDGET_TOPLEVEL(toplevel) || !GTK_WINDOW(toplevel))
    toplevel = NULL;

  if (fs->dialog == NULL) {
    GtkFileFilter *filter;
    
    dialog = fs->dialog = 
      gtk_file_chooser_dialog_new (_("Select image file"), toplevel ? GTK_WINDOW(toplevel) : NULL,
                                   GTK_FILE_CHOOSER_ACTION_OPEN,
                                   GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				   GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
				   NULL);
    gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_ACCEPT);
    g_signal_connect(GTK_OBJECT(dialog), "response",
		     G_CALLBACK(file_open_response_callback), NULL);     
    gtk_signal_connect (GTK_OBJECT (fs->dialog), "destroy",
			GTK_SIGNAL_FUNC (gtk_widget_destroyed),
			&fs->dialog);
    
    filter = gtk_file_filter_new ();
    gtk_file_filter_set_name (filter, _("Supported Formats"));
    if (fs->pattern)
      gtk_file_filter_add_pattern (filter, fs->pattern);
    else /* fallback */
      gtk_file_filter_add_pixbuf_formats (filter);
    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog), filter);

    filter = gtk_file_filter_new ();
    gtk_file_filter_set_name (filter, _("All Files"));
    gtk_file_filter_add_pattern (filter, "*");
    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog), filter);

    g_object_set_data(G_OBJECT(dialog), "user_data", fs);
  }

  filename = g_filename_from_utf8(gtk_entry_get_text(fs->entry), -1, NULL, NULL, NULL);
  /* selecting something in the filechooser officially sucks. See e.g. http://bugzilla.gnome.org/show_bug.cgi?id=307378 */
  if (g_path_is_absolute(filename))
    gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(fs->dialog), filename);

  g_free(filename);
  
  gtk_widget_show(GTK_WIDGET(fs->dialog));
}

static void
dia_file_selector_init (DiaFileSelector *fs)
{
  /* Here's where we set up the real thing */
  fs->dialog = NULL;
  fs->sys_filename = NULL;
  fs->pattern = NULL;

  fs->entry = GTK_ENTRY(gtk_entry_new());
  gtk_box_pack_start(GTK_BOX(fs), GTK_WIDGET(fs->entry), FALSE, TRUE, 0);
  g_signal_connect(GTK_OBJECT(fs->entry), "changed",
		   G_CALLBACK(dia_file_selector_entry_changed), fs);
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
    static const GtkTypeInfo dfs_info = {
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
dia_file_selector_set_extensions  (DiaFileSelector *fs, const gchar **exts)
{
  GString *pattern = g_string_new ("*.");
  int i = 0;

  g_free (fs->pattern);

  while (exts[i] != NULL) {
    if (i != 0)
      g_string_append (pattern, "|*.");
    g_string_append (pattern, exts[i]);
    ++i;
  }
  fs->pattern = pattern->str;
  g_string_free (pattern, FALSE);
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

static void dia_unit_spinner_init(DiaUnitSpinner *self);

GType
dia_unit_spinner_get_type(void)
{
  static GType us_type = 0;

  if (!us_type) {
    static const GTypeInfo us_info = {
      sizeof(DiaUnitSpinnerClass),
      NULL, /* base_init */
      NULL, /* base_finalize */
      NULL, /* class_init*/
      NULL, /* class_finalize */
      NULL, /* class_data */
      sizeof(DiaUnitSpinner),
      0,    /* n_preallocs */
      (GInstanceInitFunc) dia_unit_spinner_init,
    };
    us_type = g_type_register_static(GTK_TYPE_SPIN_BUTTON,
                                     "DiaUnitSpinner",
                                     &us_info, 0);
  }
  return us_type;
}


static void
dia_unit_spinner_init(DiaUnitSpinner *self)
{
  self->unit_num = DIA_UNIT_CENTIMETER;
}

/*
  Callback functions for the "input" and "output" signals emitted by
  GtkSpinButton. All the normal work is done by the spin button, we
  simply modify how the text in the GtkEntry is treated.
*/
static gboolean
dia_unit_spinner_input(DiaUnitSpinner *self, gdouble *value);
static gboolean dia_unit_spinner_output(DiaUnitSpinner *self);

GtkWidget *
dia_unit_spinner_new(GtkAdjustment *adjustment, DiaUnit adj_unit)
{
  DiaUnitSpinner *self;
  
  if (adjustment) {
    g_return_val_if_fail (GTK_IS_ADJUSTMENT (adjustment), NULL);
  }
  
  self = gtk_type_new(dia_unit_spinner_get_type());
  self->unit_num = adj_unit;
  
  gtk_spin_button_configure(GTK_SPIN_BUTTON(self),
                            adjustment, 0.0, units[adj_unit].digits);

  g_signal_connect(GTK_SPIN_BUTTON(self), "output",
                   G_CALLBACK(dia_unit_spinner_output),
                   NULL);
  g_signal_connect(GTK_SPIN_BUTTON(self), "input",
                   G_CALLBACK(dia_unit_spinner_input),
                   NULL);

  return GTK_WIDGET(self);
}

static gboolean
dia_unit_spinner_input(DiaUnitSpinner *self, gdouble *value)
{
  gfloat val, factor = 1.0;
  gchar *extra = NULL;

  val = g_strtod(gtk_entry_get_text(GTK_ENTRY(self)), &extra);

  /* get rid of extra white space after number */
  while (*extra && g_ascii_isspace(*extra)) extra++;
  if (*extra) {
    int i;

    for (i = 0; units[i].name != NULL; i++)
      if (!g_ascii_strcasecmp(units[i].unit, extra)) {
	factor = units[i].factor / units[self->unit_num].factor;
	break;
      }
  }
  /* convert to prefered units */
  val *= factor;

  /* Store value in the location provided by the signal emitter. */
  *value = val;

  /* Return true, so that the default input function is not invoked. */
  return TRUE;
}

static gboolean dia_unit_spinner_output(DiaUnitSpinner *self)
{
  char buf[256];

  GtkSpinButton *sbutton = GTK_SPIN_BUTTON(self);
  GtkAdjustment *adjustment = gtk_spin_button_get_adjustment(sbutton);

  g_snprintf(buf, sizeof(buf), "%0.*f %s",
             gtk_spin_button_get_digits(sbutton),
             gtk_adjustment_get_value(adjustment),
	      units[self->unit_num].unit);
  gtk_entry_set_text(GTK_ENTRY(self), buf);

  /* Return true, so that the default output function is not invoked. */
  return TRUE;
}

/** Set the value (in cm).
 * */
void
dia_unit_spinner_set_value(DiaUnitSpinner *self, gdouble val)
{
  GtkSpinButton *sbutton = GTK_SPIN_BUTTON(self);

  gtk_spin_button_set_value(sbutton,
                            val /
                            (units[self->unit_num].factor /
                             units[DIA_UNIT_CENTIMETER].factor));
}

/** Get the value (in cm) */
gdouble
dia_unit_spinner_get_value(DiaUnitSpinner *self)
{
  GtkSpinButton *sbutton = GTK_SPIN_BUTTON(self);

  return gtk_spin_button_get_value(sbutton) *
      (units[self->unit_num].factor / units[DIA_UNIT_CENTIMETER].factor);
}

GList *
get_units_name_list(void)
{
  int i;
  static GList *name_list = NULL;

  if (name_list == NULL) {
    for (i = 0; units[i].name != NULL; i++) {
      name_list = g_list_append(name_list, units[i].name);
    }
  }

  return name_list;
}

/* ************************ Dynamic menus ************************ */

static void dia_dynamic_menu_class_init(DiaDynamicMenuClass *class);
static void dia_dynamic_menu_init(DiaDynamicMenu *self);
static void dia_dynamic_menu_create_sublist(DiaDynamicMenu *ddm,
					    GList *items, 
					    DDMCreateItemFunc create);
static void dia_dynamic_menu_create_menu(DiaDynamicMenu *ddm);
static void dia_dynamic_menu_destroy(GtkObject *object);

enum {
    DDM_VALUE_CHANGED,
    DDM_LAST_SIGNAL
};

static guint ddm_signals[DDM_LAST_SIGNAL] = { 0 };

GtkType
dia_dynamic_menu_get_type(void)
{
  static GtkType us_type = 0;

  if (!us_type) {
    static const GtkTypeInfo us_info = {
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
  GtkObjectClass *object_class = (GtkObjectClass*)class;

  object_class->destroy = dia_dynamic_menu_destroy;
  
  ddm_signals[DDM_VALUE_CHANGED]
      = g_signal_new("value-changed",
		     G_TYPE_FROM_CLASS(class),
		     G_SIGNAL_RUN_FIRST,
		     0, NULL, NULL,
		     g_cclosure_marshal_VOID__VOID,
		     G_TYPE_NONE, 0);
}

static void
dia_dynamic_menu_init(DiaDynamicMenu *self)
{
}

void 
dia_dynamic_menu_destroy(GtkObject *object)
{
  DiaDynamicMenu *ddm = DIA_DYNAMIC_MENU(object);
  GtkObjectClass *parent_class = GTK_OBJECT_CLASS(g_type_class_peek_parent(GTK_OBJECT_GET_CLASS(object)));

  if (ddm->active)
    g_free(ddm->active);
  ddm->active = NULL;

  if (parent_class->destroy)
    (* parent_class->destroy) (object);
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
		     gpointer userdata,
		     GtkMenuItem *otheritem, gchar *persist)
{
  DiaDynamicMenu *ddm;

  g_assert(persist != NULL);

  ddm = DIA_DYNAMIC_MENU ( gtk_type_new (dia_dynamic_menu_get_type ()));
  
  ddm->create_func = create;
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
dia_dynamic_menu_select_entry(DiaDynamicMenu *ddm, const gchar *name)
{
  gint add_result = dia_dynamic_menu_add_entry(ddm, name);
  if (add_result == 0) {
      GList *tmp;
      int i = 0;
      for (tmp = ddm->default_entries; tmp != NULL;
	   tmp = g_list_next(tmp), i++) {
	if (!g_ascii_strcasecmp(tmp->data, name))
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
  
  g_free(ddm->active);
  ddm->active = g_strdup(name);
  g_signal_emit(GTK_OBJECT(ddm), ddm_signals[DDM_VALUE_CHANGED], 0);
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
  GtkWidget *item = gtk_menu_item_new_with_label(gettext(string));
  return item;
}

/** Utility function for dynamic menus that are entirely based on the
 * labels in the menu.
 */
GtkWidget *
dia_dynamic_menu_new_stringbased(GtkMenuItem *otheritem, 
				 gpointer userdata,
				 gchar *persist)
{
  GtkWidget *ddm = dia_dynamic_menu_new(dia_dynamic_menu_create_string_item,
					userdata,
					otheritem, persist);
  return ddm;
}

/** Utility function for dynamic menus that are based on a submenu with
 * many entries.  This is useful for allowing the user to get a smaller 
 * subset menu out of a set too large to be easily handled by a menu.
 */
GtkWidget *
dia_dynamic_menu_new_listbased(DDMCreateItemFunc create,
			       gpointer userdata,
			       gchar *other_label, GList *items, 
			       gchar *persist)
{
  GtkWidget *item = gtk_menu_item_new_with_label(other_label);
  GtkWidget *ddm = dia_dynamic_menu_new(create, userdata,
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
				     gpointer userdata,
				     gchar *persist)
{
  return dia_dynamic_menu_new_listbased(dia_dynamic_menu_create_string_item,
					userdata,
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
dia_dynamic_menu_add_default_entry(DiaDynamicMenu *ddm, const gchar *entry)
{
  ddm->default_entries = g_list_append(ddm->default_entries,
				       g_strdup(entry));

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
dia_dynamic_menu_add_entry(DiaDynamicMenu *ddm, const gchar *entry)
{
  GList *tmp;
  gboolean existed;

  for (tmp = ddm->default_entries; tmp != NULL; tmp = g_list_next(tmp)) {
    if (!g_ascii_strcasecmp(tmp->data, entry))
      return 0;
  }
  existed = persistent_list_add(ddm->persistent_name, entry);

  dia_dynamic_menu_create_menu(ddm);
  
  return existed?1:2;
}

/** Returns the currently selected entry. 
 * @returns The name of the entry that is currently selected.  This
 * string should be freed by the caller. */
gchar *
dia_dynamic_menu_get_entry(DiaDynamicMenu *ddm)
{
  return g_strdup(ddm->active);
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
		       G_CALLBACK(dia_dynamic_menu_activate), ddm);
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
		     G_CALLBACK(dia_dynamic_menu_activate), ddm);
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
  g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(dia_dynamic_menu_reset), ddm);
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
  gchar *active = dia_dynamic_menu_get_entry(ddm);
  g_list_foreach(plist->glist, (GFunc)g_free, NULL);
  g_list_free(plist->glist);
  plist->glist = NULL;
  dia_dynamic_menu_create_menu(ddm);
  if (active)
    dia_dynamic_menu_select_entry(ddm, active);
  g_free(active);
}

/** Set the maximum number of non-default entries.
 * If more than this number of entries are added, the least recently
 * selected ones are removed. */
void
dia_dynamic_menu_set_max_entries(DiaDynamicMenu *ddm, gint max)
{
}


/* ************************ Misc. util functions ************************ */
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

  if (images->on)
    g_object_unref(images->on);
  images->on = NULL;
  if (images->off)
    g_object_unref(images->off);
  images->off = NULL;
  if (images)
    g_free(images);
  images = NULL;
}

/** Create a toggle button given two image widgets for on and off */
static GtkWidget *
dia_toggle_button_new(GtkWidget *on_widget, GtkWidget *off_widget)
{
  GtkWidget *button = gtk_toggle_button_new();
  GtkRcStyle *rcstyle;
  struct image_pair *images;

  images = g_new(struct image_pair, 1);
  /* Since these may not be added at any point, make sure to
   * sink them. */
  images->on = on_widget;
  g_object_ref_sink(images->on);
  gtk_widget_show(images->on);

  images->off = off_widget;
  g_object_ref_sink(images->off);
  gtk_widget_show(images->off);

  /* Make border as small as possible */
  gtk_misc_set_padding(GTK_MISC(images->on), 0, 0);
  gtk_misc_set_padding(GTK_MISC(images->off), 0, 0);
#if GTK_CHECK_VERSION(2,18,0)
  gtk_widget_set_can_focus (GTK_WIDGET (button), FALSE);
#else
  GTK_WIDGET_UNSET_FLAGS(button, GTK_CAN_FOCUS);
#endif
#if GTK_CHECK_VERSION(2,18,0)
  gtk_widget_set_can_default (GTK_WIDGET (button), FALSE);
#else
  GTK_WIDGET_UNSET_FLAGS(button, GTK_CAN_DEFAULT);
#endif

  rcstyle = gtk_rc_style_new ();  
  rcstyle->xthickness = rcstyle->ythickness = 0;       
  gtk_widget_modify_style (button, rcstyle);
  g_object_unref (rcstyle);

  gtk_button_set_relief(GTK_BUTTON(button), GTK_RELIEF_NONE);
  /*  gtk_button_set_focus_on_click(GTK_BUTTON(button), FALSE);*/
  gtk_container_set_border_width(GTK_CONTAINER(button), 0);

  gtk_container_add(GTK_CONTAINER(button), images->off);

  g_signal_connect(G_OBJECT(button), "toggled", 
		   G_CALLBACK(dia_toggle_button_swap_images), images);
  g_signal_connect(G_OBJECT(button), "destroy",
		   G_CALLBACK(dia_toggle_button_destroy), images);

  return button;
}

/** Create a toggle button with two icons (created with gdk-pixbuf-csource,
 * for instance).  The icons represent on and off.
 */
GtkWidget *
dia_toggle_button_new_with_icons(const guint8 *on_icon,
				 const guint8 *off_icon)
{
  GdkPixbuf *p1, *p2;

  p1 = gdk_pixbuf_new_from_inline(-1, on_icon, FALSE, NULL);
  p2 = gdk_pixbuf_new_from_inline(-1, off_icon, FALSE, NULL);

  return dia_toggle_button_new(gtk_image_new_from_pixbuf(p1),
			       gtk_image_new_from_pixbuf(p2));
}


