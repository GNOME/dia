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
#include <string.h>
#include <stdarg.h>
#undef GTK_DISABLE_DEPRECATED /* for gtk_object_get_user_data */
#include <gtk/gtk.h>
#include <glib.h>

#include "intl.h"
#include "utils.h"
#include "message.h"

static GHashTable *message_hash_table;

typedef struct {
  GtkWidget *dialog;
  GtkWidget *repeat_label;
  GList *repeats;
  GtkWidget *repeat_view;
} DiaMessageInfo;

static void
gtk_message_toggle_repeats(GtkWidget *button, gpointer *userdata) {
  DiaMessageInfo *msginfo = (DiaMessageInfo*)userdata;
  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)))
    gtk_widget_show(msginfo->repeat_view);
  else {
    gtk_widget_hide(msginfo->repeat_view);
    gtk_container_check_resize(GTK_CONTAINER(msginfo->dialog));
  }
}

static void
gtk_message_internal(const char* title, const char *fmt,
                     va_list *args,  va_list *args2)
{
  static gchar *buf = NULL;
  static gint   alloc = 0;
  gint len;
  GtkWidget *dialog = NULL;
  GtkMessageType type = GTK_MESSAGE_INFO;
  DiaMessageInfo *msginfo;

  if (message_hash_table == NULL) {
    message_hash_table = g_hash_table_new(g_str_hash, g_str_equal);
  }

  len = format_string_length_upper_bound (fmt, args);

  if (len >= alloc) {
    if (buf)
      g_free (buf);
    
    alloc = nearest_pow (MAX(len + 1, 1024));
    
    buf = g_new (char, alloc);
  }
  
  vsprintf (buf, fmt, *args2);

  msginfo = (DiaMessageInfo*)g_hash_table_lookup(message_hash_table, fmt);
  if (msginfo != NULL) {
    GtkTextBuffer *textbuffer;
    if (msginfo->repeats == NULL) {
      GtkWidget *showrepeats;
      /* First time repeated, create repeat label */
      msginfo->repeats = g_list_append(msginfo->repeats, buf);
      msginfo->repeat_label = gtk_label_new(_("There is one similar message."));
      gtk_container_add(GTK_CONTAINER(GTK_DIALOG(msginfo->dialog)->vbox), 
			msginfo->repeat_label);
      gtk_widget_show(msginfo->repeat_label);

      showrepeats = gtk_check_button_new_with_label(_("Show repeated messages"));
      gtk_container_add(GTK_CONTAINER(GTK_DIALOG(msginfo->dialog)->vbox), 
			showrepeats);
      g_signal_connect(G_OBJECT(showrepeats), "toggled", G_CALLBACK(gtk_message_toggle_repeats), msginfo);
      gtk_widget_show(showrepeats);

      msginfo->repeat_view = gtk_text_view_new();
      gtk_container_add(GTK_CONTAINER(GTK_DIALOG(msginfo->dialog)->vbox), 
			msginfo->repeat_view);
      gtk_text_view_set_editable(GTK_TEXT_VIEW(msginfo->repeat_view), FALSE);
    } else {
      char *newlabel;
      msginfo->repeats = g_list_append(msginfo->repeats, buf);
      newlabel = g_strdup_printf(_("There are %d similar messages."),
				       g_list_length(msginfo->repeats));
      gtk_label_set_text(GTK_LABEL(msginfo->repeat_label), newlabel);
    }
    textbuffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(msginfo->repeat_view));
    gtk_text_buffer_insert_at_cursor(textbuffer, buf, -1);
  } else {
    /* quite dirty to not change Dia's message api */
    if (title) {
      if (0 == strcmp (title, _("Error")))
	type = GTK_MESSAGE_ERROR;
      else if (0 == strcmp (title, _("Warning")))
	type = GTK_MESSAGE_WARNING;
    }
    dialog = gtk_message_dialog_new (NULL, /* no parent window */
				     0,    /* GtkDialogFlags */
				     type,
				     GTK_BUTTONS_CLOSE,
				     buf);
    if (title) {
      gchar *real_title;

      real_title = g_strdup_printf ("Dia: %s", title);
      gtk_window_set_title (GTK_WINDOW(dialog), real_title);
      g_free (real_title);
    }
    gtk_widget_show (dialog);
    g_signal_connect (G_OBJECT (dialog), "response",
		      G_CALLBACK (gtk_widget_destroy),
		      NULL);
    /* Store relevant info to allow repeats to be collapsed */
    msginfo = g_new0(DiaMessageInfo, 1);
    msginfo->dialog = dialog;
    g_hash_table_insert(message_hash_table, fmt, msginfo);
  }
}

static MessageInternal message_internal = gtk_message_internal;

void 
set_message_func(MessageInternal func)
{
  g_assert(func);
  message_internal = func;
}

void
message(const char *title, const char *format, ...)
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
