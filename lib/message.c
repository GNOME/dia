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

#include <stdio.h>
#include <stdarg.h>
#ifdef GNOME
#include <gnome.h>
#else
#include <gtk/gtk.h>
#endif

#include "intl.h"
#include "utils.h"
#include "message.h"

static void
gtk_message_internal(char *title, const char *fmt,
		 va_list *args,  va_list *args2)
{
  static gchar *buf = NULL;
  static gint   alloc = 0;
  GtkWidget *dialog_window = NULL;
  GtkWidget *vbox;
  GtkWidget *label;
#ifndef GNOME
  GtkWidget *button;
  GtkWidget *bbox;
#endif
  gint len;

  len = format_string_length_upper_bound (fmt, args);

  if (len >= alloc) {
    if (buf)
      g_free (buf);
    
    alloc = nearest_pow (MAX(len + 1, 1024));
    
    buf = g_new (char, alloc);
  }
  
  vsprintf (buf, fmt, *args2);

#ifdef GNOME
  dialog_window = gnome_dialog_new(title, GNOME_STOCK_BUTTON_OK, NULL);
  vbox = GNOME_DIALOG(dialog_window)->vbox;
#else
  dialog_window = gtk_dialog_new ();
  gtk_window_set_title (GTK_WINDOW (dialog_window), title);
  gtk_container_set_border_width (GTK_CONTAINER (dialog_window), 0);
  vbox = GTK_DIALOG(dialog_window)->vbox;
#endif

  label = gtk_label_new (buf);
  gtk_misc_set_padding (GTK_MISC (label), 10, 10);
  gtk_box_pack_start (GTK_BOX (vbox), label, TRUE, TRUE, 0);
  gtk_widget_show (label);

#ifdef GNOME
  gnome_dialog_set_close(GNOME_DIALOG(dialog_window), TRUE);
#else
  bbox = gtk_hbutton_box_new();
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog_window)->action_area), 
		     bbox, TRUE, TRUE, 0);
  gtk_button_box_set_child_size(GTK_BUTTON_BOX(bbox), 80, 0);
  gtk_button_box_set_spacing(GTK_BUTTON_BOX(bbox), 10);
  gtk_widget_show(bbox);
  
  button = gtk_button_new_with_label (_("OK"));
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_container_add(GTK_CONTAINER(bbox), button);
  gtk_widget_grab_default (button);
  gtk_widget_show (button);

  gtk_signal_connect_object(GTK_OBJECT (button), "clicked",
			    GTK_SIGNAL_FUNC(gtk_widget_destroy),
			    GTK_OBJECT(dialog_window));
#endif

  gtk_widget_show (dialog_window);
}

static MessageInternal message_internal = gtk_message_internal;

void 
set_message_func(MessageInternal func)
{
  g_assert(func);
  message_internal = func;
}

void
message(char *title, const char *format, ...)
{
  va_list args, args2;

  va_start (args, format);
  va_start (args2, format);
  message_internal(title, format, &args, &args2);
  va_end (args);
  va_end (args2);
}

void
message_notice(const char *format, ...)
{
  va_list args, args2;

  va_start (args, format);
  va_start (args2, format);
  message_internal(_("Notice"), format, &args, &args2);
  va_end (args);
  va_end (args2);
}
void
message_warning(const char *format, ...)
{
  va_list args, args2;

  va_start (args, format);
  va_start (args2, format);
  message_internal(_("Warning"), format, &args, &args2);
  va_end (args);
  va_end (args2);
}

void
message_error(const char *format, ...)
{
  va_list args, args2;

  va_start (args, format);
  va_start (args2, format);
  message_internal(_("Error"), format, &args, &args2);
  va_end (args);
  va_end (args2);
}
