/* -*- Mode: C; c-basic-offset: 4 -*- */
/* Python plug-in for dia
 * Copyright (C) 1999  James Henstridge
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef PYDIA_CPOINT_H
#define PYDIA_CPOINT_H

#include <Python.h>

#include "connectionpoint.h"

typedef struct {
    PyObject_HEAD
    ConnectionPoint *cpoint;
} PyDiaConnectionPoint;


extern PyTypeObject PyDiaConnectionPoint_Type;

PyObject *PyDiaConnectionPoint_New(ConnectionPoint *cpoint);
#define PyDiaConnectionPoint_Check(o) ((o)->ob_type == &PyDiaConnectionPoint_Type)

#endif
