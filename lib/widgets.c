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
#undef GTK_DISABLE_DEPRECATED /* GtkOptinMenu, ... */
#include "widgets.h"
#include "message.h"
#include "dia_dirs.h"
#include "arrows.h"
#include "diaarrowchooser.h"
#include "dialinechooser.h"

#include "pixmaps/broken-chain.xpm"
#include "pixmaps/unbroken-chain.xpm"

#include <stdlib.h>
#include <glib.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <pango/pango.h>
#include <stdio.h>
#include <time.h>

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
dia_size_selector_lock_pressed(GtkWidget *widget, gpointer data)
{
  DiaSizeSelector *ss = DIA_SIZE_SELECTOR(data);
  
  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ss->aspect_locked))) {
    dia_size_selector_ratio_callback(ss->last_adjusted, (gpointer)ss);
  }
}

/* Possible args:  Init width, init height, digits */

static void
dia_size_selector_init (DiaSizeSelector *ss)
{
  /* Here's where we set up the real thing */
  GdkPixmap *pixmap;
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
  ss->aspect_locked = GTK_TOGGLE_BUTTON(gtk_toggle_button_new_with_label("X"));
  
  /*
  pixmap = gdk_pixmap_create_from_data(drawable, unlinked_xpm, 9, 24, 24, gdk_color_black, gdk_color_white);
  bitmap = gdk_pitmap_create_from_data(drawable, unlinked_xpm, 9, 24);
  image = gtk_image_new_from_pixmap(pixmap, bitmap);
  g_object_unref(pixmap);
  g_object_unref(bitmap);
  gtk_container_add(GTK_CONTAINER(ss->aspect_locked), image);
  */

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

guint
dia_size_selector_get_type (void)
{
  static guint dss_type = 0;

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
  GtkMenu *font_menu;
  GtkMenu *style_menu;
  gint old_font; /* The menu item # that was previously selected, for
		    when the Other Fonts dialog is cancelled */


  gchar *textsample;
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

typedef struct {
  gchar *name;
  PangoFontFamily *family;
  time_t last_select;
  int entry_nr;
} FontSelectorEntry;

/* Hash table from font name to FontSelectorEntry */
static GHashTable *font_hash_table = NULL;
static GList *menu_entry_list = NULL;

static void dia_font_selector_dialog_callback(GtkWidget *widget, int id, gpointer data);
static void dia_font_selector_menu_callback(GtkWidget *button, gpointer data);
static void dia_font_selector_set_styles(DiaFontSelector *fs,
			     FontSelectorEntry *fse,
			     DiaFontStyle dia_style);

static FontSelectorEntry *
dia_font_selector_add_font(const char *lowername, const gchar *fontname,
			   gboolean is_other_font) {
  FontSelectorEntry *fse;
  fse = g_new(FontSelectorEntry, 1);
  /* Not using LRU yet, so each line is but a name */
  fse->name = fontname;
  fse->family = NULL;
  fse->last_select = time(0);
  fse->entry_nr = g_list_length(menu_entry_list)+4; /* Skip first entries */
  g_hash_table_insert(font_hash_table, g_strdup(lowername), fse);
  if (is_other_font) {
    menu_entry_list = g_list_append(menu_entry_list, fontname);
  } else {
    if (!g_strcasecmp(fontname, "sans")) fse->entry_nr = 0;
    if (!g_strcasecmp(fontname, "serif")) fse->entry_nr = 1;
    if (!g_strcasecmp(fontname, "monospace")) fse->entry_nr = 2;
  }
  return fse;
}

static
gboolean strcase_equal(gconstpointer s1, gconstpointer s2)
{
  return !(g_strcasecmp((char *)s1, (char *)s2));
}

static void
dia_font_selector_read_persistence_file() {
  gchar *file_contents;
  gchar *persistence_name;
  GError *error = NULL;

  font_hash_table = g_hash_table_new(g_str_hash, strcase_equal);

  dia_font_selector_add_font("sans", "Sans", FALSE);
  dia_font_selector_add_font("serif", "Serif", FALSE);
  dia_font_selector_add_font("monospace", "Monospace", FALSE);

  persistence_name = dia_config_filename("font_menu");
  
  if (g_file_test(persistence_name, G_FILE_TEST_EXISTS) &&
      g_file_get_contents(persistence_name, &file_contents, NULL, &error)) {
    /* Should really use some macro for linebreak, but I can't find it */
    gchar **lines = g_strsplit(file_contents, "\n", -1); 
    int i;
    for (i = 0; lines[i] != NULL; i++) {
      gchar *lowername;
      if (strlen(lines[i]) == 0) continue;
      lowername = g_utf8_strdown(lines[i], -1);
      dia_font_selector_add_font(lowername, lines[i], TRUE);
      g_free(lowername);
    }
    g_free(file_contents);
  }
  if (error) g_error_free(error);
  g_free(persistence_name);
}

static void
dia_font_selector_write_persistence_file() {
  gchar *persistence_name;
  FILE *pfile;

  persistence_name = dia_config_filename("font_menu");
  if ((pfile = fopen(persistence_name, "wb")) != NULL) {
    GList *entry;
    for (entry = menu_entry_list; entry != NULL; entry = entry->next) {
      fputs((gchar *)entry->data, pfile);
      fputs("\n", pfile);
    }
    fclose(pfile);
  }
  g_free(persistence_name);
}

static GtkWidget *
dia_font_selector_add_menu_item(gchar *label, GSList **group, GtkWidget *menu) 
{
  GtkWidget *menuitem;
  menuitem = gtk_radio_menu_item_new_with_label (*group, label);
  *group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(menuitem));
  gtk_object_set_user_data(GTK_OBJECT(menuitem), label);
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
  gtk_widget_show (menuitem);
  return menuitem;
}

static void
dia_font_selector_build_font_menu(DiaFontSelector *fs) {
  GtkWidget *menu;
  GtkWidget *omenu;
  GtkWidget *menuitem;
  GSList *group;
  GList *entry;

  if (fs->font_omenu != NULL) {
    gtk_option_menu_remove_menu(fs->font_omenu);
    omenu = GTK_WIDGET(fs->font_omenu);
  } else {
    omenu = gtk_option_menu_new();
    fs->font_omenu = GTK_OPTION_MENU(omenu);
  }
  menu = gtk_menu_new ();
  fs->font_menu = GTK_MENU(menu);

  group = NULL;

  menuitem = dia_font_selector_add_menu_item("sans", &group, menu);
  menuitem = dia_font_selector_add_menu_item("serif", &group, menu);
  menuitem = dia_font_selector_add_menu_item("monospace", &group, menu);

  menuitem = gtk_separator_menu_item_new();
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
  gtk_widget_show (menuitem);

  for (entry = menu_entry_list; entry != NULL; entry = entry->next) {
    menuitem = dia_font_selector_add_menu_item((gchar *)entry->data, &group, menu);
  }
  menuitem = gtk_separator_menu_item_new();
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
  gtk_widget_show (menuitem);

  menuitem = gtk_menu_item_new_with_label(_("Other fonts..."));
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
  gtk_object_set_user_data(GTK_OBJECT(menuitem), NULL);
  gtk_widget_show (menuitem);

  gtk_option_menu_set_menu (GTK_OPTION_MENU (fs->font_omenu), menu);
  gtk_widget_show(menu);
  gtk_widget_show(omenu);

  gtk_signal_connect(GTK_OBJECT(menu), "unmap", 
                     GTK_SIGNAL_FUNC(dia_font_selector_menu_callback), fs);
}

static FontSelectorEntry *
dia_font_selector_get_new_font(DiaFontSelector *fs, const gchar *fontname)
{
  gchar *lowername = g_utf8_strdown(fontname, -1);
  FontSelectorEntry *fse = 
    (FontSelectorEntry*)g_hash_table_lookup(font_hash_table, lowername);
  if (fse == NULL) {
    fse = dia_font_selector_add_font(lowername, fontname, TRUE);
    dia_font_selector_build_font_menu(fs);
    dia_font_selector_write_persistence_file();
  }
  g_free(lowername);
  return fse;
}

static void
dia_font_selector_class_init (DiaFontSelectorClass *class)
{
  GtkObjectClass *object_class;
  
  object_class = (GtkObjectClass*) class;
}

static void
dia_font_selector_init (DiaFontSelector *fs)
{
  GtkWidget *menu;
  GtkWidget *omenu;

  dia_font_selector_build_font_menu(fs);
  
  /* Now build the style menu button */
  omenu = gtk_option_menu_new();
  fs->style_omenu = GTK_OPTION_MENU(omenu);
  menu = gtk_menu_new ();
  fs->style_menu = GTK_MENU(menu);
  gtk_option_menu_set_menu (GTK_OPTION_MENU (fs->style_omenu), menu);
  fs->old_font = 0;

  gtk_widget_show(menu);
  gtk_widget_show(omenu);

  gtk_box_pack_start_defaults(GTK_BOX(fs), GTK_WIDGET(fs->font_omenu));
  gtk_box_pack_start_defaults(GTK_BOX(fs), GTK_WIDGET(fs->style_omenu));
}

guint
dia_font_selector_get_type        (void)
{
  static guint dfs_type = 0;

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

    dia_font_selector_read_persistence_file();
  }
  
  return dfs_type;
}

GtkWidget *
dia_font_selector_new ()
{
  return GTK_WIDGET ( gtk_type_new (dia_font_selector_get_type ()));
}

static PangoFontFamily *
dia_font_selector_get_family_from_name(GtkWidget *widget, gchar *fontname)
{
  PangoFontFamily **families;
  int n_families,i;
  DiaFontFamily diafamily = DIA_FONT_FAMILY_ANY;
    
  pango_context_list_families (dia_font_get_context(),
			       &families, &n_families);
  /* Doing it the slow way until I find a better way */
  for (i = 0; i < n_families; i++) {
    if (!(g_strcasecmp(pango_font_family_get_name(families[i]), fontname)))
      return families[i];
  }
  g_warning(_("Couldn't find font family for %s\n"), fontname);
  return NULL;
}

/** Called when a font selection dialog is closed */
static void
dia_font_selector_dialog_callback(GtkWidget *widget, int id, gpointer data)
{
  DiaGtkFontSelectionDialog *fs = (DiaGtkFontSelectionDialog*)widget;
  DiaFontSelector *dfs = (DiaFontSelector*)data;
  const gchar *fontname;
  PangoFontDescription *pfd;
  DiaFont *diafont;

  switch (id) {
  case GTK_RESPONSE_OK:
    fontname = dia_gtk_font_selection_dialog_get_font_name(fs);
    pfd = pango_font_description_from_string(fontname);
    fontname = pango_font_description_get_family(pfd);
    
    dia_font_selector_get_new_font(dfs, fontname);
    diafont = dia_font_new(fontname, DIA_FONT_NORMAL | DIA_FONT_WEIGHT_NORMAL, 1.0);
    dia_font_selector_set_font(dfs, diafont);
    break;
  default: {
    /* Must set menu back to old font */
    FontSelectorEntry *fse;
    gchar *lowername;
    GtkWidget *active;
    gchar *fontname;

    gtk_option_menu_set_history(GTK_OPTION_MENU(dfs->font_omenu), 
				dfs->old_font);
    gtk_menu_set_active(dfs->font_menu, dfs->old_font);
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM (gtk_menu_get_active(dfs->font_menu)), 
                                   TRUE);

    active = gtk_menu_get_active(dfs->font_menu);
    if (active == NULL) {
      message_error("Can't find font entry for old font %d\n", dfs->old_font);
      return;
    }
    fontname = (gchar *)gtk_object_get_user_data(GTK_OBJECT(active));
    lowername = g_utf8_strdown(fontname, -1);
    fse = (FontSelectorEntry*)g_hash_table_lookup(font_hash_table, lowername);
    g_free(lowername);
    dia_font_selector_set_styles(dfs, fse, -1);
    dfs->old_font = fse->entry_nr;
  }
  }
  gtk_widget_hide(GTK_WIDGET(fs));
}

static void
dia_font_selector_menu_callback(GtkWidget *button, gpointer data)
{
  DiaFontSelector *fs = DIAFONTSELECTOR(data);
  gchar *fontname;

  GtkWidget *active = gtk_menu_get_active(fs->font_menu);
  if (active == NULL) return;
  fontname = (gchar *)gtk_object_get_user_data(GTK_OBJECT(active));
  if (fontname == NULL) {
    /* We hit the Other fonts... entry */
    GtkWidget *fsd = dia_gtk_font_selection_dialog_new(_("Select font"));
    dia_gtk_font_selection_dialog_set_context
      (DIA_GTK_FONT_SELECTION_DIALOG(fsd),
      dia_font_get_context());
    gtk_signal_connect(GTK_OBJECT(fsd), "response", 
		   GTK_SIGNAL_FUNC(dia_font_selector_dialog_callback), data);
    if (fs->textsample != NULL)
      dia_gtk_font_selection_dialog_set_preview_text(DIA_GTK_FONT_SELECTION_DIALOG(fsd), fs->textsample);
    gtk_widget_show(fsd);
  } else {
    FontSelectorEntry *fse;
    gchar *lowername = g_utf8_strdown(fontname, -1);
    fse = (FontSelectorEntry*)g_hash_table_lookup(font_hash_table, lowername);
    g_free(lowername);
    dia_font_selector_set_styles(fs, fse, -1);
    fs->old_font = fse->entry_nr;
  }
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

void
dia_font_selector_set_styles(DiaFontSelector *fs, FontSelectorEntry *fse,
                             DiaFontStyle dia_style)
{
  int i=0, select = 0;
  PangoFontFace **faces;
  int nfaces;
  GtkWidget *menu = gtk_menu_new();
  long stylebits = 0;
  int menu_item_nr = 0;
  GSList *group = NULL;

  if (fse->family == NULL) {
    PangoFontFamily *pff;
    pff = dia_font_selector_get_family_from_name(GTK_WIDGET(fs), fse->name);
    fse->family = pff;
    if (fse->family == NULL)
      return;
  }

  pango_font_family_list_faces(fse->family, &faces, &nfaces);

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

/* API functions */
/** Set a string to be used for preview in the GTK font selector dialog.
 * The start of this string will be copied.
 */
void
dia_font_selector_set_preview(DiaFontSelector *fs, gchar *text) {
  if (fs->textsample != NULL) g_free(fs->textsample);
  fs->textsample = g_strndup(text, 15);
}

/** Set the current font to be shown in the font selector.
 */
void
dia_font_selector_set_font(DiaFontSelector *fs, DiaFont *font)
{
  int font_nr;
  DiaFontStyle style = dia_font_get_style(font);
  GtkMenuItem *menuitem;
  FontSelectorEntry *fse;
  const gchar *fontname = dia_font_get_family(font);

  fse = dia_font_selector_get_new_font(fs, fontname);
  fse->last_select = time(0);
  dia_font_selector_set_styles(fs, fse, dia_font_get_style(font));

  font_nr = fse->entry_nr;

  gtk_option_menu_set_history(GTK_OPTION_MENU(fs->font_omenu), font_nr);
  gtk_menu_set_active(fs->font_menu, font_nr);
  gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_menu_get_active(fs->font_menu)), TRUE);
  fs->old_font = font_nr;
}

DiaFont *
dia_font_selector_get_font(DiaFontSelector *fs)
{
  GtkWidget *menuitem;
  char *fontname;
  DiaFontStyle style;
  FontSelectorEntry *fse;
  gchar *lowername;

  menuitem = gtk_menu_get_active(fs->font_menu);
  fontname = (gchar *)gtk_object_get_user_data(GTK_OBJECT(menuitem));
  lowername = g_utf8_strdown(fontname, -1);
  fse = (FontSelectorEntry*)g_hash_table_lookup(font_hash_table, lowername);
  g_free(lowername);
  fse->last_select = time(0);
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

guint
dia_alignment_selector_get_type        (void)
{
  static guint dfs_type = 0;

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

guint
dia_line_style_selector_get_type        (void)
{
  static guint dfs_type = 0;

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

static void
dia_color_selector_unrealize(GtkWidget *widget)
{
  DiaColorSelector *cs = DIACOLORSELECTOR(widget);

  if (cs->col_sel != NULL) {
    gtk_widget_destroy(cs->col_sel);
    cs->col_sel = NULL;
  }

  if (cs->gc != NULL) {
    gdk_gc_unref(cs->gc);
    cs->gc = NULL;
  }
  
  (* GTK_WIDGET_CLASS (gtk_type_class(gtk_button_get_type ()))->unrealize) (widget);
}

static void
dia_color_selector_class_init (DiaColorSelectorClass *klass)
{
  GtkObjectClass *object_class;
  GtkWidgetClass *widget_class;

  object_class = (GtkObjectClass*) klass;
  widget_class = (GtkWidgetClass*) klass;
  widget_class->unrealize = dia_color_selector_unrealize;
}

static gint
dia_color_selector_draw_area(GtkWidget          *area,
			     GdkEventExpose     *event,
			     DiaColorSelector *cs)
{
  if (cs->gc == NULL) {
    GdkColor col;
    cs->gc = gdk_gc_new(area->window);
    color_convert(&cs->col, &col);
    gdk_gc_set_foreground(cs->gc, &col);
  }
  
  gdk_draw_rectangle(area->window, cs->gc, TRUE,
		     event->area.x, event->area.y,
		     event->area.x + event->area.width,
		     event->area.y + event->area.height);
  return TRUE;
}



static void
dia_color_selector_ok(GtkWidget *widget, DiaColorSelector *cs)
{
  GdkColor gcol;
  Color col;

  gtk_color_selection_get_current_color(
	GTK_COLOR_SELECTION(
	    GTK_COLOR_SELECTION_DIALOG(cs->col_sel)->colorsel),
	&gcol);
  GDK_COLOR_TO_DIA(gcol, col);

  dia_color_selector_set_color(cs, &col);
  gtk_widget_hide(GTK_WIDGET(cs->col_sel));
}

static void
dia_color_selector_pressed(GtkWidget *widget)
{
  GtkColorSelectionDialog *dialog;
  DiaColorSelector *cs = DIACOLORSELECTOR(widget);
  
  GdkColor col;

  if (cs->col_sel == NULL) {
    cs->col_sel = gtk_color_selection_dialog_new(_("Select color"));
    dialog = GTK_COLOR_SELECTION_DIALOG(cs->col_sel);

    gtk_color_selection_set_has_palette (
	GTK_COLOR_SELECTION (GTK_COLOR_SELECTION_DIALOG (dialog)->colorsel),
	TRUE);

    gtk_widget_hide(dialog->help_button);
    
    gtk_signal_connect (GTK_OBJECT (dialog->ok_button), "clicked",
			(GtkSignalFunc) dia_color_selector_ok,
			cs);
    gtk_signal_connect (GTK_OBJECT (dialog), "destroy",
			(GtkSignalFunc) gtk_widget_destroyed,
			&cs->col_sel);
    
    gtk_signal_connect_object(GTK_OBJECT (dialog->cancel_button), "clicked",
			      (GtkSignalFunc) gtk_widget_hide,
			      GTK_OBJECT(dialog));
  }

  DIA_COLOR_TO_GDK(cs->col, col);

  gtk_color_selection_set_current_color(
	GTK_COLOR_SELECTION(
	    GTK_COLOR_SELECTION_DIALOG(cs->col_sel)->colorsel),
	&col);
  gtk_widget_show(cs->col_sel);
}

static void
dia_color_selector_init (DiaColorSelector *cs)
{
  GtkWidget *area;

  cs->col = DEFAULT_COLOR;
  cs->gc = NULL;
  cs->col_sel = NULL;
  
  area = gtk_drawing_area_new();
  cs->area = area;
  
  gtk_drawing_area_size(GTK_DRAWING_AREA(area), 30, 10);
  gtk_container_add(GTK_CONTAINER(cs), area);
  gtk_widget_show(area);

  gtk_signal_connect (GTK_OBJECT (area), "expose_event",
                      (GtkSignalFunc) dia_color_selector_draw_area,
                      cs);
  
  gtk_signal_connect (GTK_OBJECT (cs), "clicked",
                      (GtkSignalFunc) dia_color_selector_pressed,
                      NULL);
}

guint
dia_color_selector_get_type        (void)
{
  static guint dfs_type = 0;

  if (!dfs_type) {
    GtkTypeInfo dfs_info = {
      "DiaColorSelector",
      sizeof (DiaColorSelector),
      sizeof (DiaColorSelectorClass),
      (GtkClassInitFunc) dia_color_selector_class_init,
      (GtkObjectInitFunc) dia_color_selector_init,
      NULL,
      NULL,
      (GtkClassInitFunc) NULL,
    };
    
    dfs_type = gtk_type_unique (gtk_button_get_type (), &dfs_info);
  }
  
  return dfs_type;
}

GtkWidget *
dia_color_selector_new ()
{
  return GTK_WIDGET ( gtk_type_new (dia_color_selector_get_type ()));
}


void
dia_color_selector_get_color(DiaColorSelector *cs, Color *color)
{
  *color = cs->col;
}

void
dia_color_selector_set_color (DiaColorSelector *cs,
			      const Color *color)
{
  GdkColor col;
  
  cs->col = *color;
  if (cs->gc != NULL) {
    color_convert(&cs->col, &col);
    gdk_gc_set_foreground(cs->gc, &col);
    gtk_widget_queue_draw(GTK_WIDGET(cs));
  }

  if (cs->col_sel != NULL) {
    DIA_COLOR_TO_GDK(cs->col, col);

    gtk_color_selection_set_current_color(
	GTK_COLOR_SELECTION(
	    GTK_COLOR_SELECTION_DIALOG(cs->col_sel)->colorsel),
	&col);
  }
}


/************* DiaArrowSelector: ***************/
struct _DiaArrowSelector
{
  GtkVBox vbox;

  GtkHBox *sizebox;
  GtkLabel *sizelabel;
  DiaSizeSelector *size;
  
  GtkOptionMenu *omenu;
  GtkMenu *arrow_type_menu;
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
  GtkWidget *menuitem;
  if (!as->arrow_type_menu) return;
  menuitem = gtk_menu_get_active(as->arrow_type_menu);
  state = (GPOINTER_TO_INT(gtk_object_get_user_data(GTK_OBJECT(menuitem)))
	   != ARROW_NONE);

  gtk_widget_set_sensitive(GTK_WIDGET(as->sizelabel), state);
  gtk_widget_set_sensitive(GTK_WIDGET(as->size), state);
}

static void
arrow_type_change_callback(GtkObject *as, gboolean arg1, gpointer data)
{
  set_size_sensitivity(DIAARROWSELECTOR(as));
}

/* This is actually quite general, but only used here */
static void dia_arrow_fill_menu(GtkMenu *menu, GSList **group, 
                      const struct menudesc *menudesc)
{
  GtkWidget *mi, *ar;
  const struct menudesc *md = menudesc;

  while (md->name) {
    mi = gtk_menu_item_new();
    gtk_object_set_user_data(GTK_OBJECT(mi),
			GINT_TO_POINTER(md->enum_value));
#if 0
    if (tool_tips) {
      gtk_tooltips_set_tip(tool_tips, mi, md->name, NULL);
    }
#endif
    ar = dia_arrow_preview_new(md->enum_value, FALSE);

    gtk_container_add(GTK_CONTAINER(mi), ar);
    gtk_widget_show(ar);
    gtk_menu_shell_append (GTK_MENU_SHELL(menu), mi);
    gtk_widget_show(mi);

    md++;
  }
}


static void
dia_arrow_selector_init (DiaArrowSelector *as)
{
  GtkWidget *omenu;
  GtkWidget *menu;
  GtkWidget *submenu;
  GtkWidget *length;
  GtkWidget *width;
  GtkWidget *box;
  GtkWidget *label;
  GtkWidget *size;
  GtkAdjustment *adj;
  GSList *group;
  
  omenu = gtk_option_menu_new();
  as->omenu = GTK_OPTION_MENU(omenu);

  menu = gtk_menu_new ();
  as->arrow_type_menu = GTK_MENU(menu);
  submenu = NULL;
  group = NULL;

  dia_arrow_fill_menu(GTK_MENU(menu),&group,arrow_types);

  gtk_menu_set_active(GTK_MENU (menu), DEFAULT_ARROW);
  gtk_option_menu_set_menu (GTK_OPTION_MENU (omenu), menu);
  gtk_signal_connect_object(GTK_OBJECT(menu), "selection-done", 
			    GTK_SIGNAL_FUNC(arrow_type_change_callback), (gpointer)as);
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

guint
dia_arrow_selector_get_type        (void)
{
  static guint dfs_type = 0;

  if (!dfs_type) {
    GtkTypeInfo dfs_info = {
      "DiaArrowSelector",
      sizeof (DiaArrowSelector),
      sizeof (DiaArrowSelectorClass),
      (GtkClassInitFunc) dia_arrow_selector_class_init,
      (GtkObjectInitFunc) dia_arrow_selector_init,
      NULL,
      NULL,
      (GtkClassInitFunc) NULL,
    };
    
    dfs_type = gtk_type_unique (gtk_vbox_get_type (), &dfs_info);
  }
  
  return dfs_type;
}

GtkWidget *
dia_arrow_selector_new ()
{
  return GTK_WIDGET ( gtk_type_new (dia_arrow_selector_get_type ()));
}


Arrow 
dia_arrow_selector_get_arrow(DiaArrowSelector *as)
{
  GtkWidget *menuitem;
  Arrow at;

  menuitem = gtk_menu_get_active(as->arrow_type_menu);
  at.type = GPOINTER_TO_INT(gtk_object_get_user_data(GTK_OBJECT(menuitem)));
  dia_size_selector_get_size(as->size, &at.width, &at.length);
  return at;
}

void
dia_arrow_selector_set_arrow (DiaArrowSelector *as,
			      Arrow arrow)
{
  int arrow_type_index = 0,i = 0;
  const struct menudesc *md = arrow_types;
  
  while (md->name) {
    if (md->enum_value == arrow.type) {
      arrow_type_index = i;
      break;
    }
    md++; i++;
  }
  gtk_menu_set_active(GTK_MENU (as->arrow_type_menu), arrow_type_index);
  gtk_option_menu_set_history (GTK_OPTION_MENU(as->omenu), arrow_type_index);
/* TODO: restore CheckMenu version of menu */
/*  gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM(gtk_menu_get_active(GTK_MENU(as->arrow_type_menu))), TRUE);*/
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


guint
dia_file_selector_get_type (void)
{
  static guint dfs_type = 0;

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

