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
#  define utime(n,b) _(n,b)
#  define utim_buf _utimbuf
#endif
#endif

#include <gmodule.h>

#ifdef GNOME
#include <gnome.h>
#else
#include <gtk/gtk.h>
#endif

#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xmlmemory.h>

#include "gtkwrapbox.h"

#include "../lib/dia_dirs.h"
#include "../lib/dia_xml_libxml.h"
#include "../lib/plug-ins.h"

#include "interface.h"
#include "message.h"
#include "sheets.h"
#include "sheets_dialog_callbacks.h"
#include "sheets_dialog.h"

#include "pixmaps/line_break.xpm"

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


gboolean
on_sheets_main_dialog_destroy_event    (GtkWidget       *widget,
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

  button = lookup_widget(sheets_dialog, "button_move_up");

  /* If the active button is the 1st in the wrapbox, OR
     if the active button is a linebreak AND is 2nd in the wrapbox
     THEN set the 'Up' button to insensitive */

  if (GTK_TOGGLE_BUTTON(g_list_first(wrapbox_button_list)->data) == togglebutton
      || (!gtk_object_get_data(GTK_OBJECT(togglebutton), "sheet_object_mod")
          && g_list_nth(wrapbox_button_list, 1)
          && GTK_TOGGLE_BUTTON(g_list_nth(wrapbox_button_list, 1)->data)
             == togglebutton))
    gtk_widget_set_sensitive(button, FALSE);
  else
    gtk_widget_set_sensitive(button, TRUE);

  button = lookup_widget(sheets_dialog, "button_move_down");
  if (GTK_TOGGLE_BUTTON(g_list_last(wrapbox_button_list)->data) == togglebutton
      || (!gtk_object_get_data(GTK_OBJECT(togglebutton), "sheet_object_mod")
          && g_list_previous(g_list_last(wrapbox_button_list))
          && GTK_TOGGLE_BUTTON(g_list_previous(g_list_last(wrapbox_button_list))
                              ->data)
             == togglebutton))
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

  gtk_object_set_data(GTK_OBJECT(ud_wrapbox), "active_button", togglebutton);

  /* Same with the active wrapbox */

  table_sheets = lookup_widget(sheets_dialog, "table_sheets");
  gtk_object_set_data(GTK_OBJECT(table_sheets), "active_wrapbox", ud_wrapbox);

  optionmenu_left = lookup_widget(sheets_dialog, "optionmenu_left");
  sheet_left = gtk_object_get_data(GTK_OBJECT(optionmenu_left),
                                   "active_sheet_name");

  optionmenu_right = lookup_widget(sheets_dialog, "optionmenu_right");
  sheet_right = gtk_object_get_data(GTK_OBJECT(optionmenu_right),
                                    "active_sheet_name");

  if ((gboolean)gtk_object_get_data(GTK_OBJECT(ud_wrapbox), "is_left") ==TRUE)
  {
    gtk_object_set_data(GTK_OBJECT(table_sheets), "active_optionmenu",
                        optionmenu_left);
    button = lookup_widget(sheets_dialog, "button_copy");
    gtk_object_set(GTK_OBJECT(button), "label", "Copy ->", NULL);
    button = lookup_widget(sheets_dialog, "button_copy_all");
    gtk_object_set(GTK_OBJECT(button), "label", "Copy All ->", NULL);
    button = lookup_widget(sheets_dialog, "button_move");
    gtk_object_set(GTK_OBJECT(button), "label", "Move ->", NULL);
    button = lookup_widget(sheets_dialog, "button_move_all");
    gtk_object_set(GTK_OBJECT(button), "label", "Move All ->", NULL);
  }
  else
  {
    gtk_object_set_data(GTK_OBJECT(table_sheets), "active_optionmenu",
                        optionmenu_right);
    button = lookup_widget(sheets_dialog, "button_copy");
    gtk_object_set(GTK_OBJECT(button), "label", "<- Copy", NULL);
    button = lookup_widget(sheets_dialog, "button_copy_all");
    gtk_object_set(GTK_OBJECT(button), "label", "<- Copy All", NULL);
    button = lookup_widget(sheets_dialog, "button_move");
    gtk_object_set(GTK_OBJECT(button), "label", "<- Move", NULL);
    button = lookup_widget(sheets_dialog, "button_move_all");
    gtk_object_set(GTK_OBJECT(button), "label", "<- Move All", NULL);
  }

  sheet_left = sheet_left ? sheet_left : "";  /* initial value can be NULL */

  if (!strcmp(sheet_left, sheet_right)
      || gtk_object_get_data(GTK_OBJECT(togglebutton), "is_hidden_button")
      || !gtk_object_get_data(GTK_OBJECT(togglebutton), "sheet_object_mod"))
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

  wrapbox_button_list = gtk_container_children(GTK_CONTAINER(ud_wrapbox));

  if (g_list_length(wrapbox_button_list))
  {
    SheetMod *sm;

    sheets_dialog_up_down_set_sensitive(wrapbox_button_list, togglebutton);

    sm = gtk_object_get_data(GTK_OBJECT(togglebutton), "sheet_mod");

    button = lookup_widget(sheets_dialog, "button_edit");
    if (!gtk_object_get_data(GTK_OBJECT(togglebutton), "sheet_object_mod")
        && sm->sheet.scope == SHEET_SCOPE_SYSTEM)
      gtk_widget_set_sensitive(button, FALSE);
    else
      gtk_widget_set_sensitive(button, TRUE);

    button = lookup_widget(sheets_dialog, "button_remove");
    if (gtk_object_get_data(GTK_OBJECT(togglebutton), "is_hidden_button")
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

  sm = gtk_object_get_data(GTK_OBJECT(wrapbox), "sheet_mod");
  g_assert(sm);

  button = gtk_radio_button_new(radio_group);
  gtk_button_set_relief(GTK_BUTTON(button), GTK_RELIEF_NONE);

  gtk_object_set_data(GTK_OBJECT(button), "sheet_mod", sm);

  radio_group = gtk_radio_button_group(GTK_RADIO_BUTTON(button));

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

  gtk_tooltips_set_tip(sheets_dialog_tooltips, button, "Line Break", NULL);

  gtk_signal_connect(GTK_OBJECT(button), "toggled",
                     GTK_SIGNAL_FUNC(on_sheets_dialog_object_button_toggled),
                     wrapbox);
}

static void
sheets_dialog_object_set_tooltip(SheetObjectMod *som, GtkWidget *button)
{
  gchar *tip;

  tip = g_strdup_printf("%s\n%s", som->sheet_object.description,
                        (som->type == OBJECT_TYPE_SVG) ? "SVG Shape" :
                                                         "Programmed Object");
  gtk_tooltips_set_tip(sheets_dialog_tooltips, button, tip, NULL);
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
  radio_group = gtk_radio_button_group(GTK_RADIO_BUTTON(button));

  gtk_toggle_button_set_mode(GTK_TOGGLE_BUTTON(button), FALSE);
  gtk_container_set_border_width(GTK_CONTAINER(button), 0);

  create_object_pixmap(&som->sheet_object, wrapbox, &pixmap, &mask);
  gtkpixmap = gtk_pixmap_new(pixmap, mask);
  gtk_container_add(GTK_CONTAINER(button), gtkpixmap);
  gtk_widget_show(gtkpixmap);

  sheets_dialog_object_set_tooltip(som, button);

  gtk_object_set_data(GTK_OBJECT(button), "sheet_mod", sm);
  gtk_object_set_data(GTK_OBJECT(button), "sheet_object_mod", som);

  gtk_signal_connect(GTK_OBJECT(button), "toggled",
                     GTK_SIGNAL_FUNC(on_sheets_dialog_object_button_toggled),
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
 
  wrapbox = gtk_object_get_data(GTK_OBJECT(menuitem), "wrapbox");
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
    radio_group = gtk_radio_button_group(GTK_RADIO_BUTTON(hidden_button));
  }

  gtk_signal_connect(GTK_OBJECT(hidden_button), "toggled",
                     GTK_SIGNAL_FUNC(on_sheets_dialog_object_button_toggled),
                     wrapbox);
  gtk_object_set_data(GTK_OBJECT(hidden_button), "is_hidden_button",
                      (gpointer)TRUE);
  gtk_object_set_data(GTK_OBJECT(wrapbox), "hidden_button", hidden_button);
  gtk_object_set_data(GTK_OBJECT(hidden_button), "sheet_mod", user_data);

  if (gtk_object_get_data(GTK_OBJECT(wrapbox), "is_left"))
    optionmenu = lookup_widget(sheets_dialog, "optionmenu_left");
  else
    optionmenu = lookup_widget(sheets_dialog, "optionmenu_right");
  gtk_object_set_data(GTK_OBJECT(optionmenu), "active_sheet_name", sheet->name);

  gtk_container_foreach(GTK_CONTAINER(wrapbox),
                        (GtkCallback)gtk_widget_destroy, NULL);

  radio_group = gtk_radio_button_group(GTK_RADIO_BUTTON(hidden_button));

  gtk_wrap_box_set_aspect_ratio(GTK_WRAP_BOX(wrapbox), 4 * 1.0 / 9);
                                                /* MCNFIXME: calculate this */
#ifdef CHANGE_COLOUR_OF_MODIFIED_SHEETS
  {
    GtkRcStyle *style;

    style->fg[0].red = style->bg[0].red = 56000;
    style->fg[1].red = style->bg[1].red = 56000;
    style->fg[2].red = style->bg[2].red = 56000;
    style->fg[3].red = style->bg[3].red = 56000;
    style->fg[4].red = style->bg[4].red = 56000;
    style->color_flags[0] |= 0x0f;
    style->color_flags[1] |= 0x0f;
    style->color_flags[2] |= 0x0f;
    style->color_flags[3] |= 0x0f;
    style->color_flags[4] |= 0x0f;

    sw = lookup_widget(sheets_dialog, "scrolledwindow_right");
    gtk_widget_modify_style(wrapbox, style);
  }
#endif

  gtk_object_set_data(GTK_OBJECT(wrapbox), "sheet_mod", user_data);

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
   
    gtk_wrap_box_pack(GTK_WRAP_BOX(wrapbox), button, FALSE, TRUE, FALSE, TRUE);

    if (som->sheet_object.line_break == TRUE)
      gtk_wrap_box_set_child_forced_break(GTK_WRAP_BOX(wrapbox), button, TRUE);

    gtk_widget_show(button);
  }

  button_list = gtk_container_children(GTK_CONTAINER(wrapbox));

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
sheets_dialog_normalize_line_breaks(GtkWidget *wrapbox, SheetsDialogMoveDir dir)
{
  GList *button_list;
  GList *iter_list;
  gboolean is_line_break;

  button_list = gtk_container_children(GTK_CONTAINER(wrapbox));

  is_line_break = FALSE;
  for (iter_list = button_list; iter_list; iter_list = g_list_next(iter_list))
  {
    SheetMod *sm;
    SheetObjectMod *som;

    sm = gtk_object_get_data(GTK_OBJECT(iter_list->data), "sheet_mod");
    som = gtk_object_get_data(GTK_OBJECT(iter_list->data), "sheet_object_mod");
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
        gtk_wrap_box_set_child_forced_break(GTK_WRAP_BOX(wrapbox),
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
        gtk_wrap_box_set_child_forced_break(GTK_WRAP_BOX(wrapbox),
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
          radio_group = gtk_radio_button_group(GTK_RADIO_BUTTON(iter_list
                                                                       ->data));
        }
        else
        {
          GList *tmp_list;

          tmp_list = g_list_previous(iter_list);
          gtk_widget_destroy(GTK_WIDGET(iter_list->data));
          radio_group = gtk_radio_button_group(GTK_RADIO_BUTTON(tmp_list
                                                                       ->data));
        }
      }
      else
        is_line_break = TRUE;
    }
  }

  /* We delete a trailing linebreak button in the wrapbox */

  iter_list = g_list_last(button_list);
  if (iter_list && !gtk_object_get_data(GTK_OBJECT(iter_list->data),
                                        "sheet_object_mod"))
  {
    gtk_widget_destroy(GTK_WIDGET(iter_list->data));
    if (g_list_previous(iter_list))
    {
      gtk_toggle_button_set_active(g_list_previous(iter_list)->data, TRUE);
      radio_group = gtk_radio_button_group(GTK_RADIO_BUTTON(
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
  *wrapbox = gtk_object_get_data(GTK_OBJECT(table_sheets), "active_wrapbox");
  *button_list = gtk_container_children(GTK_CONTAINER(*wrapbox));
  return gtk_object_get_data(GTK_OBJECT(*wrapbox), "active_button");
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
  wrapbox = gtk_object_get_data(GTK_OBJECT(table_sheets), "active_wrapbox");

  button_list = gtk_container_children(GTK_CONTAINER(wrapbox));
  active_button = gtk_object_get_data(GTK_OBJECT(wrapbox), "active_button");

  button_pos = g_list_index(button_list, active_button);
  gtk_wrap_box_reorder_child(GTK_WRAP_BOX(wrapbox),
                             GTK_WIDGET(active_button), button_pos + dir);
  g_list_free(button_list);

  sheets_dialog_normalize_line_breaks(wrapbox, dir);

  /* And then reorder the backing store if necessary */

  button_list = gtk_container_children(GTK_CONTAINER(wrapbox));
  active_button = gtk_object_get_data(GTK_OBJECT(wrapbox), "active_button");

  som = gtk_object_get_data(GTK_OBJECT(active_button), "sheet_object_mod");

  next_button_list = g_list_next(g_list_find(button_list, active_button));
  if (next_button_list)
    som_next = gtk_object_get_data(GTK_OBJECT(next_button_list->data),
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

    sm = gtk_object_get_data(GTK_OBJECT(active_button), "sheet_mod");
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

GList *sheets_new_dialog_combo_list = NULL;

void
on_sheets_dialog_button_new_clicked    (GtkButton       *button,
                                        gpointer         user_data)
{
  GtkWidget *combo;
  GtkWidget *wrapbox;
  GList *button_list;
  GtkWidget *active_button;
  gboolean is_line_break_sensitive;

  sheets_new_dialog = create_sheets_new_dialog();

  if (sheets_new_dialog_combo_list)
  {
    combo = lookup_widget(sheets_new_dialog, "combo_from_file");
    gtk_combo_set_popdown_strings(GTK_COMBO(combo),
                                  sheets_new_dialog_combo_list);
  }

  /* Deterine if a line break button can be put after the active button */

  active_button = sheets_dialog_get_active_button(&wrapbox, &button_list);
  is_line_break_sensitive = TRUE;
  if (gtk_object_get_data(GTK_OBJECT(active_button), "sheet_mod"))
  {
    button_list = g_list_next(g_list_find(button_list, active_button));
    if (!button_list || !gtk_object_get_data(GTK_OBJECT(button_list->data),
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
  gtk_object_set_data(GTK_OBJECT(active_button), "active_type",
                      (gpointer)SHEETS_NEW_DIALOG_TYPE_SVG_SHAPE);

  gtk_widget_show(sheets_new_dialog);
}

void
on_sheets_new_dialog_button_cancel_clicked
                                        (GtkButton       *button,
                                        gpointer         user_data)
{
  gtk_widget_destroy(sheets_new_dialog);
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
  wrapbox = gtk_object_get_data(GTK_OBJECT(table_sheets), "active_wrapbox");

  active_type = (SheetsNewDialogType)gtk_object_get_data(GTK_OBJECT(button),
                                                         "active_type");
  g_assert(active_type);

  switch (active_type)
  {
    GtkWidget *entry;
    gchar *file_name;
    gchar *p;
    struct stat stat_buf;
    GList *plugin_list;
    ObjectType *ot;
    gboolean (*custom_object_load_fn)();
    gint pos;
    GtkWidget *active_button;
    GList *button_list;
    SheetObjectMod *som;
    SheetMod *sm;
    SheetObject *sheet_obj;
    gchar *sheet_name;
    gchar *sheet_descrip;
    Sheet *sheet;
    GtkWidget *optionmenu;

  case SHEETS_NEW_DIALOG_TYPE_SVG_SHAPE:

    entry = lookup_widget(sheets_new_dialog, "combo_entry_from_file");
    file_name = gtk_editable_get_chars(GTK_EDITABLE(entry), 0, -1);

    p = file_name + strlen(file_name) - 6;
    if (strcmp(p, ".shape"))
    {
      message_error("Filename must end with '.shape': '%s'", file_name);
      g_free(file_name);
      return;
    }

    if (stat(file_name, &stat_buf) == -1)
    {
      message_error("%s: '%s'", strerror(errno), file_name);
      g_free(file_name);
      return;
    }

    custom_object_load_fn = NULL;
    for (plugin_list = dia_list_plugins(); plugin_list != NULL;
         plugin_list = g_list_next(plugin_list))
    {
       GModule *module = ((PluginInfo *)(plugin_list->data))->module;

       if (module == NULL)
         continue;

       if (g_module_symbol(module, "custom_object_load",
                           (gpointer)&custom_object_load_fn) == TRUE)
         break;
    }
    g_assert(custom_object_load_fn);

    if (!(*custom_object_load_fn)(file_name, &ot))
    {
      message_error("Could not interpret shape file: '%s'", file_name);
      g_free(file_name);
      return;
    }
    object_register_type(ot);

    entry = lookup_widget(sheets_new_dialog, "entry_svg_description");
    sheet_obj = g_new(SheetObject, 1);
    sheet_obj->object_type = g_strdup(ot->name);
    sheet_obj->description = gtk_editable_get_chars(GTK_EDITABLE(entry), 0, -1);
    sheet_obj->pixmap = ot->pixmap;
    sheet_obj->user_data = ot->default_user_data;
    sheet_obj->user_data_type = USER_DATA_IS_OTHER;
    sheet_obj->line_break = FALSE;
    sheet_obj->pixmap_file = g_strdup(ot->pixmap_file);
    sheet_obj->has_icon_on_sheet = FALSE;

    sm = gtk_object_get_data(GTK_OBJECT(wrapbox), "sheet_mod");
    som = sheets_append_sheet_object_mod(sheet_obj, sm);
    som->mod = SHEET_OBJECT_MOD_NEW;
    som->svg_filename = strdup(file_name);
    if (sm->mod == SHEETMOD_MOD_NONE)
      sm->mod = SHEETMOD_MOD_CHANGED;

    active_button = sheets_dialog_create_object_button(som, sm, wrapbox);
    gtk_wrap_box_pack(GTK_WRAP_BOX(wrapbox), active_button,
                      FALSE, TRUE, FALSE, TRUE);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(active_button), TRUE);
    gtk_widget_show(active_button);

    sheets_new_dialog_combo_list = g_list_append(sheets_new_dialog_combo_list, 
                                                 file_name);
    break;

  case SHEETS_NEW_DIALOG_TYPE_LINE_BREAK:

    sheets_dialog_wrapbox_add_line_break(wrapbox);

    button_list = gtk_container_children(GTK_CONTAINER(wrapbox));
    active_button = gtk_object_get_data(GTK_OBJECT(wrapbox), "active_button");
    pos = g_list_index(button_list, active_button);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(g_list_last(button_list)
                                                                 ->data), TRUE);
    gtk_wrap_box_reorder_child(GTK_WRAP_BOX(wrapbox),
                               g_list_last(button_list)->data, pos + 1);
    
    sheets_dialog_normalize_line_breaks(wrapbox, SHEETS_DIALOG_MOVE_NONE);

    sm = gtk_object_get_data(GTK_OBJECT(wrapbox), "sheet_mod");
    if (sm->mod == SHEETMOD_MOD_NONE)
      sm->mod = SHEETMOD_MOD_CHANGED;
    
    g_list_free(button_list);
    break;

  case SHEETS_NEW_DIALOG_TYPE_SHEET:

    entry = lookup_widget(sheets_new_dialog, "entry_sheet_name");
    sheet_name = gtk_editable_get_chars(GTK_EDITABLE(entry), 0, -1);

    sheet_name = g_strchug(g_strchomp(sheet_name));
    if (!*sheet_name)
    {
      message_error("Sheet must have a Name");
      return;
    }

    entry = lookup_widget(sheets_new_dialog, "entry_sheet_description");
    sheet_descrip = gtk_editable_get_chars(GTK_EDITABLE(entry), 0, -1);

    sheet = g_new(Sheet, 1);
    sheet->name = sheet_name;
    sheet->filename = "";
    sheet->description = sheet_descrip;
    sheet->scope = SHEET_SCOPE_USER;
    sheet->shadowing = NULL;
    sheet->objects = NULL;

    sm = sheets_append_sheet_mods(sheet);
    sm->mod = SHEETMOD_MOD_NEW;

    table_sheets = lookup_widget(sheets_dialog, "table_sheets");
    optionmenu = gtk_object_get_data(GTK_OBJECT(table_sheets),
                                     "active_optionmenu");
    g_assert(optionmenu);
    sheets_optionmenu_create(optionmenu, wrapbox, NULL);

    break;

  default:
    g_assert_not_reached();
  }

  sheets_dialog_apply_revert_set_sensitive(TRUE);

  button_list = gtk_container_children(GTK_CONTAINER(wrapbox));
  active_button = gtk_object_get_data(GTK_OBJECT(wrapbox), "active_button");
  sheets_dialog_up_down_set_sensitive(button_list,
                                      GTK_TOGGLE_BUTTON(active_button));
  g_list_free(button_list);

  gtk_widget_destroy(sheets_new_dialog);
}

static GtkWidget *sheets_shapeselection_dialog;

void
on_sheets_new_dialog_button_browse_clicked
                                        (GtkButton       *button,
                                        gpointer         user_data)
{
  sheets_shapeselection_dialog = create_sheets_shapeselection_dialog();
  gtk_widget_show(sheets_shapeselection_dialog);
}

void
on_sheets_shapeselection_dialog_button_cancel_clicked
                                        (GtkButton       *button,
                                        gpointer         user_data)
{
  gtk_widget_destroy(sheets_shapeselection_dialog);
}

void
on_sheets_shapeselection_dialog_button_ok_clicked
                                        (GtkButton       *button,
                                        gpointer         user_data)
{
  gchar *filename;
  GtkWidget *entry;

  filename = gtk_file_selection_get_filename(
                              GTK_FILE_SELECTION(sheets_shapeselection_dialog));

  entry = lookup_widget(sheets_new_dialog, "combo_entry_from_file");
  gtk_entry_set_text(GTK_ENTRY(entry), filename);

  gtk_widget_destroy(sheets_shapeselection_dialog);
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
  gchar *descrip;
  gchar *type;
  GtkWidget *entry;
  SheetMod *sm;
  gchar *name;

  sheets_edit_dialog = create_sheets_edit_dialog();

  active_button = sheets_dialog_get_active_button(&wrapbox, &button_list);
  som = gtk_object_get_data(GTK_OBJECT(active_button), "sheet_object_mod");
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

  sm = gtk_object_get_data(GTK_OBJECT(active_button), "sheet_mod");
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
  if (gtk_object_get_data(GTK_OBJECT(active_button), "is_hidden_button"))
  {
    radio_button = lookup_widget(sheets_remove_dialog, "radiobutton_object");
    gtk_widget_set_sensitive(radio_button, FALSE);
    gtk_widget_set_sensitive(entry, FALSE);
  }
  else
  {
    SheetObjectMod *som;
    
    som = gtk_object_get_data(GTK_OBJECT(active_button), "sheet_object_mod");
    if (!som)
      gtk_entry_set_text(GTK_ENTRY(entry), "Line Break");
    else
      gtk_entry_set_text(GTK_ENTRY(entry), som->sheet_object.description);

    radio_button = lookup_widget(sheets_remove_dialog, "radiobutton_object");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio_button), TRUE);
  }

  entry = lookup_widget(sheets_remove_dialog, "entry_sheet");
  sm = gtk_object_get_data(GTK_OBJECT(active_button), "sheet_mod");

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
    gtk_object_set_data(GTK_OBJECT(tmp), "active_type", (gpointer)type);
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
    "combo_from_file",
    "button_browse",
    "label_svg_description",
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
      new_active_button = gtk_object_get_data(GTK_OBJECT(wrapbox),
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

  type = (SheetsRemoveDialogType)gtk_object_get_data(GTK_OBJECT(button),
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
  
    som = gtk_object_get_data(GTK_OBJECT(active_button), "sheet_object_mod");
    if (som)
    {
      sm = gtk_object_get_data(GTK_OBJECT(active_button), "sheet_mod");
      sm->mod = SHEETMOD_MOD_CHANGED;
      som->mod = SHEET_OBJECT_MOD_DELETED;
    }
    new_active_button = sheets_dialog_set_new_active_button();

    gtk_widget_destroy(active_button);
    radio_group = gtk_radio_button_group(GTK_RADIO_BUTTON(new_active_button));

    sheets_dialog_normalize_line_breaks(wrapbox, SHEETS_DIALOG_MOVE_NONE);
    break;
    
  case SHEETS_REMOVE_DIALOG_TYPE_SHEET:

    sm = gtk_object_get_data(GTK_OBJECT(active_button), "sheet_mod");
    sm->mod = SHEETMOD_MOD_DELETED;

    if (sm->sheet.shadowing && sm->sheet.scope == SHEET_SCOPE_USER)
      sheets_append_sheet_mods(sm->sheet.shadowing);

    table_sheets = lookup_widget(sheets_dialog, "table_sheets");
    optionmenu = gtk_object_get_data(GTK_OBJECT(table_sheets),
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
  if ((gboolean)gtk_object_get_data(GTK_OBJECT(entry), "changed") == TRUE)
  {
    SheetMod *sm;
    SheetObjectMod *som;

    som = gtk_object_get_data(GTK_OBJECT(active_button), "sheet_object_mod");
    som->sheet_object.description = gtk_editable_get_chars(GTK_EDITABLE(entry),
                                                           0, -1);
    sheets_dialog_object_set_tooltip(som, active_button);

    som->mod = SHEET_OBJECT_MOD_CHANGED;

    sm = gtk_object_get_data(GTK_OBJECT(active_button), "sheet_mod");
    if (sm->mod == SHEETMOD_MOD_NONE)
      sm->mod = SHEETMOD_MOD_CHANGED;
    
    something_changed = TRUE;
  }

  entry = lookup_widget(sheets_edit_dialog, "entry_sheet_description");
  if ((gboolean)gtk_object_get_data(GTK_OBJECT(entry), "changed") == TRUE)
  {
    SheetMod *sm;

    sm = gtk_object_get_data(GTK_OBJECT(active_button), "sheet_mod");
    sm->sheet.description = gtk_editable_get_chars(GTK_EDITABLE(entry), 0, -1);

    if (sm->mod == SHEETMOD_MOD_NONE)
      sm->mod = SHEETMOD_MOD_CHANGED;
    something_changed = TRUE;
  }
  
#ifdef CAN_EDIT_SHEET_NAME
  /* This must be last because we reload the sheets if changed */
  
  entry = lookup_widget(sheets_edit_dialog, "entry_sheet_name");
  if ((gboolean)gtk_object_get_data(GTK_OBJECT(entry), "changed") == TRUE)
  {
    SheetMod *sm;
    GtkWidget *table_sheets;
    GtkWidget *optionmenu;

    sm = gtk_object_get_data(GTK_OBJECT(active_button), "sheet_mod");
    sm->sheet.name = gtk_editable_get_chars(GTK_EDITABLE(entry), 0, -1);
    if (sm->mod == SHEETMOD_MOD_NONE)
      sm->mod = SHEETMOD_MOD_CHANGED;

    table_sheets = lookup_widget(sheets_dialog, "table_sheets");
    optionmenu = gtk_object_get_data(GTK_OBJECT(table_sheets),
                                     "active_optionmenu");
    sheets_optionmenu_create(optionmenu, wrapbox, NULL);
    
    something_changed = TRUE;
  }
#endif
  if (something_changed == TRUE)
    sheets_dialog_apply_revert_set_sensitive(TRUE);

  gtk_widget_destroy(sheets_edit_dialog);
}

void
on_sheets_edit_dialog_entry_object_description_changed
                                        (GtkEditable     *editable,
                                        gpointer         user_data)
{
  gtk_object_set_data(GTK_OBJECT(editable), "changed", (gpointer)TRUE);
}


void
on_sheets_edit_dialog_entry_sheet_description_changed
                                        (GtkEditable     *editable,
                                        gpointer         user_data)
{
  gtk_object_set_data(GTK_OBJECT(editable), "changed", (gpointer)TRUE);
}

void
on_sheets_edit_dialog_entry_sheet_name_changed
                                        (GtkEditable     *editable,
                                        gpointer         user_data)
{
  gtk_object_set_data(GTK_OBJECT(editable), "changed", (gpointer)TRUE);
}

static GtkWidget *
sheets_dialog_get_target_wrapbox(GtkWidget *wrapbox)
{
  if (gtk_object_get_data(GTK_OBJECT(wrapbox), "is_left"))
    return gtk_object_get_data(GTK_OBJECT(sheets_dialog), "wrapbox_right");
  else
    return gtk_object_get_data(GTK_OBJECT(sheets_dialog), "wrapbox_left");
}

static void
sheets_dialog_copy_object(GtkWidget *active_button, GtkWidget *target_wrapbox)
{
  GtkWidget *button;
  SheetMod *sm;
  SheetObjectMod *som;
  SheetObjectMod *som_new;
  SheetObject *so;

  sm = gtk_object_get_data(GTK_OBJECT(target_wrapbox), "sheet_mod");
  som = gtk_object_get_data(GTK_OBJECT(active_button), "sheet_object_mod");

  if (!som)
    return;

  so = g_new(SheetObject, 1);
  so->object_type = g_strdup(som->sheet_object.object_type);
  so->description = g_strdup(som->sheet_object.description);
  so->pixmap = som->sheet_object.pixmap;
  so->user_data = som->sheet_object.user_data;
  so->user_data_type = som->sheet_object.user_data_type;
  so->line_break = FALSE;          /* must be false--we don't copy linebreaks */
  so->pixmap_file = g_strdup(som->sheet_object.pixmap_file);
  so->has_icon_on_sheet = som->sheet_object.has_icon_on_sheet;

  som_new = g_new(SheetObjectMod, 1);
  som_new->sheet_object = *so;
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
  GtkWidget *active_button; 
  GtkWidget *wrapbox;
  GList *button_list, *iter_list;
  GtkWidget *target_wrapbox;

  active_button = sheets_dialog_get_active_button(&wrapbox, &button_list);

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

  som = gtk_object_get_data(GTK_OBJECT(active_button), "sheet_object_mod");
  if (som)
    som->mod = SHEET_OBJECT_MOD_DELETED;

  new_active_button = sheets_dialog_set_new_active_button();

  gtk_widget_destroy(active_button);
  radio_group = gtk_radio_button_group(GTK_RADIO_BUTTON(new_active_button));

  sheets_dialog_normalize_line_breaks(wrapbox, SHEETS_DIALOG_MOVE_NONE);
}

void
on_sheets_dialog_button_move_all_clicked
                                        (GtkButton       *button,
                                        gpointer         user_data)
{
  GtkWidget *active_button;
  GtkWidget *wrapbox;
  GList *button_list;
  GList *iter_list;
  GtkWidget *target_wrapbox;
  SheetObjectMod *som;

  active_button = sheets_dialog_get_active_button(&wrapbox, &button_list);

  target_wrapbox = sheets_dialog_get_target_wrapbox(wrapbox);

  for (iter_list = button_list; iter_list; iter_list = g_list_next(iter_list))
  {
    sheets_dialog_copy_object(iter_list->data, target_wrapbox);

    som = gtk_object_get_data(GTK_OBJECT(iter_list->data), "sheet_object_mod");
    if (som)
      som->mod = SHEET_OBJECT_MOD_DELETED;

    gtk_widget_destroy(iter_list->data);

    // MCNFIXME:  do we have to resanitize the radio_group?
  }
  
  /* Force the 1st button in the target wrapbox to be active after moving */

  button_list = gtk_container_children(GTK_CONTAINER(target_wrapbox));
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button_list->data), TRUE);
}

static gboolean
copy_file(gchar *src, gchar *dst)
{
  FILE *fp_src;
  FILE *fp_dst;
  int c;

  if ((fp_src = fopen(src, "rb")) == NULL)
  {
    message_error("Couldn't open '%s': %s", src, strerror(errno));
    return FALSE;
  }
 
  if ((fp_dst = fopen(dst, "wb")) == NULL)
  {
    message_error("Couldn't open '%s': %s", dst, strerror(errno));
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
  gchar buf[512];
  time_t time_now;
  char *username;
  gchar *dir_user_sheets;
  gchar *filename;
  SheetObject *sheetobject;
  GSList *sheet_objects;
    
  dir_user_sheets = dia_config_filename("sheets");
  if (!*(sheet->filename))
  {
    gchar *basename;
    
    basename = g_strdup(sheet->name);
    basename = g_strdelimit(basename, G_STR_DELIMITERS G_DIR_SEPARATOR_S, '_');
    filename = g_strdup_printf("%s%s%s.sheet", dir_user_sheets,
                               G_DIR_SEPARATOR_S, basename);
    g_free(basename);
  }
  else
    filename = g_strdup_printf("%s%s%s", dir_user_sheets,
                               G_DIR_SEPARATOR_S, g_basename(sheet->filename));

  file = fopen(filename, "w");
   
  if (file==NULL)
  {
    message_error(_("Couldn't open: '%s' for writing"), filename);
    g_free(filename);
    return FALSE;
  }
  fclose(file);

  time_now = time(NULL);
  username = getlogin();
  if (username==NULL)
    username = "a user";

  doc = xmlNewDoc("1.0");
  doc->encoding = xmlStrdup("UTF-8");
  doc->standalone = FALSE;
  root = xmlNewDocNode(doc, NULL, "sheet", NULL);
  doc->xmlRootNode = root;
  xmlSetProp(root, "xmlns", "http://www.lysator.liu.se/~alla/dia/dia-sheet-ns");

  /* comments */
  xmlAddChild(root, xmlNewText("\n"));
  xmlAddChild(root, xmlNewComment("Dia-Version: "VERSION));
  xmlAddChild(root, xmlNewText("\n"));
  g_snprintf(buf, sizeof(buf), "File: %s", filename);
  xmlAddChild(root, xmlNewComment(buf));
  xmlAddChild(root, xmlNewText("\n"));
  g_snprintf(buf, sizeof(buf), "Date: %s", ctime(&time_now));
  buf[strlen(buf)-1] = '\0'; /* remove the trailing new line */
  xmlAddChild(root, xmlNewComment(buf));
  xmlAddChild(root, xmlNewText("\n"));
  g_snprintf(buf, sizeof(buf), "For: %s", username);
  xmlAddChild(root, xmlNewComment(buf));
  xmlAddChild(root, xmlNewText("\n\n"));

  /* sheet name */
  node = xmlNewChild(root, NULL, "name", NULL);
  xmlAddChild(node, xmlNewText(sheet->name));
  xmlAddChild(root, xmlNewText("\n"));

  /* sheet description */
  node = xmlNewChild(root, NULL, "description", NULL);
  xmlAddChild(node, xmlNewText(sheet->description));
  xmlAddChild(root, xmlNewText("\n"));

  /* content */
  node = xmlNewChild(root, NULL, "contents", NULL);
  xmlAddChild(node, xmlNewText("\n"));
  xmlAddChild(node, xmlNewComment("add shapes here"));
  xmlAddChild(node, xmlNewText("\n"));

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

      dia_user_shapes = dia_config_filename("shapes");

      dest = g_strdup_printf("%s%s%s", dia_user_shapes, G_DIR_SEPARATOR_S,
                             g_basename(som->svg_filename));
      copy_file(som->svg_filename, dest);
      g_free(dest);

      dest = g_strdup_printf("%s%s%s", dia_user_shapes, G_DIR_SEPARATOR_S,
                             g_basename(som->sheet_object.pixmap_file));
      copy_file(som->sheet_object.pixmap_file, dest);
      g_free(dest);
    }

    sheetobject = &som->sheet_object;
    object_node = xmlNewChild(node, NULL, "object", NULL);
    
    xmlSetProp(object_node, "name", sheetobject->object_type);
    
    if (sheetobject->user_data_type == USER_DATA_IS_INTDATA)
    {
      gchar *user_data;

      user_data = g_strdup_printf("%u", (guint)(sheetobject->user_data));
      xmlSetProp(object_node, "intdata", user_data);
      g_free(user_data);
    }
      
    xmlAddChild(object_node, xmlNewText("\n"));
    object_node = xmlNewChild(object_node, NULL, "description", NULL);
    xmlAddChild(object_node, xmlNewText(sheetobject->description));
    xmlAddChild(node, xmlNewText("\n"));

    if (sheetobject->has_icon_on_sheet == TRUE)
    {
      object_node = xmlNewChild(object_node, NULL, "icon", NULL);
      xmlAddChild(object_node, xmlNewText(sheetobject->pixmap_file));
      xmlAddChild(node, xmlNewText("\n"));
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

  stat(filename, &stat_buf);
  utim_buf.actime = stat_buf.st_atime;
  utim_buf.modtime = time(NULL);
  utime(filename, &utim_buf);
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

    case SHEETMOD_MOD_NEW:
      write_user_sheet(&sm->sheet);

      sheet_object_mods_list = sm->sheet.objects;
      sm->sheet.objects = NULL;
      register_sheet(&sm->sheet);

      for (list = sheet_object_mods_list; list; list = g_slist_next(list))
      {
        SheetObjectMod *som;

        som = list->data;
        if (som->mod == SHEET_OBJECT_MOD_DELETED)
          continue;

        sheet_append_sheet_obj(&sm->sheet, &som->sheet_object);
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

        som = list->data;
        if (som->mod == SHEET_OBJECT_MOD_DELETED)
          continue;

        sheet_append_sheet_obj(sheets_list->data, &som->sheet_object);
      }
      fill_sheet_menu();
      break;

    case SHEETMOD_MOD_DELETED:
      if (sm->sheet.scope == SHEET_SCOPE_SYSTEM)
        touch_file(sm->sheet.shadowing->filename);
      else
        unlink(sm->sheet.filename);

      sheets_list = get_sheets_list();
      find_list = g_slist_find_custom(sheets_list, &sm->sheet,
                                        sheets_find_sheet);
      g_assert(sheets_list);
      g_slist_remove_link(sheets_list, find_list);
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

