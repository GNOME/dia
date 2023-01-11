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

#define G_LOG_DOMAIN "Dia"

#include "config.h"

#include <glib/gi18n-lib.h>

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <gtk/gtk.h>
#include <glib.h>

#include "message.h"
#include "persistence.h"

static GHashTable *message_hash_table;

typedef struct {
  const gchar *title;
  GtkWidget *dialog;
  GtkWidget *repeat_label;
  GList *repeats;
  GtkWidget *repeat_view;
  GtkWidget *show_repeats;
  GtkWidget *no_show_again;
} DiaMessageInfo;

static void
gtk_message_toggle_repeats(GtkWidget *button, gpointer *userdata)
{
  DiaMessageInfo *msginfo = (DiaMessageInfo*)userdata;
  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)))
    gtk_widget_show(msginfo->repeat_view);
  else {
    gtk_widget_hide(msginfo->repeat_view);
    gtk_container_check_resize(GTK_CONTAINER(msginfo->dialog));
  }
}

static void
gtk_message_toggle_show_again(GtkWidget *button, gpointer *userdata)
{
  DiaMessageInfo *msginfo = (DiaMessageInfo*)userdata;
  persistence_set_boolean(msginfo->title,
			  gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)));
}

static void
message_dialog_destroyed(GtkWidget *widget, gpointer userdata)
{
  DiaMessageInfo *msginfo = (DiaMessageInfo *)userdata;

  msginfo->title = NULL;
  msginfo->dialog = NULL;
  msginfo->repeat_label = NULL;
  msginfo->repeat_view = NULL;
  msginfo->show_repeats = NULL;
  msginfo->no_show_again = NULL;
}


/*
 * Set up a dialog for these messages.
 * Msginfo may contain repeats, and should be filled out
 */
static void
message_create_dialog(const gchar *title, DiaMessageInfo *msginfo, gchar *buf)
{
  GtkWidget *dialog = NULL;
  GtkTextBuffer *textbuffer;
  GtkMessageType type = GTK_MESSAGE_INFO;
  GList *repeats;

  /* quite dirty in order to not change Dia's message api */
  if (title) {
    if (0 == strcmp (title, _("Error")))
      type = GTK_MESSAGE_ERROR;
    else if (0 == strcmp (title, _("Warning")))
      type = GTK_MESSAGE_WARNING;
  }

  if (msginfo->repeats != NULL) {
    buf = (char *) msginfo->repeats->data;
  }

  dialog = gtk_message_dialog_new (NULL, /* no parent window */
                                   0,    /* GtkDialogFlags */
                                   type,
                                   GTK_BUTTONS_CLOSE,
                                   "%s", buf);
  gtk_window_set_keep_above (GTK_WINDOW (dialog), TRUE);
  if (title) {
    char *real_title;

    msginfo->title = title;
    real_title = g_strdup_printf ("Dia: %s", title);
    gtk_window_set_title (GTK_WINDOW (dialog), real_title);
    g_clear_pointer (&real_title, g_free);
  }
  gtk_widget_show (dialog);
  g_signal_connect (G_OBJECT (dialog), "response",
                    G_CALLBACK (gtk_widget_hide),
                    NULL);
  msginfo->dialog = dialog;
  g_signal_connect (G_OBJECT (dialog), "destroy",
                    G_CALLBACK (message_dialog_destroyed),
                    msginfo);

  msginfo->repeat_label = gtk_label_new (_("There is one similar message."));
  gtk_container_add (GTK_CONTAINER (gtk_dialog_get_content_area (GTK_DIALOG (msginfo->dialog))),
                     msginfo->repeat_label);

  msginfo->show_repeats =
    gtk_check_button_new_with_label (_("Show repeated messages"));
  gtk_container_add (GTK_CONTAINER (gtk_dialog_get_content_area (GTK_DIALOG (msginfo->dialog))),
                     msginfo->show_repeats);
  g_signal_connect (G_OBJECT (msginfo->show_repeats), "toggled",
                    G_CALLBACK (gtk_message_toggle_repeats), msginfo);

  msginfo->repeat_view = gtk_text_view_new();
  gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(msginfo->dialog))),
		    msginfo->repeat_view);
  gtk_text_view_set_editable(GTK_TEXT_VIEW(msginfo->repeat_view), FALSE);

  textbuffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(msginfo->repeat_view));
  if (msginfo->repeats != NULL) {
    repeats = msginfo->repeats;
    repeats = repeats->next;
    for (; repeats != NULL; repeats = repeats->next) {
      gtk_text_buffer_insert_at_cursor(textbuffer, (gchar*)repeats->data, -1);
    }
  }

  msginfo->no_show_again =
    gtk_check_button_new_with_label(_("Don't show this message again"));
  gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(msginfo->dialog))),
		    msginfo->no_show_again);
  g_signal_connect(G_OBJECT(msginfo->no_show_again), "toggled",
		   G_CALLBACK(gtk_message_toggle_show_again), msginfo);
}

static void G_GNUC_PRINTF(3, 0)
gtk_message_internal(const char* title, enum ShowAgainStyle showAgain,
		     char const *fmt,
                     va_list args)
{
  char *msg;
  DiaMessageInfo *msginfo;
  GtkTextBuffer *textbuffer;
  gboolean askForShowAgain = FALSE;

  if (showAgain != ALWAYS_SHOW) {
    /* We persistently stored that the user has chosen to not see the
     * dialog again (whether by checking or not unchecking the box) */
    persistence_register_boolean((gchar *)title, FALSE);
    if (persistence_get_boolean((gchar *)title)) {
      /* If not showing again, just return at once */
      return;
    }
    askForShowAgain = TRUE;
  }

  if (message_hash_table == NULL) {
    message_hash_table = g_hash_table_new(g_str_hash, g_str_equal);
  }

  msg = g_strdup_vprintf (fmt, args);

  msginfo = (DiaMessageInfo*)g_hash_table_lookup(message_hash_table, fmt);
  if (msginfo == NULL) {
    msginfo = g_new0(DiaMessageInfo, 1);
    g_hash_table_insert(message_hash_table, (char *)fmt, msginfo);
  }
  if (msginfo->dialog == NULL)
    message_create_dialog(title, msginfo, msg);

  if (msginfo->repeats != NULL) {
    if (g_list_length(msginfo->repeats) > 1) {
      char *newlabel;
      guint num = g_list_length(msginfo->repeats);
      /* See: https://live.gnome.org/TranslationProject/DevGuidelines/Plurals */
      newlabel = g_strdup_printf(g_dngettext (GETTEXT_PACKAGE,
					"There is %d similar message.", /* not triggered */
					"There are %d similar messages.", num), num);
      gtk_label_set_text(GTK_LABEL(msginfo->repeat_label), newlabel);
    }
    /* for repeated messages, show the last one */
    g_object_set (msginfo->dialog, "text", msg, NULL);

    gtk_widget_show(msginfo->repeat_label);
    gtk_widget_show(msginfo->show_repeats);
  }

  /* Insert in scrollable view, but only the repeated ones */
  if (msginfo->repeats != NULL) {
    textbuffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(msginfo->repeat_view));
    gtk_text_buffer_insert_at_cursor(textbuffer, msg, -1);
  }

  msginfo->repeats = g_list_prepend(msginfo->repeats, g_strdup(msg));

  if (askForShowAgain) {
    gtk_widget_show(msginfo->no_show_again);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(msginfo->no_show_again),
				 showAgain == SUGGEST_NO_SHOW_AGAIN);
  } else {
    gtk_widget_hide(msginfo->no_show_again);
  }

  gtk_widget_show (msginfo->dialog);
  g_clear_pointer (&msg, g_free);
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
  message_internal(title, ALWAYS_SHOW, format, args);
  va_end (args);
  va_end (args2);
}


/**
 * message_notice:
 * @format: the message
 * @...: @format arguments
 *
 * Emit a message about something the user should be aware of.
 * In the default GTK message system, this message will by default only
 * be shown once.
 *
 * Since: dawn-of-time
 */
void
message_notice (const char *format, ...)
{
  va_list args, args2;

  va_start (args, format);
  va_start (args2, format);
  message_internal (_("Notice"), SUGGEST_NO_SHOW_AGAIN, format, args);
  va_end (args);
  va_end (args2);
}


/**
 * message_warning:
 * @format: the message
 * @...: @format arguments
 *
 * Emit a message about a possible danger.
 * In the default GTK message system, this message can be made to only be
 * shown once, but is by default shown every time it is invoked.
 *
 * Since: dawn-of-time
 */
void
message_warning (const char *format, ...)
{
  va_list args, args2;

  va_start (args, format);
  va_start (args2, format);
  message_internal (_("Warning"), SUGGEST_SHOW_AGAIN, format, args);
  va_end (args);
  va_end (args2);
}


/**
 * message_error:
 * @format: the message
 * @...: @format arguments
 *
 * Emit a message about an error.
 * In the default GTK message system, this message is always shown.
 *
 * Since: dawn-of-time
 */
void
message_error (const char *format, ...)
{
  va_list args, args2;

  va_start (args, format);
  va_start (args2, format);
  message_internal (_("Error"), ALWAYS_SHOW, format, args);
  va_end (args);
  va_end (args2);
}

static gboolean log_enabled = FALSE;


void
dia_log_message_enable (gboolean yes)
{
  log_enabled = yes;
}

void
dia_log_message (const char *format, ...)
{
  static GTimer *timer = NULL;
  char *log;
  va_list args;
  gint64 t;
  gulong h, m, s, ms;

  if (!log_enabled)
     return;

  if (!timer)
    timer = g_timer_new ();

  va_start (args, format);
  log  = g_strdup_vprintf (format, args);
  va_end (args);

  t = (gint64)g_timer_elapsed (timer, &ms);
  s = t % 60; t = (t - s) / 60;
  m = t % 60; t = (t - m) / 60;
  h = t;
  g_message ("%02d:%02d:%02d.%03d - %s", (int)h, (int)m, (int)s, (int)(ms/1000), log);

  g_clear_pointer (&log, g_free);
}
