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

#pragma once

#include "dia-lib-enums.h"

/**
 * DiaUnit:
 * @DIA_UNIT_CENTIMETER: 0.01 metre
 * @DIA_UNIT_DECIMETER: 0.1 metre
 * @DIA_UNIT_FEET: 0.3048 metre (12 inch)
 * @DIA_UNIT_INCH: 0.0254 metre
 * @DIA_UNIT_METER: 1 metre
 * @DIA_UNIT_MILLIMETER: 0.001 metre
 * @DIA_UNIT_POINT: 1 pixel
 * @DIA_UNIT_PICA: 12 pixels
 * @DIA_LAST_UNIT: Dummy value indicating the number of units
 *
 * Since: dawn-of-time
 */
typedef enum /*< enum,prefix=DIA >*/ {
  DIA_UNIT_CENTIMETER, /*< nick=centimetre >*/
  DIA_UNIT_DECIMETER,  /*< nick=decimetre >*/
  DIA_UNIT_FEET,       /*< nick=feet >*/
  DIA_UNIT_INCH,       /*< nick=inch >*/
  DIA_UNIT_METER,      /*< nick=metre >*/
  DIA_UNIT_MILLIMETER, /*< nick=millimetre >*/
  DIA_UNIT_POINT,      /*< nick=point >*/
  DIA_UNIT_PICA,       /*< nick=pica >*/
  DIA_LAST_UNIT        /*< skip >*/
} DiaUnit;


const char *dia_unit_get_name   (DiaUnit self);
const char *dia_unit_get_symbol (DiaUnit self);
double      dia_unit_get_factor (DiaUnit self);
int         dia_unit_get_digits (DiaUnit self);
