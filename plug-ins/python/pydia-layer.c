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

#include <config.h>

#include "pydia-layer.h"
#include "pydia-object.h"
#include "pydia-cpoint.h"
#include "pydia-render.h"

#include <structmember.h> /* PyMemberDef */

PyObject *
PyDiaLayer_New(Layer *layer)
{
    PyDiaLayer *self;

    self = PyObject_NEW(PyDiaLayer, &PyDiaLayer_Type);

    if (!self) return NULL;
    self->layer = layer;
    return (PyObject *)self;
}

static void
PyDiaLayer_Dealloc(PyDiaLayer *self)
{
     PyObject_DEL(self);
}

static int
PyDiaLayer_Compare(PyDiaLayer *self, PyDiaLayer *other)
{
    if (self->layer == other->layer) return 0;
    if (self->layer > other->layer) return -1;
    return 1;
}

static long
PyDiaLayer_Hash(PyDiaLayer *self)
{
    return (long)self->layer;
}

static PyObject *
PyDiaLayer_Str(PyDiaLayer *self)
{
    return PyString_FromString(self->layer->name);
}

/* methods here */

static PyObject *
PyDiaLayer_Destroy(PyDiaLayer *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ":Layer.destroy"))
	return NULL;
    layer_destroy(self->layer);
    self->layer = NULL; /* we need some error checking elsewhere */
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
PyDiaLayer_ObjectGetIndex(PyDiaLayer *self, PyObject *args)
{
    PyDiaObject *obj;

    if (!PyArg_ParseTuple(args, "O!:Layer.object_get_index",
			  &PyDiaObject_Type, &obj))
	return NULL;
    return PyInt_FromLong(layer_object_get_index(self->layer, obj->object));
}

static PyObject *
PyDiaLayer_AddObject(PyDiaLayer *self, PyObject *args)
{
    PyDiaObject *obj;
    int pos = -1;

    if (!PyArg_ParseTuple(args, "O!|i:Layer.add_object",
			  &PyDiaObject_Type, &obj,
			  &pos))
	return NULL;
    if (pos != -1)
	layer_add_object_at(self->layer, obj->object, pos);
    else
	layer_add_object(self->layer, obj->object);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
PyDiaLayer_RemoveObject(PyDiaLayer *self, PyObject *args)
{
    PyDiaObject *obj;

    if (!PyArg_ParseTuple(args, "O!:Layer.remove_object",
			  &PyDiaObject_Type, &obj))
	return NULL;
    layer_remove_object(self->layer, obj->object);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
PyDiaLayer_FindObjectsInRectangle(PyDiaLayer *self, PyObject *args)
{
    Rectangle rect;
    GList *list, *tmp;
    PyObject *ret;

    if (!PyArg_ParseTuple(args, "dddd:Layer.find_objects_in_rectange",
			  &rect.top, &rect.left, &rect.bottom, &rect.right))
	return NULL;
    list = layer_find_objects_in_rectangle(self->layer, &rect);
    ret = PyList_New(0);
    for (tmp = list; tmp; tmp = tmp->next)
	PyList_Append(ret, PyDiaObject_New((DiaObject *)tmp->data));
    g_list_free(list);
    return ret;
}

static PyObject *
PyDiaLayer_FindClosestObject(PyDiaLayer *self, PyObject *args)
{
    Point pos;
    real maxdist;
    DiaObject *obj;

    if (!PyArg_ParseTuple(args, "ddd:Layer.find_closest_object",
			  &pos.x, &pos.y, &maxdist))
	return NULL;
    obj = layer_find_closest_object(self->layer, &pos, maxdist);
    if (obj)
	return PyDiaObject_New(obj);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
PyDiaLayer_FindClosestConnectionPoint(PyDiaLayer *self, PyObject *args)
{
    ConnectionPoint *cpoint = NULL;
    Point pos;
    real dist;
    PyObject *ret;
    PyDiaObject *obj;

    if (!PyArg_ParseTuple(args, "dd|O!:Layer.find_closest_connection_point",
			  &pos.x, &pos.y, PyDiaObject_Type, &obj))
	return NULL;
    dist = layer_find_closest_connectionpoint(self->layer, &cpoint, &pos, obj ? obj->object : NULL);

    ret = PyTuple_New(2);
    PyTuple_SetItem(ret, 0, PyFloat_FromDouble(dist));
    if (cpoint)
	PyTuple_SetItem(ret, 1, PyDiaConnectionPoint_New(cpoint));
    else {
	Py_INCREF(Py_None);
	PyTuple_SetItem(ret, 1, Py_None);
    }
    return ret;
}

static PyObject *
PyDiaLayer_UpdateExtents(PyDiaLayer *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ":Layer.update_extents"))
	return NULL;
    return PyInt_FromLong(layer_update_extents(self->layer));
}

static PyObject *
PyDiaLayer_Render(PyDiaLayer *self, PyObject *args)
{
    PyObject* renderer;
    DiaRenderer *wrapper;
    Rectangle *update = NULL;
    gboolean active = FALSE; /* could derive from layer->parent_diagram->active_layer
			      * but not sure if it's worth the effort. */

    if (!PyArg_ParseTuple(args, "O:Layer.render", &renderer))
	return NULL;

    /* We need to create the PythonRenderer wrapper to provide the gobject interface.
     * This could be done much more efficient if it would somehow be cached for the
     * whole rendering pass ...
     */
    wrapper = PyDia_new_renderer_wrapper (renderer);
    layer_render (self->layer, wrapper, update,
		  NULL, /* no special object renderer */
		  NULL, /* no user data */
		  active);
    g_object_unref (wrapper);

    Py_INCREF(Py_None);
    return Py_None;
}

/* missing functions:
 *  layer_add_objects
 *  layer_add_objects_first
 *  layer_remove_objects
 *  layer_replace_object_with_list
 *  layer_set_object_list
 */

static PyMethodDef PyDiaLayer_Methods[] = {
    {"destroy", (PyCFunction)PyDiaLayer_Destroy, METH_VARARGS,
     "Release the layer. Must not be called when already added to a diagram."},
    {"object_get_index", (PyCFunction)PyDiaLayer_ObjectGetIndex, METH_VARARGS,
     "object_get_index(Object: o) -> int."
     "  Returns the index of the object in the layers list of objects."},
    {"add_object", (PyCFunction)PyDiaLayer_AddObject, METH_VARARGS,
     "add_object(Object: o[, int: position]) -> None."
     "  Add the object to the layer at the top or the given position counting from bottom."},
    {"remove_object", (PyCFunction)PyDiaLayer_RemoveObject, METH_VARARGS,
     "remove_object(Object: o) -> None"
     "  Remove the object from the layer and delete it."},
    {"find_objects_in_rectangle",
     (PyCFunction)PyDiaLayer_FindObjectsInRectangle, METH_VARARGS,
     "find_objects_in_rectangle(real: top, real left, real: bottom, real: right) -> Objects"
     "  Returns a list of objects found in the given rectangle."},
    {"find_closest_object", (PyCFunction)PyDiaLayer_FindClosestObject, METH_VARARGS,
     "find_closest_object(real: x, real: y, real: maxdist) -> Object."
     "  Find an object in the given maximum distance of the given point."},
    {"find_closest_connection_point",
     (PyCFunction)PyDiaLayer_FindClosestConnectionPoint, METH_VARARGS,
     "find_closest_connectionpoint(real: x, real: y[, Object: o]) -> (real: dist, ConnectionPoint: cp)."
     "  Given a point and an optional object to exclude return the distance and the closest connection point or None."},
    {"update_extents", (PyCFunction)PyDiaLayer_UpdateExtents, METH_VARARGS,
     "update_extents() -> None.  Force recaculation of the layer extents."},
    { "render", (PyCFunction)PyDiaLayer_Render, METH_VARARGS,
      "render(dia.Renderer: r) -> None."
      "  Render the layer with the given renderer" },
    {NULL, 0, 0, NULL}
};

#define T_INVALID -1 /* can't allow direct access due to pyobject->data indirection */
static PyMemberDef PyDiaLayer_Members[] = {
    { "extents", T_INVALID, 0, RESTRICTED|READONLY,
      "Rectangle covering all object's bounding boxes." },
    { "name", T_INVALID, 0, RESTRICTED|READONLY,
      "The name of the layer." },
    { "objects", T_INVALID, 0, RESTRICTED|READONLY,
      "The list of objects in the layer." },
    { "visible", T_INVALID, 0, RESTRICTED|READONLY,
      "The visibility of the layer." },
    { NULL }
};

static PyObject *
PyDiaLayer_GetAttr(PyDiaLayer *self, gchar *attr)
{
    if (!strcmp(attr, "__members__"))
	return Py_BuildValue("[ssss]", "extents", "name", "objects",
			     "visible");
    else if (!strcmp(attr, "name"))
	return PyString_FromString(self->layer->name);
    else if (!strcmp(attr, "extents"))
	return Py_BuildValue("(dddd)", self->layer->extents.top,
			     self->layer->extents.left,
			     self->layer->extents.bottom,
			     self->layer->extents.right);
    else if (!strcmp(attr, "objects")) {
	PyObject *ret;
	GList *tmp;
	gint i;

	ret = PyTuple_New(g_list_length(self->layer->objects));
	for (i = 0, tmp = self->layer->objects; tmp; i++, tmp = tmp->next)
	    PyTuple_SetItem(ret, i, PyDiaObject_New((DiaObject *)tmp->data));
	return ret;
    } else if (!strcmp(attr, "visible"))
	return PyInt_FromLong(self->layer->visible);

    return Py_FindMethod(PyDiaLayer_Methods, (PyObject *)self, attr);
}

PyTypeObject PyDiaLayer_Type = {
    PyObject_HEAD_INIT(NULL)
    0,
    "dia.Layer",
    sizeof(PyDiaLayer),
    0,
    (destructor)PyDiaLayer_Dealloc,
    (printfunc)0,
    (getattrfunc)PyDiaLayer_GetAttr,
    (setattrfunc)0,
    (cmpfunc)PyDiaLayer_Compare,
    (reprfunc)0,
    0,
    0,
    0,
    (hashfunc)PyDiaLayer_Hash,
    (ternaryfunc)0,
    (reprfunc)PyDiaLayer_Str,
    (getattrofunc)0,
    (setattrofunc)0,
    (PyBufferProcs *)0,
    0L, /* Flags */
    "A Layer is part of a Diagram and can contain objects.",
    (traverseproc)0,
    (inquiry)0,
    (richcmpfunc)0,
    0, /* tp_weakliszoffset */
    (getiterfunc)0,
    (iternextfunc)0,
    PyDiaLayer_Methods, /* tp_methods */
    PyDiaLayer_Members, /* tp_members */
    0
};
