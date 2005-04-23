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

#ifndef PYDIA_OBJECT_H
#define PYDIA_OBJECT_H

#include <Python.h>

#include "lib/object.h"

typedef struct {
    PyObject_HEAD
    DiaObject *object;
} PyDiaObject;

typedef struct {
    PyObject_HEAD
    DiaObjectType *otype;
} PyDiaObjectType;

extern PyTypeObject PyDiaObject_Type;
extern PyTypeObject PyDiaObjectType_Type;

PyObject *PyDiaObject_New(DiaObject *object);
PyObject *PyDiaObjectType_New(DiaObjectType *otype);

#define PyDiaObject_Check(o) ((o)->ob_type == &PyDiaObject_Type)
#define PyDiaObjectType_Check(o) ((o)->ob_type == &PyDiaObjectType_Type)

#ifdef _MSC_VER
#  pragma warning(default:4047)
  /* see: Python FAQ 3.24 "Initializer not a constant." */
#  if 1 /* set to 0 to get all todos */
#    undef PyObject_HEAD_INIT
#    ifdef Py_TRACE_REFS
#      define PyObject_HEAD_INIT(a) \
         0, 0, 1, NULL, /* must be set by init function */
#    else
#      define PyObject_HEAD_INIT(a) \
         1, NULL, /* must be set by init function */
#    endif /* Py_TRACE_REFS */
#  endif
#endif

#endif
