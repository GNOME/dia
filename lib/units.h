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
#ifndef UNITS_H
#define UNITS_H

#include "config.h"
#include "diavar.h"

typedef enum {
  DIA_UNIT_CENTIMETER,
  DIA_UNIT_DECIMETER,
  DIA_UNIT_FEET,
  DIA_UNIT_INCH,
  DIA_UNIT_METER,
  DIA_UNIT_MILLIMETER,
  DIA_UNIT_POINT,
  DIA_UNIT_PICA,
} DiaUnit;

typedef struct _DiaUnitDef DiaUnitDef;
struct _DiaUnitDef {
  char* name;
  char* unit;
  float factor;
  int digits; /** Number of digits after the decimal separator */
};

extern const DIAVAR DiaUnitDef units[];

#endif /* UNITS_H */
