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

#pragma once

#include "dia-lib-enums.h"

G_BEGIN_DECLS

typedef enum /*< enum >*/ {
  DIA_LINE_CAPS_DEFAULT = -1, /* default usually butt, this is unset */
  DIA_LINE_CAPS_BUTT = 0,
  DIA_LINE_CAPS_ROUND,
  DIA_LINE_CAPS_PROJECTING,
} DiaLineCaps;

typedef enum /*< enum >*/ {
  DIA_LINE_JOIN_DEFAULT = -1, /* default usually miter, this is unset */
  DIA_LINE_JOIN_MITER = 0,
  DIA_LINE_JOIN_ROUND,
  DIA_LINE_JOIN_BEVEL
} DiaLineJoin;

typedef enum /*< enum >*/ {
  DIA_LINE_STYLE_DEFAULT = -1, /* default usually solid, this is unset */
  DIA_LINE_STYLE_SOLID = 0,
  DIA_LINE_STYLE_DASHED,
  DIA_LINE_STYLE_DASH_DOT,
  DIA_LINE_STYLE_DASH_DOT_DOT,
  DIA_LINE_STYLE_DOTTED
} DiaLineStyle;

typedef enum /*< enum >*/ {
  DIA_FILL_STYLE_SOLID
} DiaFillStyle;

typedef enum /*< enum >*/ {
  DIA_ALIGN_LEFT,
  DIA_ALIGN_CENTRE,
  DIA_ALIGN_RIGHT
} DiaAlignment;

typedef enum /*< enum >*/ {
  DIA_TEXT_FIT_NEVER,
  DIA_TEXT_FIT_WHEN_NEEDED,
  DIA_TEXT_FIT_ALWAYS
} DiaTextFitting;

/* Used to be in widgets.h polluting a lot of object implementations */
#define DEFAULT_LINESTYLE LINESTYLE_SOLID
#define DEFAULT_LINESTYLE_DASHLEN 1.0

G_END_DECLS
