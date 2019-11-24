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

#include <gtk/gtk.h>

#pragma once

G_BEGIN_DECLS

/* DiaDynamicMenu */

G_DEFINE_AUTOPTR_CLEANUP_FUNC (GtkOptionMenu, g_object_unref)

#define DIA_TYPE_DYNAMIC_MENU dia_dynamic_menu_get_type ()
G_DECLARE_DERIVABLE_TYPE (DiaDynamicMenu, dia_dynamic_menu, DIA, DYNAMIC_MENU, GtkOptionMenu)

struct _DiaDynamicMenuClass {
  GtkOptionMenuClass parent_class;
};

typedef GtkWidget *(* DDMCreateItemFunc)(DiaDynamicMenu *, gchar *);
typedef void (* DDMCallbackFunc)(DiaDynamicMenu *, const gchar *, gpointer);

GtkWidget *dia_dynamic_menu_new                 (DDMCreateItemFunc  create,
                                                 gpointer           userdata,
                                                 GtkMenuItem       *otheritem,
                                                 gchar             *persist);
GtkWidget *dia_dynamic_menu_new_listbased       (DDMCreateItemFunc  create,
                                                 gpointer           userdata,
                                                 gchar             *other_label,
                                                 GList             *items,
                                                 gchar             *persist);
GtkWidget *dia_dynamic_menu_new_stringlistbased (gchar             *other_label,
                                                 GList             *items,
                                                 gpointer           userdata,
                                                 gchar             *persist);
void dia_dynamic_menu_add_default_entry(DiaDynamicMenu *ddm, const gchar *entry);
gint dia_dynamic_menu_add_entry(DiaDynamicMenu *ddm, const gchar *entry);
void dia_dynamic_menu_reset(GtkWidget *widget, gpointer userdata);
gchar *dia_dynamic_menu_get_entry(DiaDynamicMenu *ddm);
void dia_dynamic_menu_select_entry(DiaDynamicMenu *ddm, const gchar *entry);

GList *dia_dynamic_menu_get_default_entries(DiaDynamicMenu *ddm);
const gchar *dia_dynamic_menu_get_persistent_name(DiaDynamicMenu *ddm);

G_END_DECLS
