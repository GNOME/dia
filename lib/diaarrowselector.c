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

#include <gtk/gtk.h>

#include "intl.h"
#include "widgets.h"
#include "arrows.h"
#include "diaarrowchooser.h"
#include "diadynamicmenu.h"

/************* DiaArrowSelector: ***************/

/* FIXME: Should these structs be in widgets.h instead? */
struct _DiaArrowSelector
{
  GtkVBox vbox;

  GtkHBox *sizebox;
  GtkLabel *sizelabel;
  DiaSizeSelector *size;
  
  GtkWidget *omenu;
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
  GtkWidget *preview;

  preview = dia_arrow_preview_new(atype, FALSE);

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
  as->omenu = 
  omenu = dia_dynamic_menu_new_listbased(create_arrow_menu_item,
					 as,
					 _("More arrows"),
					 arrow_names,
					 "arrow-menu");
  dia_dynamic_menu_add_default_entry(DIA_DYNAMIC_MENU(omenu), "None");
  dia_dynamic_menu_add_default_entry(DIA_DYNAMIC_MENU(omenu), "Lines");
  dia_dynamic_menu_add_default_entry(DIA_DYNAMIC_MENU(omenu), "Filled Concave");

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
      sizeof (DiaArrowSelectorClass),
      (GBaseInitFunc) NULL,
      (GBaseFinalizeFunc) NULL,
      (GClassInitFunc) dia_arrow_selector_class_init,
      NULL, /* class_finalize */
      NULL, /* class_data */
      sizeof (DiaArrowSelector),
      0,    /* n_preallocs */
      (GInstanceInitFunc)dia_arrow_selector_init,  /* init */
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
