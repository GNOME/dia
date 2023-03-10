/* Python plug-in for dia
 * Copyright (C) 1999  James Henstridge
 * Copyright (C) 2011  Hans Breuer
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

#include "pydia-menuitem.h"
#include "pydia-object.h" /* for PyObject_HEAD_INIT */

#include <structmember.h> /* PyMemberDef */


/*
 * New
 */
PyObject *
PyDiaMenuitem_New (const DiaMenuItem *menuitem)
{
  PyDiaMenuitem *self;

  self = PyObject_NEW (PyDiaMenuitem, &PyDiaMenuitem_Type);

  if (!self) return NULL;

  self->menuitem = menuitem;

  return (PyObject *)self;
}


/*
 * Dealloc
 */
static void
PyDiaMenuitem_Dealloc (PyObject *self)
{
  /* we dont own the object */
  PyObject_DEL (self);
}


/*
 * Compare
 */
static PyObject *
PyDiaMenuitem_RichCompare (PyObject *self,
                           PyObject *other,
                           int       op)
{
  Py_RETURN_RICHCOMPARE (((PyDiaMenuitem *) self)->menuitem,
                         ((PyDiaMenuitem *) other)->menuitem,
                         op);
}


/*
 * Hash
 */
static Py_hash_t
PyDiaMenuitem_Hash (PyObject *self)
{
  return (long) self;
}


/*
 * GetAttr
 */
static PyObject *
PyDiaMenuitem_GetAttr (PyObject *obj, PyObject *arg)
{
  PyDiaMenuitem *self;
  const char *attr;

  if (PyUnicode_Check (arg)) {
    attr = PyUnicode_AsUTF8 (arg);
  } else {
    goto generic;
  }

  self = (PyDiaMenuitem *) obj;

  if (!g_strcmp0 (attr, "__members__")) {
    return Py_BuildValue ("[ss]", "text", "active");
  } else if (!g_strcmp0 (attr, "text")) {
    return PyUnicode_FromString (self->menuitem->text);
  } else if (!g_strcmp0 (attr, "active")) {
    return PyLong_FromLong (self->menuitem->active);
  }

generic:
  return PyObject_GenericGetAttr (obj, arg);
}


static PyObject *
PyDiaMenuitem_Call (PyDiaMenuitem *self, PyObject *args)
{
  const DiaMenuItem *mi;
  DiaObjectChange *oc;
  DiaObject *obj;
  Point clicked;

  if (!PyArg_ParseTuple (args, "O!(dd)|ii:Menuitem.callback",
                         &PyDiaObject_Type, &obj, &clicked.x, &clicked.y)) {
    return NULL;
  }

  mi = self->menuitem;

  oc = mi->callback (obj, &clicked, mi->callback_data);

  /* Throw away the undo information */
  g_clear_pointer (&oc, dia_object_change_unref);

  Py_INCREF (Py_None);

  return Py_None;
}


/*
 * Repr / _Str
 */
static PyObject *
PyDiaMenuitem_Str (PyObject *obj)
{
  PyDiaMenuitem *self = (PyDiaMenuitem *) obj;
  PyObject *py_s;
  char *s = g_strdup_printf ("%s - %s,%s,%s",
                             self->menuitem->text,
                             self->menuitem->active & DIAMENU_ACTIVE ? "active" : "inactive",
                             self->menuitem->active & DIAMENU_TOGGLE ? "toggle" : "",
                             self->menuitem->active & DIAMENU_TOGGLE_ON ? "on" : "");

  py_s = PyUnicode_FromString (s);
  g_clear_pointer (&s, g_free);

  return py_s;
}


static PyMethodDef PyDiaMenuitem_Methods[] = {
    { "call", (PyCFunction)PyDiaMenuitem_Call, METH_VARARGS,
      "call() -> None."
      "  Invoke the menuitem callback on object." },
    { NULL, 0, 0, NULL }
};

#define T_INVALID -1 /* can't allow direct access due to pyobject->menuitem indirection */
static PyMemberDef PyDiaMenuitem_Members[] = {
    { "text", T_INVALID, 0, RESTRICTED|READONLY,
      "string: what would be written in the menu" },
    { "active", T_INVALID, 0, RESTRICTED|READONLY,
      "boolean: if it is callable" },
    { NULL }
};


/*
 * Python objetcs
 */
PyTypeObject PyDiaMenuitem_Type = {
  PyVarObject_HEAD_INIT (NULL, 0)
  .tp_name = "dia.Menuitem",
  .tp_basicsize = sizeof (PyDiaMenuitem),
  .tp_dealloc = PyDiaMenuitem_Dealloc,
  .tp_getattro = PyDiaMenuitem_GetAttr,
  .tp_richcompare = PyDiaMenuitem_RichCompare,
  .tp_hash = PyDiaMenuitem_Hash,
  .tp_str = PyDiaMenuitem_Str,
  .tp_doc = "dia.Menuitem is holding menu functions for dia.Object",
  .tp_methods = PyDiaMenuitem_Methods,
  .tp_members = PyDiaMenuitem_Members,
};
