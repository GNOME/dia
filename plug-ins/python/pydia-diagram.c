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

#include <glib.h>

#include "pydia-diagram.h"
#include "pydia-display.h"
#include "pydia-layer.h"
#include "pydia-object.h"
#include "pydia-handle.h"
#include "pydia-cpoint.h"

#include "app/load_save.h"

PyObject *
PyDiaDiagram_New(Diagram *dia)
{
    PyDiaDiagram *self;

    self = PyObject_NEW(PyDiaDiagram, &PyDiaDiagram_Type);

    if (!self) return NULL;
    self->dia = dia;;
    return (PyObject *)self;
}

static void
PyDiaDiagram_Dealloc(PyDiaDiagram *self)
{
     PyMem_DEL(self);
}

static int
PyDiaDiagram_Compare(PyDiaDiagram *self, PyDiaDiagram *other)
{
    if (self->dia == other->dia) return 0;
    if (self->dia > other->dia) return -1;
    return 1;
}

static long
PyDiaDiagram_Hash(PyDiaDiagram *self)
{
    return (long)self->dia;
}

static PyObject *
PyDiaDiagram_Str(PyDiaDiagram *self)
{
    return PyString_FromString(self->dia->filename);
}

static PyObject *
PyDiaDiagram_RaiseLayer(PyDiaDiagram *self, PyObject *args)
{
    PyDiaLayer *layer;

    if (!PyArg_ParseTuple(args, "O!:DiaDiagram.raise_layer",
			  &PyDiaLayer_Type, &layer))
	return NULL;
    data_raise_layer(self->dia->data, layer->layer);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
PyDiaDiagram_LowerLayer(PyDiaDiagram *self, PyObject *args)
{
    PyDiaLayer *layer;

    if (!PyArg_ParseTuple(args, "O!:DiaDiagram.lower_layer",
			  &PyDiaLayer_Type, &layer))
	return NULL;
    data_lower_layer(self->dia->data, layer->layer);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
PyDiaDiagram_AddLayer(PyDiaDiagram *self, PyObject *args)
{
    gchar *name;
    int pos = -1;
    Layer *layer;

    if (!PyArg_ParseTuple(args, "s|i:DiaDiagram.add_layer", &name, &pos))
	return NULL;
    layer = new_layer(g_strdup(name),self->dia->data);
    if (pos != -1)
	data_add_layer_at(self->dia->data, layer, pos);
    else
	data_add_layer(self->dia->data, layer);
    return PyDiaLayer_New(layer);
}

static PyObject *
PyDiaDiagram_SetActiveLayer(PyDiaDiagram *self, PyObject *args)
{
    PyDiaLayer *layer;

    if (!PyArg_ParseTuple(args, "O!:DiaDiagram.set_active_layer",
			  &PyDiaLayer_Type, &layer))
	return NULL;
    data_set_active_layer(self->dia->data, layer->layer);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
PyDiaDiagram_DeleteLayer(PyDiaDiagram *self, PyObject *args)
{
    PyDiaLayer *layer;

    if (!PyArg_ParseTuple(args, "O!:DiaDiagram.delete_layer",
			  &PyDiaLayer_Type, &layer))
	return NULL;
    data_delete_layer(self->dia->data, layer->layer);
    layer_destroy(layer->layer);
    layer->layer = NULL;
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
PyDiaDiagram_Select(PyDiaDiagram *self, PyObject *args)
{
    PyDiaObject *obj;

    if (!PyArg_ParseTuple(args, "O!:DiaDiagram.select",
			  &PyDiaObject_Type, &obj))
	return NULL;
    diagram_select(self->dia, obj->object);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
PyDiaDiagram_IsSelected(PyDiaDiagram *self, PyObject *args)
{
    PyDiaObject *obj;

    if (!PyArg_ParseTuple(args, "O!:DiaDiagram.is_selected",
			  &PyDiaObject_Type, &obj))
	return NULL;
    return PyInt_FromLong(diagram_is_selected(self->dia, obj->object));
}

static PyObject *
PyDiaDiagram_Unselect(PyDiaDiagram *self, PyObject *args)
{
    PyDiaObject *obj;

    if (!PyArg_ParseTuple(args, "O!:DiaDiagram.unselect",
			  &PyDiaObject_Type, &obj))
	return NULL;
    diagram_unselect_object(self->dia, obj->object);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
PyDiaDiagram_RemoveAllSelected(PyDiaDiagram *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ":DiaDiagram.remove_all_selected"))
	return NULL;
    diagram_remove_all_selected(self->dia, TRUE);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
PyDiaDiagram_UpdateExtents(PyDiaDiagram *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ":DiaDiagram.update_extents"))
	return NULL;
    diagram_update_extents(self->dia);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
PyDiaDiagram_GetSortedSelected(PyDiaDiagram *self, PyObject *args)
{
    GList *list, *tmp;
    PyObject *ret;
    guint i, len;

    if (!PyArg_ParseTuple(args, ":DiaDiagram.get_sorted_selected"))
	return NULL;
    list = tmp = diagram_get_sorted_selected(self->dia);

    len = self->dia->data->selected_count;
    ret = PyTuple_New(len);

    for (i = 0, tmp = self->dia->data->selected; tmp; i++, tmp = tmp->next)
	PyTuple_SetItem(ret, i, PyDiaObject_New((Object *)tmp->data));
    g_list_free(list);
    return ret;
}

static PyObject *
PyDiaDiagram_GetSortedSelectedRemove(PyDiaDiagram *self, PyObject *args)
{
    GList *list, *tmp;
    PyObject *ret;
    guint i, len;

    if (!PyArg_ParseTuple(args, ":DiaDiagram.get_sorted_selected_remove"))
	return NULL;
    list = tmp = diagram_get_sorted_selected_remove(self->dia);

    len = self->dia->data->selected_count;
    ret = PyTuple_New(len);

    for (i = 0, tmp = self->dia->data->selected; tmp; i++, tmp = tmp->next)
	PyTuple_SetItem(ret, i, PyDiaObject_New((Object *)tmp->data));
    g_list_free(list);
    return ret;
}

static PyObject *
PyDiaDiagram_AddUpdate(PyDiaDiagram *self, PyObject *args)
{
    Rectangle r;

    if (!PyArg_ParseTuple(args, "dddd:DiaDiagram.add_update", &r.top,
			  &r.left, &r.bottom, &r.right))
	return NULL;
    diagram_add_update(self->dia, &r);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
PyDiaDiagram_AddUpdateAll(PyDiaDiagram *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ":DiaDiagram.add_update_all"))
	return NULL;
    diagram_add_update_all(self->dia);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
PyDiaDiagram_Flush(PyDiaDiagram *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ":DiaDiagram.flush"))
	return NULL;
    diagram_flush(self->dia);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
PyDiaDiagram_FindClickedObject(PyDiaDiagram *self, PyObject *args)
{
    Point p;
    double dist;
    Object *obj;

    if (!PyArg_ParseTuple(args, "(dd)d:DiaDiagram.find_clicked_object",
			  &p.x, &p.y, &dist))
	return NULL;
    obj = diagram_find_clicked_object(self->dia, &p, dist);
    if (obj)
	return PyDiaObject_New(obj);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
PyDiaDiagram_FindClosestHandle(PyDiaDiagram *self, PyObject *args)
{
    Point p;
    double dist;
    Handle *handle;
    Object *obj;
    PyObject *ret;

    if (!PyArg_ParseTuple(args, "dd:DiaDiagram.find_closest_handle",
			  &p.x, &p.y))
	return NULL;
    dist = diagram_find_closest_handle(self->dia, &handle, &obj, &p);
    ret = PyTuple_New(3);
    PyTuple_SetItem(ret, 0, PyFloat_FromDouble(dist));
    if (handle)
	PyTuple_SetItem(ret, 1, PyDiaHandle_New(handle, obj));
    else {
	Py_INCREF(Py_None);
	PyTuple_SetItem(ret, 1, Py_None);
    }
    if (obj)
	PyTuple_SetItem(ret, 1, PyDiaObject_New(obj));
    else {
	Py_INCREF(Py_None);
	PyTuple_SetItem(ret, 1, Py_None);
    }
    return ret;
}

static PyObject *
PyDiaDiagram_FindClosestConnectionPoint(PyDiaDiagram *self, PyObject *args)
{
    Point p;
    double dist;
    ConnectionPoint *cpoint;
    PyObject *ret;
    PyDiaObject *obj;

    if (!PyArg_ParseTuple(args, "ddO!:DiaDiagram.find_closest_connectionpoint",
			  &p.x, &p.y, PyDiaObject_Type, &obj))
	return NULL;
    dist = diagram_find_closest_connectionpoint(self->dia, &cpoint, &p, 
						obj->object);
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
PyDiaDiagram_GroupSelected(PyDiaDiagram *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ":DiaDiagram.group_selected"))
	return NULL;
    diagram_group_selected(self->dia);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
PyDiaDiagram_UngroupSelected(PyDiaDiagram *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ":DiaDiagram.ungroup_selected"))
	return NULL;
    diagram_ungroup_selected(self->dia);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
PyDiaDiagram_Save(PyDiaDiagram *self, PyObject *args)
{
    gchar *filename = self->dia->filename;

    if (!PyArg_ParseTuple(args, "|s:DiaDiagram.save", &filename))
	return NULL;
    return PyInt_FromLong(diagram_save(self->dia, filename));
}

static PyMethodDef PyDiaDiagram_Methods[] = {
    {"raise_layer", (PyCFunction)PyDiaDiagram_RaiseLayer, 1},
    {"lower_layer", (PyCFunction)PyDiaDiagram_LowerLayer, 1},
    {"add_layer", (PyCFunction)PyDiaDiagram_AddLayer, 1},
    {"set_active_layer", (PyCFunction)PyDiaDiagram_SetActiveLayer, 1},
    {"delete_layer", (PyCFunction)PyDiaDiagram_DeleteLayer, 1},
    {"select", (PyCFunction)PyDiaDiagram_Select, 1},
    {"is_selected", (PyCFunction)PyDiaDiagram_IsSelected, 1},
    {"unselect", (PyCFunction)PyDiaDiagram_Unselect, 1},
    {"remove_all_selected", (PyCFunction)PyDiaDiagram_RemoveAllSelected, 1},
    {"update_extents", (PyCFunction)PyDiaDiagram_UpdateExtents, 1},
    {"get_sorted_selected", (PyCFunction)PyDiaDiagram_GetSortedSelected, 1},
    {"get_sorted_selected_remove",
     (PyCFunction)PyDiaDiagram_GetSortedSelectedRemove, 1},
    {"add_update", (PyCFunction)PyDiaDiagram_AddUpdate, 1},
    {"add_update_all", (PyCFunction)PyDiaDiagram_AddUpdateAll, 1},
    {"flush", (PyCFunction)PyDiaDiagram_Flush, 1},
    {"find_clicked_object", (PyCFunction)PyDiaDiagram_FindClickedObject, 1},
    {"find_closest_handle", (PyCFunction)PyDiaDiagram_FindClosestHandle, 1},
    {"find_closest_connectionpoint",
     (PyCFunction)PyDiaDiagram_FindClosestConnectionPoint, 1},
    {"group_selected", (PyCFunction)PyDiaDiagram_GroupSelected, 1},
    {"ungroup_selected", (PyCFunction)PyDiaDiagram_UngroupSelected, 1},
    {"save", (PyCFunction)PyDiaDiagram_Save, 1},
    {NULL, 0, 0, NULL}
};

static PyObject *
PyDiaDiagram_GetAttr(PyDiaDiagram *self, gchar *attr)
{
    if (!strcmp(attr, "__members__"))
	return Py_BuildValue("[sssssssss]", "active_layer", "bg_color",
			     "displays", "extents", "filename", "layers",
			     "modified", "selected", "unsaved");
    else if (!strcmp(attr, "filename"))
	return PyString_FromString(self->dia->filename);
    else if (!strcmp(attr, "unsaved"))
	return PyInt_FromLong(self->dia->unsaved);
    else if (!strcmp(attr, "modified"))
	return PyInt_FromLong(self->dia->modified);
    else if (!strcmp(attr, "extents"))
	return Py_BuildValue("(dddd)", self->dia->data->extents.top,
			     self->dia->data->extents.left,
			     self->dia->data->extents.bottom,
			     self->dia->data->extents.right);
    else if (!strcmp(attr, "bg_color"))
	return Py_BuildValue("(ddd)", self->dia->data->bg_color.red,
			     self->dia->data->bg_color.green,
			     self->dia->data->bg_color.blue);
    else if (!strcmp(attr, "layers")) {
	guint i, len = self->dia->data->layers->len;
	PyObject *ret = PyTuple_New(len);

	for (i = 0; i < len; i++)
	    PyTuple_SetItem(ret, i, PyDiaLayer_New(
			g_ptr_array_index(self->dia->data->layers, i)));
	return ret;
    } else if (!strcmp(attr, "active_layer"))
	return PyDiaLayer_New(self->dia->data->active_layer);
    else if (!strcmp(attr, "selected")) {
	guint i, len = self->dia->data->selected_count;
	PyObject *ret = PyTuple_New(len);
	GList *tmp;

	for (i = 0, tmp = self->dia->data->selected; tmp; i++, tmp = tmp->next)
	    PyTuple_SetItem(ret, i, PyDiaObject_New((Object *)tmp->data));
	return ret;
    } else if (!strcmp(attr, "displays")) {
	PyObject *ret;
	GSList *tmp;
	gint i;

	ret = PyTuple_New(g_slist_length(self->dia->displays));
	for (i = 0, tmp = self->dia->displays; tmp; i++, tmp = tmp->next)
	    PyTuple_SetItem(ret, i, PyDiaDisplay_New((DDisplay *)tmp->data));
	return ret;
    }

    return Py_FindMethod(PyDiaDiagram_Methods, (PyObject *)self, attr);
}

PyTypeObject PyDiaDiagram_Type = {
    PyObject_HEAD_INIT(&PyType_Type)
    0,
    "DiaDiagram",
    sizeof(PyDiaDiagram),
    0,
    (destructor)PyDiaDiagram_Dealloc,
    (printfunc)0,
    (getattrfunc)PyDiaDiagram_GetAttr,
    (setattrfunc)0,
    (cmpfunc)PyDiaDiagram_Compare,
    (reprfunc)0,
    0,
    0,
    0,
    (hashfunc)PyDiaDiagram_Hash,
    (ternaryfunc)0,
    (reprfunc)PyDiaDiagram_Str,
    0L,0L,0L,0L,
    NULL
};
