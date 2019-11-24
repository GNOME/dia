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
#include <gtk/gtk.h>
#include "diadynamicmenu.h"
#include "persistence.h"

/* hidden internals for two reasosn:
 * - noone is supposed to mess with the internals
 * - it uses deprecated stuff
 */

typedef struct _DiaDynamicMenuPrivate DiaDynamicMenuPrivate;
struct _DiaDynamicMenuPrivate {
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


G_DEFINE_TYPE_WITH_PRIVATE (DiaDynamicMenu, dia_dynamic_menu, GTK_TYPE_OPTION_MENU)

/* ************************ Dynamic menus ************************ */

static void dia_dynamic_menu_create_sublist(DiaDynamicMenu *ddm,
					    GList *items,
					    DDMCreateItemFunc create);
static void dia_dynamic_menu_create_menu(DiaDynamicMenu *ddm);
static void dia_dynamic_menu_destroy(GObject *object);

enum {
    DDM_VALUE_CHANGED,
    DDM_LAST_SIGNAL
};

static guint ddm_signals[DDM_LAST_SIGNAL] = { 0 };


static void
dia_dynamic_menu_class_init(DiaDynamicMenuClass *class)
{
  GObjectClass *object_class = (GObjectClass*)class;

  object_class->dispose = dia_dynamic_menu_destroy;

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
dia_dynamic_menu_destroy(GObject *object)
{
  DiaDynamicMenu *self = DIA_DYNAMIC_MENU(object);
  DiaDynamicMenuPrivate *priv = dia_dynamic_menu_get_instance_private (self);

  g_free (priv->active);
  priv->active = NULL;

  if (G_OBJECT_CLASS (dia_dynamic_menu_parent_class)->dispose)
    G_OBJECT_CLASS (dia_dynamic_menu_parent_class)->dispose (object);
}


/**
 * dia_dynamic_menu_new:
 * @create: A function that creates menuitems from gpointers.
 * @otheritem: A menuitem that can be selected by the user to
 * add more entries, for instance making a dialog box or a submenu.
 * @persist: A string naming this menu for persistence purposes, or NULL.
 *
 * Create a new dynamic menu.  The entries are represented with
 * gpointers.
 *
 * Returns: A new menu
 */
GtkWidget *
dia_dynamic_menu_new(DDMCreateItemFunc create,
		     gpointer userdata,
		     GtkMenuItem *otheritem, gchar *persist)
{
  DiaDynamicMenu *ddm;
  DiaDynamicMenuPrivate *priv;

  g_assert(persist != NULL);

  ddm = DIA_DYNAMIC_MENU ( g_object_new (dia_dynamic_menu_get_type (), NULL));
  priv = dia_dynamic_menu_get_instance_private (ddm);

  priv->create_func = create;
  priv->userdata = userdata;
  priv->other_item = otheritem;
  priv->persistent_name = persist;
  priv->cols = 1;

  persistence_register_list(persist);

  dia_dynamic_menu_create_menu(ddm);

  return GTK_WIDGET(ddm);
}

/** Select the given entry, adding it if necessary */
void
dia_dynamic_menu_select_entry(DiaDynamicMenu *ddm, const gchar *name)
{
  DiaDynamicMenuPrivate *priv = dia_dynamic_menu_get_instance_private (ddm);
  gint add_result = dia_dynamic_menu_add_entry(ddm, name);
  if (add_result == 0) {
      GList *tmp;
      int i = 0;
      for (tmp = priv->default_entries; tmp != NULL;
	   tmp = g_list_next(tmp), i++) {
	if (!g_ascii_strcasecmp(tmp->data, name))
	  gtk_option_menu_set_history(GTK_OPTION_MENU(ddm), i);
      }
      /* Not there after all? */
  } else {
    if (priv->default_entries != NULL)
      gtk_option_menu_set_history(GTK_OPTION_MENU(ddm),
				  g_list_length(priv->default_entries)+1);
    else
      gtk_option_menu_set_history(GTK_OPTION_MENU(ddm), 0);
  }

  g_free(priv->active);
  priv->active = g_strdup(name);
  g_signal_emit(G_OBJECT(ddm), ddm_signals[DDM_VALUE_CHANGED], 0);
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
dia_dynamic_menu_create_sublist (DiaDynamicMenu    *ddm,
                                 GList             *items,
                                 DDMCreateItemFunc  create)
{
  DiaDynamicMenuPrivate *priv = dia_dynamic_menu_get_instance_private (ddm);
  GtkWidget *item = GTK_WIDGET(priv->other_item);

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


/**
 * dia_dynamic_menu_add_default_entry:
 * @ddm: A dynamic menu to add the entry to.
 * @entry: An entry for the menu.
 *
 * Add a new default entry to this menu.
 * The default entries are always shown, also after resets, above the
 * other entries.  Possible uses are standard fonts or common colors.
 * The entry is added at the end of the default entries section.
 * Do not add too many default entries.
 */
void
dia_dynamic_menu_add_default_entry (DiaDynamicMenu *ddm, const gchar *entry)
{
  DiaDynamicMenuPrivate *priv = dia_dynamic_menu_get_instance_private (ddm);

  priv->default_entries = g_list_append (priv->default_entries,
                                         g_strdup (entry));

  dia_dynamic_menu_create_menu (ddm);
}


/**
 * dia_dynamic_menu_add_entry:
 * @ddm: A dynamic menu to add the entry to.
 * @entry: An entry for the menu.
 *
 * Add a new entry to this menu.
 *
 * Returns: 0 if the entry was one of the default entries.
 * 1 if the entry was already there.
 * 2 if the entry got added.
 */
gint
dia_dynamic_menu_add_entry (DiaDynamicMenu *ddm, const gchar *entry)
{
  GList *tmp;
  gboolean existed;
  DiaDynamicMenuPrivate *priv = dia_dynamic_menu_get_instance_private (ddm);

  for (tmp = priv->default_entries; tmp != NULL; tmp = g_list_next(tmp)) {
    if (!g_ascii_strcasecmp(tmp->data, entry))
      return 0;
  }
  existed = persistent_list_add(priv->persistent_name, entry);

  dia_dynamic_menu_create_menu(ddm);

  return existed?1:2;
}


/**
 * dia_dynamic_menu_get_entry:
 *
 * Returns the currently selected entry.
 *
 * Returns: The name of the entry that is currently selected.  This
 * string should be freed by the caller.
 */
gchar *
dia_dynamic_menu_get_entry (DiaDynamicMenu *ddm)
{
  DiaDynamicMenuPrivate *priv = dia_dynamic_menu_get_instance_private (ddm);

  return g_strdup (priv->active);
}

/**
 * dia_dynamic_menu_create_menu:
 *
 * Rebuild the actual menu of a DDM.
 * Ignores columns for now.
 */
static void
dia_dynamic_menu_create_menu (DiaDynamicMenu *ddm)
{
  GtkWidget *sep;
  GList *tmplist;
  GtkWidget *menu;
  GtkWidget *item;
  DiaDynamicMenuPrivate *priv = dia_dynamic_menu_get_instance_private (ddm);

  g_object_ref(G_OBJECT(priv->other_item));
  menu = gtk_option_menu_get_menu(GTK_OPTION_MENU(ddm));
  if (menu != NULL) {
    gtk_container_remove(GTK_CONTAINER(menu), GTK_WIDGET(priv->other_item));
    gtk_container_foreach(GTK_CONTAINER(menu),
			  (GtkCallback)gtk_widget_destroy, NULL);
    gtk_option_menu_remove_menu(GTK_OPTION_MENU(ddm));
  }

  menu = gtk_menu_new();

  if (priv->default_entries != NULL) {
    for (tmplist = priv->default_entries; tmplist != NULL; tmplist = g_list_next(tmplist)) {
      GtkWidget *item =  (priv->create_func)(ddm, tmplist->data);
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

  for (tmplist = persistent_list_get_glist(priv->persistent_name);
       tmplist != NULL; tmplist = g_list_next(tmplist)) {
    GtkWidget *item = (priv->create_func)(ddm, tmplist->data);
    g_object_set_data(G_OBJECT(item), "ddm_name", tmplist->data);
    g_signal_connect(G_OBJECT(item), "activate",
		     G_CALLBACK(dia_dynamic_menu_activate), ddm);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
    gtk_widget_show(item);
  }
  sep = gtk_separator_menu_item_new();
  gtk_widget_show(sep);
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), sep);

  gtk_menu_shell_append(GTK_MENU_SHELL(menu), GTK_WIDGET(priv->other_item));
  g_object_unref(G_OBJECT(priv->other_item));
  /* Eventually reset item here */
  gtk_widget_show(menu);

  item = gtk_menu_item_new_with_label(_("Reset menu"));
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
  g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(dia_dynamic_menu_reset), ddm);
  gtk_widget_show(item);

  gtk_option_menu_set_menu(GTK_OPTION_MENU(ddm), menu);

  gtk_option_menu_set_history(GTK_OPTION_MENU(ddm), 0);
}


/**
 * dia_dynamic_menu_reset:
 *
 * Reset the non-default entries of a menu
 */
void
dia_dynamic_menu_reset (GtkWidget *item, gpointer userdata)
{
  DiaDynamicMenu *ddm = DIA_DYNAMIC_MENU (userdata);
  gchar *active = dia_dynamic_menu_get_entry (ddm);
  DiaDynamicMenuPrivate *priv = dia_dynamic_menu_get_instance_private (ddm);

  persistent_list_clear(priv->persistent_name);

  dia_dynamic_menu_create_menu(ddm);
  if (active)
    dia_dynamic_menu_select_entry(ddm, active);
  g_free(active);
}


/**
 * dia_dynamic_menu_get_default_entries:
 *
 * Deliver the list of default entries, NULL for empty list
 */
GList *
dia_dynamic_menu_get_default_entries (DiaDynamicMenu *ddm)
{
  DiaDynamicMenuPrivate *priv = dia_dynamic_menu_get_instance_private (ddm);

  return priv->default_entries;
}

/**
 * dia_dynamic_menu_get_persistent_name:
 *
 * Delivers the name used for persitence
 */
const gchar *
dia_dynamic_menu_get_persistent_name (DiaDynamicMenu *ddm)
{
  DiaDynamicMenuPrivate *priv = dia_dynamic_menu_get_instance_private (ddm);

  return priv->persistent_name;
}
