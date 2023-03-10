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

#include "pydia-layer.h"
#include "pydia-object.h"
#include "pydia-cpoint.h"
#include "pydia-render.h"
#include "dia-layer.h"


PyObject *
PyDiaLayer_New (DiaLayer *layer)
{
  PyDiaLayer *self;

  self = PyObject_NEW (PyDiaLayer, &PyDiaLayer_Type);

  if (!self) {
    return NULL;
  }

  self->layer = g_object_ref (layer);

  return (PyObject *) self;
}


static void
PyDiaLayer_Dealloc (PyObject *self)
{
  g_clear_object (&((PyDiaLayer *) self)->layer);

  PyObject_DEL (self);
}


static PyObject *
PyDiaLayer_RichCompare (PyObject *self, PyObject *other, int op)
{
  Py_RETURN_RICHCOMPARE (((PyDiaLayer *) self)->layer,
                         ((PyDiaLayer *) other)->layer,
                         op);
}


static Py_hash_t
PyDiaLayer_Hash (PyObject *self)
{
  return (long) ((PyDiaLayer *) self)->layer;
}


static PyObject *
PyDiaLayer_Str (PyObject *self)
{
  return PyUnicode_FromString (dia_layer_get_name (((PyDiaLayer *) self)->layer));
}

/* methods here */

static PyObject *
PyDiaLayer_Destroy (PyDiaLayer *self, PyObject *args)
{
  if (!PyArg_ParseTuple (args, ":Layer.destroy"))
    return NULL;

  g_clear_object (&self->layer);
  /* we need some error checking elsewhere */

  Py_RETURN_NONE;
}


static PyObject *
PyDiaLayer_ObjectGetIndex (PyDiaLayer *self, PyObject *args)
{
  PyDiaObject *obj;

  if (!PyArg_ParseTuple (args, "O!:Layer.object_get_index",
                         &PyDiaObject_Type, &obj)) {
    return NULL;
  }

  return PyLong_FromLong (dia_layer_object_get_index (self->layer,
                                                      obj->object));
}


static PyObject *
PyDiaLayer_AddObject (PyDiaLayer *self, PyObject *args)
{
  PyDiaObject *obj;
  int pos = -1;

  if (!PyArg_ParseTuple (args, "O!|i:Layer.add_object",
                         &PyDiaObject_Type, &obj, &pos)) {
    return NULL;
  }

  if (pos != -1) {
    dia_layer_add_object_at (self->layer, obj->object, pos);
  } else {
    dia_layer_add_object (self->layer, obj->object);
  }

  Py_RETURN_NONE;
}


static PyObject *
PyDiaLayer_RemoveObject (PyDiaLayer *self, PyObject *args)
{
  PyDiaObject *obj;

  if (!PyArg_ParseTuple (args, "O!:Layer.remove_object",
                         &PyDiaObject_Type, &obj)) {
    return NULL;
  }

  dia_layer_remove_object (self->layer, obj->object);

  Py_RETURN_NONE;
}


static PyObject *
PyDiaLayer_FindObjectsInRectangle (PyDiaLayer *self, PyObject *args)
{
  DiaRectangle rect;
  GList *list, *tmp;
  PyObject *ret;

  if (!PyArg_ParseTuple (args, "dddd:Layer.find_objects_in_rectange",
                         &rect.top, &rect.left, &rect.bottom, &rect.right)) {
    return NULL;
  }

  list = dia_layer_find_objects_in_rectangle (self->layer, &rect);
  ret = PyList_New (0);
  for (tmp = list; tmp; tmp = tmp->next) {
    PyList_Append (ret, PyDiaObject_New ((DiaObject *) tmp->data));
  }
  g_list_free (list);

  return ret;
}


static PyObject *
PyDiaLayer_FindClosestObject (PyDiaLayer *self, PyObject *args)
{
  Point pos;
  double maxdist;
  DiaObject *obj;

  if (!PyArg_ParseTuple (args, "ddd:Layer.find_closest_object",
                         &pos.x, &pos.y, &maxdist)) {
    return NULL;
  }

  obj = dia_layer_find_closest_object (self->layer, &pos, maxdist);

  if (obj) {
    return PyDiaObject_New (obj);
  }

  Py_RETURN_NONE;
}


static PyObject *
PyDiaLayer_FindClosestConnectionPoint (PyDiaLayer *self, PyObject *args)
{
  ConnectionPoint *cpoint = NULL;
  Point pos;
  double dist;
  PyObject *ret;
  PyDiaObject *obj;

   if (!PyArg_ParseTuple (args, "dd|O!:Layer.find_closest_connection_point",
                          &pos.x, &pos.y, PyDiaObject_Type, &obj)) {
    return NULL;
  }

  dist = dia_layer_find_closest_connectionpoint (self->layer,
                                                 &cpoint,
                                                 &pos,
                                                 obj ? obj->object : NULL);

  ret = PyTuple_New (2);
  PyTuple_SetItem (ret, 0, PyFloat_FromDouble (dist));
  if (cpoint) {
    PyTuple_SetItem (ret, 1, PyDiaConnectionPoint_New (cpoint));
  } else {
    Py_INCREF (Py_None);
    PyTuple_SetItem (ret, 1, Py_None);
  }
  return ret;
}


static PyObject *
PyDiaLayer_UpdateExtents (PyDiaLayer *self, PyObject *args)
{
  if (!PyArg_ParseTuple (args, ":Layer.update_extents")) {
    return NULL;
  }

  return PyLong_FromLong (dia_layer_update_extents (self->layer));
}


static PyObject *
PyDiaLayer_Render (PyDiaLayer *self, PyObject *args)
{
  PyObject* renderer;
  DiaRenderer *wrapper;
  DiaRectangle *update = NULL;
  gboolean active = FALSE;
  /* could derive but not sure if it's worth the effort. */

  if (!PyArg_ParseTuple (args, "O:Layer.render", &renderer)) {
    return NULL;
  }

  /* We need to create the PythonRenderer wrapper to provide the gobject interface.
   * This could be done much more efficient if it would somehow be cached for the
   * whole rendering pass ...
   */
  wrapper = PyDia_new_renderer_wrapper (renderer);
  dia_layer_render (self->layer, wrapper, update,
                    NULL, /* no special object renderer */
                    NULL, /* no user data */
                    active);
  g_clear_object (&wrapper);

  Py_RETURN_NONE;
}

/* missing functions:
 *  layer_add_objects
 *  layer_add_objects_first
 *  layer_remove_objects
 *  layer_replace_object_with_list
 *  layer_set_object_list
 */

static PyMethodDef PyDiaLayer_Methods[] = {
  { "destroy", (PyCFunction) PyDiaLayer_Destroy, METH_VARARGS,
    "Release the layer. Must not be called when already added to a diagram."},
  { "object_get_index", (PyCFunction) PyDiaLayer_ObjectGetIndex, METH_VARARGS,
    "object_get_index(Object: o) -> int."
    "  Returns the index of the object in the layers list of objects."},
  { "add_object", (PyCFunction) PyDiaLayer_AddObject, METH_VARARGS,
    "add_object(Object: o[, int: position]) -> None."
    "  Add the object to the layer at the top or the given position counting from bottom."},
  { "remove_object", (PyCFunction) PyDiaLayer_RemoveObject, METH_VARARGS,
    "remove_object(Object: o) -> None"
    "  Remove the object from the layer and delete it."},
  { "find_objects_in_rectangle",
    (PyCFunction) PyDiaLayer_FindObjectsInRectangle, METH_VARARGS,
    "find_objects_in_rectangle(real: top, real left, real: bottom, real: right) -> Objects"
    "  Returns a list of objects found in the given rectangle."},
  { "find_closest_object", (PyCFunction) PyDiaLayer_FindClosestObject, METH_VARARGS,
    "find_closest_object(real: x, real: y, real: maxdist) -> Object."
    "  Find an object in the given maximum distance of the given point."},
  { "find_closest_connection_point",
    (PyCFunction) PyDiaLayer_FindClosestConnectionPoint, METH_VARARGS,
    "find_closest_connectionpoint(real: x, real: y[, Object: o]) -> (real: dist, ConnectionPoint: cp)."
    "  Given a point and an optional object to exclude return the distance and the closest connection point or None."},
  { "update_extents", (PyCFunction) PyDiaLayer_UpdateExtents, METH_VARARGS,
    "update_extents() -> None.  Force recaculation of the layer extents."},
  { "render", (PyCFunction) PyDiaLayer_Render, METH_VARARGS,
    "render(dia.Renderer: r) -> None."
    "  Render the layer with the given renderer" },
  { NULL, 0, 0, NULL }
};


static PyObject *
PyDiaLayer_GetExtents (PyDiaLayer *self, void *closure)
{
  DiaRectangle extents;

  dia_layer_get_extents (self->layer, &extents);

  return Py_BuildValue ("(dddd)",
                        extents.top,
                        extents.left,
                        extents.bottom,
                        extents.right);
}


static PyObject *
PyDiaLayer_GetName (PyDiaLayer *self, void *closure)
{
  return PyUnicode_FromString (dia_layer_get_name (self->layer));
}


static PyObject *
PyDiaLayer_GetObjects (PyDiaLayer *self, void *closure)
{
  PyObject *ret;
  GList *tmp;
  int i;

  ret = PyTuple_New (g_list_length (dia_layer_get_object_list (self->layer)));

  for (i = 0, tmp = dia_layer_get_object_list (self->layer);
       tmp;
       i++, tmp = tmp->next) {
    PyTuple_SetItem (ret, i, PyDiaObject_New ((DiaObject *)tmp->data));
  }

  return ret;
}


static PyObject *
PyDiaLayer_GetVisible (PyDiaLayer *self, void *closure)
{
  return PyBool_FromLong (dia_layer_is_visible (self->layer));
}


static PyGetSetDef PyDiaLayer_GetSetters[] = {
  { "extents", (getter) PyDiaLayer_GetExtents, NULL,
    "Rectangle covering all object's bounding boxes.", NULL },
  { "name", (getter) PyDiaLayer_GetName, NULL,
    "The name of the layer.", NULL },
  { "objects", (getter) PyDiaLayer_GetObjects, NULL,
    "The list of objects in the layer.", NULL },
  { "visible", (getter) PyDiaLayer_GetVisible, NULL,
    "The visibility of the layer.", NULL },
  { NULL }
};


PyTypeObject PyDiaLayer_Type = {
  PyVarObject_HEAD_INIT (NULL, 0)
  .tp_name = "dia.Layer",
  .tp_basicsize = sizeof (PyDiaLayer),
  .tp_flags = Py_TPFLAGS_DEFAULT,
  .tp_dealloc = PyDiaLayer_Dealloc,
  .tp_richcompare = PyDiaLayer_RichCompare,
  .tp_hash = PyDiaLayer_Hash,
  .tp_str = PyDiaLayer_Str,
  .tp_doc = "A Layer is part of a Diagram and can contain objects.",
  .tp_methods = PyDiaLayer_Methods,
  .tp_getset = PyDiaLayer_GetSetters,
};
