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

#include "pydia-object.h"
#include "pydia-cpoint.h"
#include "pydia-handle.h"
#include "pydia-geometry.h"
#include "pydia-properties.h"
#include "pydia-render.h"
#include "pydia-menuitem.h"

#include <structmember.h> /* PyMemberDef */

PyObject *
PyDiaObject_New(DiaObject *object)
{
    PyDiaObject *self;

    self = PyObject_NEW(PyDiaObject, &PyDiaObject_Type);

    if (!self) return NULL;
    self->object = object;
    return (PyObject *)self;
}


static void
PyDiaObject_Dealloc (PyObject *self)
{
  PyObject_DEL (self);
}


static PyObject *
PyDiaObject_RichCompare (PyObject *self, PyObject *other, int op)
{
  Py_RETURN_RICHCOMPARE (((PyDiaObject *) self)->object,
                         ((PyDiaObject *) other)->object,
                         op);
}


static Py_hash_t
PyDiaObject_Hash (PyObject *self)
{
  return (long) ((PyDiaObject *) self)->object;
}


static PyObject *
PyDiaObject_Str (PyObject *self)
{
  char *strname = g_strdup_printf ("<DiaObject of type \"%s\" at %lx>",
                                  ((PyDiaObject *) self)->object->type->name,
                                  (long) ((PyDiaObject *) self)->object);
  PyObject *ret = PyUnicode_FromString (strname);

  g_clear_pointer (&strname, g_free);
  return ret;
}


static PyObject *
PyDiaObject_Destroy(PyDiaObject *self, PyObject *args)
{
  if (!PyArg_ParseTuple (args, ":Object.destroy")) {
    return NULL;
  }

  if (!self->object->ops->destroy) {
    PyErr_SetString(PyExc_RuntimeError,"object does not implement method");
    return NULL;
  }

  self->object->ops->destroy (self->object);
  g_clear_pointer (&self->object, g_free);
  Py_INCREF (Py_None);

  return Py_None;
}


static PyObject *
PyDiaObject_Draw (PyDiaObject *self, PyObject *args)
{
  PyObject* renderer;
  DiaRenderer *wrapper;

  if (!PyArg_ParseTuple (args, "O:Object.draw", &renderer)) {
    return NULL;
  }

  /* We need to create the PythonRenderer wrapper to provide the gobject interface.
    * This could be done much more efficient if it would somehow be cached for the
    * whole rendering pass ...
    */
  wrapper = PyDia_new_renderer_wrapper (renderer);

  if (!self->object->ops->draw) {
    PyErr_SetString (PyExc_RuntimeError, "object does not implement method");
    return NULL;
  }

  dia_object_draw (self->object, wrapper);

  g_clear_object (&wrapper);

  Py_INCREF (Py_None);

  return Py_None;
}


static PyObject *
PyDiaObject_DistanceFrom(PyDiaObject *self, PyObject *args)
{
    Point point;

    if (!PyArg_ParseTuple(args, "dd:Object.distance_from",
			  &point.x, &point.y))
	return NULL;

    if (!self->object->ops->distance_from) {
	PyErr_SetString(PyExc_RuntimeError,"object does not implement method");
	return NULL;
    }

    return PyFloat_FromDouble(self->object->ops->distance_from(self->object,
							       &point));
}

/* select */

static PyObject *
PyDiaObject_Copy(PyDiaObject *self, PyObject *args)
{
    DiaObject *cp;

    if (!PyArg_ParseTuple(args, ":Object.copy"))
	return NULL;

    if (!self->object->ops->copy) {
	PyErr_SetString(PyExc_RuntimeError,"object does not implement method");
	return NULL;
    }

    cp = self->object->ops->copy(self->object);
    if (cp)
	return PyDiaObject_New(cp);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
PyDiaObject_GetMenu(PyDiaObject *self, PyObject *args)
{
    PyObject *ret, *items;
    Point clicked = { 0, 0 };
    DiaMenu *m;
    int i;


    if (!PyArg_ParseTuple(args, ":Object.get_object_menu"))
	return NULL;

    if (!self->object->ops->get_object_menu ||
        !(m = self->object->ops->get_object_menu (self->object, &clicked))) {
        Py_INCREF(Py_None);
        return Py_None;
    }    ;

    ret = PyTuple_New (2);
    PyTuple_SetItem(ret, 0, PyUnicode_FromString (m->title ? m->title : ""));
    items = PyList_New(0);
    for (i = 0; i < m->num_items; ++i) {
        DiaMenuItem *mi = &m->items[i];
	if (mi->text && mi->callback)
	  PyList_Append(items, PyDiaMenuitem_New (mi));
    }
    PyTuple_SetItem(ret, 1, items);
    return ret;
}


static PyObject *
PyDiaObject_Move (PyDiaObject *self, PyObject *args)
{
  Point point;
  DiaObjectChange *change;

  if (!PyArg_ParseTuple (args, "dd:Object.move", &point.x, &point.y)) {
    return NULL;
  }

  if (!self->object->ops->move) {
    PyErr_SetString (PyExc_RuntimeError,
                     "object does not implement method");
    return NULL;
  }

  change = dia_object_move (self->object, &point);

  if (G_UNLIKELY (change)) {
    /* TODO: return the change? */
    dia_object_change_unref (change);
  }

  Py_INCREF (Py_None);

  return Py_None;
}


static PyObject *
PyDiaObject_MoveHandle (PyDiaObject *self, PyObject *args)
{
  PyDiaHandle *handle;
  Point point;
  HandleMoveReason reason = 0;
  ModifierKeys modifiers = 0;
  DiaObjectChange *change;

  if (!PyArg_ParseTuple (args, "O!(dd)|ii:Object.move_handle",
                         &PyDiaHandle_Type, &handle, &point.x, &point.y,
                         &reason, &modifiers)) {
    return NULL;
  }

  if (!self->object->ops->move_handle) {
    PyErr_SetString (PyExc_RuntimeError,
                     "object does not implement method");

    return NULL;
  }

  change = dia_object_move_handle (self->object,
                                   handle->handle,
                                   &point,
                                   NULL,
                                   reason,
                                   modifiers);

  if (G_UNLIKELY (change)) {
    /* TODO: return the change? */
    dia_object_change_unref (change);
  }

  Py_INCREF (Py_None);

  return Py_None;
}


static PyMethodDef PyDiaObject_Methods[] = {
    { "destroy", (PyCFunction)PyDiaObject_Destroy, METH_VARARGS,
      "destroy() -> None."
      "  Release the object. Must not be called when already added to a group or layer." },
    { "distance_from", (PyCFunction)PyDiaObject_DistanceFrom, METH_VARARGS,
      "distance_from(real: x, real: y) -> real."
      "  Calculate the object's distance from the given point." },
    { "copy", (PyCFunction)PyDiaObject_Copy, METH_VARARGS,
      "copy() -> Object.  Create a new object copy." },
    { "draw", (PyCFunction)PyDiaObject_Draw, METH_VARARGS,
      "draw(dia.Renderer: r) -> None."
      "  Draw the object with the given renderer" },
    { "get_object_menu", (PyCFunction)PyDiaObject_GetMenu, METH_VARARGS,
      "get_object_menu() -> Tuple.  Returns a named list of Menuitem(s)." },
    { "move", (PyCFunction)PyDiaObject_Move, METH_VARARGS,
      "move(real: x, real: y) -> None."
      "  Move the entire object. The given point is the new object.obj_pos." },
    { "move_handle", (PyCFunction)PyDiaObject_MoveHandle, METH_VARARGS,
      "move_handle(Handle: h, (real: x, real: y)[int: reason, int: modifiers]) -> None."
      "  Move the given handle of the object to the given position"},
    { NULL, 0, 0, NULL }
};

#define T_INVALID -1 /* can't allow direct access due to pyobject->data indirection */
static PyMemberDef PyDiaObject_Members[] = {
    { "bounding_box", T_INVALID, 0, RESTRICTED|READONLY,
      "Box covering all the object." },
    { "connections", T_INVALID, 0, RESTRICTED|READONLY,
      "Vector of connection points." },
    { "handles", T_INVALID, 0, RESTRICTED|READONLY,
      "Vector of handles." },
    { "parent", T_INVALID, 0, RESTRICTED|READONLY,
      "The parent object when parenting is in place, None otherwise." },
    { "properties", T_INVALID, 0, RESTRICTED|READONLY,
      "Dictionary of object properties." },
    { "type", T_INVALID, 0, RESTRICTED|READONLY,
      "The dia.ObjectType of the object" },
    { NULL }
};


static PyObject *
PyDiaObject_GetAttr (PyObject *obj, PyObject *arg)
{
  PyDiaObject *self;
  const char *attr;

  if (PyUnicode_Check (arg)) {
    attr = PyUnicode_AsUTF8 (arg);
  } else {
    goto generic;
  }

  self = (PyDiaObject *) obj;

  if (!g_strcmp0 (attr, "__members__")) {
    return Py_BuildValue ("[sssss]",
                          "bounding_box", "connections", "handles", "parent",
                          "properties", "type");
  } else if (!g_strcmp0 (attr, "type")) {
    return PyDiaObjectType_New (self->object->type);
  } else if (!g_strcmp0 (attr, "bounding_box")) {
    return PyDiaRectangle_New (&(self->object->bounding_box));
  } else if (!g_strcmp0 (attr, "handles")) {
    int i;
    PyObject *ret = PyTuple_New (self->object->num_handles);

    for (i = 0; i < self->object->num_handles; i++) {
      PyTuple_SetItem(ret, i, PyDiaHandle_New(self->object->handles[i], self->object));
    }

    return ret;
  } else if (!g_strcmp0 (attr, "connections")) {
    int i;
    PyObject *ret = PyTuple_New(self->object->num_connections);

    for (i = 0; i < self->object->num_connections; i++) {
      PyTuple_SetItem (ret, i,
                       PyDiaConnectionPoint_New (self->object->connections[i]));
    }
    return ret;
  } else if (!g_strcmp0 (attr, "properties")) {
    return PyDiaProperties_New(self->object);
  } else if (!g_strcmp0 (attr, "parent")) {
    if (!self->object->parent) {
      Py_INCREF (Py_None);
      return Py_None;
    } else {
      return PyDiaObject_New (self->object->parent);
    }
  }

generic:
  return PyObject_GenericGetAttr (obj, arg);
}


PyTypeObject PyDiaObject_Type = {
  PyVarObject_HEAD_INIT (NULL, 0)
  .tp_name = "dia.Object",
  .tp_basicsize = sizeof (PyDiaObject),
  .tp_dealloc = PyDiaObject_Dealloc,
  .tp_getattro = PyDiaObject_GetAttr,
  .tp_richcompare = PyDiaObject_RichCompare,
  .tp_repr = PyDiaObject_Str,
  .tp_hash = PyDiaObject_Hash,
  .tp_str = PyDiaObject_Str,
  .tp_doc = "The main building block of diagrams.",
  .tp_methods = PyDiaObject_Methods,
  .tp_members = PyDiaObject_Members,
};


PyObject *
PyDiaObjectType_New (DiaObjectType *otype)
{
  PyDiaObjectType *self;

  self = PyObject_NEW (PyDiaObjectType, &PyDiaObjectType_Type);

  if (!self) {
    return NULL;
  }

  self->otype = otype;

  return (PyObject *) self;
}


static void
PyDiaObjectType_Dealloc (PyObject *self)
{
  PyObject_DEL (self);
}


static PyObject *
PyDiaObjectType_RichCompare (PyObject *self, PyObject *other, int op)
{
  Py_RETURN_RICHCOMPARE (((PyDiaObjectType *) self)->otype,
                         ((PyDiaObjectType *) other)->otype,
                         op);
}


static Py_hash_t
PyDiaObjectType_Hash (PyObject *self)
{
  return (long) ((PyDiaObjectType *) self)->otype;
}


static PyObject *
PyDiaObjectType_Repr (PyObject *self)
{
  char *strname = g_strdup_printf ("<DiaObjectType \"%s\">",
                                   ((PyDiaObjectType *) self)->otype->name);
  PyObject *ret = PyUnicode_FromString (strname);

  g_clear_pointer (&strname, g_free);

  return ret;
}


static PyObject *
PyDiaObjectType_Str (PyObject *self)
{
  return PyUnicode_FromString (((PyDiaObjectType *) self)->otype->name);
}


static PyObject *
PyDiaObjectType_Create (PyDiaObjectType *self, PyObject *args)
{
  Point p;
  gint data = 0;
  gpointer user_data;
  DiaObject *ret;
  Handle *h1 = NULL, *h2 = NULL;
  PyObject *pyret;

  if (!PyArg_ParseTuple(args, "dd|i:ObjectType.create", &p.x,&p.y, &data)) {
    return NULL;
  }
  user_data = GINT_TO_POINTER (data);
  if (!self->otype->ops) {
    PyErr_SetString (PyExc_RuntimeError, "Type has no ops!?");
    return NULL;
  }
  ret = self->otype->ops->create (&p,
                                  user_data ? user_data : self->otype->default_user_data,
                                  &h1,
                                  &h2);

  if (!ret) {
    PyErr_SetString (PyExc_RuntimeError, "could not create new object");
    return NULL;
  }

  pyret = PyTuple_New (3);
  PyTuple_SetItem (pyret, 0, PyDiaObject_New (ret));

  if (h1) {
    PyTuple_SetItem (pyret, 1, PyDiaHandle_New (h1, ret));
  } else {
    Py_INCREF (Py_None);
    PyTuple_SetItem (pyret, 1, Py_None);
  }

  if (h2) {
    PyTuple_SetItem (pyret, 2, PyDiaHandle_New (h2, ret));
  } else {
    Py_INCREF (Py_None);
    PyTuple_SetItem (pyret, 2, Py_None);
  }
  return pyret;
}


static PyMethodDef PyDiaObjectType_Methods[] = {
    { "create", (PyCFunction)PyDiaObjectType_Create, METH_VARARGS,
      "create(real: x, real: y) -> (Object: o, Handle: h1, Handle: h2)"
      "  Create a new object of this type. Returns a tuple containing the new object and up to two handles." },
    { NULL, 0, 0, NULL }
};

static PyMemberDef PyDiaObjectType_Members[] = {
    { "name", T_INVALID, 0, RESTRICTED|READONLY,
      "string: the unique type indentifier of the object type." },
    { "version", T_INVALID, 0, RESTRICTED|READONLY,
      "int: version number" },
    { NULL }
};


static PyObject *
PyDiaObjectType_GetAttr (PyObject *obj, PyObject *arg)
{
  PyDiaObjectType *self;
  const char *attr;

  if (PyUnicode_Check (arg)) {
    attr = PyUnicode_AsUTF8 (arg);
  } else {
    goto generic;
  }

  self = (PyDiaObjectType *) obj;

  if (!g_strcmp0 (attr, "__members__")) {
    return Py_BuildValue ("[ss]", "name", "version");
  } else if (!g_strcmp0 (attr, "name")) {
    return PyUnicode_FromString (self->otype->name);
  } else if (!g_strcmp0 (attr, "version")) {
    return PyLong_FromLong (self->otype->version);
  }

generic:
  return PyObject_GenericGetAttr (obj, arg);
}


PyTypeObject PyDiaObjectType_Type = {
  PyVarObject_HEAD_INIT (NULL, 0)
  .tp_name = "dia.ObjectType",
  .tp_basicsize = sizeof(PyDiaObjectType),
  .tp_dealloc = PyDiaObjectType_Dealloc,
  .tp_getattro = PyDiaObjectType_GetAttr,
  .tp_richcompare = PyDiaObjectType_RichCompare,
  .tp_repr = PyDiaObjectType_Repr,
  .tp_hash = PyDiaObjectType_Hash,
  .tp_str = PyDiaObjectType_Str,
  .tp_doc = "The dia.Object factory. Allows to create objects of the "
            "specific type. Use: factory = get_object_type(<type name>) to "
            "get a grip on it.",
  .tp_methods = PyDiaObjectType_Methods,
  .tp_members = PyDiaObjectType_Members,
};
