/* Dia -- a diagram creation/manipulation program -*- c -*-
 * Copyright (C) 1998 Alexander Larsson
 *
 * Property system for dia objects/shapes.
 * Copyright (C) 2000 James Henstridge
 * Copyright (C) 2001 Cyrille Chepelov
 * Copyright (C) 2009 Hans Breuer
 *
 * Property types for dictonaries.
 * These dictionaries are simple key/value pairs both of type string.
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
#ifndef PROP_DICT_H
#define PROP_DICT_H

#include "properties.h"
#include "dia_xml.h"

/*!
 * \brief Property for key=value storage
 * \extends _Property
 */
typedef struct {
  Property common;
  GHashTable *dict; /*!< subprops[i] is a GPtrArray of (Property *) */  
} DictProperty;

void prop_dicttypes_register(void);
#endif
