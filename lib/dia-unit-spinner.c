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

#include "dia-unit-spinner.h"

/**
 * DiaUnitSpinner:
 *
 * A Spinner that allows a 'favoured' unit to display in. External access
 * to the value still happens in cm, but display is in the favored unit.
 * Internally, the value is kept in the favored unit to a) allow proper
 * limits, and b) avoid rounding problems while editing.
 *
 * Since: dawn-of-time
 */
struct _DiaUnitSpinner {
  GtkSpinButton parent;

  DiaUnit unit_num;
};

G_DEFINE_TYPE (DiaUnitSpinner, dia_unit_spinner, GTK_TYPE_SPIN_BUTTON)


static void
dia_unit_spinner_class_init (DiaUnitSpinnerClass *klass)
{
}


static void
dia_unit_spinner_init (DiaUnitSpinner *self)
{
  self->unit_num = DIA_UNIT_CENTIMETER;
}


/*
  Callback functions for the "input" and "output" signals emitted by
  GtkSpinButton. All the normal work is done by the spin button, we
  simply modify how the text in the GtkEntry is treated.
*/


static gboolean
dia_unit_spinner_input (DiaUnitSpinner *self, double *value)
{
  double val, factor = 1.0;
  char *extra = NULL;

  val = g_strtod (gtk_entry_get_text (GTK_ENTRY (self)), &extra);

  /* get rid of extra white space after number */
  while (*extra && g_ascii_isspace (*extra)) {
    extra++;
  }

  if (*extra) {
    for (int i = 0; i < DIA_LAST_UNIT; i++) {
      if (!g_ascii_strcasecmp (dia_unit_get_symbol (i), extra)) {
        factor = dia_unit_get_factor (i) /
                  dia_unit_get_factor (self->unit_num);
        break;
      }
    }
  }

  /* convert to preferred units */
  val *= factor;

  /* Store value in the location provided by the signal emitter. */
  *value = val;

  /* Return true, so that the default input function is not invoked. */
  return TRUE;
}


static gboolean
dia_unit_spinner_output (DiaUnitSpinner *self)
{
  char buf[256];
  GtkSpinButton *sbutton = GTK_SPIN_BUTTON (self);
  GtkAdjustment *adjustment = gtk_spin_button_get_adjustment (sbutton);

  g_snprintf (buf,
              sizeof(buf),
              "%0.*f %s",
              gtk_spin_button_get_digits(sbutton),
              gtk_adjustment_get_value(adjustment),
              dia_unit_get_symbol (self->unit_num));
  gtk_entry_set_text (GTK_ENTRY(self), buf);

  /* Return true, so that the default output function is not invoked. */
  return TRUE;
}


GtkWidget *
dia_unit_spinner_new (GtkAdjustment *adjustment, DiaUnit adj_unit)
{
  DiaUnitSpinner *self;

  if (adjustment) {
    g_return_val_if_fail (GTK_IS_ADJUSTMENT (adjustment), NULL);
  }

  self = g_object_new (DIA_TYPE_UNIT_SPINNER, NULL);
  gtk_entry_set_activates_default (GTK_ENTRY (self), TRUE);
  self->unit_num = adj_unit;

  gtk_spin_button_configure (GTK_SPIN_BUTTON (self),
                             adjustment, 0.0, dia_unit_get_digits (adj_unit));

  g_signal_connect (GTK_SPIN_BUTTON (self), "output",
                    G_CALLBACK (dia_unit_spinner_output),
                    NULL);
  g_signal_connect (GTK_SPIN_BUTTON (self), "input",
                    G_CALLBACK (dia_unit_spinner_input),
                    NULL);

  return GTK_WIDGET (self);
}


/**
 * dia_unit_spinner_set_value:
 * @self: the DiaUnitSpinner
 * @val: value in %DIA_UNIT_CENTIMETER
 *
 * Set the value (in cm).
 *
 * Since: dawn-of-time
 */
void
dia_unit_spinner_set_value (DiaUnitSpinner *self, double val)
{
  GtkSpinButton *sbutton = GTK_SPIN_BUTTON(self);

  gtk_spin_button_set_value (sbutton,
                             val /
                             (dia_unit_get_factor (self->unit_num) /
                              (dia_unit_get_factor (DIA_UNIT_CENTIMETER))));
}


/**
 * dia_unit_spinner_get_value:
 * @self: the DiaUnitSpinner
 *
 * Get the value (in cm)
 *
 * Returns: The value in %DIA_UNIT_CENTIMETER
 *
 * Since: dawn-of-time
 */
double
dia_unit_spinner_get_value (DiaUnitSpinner *self)
{
  GtkSpinButton *sbutton = GTK_SPIN_BUTTON(self);

  return gtk_spin_button_get_value (sbutton) *
                    (dia_unit_get_factor (self->unit_num) /
                     dia_unit_get_factor (DIA_UNIT_CENTIMETER));
}


/**
 * dia_unit_spinner_set_upper:
 * @self: the DiaUnitSpinner
 * @val: value in %DIA_UNIT_CENTIMETER
 *
 * Must manipulate the limit values through this to also consider unit.
 *
 * Given value is in centimeter.
 *
 * Since: dawn-of-time
 */
void
dia_unit_spinner_set_upper (DiaUnitSpinner *self, double val)
{
  val /= (dia_unit_get_factor (self->unit_num) /
          dia_unit_get_factor (DIA_UNIT_CENTIMETER));

  gtk_adjustment_set_upper (
    gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (self)), val);
}

