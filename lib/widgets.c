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

#include "config.h"
#include "intl.h"
#include "widgets.h"
#include "message.h"

#include <glib.h>
#include <gdk/gdk.h>
#include <gtk/gtkradiomenuitem.h>
#include <gtk/gtklabel.h>
#include <gtk/gtksignal.h>

/************* DiaFontSelector: ***************/

static GHashTable *font_nr_hashtable = NULL;

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
  GtkWidget *submenu;
  GtkWidget *menuitem;
  GSList *group;
  GList *list;
  char *fontname;
  
  menu = gtk_menu_new ();
  fs->font_menu = GTK_MENU(menu);
  submenu = NULL;
  group = NULL;

  list = font_names;
  while (list != NULL) {
    fontname = (char *) list->data;
    menuitem = gtk_radio_menu_item_new_with_label (group, fontname);
    gtk_object_set_user_data(GTK_OBJECT(menuitem), fontname);
    group = gtk_radio_menu_item_group (GTK_RADIO_MENU_ITEM (menuitem));
    gtk_menu_append (GTK_MENU (menu), menuitem);
    gtk_widget_show (menuitem);

    list = g_list_next(list);
  }
  
  gtk_option_menu_set_menu (GTK_OPTION_MENU (fs), menu);
}


guint
dia_font_selector_get_type        (void)
{
  static guint dfs_type = 0;
  GList *list;
  char *fontname;
  int i;

  if (!dfs_type) {
    GtkTypeInfo dfs_info = {
      "DiaFontSelector",
      sizeof (DiaFontSelector),
      sizeof (DiaFontSelectorClass),
      (GtkClassInitFunc) dia_font_selector_class_init,
      (GtkObjectInitFunc) dia_font_selector_init,
      (GtkArgSetFunc) NULL,
      (GtkArgGetFunc) NULL
    };
    
    dfs_type = gtk_type_unique (gtk_option_menu_get_type (), &dfs_info);


    /* Init the font hash_table: */
    font_nr_hashtable = g_hash_table_new(g_str_hash, g_str_equal);

    i=0;
    list = font_names;
    while (list != NULL) {
      fontname = (char *) list->data;
      
      g_hash_table_insert(font_nr_hashtable,
			  fontname,
			  GINT_TO_POINTER(i));
  
      list = g_list_next(list);
      i++;
    }
    
  }
  
  return dfs_type;
}

GtkWidget *
dia_font_selector_new ()
{
  return GTK_WIDGET ( gtk_type_new (dia_font_selector_get_type ()));
}

void
dia_font_selector_set_font(DiaFontSelector *fs, Font *font)
{
  void *font_nr_ptr;
  int font_nr;

  font_nr_ptr = g_hash_table_lookup(font_nr_hashtable,
				    font->name);

  if (font_nr_ptr==NULL) {
    message_error("Trying to set invalid font!\n");
    font_nr = 0;
  } else {
    font_nr = GPOINTER_TO_INT(font_nr_ptr);
  }
  
  gtk_option_menu_set_history(GTK_OPTION_MENU(fs), font_nr);
  gtk_menu_set_active(fs->font_menu, font_nr);
}

Font *
dia_font_selector_get_font(DiaFontSelector *fs)
{
  GtkWidget *menuitem;
  char *fontname;
  
  menuitem = gtk_menu_get_active(fs->font_menu);
  fontname = gtk_object_get_user_data(GTK_OBJECT(menuitem));

  return font_getfont(fontname);
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
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_widget_show (menuitem);

  menuitem = gtk_radio_menu_item_new_with_label (group, _("Center"));
  gtk_object_set_user_data(GTK_OBJECT(menuitem), GINT_TO_POINTER(ALIGN_CENTER));
  group = gtk_radio_menu_item_group (GTK_RADIO_MENU_ITEM (menuitem));
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_widget_show (menuitem);

  menuitem = gtk_radio_menu_item_new_with_label (group, _("Right"));
  gtk_object_set_user_data(GTK_OBJECT(menuitem), GINT_TO_POINTER(ALIGN_RIGHT));
  group = gtk_radio_menu_item_group (GTK_RADIO_MENU_ITEM (menuitem));
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_widget_show (menuitem);
  
  gtk_menu_set_active(GTK_MENU (menu), 0);
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
      (GtkArgSetFunc) NULL,
      (GtkArgGetFunc) NULL
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
dia_line_style_selector_init (DiaLineStyleSelector *fs)
{
  GtkWidget *menu;
  GtkWidget *submenu;
  GtkWidget *menuitem;
  GSList *group;
  
  menu = gtk_menu_new ();
  fs->linestyle_menu = GTK_MENU(menu);
  submenu = NULL;
  group = NULL;

  menuitem = gtk_radio_menu_item_new_with_label (group, _("Solid"));
  gtk_object_set_user_data(GTK_OBJECT(menuitem), GINT_TO_POINTER(LINESTYLE_SOLID));
  group = gtk_radio_menu_item_group (GTK_RADIO_MENU_ITEM (menuitem));
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_widget_show (menuitem);

  menuitem = gtk_radio_menu_item_new_with_label (group, _("Dashed"));
  gtk_object_set_user_data(GTK_OBJECT(menuitem), GINT_TO_POINTER(LINESTYLE_DASHED));
  group = gtk_radio_menu_item_group (GTK_RADIO_MENU_ITEM (menuitem));
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_widget_show (menuitem);

  menuitem = gtk_radio_menu_item_new_with_label (group, _("Dash-Dot"));
  gtk_object_set_user_data(GTK_OBJECT(menuitem), GINT_TO_POINTER(LINESTYLE_DASH_DOT));
  group = gtk_radio_menu_item_group (GTK_RADIO_MENU_ITEM (menuitem));
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_widget_show (menuitem);

  menuitem = gtk_radio_menu_item_new_with_label (group, _("Dash-Dot-Dot"));
  gtk_object_set_user_data(GTK_OBJECT(menuitem), GINT_TO_POINTER(LINESTYLE_DASH_DOT_DOT));
  group = gtk_radio_menu_item_group (GTK_RADIO_MENU_ITEM (menuitem));
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_widget_show (menuitem);
  
  menuitem = gtk_radio_menu_item_new_with_label (group, _("Dotted"));
  gtk_object_set_user_data(GTK_OBJECT(menuitem), GINT_TO_POINTER(LINESTYLE_DOTTED));
  group = gtk_radio_menu_item_group (GTK_RADIO_MENU_ITEM (menuitem));
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_widget_show (menuitem);
  
  gtk_menu_set_active(GTK_MENU (menu), 0);
  gtk_option_menu_set_menu (GTK_OPTION_MENU (fs), menu);
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
      (GtkArgSetFunc) NULL,
      (GtkArgGetFunc) NULL
    };
    
    dfs_type = gtk_type_unique (gtk_option_menu_get_type (), &dfs_info);
  }
  
  return dfs_type;
}

GtkWidget *
dia_line_style_selector_new ()
{
  return GTK_WIDGET ( gtk_type_new (dia_line_style_selector_get_type ()));
}


LineStyle 
dia_line_style_selector_get_linestyle(DiaLineStyleSelector *fs)
{
  GtkWidget *menuitem;
  void *align;
  
  menuitem = gtk_menu_get_active(fs->linestyle_menu);
  align = gtk_object_get_user_data(GTK_OBJECT(menuitem));

  return GPOINTER_TO_INT(align);
}

void
dia_line_style_selector_set_linestyle (DiaLineStyleSelector *as,
				       LineStyle linestyle)
{
  gtk_menu_set_active(GTK_MENU (as->linestyle_menu), linestyle);
  gtk_option_menu_set_history (GTK_OPTION_MENU(as), linestyle);
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
  gdouble gcol[3];
  Color col;

  gtk_color_selection_get_color(
	GTK_COLOR_SELECTION(
	    GTK_COLOR_SELECTION_DIALOG(cs->col_sel)->colorsel),
	gcol);
  col.red = gcol[0];
  col.green = gcol[1];
  col.blue = gcol[2];

  dia_color_selector_set_color(cs, &col);
  gtk_widget_hide(GTK_WIDGET(cs->col_sel));
}

static void
dia_color_selector_pressed(GtkWidget *widget)
{
  GtkColorSelectionDialog *dialog;
  DiaColorSelector *cs = DIACOLORSELECTOR(widget);
  
  gdouble col[3];

  if (cs->col_sel == NULL) {
    cs->col_sel = gtk_color_selection_dialog_new(_("Select color"));
    dialog = GTK_COLOR_SELECTION_DIALOG(cs->col_sel);
    gtk_widget_hide(dialog->help_button);
    
    gtk_signal_connect (GTK_OBJECT (dialog->ok_button), "pressed",
			(GtkSignalFunc) dia_color_selector_ok,
			cs);
    
    gtk_signal_connect_object(GTK_OBJECT (dialog->cancel_button), "pressed",
			      (GtkSignalFunc) gtk_widget_hide,
			      GTK_OBJECT(dialog));
  }

  col[0] = cs->col.red;
  col[1] = cs->col.green;
  col[2] = cs->col.blue;

  gtk_color_selection_set_color(
	GTK_COLOR_SELECTION(
	    GTK_COLOR_SELECTION_DIALOG(cs->col_sel)->colorsel),
	col);
  gtk_widget_show(cs->col_sel);
}

static void
dia_color_selector_init (DiaColorSelector *cs)
{
  GtkWidget *area;

  cs->col = color_white;
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
  
  gtk_signal_connect (GTK_OBJECT (cs), "pressed",
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
      (GtkArgSetFunc) NULL,
      (GtkArgGetFunc) NULL
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
			      Color *color)
{
  GdkColor col;
  
  cs->col = *color;
  if (cs->gc != NULL) {
    color_convert(&cs->col, &col);
    gdk_gc_set_foreground(cs->gc, &col);
    gtk_widget_queue_draw(GTK_WIDGET(cs));
  }

  if (cs->col_sel != NULL) {
    gdouble col[3];

    col[0] = cs->col.red;
    col[1] = cs->col.green;
    col[2] = cs->col.blue;

    gtk_color_selection_set_color(
	GTK_COLOR_SELECTION(
	    GTK_COLOR_SELECTION_DIALOG(cs->col_sel)->colorsel),
	col);
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

static void
dia_arrow_selector_init (DiaArrowSelector *as)
{
  GtkWidget *omenu;
  GtkWidget *menu;
  GtkWidget *submenu;
  GtkWidget *menuitem;
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

  menuitem = gtk_radio_menu_item_new_with_label (group, _("None"));
  gtk_object_set_user_data(GTK_OBJECT(menuitem), GINT_TO_POINTER(ARROW_NONE));
  group = gtk_radio_menu_item_group (GTK_RADIO_MENU_ITEM (menuitem));
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_widget_show (menuitem);

  menuitem = gtk_radio_menu_item_new_with_label (group, _("Lines"));
  gtk_object_set_user_data(GTK_OBJECT(menuitem), GINT_TO_POINTER(ARROW_LINES));
  group = gtk_radio_menu_item_group (GTK_RADIO_MENU_ITEM (menuitem));
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_widget_show (menuitem);

  menuitem = gtk_radio_menu_item_new_with_label (group, _("Hollow Triangle"));
  gtk_object_set_user_data(GTK_OBJECT(menuitem), GINT_TO_POINTER(ARROW_HOLLOW_TRIANGLE));
  group = gtk_radio_menu_item_group (GTK_RADIO_MENU_ITEM (menuitem));
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_widget_show (menuitem);

  menuitem = gtk_radio_menu_item_new_with_label (group, _("Filled Triangle"));
  gtk_object_set_user_data(GTK_OBJECT(menuitem), GINT_TO_POINTER(ARROW_FILLED_TRIANGLE));
  group = gtk_radio_menu_item_group (GTK_RADIO_MENU_ITEM (menuitem));
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_widget_show (menuitem);
  
  menuitem = gtk_radio_menu_item_new_with_label (group, _("Hollow Diamond"));
  gtk_object_set_user_data(GTK_OBJECT(menuitem), GINT_TO_POINTER(ARROW_HOLLOW_DIAMOND));
  group = gtk_radio_menu_item_group (GTK_RADIO_MENU_ITEM (menuitem));
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_widget_show (menuitem);

  menuitem = gtk_radio_menu_item_new_with_label (group, _("Filled Diamond"));
  gtk_object_set_user_data(GTK_OBJECT(menuitem), GINT_TO_POINTER(ARROW_FILLED_DIAMOND));
  group = gtk_radio_menu_item_group (GTK_RADIO_MENU_ITEM (menuitem));
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_widget_show (menuitem);
  
  menuitem = gtk_radio_menu_item_new_with_label (group, _("Half Head"));
  gtk_object_set_user_data(GTK_OBJECT(menuitem), GINT_TO_POINTER(ARROW_HALF_HEAD));
  group = gtk_radio_menu_item_group (GTK_RADIO_MENU_ITEM (menuitem));
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_widget_show (menuitem);

  gtk_menu_set_active(GTK_MENU (menu), 0);
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

  adj = (GtkAdjustment *)gtk_adjustment_new(0.1, 0.00, 10.0, 0.1, 0.0, 0.0);
  length = gtk_spin_button_new(adj, 1.0, 2);
  gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(length), TRUE);
  gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(length), TRUE);
  as->length = GTK_SPIN_BUTTON(length);
  gtk_box_pack_start_defaults(GTK_BOX (box), length);
  gtk_widget_show (length);

  label = gtk_label_new(_("Width: "));
  as->widthlabel = GTK_LABEL(label);
  gtk_box_pack_start_defaults(GTK_BOX(box), label);
  gtk_widget_show(label);

  adj = (GtkAdjustment *)gtk_adjustment_new(0.1, 0.00, 10.0, 0.1, 0.0, 0.0);
  width = gtk_spin_button_new(adj, 1.0, 2);
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
      (GtkArgSetFunc) NULL,
      (GtkArgGetFunc) NULL
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
  gtk_menu_set_active(GTK_MENU (as->arrow_type_menu), arrow.type);
  gtk_option_menu_set_history (GTK_OPTION_MENU(as->omenu), arrow.type);
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
  gtk_signal_connect (GTK_OBJECT (fs->browse), "pressed",
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
      (GtkArgSetFunc) NULL,
      (GtkArgGetFunc) NULL
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
  gtk_entry_set_text(GTK_ENTRY(fs->entry), file);
}

gchar *
dia_file_selector_get_file(DiaFileSelector *fs)
{
  return gtk_entry_get_text(GTK_ENTRY(fs->entry));
}

