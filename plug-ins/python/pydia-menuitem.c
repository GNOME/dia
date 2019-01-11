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

#include <config.h>

#include "pydia-menuitem.h"
#include "pydia-object.h" /* for PyObject_HEAD_INIT */

#include <structmember.h> /* PyMemberDef */

/*
 * New
 */
PyObject* PyDiaMenuitem_New (const DiaMenuItem *menuitem)
{
  PyDiaMenuitem *self;
  
  self = PyObject_NEW(PyDiaMenuitem, &PyDiaMenuitem_Type);
  if (!self) return NULL;
  
  self->menuitem = menuitem;

  return (PyObject *)self;
}

/*
 * Dealloc
 */
static void
PyDiaMenuitem_Dealloc(PyDiaMenuitem *self)
{
  /* we dont own the object */
  PyObject_DEL(self);
}

/*
 * Compare
 */
static int
PyDiaMenuitem_Compare(PyDiaMenuitem *self,
                      PyDiaMenuitem *other)
{
  return (self->menuitem == other->menuitem);
}

/*
 * Hash
 */
static long
PyDiaMenuitem_Hash(PyObject *self)
{
  return (long)self;
}

/*
 * GetAttr
 */
static PyObject *
PyDiaMenuitem_GetAttr(PyDiaMenuitem *self, gchar *attr)
{
  if (!strcmp(attr, "__members__"))
    return Py_BuildValue("[ss]", "text", "active");
  else if (!strcmp(attr, "text"))
    return PyString_FromString(self->menuitem->text);
  else if (!strcmp(attr, "active"))
    return PyInt_FromLong(self->menuitem->active);

  PyErr_SetString(PyExc_AttributeError, attr);
  return NULL;
}


static PyObject *
PyDiaMenuitem_Call(PyDiaMenuitem *self, PyObject *args)
{
  const DiaMenuItem *mi;
  ObjectChange *oc;
  DiaObject *obj;
  Point clicked;

  if (!PyArg_ParseTuple(args, "O!(dd)|ii:Menuitem.callback",
                        &PyDiaObject_Type, &obj, &clicked.x, &clicked.y))
    return NULL;
    
  mi = self->menuitem;

  oc = mi->callback (obj, &clicked, mi->callback_data);
  /* Throw away the undo information */
  if (oc) {
    if (oc->free)
      oc->free(oc);
    g_free (oc);
  }

  Py_INCREF(Py_None);
  return Py_None;
}
/*
 * Repr / _Str
 */
static PyObject *
PyDiaMenuitem_Str(PyDiaMenuitem *self)
{
  PyObject* py_s;
  gchar* s = g_strdup_printf("%s - %s,%s,%s",
                             self->menuitem->text, 
			     self->menuitem->active & DIAMENU_ACTIVE ? "active" : "inactive",
			     self->menuitem->active & DIAMENU_TOGGLE ? "toggle" : "",
			     self->menuitem->active & DIAMENU_TOGGLE_ON ? "on" : "");
  py_s = PyString_FromString(s);
  g_free (s);
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
    PyObject_HEAD_INIT(NULL)
    0,
    "dia.Menuitem",
    sizeof(PyDiaMenuitem),
    0,
    (destructor)PyDiaMenuitem_Dealloc,
    (printfunc)0,
    (getattrfunc)PyDiaMenuitem_GetAttr,
    (setattrfunc)0,
    (cmpfunc)PyDiaMenuitem_Compare,
    (reprfunc)0,
    0,
    0,
    0,
    (hashfunc)PyDiaMenuitem_Hash,
    (ternaryfunc)0,
    (reprfunc)PyDiaMenuitem_Str,
    (getattrofunc)0,
    (setattrofunc)0,
    (PyBufferProcs *)0,
    0L, /* Flags */
    "dia.Menuitem is holding menu functions for dia.Object",
    (traverseproc)0,
    (inquiry)0,
    (richcmpfunc)0,
    0, /* tp_weakliszoffset */
    (getiterfunc)0,
    (iternextfunc)0,
    PyDiaMenuitem_Methods, /* tp_methods */
    PyDiaMenuitem_Members, /* tp_members */
    0
};
