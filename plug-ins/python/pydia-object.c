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
PyDiaObject_Dealloc(PyDiaObject *self)
{
     PyObject_DEL(self);
}

static int
PyDiaObject_Compare(PyDiaObject *self, PyDiaObject *other)
{
    if (self->object == other->object) return 0;
    if (self->object > other->object) return -1;
    return 1;
}

static long
PyDiaObject_Hash(PyDiaObject *self)
{
    return (long)self->object;
}

static PyObject *
PyDiaObject_Str(PyDiaObject *self)
{
    gchar *strname = g_strdup_printf("<DiaObject of type \"%s\" at %lx>",
				     self->object->type->name,
				     (long)self->object);
    PyObject *ret = PyString_FromString(strname);

    g_free(strname);
    return ret;
}

static PyObject *
PyDiaObject_Destroy(PyDiaObject *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ":Object.destroy"))
	return NULL;

    if (!self->object->ops->destroy) {
	PyErr_SetString(PyExc_RuntimeError,"object does not implement method");
	return NULL;
    }

    self->object->ops->destroy(self->object);
    g_free(self->object);
    self->object = NULL;
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
PyDiaObject_Draw(PyDiaObject *self, PyObject *args)
{
    PyObject* renderer;
    DiaRenderer *wrapper;

    if (!PyArg_ParseTuple(args, "O:Object.draw", &renderer))
	return NULL;

    /* We need to create the PythonRenderer wrapper to provide the gobject interface.
     * This could be done much more efficient if it would somehow be cached for the
     * whole rendering pass ...
     */
    wrapper = PyDia_new_renderer_wrapper (renderer);

    if (!self->object->ops->draw) {
	PyErr_SetString(PyExc_RuntimeError,"object does not implement method");
	return NULL;
    }

    self->object->ops->draw(self->object, wrapper);

    g_object_unref (wrapper);

    Py_INCREF(Py_None);
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
    PyTuple_SetItem(ret, 0, PyString_FromString (m->title ? m->title : ""));
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
PyDiaObject_Move(PyDiaObject *self, PyObject *args)
{
    Point point;
    ObjectChange *change;

    if (!PyArg_ParseTuple(args, "dd:Object.move", &point.x, &point.y))
	return NULL;

    if (!self->object->ops->move) {
	PyErr_SetString(PyExc_RuntimeError,"object does not implement method");
	return NULL;
    }

    change = self->object->ops->move(self->object, &point);
    if (G_UNLIKELY(change)) /* TODO: return the change? */
        change->free(change);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
PyDiaObject_MoveHandle(PyDiaObject *self, PyObject *args)
{
    PyDiaHandle *handle;
    Point point;
    HandleMoveReason reason = 0;
    ModifierKeys modifiers = 0;
    ObjectChange *change;

    if (!PyArg_ParseTuple(args, "O!(dd)|ii:Object.move_handle",
			  &PyDiaHandle_Type, &handle, &point.x, &point.y,
			  &reason, &modifiers))
	return NULL;

    if (!self->object->ops->move_handle) {
	PyErr_SetString(PyExc_RuntimeError,"object does not implement method");
	return NULL;
    }

    change = self->object->ops->move_handle(self->object, handle->handle, &point,
				            NULL, reason, modifiers);

    if (G_UNLIKELY(change)) /* TODO: return the change? */
        change->free(change);

    Py_INCREF(Py_None);
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
PyDiaObject_GetAttr(PyDiaObject *self, gchar *attr)
{
    if (!strcmp(attr, "__members__"))
	return Py_BuildValue("[sssss]", "bounding_box", "connections",
			     "handles", "parent", "properties", "type");
    else if (!strcmp(attr, "type"))
	return PyDiaObjectType_New(self->object->type);
    else if (!strcmp(attr, "bounding_box"))
	return PyDiaRectangle_New(&(self->object->bounding_box), NULL);
    else if (!strcmp(attr, "handles")) {
	int i;
	PyObject *ret = PyTuple_New(self->object->num_handles);

	for (i = 0; i < self->object->num_handles; i++)
	    PyTuple_SetItem(ret, i, PyDiaHandle_New(self->object->handles[i], self->object));
	return ret;
    } else if (!strcmp(attr, "connections")) {
	int i;
	PyObject *ret = PyTuple_New(self->object->num_connections);

	for (i = 0; i < self->object->num_connections; i++)
	    PyTuple_SetItem(ret, i, PyDiaConnectionPoint_New(
				self->object->connections[i]));
	return ret;
    } else if (!strcmp(attr, "properties")) {
	return PyDiaProperties_New(self->object);
    } else if (!strcmp(attr, "parent")) {
	if (!self->object->parent) {
	    Py_INCREF(Py_None);
	    return Py_None;
	} else {
	    return PyDiaObject_New(self->object->parent);
	}
    }

    return Py_FindMethod(PyDiaObject_Methods, (PyObject *)self, attr);
}

PyTypeObject PyDiaObject_Type = {
    PyObject_HEAD_INIT(&PyType_Type)
    0,
    "dia.Object",
    sizeof(PyDiaObject),
    0,
    (destructor)PyDiaObject_Dealloc,
    (printfunc)0,
    (getattrfunc)PyDiaObject_GetAttr,
    (setattrfunc)0,
    (cmpfunc)PyDiaObject_Compare,
    (reprfunc)PyDiaObject_Str,
    0,
    0,
    0,
    (hashfunc)PyDiaObject_Hash,
    (ternaryfunc)0,
    (reprfunc)PyDiaObject_Str,
    (getattrofunc)0,
    (setattrofunc)0,
    (PyBufferProcs *)0,
    0L, /* Flags */
    "The main building block of diagrams.",
    (traverseproc)0,
    (inquiry)0,
    (richcmpfunc)0,
    0, /* tp_weakliszoffset */
    (getiterfunc)0,
    (iternextfunc)0,
    PyDiaObject_Methods, /* tp_methods */
    PyDiaObject_Members, /* tp_members */
    0
};

PyObject *
PyDiaObjectType_New(DiaObjectType *otype)
{
    PyDiaObjectType *self;

    self = PyObject_NEW(PyDiaObjectType, &PyDiaObjectType_Type);

    if (!self) return NULL;
    self->otype = otype;
    return (PyObject *)self;
}

static void
PyDiaObjectType_Dealloc(PyDiaObjectType *self)
{
     PyObject_DEL(self);
}

static int
PyDiaObjectType_Compare(PyDiaObjectType *self, PyDiaObjectType *other)
{
    if (self->otype == other->otype) return 0;
    if (self->otype > other->otype) return -1;
    return 1;
}

static long
PyDiaObjectType_Hash(PyDiaObjectType *self)
{
    return (long)self->otype;
}

static PyObject *
PyDiaObjectType_Repr(PyDiaObjectType *self)
{
    gchar *strname = g_strdup_printf("<DiaObjectType \"%s\">",
				     self->otype->name);
    PyObject *ret = PyString_FromString(strname);

    g_free(strname);
    return ret;
}

static PyObject *
PyDiaObjectType_Str(PyDiaObjectType *self)
{
    return PyString_FromString(self->otype->name);
}

static PyObject *
PyDiaObjectType_Create(PyDiaObjectType *self, PyObject *args)
{
    Point p;
    gint data = 0;
    gpointer user_data;
    DiaObject *ret;
    Handle *h1 = NULL, *h2 = NULL;
    PyObject *pyret;

    if (!PyArg_ParseTuple(args, "dd|i:ObjectType.create", &p.x,&p.y, &data))
	return NULL;
    user_data = GINT_TO_POINTER(data);
    if (!self->otype->ops) {
	PyErr_SetString(PyExc_RuntimeError, "Type has no ops!?");
	return NULL;
    }
    ret = self->otype->ops->create(&p, 
		  user_data ? user_data : self->otype->default_user_data, &h1, &h2);
    if (!ret) {
	PyErr_SetString(PyExc_RuntimeError, "could not create new object");
	return NULL;
    }
    pyret = PyTuple_New(3);
    PyTuple_SetItem(pyret, 0, PyDiaObject_New(ret));
    if (h1)
	PyTuple_SetItem(pyret, 1, PyDiaHandle_New(h1, ret));
    else {
	Py_INCREF(Py_None);
	PyTuple_SetItem(pyret, 1, Py_None);
    }
    if (h2)
	PyTuple_SetItem(pyret, 2, PyDiaHandle_New(h2, ret));
    else {
	Py_INCREF(Py_None);
	PyTuple_SetItem(pyret, 2, Py_None);
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
PyDiaObjectType_GetAttr(PyDiaObjectType *self, gchar *attr)
{
    if (!strcmp(attr, "__members__"))
	return Py_BuildValue("[ss]", "name", "version");
    else if (!strcmp(attr, "name"))
	return PyString_FromString(self->otype->name);
    else if (!strcmp(attr, "version"))
	return PyInt_FromLong(self->otype->version);

    return Py_FindMethod(PyDiaObjectType_Methods, (PyObject *)self, attr);
}

PyTypeObject PyDiaObjectType_Type = {
    PyObject_HEAD_INIT(&PyType_Type)
    0,
    "dia.ObjectType",
    sizeof(PyDiaObjectType),
    0,
    (destructor)PyDiaObjectType_Dealloc,
    (printfunc)0,
    (getattrfunc)PyDiaObjectType_GetAttr,
    (setattrfunc)0,
    (cmpfunc)PyDiaObjectType_Compare,
    (reprfunc)PyDiaObjectType_Repr,
    0,
    0,
    0,
    (hashfunc)PyDiaObjectType_Hash,
    (ternaryfunc)0,
    (reprfunc)PyDiaObjectType_Str,
    (getattrofunc)0,
    (setattrofunc)0,
    (PyBufferProcs *)0,
    0L, /* Flags */
    "The dia.Object factory. Allows to create objects of the specific type. "
    "Use: factory = get_object_type(<type name>) to get a grip on it.",
    (traverseproc)0,
    (inquiry)0,
    (richcmpfunc)0,
    0, /* tp_weakliszoffset */
    (getiterfunc)0,
    (iternextfunc)0,
    PyDiaObjectType_Methods, /* tp_methods */
    PyDiaObjectType_Members, /* tp_members */
    0
};

