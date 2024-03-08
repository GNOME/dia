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

#include "config.h"

#include <glib/gi18n-lib.h>

#include "pydia-geometry.h"
#include "pydia-cpoint.h"
#include "pydia-object.h"

#include <structmember.h> /* PyMemberDef */


PyObject *
PyDiaConnectionPoint_New (ConnectionPoint *cpoint)
{
  PyDiaConnectionPoint *self;

  self = PyObject_NEW (PyDiaConnectionPoint, &PyDiaConnectionPoint_Type);

  if (!self) {
    return NULL;
  }

  self->cpoint = cpoint;

  return (PyObject *) self;
}


static void
PyDiaConnectionPoint_Dealloc (PyObject *self)
{
  PyObject_DEL (self);
}


static PyObject *
PyDiaConnectionPoint_RichCompare (PyObject *self,
                                  PyObject *other,
                                  int       op)
{
  Py_RETURN_RICHCOMPARE (((PyDiaConnectionPoint *) self)->cpoint,
                         ((PyDiaConnectionPoint *) other)->cpoint,
                         op);
}


static Py_hash_t
PyDiaConnectionPoint_Hash (PyObject *self)
{
  return (long) ((PyDiaConnectionPoint *) self)->cpoint;
}


static PyObject *
PyDiaConnectionPoint_GetAttr (PyObject *obj, PyObject *arg)
{
  PyDiaConnectionPoint *self = (PyDiaConnectionPoint *) obj;

  const char *attr;

  if (PyUnicode_Check (arg)) {
    attr = PyUnicode_AsUTF8 (arg);
  } else {
    goto generic;
  }

  if (!g_strcmp0 (attr, "__members__")) {
    return Py_BuildValue("[sssss]", "connected", "object", "pos", "flags", "directions");
  } else if (!g_strcmp0 (attr, "pos")) {
    return PyDiaPoint_New (&(self->cpoint->pos));
  } else if (!g_strcmp0 (attr, "object")) {
    return PyDiaObject_New (self->cpoint->object);
  } else if (!g_strcmp0 (attr, "flags")) {
    return PyLong_FromLong (self->cpoint->flags);
  } else if (!g_strcmp0 (attr, "directions")) {
    return PyLong_FromLong (self->cpoint->directions);
  } else if (!g_strcmp0 (attr, "connected")) {
    PyObject *ret;
    GList *tmp;
    int i;

    ret = PyTuple_New (g_list_length (self->cpoint->connected));
    for (i = 0, tmp = self->cpoint->connected; tmp; i++, tmp = tmp->next) {
      PyTuple_SetItem (ret, i, PyDiaObject_New (DIA_OBJECT (tmp->data)));
    }
    return ret;
  }

generic:
  return PyObject_GenericGetAttr (obj, arg);
}


#define T_INVALID -1 /* can't allow direct access due to pyobject->cpoint indirection */
static PyMemberDef PyDiaConnectionPoint_Members[] = {
    { "connected", T_INVALID, 0, RESTRICTED|READONLY,
      "List of Object: read-only list of connected objects" },
    { "object", T_INVALID, 0, RESTRICTED|READONLY,
      "Object: the object owning this connection point" },
    { "pos", T_INVALID, 0, RESTRICTED|READONLY,
      "Point: read-only position of the connection point" },
    { "flags", T_INVALID, 0, RESTRICTED|READONLY,
      "Flags, e.g. CP_FLAGS_MAIN (=0x3)" },
    { "directions", T_INVALID, 0, RESTRICTED|READONLY,
      "Preferred directions away from the object (e.g. DIR_NORTH=0x1)" },
    { NULL }
};


PyTypeObject PyDiaConnectionPoint_Type = {
  PyVarObject_HEAD_INIT (NULL, 0)
  .tp_name = "dia.ConnectionPoint",
  .tp_basicsize = sizeof (PyDiaConnectionPoint),
  .tp_dealloc = PyDiaConnectionPoint_Dealloc,
  .tp_getattro = PyDiaConnectionPoint_GetAttr,
  .tp_richcompare = PyDiaConnectionPoint_RichCompare,
  .tp_hash = PyDiaConnectionPoint_Hash,
  .tp_doc = "One of the major features of Dia are connectable objects. They "
            "work by this type accessible through dia.Object.connections[].",
  .tp_members = PyDiaConnectionPoint_Members,
};
