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
#include "intl.h"
#undef GTK_DISABLE_DEPRECATED /* ... */
#include "widgets.h"
#include "message.h"

#include <stdlib.h>
#include <glib.h>
#include <gdk/gdk.h>
#include <gtk/gtkradiomenuitem.h>
#include <gtk/gtklabel.h>
#include <gtk/gtksignal.h>
#include <pango/pango.h>

void dia_font_selector_set_styles (DiaFontSelector *fs, DiaFont *font);
guchar **dia_font_selector_get_sorted_names();

struct menudesc {
  char *name;
  int enum_value;
};

static void fill_menu(GtkMenu *menu, GSList **group, 
                      const struct menudesc *menudesc)
{
  GtkWidget *menuitem;
  const struct menudesc *md = menudesc;

  while (md->name) {
    menuitem = gtk_radio_menu_item_new_with_label (*group, md->name);
    gtk_object_set_user_data(
        GTK_OBJECT(menuitem), 
        GINT_TO_POINTER(((struct menudesc*)md)->enum_value));
    *group = gtk_radio_menu_item_group (GTK_RADIO_MENU_ITEM (menuitem));
    gtk_menu_shell_append (GTK_MENU_SHELL(menu), menuitem);
    gtk_widget_show (menuitem);

    md++;
  }
}

/************* DiaFontSelector: ***************/

static GHashTable *font_nr_hashtable = NULL;

static void
dia_font_selector_class_init (DiaFontSelectorClass *class)
{
  GtkObjectClass *object_class;
  
  object_class = (GtkObjectClass*) class;
}

#ifdef HAVE_FREETYPE
/* This function is stolen from dillo.
 */
static void
dia_font_selector_scroll_popup(GtkWidget *widget)
{
  /*
   * todo:
   *   1) Scrolling menues should rather be the task of Gtk+. This is
   *      a hack, and I don't know if it does not break anything.
   *   2) It could be improved, e.g. a timeout could be added for
   *      better mouse navigation.
    */
  int y, h, mx, my, sh;

  y = widget->allocation.y;
  h = widget->allocation.height;
  gdk_window_get_geometry (widget->parent->parent->window,
			   &mx, &my, NULL, NULL, NULL);
  sh = gdk_screen_height ();

  if (y + my < 0)
    gdk_window_move (widget->parent->parent->window, mx, - y + 1);
  else if (y + my > sh - h)
    gdk_window_move (widget->parent->parent->window, mx, sh - h - y - 1);
}

static void
dia_font_selector_font_selected(GtkWidget *button, gpointer data)
{
  DiaFontSelector *fs = DIAFONTSELECTOR(data);

  GtkWidget *active = gtk_menu_get_active(fs->font_menu);
  char *fontname = (char *)gtk_object_get_user_data(GTK_OBJECT(active));
  dia_font_selector_set_styles(fs, font_getfont(fontname));
}

struct select_pair { guchar **array; int counter; };

static void
dia_font_selector_collect_names(gpointer key, gpointer value,
				gpointer userdata) {
  struct select_pair *pair = (struct select_pair *)userdata;
  pair->array[pair->counter++] = (guchar *)key;
}

static int
dia_font_selector_sort_string(const void *s1, const void *s2) {
  return strcasecmp(*(char **)s1, *(char **)s2);
}


#endif

static int
dia_font_selector_compare_families(const void *o1, const void *o2) {
  PangoFontFamily *f1 = *(PangoFontFamily**)o1;
  PangoFontFamily *f2 = *(PangoFontFamily**)o2;
  
  return strcasecmp(pango_font_family_get_name(f1), 
		    pango_font_family_get_name(f2));
}

static void
dia_font_selector_init (DiaFontSelector *fs)
{
#ifdef HAVE_FREETYPE
        /* Lars said to not nuke this yet  -- cc, 22 jun 2002 */
    
  /* Go through hash table and build menu */
  GtkWidget *menu;
  GtkWidget *omenu;
  int i;
  guchar **fontnames;
  int num_fonts;

  omenu = gtk_option_menu_new();
  fs->font_omenu = GTK_OPTION_MENU(omenu);
  menu = gtk_menu_new ();
  fs->font_menu = GTK_MENU(menu);

  fontnames = dia_font_selector_get_fonts_sorted();
  num_fonts = g_hash_table_size(freetype_fonts);

  for (i = 0; i < num_fonts; i++) {
    GtkWidget *menuitem;
    menuitem = gtk_menu_item_new_with_label (fontnames[i]);
    gtk_object_set_user_data(GTK_OBJECT(menuitem), fontnames[i]);
    gtk_signal_connect(GTK_OBJECT(menuitem), "select", dia_font_selector_scroll_popup, NULL);
    gtk_menu_append (GTK_MENU (menu), menuitem);
    gtk_widget_show (menuitem);
  }

  g_free(fontnames);

  gtk_option_menu_set_menu (GTK_OPTION_MENU (fs->font_omenu), menu);
  gtk_widget_show(menu);
  gtk_widget_show(omenu);

  gtk_signal_connect(GTK_OBJECT(menu), "unmap", dia_font_selector_font_selected, fs);

  omenu = gtk_option_menu_new();
  fs->style_omenu = GTK_OPTION_MENU(omenu);
  menu = gtk_menu_new ();
  fs->style_menu = GTK_MENU(menu);
  gtk_option_menu_set_menu (GTK_OPTION_MENU (fs->style_omenu), menu);

  gtk_widget_show(menu);
  gtk_widget_show(omenu);

  gtk_box_pack_start_defaults(GTK_BOX(fs), GTK_WIDGET(fs->font_omenu));
  gtk_box_pack_start_defaults(GTK_BOX(fs), GTK_WIDGET(fs->style_omenu));
#else
  GtkWidget *menu;
  GtkWidget *submenu;
  GtkWidget *menuitem;
  GSList *group;
  const char *familyname;
  PangoFontFamily **families;
  int n_families,i;
      
  menu = gtk_menu_new ();
  fs->font_menu = GTK_MENU(menu);
  submenu = NULL;
  group = NULL;

  pango_context_list_families (gtk_widget_get_pango_context (GTK_WIDGET (menu)),
			             &families, &n_families);
  qsort(families, n_families, sizeof(*families), dia_font_selector_compare_families);
  for (i = 0; i < n_families; ++i) {
    familyname = pango_font_family_get_name(families[i]);
    menuitem = gtk_radio_menu_item_new_with_label (group, familyname);
    gtk_object_set_user_data(GTK_OBJECT(menuitem), (char *)familyname);
    group = gtk_radio_menu_item_group (GTK_RADIO_MENU_ITEM (menuitem));
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
    gtk_widget_show (menuitem);
        /* We COULD (SHOULD?) open sub-menus listing each face in each family
         */           
  }
  
  gtk_option_menu_set_menu (GTK_OPTION_MENU (fs), menu);
#endif
}

guint
dia_font_selector_get_type        (void)
{
  static guint dfs_type = 0;
  char *fontname;
  int i;
  PangoFontMap *fmap;
  PangoFontFamily **families;
  int n_families;

#ifdef HAVE_FREETYPE
  guchar **fontnames;
  int num_fonts;
#endif

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
    
#ifdef HAVE_FREETYPE
    dfs_type = gtk_type_unique (gtk_hbox_get_type (), &dfs_info);
#else
    dfs_type = gtk_type_unique (gtk_option_menu_get_type (), &dfs_info);
#endif

    /* Init the font hash_table: */
    font_nr_hashtable = g_hash_table_new(g_str_hash, g_str_equal);

#ifdef HAVE_FREETYPE
    num_fonts = g_hash_table_size(freetype_fonts);
    fontnames = dia_font_selector_get_fonts_sorted();
    for (i = 0; i < num_fonts; i++) {
      /* Notice adding 1, to be able to discern from NULL return value */
      g_hash_table_insert(font_nr_hashtable,
			  fontnames[i], GINT_TO_POINTER(i+1));
    }
    g_free(fontnames);
#else
    /* Notice adding 1, to be able to discern from NULL return value */

    pango_context_list_families (gdk_pango_context_get(),
			               &families, &n_families);

    for (i = 0; i < n_families; ++i) {
        fontname = (char *) pango_font_family_get_name(families[i]);
      
        g_hash_table_insert(font_nr_hashtable,
                            fontname,
                            GINT_TO_POINTER(i+1));
    }
#endif
  }
  
  return dfs_type;
}

GtkWidget *
dia_font_selector_new ()
{
  return GTK_WIDGET ( gtk_type_new (dia_font_selector_get_type ()));
}

#ifdef HAVE_FREETYPE
void
dia_font_selector_set_styles(DiaFontSelector *fs, DiaFont *font)
{
  int i=0, select = 0;
  GList *style_list = font->family->diafonts;

  GtkWidget *menu = gtk_menu_new();

  while (style_list != NULL) {
    DiaFont *style_font = (DiaFont *)style_list->data;
    GtkWidget *menuitem = gtk_menu_item_new_with_label (style_font->style);
    gtk_object_set_user_data(GTK_OBJECT(menuitem), style_font->style);
    if (!strcmp(style_font->style, font->style))
      select = i;
    i++;
    gtk_menu_append (GTK_MENU (menu), menuitem);
    gtk_widget_show (menuitem);
    style_list = g_list_next(style_list);
  }
  gtk_widget_show(menu);
  /* Need to dealloc the menu, methinks */
  gtk_option_menu_remove_menu(fs->style_omenu);
  gtk_option_menu_set_menu(fs->style_omenu, menu);
  fs->style_menu = GTK_MENU(menu);
  gtk_option_menu_set_history(GTK_OPTION_MENU(fs->style_omenu), select);
  gtk_menu_set_active(fs->style_menu, select);
}
#endif

void
dia_font_selector_set_font(DiaFontSelector *fs, DiaFont *font)
{
  void *font_nr_ptr;
  int font_nr;
  char *lower_name;

  lower_name = g_ascii_strdown(dia_font_get_family(font), -1);
  font_nr_ptr = g_hash_table_lookup(font_nr_hashtable,
                                    lower_name);
  g_free(lower_name);

  if (font_nr_ptr==NULL) {
    message_error(_("Trying to set invalid font %s!\n"),
                  dia_font_get_family(font));
    font_nr = 0;
  } else {
    /* Notice subtracting 1, to be able to discern from NULL return value */
    font_nr = GPOINTER_TO_INT(font_nr_ptr)-1;
  }
  
#ifdef HAVE_FREETYPE
  gtk_option_menu_set_history(GTK_OPTION_MENU(fs->font_omenu), font_nr);
  gtk_menu_set_active(fs->font_menu, font_nr);

  dia_font_selector_set_styles(fs, font);
#else
  gtk_option_menu_set_history(GTK_OPTION_MENU(fs), font_nr);
  gtk_menu_set_active(fs->font_menu, font_nr);
#endif
}

DiaFont *
dia_font_selector_get_font(DiaFontSelector *fs)
{
  GtkWidget *menuitem;
  char *fontname;
#ifdef HAVE_FREETYPE
  char *stylename;
#endif

  menuitem = gtk_menu_get_active(fs->font_menu);
  fontname = gtk_object_get_user_data(GTK_OBJECT(menuitem));

#ifdef HAVE_FREETYPE
  menuitem = gtk_menu_get_active(fs->style_menu);
  stylename = gtk_object_get_user_data(GTK_OBJECT(menuitem));
  return font_getfont_with_style(fontname, stylename);
#else
  return dia_font_new(fontname,STYLE_NORMAL,1.0);
#endif
}

/************* DiaAlignmentSelector: ***************/
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
  GtkWidget *menuitem;
  GtkWidget *label;
  GtkWidget *length;
  GtkWidget *box;
  GSList *group;
  GtkAdjustment *adj;
  
  menu = gtk_option_menu_new();
  fs->omenu = GTK_OPTION_MENU(menu);

  menu = gtk_menu_new ();
  fs->linestyle_menu = GTK_MENU(menu);
  submenu = NULL;
  group = NULL;

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

  gtk_widget_set_sensitive(GTK_WIDGET(as->lengthlabel), state);
  gtk_widget_set_sensitive(GTK_WIDGET(as->length), state);
  gtk_widget_set_sensitive(GTK_WIDGET(as->widthlabel), state);
  gtk_widget_set_sensitive(GTK_WIDGET(as->width), state);
}

static void
arrow_type_change_callback(GtkObject *as, gboolean arg1, gpointer data)
{
  set_size_sensitivity(DIAARROWSELECTOR(as));
}

static struct menudesc arrow_types[] =
{{N_("None"),ARROW_NONE},
 {N_("Lines"),ARROW_LINES},
 {N_("Hollow Triangle"),ARROW_HOLLOW_TRIANGLE},
 {N_("Filled Triangle"),ARROW_FILLED_TRIANGLE},
 {N_("Unfilled Triangle"),ARROW_UNFILLED_TRIANGLE},
 {N_("Filled Diamond"),ARROW_FILLED_DIAMOND},
 {N_("Half Head"),ARROW_HALF_HEAD},
 {N_("Slashed Cross"),ARROW_SLASHED_CROSS},
 {N_("Filled Ellipse"),ARROW_FILLED_ELLIPSE},
 {N_("Hollow Ellipse"),ARROW_HOLLOW_ELLIPSE},
 {N_("Filled Dot"),ARROW_FILLED_DOT},
 {N_("Dimension Origin"),ARROW_DIMENSION_ORIGIN},
 {N_("Blanked Dot"),ARROW_BLANKED_DOT},
 {N_("Double Hollow triangle"),ARROW_DOUBLE_HOLLOW_TRIANGLE},
 {N_("Double Filled triangle"),ARROW_DOUBLE_FILLED_TRIANGLE},
 {N_("Filled Box"),ARROW_FILLED_BOX},
 {N_("Blanked Box"),ARROW_BLANKED_BOX},
 {N_("Slashed"),ARROW_SLASH_ARROW},
 {N_("Integral Symbol"),ARROW_INTEGRAL_SYMBOL},
 {N_("Crow Foot"),ARROW_CROW_FOOT},
 {N_("Cross"),ARROW_CROSS},
 {NULL,0}};


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
  GtkAdjustment *adj;
  GSList *group;
  
  omenu = gtk_option_menu_new();
  as->omenu = GTK_OPTION_MENU(omenu);

  menu = gtk_menu_new ();
  as->arrow_type_menu = GTK_MENU(menu);
  submenu = NULL;
  group = NULL;

  fill_menu(GTK_MENU(menu),&group,arrow_types);

  gtk_menu_set_active(GTK_MENU (menu), DEFAULT_ARROW);
  gtk_option_menu_set_menu (GTK_OPTION_MENU (omenu), menu);
  gtk_signal_connect_object(GTK_OBJECT(menu), "selection-done", 
			    GTK_SIGNAL_FUNC(arrow_type_change_callback), (gpointer)as);
  gtk_box_pack_start(GTK_BOX(as), omenu, FALSE, TRUE, 0);
  gtk_widget_show(omenu);

  box = gtk_hbox_new(FALSE,0);
  as->sizebox = GTK_HBOX(box);

  label = gtk_label_new(_("Length: "));
  as->lengthlabel = GTK_LABEL(label);
  gtk_box_pack_start_defaults(GTK_BOX(box), label);
  gtk_widget_show(label);

  adj = (GtkAdjustment *)gtk_adjustment_new(0.1, 0.00, 10.0, 0.1, 1.0, 1.0);
  length = gtk_spin_button_new(adj, DEFAULT_ARROW_LENGTH, 2);
  gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(length), TRUE);
  gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(length), TRUE);
  as->length = GTK_SPIN_BUTTON(length);
  gtk_box_pack_start_defaults(GTK_BOX (box), length);
  gtk_widget_show (length);

  label = gtk_label_new(_("Width: "));
  as->widthlabel = GTK_LABEL(label);
  gtk_box_pack_start_defaults(GTK_BOX(box), label);
  gtk_widget_show(label);

  adj = (GtkAdjustment *)gtk_adjustment_new(0.1, 0.00, 10.0, 0.1, 1.0, 1.0);
  width = gtk_spin_button_new(adj, DEFAULT_ARROW_WIDTH, 2);
  gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(width), TRUE);
  gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(width), TRUE);
  as->width = GTK_SPIN_BUTTON(width);
  gtk_box_pack_start_defaults(GTK_BOX (box), width);
  gtk_widget_show (width);

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
  at.width = gtk_spin_button_get_value_as_float(as->width);
  at.length = gtk_spin_button_get_value_as_float(as->length);
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
  set_size_sensitivity(as);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(as->width), arrow.width);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(as->length), arrow.length);
}

/************* DiaFileSelector: ***************/
static void
dia_file_selector_unrealize(GtkWidget *widget)
{
  DiaFileSelector *fs = DIAFILESELECTOR(widget);

  if (fs->dialog != NULL) {
    gtk_widget_destroy(GTK_WIDGET(fs->dialog));
    fs->dialog = NULL;
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
  GtkFileSelection *dialog = GTK_FILE_SELECTION(data);
  DiaFileSelector *fs =
    DIAFILESELECTOR(gtk_object_get_user_data(GTK_OBJECT(dialog)));
  gtk_entry_set_text(GTK_ENTRY(fs->entry),
		     gtk_file_selection_get_filename(dialog));
  gtk_widget_hide(GTK_WIDGET(dialog));
}

static void
dia_file_selector_browse_pressed(GtkWidget *widget, gpointer data)
{
  GtkFileSelection *dialog;
  DiaFileSelector *fs = DIAFILESELECTOR(data);
  
  if (fs->dialog == NULL) {
    dialog = fs->dialog =
      GTK_FILE_SELECTION(gtk_file_selection_new(_("Select image file")));

    if (dialog->help_button != NULL)
      gtk_widget_hide(dialog->help_button);
    
    gtk_signal_connect (GTK_OBJECT (dialog->ok_button), "clicked",
			(GtkSignalFunc) dia_file_selector_ok,
			dialog);
    
    gtk_signal_connect_object(GTK_OBJECT (dialog->cancel_button), "clicked",
			      (GtkSignalFunc) gtk_widget_hide,
			      GTK_OBJECT(dialog));
    
    gtk_object_set_user_data(GTK_OBJECT(dialog), fs);
  }

  gtk_file_selection_set_filename(fs->dialog,
				  gtk_entry_get_text(fs->entry));
  
  gtk_widget_show(GTK_WIDGET(fs->dialog));
}

static void
dia_file_selector_init (DiaFileSelector *fs)
{
  /* Here's where we set up the real thing */
  fs->dialog = NULL;
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
  /* UTF8 conversions here ? */
  gtk_entry_set_text(GTK_ENTRY(fs->entry), file);
}

const gchar *
dia_file_selector_get_file(DiaFileSelector *fs)
{
  return gtk_entry_get_text(GTK_ENTRY(fs->entry));
}

