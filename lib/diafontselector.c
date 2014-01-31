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

#include <stdlib.h>
#include <string.h>
#include "intl.h"

#undef GTK_DISABLE_DEPRECATED /* GtkOptionMenu, ... */
#include <gtk/gtk.h>
#include "diafontselector.h"
#include "diadynamicmenu.h"
#include "font.h"

/************* DiaFontSelector: ***************/

/* Should these structs be in widgets.h instead? : 
 * _no_ they should not, noone is supposed to mess with the internals, ever heard of information hiding? 
 */

struct _DiaFontSelector
{
  GtkHBox hbox;

  GtkWidget *font_omenu;
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
  
  fs->font_omenu = dia_dynamic_menu_new_listbased(dia_font_selector_create_string_item,
				     fs, _("Other fonts"),
				     fontnames, "font-menu");
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

GType
dia_font_selector_get_type        (void)
{
  static GType dfs_type = 0;

  if (!dfs_type) {
    static const GTypeInfo dfs_info = {
      sizeof (DiaFontSelectorClass),
      (GBaseInitFunc) NULL,
      (GBaseFinalizeFunc) NULL,
      (GClassInitFunc) dia_font_selector_class_init,
      NULL,           /* class_finalize */
      NULL,           /* class_data */
      sizeof (DiaFontSelector),
      0,              /* n_preallocs */
      (GInstanceInitFunc) dia_font_selector_init,
    };
    
    dfs_type = g_type_register_static (gtk_hbox_get_type (), 
				       "DiaFontSelector",
				       &dfs_info, 0);
  }
  
  return dfs_type;
}

GtkWidget *
dia_font_selector_new ()
{
  return GTK_WIDGET ( g_object_new (dia_font_selector_get_type (), NULL));
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
  g_signal_emit(G_OBJECT(fs),
		dfontsel_signals[DFONTSEL_VALUE_CHANGED], 0);
  g_free(fontname);
}

static void
dia_font_selector_stylemenu_callback(GtkMenu *menu, gpointer data)
{
  DiaFontSelector *fs = DIAFONTSELECTOR(data);
  g_signal_emit(G_OBJECT(fs),
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
  int nfaces = 0;
  int i=0;
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
  g_signal_emit(G_OBJECT(fs),
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
 
