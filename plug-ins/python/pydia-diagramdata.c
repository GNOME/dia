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

#include "config.h"

#include <glib/gi18n-lib.h>

#include "pydia-object.h"
#include "pydia-diagramdata.h"

#include "pydia-geometry.h"
#include "pydia-layer.h"
#include "pydia-color.h"
#include "pydia-paperinfo.h"
#include "pydia-error.h"

#include "app/diagram.h"
#include "dia-layer.h"
#include "pydia-diagram.h" /* support dynamic_cast */


PyObject *
PyDiaDiagramData_New (DiagramData *dd)
{
  PyDiaDiagramData *self;

  // Bit of a hack
  if (DIA_IS_DIAGRAM (dd)) {
    return PyDiaDiagram_New (DIA_DIAGRAM (dd));
  }

  self = PyObject_NEW (PyDiaDiagramData, &PyDiaDiagramData_Type);

  if (!self) return NULL;

  self->data = g_object_ref (dd);

  return (PyObject *) self;
}


static void
PyDiaDiagramData_Dealloc (PyObject *self)
{
  g_clear_object (&((PyDiaDiagramData *) self)->data);

  PyObject_DEL (self);
}


static PyObject *
PyDiaDiagramData_RichCompare (PyObject *self,
                              PyObject *other,
                              int       op)
{
  Py_RETURN_RICHCOMPARE (((PyDiaDiagramData *) self)->data,
                         ((PyDiaDiagramData *) other)->data,
                         op);
}


static Py_hash_t
PyDiaDiagramData_Hash (PyObject *self)
{
  return (long) ((PyDiaDiagramData *) self)->data;
}


static PyObject *
PyDiaDiagramData_Str (PyObject *self)
{
  PyObject *py_s;
  char *s = g_strdup_printf ("<PyDiaDiagramData %p>", self);

  py_s = PyUnicode_FromString (s);

  g_clear_pointer (&s, g_free);

  return py_s;
}


/*
 * "real" member function implementaion ?
 */
static PyObject *
PyDiaDiagramData_UpdateExtents (PyDiaDiagramData *self, PyObject *args)
{
  if (!PyArg_ParseTuple (args, ":DiagramData.update_extents")) {
    return NULL;
  }

  data_update_extents(self->data);

  Py_RETURN_NONE;
}


static PyObject *
PyDiaDiagramData_GetSortedSelected (PyDiaDiagramData *self, PyObject *args)
{
  GList *list, *tmp;
  PyObject *ret;
  guint i, len;

  if (!PyArg_ParseTuple(args, ":DiagramData.get_sorted_selected")) {
    return NULL;
  }

  list = data_get_sorted_selected (self->data);

  len = g_list_length (list);
  ret = PyTuple_New (len);

  for (i = 0, tmp = list; tmp; i++, tmp = tmp->next) {
    PyTuple_SetItem (ret, i, PyDiaObject_New (DIA_OBJECT (tmp->data)));
  }

  g_list_free (list);

  return ret;
}


static PyObject *
PyDiaDiagramData_AddLayer (PyDiaDiagramData *self, PyObject *args)
{
  char *name;
  int pos = -1;
  DiaLayer *layer;

  if (!PyArg_ParseTuple(args, "s|i:DiagramData.add_layer", &name, &pos))
    return NULL;

  layer = dia_layer_new (name, self->data);
  if (pos != -1) {
    data_add_layer_at (self->data, layer, pos);
  } else {
    data_add_layer (self->data, layer);
  }
  // self->data now owns the layer
  g_object_unref (layer);

  return PyDiaLayer_New (layer);
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

  data_remove_layer (self->data, layer->layer);
  g_clear_object (&layer->layer);
  layer->layer = NULL;
  Py_INCREF (Py_None);
  return Py_None;
}


/**
 * PyDiaDiagramData_CallbackObject:
 * @dia: The #DiagramData that emitted the signal
 * @layer: The Layer that the object is removed or added to.
 * @obj: The DiaObject that the signal is about.
 * @user_data: The python function to be called by the callback.
 *
 * Callback for "object_add" and "object_remove "signal, used by the
 * connect_after method, it's a proxy for the python function, creating the
 * values it needs. Params are those of the signals on the Diagram object.
 */
static void
PyDiaDiagramData_CallbackObject (DiagramData *dia,
                                 DiaLayer    *layer,
                                 DiaObject   *obj,
                                 void        *user_data)
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
  if (dia) {
    pydata = PyDiaDiagramData_New (dia);
  } else {
    pydata = Py_None;
    Py_INCREF (pydata);
  }

  /*
    * Create PyDiaLayer
    */
  if (layer) {
    pylayer = PyDiaLayer_New (layer);
  } else {
    pylayer = Py_None;
    Py_INCREF (pylayer);
  }

  /*
    * Create PyDiaObject
    */
  if (layer) {
    pyobj = PyDiaObject_New (obj);
  } else {
    pyobj = Py_None;
    Py_INCREF (pyobj);
  }


  Py_INCREF (func);

  /* Call the callback. */
  arg = Py_BuildValue ("(OOO)", pydata, pylayer, pyobj);
  if (arg) {
    res = PyObject_CallObject (func, arg);
    ON_RES (res, FALSE);
  }

  /* Cleanup */
  Py_XDECREF (arg);
  Py_DECREF (func);
  Py_XDECREF (pydata);
  Py_XDECREF (pylayer);
  Py_XDECREF (pyobj);
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


static PyObject *
PyDiaDiagramData_GetExtents (PyDiaDiagramData *self, void *closure)
{
  return PyDiaRectangle_New (&self->data->extents);
}


static PyObject *
PyDiaDiagramData_GetBgColor (PyDiaDiagramData *self, void *closure)
{
  return PyDiaColor_New (&(self->data->bg_color));
}


static PyObject *
PyDiaDiagramData_GetPaper (PyDiaDiagramData *self, void *closure)
{
  return PyDiaPaperinfo_New (&self->data->paper);
}


static PyObject *
PyDiaDiagramData_GetLayers (PyDiaDiagramData *self, void *closure)
{
  PyObject *ret = PyTuple_New (data_layer_count (self->data));

  DIA_FOR_LAYER_IN_DIAGRAM (self->data, layer, i, {
    PyTuple_SetItem (ret, i, PyDiaLayer_New (layer));
  });

  return ret;
}


static PyObject *
PyDiaDiagramData_GetActiveLayer (PyDiaDiagramData *self, void *closure)
{
  return PyDiaLayer_New (dia_diagram_data_get_active_layer (self->data));
}


static PyObject *
PyDiaDiagramData_GetGridWidth (PyDiaDiagramData *self, void *closure)
{
  if (DIA_IS_DIAGRAM (self->data)) {
    return Py_BuildValue ("(dd)",
                          DIA_DIAGRAM (self->data)->grid.width_x,
                          DIA_DIAGRAM (self->data)->grid.width_y);
  }

  Py_RETURN_NONE;
}


static PyObject *
PyDiaDiagramData_GetGridVisible (PyDiaDiagramData *self, void *closure)
{
  return Py_BuildValue ("(ii)",
                        DIA_DIAGRAM (self->data)->grid.visible_x,
                        DIA_DIAGRAM (self->data)->grid.visible_y);
}


static PyObject *
PyDiaDiagramData_GetSelected (PyDiaDiagramData *self, void *closure)
{
  PyObject *ret;
  GList *tmp;
  int i;

  ret = PyTuple_New (g_list_length (self->data->selected));
  for (i = 0, tmp = self->data->selected; tmp; i++, tmp = tmp->next) {
    PyTuple_SetItem (ret, i, PyDiaObject_New ((DiaObject *) tmp->data));
  }

  return ret;
}


static PyObject *
PyDiaDiagramData_GetDiagram (PyDiaDiagramData *self, void *closure)
{
  g_warning ("Use of <PyDiaDiagramData>.diagram. PyDiaDiagram is PyDiaDiagramData, use directly");

  if (DIA_IS_DIAGRAM (self->data)) {
    Py_INCREF (self);
    return (PyObject *) self;
  }

  Py_RETURN_NONE;
}


static PyGetSetDef PyDiaDiagramData_GetSetters[] = {
  { "extents", (getter) PyDiaDiagramData_GetExtents, NULL,
    "Rectangle covering all object's bounding boxes.", NULL },
  { "bg_color", (getter) PyDiaDiagramData_GetBgColor, NULL,
    "Color of the diagram's background.", NULL },
  { "paper", (getter) PyDiaDiagramData_GetPaper, NULL,
    "Paperinfo of the diagram.", NULL },
  { "layers", (getter) PyDiaDiagramData_GetLayers, NULL,
    "Read-only list of the diagrams layers.", NULL },
  { "active_layer", (getter) PyDiaDiagramData_GetActiveLayer, NULL,
    "Layer currently active in the diagram.", NULL },
  { "grid_width", (getter) PyDiaDiagramData_GetGridWidth, NULL,
    "Tuple(real: x, real: y) : describing the grid size.", NULL },
  { "grid_visible", (getter) PyDiaDiagramData_GetGridVisible, NULL,
    "bool: visibility of the grid.", NULL },
  { "selected", (getter) PyDiaDiagramData_GetSelected, NULL,
    "List of Object: current selection.", NULL },
  { "diagram", (getter) PyDiaDiagramData_GetDiagram, NULL,
    "This data objects Diagram or None", NULL },
  { NULL }
};


PyTypeObject PyDiaDiagramData_Type = {
  PyVarObject_HEAD_INIT (NULL, 0)
  .tp_name = "dia.DiagramData",
  .tp_basicsize = sizeof (PyDiaDiagramData),
  .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
  .tp_dealloc = PyDiaDiagramData_Dealloc,
  .tp_richcompare = PyDiaDiagramData_RichCompare,
  .tp_hash = PyDiaDiagramData_Hash,
  .tp_str = PyDiaDiagramData_Str,
  .tp_doc = "The 'low level' diagram object. It contains everything to "
            "manipulate diagrams from im- and export filters as well as"
            " from the UI. It does not provide any access to GUI elements "
            "related to the diagram. Use the subclass dia.Diagram object"
            " for such matters.",
  .tp_methods = PyDiaDiagramData_Methods,
  .tp_getset = PyDiaDiagramData_GetSetters,
};
