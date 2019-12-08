/*
 * File: plug-ins/xslt/xsltdialog.c
 *
 * Made by Matthieu Sozeau <mattam@netcourrier.com>
 *
 * Started on  Thu May 16 20:30:42 2002 Matthieu Sozeau
 * Last update Fri May 17 00:30:24 2002 Matthieu Sozeau
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

/*
 * Opens a dialog for export options
 */

#include "xslt.h"
#include <stdio.h>

#include <gtk/gtk.h>

static GtkWidget *dialog;

static void xslt_dialog_respond (GtkWidget *widget,
                                 gint       response_id,
                                 gpointer   user_data);

static void
from_changed (GtkComboBox *combo,
              GtkComboBox *to)
{
  toxsl_t *cur_to;
  fromxsl_t *from = g_ptr_array_index (froms,
                                       gtk_combo_box_get_active (combo));
  GtkListStore *store;

  // GTK3: gtk_combo_box_text_remove_all
  store = GTK_LIST_STORE (gtk_combo_box_get_model (GTK_COMBO_BOX (to)));
  gtk_list_store_clear (store);

  xsl_from = from;

  cur_to = from->xsls;
  while(cur_to != NULL) {
    gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (to), cur_to->name);

    cur_to = cur_to->next;
  }
}

static void
to_changed (GtkComboBox *combo,
            gpointer     data)
{
  toxsl_t *to = xsl_from->xsls + gtk_combo_box_get_active (combo);

  xsl_to = to;
}

void
xslt_dialog_create (void)
{
  GtkWidget *box, *vbox;
  GtkWidget *label;
  GtkWidget *from, *to;

  g_return_if_fail (froms != NULL);

  dialog = gtk_dialog_new_with_buttons (_("Export through XSLT"),
                                        NULL, 0,
                                        _("_Cancel"), GTK_RESPONSE_CANCEL,
                                        _("_OK"), GTK_RESPONSE_OK,
                                        NULL);

  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);

  box = gtk_dialog_get_content_area (GTK_DIALOG (dialog));

  vbox = gtk_vbox_new (FALSE, 5);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 5);
  gtk_container_set_border_width (GTK_CONTAINER (box), 10);

  label = gtk_label_new (_("From:"));

  from = gtk_combo_box_text_new ();
  to = gtk_combo_box_text_new ();

  g_signal_connect (from, "changed", G_CALLBACK (from_changed), to);
  g_signal_connect (to, "changed", G_CALLBACK (to_changed), NULL);

  for (int i = 0; i < froms->len; i++) {
    fromxsl_t *cur_f = g_ptr_array_index (froms, i);

    gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (from), cur_f->name);
  }

  // Select the first item
  gtk_combo_box_set_active (GTK_COMBO_BOX (from), 0);

  gtk_widget_show (from);
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), from, FALSE, TRUE, 0);

  gtk_widget_show_all (vbox);

  gtk_box_pack_start (GTK_BOX (box), vbox, FALSE, TRUE, 0);

  vbox = gtk_vbox_new (FALSE, 5);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 5);
  gtk_container_set_border_width (GTK_CONTAINER (box), 10);

  label = gtk_label_new (_("To:"));

  gtk_widget_show (to);
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), to, FALSE, TRUE, 0);

  gtk_widget_show_all (vbox);

  gtk_box_pack_start (GTK_BOX (box), vbox, FALSE, TRUE, 0);

  gtk_widget_show_all (box);

  g_signal_connect (G_OBJECT (dialog), "response",
                    G_CALLBACK (xslt_dialog_respond), NULL);
  g_signal_connect (G_OBJECT (dialog), "delete_event",
                    G_CALLBACK (gtk_widget_hide), NULL);

  gtk_widget_show (dialog);
}

void
xslt_clear (void)
{
  gtk_widget_destroy (dialog);
}

void
xslt_dialog_respond (GtkWidget *widget,
                     gint       response_id,
                     gpointer   user_data)
{
  gtk_widget_hide (dialog);
  if (response_id == GTK_RESPONSE_OK) {
    xslt_ok ();
  }
}
