/* Python plug-in for dia
 * Copyright (C) 1999  James Henstridge
 * Copyright (C) 2000  Hans Breuer
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

#include <config.h>

#include <glib.h>

#include "pydia-object.h"
#include "pydia-properties.h"
#include "pydia-geometry.h"
#include "pydia-font.h"
#include "pydia-color.h"

/*
 * New
 */
PyObject* PyDiaProperty_New (Property* property)
{
  PyDiaProperty *self;
  
  self = PyObject_NEW(PyDiaProperty, &PyDiaProperty_Type);
  if (!self) return NULL;
  
  self->property = property->ops->copy (property);

  return (PyObject *)self;
}

/*
 * Dealloc
 */
static void
PyDiaProperty_Dealloc(PyDiaProperty *self)
{
  self->property->ops->free(self->property);
  PyMem_DEL(self);
}

/*
 * Compare
 */
static int
PyDiaProperty_Compare(PyDiaProperty *self,
                      PyDiaProperty *other)
{
  /* ??? */
  return memcmp(&(self->property), &(other->property), sizeof(Property));
}

/*
 * Hash
 */
static long
PyDiaProperty_Hash(PyObject *self)
{
  return (long)self;
}

/*
 * GetAttr
 */
static PyObject *
PyDiaProperty_GetAttr(PyDiaProperty *self, gchar *attr)
{
  if (!strcmp(attr, "__members__"))
    return Py_BuildValue("[sss]", "name", "type", "value");
  else if (!strcmp(attr, "name"))
    return PyString_FromString(self->property->name);
  else if (!strcmp(attr, "type"))
    return PyInt_FromLong(self->property->type);
  else if (!strcmp(attr, "value")) {
#ifdef THE_PROP_TYPE_ID_IS_INTEGRAL
    switch (self->property->type) {
    case PROP_TYPE_CHAR :
      return PyInt_FromLong(((CharProperty*)self->property)->char_data);
    case PROP_TYPE_BOOL :
      return PyInt_FromLong(self->property.d.bool_data);
    case PROP_TYPE_INT :
    case PROP_TYPE_ENUM :
    case PROP_TYPE_LINESTYLE :
      return PyInt_FromLong(self->property.d.int_data);
    case PROP_TYPE_REAL :
      return PyFloat_FromDouble(self->property.d.real_data);
    case PROP_TYPE_STRING :
    case PROP_TYPE_FILE :
      if (self->property.d.string_data)
        return PyString_FromString(self->property.d.string_data);
      else
        return PyString_FromString("(NULL)");
    case PROP_TYPE_POINT :
      return PyDiaPoint_New (&(self->property.d.point_data));
    case PROP_TYPE_POINTARRAY :
      return PyDiaPointTuple_New (self->property.d.ptarray_data.pts,
                                  self->property.d.ptarray_data.npts);
    case PROP_TYPE_BEZPOINT :
      return PyDiaBezPoint_New (&(self->property.d.bpoint_data));
    case PROP_TYPE_BEZPOINTARRAY :
      return PyDiaBezPointTuple_New (self->property.d.bptarray_data.pts,
                                     self->property.d.bptarray_data.npts);
    case PROP_TYPE_RECT :
      return PyDiaRectangle_New (&(self->property.d.rect_data), NULL);
    case PROP_TYPE_ARROW :
      return PyDiaArrow_New (&(self->property.d.arrow_data));
    case PROP_TYPE_COLOUR :
      return PyDiaColor_New (&(self->property.d.colour_data));
    case PROP_TYPE_FONT :
      return PyDiaFont_New (self->property.d.font_data);
    default :
      Py_INCREF(Py_None);
      return Py_None;
    } /* switch */
#endif
  }

  PyErr_SetString(PyExc_AttributeError, attr);
  return NULL;
}

/*
 * Repr / _Str
 */
static PyObject *
PyDiaProperty_Str(PyDiaProperty *self)
{
  PyObject* py_s;
  gchar* tname = "OTHER";
  gchar* s;

#ifdef THE_PROP_TYPE_ID_IS_INTEGRAL
#define CASE_STR(s) case PROP_TYPE_##s : tname = #s; break;
  switch (self->property.type) {
  CASE_STR(INVALID)
  CASE_STR(CHAR)
  CASE_STR(BOOL)
  CASE_STR(INT)
  CASE_STR(ENUM)
  CASE_STR(REAL)
  CASE_STR(STRING)
  CASE_STR(POINT)
  CASE_STR(POINTARRAY)
  CASE_STR(BEZPOINT)
  CASE_STR(BEZPOINTARRAY)
  CASE_STR(RECT)
  CASE_STR(LINESTYLE)
  CASE_STR(ARROW)
  CASE_STR(COLOUR)
  CASE_STR(FONT)
  CASE_STR(FILE)
  default :
    tname = "OTHER";
  }
#undef CASE_STR
  s = g_strdup_printf("<DiaProperty at 0x%08x, \"%s\", %s>",
                      self,
                      self->property.name,
                      tname);
#else
  s = g_strdup_printf("<DiaProperty at 0x%08x, \"%s\", %s>",
                      self,
                      self->property->name,
                      self->property->type);
#endif
  py_s = PyString_FromString(s);
  g_free (s);
  return py_s;
}

/*
 * Python object
 */
PyTypeObject PyDiaProperty_Type = {
    PyObject_HEAD_INIT(&PyType_Type)
    0,
    "DiaProperty",
    sizeof(PyDiaProperty),
    0,
    (destructor)PyDiaProperty_Dealloc,
    (printfunc)0,
    (getattrfunc)PyDiaProperty_GetAttr,
    (setattrfunc)0,
    (cmpfunc)PyDiaProperty_Compare,
    (reprfunc)0,
    0,
    0,
    0,
    (hashfunc)PyDiaProperty_Hash,
    (ternaryfunc)0,
    (reprfunc)PyDiaProperty_Str,
    0L,0L,0L,0L,
    NULL
};
