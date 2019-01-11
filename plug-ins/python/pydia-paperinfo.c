/* Python plug-in for dia
 * Copyright (C) 1999  James Henstridge
 * Copyright (C) 2007  Hans Breuer
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

#include "pydia-paperinfo.h"
#include "pydia-object.h" /* for PyObject_HEAD_INIT */

#include <structmember.h> /* PyMemberDef */

/*
 * New
 */
PyObject* PyDiaPaperinfo_New (const PaperInfo *paper)
{
  PyDiaPaperinfo *self;
  
  self = PyObject_NEW(PyDiaPaperinfo, &PyDiaPaperinfo_Type);
  if (!self) return NULL;
  
  self->paper = paper;

  return (PyObject *)self;
}

/*
 * Dealloc
 */
static void
PyDiaPaperinfo_Dealloc(PyDiaPaperinfo *self)
{
  /* we dont own the object */
  PyObject_DEL(self);
}

/*
 * Compare
 */
static int
PyDiaPaperinfo_Compare(PyDiaPaperinfo *self,
                   PyDiaPaperinfo *other)
{
  return memcmp(&(self->paper), &(other->paper), sizeof(PaperInfo));
}

/*
 * Hash
 */
static long
PyDiaPaperinfo_Hash(PyObject *self)
{
  return (long)self;
}

/*
 * GetAttr
 */
static PyObject *
PyDiaPaperinfo_GetAttr(PyDiaPaperinfo *self, gchar *attr)
{
  if (!strcmp(attr, "__members__"))
    return Py_BuildValue("[sssss]", "name", "is_portrait", 
                                    "scaling",
                                    "width", "height");
  else if (!strcmp(attr, "name"))
    return PyString_FromString(self->paper->name);
  else if (!strcmp(attr, "is_portrait"))
    return PyInt_FromLong(self->paper->is_portrait);
  else if (!strcmp(attr, "scaling"))
    return PyFloat_FromDouble(self->paper->scaling);
  else if (!strcmp(attr, "width"))
    return PyFloat_FromDouble(self->paper->width);
  else if (!strcmp(attr, "height"))
    return PyFloat_FromDouble(self->paper->height);

  PyErr_SetString(PyExc_AttributeError, attr);
  return NULL;
}

/*
 * Repr / _Str
 */
static PyObject *
PyDiaPaperinfo_Str(PyDiaPaperinfo *self)
{
  PyObject* py_s;
  gchar* s = g_strdup_printf("%s - %fx%f %f%%",
                             self->paper->name ? self->paper->name : "(null)", 
			     self->paper->width, self->paper->height,
			     self->paper->scaling);
  py_s = PyString_FromString(s);
  g_free (s);
  return py_s;
}

#define T_INVALID -1 /* can't allow direct access due to pyobject->paper indirection */
static PyMemberDef PyDiaPaperinfo_Members[] = {
    { "name", T_INVALID, 0, RESTRICTED|READONLY,
      "string: paper name, e.g. A4 or Letter" },
    { "is_portrait", T_INVALID, 0, RESTRICTED|READONLY,
      "int: paper orientation" },
    { "scaling", T_INVALID, 0, RESTRICTED|READONLY,
      "real: factor to zoom the diagram on the paper" },
    { "width", T_INVALID, 0, RESTRICTED|READONLY,
      "real: width of the drawable area (sans margins)" },
    { "height", T_INVALID, 0, RESTRICTED|READONLY,
      "real: height of the drawable area (sans margins)" },
    { NULL }
};
/*
 * Python objetcs
 */
PyTypeObject PyDiaPaperinfo_Type = {
    PyObject_HEAD_INIT(NULL)
    0,
    "dia.Paperinfo",
    sizeof(PyDiaPaperinfo),
    0,
    (destructor)PyDiaPaperinfo_Dealloc,
    (printfunc)0,
    (getattrfunc)PyDiaPaperinfo_GetAttr,
    (setattrfunc)0,
    (cmpfunc)PyDiaPaperinfo_Compare,
    (reprfunc)0,
    0,
    0,
    0,
    (hashfunc)PyDiaPaperinfo_Hash,
    (ternaryfunc)0,
    (reprfunc)PyDiaPaperinfo_Str,
    (getattrofunc)0,
    (setattrofunc)0,
    (PyBufferProcs *)0,
    0L, /* Flags */
    "dia.Paperinfo is part of dia.DiagramData describing the paper",
    (traverseproc)0,
    (inquiry)0,
    (richcmpfunc)0,
    0, /* tp_weakliszoffset */
    (getiterfunc)0,
    (iternextfunc)0,
    0, /* tp_methods */
    PyDiaPaperinfo_Members, /* tp_members */
    0
};
