/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * confirm.c - a dialog usually not to be shown
 *
 * Copyright (C) 2009 Hans Breuer
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

#include "confirm.h"
#include "intl.h"

static gint 
confirm_respond (GtkWidget *widget, gint response_id, gpointer data)
{
  /* just close it in any case */
  gtk_widget_hide (widget);
  return 0;  
}

/*!
 * \brief Check and - if huge - let the user confirm the diagram size before export
 */
gboolean
confirm_export_size (Diagram *dia, GtkWindow *parent, guint flags)
{
  GtkWidget *dialog;
  int pages = 0;
  gint64 bytes = 0;
  gchar *size, *msg;
  gboolean ret;
 
  pages = ceil((dia->data->extents.right - dia->data->extents.left) / dia->data->paper.width)
        * ceil((dia->data->extents.bottom - dia->data->extents.top) /  dia->data->paper.height);
  /* three guesses: 4 bytes per pixel, 20 pixels per cm; using * dia->data->paper.scaling  */
  bytes = (gint64)4
        * ceil((dia->data->extents.right - dia->data->extents.left) * dia->data->paper.scaling * 20)
        * ceil((dia->data->extents.bottom - dia->data->extents.top) * dia->data->paper.scaling * 20);

  if ((flags & CONFIRM_PRINT) && (pages < 10)) /* hardcoded limits for the dialog to show */
    return TRUE;
  else if ((flags & CONFIRM_MEMORY) && (bytes < (G_GINT64_CONSTANT(100)<<16)))
    return TRUE; /* smaller than 100MB  */
  else if ((flags & CONFIRM_PAGES) && (pages < 50))
    return TRUE;
  
  /* message and limits depend on the flags give */
#if GLIB_CHECK_VERSION(2,30,0)
  size = g_format_size (bytes);
#else
  size = g_format_size_for_display (bytes);
#endif
  /* See: https://live.gnome.org/TranslationProject/DevGuidelines/Plurals */
  if (flags & CONFIRM_PRINT)
    msg = g_strdup_printf (g_dngettext (GETTEXT_PACKAGE,
		"You are about to print a diagram with %d page.", /* not triggered */
		"You are about to print a diagram with %d pages.", pages), pages);
  else if ((flags & ~CONFIRM_PAGES) == 0)
    msg = g_strdup_printf (g_dngettext (GETTEXT_PACKAGE,
		"You are about to export a diagram with %d page.",  /* not triggered */
		"You are about to export a diagram with %d pages.", pages), pages);
  else
    msg = g_strdup_printf (g_dngettext (GETTEXT_PACKAGE,
		"You are about to export a diagram which may require %s of memory.(%d page).",
		"You are about to export a diagram which may require %s of memory.(%d pages).", pages),
		size, pages);
  dialog = gtk_message_dialog_new (parent, /* diagrams display 'shell' */
				   GTK_DIALOG_MODAL|GTK_DIALOG_DESTROY_WITH_PARENT,
				   GTK_MESSAGE_WARNING,
				   GTK_BUTTONS_OK_CANCEL,
				   "%s", msg);
  gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog), 
					    _("You can adjust the size of the diagram by changing "
					      "the 'Scaling' in the 'Page Setup' dialog.\n"
					      "Alternatively use 'Best Fit' "
					      "to move objects/handles into the intended bounds."));
  gtk_window_set_title (GTK_WINDOW (dialog), _("Confirm Diagram Size"));
  g_free (size);

  g_signal_connect(G_OBJECT (dialog), "response",
		   G_CALLBACK(confirm_respond), NULL);

  ret = (GTK_RESPONSE_OK == gtk_dialog_run (GTK_DIALOG (dialog)));
  gtk_widget_destroy(dialog);
  return ret;
}
