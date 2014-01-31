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

#undef GTK_DISABLE_DEPRECATED /* GtkOptionMenu, ... */
#include <gtk/gtk.h>

#include "intl.h"
#include "dialinechooser.h"
#include "widgets.h"

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
  GtkWidget *menuitem, *ln;
  GtkWidget *label;
  GtkWidget *length;
  GtkWidget *box;
  GtkAdjustment *adj;
  gint i;
  static const gchar *_line_style_names[LINESTYLE_DOTTED+1];
  static gboolean once = FALSE;

  if (!once) {
    _line_style_names[LINESTYLE_SOLID]        = Q_("line|Solid");
    _line_style_names[LINESTYLE_DASHED]       = Q_("line|Dashed");
    _line_style_names[LINESTYLE_DASH_DOT]     = Q_("line|Dash-Dot");
    _line_style_names[LINESTYLE_DASH_DOT_DOT] = Q_("line|Dash-Dot-Dot");
    _line_style_names[LINESTYLE_DOTTED]       = Q_("line|Dotted");
    once = TRUE;
  }

  menu = gtk_option_menu_new();
  fs->omenu = GTK_OPTION_MENU(menu);

  menu = gtk_menu_new ();
  fs->linestyle_menu = GTK_MENU(menu);

  for (i = 0; i <= LINESTYLE_DOTTED; i++) {
    menuitem = gtk_menu_item_new();
    gtk_widget_set_tooltip_text(menuitem, _line_style_names[i]);
    g_object_set_data(G_OBJECT(menuitem), "user_data", GINT_TO_POINTER(i));
    ln = dia_line_preview_new(i);
    gtk_container_add(GTK_CONTAINER(menuitem), ln);
    gtk_widget_show(ln);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
    gtk_widget_show(menuitem);
  }
  
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

  g_signal_connect(G_OBJECT(length), "changed", 
		   G_CALLBACK(linestyle_dashlength_change_callback), fs);

  set_linestyle_sensitivity(fs);
  gtk_box_pack_start(GTK_BOX(fs), box, TRUE, TRUE, 0);
  gtk_widget_show(box);
  
}

GType
dia_line_style_selector_get_type (void)
{
  static GType dfs_type = 0;

  if (!dfs_type) {
    static const GTypeInfo dfs_info = {
      sizeof (DiaLineStyleSelectorClass),
      (GBaseInitFunc) NULL,
      (GBaseFinalizeFunc) NULL,
      (GClassInitFunc) dia_line_style_selector_class_init,
      NULL, /* class_finalize */
      NULL, /* class_data */
      sizeof (DiaLineStyleSelector),
      0,    /* n_preallocs */
      (GInstanceInitFunc) dia_line_style_selector_init,
    };
    
    dfs_type = g_type_register_static (gtk_vbox_get_type (), 
				       "DiaLineStyleSelector",
				       &dfs_info, 0);
  }

  return dfs_type;
}

GtkWidget *
dia_line_style_selector_new ()
{
  return GTK_WIDGET ( g_object_new (dia_line_style_selector_get_type (), NULL));
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
