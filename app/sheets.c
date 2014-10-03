/* Dia -- a diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * sheets.c : a sheets and objects dialog
 * Copyright (C) 2002 M.C. Nelson
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
#include "config.h"

#include <string.h>

#undef GTK_DISABLE_DEPRECATED /* GtkOptionMenu */
#include <gtk/gtk.h>

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gmodule.h>

#include "../lib/plug-ins.h"
#include "../lib/sheet.h"
#include "../lib/message.h"
#include "../lib/object.h"
#include "../app/pixmaps/missing.xpm"

#include "interface.h"
#include "sheets.h"
#include "sheets_dialog.h"
#include "intl.h"
#include "gtkhwrapbox.h"
#include "preferences.h"
#include "toolbox.h" /* just for interface_current_sheet_name */
#include "commands.h" /* sheets_dialog_show_callback */

GtkWidget *sheets_dialog = NULL;
GSList *sheets_mods_list = NULL;
static gpointer custom_type_symbol = NULL;

/* Given a SheetObject and a SheetMod, create a new SheetObjectMod
   and hook it into the 'objects' list in the SheetMod->sheet
   
   NOTE: that the objects structure points to SheetObjectMod's and
        *not* SheetObject's */

SheetObjectMod *
sheets_append_sheet_object_mod(SheetObject *so, SheetMod *sm)
{
  SheetObjectMod *sheet_object_mod;
  DiaObjectType *ot;

  g_return_val_if_fail (so != NULL && sm != NULL, NULL);
  g_return_val_if_fail (custom_type_symbol != NULL, NULL);

  sheet_object_mod = g_new0(SheetObjectMod, 1);
  sheet_object_mod->sheet_object = *so;
  sheet_object_mod->mod = SHEET_OBJECT_MOD_NONE;

  ot = object_get_type(so->object_type);
  g_assert(ot);
  if (ot->ops == ((DiaObjectType *)(custom_type_symbol))->ops)
    sheet_object_mod->type = OBJECT_TYPE_SVG;
  else
    sheet_object_mod->type = OBJECT_TYPE_PROGRAMMED;

  sm->sheet.objects = g_slist_append(sm->sheet.objects, sheet_object_mod);

  return sheet_object_mod;
}

/* Given a Sheet, create a SheetMod wrapper for a list of SheetObjectMod's */

SheetMod *
sheets_append_sheet_mods(Sheet *sheet)
{
  SheetMod *sheet_mod;
  GSList *sheet_objects_list;

  g_return_val_if_fail (sheet != NULL, NULL);

  sheet_mod = g_new0(SheetMod, 1);
  sheet_mod->sheet = *sheet;
  sheet_mod->type = SHEETMOD_TYPE_NORMAL;
  sheet_mod->mod = SHEETMOD_MOD_NONE;
  sheet_mod->sheet.objects = NULL;

  for (sheet_objects_list = sheet->objects; sheet_objects_list;
       sheet_objects_list = g_slist_next(sheet_objects_list))
    sheets_append_sheet_object_mod(sheet_objects_list->data, sheet_mod);
    
  sheets_mods_list = g_slist_append(sheets_mods_list, sheet_mod);

  return sheet_mod;
}

static gint
menu_item_compare_labels(gconstpointer a, gconstpointer b)
{
  GList *a_list;
  const gchar *label;

  a_list = gtk_container_get_children(GTK_CONTAINER(GTK_MENU_ITEM(a)));
  g_assert(g_list_length(a_list) == 1);

  label = gtk_label_get_text(GTK_LABEL(a_list->data));
  g_list_free(a_list);

  if (!strcmp(label, (gchar *)b))
    return 0;
  else
    return 1;
}

void
sheets_optionmenu_create(GtkWidget *option_menu, GtkWidget *wrapbox,
                         gchar *sheet_name)
{
  GtkWidget *optionmenu_menu;
  GSList *sheets_list;
  GList *menu_item_list;

  /* Delete the contents, if any, of this optionemnu first */

  optionmenu_menu = gtk_option_menu_get_menu(GTK_OPTION_MENU(option_menu));
  gtk_container_foreach(GTK_CONTAINER(optionmenu_menu),
                        (GtkCallback)gtk_widget_destroy, NULL);

  for (sheets_list = sheets_mods_list; sheets_list;
       sheets_list = g_slist_next(sheets_list))
  {
    SheetMod *sheet_mod;
    GtkWidget *menu_item;
    gchar *tip;

    sheet_mod = sheets_list->data;

    /* We don't display sheets which have been deleted prior to Apply */

    if (sheet_mod->mod == SHEETMOD_MOD_DELETED)
      continue;

    {
      menu_item = gtk_menu_item_new_with_label(gettext(sheet_mod->sheet.name));

      gtk_menu_append(GTK_MENU(optionmenu_menu), menu_item);
      
      if (sheet_mod->sheet.scope == SHEET_SCOPE_SYSTEM)
	tip = g_strdup_printf(_("%s\nSystem sheet"), sheet_mod->sheet.description);
      else
	tip = g_strdup_printf(_("%s\nUser sheet"), sheet_mod->sheet.description);
      
      gtk_widget_set_tooltip_text(menu_item, tip);
      g_free(tip);
    }

    gtk_widget_show(menu_item);

    g_object_set_data(G_OBJECT(menu_item), "wrapbox", wrapbox);

    g_signal_connect(G_OBJECT(menu_item), "activate",
		     G_CALLBACK(on_sheets_dialog_optionmenu_activate),
                       (gpointer)sheet_mod);
  }
 
  menu_item_list = gtk_container_get_children(GTK_CONTAINER(optionmenu_menu));

  /* If we were passed a sheet_name, then make the optionmenu point to that
     name after creation */

  if (sheet_name)
  {
    gint index = 0;
    GList *list;

    list = g_list_find_custom(menu_item_list, sheet_name,
                              menu_item_compare_labels);
    if (list)
      index = g_list_position(menu_item_list, list);
    gtk_option_menu_set_history(GTK_OPTION_MENU(option_menu), index);
    gtk_menu_item_activate(GTK_MENU_ITEM(g_list_nth_data(menu_item_list,
                                                         index)));
  }
  else
  {
    gtk_option_menu_set_history(GTK_OPTION_MENU(option_menu), 0);
    gtk_menu_item_activate(GTK_MENU_ITEM(menu_item_list->data));
  }

  g_list_free(menu_item_list);
}

gboolean
sheets_dialog_create(void)
{
  GList *plugin_list;
  GSList *sheets_list;
  GtkWidget *option_menu;
  GtkWidget *sw;
  GtkWidget *wrapbox;
  gchar *sheet_left;
  gchar *sheet_right;

  if (sheets_mods_list)
    {
      /* not sure if I understood the data structure
       * but simply leaking isn't acceptable ... --hb
       */
      g_slist_foreach(sheets_mods_list, (GFunc)g_free, NULL);
      g_slist_free(sheets_mods_list);
    }
  sheets_mods_list = NULL;

  if (sheets_dialog == NULL)
  {
    sheets_dialog = create_sheets_main_dialog();
    if (!sheets_dialog) {
      /* don not let a broken builder file crash Dia */
      g_warning("SheetDialog creation failed");
      return FALSE;
    }
    /* Make sure to null our pointer when destroyed */
    g_signal_connect (G_OBJECT (sheets_dialog), "destroy",
		      G_CALLBACK (gtk_widget_destroyed),
		      &sheets_dialog);

    sheet_left = NULL;
    sheet_right = NULL;
  }
  else
  {
    option_menu = lookup_widget(sheets_dialog, "optionmenu_left");
    sheet_left = g_object_get_data(G_OBJECT(option_menu),
                                   "active_sheet_name");

    option_menu = lookup_widget(sheets_dialog, "optionmenu_right");
    sheet_right = g_object_get_data(G_OBJECT(option_menu),
                                    "active_sheet_name");

    wrapbox = lookup_widget(sheets_dialog, "wrapbox_left");
    if (wrapbox)
      gtk_widget_destroy(wrapbox);

    wrapbox = lookup_widget(sheets_dialog, "wrapbox_right");
    if (wrapbox)
      gtk_widget_destroy(wrapbox);
  }

  if (custom_type_symbol == NULL)
  {
    /* This little bit identifies a custom object symbol so we can tell the
       difference later between a SVG shape and a Programmed shape */

    custom_type_symbol = NULL;
    for (plugin_list = dia_list_plugins(); plugin_list != NULL;
         plugin_list = g_list_next(plugin_list))
    {
       PluginInfo *info = plugin_list->data;

       custom_type_symbol = (gpointer)dia_plugin_get_symbol (info,
                                                             "custom_type");
       if (custom_type_symbol)
         break;
    }
  }

  if (!custom_type_symbol)
  {
    message_warning (_("Can't get symbol 'custom_type' from any module.\n"
		       "Editing shapes is disabled."));
    return FALSE;
  }

  for (sheets_list = get_sheets_list(); sheets_list;
       sheets_list = g_slist_next(sheets_list))
    sheets_append_sheet_mods(sheets_list->data);
    
  sw = lookup_widget(sheets_dialog, "scrolledwindow_right");
  /* In case glade already add a child to scrolledwindow */
  wrapbox = gtk_bin_get_child (GTK_BIN(sw));
  if (wrapbox)
    gtk_container_remove(GTK_CONTAINER(sw), wrapbox);
  wrapbox = gtk_hwrap_box_new(FALSE);
  g_object_ref(wrapbox);
  g_object_set_data(G_OBJECT(sheets_dialog), "wrapbox_right", wrapbox);
  gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(sw), wrapbox);
  gtk_wrap_box_set_justify(GTK_WRAP_BOX(wrapbox), GTK_JUSTIFY_TOP);
  gtk_wrap_box_set_line_justify(GTK_WRAP_BOX(wrapbox), GTK_JUSTIFY_LEFT);
  gtk_widget_show(wrapbox);
  g_object_set_data(G_OBJECT(wrapbox), "is_left", FALSE);
  option_menu = lookup_widget(sheets_dialog, "optionmenu_right");
  sheets_optionmenu_create(option_menu, wrapbox, sheet_right);

  sw = lookup_widget(sheets_dialog, "scrolledwindow_left");
  /* In case glade already add a child to scrolledwindow */
  wrapbox = gtk_bin_get_child (GTK_BIN(sw));
  if (wrapbox)
    gtk_container_remove(GTK_CONTAINER(sw), wrapbox);
  wrapbox = gtk_hwrap_box_new(FALSE);
  g_object_ref(wrapbox);
  g_object_set_data(G_OBJECT(sheets_dialog), "wrapbox_left", wrapbox);
  gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(sw), wrapbox);
  gtk_wrap_box_set_justify(GTK_WRAP_BOX(wrapbox), GTK_JUSTIFY_TOP);
  gtk_wrap_box_set_line_justify(GTK_WRAP_BOX(wrapbox), GTK_JUSTIFY_LEFT);
  gtk_widget_show(wrapbox);
  g_object_set_data(G_OBJECT(wrapbox), "is_left", (gpointer)TRUE);
  option_menu = lookup_widget(sheets_dialog, "optionmenu_left");
  sheets_optionmenu_create(option_menu, wrapbox, sheet_left);

  return TRUE;
}

void
create_object_pixmap(SheetObject *so, GtkWidget *parent,
                     GdkPixmap **pixmap, GdkBitmap **mask)
{
  GtkStyle *style;

  g_assert(so);
  g_assert(pixmap);
  g_assert(mask);
  
  style = gtk_widget_get_style(parent);
  
  if (so->pixmap != NULL)
  {
    *pixmap =
      gdk_pixmap_colormap_create_from_xpm_d(NULL,
                                            gtk_widget_get_colormap(parent),
                                            mask, 
                                            &style->bg[GTK_STATE_NORMAL],
                                            (gchar **)so->pixmap);
  }
  else
  {
    if (so->pixmap_file != NULL)
    {
      GdkPixbuf *pixbuf;
      GError *error = NULL;

      pixbuf = gdk_pixbuf_new_from_file(so->pixmap_file, &error);
      if (pixbuf != NULL)
      {
        int width = gdk_pixbuf_get_width (pixbuf);
        int height = gdk_pixbuf_get_height (pixbuf);
        if (width > 22 && prefs.fixed_icon_size) 
	{
	  GdkPixbuf *cropped;
	  g_warning ("Shape icon '%s' size wrong, cropped.", so->pixmap_file);
	  cropped = gdk_pixbuf_new_subpixbuf (pixbuf, 
	                                      (width - 22) / 2, height > 22 ? (height - 22) / 2 : 0, 
					      22, height > 22 ? 22 : height);
	  g_object_unref (pixbuf);
	  pixbuf = cropped;
	}
        gdk_pixbuf_render_pixmap_and_mask(pixbuf, pixmap, mask, 1.0);
        g_object_unref(pixbuf);
      } else {
        message_warning ("%s", error->message);
        g_error_free (error);
	*pixmap = gdk_pixmap_colormap_create_from_xpm_d
	  (NULL,
	   gtk_widget_get_colormap(parent),
	   mask, 
	   &style->bg[GTK_STATE_NORMAL],
	   (gchar **)missing);
      }
    }
    else
    {
      *pixmap = NULL;
      *mask = NULL;
    }
  }
}

GtkWidget*
lookup_widget (GtkWidget   *widget,
               const gchar *widget_name)
{
  GtkWidget *parent, *found_widget;
  GtkBuilder *builder;

  g_return_val_if_fail(widget != NULL, NULL);

  for (;;)
    {
      if (GTK_IS_MENU (widget))
        parent = gtk_menu_get_attach_widget (GTK_MENU (widget));
      else
        parent = widget->parent;
      if (parent == NULL)
        break;
      widget = parent;
    }

  builder = g_object_get_data (G_OBJECT (widget), "_sheet_dialogs_builder");
  found_widget = GTK_WIDGET (gtk_builder_get_object (builder, widget_name));
  /* not everything is under control of the builder,
   * e.g. "wrapbox_left" */
  if (!found_widget)
    found_widget = (GtkWidget*) g_object_get_data (G_OBJECT (widget), widget_name);
  if (!found_widget)
    g_warning (_("Widget not found: %s"), widget_name);
  return found_widget;
}

/* The menu calls us here, after we've been instantiated */
void
sheets_dialog_show_callback(GtkAction *action)
{
  GtkWidget *wrapbox;
  GtkWidget *option_menu;

  if (!sheets_dialog)
    sheets_dialog_create();
  if (!sheets_dialog)
    return;

  wrapbox = g_object_get_data(G_OBJECT(sheets_dialog), "wrapbox_left");
  option_menu = lookup_widget(sheets_dialog, "optionmenu_left");
  sheets_optionmenu_create(option_menu, wrapbox, interface_current_sheet_name);

  g_assert(GTK_IS_WIDGET(sheets_dialog));
  gtk_widget_show(sheets_dialog);
}

gchar *
sheet_object_mod_get_type_string(SheetObjectMod *som)
{
  switch (som->type)
  {
  case OBJECT_TYPE_SVG:
    return _("SVG Shape");
  case OBJECT_TYPE_PROGRAMMED:
    return _("Programmed DiaObject");
  default:
    g_assert_not_reached();
    return "";
  }
}

