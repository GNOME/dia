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

#include "config.h"

#include <gtk/gtk.h>

#include "diamarshal.h"

#include "dia-xslt.h"

#include "dia-xslt-dialogue.h"


struct _DiaXsltDialogue {
  GtkDialog parent;

  GtkWidget *from;
  GtkWidget *to;

  GPtrArray *stylesheets;
};


G_DEFINE_FINAL_TYPE (DiaXsltDialogue, dia_xslt_dialogue, GTK_TYPE_DIALOG)


enum {
  PROP_0,
  PROP_STYLESHEETS,
  LAST_PROP
};
static GParamSpec *pspecs[LAST_PROP] = { NULL, };


enum {
  ZOOM,
  APPLY_TRANSFORMATION,
  N_SIGNALS
};
static guint signals[N_SIGNALS];


static void
dia_xslt_dialogue_dispose (GObject *object)
{
  DiaXsltDialogue *self = DIA_XSLT_DIALOGUE (object);

  g_clear_pointer (&self->stylesheets, g_ptr_array_unref);

  G_OBJECT_CLASS (dia_xslt_dialogue_parent_class)->dispose (object);
}


static void
dia_xslt_dialogue_set_property (GObject      *object,
                                guint         property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  DiaXsltDialogue *self = DIA_XSLT_DIALOGUE (object);

  switch (property_id) {
    case PROP_STYLESHEETS:
      self->stylesheets = g_value_dup_boxed (value);

      for (int i = 0; i < self->stylesheets->len; i++) {
        DiaXsltStylesheet *sheet = g_ptr_array_index (self->stylesheets, i);

        gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (self->from),
                                        dia_xslt_stylesheet_get_name (sheet));
      }

      gtk_combo_box_set_active (GTK_COMBO_BOX (self->from), 0);

      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}


static void
dia_xslt_dialogue_response (GtkDialog *dialog, int response)
{
  DiaXsltDialogue *self = DIA_XSLT_DIALOGUE (dialog);

  if (response == GTK_RESPONSE_OK) {
    int active_from = gtk_combo_box_get_active (GTK_COMBO_BOX (self->from));
    int active_to = gtk_combo_box_get_active (GTK_COMBO_BOX (self->to));
    DiaXsltStylesheet *stylesheet;
    DiaXsltStylesheetTarget **targets;
    size_t n_targets;

    g_return_if_fail (active_from <= self->stylesheets->len);

    stylesheet = g_ptr_array_index (self->stylesheets, active_from);

    dia_xslt_stylesheet_get_targets (stylesheet, &targets, &n_targets);

    g_return_if_fail (active_to <= n_targets);

    g_signal_emit (dialog,
                   signals[APPLY_TRANSFORMATION],
                   0,
                   stylesheet,
                   targets[active_to]);
  }

  gtk_widget_destroy (GTK_WIDGET (dialog));
}


static void
from_changed (GtkComboBox     *combo,
              DiaXsltDialogue *self)
{
  DiaXsltStylesheet *from =
    g_ptr_array_index (self->stylesheets, gtk_combo_box_get_active (combo));
  DiaXsltStylesheetTarget **targets;
  size_t n_targets;

  gtk_combo_box_text_remove_all (GTK_COMBO_BOX_TEXT (self->to));

  dia_xslt_stylesheet_get_targets (from, &targets, &n_targets);

  for (size_t i = 0; i < n_targets; i++) {
    gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (self->to),
                                    dia_xslt_stylesheet_target_get_name (targets[i]));
  }
}


static void
dia_xslt_dialogue_class_init (DiaXsltDialogueClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GtkDialogClass *dialog_class = GTK_DIALOG_CLASS (klass);

  object_class->dispose = dia_xslt_dialogue_dispose;
  object_class->set_property = dia_xslt_dialogue_set_property;

  dialog_class->response = dia_xslt_dialogue_response;

  pspecs[PROP_STYLESHEETS] =
    g_param_spec_boxed ("stylesheets", NULL, NULL,
                        G_TYPE_PTR_ARRAY,
                        G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, LAST_PROP, pspecs);

  signals[APPLY_TRANSFORMATION] = g_signal_new ("apply-transformation",
                                                G_TYPE_FROM_CLASS (klass),
                                                G_SIGNAL_RUN_LAST,
                                                0,
                                                NULL, NULL,
                                                dia_marshal_VOID__BOXED_BOXED,
                                                G_TYPE_NONE,
                                                2,
                                                DIA_XSLT_TYPE_STYLESHEET,
                                                DIA_XSLT_TYPE_STYLESHEET_TARGET);
  g_signal_set_va_marshaller (signals[APPLY_TRANSFORMATION],
                              G_TYPE_FROM_CLASS (klass),
                              dia_marshal_VOID__BOXED_BOXEDv);

  gtk_widget_class_set_template_from_resource (widget_class,
                                               DIA_APPLICATION_PATH "xslt/dia-xslt-dialogue.ui");

  gtk_widget_class_bind_template_child (widget_class, DiaXsltDialogue, from);
  gtk_widget_class_bind_template_child (widget_class, DiaXsltDialogue, to);

  gtk_widget_class_bind_template_callback (widget_class, from_changed);
}


static void
dia_xslt_dialogue_init (DiaXsltDialogue *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}
