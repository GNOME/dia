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

#include <glib.h>

#include "pydia-diagram.h"
#include "pydia-layer.h"
#include "pydia-object.h"

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
    layer = new_layer(name);
    if (pos != -1)
	data_add_layer_at(self->dia->data, layer, pos);
    else
	data_at_layer(self->dia->data, layer);
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

/* missing:
 *  data_select
 *  data_unselect
 *  data_remove_all_selected
 *  data_update_extents
 *  data_get_sorted_selected
 *  data_get_sorted_selected_remove
 *
 *  diagram_unselect_object
 *  diagram_select
 *  diagram_add_update
 *  diagram_flush
 */

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
#if 0
	PyObject *ret;
	GSList *tmp;
	gint i;

	ret = PyTuple_New(g_slist_length(self->dia->displays));
	for (i = 0, tmp = self->dia->displays; tmp; i++, tmp = tmp->next)
	    PyTuple_SetItem(ret, i, PyDiaDisplay_New((Object *)tmp->data));
	return ret;
#endif
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
    (getattrfunc)0,
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
