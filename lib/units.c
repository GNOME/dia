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
 *
 * Copyright © 2020 Zander Brown <zbrown@gnome.org>
 */

#include "config.h"
#include "units.h"

#include <glib/gi18n-lib.h>


/**
 * dia_unit_get_name:
 * @self: the #DiaUnit
 *
 * Get the human readable localised name for @self
 *
 * Since: 0.98
 */
const char *
dia_unit_get_name (DiaUnit self)
{
  switch (self) {
    case DIA_UNIT_CENTIMETER:
      return _("Centimeter");
    case DIA_UNIT_DECIMETER:
      return _("Decimeter");
    case DIA_UNIT_FEET:
      return _("Feet");
    case DIA_UNIT_INCH:
      return _("Inch");
    case DIA_UNIT_METER:
      return _("Meter");
    case DIA_UNIT_MILLIMETER:
      return _("Millimeter");
    case DIA_UNIT_POINT:
      return _("Point");
    case DIA_UNIT_PICA:
      return _("Pica");
    case DIA_LAST_UNIT:
    default:
      g_return_val_if_reached (NULL);
  }
}


/**
 * dia_unit_get_symbol:
 * @self: the #DiaUnit
 *
 * Get the symbol for @self, e.g. cm
 *
 * Since: 0.98
 */
const char *
dia_unit_get_symbol (DiaUnit self)
{
  switch (self) {
    case DIA_UNIT_CENTIMETER:
      return "cm";
    case DIA_UNIT_DECIMETER:
      return "dm";
    case DIA_UNIT_FEET:
      return "ft";
    case DIA_UNIT_INCH:
      return "in";
    case DIA_UNIT_METER:
      return "m";
    case DIA_UNIT_MILLIMETER:
      return "mm";
    case DIA_UNIT_POINT:
      return "pt";
    case DIA_UNIT_PICA:
      return "pi";
    case DIA_LAST_UNIT:
    default:
      g_return_val_if_reached (NULL);
  }
}


/**
 * dia_unit_get_factor:
 * @self: the #DiaUnit
 *
 * The number of %DIA_UNIT_POINT per @self
 *
 * Since: 0.98
 */
double
dia_unit_get_factor (DiaUnit self)
{
  switch (self) {
    case DIA_UNIT_CENTIMETER:
      return 28.346457;
    case DIA_UNIT_DECIMETER:
      return 283.46457;
    case DIA_UNIT_FEET:
      return 864;
    case DIA_UNIT_INCH:
      return 72;
    case DIA_UNIT_METER:
      return 2834.6457;
    case DIA_UNIT_MILLIMETER:
      return 2.8346457;
    case DIA_UNIT_POINT:
      return 1;
    case DIA_UNIT_PICA:
      return 12;
    case DIA_LAST_UNIT:
    default:
      g_return_val_if_reached (-1);
  }
}


/**
 * dia_unit_get_digits:
 * @self: the #DiaUnit
 *
 * Number of digits after the decimal separator
 *
 * Since: 0.98
 */
int
dia_unit_get_digits (DiaUnit self)
{
  switch (self) {
    case DIA_UNIT_CENTIMETER:
      return 2;
    case DIA_UNIT_DECIMETER:
      return 3;
    case DIA_UNIT_FEET:
      return 4;
    case DIA_UNIT_INCH:
      return 3;
    case DIA_UNIT_METER:
      return 4;
    case DIA_UNIT_MILLIMETER:
      return 2;
    case DIA_UNIT_POINT:
      return 2;
    case DIA_UNIT_PICA:
      return 2;
    case DIA_LAST_UNIT:
    default:
      g_return_val_if_reached (-1);
  }
}
