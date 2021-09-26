/* Dia -- a diagram creation/manipulation program -*- c -*-
 * Copyright (C) 1998 Alexander Larsson
 *
 * Property system for dia objects/shapes.
 * Copyright (C) 2000 James Henstridge
 * Copyright (C) 2001 Cyrille Chepelov
 * Major restructuration done in August 2001 by C. Chépélov
 *
 * Property types for "attribute" types (line style, arrow, colour, font)
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

#include "properties.h"
#include "dia_xml.h"

G_BEGIN_DECLS

/**
 * LinestyleProperty:
 *
 * #Property for #DiaLineStyle
 */
typedef struct {
  Property common;
  DiaLineStyle style;
  double dash;
} LinestyleProperty;

/*!
 * \brief Property for _Arrow
 * \extends _Property
 */
typedef struct {
  Property common;
  Arrow arrow_data;
} ArrowProperty;

/*!
 * \brief Property for _Color
 * \extends _Property
 */
typedef struct {
  Property common;
  Color color_data;
} ColorProperty;

/*!
 * \brief Property for _DiaFont
 * \extends _Property
 */
typedef struct {
  Property common;
  DiaFont *font_data;
} FontProperty;

void prop_attr_register(void);

G_END_DECLS
