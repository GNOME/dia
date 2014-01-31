/* Dia -- a diagram creation/manipulation program -*- c -*-
 * Copyright (C) 1998 Alexander Larsson
 *
 * Property system for dia objects/shapes.
 * Copyright (C) 2000 James Henstridge
 * Copyright (C) 2001 Cyrille Chepelov
 * Major restructuration done in August 2001 by C. Chepelov
 *
 * Property types for static and dynamic arrays.
 * These arrays are simply arrays of records whose structures are themselves
 * described by property descriptors.
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
#ifndef PROP_SDARRAY_H
#define PROP_SDARRAY_H

#include "properties.h"
#include "dia_xml.h"

/*!
 * \brief Property for an Array of _Property
 * \extends _Property
 */
typedef struct {
  Property common;     /*!< inheritance */
  GPtrArray *ex_props; /*!< "example" properties. */
  GPtrArray *records;  /*!< subprops[i] is a GPtrArray of (Property *) */  
} ArrayProperty;

void prop_sdarray_register(void);
#endif
