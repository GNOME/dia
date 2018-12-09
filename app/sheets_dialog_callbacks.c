/* Dia -- a diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * sheets_dialog_callbacks.c : a sheets and objects dialog
 * Copyright (C) 2002 M.C. Nelson
                      Steffen Macke
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

#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <time.h>
#ifdef HAVE_UTIME_H
#include <utime.h>
#else
/* isn't this placement common ? */
#include <sys/utime.h>
#ifdef G_OS_WIN32
#  define utime(n,b) _utime(n,b)
#  define utim_buf _utimbuf
#endif
#endif

#include <glib/gstdio.h>
#include <gmodule.h>

#undef GTK_DISABLE_DEPRECATED /* GtkOptionMenu */
#include <gtk/gtk.h>

#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xmlmemory.h>

#include "gtkwrapbox.h"

#include "dia_dirs.h"
#include "dia_xml_libxml.h"
#include "plug-ins.h"
#include "object.h"

#include "toolbox.h"
#include "message.h"
#include "sheets.h"
#include "sheets_dialog_callbacks.h"
#include "sheets_dialog.h"

#include "pixmaps/line_break.xpm"
#include "intl.h"

static GSList *radio_group = NULL;

static void
sheets_dialog_hide(void)
{
  gtk_widget_hide(sheets_dialog);
}

gboolean
on_sheets_main_dialog_delete_event     (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
  sheets_dialog_hide();
  return TRUE; 
}


static void
sheets_dialog_up_down_set_sensitive(GList *wrapbox_button_list, 
                                    GtkToggleButton *togglebutton)
{
  GtkWidget *button;
  GList *second;

  button = lookup_widget(sheets_dialog, "button_move_up");
  g_return_if_fail(button != NULL);

  /* If the active button is the 1st in the wrapbox, OR
     if the active button is a linebreak AND is 2nd in the wrapbox
     THEN set the 'Up' button to insensitive */

  if (!wrapbox_button_list
      || GTK_TOGGLE_BUTTON(g_list_first(wrapbox_button_list)->data) == togglebutton
      || (!g_object_get_data(G_OBJECT(togglebutton), "sheet_object_mod")
          && g_list_nth(wrapbox_button_list, 1)
          && GTK_TOGGLE_BUTTON(g_list_nth(wrapbox_button_list, 1)->data)
             == togglebutton))
    gtk_widget_set_sensitive(button, FALSE);
  else
    gtk_widget_set_sensitive(button, TRUE);

  button = lookup_widget(sheets_dialog, "button_move_down");
  g_return_if_fail(button != NULL);
  if (!wrapbox_button_list
      || GTK_TOGGLE_BUTTON(g_list_last(wrapbox_button_list)->data) == togglebutton
      || (!g_object_get_data(G_OBJECT(togglebutton), "sheet_object_mod")
          && ((second = g_list_previous(g_list_last(wrapbox_button_list))) != NULL)
          && GTK_TOGGLE_BUTTON(second->data) == togglebutton))
    gtk_widget_set_sensitive(button, FALSE);
  else
    gtk_widget_set_sensitive(button, TRUE);
}

static void
on_sheets_dialog_object_button_toggled(GtkToggleButton *togglebutton,
                                       gpointer ud_wrapbox)
{
  GList *wrapbox_button_list;
  GtkWidget *button;
  GtkWidget *table_sheets;
  GtkWidget *optionmenu_left;
  GtkWidget *optionmenu_right;
  gchar *sheet_left;
  gchar *sheet_right;

  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(togglebutton)) == FALSE)
    return;

  /* We remember the active button so we don't have to search for it later */

  g_object_set_data(G_OBJECT(ud_wrapbox), "active_button", togglebutton);

  /* Same with the active wrapbox */

  table_sheets = lookup_widget(sheets_dialog, "table_sheets");
  g_object_set_data(G_OBJECT(table_sheets), "active_wrapbox", ud_wrapbox);

  optionmenu_left = lookup_widget(sheets_dialog, "optionmenu_left");
  sheet_left = g_object_get_data(G_OBJECT(optionmenu_left),
                                 "active_sheet_name");

  optionmenu_right = lookup_widget(sheets_dialog, "optionmenu_right");
  sheet_right = g_object_get_data(G_OBJECT(optionmenu_right),
                                  "active_sheet_name");

  if (GPOINTER_TO_INT(g_object_get_data(G_OBJECT(ud_wrapbox), "is_left")) != 0)
  {
    g_object_set_data(G_OBJECT(table_sheets), "active_optionmenu",
                      optionmenu_left);
    button = lookup_widget(sheets_dialog, "button_copy");
    g_object_set(button, "label", _("Copy ->"), NULL);
    button = lookup_widget(sheets_dialog, "button_copy_all");
    g_object_set(button, "label", _("Copy All ->"), NULL);
    button = lookup_widget(sheets_dialog, "button_move");
    g_object_set(button, "label", _("Move ->"), NULL);
    button = lookup_widget(sheets_dialog, "button_move_all");
    g_object_set(button, "label", _("Move All ->"), NULL);
  }
  else
  {
    g_object_set_data(G_OBJECT(table_sheets), "active_optionmenu",
                      optionmenu_right);
    button = lookup_widget(sheets_dialog, "button_copy");
    g_object_set(button, "label", _("<- Copy"), NULL);
    button = lookup_widget(sheets_dialog, "button_copy_all");
    g_object_set(button, "label", _("<- Copy All"), NULL);
    button = lookup_widget(sheets_dialog, "button_move");
    g_object_set(button, "label", _("<- Move"), NULL);
    button = lookup_widget(sheets_dialog, "button_move_all");
    g_object_set(button, "label", _("<- Move All"), NULL);
  }

  sheet_left = sheet_left ? sheet_left : "";  /* initial value can be NULL */

  if (!strcmp(sheet_left, sheet_right)
      || g_object_get_data(G_OBJECT(togglebutton), "is_hidden_button")
      || !g_object_get_data(G_OBJECT(togglebutton), "sheet_object_mod"))
  {
    button = lookup_widget(sheets_dialog, "button_copy");
    gtk_widget_set_sensitive(button, FALSE);
    button = lookup_widget(sheets_dialog, "button_copy_all");
    gtk_widget_set_sensitive(button, FALSE);
    button = lookup_widget(sheets_dialog, "button_move");
    gtk_widget_set_sensitive(button, FALSE);
    button = lookup_widget(sheets_dialog, "button_move_all");
    gtk_widget_set_sensitive(button, FALSE);
  }
  else
  {
    button = lookup_widget(sheets_dialog, "button_copy");
    gtk_widget_set_sensitive(button, TRUE);
    button = lookup_widget(sheets_dialog, "button_copy_all");
    gtk_widget_set_sensitive(button, TRUE);
    button = lookup_widget(sheets_dialog, "button_move");
    gtk_widget_set_sensitive(button, TRUE);
    button = lookup_widget(sheets_dialog, "button_move_all");
    gtk_widget_set_sensitive(button, TRUE);
  }

  wrapbox_button_list = gtk_container_get_children(GTK_CONTAINER(ud_wrapbox));

  if (g_list_length(wrapbox_button_list))
  {
    SheetMod *sm;

    sheets_dialog_up_down_set_sensitive(wrapbox_button_list, togglebutton);

    sm = g_object_get_data(G_OBJECT(togglebutton), "sheet_mod");

    button = lookup_widget(sheets_dialog, "button_edit");
    if (!g_object_get_data(G_OBJECT(togglebutton), "sheet_object_mod")
        && sm->sheet.scope == SHEET_SCOPE_SYSTEM)
      gtk_widget_set_sensitive(button, FALSE);
    else
      gtk_widget_set_sensitive(button, TRUE);

    button = lookup_widget(sheets_dialog, "button_remove");
    if (g_object_get_data(G_OBJECT(togglebutton), "is_hidden_button")
        && sm->sheet.shadowing == NULL
        && sm->sheet.scope == SHEET_SCOPE_SYSTEM)
      gtk_widget_set_sensitive(button, FALSE);
    else
      gtk_widget_set_sensitive(button, TRUE);
  }
  else
  {
    button = lookup_widget(sheets_dialog, "button_move_up");
    gtk_widget_set_sensitive(button, FALSE);
    button = lookup_widget(sheets_dialog, "button_move_down");
    gtk_widget_set_sensitive(button, FALSE);
    button = lookup_widget(sheets_dialog, "button_edit");
    gtk_widget_set_sensitive(button, FALSE);
    button = lookup_widget(sheets_dialog, "button_remove");
    gtk_widget_set_sensitive(button, FALSE);
  }
  g_list_free(wrapbox_button_list);
}

static void
sheets_dialog_wrapbox_add_line_break(GtkWidget *wrapbox)
{
  SheetMod *sm;
  GtkWidget *button;
  GtkStyle *style;
  GdkPixmap *pixmap, *mask;
  GtkWidget *gtkpixmap;

  sm = g_object_get_data(G_OBJECT(wrapbox), "sheet_mod");
  g_assert(sm);

  button = gtk_radio_button_new(radio_group);
  gtk_button_set_relief(GTK_BUTTON(button), GTK_RELIEF_NONE);

  g_object_set_data(G_OBJECT(button), "sheet_mod", sm);

  radio_group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(button));

  gtk_toggle_button_set_mode(GTK_TOGGLE_BUTTON(button), FALSE);
  gtk_container_set_border_width(GTK_CONTAINER(button), 0);

  style = gtk_widget_get_style(wrapbox);
  pixmap =
    gdk_pixmap_colormap_create_from_xpm_d(NULL,
                                          gtk_widget_get_colormap(wrapbox),
                                          &mask,
                                          &style->bg[GTK_STATE_NORMAL],
                                          line_break_xpm);

  gtkpixmap = gtk_pixmap_new(pixmap, mask);
  gtk_container_add(GTK_CONTAINER(button), gtkpixmap);
  gtk_widget_show(gtkpixmap);

  gtk_wrap_box_pack(GTK_WRAP_BOX(wrapbox), button, FALSE, TRUE, FALSE, TRUE);
  gtk_widget_show(button);

  gtk_widget_set_tooltip_text(button, _("Line Break"));

  g_signal_connect(G_OBJECT(button), "toggled",
		   G_CALLBACK(on_sheets_dialog_object_button_toggled),
                   wrapbox);
}

static void
sheets_dialog_object_set_tooltip(SheetObjectMod *som, GtkWidget *button)
{
  gchar *tip;

   switch (som->type){
   case OBJECT_TYPE_SVG:
    tip = g_strdup_printf(_("%s\nShape"), som->sheet_object.description);
    break;
   case OBJECT_TYPE_PROGRAMMED:    
    tip = g_strdup_printf(_("%s\nObject"), som->sheet_object.description);
    break;
   default:    
    tip = g_strdup_printf(_("%s\nUnassigned type"), som->sheet_object.description);
    break;
  }

  gtk_widget_set_tooltip_text(button, tip);
  g_free(tip);
}

static GtkWidget *
sheets_dialog_create_object_button(SheetObjectMod *som, SheetMod *sm,
                                   GtkWidget *wrapbox)
{
  GtkWidget *button;
  GdkPixmap *pixmap, *mask;
  GtkWidget *gtkpixmap;

  button = gtk_radio_button_new(radio_group);
  radio_group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(button));

  gtk_toggle_button_set_mode(GTK_TOGGLE_BUTTON(button), FALSE);
  gtk_container_set_border_width(GTK_CONTAINER(button), 0);

  create_object_pixmap(&som->sheet_object, wrapbox, &pixmap, &mask);
  gtkpixmap = gtk_pixmap_new(pixmap, mask);
  gtk_container_add(GTK_CONTAINER(button), gtkpixmap);
  gtk_widget_show(gtkpixmap);

  sheets_dialog_object_set_tooltip(som, button);

  g_object_set_data(G_OBJECT(button), "sheet_mod", sm);
  g_object_set_data(G_OBJECT(button), "sheet_object_mod", som);

  g_signal_connect(G_OBJECT(button), "toggled",
		   G_CALLBACK(on_sheets_dialog_object_button_toggled),
                   wrapbox);
  return button;
}

gboolean optionmenu_activate_first_pass = TRUE;

void
on_sheets_dialog_optionmenu_activate   (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  GtkWidget *wrapbox;
  Sheet *sheet;
  GtkWidget *optionmenu;
  GSList *object_mod_list;
  GtkWidget *hidden_button;
  GList *button_list;

  sheet = &(((SheetMod *)(user_data))->sheet);
 
  wrapbox = g_object_get_data(G_OBJECT(menuitem), "wrapbox");
  g_assert(wrapbox);

  /* The hidden button is necessary to keep track of radio_group
     when there are no objects and to force 'toggled' events    */

  if (optionmenu_activate_first_pass)
  {
    hidden_button = gtk_radio_button_new(NULL);
    optionmenu_activate_first_pass = FALSE;
  }
  else
  {
    hidden_button = gtk_radio_button_new(radio_group);
    radio_group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(hidden_button));
  }

  g_signal_connect(G_OBJECT(hidden_button), "toggled",
		   G_CALLBACK(on_sheets_dialog_object_button_toggled),
                   wrapbox);
  g_object_set_data(G_OBJECT(hidden_button), "is_hidden_button",
                    (gpointer)TRUE);
  g_object_set_data(G_OBJECT(wrapbox), "hidden_button", hidden_button);
  g_object_set_data(G_OBJECT(hidden_button), "sheet_mod", user_data);

  if (g_object_get_data(G_OBJECT(wrapbox), "is_left"))
    optionmenu = lookup_widget(sheets_dialog, "optionmenu_left");
  else
    optionmenu = lookup_widget(sheets_dialog, "optionmenu_right");
  g_object_set_data(G_OBJECT(optionmenu), "active_sheet_name", sheet->name);

  gtk_container_foreach(GTK_CONTAINER(wrapbox),
                        (GtkCallback)gtk_widget_destroy, NULL);

  radio_group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(hidden_button));

  gtk_wrap_box_set_aspect_ratio(GTK_WRAP_BOX(wrapbox), 4 * 1.0 / 9);
                                                /* MCNFIXME: calculate this */

  g_object_set_data(G_OBJECT(wrapbox), "sheet_mod", user_data);

  for (object_mod_list = sheet->objects; object_mod_list;
       object_mod_list = g_slist_next(object_mod_list))
  {
    GtkWidget *button;
    SheetObjectMod *som;

    som = object_mod_list->data;
    if (som->mod == SHEET_OBJECT_MOD_DELETED)
      continue;

    if (som->sheet_object.line_break == TRUE)
      sheets_dialog_wrapbox_add_line_break(wrapbox);

    button = sheets_dialog_create_object_button(som, user_data, wrapbox);
   
    gtk_wrap_box_pack_wrapped(GTK_WRAP_BOX(wrapbox), button,
		      FALSE, TRUE, FALSE, TRUE, som->sheet_object.line_break);

    gtk_widget_show(button);
  }

  button_list = gtk_container_get_children(GTK_CONTAINER(wrapbox));

  if (g_list_length(button_list))
  {
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(hidden_button), TRUE);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button_list->data), TRUE);
  }
  else
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(hidden_button), TRUE);

  g_list_free(button_list);
}

static void
sheets_dialog_apply_revert_set_sensitive(gboolean is_sensitive)
{
  GtkWidget *button;

  button = lookup_widget(sheets_dialog, "button_apply");
  gtk_widget_set_sensitive(button, is_sensitive);

  button = lookup_widget(sheets_dialog, "button_revert");
  gtk_widget_set_sensitive(button, is_sensitive);
}

typedef enum
{
  SHEETS_DIALOG_MOVE_UP   = -1,
  SHEETS_DIALOG_MOVE_DOWN = +1
}
SheetsDialogMoveDir;
#define \
  SHEETS_DIALOG_MOVE_NONE   SHEETS_DIALOG_MOVE_UP

static void
_gtk_wrap_box_set_child_forced_break (GtkWrapBox* box, GtkWidget* child, 
                                      gboolean wrapped)
{
  gboolean hexpand, hfill, vexpand, vfill, dummy;

  gtk_wrap_box_query_child_packing (box, child, &hexpand, &hfill, 
                                    &vexpand, &vfill, &dummy);
  gtk_wrap_box_set_child_packing (box, child, hexpand, hfill, 
                                  vexpand, vfill, wrapped);
}
static void
sheets_dialog_normalize_line_breaks(GtkWidget *wrapbox, SheetsDialogMoveDir dir)
{
  GList *button_list;
  GList *iter_list;
  gboolean is_line_break;

  button_list = gtk_container_get_children(GTK_CONTAINER(wrapbox));

  is_line_break = FALSE;
  for (iter_list = button_list; iter_list; iter_list = g_list_next(iter_list))
  {
    SheetMod *sm;
    SheetObjectMod *som;

    sm = g_object_get_data(G_OBJECT(iter_list->data), "sheet_mod");
    som = g_object_get_data(G_OBJECT(iter_list->data), "sheet_object_mod");
    if (som)
    {
      if (is_line_break)
      {
        if (som->sheet_object.line_break == FALSE)
        {
          if (sm->mod == SHEETMOD_MOD_NONE)
            sm->mod = SHEETMOD_MOD_CHANGED;

          som->mod = SHEET_OBJECT_MOD_CHANGED;
        }

        som->sheet_object.line_break = TRUE;
        _gtk_wrap_box_set_child_forced_break(GTK_WRAP_BOX(wrapbox),
                                             GTK_WIDGET(iter_list->data), TRUE);
      }
      else
      {
        if (som->sheet_object.line_break == TRUE)
        {
          if (sm->mod == SHEETMOD_MOD_NONE)
            sm->mod = SHEETMOD_MOD_CHANGED;
          som->mod = SHEET_OBJECT_MOD_CHANGED;
        }

        som->sheet_object.line_break = FALSE;
        _gtk_wrap_box_set_child_forced_break(GTK_WRAP_BOX(wrapbox),
                                            GTK_WIDGET(iter_list->data), FALSE);
      }
      is_line_break = FALSE;
    }
    else
    {
      if (is_line_break)
      {
        if (dir == SHEETS_DIALOG_MOVE_UP)
        {
          iter_list = g_list_previous(iter_list);
          gtk_widget_destroy(GTK_WIDGET(iter_list->data));
          iter_list = g_list_next(iter_list);
          radio_group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(iter_list
                                                                      ->data));
        }
        else
        {
          GList *tmp_list;

          tmp_list = g_list_previous(iter_list);
          gtk_widget_destroy(GTK_WIDGET(iter_list->data));
          radio_group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(tmp_list
                                                                      ->data));
        }
      }
      else
        is_line_break = TRUE;
    }
  }

  /* We delete a trailing linebreak button in the wrapbox */

  iter_list = g_list_last(button_list);
  if (iter_list && !g_object_get_data(G_OBJECT(iter_list->data),
                                      "sheet_object_mod"))
  {
    gtk_widget_destroy(GTK_WIDGET(iter_list->data));
    if (g_list_previous(iter_list))
    {
      gtk_toggle_button_set_active(g_list_previous(iter_list)->data, TRUE);
      radio_group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(
                                            g_list_previous(iter_list)->data));
    }
  }

  g_list_free(button_list);
}

GtkWidget *
sheets_dialog_get_active_button(GtkWidget **wrapbox, GList **button_list)
{
  GtkWidget *table_sheets;

  table_sheets = lookup_widget(sheets_dialog, "table_sheets");
  *wrapbox = g_object_get_data(G_OBJECT(table_sheets), "active_wrapbox");
  *button_list = gtk_container_get_children(GTK_CONTAINER(*wrapbox));
  return g_object_get_data(G_OBJECT(*wrapbox), "active_button");
}


static void
sheets_dialog_move_up_or_down(SheetsDialogMoveDir dir)
{
  GtkWidget *table_sheets;
  GtkWidget *wrapbox;
  GList *button_list;
  GtkWidget *active_button;
  gint button_pos;
  SheetObjectMod *som;
  SheetObjectMod *som_next;
  GList *next_button_list;

  table_sheets = lookup_widget(sheets_dialog, "table_sheets");
  wrapbox = g_object_get_data(G_OBJECT(table_sheets), "active_wrapbox");

  button_list = gtk_container_get_children(GTK_CONTAINER(wrapbox));
  active_button = g_object_get_data(G_OBJECT(wrapbox), "active_button");

  button_pos = g_list_index(button_list, active_button);
  gtk_wrap_box_reorder_child(GTK_WRAP_BOX(wrapbox),
                             GTK_WIDGET(active_button), button_pos + dir);
  g_list_free(button_list);

  sheets_dialog_normalize_line_breaks(wrapbox, dir);

  /* And then reorder the backing store if necessary */

  button_list = gtk_container_get_children(GTK_CONTAINER(wrapbox));
  active_button = g_object_get_data(G_OBJECT(wrapbox), "active_button");

  som = g_object_get_data(G_OBJECT(active_button), "sheet_object_mod");

  next_button_list = g_list_next(g_list_find(button_list, active_button));
  if (next_button_list)
    som_next = g_object_get_data(G_OBJECT(next_button_list->data),
                                   "sheet_object_mod");
  else
    som_next = NULL; /* either 1) no button after 'active_button' 
                            or 2) button after 'active_button' is line break */

  /* This is all a little hairy, but the idea is that we don't need
     to reorder the backing store if the button was moved to the other
     side of a line break button since a line break button doesn't 
     exist in the backing store.  Starting to look a little like lisp... */

  if (som && (((dir == SHEETS_DIALOG_MOVE_DOWN)
                 && (som->sheet_object.line_break == FALSE))
          || (((dir == SHEETS_DIALOG_MOVE_UP)
                 && som_next))))
  {
    SheetMod *sm;
    GSList *object_list;
    gint object_pos;

    som->mod = SHEET_OBJECT_MOD_CHANGED;

    sm = g_object_get_data(G_OBJECT(active_button), "sheet_mod");
    if (sm->mod == SHEETMOD_MOD_NONE)
      sm->mod = SHEETMOD_MOD_CHANGED;
    
    object_list = g_slist_find(sm->sheet.objects, som);
    g_assert(object_list);

    object_pos = g_slist_position(sm->sheet.objects, object_list);
    sm->sheet.objects = g_slist_remove_link(sm->sheet.objects, object_list);
    sm->sheet.objects = g_slist_insert(sm->sheet.objects, object_list->data,
                                       object_pos + dir);
  }

  sheets_dialog_apply_revert_set_sensitive(TRUE);

  sheets_dialog_up_down_set_sensitive(button_list,
                                      GTK_TOGGLE_BUTTON(active_button));
  g_list_free(button_list);
}

void
on_sheets_dialog_button_move_up_clicked(GtkButton *button, gpointer user_data)
{
  sheets_dialog_move_up_or_down(SHEETS_DIALOG_MOVE_UP);
}

void
on_sheets_dialog_button_move_down_clicked(GtkButton *button, gpointer user_data)
{
  sheets_dialog_move_up_or_down(SHEETS_DIALOG_MOVE_DOWN);
}

void
on_sheets_dialog_button_close_clicked  (GtkButton       *button,
                                        gpointer         user_data)
{
  sheets_dialog_hide();
}

static GtkWidget *sheets_new_dialog;

typedef enum 
{
  SHEETS_NEW_DIALOG_TYPE_SVG_SHAPE = 1,   /* allows g_assert() */
  SHEETS_NEW_DIALOG_TYPE_LINE_BREAK,
  SHEETS_NEW_DIALOG_TYPE_SHEET
}
SheetsNewDialogType;

void
on_sheets_dialog_button_new_clicked    (GtkButton       *button,
                                        gpointer         user_data)
{
  GtkWidget *wrapbox;
  GList *button_list;
  GtkWidget *active_button;
  gboolean is_line_break_sensitive;

  sheets_new_dialog = create_sheets_new_dialog();

  /* Deterine if a line break button can be put after the active button */

  active_button = sheets_dialog_get_active_button(&wrapbox, &button_list);
  is_line_break_sensitive = TRUE;
  if (g_object_get_data(G_OBJECT(active_button), "sheet_mod"))
  {
    button_list = g_list_next(g_list_find(button_list, active_button));
    if (!button_list || !g_object_get_data(G_OBJECT(button_list->data),
                                         "sheet_mod"))
      is_line_break_sensitive = FALSE;
    g_list_free(button_list);
  }
  else
    is_line_break_sensitive = FALSE;

  active_button = lookup_widget(sheets_new_dialog, "radiobutton_line_break");
  gtk_widget_set_sensitive(active_button, is_line_break_sensitive);

  /* Use the 'ok' button to hold the current 'type' selection */

  active_button = lookup_widget(sheets_new_dialog, "button_ok");
  g_object_set_data(G_OBJECT(active_button), "active_type",
                    (gpointer)SHEETS_NEW_DIALOG_TYPE_SVG_SHAPE);

  gtk_widget_show(sheets_new_dialog);
}

void
on_sheets_new_dialog_button_cancel_clicked
                                        (GtkButton       *button,
                                        gpointer         user_data)
{
  gtk_widget_destroy(sheets_new_dialog);
  sheets_new_dialog = NULL;
}

void
on_sheets_new_dialog_button_ok_clicked (GtkButton       *button,
                                        gpointer         user_data)
{
  SheetsNewDialogType active_type;
  GtkWidget *table_sheets;
  GtkWidget *wrapbox;
  GList *button_list;
  GtkWidget *active_button;

  table_sheets = lookup_widget(sheets_dialog, "table_sheets");
  wrapbox = g_object_get_data(G_OBJECT(table_sheets), "active_wrapbox");

  active_type = (SheetsNewDialogType)g_object_get_data(G_OBJECT(button),
                                                       "active_type");
  g_assert(active_type);

  switch (active_type)
  {
    GtkWidget *entry;
    gchar *file_name;
    gchar *p;
    struct stat stat_buf;
    GList *plugin_list;
    DiaObjectType *ot;
    typedef gboolean (*CustomObjectLoadFunc) (gchar*, DiaObjectType **);
    CustomObjectLoadFunc custom_object_load_fn;
    gint pos;
    GtkWidget *active_button;
    GList *button_list;
    SheetObjectMod *som;
    SheetMod *sm;
    SheetObject *sheet_obj;
    Sheet *sheet;
    GtkWidget *optionmenu;

  case SHEETS_NEW_DIALOG_TYPE_SVG_SHAPE:

    entry = lookup_widget(sheets_new_dialog, "file_chooser_button");
    /* Since this is a file name, no utf8 translation is needed */
    file_name = gtk_file_chooser_get_preview_filename(GTK_FILE_CHOOSER(entry));

    if (!file_name)
    {
      message_error(_("Please select a .shape file"));
      return;
    }

    p = file_name + strlen(file_name) - 6;
    if (strcmp(p, ".shape"))
    {
      message_error(_("Filename must end with '%s': '%s'"),
                    ".shape", dia_message_filename(file_name));
      g_free(file_name);
      return;
    }

    if (g_stat(file_name, &stat_buf) == -1)
    {
      message_error(_("Error examining %s: %s"), 
		    dia_message_filename(file_name), strerror(errno));
      g_free(file_name);
      return;
    }

    custom_object_load_fn = NULL;
    for (plugin_list = dia_list_plugins(); plugin_list != NULL;
         plugin_list = g_list_next(plugin_list))
    {
       PluginInfo *info = plugin_list->data;

       custom_object_load_fn = 
         (CustomObjectLoadFunc)dia_plugin_get_symbol (info, "custom_object_load");
       if (custom_object_load_fn)
         break;
    }
    g_assert(custom_object_load_fn);

    if (!(*custom_object_load_fn)(file_name, &ot))
    {
    	xmlDoc *doc = NULL;
	xmlNode *root_element = NULL;
	
	/* See if the user tries to open a diagram as a shape */	
	doc = xmlReadFile(file_name, NULL, 0);
	if(doc != NULL)
	{
		root_element = xmlDocGetRootElement(doc);
		if(0 == g_ascii_strncasecmp((gchar *)root_element->name, "dia", 3))
			message_error(
				_("Please export the diagram as a shape."));
		xmlFreeDoc(doc);
	}
	message_error(_("Could not interpret shape file: '%s'"), 
		    dia_message_filename(file_name));
        g_free(file_name);
      return;
    }
    object_register_type(ot);

    entry = lookup_widget(sheets_new_dialog, "entry_svg_description");
    sheet_obj = g_new0(SheetObject, 1);
    sheet_obj->object_type = g_strdup(ot->name);
    {
      sheet_obj->description =
	gtk_editable_get_chars(GTK_EDITABLE(entry), 0, -1);
    }
    sheet_obj->pixmap = ot->pixmap;
    sheet_obj->user_data = ot->default_user_data;
    sheet_obj->user_data_type = USER_DATA_IS_OTHER;
    sheet_obj->line_break = FALSE;
    sheet_obj->pixmap_file = g_strdup(ot->pixmap_file);
    sheet_obj->has_icon_on_sheet = FALSE;

    sm = g_object_get_data(G_OBJECT(wrapbox), "sheet_mod");
    som = sheets_append_sheet_object_mod(sheet_obj, sm);
    som->mod = SHEET_OBJECT_MOD_NEW;
    som->svg_filename = g_strdup(file_name);
    if (sm->mod == SHEETMOD_MOD_NONE)
      sm->mod = SHEETMOD_MOD_CHANGED;

    active_button = sheets_dialog_create_object_button(som, sm, wrapbox);
    gtk_wrap_box_pack(GTK_WRAP_BOX(wrapbox), active_button,
                      FALSE, TRUE, FALSE, TRUE);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(active_button), TRUE);
    gtk_widget_show(active_button);

    break;

  case SHEETS_NEW_DIALOG_TYPE_LINE_BREAK:

    sheets_dialog_wrapbox_add_line_break(wrapbox);

    button_list = gtk_container_get_children(GTK_CONTAINER(wrapbox));
    active_button = g_object_get_data(G_OBJECT(wrapbox), "active_button");
    pos = g_list_index(button_list, active_button);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(g_list_last(button_list)
                                                                 ->data), TRUE);
    gtk_wrap_box_reorder_child(GTK_WRAP_BOX(wrapbox),
                               g_list_last(button_list)->data, pos + 1);
    
    sheets_dialog_normalize_line_breaks(wrapbox, SHEETS_DIALOG_MOVE_NONE);

    sm = g_object_get_data(G_OBJECT(wrapbox), "sheet_mod");
    if (sm->mod == SHEETMOD_MOD_NONE)
      sm->mod = SHEETMOD_MOD_CHANGED;
    
    g_list_free(button_list);
    break;

  case SHEETS_NEW_DIALOG_TYPE_SHEET:
    {
      gchar *sheet_name;
      gchar *sheet_descrip;

      entry = lookup_widget(sheets_new_dialog, "entry_sheet_name");
      sheet_name = gtk_editable_get_chars(GTK_EDITABLE(entry), 0, -1);

      sheet_name = g_strchug(g_strchomp(sheet_name));
      if (!*sheet_name)
      {
        message_error(_("Sheet must have a Name"));
        return;
      }

      entry = lookup_widget(sheets_new_dialog, "entry_sheet_description");
      sheet_descrip = gtk_editable_get_chars(GTK_EDITABLE(entry), 0, -1);

      sheet = g_new0(Sheet, 1);
      sheet->name = sheet_name;
      sheet->filename = "";
      sheet->description = sheet_descrip;
      sheet->scope = SHEET_SCOPE_USER;
      sheet->shadowing = NULL;
      sheet->objects = NULL;

      sm = sheets_append_sheet_mods(sheet);
      sm->mod = SHEETMOD_MOD_NEW;

      table_sheets = lookup_widget(sheets_dialog, "table_sheets");
      optionmenu = g_object_get_data(G_OBJECT(table_sheets),
                                     "active_optionmenu");
      g_assert(optionmenu);
      sheets_optionmenu_create(optionmenu, wrapbox, sheet_name);
    }
    break;

  default:
    g_assert_not_reached();
  }

  sheets_dialog_apply_revert_set_sensitive(TRUE);

  button_list = gtk_container_get_children(GTK_CONTAINER(wrapbox));
  active_button = g_object_get_data(G_OBJECT(wrapbox), "active_button");
  sheets_dialog_up_down_set_sensitive(button_list,
                                      GTK_TOGGLE_BUTTON(active_button));
  g_list_free(button_list);

  gtk_widget_destroy(sheets_new_dialog);
  sheets_new_dialog = NULL;
}

static GtkWidget *sheets_shapeselection_dialog;

void
on_sheets_shapeselection_dialog_button_cancel_clicked
                                        (GtkButton       *button,
                                        gpointer         user_data)
{
  gtk_widget_destroy(sheets_shapeselection_dialog);
  sheets_shapeselection_dialog = NULL;
}

void
on_sheets_shapeselection_dialog_button_ok_clicked
                                        (GtkButton       *button,
                                        gpointer         user_data)
{
  const gchar *filename;
  GtkWidget *entry;

  filename =
      gtk_file_selection_get_filename(
          GTK_FILE_SELECTION(sheets_shapeselection_dialog));

  entry = lookup_widget(sheets_new_dialog, "combo_entry_from_file");
  /* Since this is a filename entry from the dialog, no utf is needed */
  gtk_entry_set_text(GTK_ENTRY(entry), filename);

  gtk_widget_destroy(sheets_shapeselection_dialog);
  sheets_shapeselection_dialog = NULL;
}

static GtkWidget *sheets_edit_dialog;

void
on_sheets_dialog_button_edit_clicked   (GtkButton       *button,
                                        gpointer         user_data)
{
  GtkWidget *wrapbox;
  GList *button_list;
  GtkWidget *active_button;
  SheetObjectMod *som;
  gchar *descrip = "";
  gchar *type = "";
  GtkWidget *entry;
  SheetMod *sm;
  gchar *name = "";

  if (sheets_edit_dialog == NULL)
    sheets_edit_dialog = create_sheets_edit_dialog();

  active_button = sheets_dialog_get_active_button(&wrapbox, &button_list);
  som = g_object_get_data(G_OBJECT(active_button), "sheet_object_mod");
  if (som)
  {
    descrip = som->sheet_object.description;
    type = sheet_object_mod_get_type_string(som);
  }
  
  entry = lookup_widget(sheets_edit_dialog, "entry_object_description");
  if (som)
    gtk_entry_set_text(GTK_ENTRY(entry), descrip);
  else 
    gtk_widget_set_sensitive(entry, FALSE);
    
  entry = lookup_widget(sheets_edit_dialog, "entry_object_type");
  if (som)
    gtk_entry_set_text(GTK_ENTRY(entry), type);
  else
    gtk_widget_set_sensitive(entry, FALSE);

  sm = g_object_get_data(G_OBJECT(active_button), "sheet_mod");
  if (sm)
  {
    name = sm->sheet.name;
    descrip = sm->sheet.description;
  }

  entry = lookup_widget(sheets_edit_dialog, "entry_sheet_name");
  gtk_entry_set_text(GTK_ENTRY(entry), name);
#if CAN_EDIT_SHEET_NAME
  if (sm->sheet.scope == SHEET_SCOPE_SYSTEM)
#endif
    gtk_widget_set_sensitive(entry, FALSE);

  entry = lookup_widget(sheets_edit_dialog, "entry_sheet_description");
  gtk_entry_set_text(GTK_ENTRY(entry), descrip);

  gtk_widget_show(sheets_edit_dialog);
}

void
on_sheets_edit_dialog_button_cancel_clicked
                                        (GtkButton       *button,
                                        gpointer         user_data)
{
  gtk_widget_destroy(sheets_edit_dialog);
  sheets_edit_dialog = NULL;
}

static GtkWidget *sheets_remove_dialog;

typedef enum
{
  SHEETS_REMOVE_DIALOG_TYPE_OBJECT = 1,
  SHEETS_REMOVE_DIALOG_TYPE_SHEET
}
SheetsRemoveDialogType;

void
on_sheets_dialog_button_remove_clicked (GtkButton       *button,
                                        gpointer         user_data)
{
  GtkWidget *wrapbox;
  GList *button_list;
  GtkWidget *active_button;
  SheetMod *sm;
  GtkWidget *entry;
  GtkWidget *radio_button;

  sheets_remove_dialog = create_sheets_remove_dialog();

  active_button = sheets_dialog_get_active_button(&wrapbox, &button_list);
  g_assert(active_button);
  
  /* Force a change in state in the radio button for set sensitive */

  radio_button = lookup_widget(sheets_remove_dialog, "radiobutton_sheet");
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio_button), TRUE);

  entry = lookup_widget(sheets_remove_dialog, "entry_object");
  if (g_object_get_data(G_OBJECT(active_button), "is_hidden_button"))
  {
    radio_button = lookup_widget(sheets_remove_dialog, "radiobutton_object");
    gtk_widget_set_sensitive(radio_button, FALSE);
    gtk_widget_set_sensitive(entry, FALSE);
  }
  else
  {
    SheetObjectMod *som;
    
    som = g_object_get_data(G_OBJECT(active_button), "sheet_object_mod");
    if (!som)
      gtk_entry_set_text(GTK_ENTRY(entry), _("Line Break"));
    else
      gtk_entry_set_text(GTK_ENTRY(entry), som->sheet_object.description);

    radio_button = lookup_widget(sheets_remove_dialog, "radiobutton_object");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio_button), TRUE);
  }

  entry = lookup_widget(sheets_remove_dialog, "entry_sheet");
  sm = g_object_get_data(G_OBJECT(active_button), "sheet_mod");

  /* Note:  It is currently impossible to remove a user sheet that has
            been shadowed by a more recent system sheet--the logic is just
            too hairy.  Once the user sheet has been updated, or the [copy of
            system] sheet has been removed, then they can remove the user
            sheet just fine (in the next invocation of dia).  This would
            be rare, to say the least... */

  if (sm->sheet.shadowing == NULL && sm->sheet.scope == SHEET_SCOPE_SYSTEM)
  {
    GtkWidget *radio_button;

    radio_button = lookup_widget(sheets_remove_dialog, "radiobutton_sheet");
    gtk_widget_set_sensitive(radio_button, FALSE);
    gtk_widget_set_sensitive(entry, FALSE);
  }
  gtk_entry_set_text(GTK_ENTRY(entry), sm->sheet.name);
  
  gtk_widget_show(sheets_remove_dialog);
}

void
on_sheets_remove_dialog_button_cancel_clicked
                                        (GtkButton       *button,
                                        gpointer         user_data)
{
  gtk_widget_destroy(sheets_remove_dialog);
  sheets_remove_dialog = NULL;
}

static void
sheets_dialog_togglebutton_set_sensitive(GtkToggleButton *togglebutton,
                                         GtkWidget *dialog,
                                         gchar *widget_names[], gint type)
{
  gboolean is_sensitive;
  guint i;
  GtkWidget *tmp;

  is_sensitive = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(togglebutton));
  if (is_sensitive)
  {
    tmp = lookup_widget(dialog, "button_ok");
    g_object_set_data(G_OBJECT(tmp), "active_type", GINT_TO_POINTER(type));
  }

  for (i = 0; widget_names[i]; i++)
  {
    tmp = lookup_widget(dialog, widget_names[i]);
    gtk_widget_set_sensitive(tmp, is_sensitive);
  }
}

void
on_sheets_new_dialog_radiobutton_svg_shape_toggled
                                        (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
  static gchar *widget_names[] =
  {
    "file_chooser_button",
    "entry_svg_description",
    NULL
  };

  sheets_dialog_togglebutton_set_sensitive(togglebutton, sheets_new_dialog,
                                           widget_names,
                                           SHEETS_NEW_DIALOG_TYPE_SVG_SHAPE);
}

void
on_sheets_new_dialog_radiobutton_sheet_toggled
                                        (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
  static gchar *widget_names[] =
  {
    "entry_sheet_name",
    "entry_sheet_description",
    "label_description", 
    NULL
  };

  sheets_dialog_togglebutton_set_sensitive(togglebutton, sheets_new_dialog,
                                           widget_names, 
                                           SHEETS_NEW_DIALOG_TYPE_SHEET);
}

void
on_sheets_new_dialog_radiobutton_line_break_toggled
                                        (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
  static gchar *widget_names[] =
  {
    NULL
  };

  sheets_dialog_togglebutton_set_sensitive(togglebutton, sheets_new_dialog,
                                           widget_names,
                                           SHEETS_NEW_DIALOG_TYPE_LINE_BREAK);
}

void
on_sheets_remove_dialog_radiobutton_object_toggled
                                        (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
  gchar *widget_names[] =
  {
    "pixmap_object",
    "entry_object",
    NULL
  };

  sheets_dialog_togglebutton_set_sensitive(togglebutton, sheets_remove_dialog,
                                           widget_names,
                                           SHEETS_REMOVE_DIALOG_TYPE_OBJECT);
}

void
on_sheets_remove_dialog_radiobutton_sheet_toggled
                                        (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
  gchar *widget_names[] =
  {
    "entry_sheet",
    NULL
  };

  sheets_dialog_togglebutton_set_sensitive(togglebutton, sheets_remove_dialog,
                                           widget_names,
                                           SHEETS_REMOVE_DIALOG_TYPE_SHEET);
}

static GtkWidget *
sheets_dialog_set_new_active_button(void)
{
  GtkWidget *active_button;
  GtkWidget *wrapbox;
  GList *button_list;
  GList *active_button_list;
  GtkWidget *new_active_button;

  active_button = sheets_dialog_get_active_button(&wrapbox, &button_list);

  active_button_list = g_list_find(button_list, active_button);
  g_assert(active_button_list);
  if (g_list_next(active_button_list))
    new_active_button = g_list_next(active_button_list)->data;
  else
  {
    if (g_list_previous(active_button_list))
      new_active_button = g_list_previous(active_button_list)->data;
    else
      new_active_button = g_object_get_data(G_OBJECT(wrapbox),
                                            "hidden_button");
  }
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(new_active_button), TRUE);

  return new_active_button;
}

void
on_sheets_remove_dialog_button_ok_clicked
                                        (GtkButton       *button,
                                        gpointer         user_data)
{
  SheetsRemoveDialogType type;
  GtkWidget *wrapbox;
  GList *button_list;
  GtkWidget *active_button;

  type = (SheetsRemoveDialogType)g_object_get_data(G_OBJECT(button),
                                                   "active_type");
                                                     
  active_button = sheets_dialog_get_active_button(&wrapbox, &button_list);

  switch (type)
  {
    SheetObjectMod *som;
    GtkWidget *new_active_button;
    SheetMod *sm;
    GtkWidget *table_sheets;
    GtkWidget *optionmenu;

  case SHEETS_REMOVE_DIALOG_TYPE_OBJECT:
  
    som = g_object_get_data(G_OBJECT(active_button), "sheet_object_mod");
    if (som)
    {
      sm = g_object_get_data(G_OBJECT(active_button), "sheet_mod");
      sm->mod = SHEETMOD_MOD_CHANGED;
      som->mod = SHEET_OBJECT_MOD_DELETED;
    }
    new_active_button = sheets_dialog_set_new_active_button();

    gtk_widget_destroy(active_button);
    radio_group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(new_active_button));

    sheets_dialog_normalize_line_breaks(wrapbox, SHEETS_DIALOG_MOVE_NONE);
    break;
    
  case SHEETS_REMOVE_DIALOG_TYPE_SHEET:

    sm = g_object_get_data(G_OBJECT(active_button), "sheet_mod");
    sm->mod = SHEETMOD_MOD_DELETED;

    if (sm->sheet.shadowing && sm->sheet.scope == SHEET_SCOPE_USER)
      sheets_append_sheet_mods(sm->sheet.shadowing);

    table_sheets = lookup_widget(sheets_dialog, "table_sheets");
    optionmenu = g_object_get_data(G_OBJECT(table_sheets),
                                   "active_optionmenu");
    g_assert(optionmenu);
    sheets_optionmenu_create(optionmenu, wrapbox, NULL);

    break;

  default:
    g_assert_not_reached();
  }

  g_list_free(button_list);

  sheets_dialog_apply_revert_set_sensitive(TRUE);
  gtk_widget_destroy(sheets_remove_dialog);
  sheets_remove_dialog = NULL;
}

void
on_sheets_edit_dialog_button_ok_clicked
                                        (GtkButton       *button,
                                        gpointer         user_data)
{
  GtkWidget *active_button;
  GtkWidget *wrapbox;
  GList *button_list;
  GtkWidget *entry;
  gboolean something_changed = FALSE;

  active_button = sheets_dialog_get_active_button(&wrapbox, &button_list);
  g_assert(active_button);

  entry = lookup_widget(sheets_edit_dialog, "entry_object_description");
  if (GPOINTER_TO_INT(g_object_get_data(G_OBJECT(entry), "changed")) != 0)
  {
    SheetMod *sm;
    SheetObjectMod *som;

    som = g_object_get_data(G_OBJECT(active_button), "sheet_object_mod");
    som->sheet_object.description = 
      gtk_editable_get_chars(GTK_EDITABLE(entry), 0, -1);
    sheets_dialog_object_set_tooltip(som, active_button);

    som->mod = SHEET_OBJECT_MOD_CHANGED;

    sm = g_object_get_data(G_OBJECT(active_button), "sheet_mod");
    if (sm->mod == SHEETMOD_MOD_NONE)
      sm->mod = SHEETMOD_MOD_CHANGED;
    
    something_changed = TRUE;
  }

  entry = lookup_widget(sheets_edit_dialog, "entry_sheet_description");
  if (GPOINTER_TO_INT(g_object_get_data(G_OBJECT(entry), "changed")) != 0)
  {
    SheetMod *sm;

    sm = g_object_get_data(G_OBJECT(active_button), "sheet_mod");
    sm->sheet.description = 
      gtk_editable_get_chars(GTK_EDITABLE(entry), 0, -1);

    if (sm->mod == SHEETMOD_MOD_NONE)
      sm->mod = SHEETMOD_MOD_CHANGED;
    something_changed = TRUE;
  }
  
#ifdef CAN_EDIT_SHEET_NAME
  /* This must be last because we reload the sheets if changed */
  
  entry = lookup_widget(sheets_edit_dialog, "entry_sheet_name");
  if ((gboolean)g_object_get_data(G_OBJECT(entry), "changed") == TRUE)
  {
    SheetMod *sm;
    GtkWidget *table_sheets;
    GtkWidget *optionmenu;

    sm = g_object_get_data(G_OBJECT(active_button), "sheet_mod");
    sm->sheet.name = gtk_editable_get_chars(GTK_EDITABLE(entry), 0, -1);
    if (sm->mod == SHEETMOD_MOD_NONE)
      sm->mod = SHEETMOD_MOD_CHANGED;

    table_sheets = lookup_widget(sheets_dialog, "table_sheets");
    optionmenu = g_object_get_data(G_OBJECT(table_sheets),
                                   "active_optionmenu");
    sheets_optionmenu_create(optionmenu, wrapbox, NULL);
    
    something_changed = TRUE;
  }
#endif
  if (something_changed == TRUE)
    sheets_dialog_apply_revert_set_sensitive(TRUE);

  gtk_widget_destroy(sheets_edit_dialog);
  sheets_edit_dialog = NULL;
}

void
on_sheets_edit_dialog_entry_object_description_changed
                                        (GtkEditable     *editable,
                                        gpointer         user_data)
{
  g_object_set_data(G_OBJECT(editable), "changed", (gpointer)TRUE);
}


void
on_sheets_edit_dialog_entry_sheet_description_changed
                                        (GtkEditable     *editable,
                                        gpointer         user_data)
{
  g_object_set_data(G_OBJECT(editable), "changed", (gpointer)TRUE);
}

void
on_sheets_edit_dialog_entry_sheet_name_changed
                                        (GtkEditable     *editable,
                                        gpointer         user_data)
{
  g_object_set_data(G_OBJECT(editable), "changed", (gpointer)TRUE);
}

static GtkWidget *
sheets_dialog_get_target_wrapbox(GtkWidget *wrapbox)
{
  if (g_object_get_data(G_OBJECT(wrapbox), "is_left"))
    return g_object_get_data(G_OBJECT(sheets_dialog), "wrapbox_right");
  else
    return g_object_get_data(G_OBJECT(sheets_dialog), "wrapbox_left");
}

static void
sheets_dialog_copy_object(GtkWidget *active_button, GtkWidget *target_wrapbox)
{
  GtkWidget *button;
  SheetMod *sm;
  SheetObjectMod *som;
  SheetObjectMod *som_new;

  sm = g_object_get_data(G_OBJECT(target_wrapbox), "sheet_mod");
  som = g_object_get_data(G_OBJECT(active_button), "sheet_object_mod");

  if (!som)
    return;


  som_new = g_new0(SheetObjectMod, 1);
  {
    SheetObject *so = &som_new->sheet_object;

    so->object_type = g_strdup(som->sheet_object.object_type);
    so->description = g_strdup(som->sheet_object.description);
    so->pixmap = som->sheet_object.pixmap;
    so->user_data = som->sheet_object.user_data;
    so->user_data_type = som->sheet_object.user_data_type;
    so->line_break = FALSE;          /* must be false--we don't copy linebreaks */
    so->pixmap_file = g_strdup(som->sheet_object.pixmap_file);
    so->has_icon_on_sheet = som->sheet_object.has_icon_on_sheet;
  }

  som_new->type = som->type;
  som_new->mod = SHEET_OBJECT_MOD_NONE;

  sm->sheet.objects = g_slist_append(sm->sheet.objects, som_new);
  if (sm->mod == SHEETMOD_MOD_NONE)
    sm->mod = SHEETMOD_MOD_CHANGED;

  button = sheets_dialog_create_object_button(som_new, sm, target_wrapbox);
  gtk_wrap_box_pack(GTK_WRAP_BOX(target_wrapbox), button,
                    FALSE, TRUE, FALSE, TRUE);
  gtk_widget_show(button);

  sheets_dialog_apply_revert_set_sensitive(TRUE);
}

void
on_sheets_dialog_button_copy_clicked   (GtkButton       *button,
                                        gpointer         user_data)
{
  GtkWidget *active_button;
  GtkWidget *wrapbox;
  GList *button_list;
  GtkWidget *target_wrapbox;

  active_button = sheets_dialog_get_active_button(&wrapbox, &button_list);

  target_wrapbox = sheets_dialog_get_target_wrapbox(wrapbox);

  sheets_dialog_copy_object(active_button, target_wrapbox);
}

void
on_sheets_dialog_button_copy_all_clicked
                                        (GtkButton       *button,
                                        gpointer         user_data)
{
  GtkWidget *wrapbox;
  GList *button_list, *iter_list;
  GtkWidget *target_wrapbox;

  sheets_dialog_get_active_button(&wrapbox, &button_list);

  target_wrapbox = sheets_dialog_get_target_wrapbox(wrapbox);

  for (iter_list = button_list; iter_list; iter_list = g_list_next(iter_list))
    sheets_dialog_copy_object(iter_list->data, target_wrapbox);
}

void
on_sheets_dialog_button_move_clicked   (GtkButton       *button,
                                        gpointer         user_data)
{
  GtkWidget *active_button;
  GtkWidget *wrapbox;
  GList *button_list;
  GtkWidget *target_wrapbox;
  SheetObjectMod *som;
  GtkWidget *new_active_button;

  active_button = sheets_dialog_get_active_button(&wrapbox, &button_list);

  target_wrapbox = sheets_dialog_get_target_wrapbox(wrapbox);

  sheets_dialog_copy_object(active_button, target_wrapbox);

  som = g_object_get_data(G_OBJECT(active_button), "sheet_object_mod");
  if (som) {
    SheetMod *sm;

    som->mod = SHEET_OBJECT_MOD_DELETED;
    /* also mark the source sheet as changed */
    sm = g_object_get_data(G_OBJECT(active_button), "sheet_mod");
    sm->mod = SHEETMOD_MOD_CHANGED;
  }

  new_active_button = sheets_dialog_set_new_active_button();

  gtk_widget_destroy(active_button);
  radio_group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(new_active_button));

  sheets_dialog_normalize_line_breaks(wrapbox, SHEETS_DIALOG_MOVE_NONE);
}

void
on_sheets_dialog_button_move_all_clicked
                                        (GtkButton       *button,
                                        gpointer         user_data)
{
  GtkWidget *wrapbox;
  GList *button_list;
  GList *iter_list;
  GtkWidget *target_wrapbox;
  SheetObjectMod *som;

  sheets_dialog_get_active_button(&wrapbox, &button_list);

  target_wrapbox = sheets_dialog_get_target_wrapbox(wrapbox);

  for (iter_list = button_list; iter_list; iter_list = g_list_next(iter_list))
  {
    sheets_dialog_copy_object(iter_list->data, target_wrapbox);

    som = g_object_get_data(G_OBJECT(iter_list->data), "sheet_object_mod");
    if (som)
      som->mod = SHEET_OBJECT_MOD_DELETED;

    gtk_widget_destroy(iter_list->data);

    /* MCNFIXME:  do we have to resanitize the radio_group? */
  }
  
  /* Force the 1st button in the target wrapbox to be active after moving */

  button_list = gtk_container_get_children(GTK_CONTAINER(target_wrapbox));
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button_list->data), TRUE);
}

static gboolean
copy_file(gchar *src, gchar *dst)
{
  FILE *fp_src;
  FILE *fp_dst;
  int c;

  if ((fp_src = g_fopen(src, "rb")) == NULL)
  {
    message_error(_("Couldn't open '%s': %s"), 
		  dia_message_filename(src), strerror(errno));
    return FALSE;
  }
 
  if ((fp_dst = g_fopen(dst, "wb")) == NULL)
  {
    message_error(_("Couldn't open '%s': %s"), 
		  dia_message_filename(dst), strerror(errno));
    return FALSE;
  }

  while ((c = fgetc(fp_src)) != EOF)
    fputc(c, fp_dst);

  fclose(fp_src);
  fclose(fp_dst);

  return TRUE;
}

/* write a sheet to ~/.dia/sheets */
static gboolean
write_user_sheet(Sheet *sheet)
{
  FILE *file;
  xmlDocPtr doc;
  xmlNodePtr root;
  xmlNodePtr node;
  xmlNodePtr object_node;
  xmlNodePtr desc_node;
  xmlNodePtr icon_node;
  gchar buf[512];
  time_t time_now;
  const char *username;
  gchar *dir_user_sheets;
  gchar *filename;
  SheetObject *sheetobject;
  GSList *sheet_objects;
    
  dir_user_sheets = dia_config_filename("sheets");
  if (!*(sheet->filename)) {
    gchar *basename;

    basename = g_strdup(sheet->name);
    basename = g_strdelimit(basename, G_STR_DELIMITERS G_DIR_SEPARATOR_S, '_');
    filename = g_strdup_printf("%s%s%s.sheet", dir_user_sheets,
                               G_DIR_SEPARATOR_S, basename);
    g_free(basename);
  } else {
    gchar *basename;

    basename = g_path_get_basename(sheet->filename);
    filename = g_strdup_printf("%s%s%s", dir_user_sheets,
                               G_DIR_SEPARATOR_S, basename);
    g_free(basename);
  }
  file = g_fopen(filename, "w");
   
  if (file==NULL)
  {
    message_error(_("Couldn't open: '%s' for writing"), 
		  dia_message_filename(filename));
    g_free(filename);
    return FALSE;
  }
  fclose(file);

  time_now = time(NULL);
  username = g_get_user_name();
  if (username==NULL)
    username = _("a user");

  doc = xmlNewDoc((const xmlChar *)"1.0");
  doc->encoding = xmlStrdup((const xmlChar *)"UTF-8");
  doc->standalone = FALSE;
  root = xmlNewDocNode(doc, NULL, (const xmlChar *)"sheet", NULL);
  doc->xmlRootNode = root;
  xmlSetProp(root, (const xmlChar *)"xmlns", (const xmlChar *) DIA_XML_NAME_SPACE_BASE "dia-sheet-ns");

  /* comments */
  xmlAddChild(root, xmlNewText((const xmlChar *)"\n"));
  xmlAddChild(root, xmlNewComment((const xmlChar *)"Dia-Version: "VERSION));
  xmlAddChild(root, xmlNewText((const xmlChar *)"\n"));
  g_snprintf(buf, sizeof(buf), _("File: %s"), filename);
  xmlAddChild(root, xmlNewComment((xmlChar *)buf));
  xmlAddChild(root, xmlNewText((const xmlChar *)"\n"));
  g_snprintf(buf, sizeof(buf), _("Date: %s"), ctime(&time_now));
  buf[strlen(buf)-1] = '\0'; /* remove the trailing new line */
  xmlAddChild(root, xmlNewComment((xmlChar *)buf));
  xmlAddChild(root, xmlNewText((const xmlChar *)"\n"));
  g_snprintf(buf, sizeof(buf), _("For: %s"), username);
  xmlAddChild(root, xmlNewComment((xmlChar *)buf));
  xmlAddChild(root, xmlNewText((const xmlChar *)"\n\n"));

  /* sheet name */
  node = xmlNewChild(root, NULL, (const xmlChar *)"name", NULL);
  xmlAddChild(node, xmlNewText((xmlChar *)sheet->name));
  xmlAddChild(root, xmlNewText((const xmlChar *)"\n"));

  /* sheet description */
  node = xmlNewChild(root, NULL, (const xmlChar *)"description", NULL);
  xmlAddChild(node, xmlNewText((xmlChar *)sheet->description));
  xmlAddChild(root, xmlNewText((const xmlChar *)"\n"));

  /* content */
  node = xmlNewChild(root, NULL, (const xmlChar *)"contents", NULL);
  xmlAddChild(node, xmlNewText((const xmlChar *)"\n"));
  xmlAddChild(node, xmlNewComment((const xmlChar *) _("add shapes here")));
  xmlAddChild(node, xmlNewText((const xmlChar *)"\n"));

  /* objects */
  for (sheet_objects = sheet->objects; sheet_objects; 
       sheet_objects = g_slist_next(sheet_objects))
  {
    SheetObjectMod *som;

    som = sheet_objects->data;

    if (som->mod == SHEET_OBJECT_MOD_DELETED)
      continue;

    /* If its a new shape, then copy the .shape and .xpm files from
       their current location to ~/.dia/shapes/ */
       
    if (som->mod == SHEET_OBJECT_MOD_NEW)
    {
      gchar *dia_user_shapes;
      gchar *dest;
      gchar *basename;

      dia_user_shapes = dia_config_filename("shapes");

      basename = g_path_get_basename(som->svg_filename);
      dest = g_strdup_printf("%s%s%s", dia_user_shapes, G_DIR_SEPARATOR_S, basename);
      g_free(basename);
      copy_file(som->svg_filename, dest);
      g_free(dest);

      /* avoid crashing when there is no icon */
      if (som->sheet_object.pixmap_file) {
	basename = g_path_get_basename(som->sheet_object.pixmap_file);
	dest = g_strdup_printf("%s%s%s", dia_user_shapes, G_DIR_SEPARATOR_S, basename);
	g_free(basename);
	copy_file(som->sheet_object.pixmap_file, dest);
	g_free(dest);
      }
    }

    sheetobject = &som->sheet_object;
    object_node = xmlNewChild(node, NULL, (const xmlChar *)"object", NULL);
    
    xmlSetProp(object_node, (const xmlChar *)"name", (xmlChar *)sheetobject->object_type);
    
    if (sheetobject->user_data_type == USER_DATA_IS_INTDATA)
    {
      gchar *user_data;

      user_data = g_strdup_printf("%u", GPOINTER_TO_UINT(sheetobject->user_data));
      xmlSetProp(object_node, (const xmlChar *)"intdata", (xmlChar *) user_data);
      g_free(user_data);
    }
      
    xmlAddChild(object_node, xmlNewText((const xmlChar *)"\n"));
    desc_node = xmlNewChild(object_node, NULL, (const xmlChar *)"description", NULL);
    xmlAddChild(desc_node, xmlNewText((xmlChar *) sheetobject->description));
    /*    xmlAddChild(object_node, xmlNewText((const xmlChar *)"\n")); */

    if (sheetobject->has_icon_on_sheet == TRUE)
    {
      gchar *canonical_icon;
      gchar *canonical_user_sheets;
      gchar *canonical_sheets;
      gchar *icon;

      xmlAddChild(object_node, xmlNewText((const xmlChar *)"\n"));
      icon_node = xmlNewChild(object_node, NULL, (const xmlChar *)"icon", NULL);
      canonical_icon = dia_get_canonical_path(sheetobject->pixmap_file);
      icon = canonical_icon;
      canonical_user_sheets = dia_get_canonical_path(dir_user_sheets);
      canonical_sheets = dia_get_canonical_path(dia_get_data_directory("sheets"));
      if(g_str_has_prefix(icon, canonical_user_sheets))
        icon += strlen(canonical_user_sheets) + 1;
      if(g_str_has_prefix(icon, canonical_sheets))
        icon += strlen(canonical_sheets) + 1; 
      xmlAddChild(icon_node, xmlNewText((xmlChar *) icon));
      xmlAddChild(object_node, xmlNewText((const xmlChar *)"\n"));
      g_free(canonical_icon);
      g_free(canonical_user_sheets);
      g_free(canonical_sheets);
    }
  }
  xmlSetDocCompressMode(doc, 0);
  xmlDiaSaveFile(filename, doc);
  xmlFreeDoc(doc);
  return TRUE;
}

static void
touch_file(gchar *filename)
{
  struct stat stat_buf;
  struct utimbuf utim_buf;

  g_stat(filename, &stat_buf);
  utim_buf.actime = stat_buf.st_atime;
  utim_buf.modtime = time(NULL);
  g_utime(filename, &utim_buf);
}

static gint
sheets_find_sheet(gconstpointer a, gconstpointer b)
{
  if (!strcmp(((Sheet *)a)->name, ((Sheet *)b)->name))
    return 0;
  else
    return 1;
}

extern GSList *sheets_mods_list;

void
on_sheets_dialog_button_apply_clicked  (GtkButton       *button,
                                        gpointer         user_data)
{
  GSList *iter_list;

  for (iter_list = sheets_mods_list; iter_list;
       iter_list = g_slist_next(iter_list))
  {
    SheetMod *sm;
    GSList *sheets_list;
    GSList *find_list;

    sm = iter_list->data;
    switch (sm->mod)
    {
      GSList *sheet_object_mods_list;
      GSList *list;
      Sheet *new_sheet = NULL;

    case SHEETMOD_MOD_NEW:
      write_user_sheet(&sm->sheet);

      sheet_object_mods_list = sm->sheet.objects;
      sm->sheet.objects = NULL;
      /* we have to transfer 'permanent' memory */
      new_sheet = g_new0 (Sheet, 1);
      *new_sheet = sm->sheet;
      register_sheet(new_sheet);

      for (list = sheet_object_mods_list; list; list = g_slist_next(list))
      {
        SheetObjectMod *som;
	SheetObject *new_object;

        som = list->data;
        if (som->mod == SHEET_OBJECT_MOD_DELETED)
          continue;

	new_object = g_new0(SheetObject, 1);
	*new_object = som->sheet_object;
        sheet_append_sheet_obj(new_sheet, new_object);
      }

      dia_sort_sheets();      
      fill_sheet_menu();
      break;

    case SHEETMOD_MOD_CHANGED:
      write_user_sheet(&sm->sheet);

      sheet_object_mods_list = sm->sheet.objects;
      sheets_list = get_sheets_list();

      sheets_list = g_slist_find_custom(sheets_list, &sm->sheet,
                                        sheets_find_sheet);
      g_assert(sheets_list);
      ((Sheet *)(sheets_list->data))->objects = NULL;

      for (list = sheet_object_mods_list; list; list = g_slist_next(list))
      {
        SheetObjectMod *som;
	SheetObject *new_object;

        som = list->data;
        if (som->mod == SHEET_OBJECT_MOD_DELETED)
          continue;

	new_object = g_new0(SheetObject, 1);
	*new_object = som->sheet_object;
        sheet_append_sheet_obj(sheets_list->data, new_object);
      }
      fill_sheet_menu();
      break;

    case SHEETMOD_MOD_DELETED:
      if (sm->sheet.scope == SHEET_SCOPE_SYSTEM)
        touch_file(sm->sheet.shadowing->filename);
      else
        g_unlink(sm->sheet.filename);

      sheets_list = get_sheets_list();
      find_list = g_slist_find_custom(sheets_list, &sm->sheet,
                                        sheets_find_sheet);
      g_assert(sheets_list);
      if (!g_slist_remove_link(sheets_list, find_list))
	g_warning ("No sheets left?");

      dia_sort_sheets();
      fill_sheet_menu();
      break;

    case SHEETMOD_MOD_NONE:
      break;

    default:
      g_assert_not_reached();
    }
  }
  {
    optionmenu_activate_first_pass = TRUE;

    sheets_dialog_apply_revert_set_sensitive(FALSE);
    sheets_dialog_create();
  }
}

void
on_sheets_dialog_button_revert_clicked (GtkButton       *button,
                                        gpointer         user_data)
{
  optionmenu_activate_first_pass = TRUE;

  sheets_dialog_apply_revert_set_sensitive(FALSE);
  sheets_dialog_create();
}

