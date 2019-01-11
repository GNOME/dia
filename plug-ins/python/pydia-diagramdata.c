/* Python plug-in for dia
 * Copyright (C) 1999  James Henstridge
 * Copyright (C) 2000  Hans Breuer
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
#include "pydia-diagramdata.h"

#include "pydia-geometry.h"
#include "pydia-layer.h"
#include "pydia-color.h"
#include "pydia-paperinfo.h"
#include "pydia-error.h"

#include <structmember.h> /* PyMemberDef */

#include "app/diagram.h"
#include "pydia-diagram.h" /* support dynamic_cast */

PyObject *
PyDiaDiagramData_New(DiagramData *dd)
{
    PyDiaDiagramData *self;

    self = PyObject_NEW(PyDiaDiagramData, &PyDiaDiagramData_Type);

    if (!self) return NULL;
    g_object_ref (dd);
    self->data = dd;
    return (PyObject *)self;
}

static void
PyDiaDiagramData_Dealloc(PyDiaDiagramData *self)
{
    g_object_unref (self->data);
    PyObject_DEL(self);
}

static int
PyDiaDiagramData_Compare(PyDiaDiagramData *self, PyDiaDiagramData *other)
{
    if (self->data == other->data) return 0;
    if (self->data > other->data) return -1;
    return 1;
}

static long
PyDiaDiagramData_Hash(PyDiaDiagramData *self)
{
    return (long)self->data;
}

static PyObject *
PyDiaDiagramData_Str(PyDiaDiagramData *self)
{
    PyObject* py_s;
    gchar* s = g_strdup_printf ("<PyDiaDiagramData %p>", self);
    py_s = PyString_FromString(s);
    g_free(s);
    return py_s;
}

/*
 * "real" member function implementaion ?
 */
static PyObject *
PyDiaDiagramData_UpdateExtents(PyDiaDiagramData *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ":DiagramData.update_extents"))
	return NULL;
    data_update_extents(self->data);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
PyDiaDiagramData_GetSortedSelected(PyDiaDiagramData *self, PyObject *args)
{
    GList *list, *tmp;
    PyObject *ret;
    guint i, len;

    if (!PyArg_ParseTuple(args, ":DiagramData.get_sorted_selected"))
	return NULL;
    list = data_get_sorted_selected(self->data);

    len = g_list_length (list);
    ret = PyTuple_New(len);

    for (i = 0, tmp = list; tmp; i++, tmp = tmp->next)
	PyTuple_SetItem(ret, i, PyDiaObject_New((DiaObject *)tmp->data));
    g_list_free(list);
    return ret;
}

static PyObject *
PyDiaDiagramData_AddLayer(PyDiaDiagramData *self, PyObject *args)
{
    gchar *name;
    int pos = -1;
    Layer *layer;

    if (!PyArg_ParseTuple(args, "s|i:DiagramData.add_layer", &name, &pos))
	return NULL;
    layer = new_layer(g_strdup(name),self->data);
    if (pos != -1)
	data_add_layer_at(self->data, layer, pos);
    else
	data_add_layer(self->data, layer);
    return PyDiaLayer_New(layer);
}

static PyObject *
PyDiaDiagramData_RaiseLayer(PyDiaDiagramData *self, PyObject *args)
{
    PyDiaLayer *layer;

    if (!PyArg_ParseTuple(args, "O!:DiagramData.raise_layer",
			  &PyDiaLayer_Type, &layer))
	return NULL;
    data_raise_layer(self->data, layer->layer);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
PyDiaDiagramData_LowerLayer(PyDiaDiagramData *self, PyObject *args)
{
    PyDiaLayer *layer;

    if (!PyArg_ParseTuple(args, "O!:DiagramData.lower_layer",
			  &PyDiaLayer_Type, &layer))
	return NULL;
    data_lower_layer(self->data, layer->layer);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
PyDiaDiagramData_SetActiveLayer(PyDiaDiagramData *self, PyObject *args)
{
    PyDiaLayer *layer;

    if (!PyArg_ParseTuple(args, "O!:DiagramData.set_active_layer",
			  &PyDiaLayer_Type, &layer))
	return NULL;
    data_set_active_layer(self->data, layer->layer);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
PyDiaDiagramData_DeleteLayer(PyDiaDiagramData *self, PyObject *args)
{
    PyDiaLayer *layer;

    if (!PyArg_ParseTuple(args, "O!:DiagramData.delete_layer",
			  &PyDiaLayer_Type, &layer))
	return NULL;
    data_remove_layer(self->data, layer->layer);
    layer_destroy(layer->layer);
    layer->layer = NULL;
    Py_INCREF(Py_None);
    return Py_None;
}

/*! 
 *  Callback for "object_add" and "object_remove "signal, used by the connect_after method,
 *  it's a proxy for the python function, creating the values it needs.
 *  Params are those of the signals on the Diagram object.
 *  @param dia The DiagramData that emitted the signal
 *  @param layer The Layer that the object is removed or added to.
 *  @param obj The DiaObject that the signal is about.
 *  @param user_data The python function to be called by the callback.
 */
static void
PyDiaDiagramData_CallbackObject(DiagramData *dia,Layer *layer,DiaObject *obj,void *user_data)
{
    PyObject *pydata,*pylayer,*pyobj,*res,*arg;
    PyObject *func = user_data;
    
    /* Check that we got a function */   
    if (!func || !PyCallable_Check (func)) {
        g_warning ("Callback called without valid callback function.");
        return;
    }
      
    /* Create a new PyDiaDiagramData object.
     */
    if (dia)
        pydata = PyDiaDiagramData_New (dia);
    else {
        pydata = Py_None;
        Py_INCREF (pydata);
    }
      
    /*
     * Create PyDiaLayer
     */
    if (layer)
        pylayer = PyDiaLayer_New (layer);
    else {
        pylayer = Py_None;
        Py_INCREF (pylayer);
    }   
    
    /*
     * Create PyDiaObject
     */
    if (layer)
        pyobj = PyDiaObject_New (obj);
    else {
        pyobj = Py_None;
        Py_INCREF (pyobj);
    }   
    
    
    Py_INCREF(func);

    /* Call the callback. */
    arg = Py_BuildValue ("(OOO)", pydata,pylayer,pyobj);
    if (arg) {
      res = PyEval_CallObject (func, arg);
      ON_RES(res, FALSE);
    }
    
    /* Cleanup */
    Py_XDECREF (arg);
    Py_DECREF(func);
    Py_XDECREF(pydata);
    Py_XDECREF(pylayer);
    Py_XDECREF(pyobj);
}


/** Connects a python function to a signal.
 *  @param self The PyDiaDiagramData this is a method of.
 *  @param args A tuple containing the arguments, a str for signal name
 *  and a callable object (like a function)
 */
static PyObject *
PyDiaDiagramData_ConnectAfter(PyDiaDiagramData *self, PyObject *args)
{
    PyObject *func;
    char *signal;

    /* Check arguments */
    if (!PyArg_ParseTuple(args, "sO:DiagramData.connect_after",&signal,&func))
        return NULL;

    /* Check that the arg is callable */
    if (!PyCallable_Check(func)) {
        PyErr_SetString(PyExc_TypeError, "Second parameter must be callable");
        return NULL;
    }

    /* check if the signals name is valid */
    if ( strcmp("object_remove",signal) == 0 || strcmp("object_add",signal) == 0) {

        Py_INCREF(func); /* stay alive, where to kill ?? */

        /* connect to signal */
        g_signal_connect_after(DIA_DIAGRAM_DATA(self->data),signal,G_CALLBACK(PyDiaDiagramData_CallbackObject), func);
        
        Py_INCREF(Py_None);
        return Py_None;
    }
    else {
            PyErr_SetString(PyExc_TypeError, "Wrong signal name");
            return NULL;
    }
}


static PyMethodDef PyDiaDiagramData_Methods[] = {
    {"update_extents", (PyCFunction)PyDiaDiagramData_UpdateExtents, METH_VARARGS,
     "update_extents() -> None.  Recalculation of the diagram extents."},
    {"get_sorted_selected", (PyCFunction)PyDiaDiagramData_GetSortedSelected, METH_VARARGS,
     "get_sorted_selected() -> list.  Return the current selection sorted by Z-Order."},
    {"add_layer", (PyCFunction)PyDiaDiagramData_AddLayer, METH_VARARGS,
     "add_layer(Layer: layer[, int: position]) -> Layer."
     "  Add a layer to the diagram at the top or the given position counting from bottom."},
    {"raise_layer", (PyCFunction)PyDiaDiagramData_RaiseLayer, METH_VARARGS,
     "raise_layer() -> None.  Move the layer towards the top."},
    {"lower_layer", (PyCFunction)PyDiaDiagramData_LowerLayer, METH_VARARGS,
     "lower_layer() -> None.  Move the layer towards the bottom."},
    {"set_active_layer", (PyCFunction)PyDiaDiagramData_SetActiveLayer, METH_VARARGS,
     "set_active_layer(Layer: layer) -> None.  Make the given layer the active one."},
    {"delete_layer", (PyCFunction)PyDiaDiagramData_DeleteLayer, METH_VARARGS,
     "delete_layer(Layer: layer) -> None.  Remove the given layer from the diagram."},
    {"connect_after", (PyCFunction)PyDiaDiagramData_ConnectAfter, METH_VARARGS,
     "connect_after(string: signal_name, Callback: func) -> None."
     "  Listen to diagram events in ['object_add', 'object_remove']."},
    {NULL, 0, 0, NULL}
};

#define T_INVALID -1 /* can't allow direct access due to pyobject->data indirection */
static PyMemberDef PyDiaDiagramData_Members[] = {
    { "extents", T_INVALID, 0, RESTRICTED|READONLY,
      "Rectangle covering all object's bounding boxes." },
    { "bg_color", T_INVALID, 0, RESTRICTED|READONLY,
      "Color of the diagram's background."},
    { "paper", T_INVALID, 0, RESTRICTED|READONLY,
      "Paperinfo of the diagram."},
    { "layers", T_INVALID, 0, RESTRICTED|READONLY,
      "Read-only list of the diagrams layers."},
    { "active_layer", T_INVALID, 0, RESTRICTED|READONLY,
      "Layer currently active in the diagram."},
    { "grid_width", T_INVALID, 0, RESTRICTED|READONLY,
      "Tuple(real: x, real: y) : describing the grid size."},
    { "grid_visible", T_INVALID, 0, RESTRICTED|READONLY,
      "bool: visibility of the grid."},
    { "hguides", T_INVALID, 0, RESTRICTED|READONLY,
      "List of real: horizontal guides."},
    { "vguides", T_INVALID, 0, RESTRICTED|READONLY,
      "List of real: vertical guides."},
    { "selected", T_INVALID, 0, RESTRICTED|READONLY,
      "List of Object: current selection."},
    { "diagram", T_INVALID, 0, RESTRICTED|READONLY,
      "This data objects Diagram or None"},
    { NULL }
};

static PyObject *
PyDiaDiagramData_GetAttr(PyDiaDiagramData *self, gchar *attr)
{
    DiagramData *data = self->data;

    if (!strcmp(attr, "__members__"))
	return Py_BuildValue("[ssssssssssss]",
                           "extents", "bg_color", "paper",
                           "layers", "active_layer", 
                           "grid_width", "grid_visible", 
                           "hguides", "vguides",
                           "layers", "active_layer",
                           "selected" );
    else if (!strcmp(attr, "extents"))
      return PyDiaRectangle_New(&data->extents, NULL);
    else if (!strcmp(attr, "bg_color")) {
      return PyDiaColor_New (&(data->bg_color));
    }
    else if (!strcmp(attr, "layers")) {
	guint i, len = data->layers->len;
	PyObject *ret = PyTuple_New(len);

	for (i = 0; i < len; i++)
	    PyTuple_SetItem(ret, i, PyDiaLayer_New(
			g_ptr_array_index(data->layers, i)));
	return ret;
    } else if (!strcmp(attr, "active_layer")) {
	return PyDiaLayer_New(data->active_layer);
    } else if (!strcmp(attr, "paper")) {
        return PyDiaPaperinfo_New (&data->paper);
    } else if (!strcmp(attr, "layers")) {
	guint i, len = data->layers->len;
	PyObject *ret = PyTuple_New(len);

	for (i = 0; i < len; i++)
	    PyTuple_SetItem(ret, i, PyDiaLayer_New(
			g_ptr_array_index(self->data->layers, i)));
	return ret;
    } else if (!strcmp(attr, "active_layer")) {
      return PyDiaLayer_New(data->active_layer);
    } else if (!strcmp(attr, "selected")) {
	PyObject *ret;
	GList *tmp;
	gint i;

	ret = PyTuple_New(g_list_length(self->data->selected));
	for (i = 0, tmp = data->selected; tmp; i++, tmp = tmp->next)
	    PyTuple_SetItem(ret, i, PyDiaObject_New((DiaObject *)tmp->data));
	return ret;
    } else if (!strcmp(attr, "diagram")) {
	if (DIA_IS_DIAGRAM (self->data))
	    return PyDiaDiagram_New (DIA_DIAGRAM (self->data));
	Py_INCREF(Py_None);
	return Py_None;
    } else {
      /* In the interactive case diagramdata is_a diagram */
      if (DIA_IS_DIAGRAM (self->data)) {
	Diagram *diagram = DIA_DIAGRAM(self->data);
	if (diagram) { /* paranoid and helping scan-build */
	  if (!strcmp(attr, "grid_width")) 
      	    return Py_BuildValue("(dd)", diagram->grid.width_x, diagram->grid.width_y);
	  else if (!strcmp(attr, "grid_visible"))
	    return Py_BuildValue("(ii)", diagram->grid.visible_x, diagram->grid.visible_y);
	  else if (!strcmp(attr, "hguides")) {
	    int len = diagram->guides.nhguides;
	    PyObject *ret = PyTuple_New(len);
	    int i;
	    for (i = 0; i < len; i++)
	      PyTuple_SetItem(ret, i, PyFloat_FromDouble(diagram->guides.hguides[i]));
	    return ret;
	  } else if (diagram && !strcmp(attr, "vguides")) {
	    int len = diagram->guides.nvguides;
	    PyObject *ret = PyTuple_New(len);
	    int i;
	    for (i = 0; i < len; i++)
	      PyTuple_SetItem(ret, i, PyFloat_FromDouble(diagram->guides.vguides[i]));
	    return ret;
	  }
	}
      }
    }

    return Py_FindMethod(PyDiaDiagramData_Methods, (PyObject *)self, attr);
}

PyTypeObject PyDiaDiagramData_Type = {
    PyObject_HEAD_INIT(NULL)
    0,
    "dia.DiagramData",
    sizeof(PyDiaDiagramData),
    0,
    (destructor)PyDiaDiagramData_Dealloc,
    (printfunc)0,
    (getattrfunc)PyDiaDiagramData_GetAttr,
    (setattrfunc)0,
    (cmpfunc)PyDiaDiagramData_Compare,
    (reprfunc)0,
    0,
    0,
    0,
    (hashfunc)PyDiaDiagramData_Hash,
    (ternaryfunc)0,
    (reprfunc)PyDiaDiagramData_Str,
    (getattrofunc)0,
    (setattrofunc)0,
    (PyBufferProcs *)0,
    0L, /* Flags */
    "The 'low level' diagram object. It contains everything to manipulate diagrams from im- and export "
    "filters as well as from the UI. It does not provide any access to GUI elements related to the diagram."
    "Use the subclass dia.Diagram object for such matters.",
    (traverseproc)0,
    (inquiry)0,
    (richcmpfunc)0,
    0, /* tp_weakliszoffset */
    (getiterfunc)0,
    (iternextfunc)0,
    PyDiaDiagramData_Methods, /* tp_methods */
    PyDiaDiagramData_Members, /* tp_members */
    0
};
