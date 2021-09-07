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
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <math.h>

#include "dia-size-selector.h"
#include "widgets.h"

/**
 * DiaSizeSelector:
 *
 * A widget that selects two sizes, width and height, optionally keeping aspect
 * ratio. When created, aspect ratio is locked, but the user can unlock it. The
 * current users do not store aspect ratio, so we have to give a good default.
 */
struct _DiaSizeSelector {
  GtkBox hbox;
  GtkSpinButton *width, *height;
  GtkToggleButton *aspect_locked;
  double ratio;
  GtkAdjustment *last_adjusted;
};

G_DEFINE_TYPE (DiaSizeSelector, dia_size_selector, GTK_TYPE_BOX)

enum {
  DSS_VALUE_CHANGED,
  DSS_LAST_SIGNAL
};

static guint dss_signals[DSS_LAST_SIGNAL] = { 0 };


static void
dia_size_selector_class_init (DiaSizeSelectorClass *klass)
{
  dss_signals[DSS_VALUE_CHANGED] = g_signal_new ("value-changed",
                                                 G_TYPE_FROM_CLASS (klass),
                                                 G_SIGNAL_RUN_FIRST,
                                                 0, NULL, NULL,
                                                 g_cclosure_marshal_VOID__VOID,
                                                 G_TYPE_NONE, 0);
}


static void
dia_size_selector_adjust_width (DiaSizeSelector *ss)
{
  double height = gtk_spin_button_get_value (GTK_SPIN_BUTTON (ss->height));

  if (fabs (ss->ratio) > 1e-6) {
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (ss->width),
                               height * ss->ratio);
  }
}


static void
dia_size_selector_adjust_height (DiaSizeSelector *ss)
{
  double width = gtk_spin_button_get_value (GTK_SPIN_BUTTON (ss->width));

  if (fabs (ss->ratio) > 1e-6) {
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (ss->height),
                               width / ss->ratio);
  }
}


static void
dia_size_selector_ratio_callback (GtkAdjustment *limits, gpointer userdata)
{
  static gboolean in_progress = FALSE;
  DiaSizeSelector *ss = DIA_SIZE_SELECTOR (userdata);

  ss->last_adjusted = limits;

  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (ss->aspect_locked)) &&
      ss->ratio != 0.0) {

    if (in_progress) {
      return;
    }
    in_progress = TRUE;

    if (limits == gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (ss->width))) {
      dia_size_selector_adjust_height (ss);
    } else {
      dia_size_selector_adjust_width (ss);
    }

    in_progress = FALSE;
  }

  g_signal_emit (ss, dss_signals[DSS_VALUE_CHANGED], 0);
}


/*
 * Update the ratio of this DSS to be the ratio of width to height.
 * If height is 0, ratio becomes 0.0.
 */
static void
dia_size_selector_set_ratio (DiaSizeSelector *ss, double width, double height)
{
  if (height > 0.0) {
    ss->ratio = width/height;
  } else {
    ss->ratio = 0.0;
  }
}


static void
dia_size_selector_lock_pressed (GtkWidget *widget, gpointer data)
{
  DiaSizeSelector *ss = DIA_SIZE_SELECTOR (data);

  dia_size_selector_set_ratio (ss,
                               gtk_spin_button_get_value (GTK_SPIN_BUTTON (ss->width)),
                               gtk_spin_button_get_value (GTK_SPIN_BUTTON (ss->height)));
}

/* Possible args:  Init width, init height, digits */

static void
dia_size_selector_init (DiaSizeSelector *ss)
{
  GtkAdjustment *adj;

  ss->ratio = 0.0;

  /* Here's where we set up the real thing */
  adj = GTK_ADJUSTMENT (gtk_adjustment_new(1.0, 0.01, 10, 0.1, 1.0, 0));
  ss->width = GTK_SPIN_BUTTON (gtk_spin_button_new (adj, 1.0, 2));
  gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (ss->width), TRUE);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (ss->width), TRUE);
  gtk_box_pack_start (GTK_BOX (ss), GTK_WIDGET (ss->width), FALSE, TRUE, 0);
  gtk_widget_show (GTK_WIDGET (ss->width));

  adj = GTK_ADJUSTMENT (gtk_adjustment_new (1.0, 0.01, 10, 0.1, 1.0, 0));
  ss->height = GTK_SPIN_BUTTON (gtk_spin_button_new (adj, 1.0, 2));
  gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (ss->height), TRUE);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (ss->height), TRUE);
  gtk_box_pack_start (GTK_BOX (ss), GTK_WIDGET (ss->height), FALSE, TRUE, 0);
  gtk_widget_show (GTK_WIDGET (ss->height));

  /* Replace label with images */
  /* should make sure they're both unallocated when the widget dies.
  * That should happen in the "destroy" handler, where both should
  * be unref'd */
  ss->aspect_locked = GTK_TOGGLE_BUTTON (
    dia_toggle_button_new_with_icon_names ("dia-chain-unbroken",
                                           "dia-chain-broken"));

  gtk_container_set_border_width (GTK_CONTAINER (ss->aspect_locked), 0);

  gtk_box_pack_start (GTK_BOX (ss),
                      GTK_WIDGET (ss->aspect_locked),
                      FALSE,
                      TRUE,
                      0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (ss->aspect_locked), TRUE);
  gtk_widget_show (GTK_WIDGET (ss->aspect_locked));

  g_signal_connect (G_OBJECT (ss->aspect_locked),
                    "clicked", G_CALLBACK (dia_size_selector_lock_pressed),
                    ss);
  /* Make sure that the aspect ratio stays the same */
  g_signal_connect (G_OBJECT (gtk_spin_button_get_adjustment (ss->width)),
                    "value_changed", G_CALLBACK (dia_size_selector_ratio_callback),
                    ss);
  g_signal_connect (G_OBJECT (gtk_spin_button_get_adjustment (ss->height)),
                    "value_changed", G_CALLBACK (dia_size_selector_ratio_callback),
                    ss);
}


GtkWidget *
dia_size_selector_new (double width, double height)
{
  GtkWidget *wid = g_object_new (DIA_TYPE_SIZE_SELECTOR, NULL);

  dia_size_selector_set_size (DIA_SIZE_SELECTOR (wid), width, height);

  return wid;
}


void
dia_size_selector_set_size (DiaSizeSelector *ss, double width, double height)
{
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (ss->width), width);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (ss->height), height);
  /*
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ss->aspect_locked),
			       fabs(width - height) < 0.000001);
  */
  dia_size_selector_set_ratio (ss, width, height);
}


void
dia_size_selector_set_locked (DiaSizeSelector *ss, gboolean locked)
{
  if (!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (ss->aspect_locked)) &&
      locked) {
    dia_size_selector_set_ratio (ss,
                                 gtk_spin_button_get_value (GTK_SPIN_BUTTON (ss->width)),
                                 gtk_spin_button_get_value (GTK_SPIN_BUTTON (ss->height)));
  }
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (ss->aspect_locked), locked);
}


gboolean
dia_size_selector_get_size (DiaSizeSelector *ss,
                            double          *width,
                            double          *height)
{
  g_return_val_if_fail (DIA_IS_SIZE_SELECTOR (ss), FALSE);

  *width = gtk_spin_button_get_value (GTK_SPIN_BUTTON (ss->width));
  *height = gtk_spin_button_get_value (GTK_SPIN_BUTTON (ss->height));

  return gtk_toggle_button_get_active (ss->aspect_locked);
}
