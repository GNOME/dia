/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998, 1999 Alexander Larsson
 *
 * diaunitspinner.[ch] -- a spin button widget for length measurements.
 * Copyright (C) 1999 James Henstridge
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

#ifndef DIAUNITSPINNER_H
#define DIAUNITSPINNER_H

#include <gtk/gtk.h>

#if 0
#define DIA_UNIT_SPINNER(obj) GTK_CHECK_CAST(obj, dia_unit_spinner_get_type(), DiaUnitSpinner)
#define DIA_UNIT_SPINNER_CLASS(klass) GTK_CHECK_CLASS_CAST(klass, dia_unit_spinner_get_type(), DiaUnitSpinnerClass)
#define DIA_IS_UNIT_SPINNER(obj) GTK_CHECK_TYPE(obj, dia_unit_spinner_get_type())

typedef struct _DiaUnitSpinner DiaUnitSpinner;
typedef struct _DiaUnitSpinnerClass DiaUnitSpinnerClass;

typedef enum {
  DIA_UNIT_FEET,
  DIA_UNIT_METER,
  DIA_UNIT_DECIMETER,
  DIA_UNIT_MILLIMETER,
  DIA_UNIT_POINT,
  DIA_UNIT_CENTIMETER,
  DIA_UNIT_INCH,
} DiaUnit;

struct _DiaUnitSpinner {
  GtkSpinButton parent;

  DiaUnit unit_num;
};

struct _DiaUnitSpinnerClass {
  GtkSpinButtonClass parent_class;

};

GtkType    dia_unit_spinner_get_type  (void);
GtkWidget *dia_unit_spinner_new       (GtkAdjustment *adjustment,
				       guint digits,
				       DiaUnit adj_unit);
void       dia_unit_spinner_set_value (DiaUnitSpinner *self, gfloat val);
gfloat     dia_unit_spinner_get_value (DiaUnitSpinner *self);

#endif
#endif
