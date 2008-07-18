/* Dia -- a diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * diagram_tree_menu.c : menus for the diagram tree.
 * Copyright (C) 2001 Jose A Ortega Ruiz
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
 *  
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#undef GTK_DISABLE_DEPRECATED /* GtkItemFactory, GtkCTree */
#include "diagram_tree_menu_callbacks.h"
#include "diagram_tree_menu.h"
#include "diagram_tree_window.h"
#include "preferences.h"
#include "intl.h"

struct _DiagramTreeMenus {
  GtkItemFactory *factories[2];	/* menu factories */
  GtkWidget *menus[2];		/* popup menus */
  GtkWidget *show_menus[2];	/* show object type menus */
  DiagramTree *tree;		/* associated diagram tree */
};

#define SHOW_TYPE_PATH "/Show object type"

static const gchar *ROOT_NAMES_[] = {"<DiagramTreeDia>", "<DiagramTreeObj>"};

static GtkItemFactoryEntry common_items_[] = {
  { "/sep0", NULL, NULL, 0, "<Separator>" },
  { N_("/_Sort objects"), NULL, NULL, 0, "<Branch>" },
  { N_("/Sort objects/by _name"), NULL, on_sort_objects_activate,
    DIA_TREE_SORT_NAME, "" },
  { N_("/Sort objects/by _type"), NULL, on_sort_objects_activate,
    DIA_TREE_SORT_TYPE, "" },
  { N_("/Sort objects/as _inserted"), NULL, on_sort_objects_activate,
    DIA_TREE_SORT_INSERT, "" },
  { "/Sort objects/sep0", NULL, NULL, 0, "<Separator>" },
  { N_("/Sort objects/All by name"), NULL, on_sort_all_objects_activate,
    DIA_TREE_SORT_NAME, "" },
  { N_("/Sort objects/All by type"), NULL, on_sort_all_objects_activate,
    DIA_TREE_SORT_TYPE, "" },
  { N_("/Sort objects/All as inserted"), NULL, on_sort_all_objects_activate,
    DIA_TREE_SORT_INSERT, "" },
  { N_("/Sort objects/_Default"), NULL, NULL, 0, "<Branch>"},
  { N_("/Sort objects/Default/by _name"), NULL, on_sort_def_activate,
    DIA_TREE_SORT_NAME, "<RadioItem>" },
  { N_("/Sort objects/Default/by _type"), NULL, on_sort_def_activate,
    DIA_TREE_SORT_TYPE, "/Sort objects/Default/by name" },
  { N_("/Sort objects/Default/as _inserted"), NULL, on_sort_def_activate,
    DIA_TREE_SORT_INSERT, "/Sort objects/Default/by name" },
  { N_("/Sort _diagrams"), NULL, NULL, 0, "<Branch>" },
  { N_("/Sort _diagrams/by _name"), NULL, on_sort_diagrams_activate,
    DIA_TREE_SORT_NAME, "" },
  { N_("/Sort _diagrams/as _inserted"), NULL, on_sort_diagrams_activate,
    DIA_TREE_SORT_INSERT, "" },
  { N_("/Sort diagrams/_Default"), NULL, NULL, 0, "<Branch>"},
  { N_("/Sort diagrams/Default/by _name"), NULL, on_sort_dia_def_activate,
    DIA_TREE_SORT_NAME, "<RadioItem>" },
  { N_("/Sort diagrams/Default/as _inserted"), NULL, on_sort_dia_def_activate,
    DIA_TREE_SORT_INSERT, "/Sort diagrams/Default/by name" },
};

static const gint COMMON_ITEMS_SIZE_ =
sizeof (common_items_) / sizeof (common_items_[0]);

static GtkItemFactoryEntry object_items_[] = {
  { N_("/_Locate"), NULL, on_locate_object_activate, 0, "" },
  { N_("/_Properties"), NULL, on_properties_activate, 0, "" },
  { N_("/_Hide this type"), NULL, on_hide_object_activate, 0, "" },
  { N_(SHOW_TYPE_PATH), NULL, NULL, 0, "<Branch>" },
  /*  { "/sep1", NULL, NULL, 0, "<Separator>" },
      { N_("/_Delete"), NULL, on_delete_object_activate, 0, "" }, */
};

#define OBJ_ITEMS_SIZE_  (sizeof (object_items_) / sizeof (object_items_[0]))

static GtkItemFactoryEntry dia_items_[] = {
  { N_("/_Locate"), NULL, on_locate_dia_activate, 0, "" },
  { N_(SHOW_TYPE_PATH), NULL, NULL, 0, "<Branch>" },
};

#define DIA_ITEMS_SIZE_ (sizeof (dia_items_) / sizeof (dia_items_[0]))

static GtkItemFactoryEntry *items_[] = { dia_items_, object_items_ };
static gint items_size_[] = { DIA_ITEMS_SIZE_, OBJ_ITEMS_SIZE_ };

static gchar*
_dia_translate (const gchar *term, gpointer data)
{
  gchar *trans = term;
  
  if (term && *term) {
    /* first try our own ... */
    trans = dgettext (GETTEXT_PACKAGE, term);
    /* ... than gtk */
    if (term == trans)
      trans = dgettext ("gtk20", term);
#if 0
    /* FIXME: final fallback */
    if (term == trans) { /* FIXME: translation to be updated */
      gchar* kludge = g_strdup_printf ("/%s", term);
      trans = dgettext (GETTEXT_PACKAGE, kludge);
      if (kludge == trans)
	trans = term;
      else
	++trans;
      g_free (kludge);
    }
    if (term == trans)
      trans = g_strdup_printf ("XXX: %s", term);
#endif
  }
  return trans;
}

static GtkItemFactory*
create_factory(DiagramTree *tree, GtkWindow *window, gint no,
	       GtkItemFactoryEntry entries[], const gchar *path)
{
  GtkItemFactory *factory;
  GtkAccelGroup *accel = gtk_accel_group_new();
  gchar *item_path = NULL;
  GtkWidget *menu;
  
  factory = gtk_item_factory_new(GTK_TYPE_MENU, path, accel);
  gtk_item_factory_set_translate_func(factory, _dia_translate, NULL, NULL);
  gtk_item_factory_create_items(factory, no, entries,tree);
  gtk_item_factory_create_items(factory, COMMON_ITEMS_SIZE_, common_items_,
				tree);
  gtk_window_add_accel_group(window, accel);

  switch (diagram_tree_object_sort_type(tree)) {
  case DIA_TREE_SORT_NAME:
    item_path = g_strconcat(path, "/Sort objects/Default/by name", NULL);
    break;
  case DIA_TREE_SORT_TYPE:
    item_path = g_strconcat(path, "/Sort objects/Default/by type", NULL);
    break;
  default:
  case DIA_TREE_SORT_INSERT:
    item_path = g_strconcat(path, "/Sort objects/Default/as inserted", NULL);
    break;
  }

  menu = gtk_item_factory_get_widget(factory, item_path);
  gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu), TRUE);
  g_free(item_path);
  
  switch (diagram_tree_diagram_sort_type(tree)) {
  case DIA_TREE_SORT_NAME:
    item_path = g_strconcat(path, "/Sort diagrams/Default/by name", NULL);
    break;
  default:
  case DIA_TREE_SORT_INSERT:
    item_path = g_strconcat(path, "/Sort diagrams/Default/as inserted", NULL);
    break;
  }

  menu = gtk_item_factory_get_widget(factory, item_path);
  gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu), TRUE);
  g_free(item_path);

  return factory;
}

DiagramTreeMenus *
diagram_tree_menus_new(DiagramTree *tree, GtkWindow *window)
{
  DiagramTreeMenus *result;
  int k;
  
  g_return_val_if_fail(tree, NULL);
  g_return_val_if_fail(window, NULL);
  
  result = g_new(DiagramTreeMenus, 1);
  result->tree = tree;
  for (k = 0; k < 2; ++k) {
    enum {LEN = 128};
    static gchar BUFF[LEN];
    result->factories[k] = create_factory(tree, window, items_size_[k],
					  items_[k], ROOT_NAMES_[k]);
    result->menus[k] =
      gtk_item_factory_get_widget(result->factories[k], ROOT_NAMES_[k]);
    g_snprintf(BUFF, LEN, "%s%s", ROOT_NAMES_[k], SHOW_TYPE_PATH);
    result->show_menus[k] =
      gtk_item_factory_get_widget(result->factories[k], BUFF);
  }
  return result;
}

GtkWidget *
diagram_tree_menus_get_menu(const DiagramTreeMenus *menus,
			    DiagramTreeMenuType type)
{
  g_return_val_if_fail(menus, NULL);
  g_return_val_if_fail(type <= DIA_MENU_OBJECT, NULL);
  return menus->menus[type];
}

void
diagram_tree_menus_popup_menu(const DiagramTreeMenus *menus,
			      DiagramTreeMenuType type, gint time)
{
  g_return_if_fail(menus);
  g_return_if_fail(type <= DIA_MENU_OBJECT);
  gtk_menu_popup(GTK_MENU(menus->menus[type]), NULL, NULL, NULL, NULL, 3, time);
}


typedef struct _ShowTypeData 
{
  DiagramTreeMenus *menus;
  GtkWidget *items[2];
  gchar *type;
} ShowTypeData;

static void
on_show_object_type(GtkMenuItem *item, ShowTypeData *data)
{
  int k;
  diagram_tree_unhide_type(data->menus->tree, data->type);
  for (k = 0; k < 2; ++k) {
    gtk_container_remove(GTK_CONTAINER(data->menus->show_menus[k]),
			 data->items[k]);
  }
  g_free(data->type);
  g_free(data);
}

void
diagram_tree_menus_add_hidden_type(DiagramTreeMenus *menus,
				   const gchar *type)
{
  int k;
  ShowTypeData *data;
  g_return_if_fail(menus);
  g_return_if_fail(type);
  data = g_new(ShowTypeData, 1);
  data->type = g_strdup(type);
  data->menus = menus;
  for (k = 0; k < 2; ++k) {
    GtkMenuShell *shell = GTK_MENU_SHELL(menus->show_menus[k]);
    GtkWidget *item = gtk_menu_item_new_with_label(type);
    data->items[k] = item;
    gtk_menu_shell_append(shell, item);
    g_signal_connect(GTK_OBJECT(item),
		       "activate",
		     G_CALLBACK(on_show_object_type),
		       (gpointer)data);
    gtk_widget_show(item);
  }
}

