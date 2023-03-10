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

#include "config.h"

#include <glib/gi18n-lib.h>

#include "pydia-paperinfo.h"
#include "pydia-object.h" /* for PyObject_HEAD_INIT */

#include <structmember.h> /* PyMemberDef */


/*
 * New
 */
PyObject *
PyDiaPaperinfo_New (const PaperInfo *paper)
{
  PyDiaPaperinfo *self;

  self = PyObject_NEW (PyDiaPaperinfo, &PyDiaPaperinfo_Type);

  if (!self) return NULL;

  self->paper = paper;

  return (PyObject *)self;
}


/*
 * Dealloc
 */
static void
PyDiaPaperinfo_Dealloc (PyObject *self)
{
  /* we dont own the object */
  PyObject_DEL (self);
}


/*
 * Compare
 */
static PyObject *
PyDiaPaperinfo_Compare (PyObject *a,
                        PyObject *b,
                        int       op)
{
  PyDiaPaperinfo *self = (PyDiaPaperinfo *) a;
  PyDiaPaperinfo *other = (PyDiaPaperinfo *) b;
  int cmp = memcmp (&self->paper, &other->paper, sizeof (DiaMatrix));
  PyObject *ret;

  switch (op) {
    case Py_EQ:
      ret = cmp == 0 ? Py_True : Py_False;
      break;
    case Py_NE:
      ret = cmp != 0 ? Py_True : Py_False;
      break;
    case Py_LE:
      ret = cmp <= 0 ? Py_True : Py_False;
      break;
    case Py_GE:
      ret = cmp >= 0 ? Py_True : Py_False;
      break;
    case Py_LT:
      ret = cmp < 0 ? Py_True : Py_False;
      break;
    case Py_GT:
      ret = cmp > 0 ? Py_True : Py_False;
      break;
    default:
      ret = Py_NotImplemented;
      break;
  }

  Py_INCREF (ret);

  return ret;
}


/*
 * Hash
 */
static Py_hash_t
PyDiaPaperinfo_Hash (PyObject *self)
{
  return (long) self;
}


/*
 * GetAttr
 */
static PyObject *
PyDiaPaperinfo_GetAttr (PyObject *obj, PyObject *arg)
{
  PyDiaPaperinfo *self;
  const char *attr;

  if (PyUnicode_Check (arg)) {
    attr = PyUnicode_AsUTF8 (arg);
  } else {
    goto generic;
  }

  self = (PyDiaPaperinfo *) obj;

  if (!g_strcmp0 (attr, "__members__")) {
    return Py_BuildValue ("[sssss]",
                          "name", "is_portrait", "scaling", "width", "height");
  } else if (!g_strcmp0 (attr, "name")) {
    return PyUnicode_FromString (self->paper->name);
  } else if (!g_strcmp0 (attr, "is_portrait")) {
    return PyLong_FromLong (self->paper->is_portrait);
  } else if (!g_strcmp0 (attr, "scaling")) {
    return PyFloat_FromDouble (self->paper->scaling);
  } else if (!g_strcmp0 (attr, "width")) {
    return PyFloat_FromDouble (self->paper->width);
  } else if (!g_strcmp0 (attr, "height")) {
    return PyFloat_FromDouble (self->paper->height);
  }

generic:
  return PyObject_GenericGetAttr (obj, arg);
}


/*
 * Repr / _Str
 */
static PyObject *
PyDiaPaperinfo_Str (PyObject *obj)
{
  PyDiaPaperinfo *self = (PyDiaPaperinfo *) obj;
  PyObject *py_s;
  char *s = g_strdup_printf ("%s - %fx%f %f%%",
                             self->paper->name ? self->paper->name : "(null)",
                             self->paper->width,
                             self->paper->height,
                             self->paper->scaling);

  py_s = PyUnicode_FromString (s);
  g_clear_pointer (&s, g_free);

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
  PyVarObject_HEAD_INIT (NULL, 0)
  .tp_name = "dia.Paperinfo",
  .tp_basicsize = sizeof (PyDiaPaperinfo),
  .tp_dealloc = PyDiaPaperinfo_Dealloc,
  .tp_getattro = PyDiaPaperinfo_GetAttr,
  .tp_richcompare = PyDiaPaperinfo_Compare,
  .tp_hash = PyDiaPaperinfo_Hash,
  .tp_str = PyDiaPaperinfo_Str,
  .tp_doc = "dia.Paperinfo is part of dia.DiagramData describing the paper",
  .tp_members = PyDiaPaperinfo_Members,
};
