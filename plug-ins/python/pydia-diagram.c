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

#include "pydia-diagram.h"
#include "pydia-diagramdata.h"
#include "pydia-display.h"
#include "pydia-layer.h"
#include "pydia-object.h"
#include "pydia-handle.h"
#include "pydia-cpoint.h"
#include "pydia-geometry.h"
#include "pydia-color.h"
#include "pydia-error.h"

#include "app/load_save.h"
#include "app/connectionpoint_ops.h"


#define PYDIA_DIAGRAM(self) DIA_DIAGRAM (((PyDiaDiagramData *) self)->data)

PyObject *
PyDiaDiagram_New (Diagram *dia)
{
  PyDiaDiagram *self;

  self = PyObject_NEW (PyDiaDiagram, &PyDiaDiagram_Type);

  if (!self) return NULL;

  ((PyDiaDiagramData *) self)->data = DIA_DIAGRAM_DATA (g_object_ref (dia));

  return (PyObject *) self;
}


static void
PyDiaDiagram_Dealloc (PyDiaDiagram *self)
{
  PyObject_DEL (self);
}


static PyObject *
PyDiaDiagram_Str (PyDiaDiagram *self)
{
  return PyUnicode_FromString (PYDIA_DIAGRAM (self)->filename);
}


static PyObject *
PyDiaDiagram_Select (PyDiaDiagram *self, PyObject *args)
{
  PyDiaObject *obj;

  if (!PyArg_ParseTuple (args, "O!:Diagram.select",
      &PyDiaObject_Type, &obj)) {
    return NULL;
  }
  diagram_select (PYDIA_DIAGRAM (self), obj->object);

  Py_RETURN_NONE;
}


static PyObject *
PyDiaDiagram_IsSelected (PyDiaDiagram *self, PyObject *args)
{
  PyDiaObject *obj;

  if (!PyArg_ParseTuple (args, "O!:Diagram.is_selected",
                         &PyDiaObject_Type, &obj)) {
    return NULL;
  }

  return PyBool_FromLong (diagram_is_selected (PYDIA_DIAGRAM (self),
                                               obj->object));
}


static PyObject *
PyDiaDiagram_Unselect (PyDiaDiagram *self, PyObject *args)
{
  PyDiaObject *obj;

  if (!PyArg_ParseTuple (args, "O!:Diagram.unselect",
                         &PyDiaObject_Type, &obj)) {
    return NULL;
  }

  diagram_unselect_object (PYDIA_DIAGRAM (self),
                           obj->object);

  Py_RETURN_NONE;
}


static PyObject *
PyDiaDiagram_RemoveAllSelected (PyDiaDiagram *self, PyObject *args)
{
  if (!PyArg_ParseTuple (args, ":Diagram.remove_all_selected")) {
    return NULL;
  }

  diagram_remove_all_selected (PYDIA_DIAGRAM (self), TRUE);

  Py_RETURN_NONE;
}


static PyObject *
PyDiaDiagram_UpdateExtents (PyDiaDiagram *self, PyObject *args)
{
  if (!PyArg_ParseTuple (args, ":Diagram.update_extents")) {
    return NULL;
  }

  diagram_update_extents (PYDIA_DIAGRAM (self));

  Py_RETURN_NONE;
}


static PyObject *
PyDiaDiagram_GetSortedSelected (PyDiaDiagram *self, PyObject *args)
{
  GList *list, *tmp;
  PyObject *ret;
  guint i, len;

  if (!PyArg_ParseTuple (args, ":Diagram.get_sorted_selected")) {
    return NULL;
  }

  list = diagram_get_sorted_selected (PYDIA_DIAGRAM (self));

  len = g_list_length (list);
  ret = PyTuple_New (len);

  for (i = 0, tmp = list; tmp; i++, tmp = tmp->next) {
    PyTuple_SetItem (ret, i, PyDiaObject_New (DIA_OBJECT (tmp->data)));
  }

  g_list_free (list);

  return ret;
}


static PyObject *
PyDiaDiagram_GetSortedSelectedRemove (PyDiaDiagram *self, PyObject *args)
{
  GList *list, *tmp;
  PyObject *ret;
  guint i, len;

  if (!PyArg_ParseTuple (args, ":Diagram.get_sorted_selected_remove")) {
    return NULL;
  }

  list = diagram_get_sorted_selected_remove (PYDIA_DIAGRAM (self));

  len = g_list_length (list);
  ret = PyTuple_New (len);

  for (i = 0, tmp = list; tmp; i++, tmp = tmp->next) {
    PyTuple_SetItem (ret, i, PyDiaObject_New (DIA_OBJECT (tmp->data)));
  }

  g_list_free (list);

  return ret;
}


static PyObject *
PyDiaDiagram_AddUpdate (PyDiaDiagram *self, PyObject *args)
{
  DiaRectangle r;

  if (!PyArg_ParseTuple (args, "dddd:Diagram.add_update", &r.top,
                         &r.left, &r.bottom, &r.right)) {
    return NULL;
  }

  diagram_add_update (PYDIA_DIAGRAM (self), &r);

  Py_RETURN_NONE;
}


static PyObject *
PyDiaDiagram_AddUpdateAll (PyDiaDiagram *self, PyObject *args)
{
  if (!PyArg_ParseTuple (args, ":Diagram.add_update_all")) {
    return NULL;
  }

  diagram_add_update_all (PYDIA_DIAGRAM (self));

  Py_RETURN_NONE;
}


static PyObject *
PyDiaDiagram_UpdateConnections (PyDiaDiagram *self, PyObject *args)
{
  PyDiaObject *obj;

  if (!PyArg_ParseTuple (args, "O!:Diagram.update_connections",
                         &PyDiaObject_Type, &obj)) {
    return NULL;
  }

  diagram_update_connections_object (PYDIA_DIAGRAM (self), obj->object, TRUE);

  Py_RETURN_NONE;
}


static PyObject *
PyDiaDiagram_Flush(PyDiaDiagram *self, PyObject *args)
{
  if (!PyArg_ParseTuple (args, ":Diagram.flush")) {
    return NULL;
  }

  diagram_flush (PYDIA_DIAGRAM (self));

  Py_RETURN_NONE;
}


static PyObject *
PyDiaDiagram_FindClickedObject (PyDiaDiagram *self, PyObject *args)
{
  Point p;
  double dist;
  DiaObject *obj;

  if (!PyArg_ParseTuple (args, "(dd)d:Diagram.find_clicked_object",
                         &p.x, &p.y, &dist)) {
    return NULL;
  }
  obj = diagram_find_clicked_object (PYDIA_DIAGRAM (self), &p, dist);
  if (obj) {
    return PyDiaObject_New (obj);
  }

  Py_RETURN_NONE;
}


static PyObject *
PyDiaDiagram_FindClosestHandle (PyDiaDiagram *self, PyObject *args)
{
  Point p;
  double dist;
  Handle *handle;
  DiaObject *obj;
  PyObject *ret;

  if (!PyArg_ParseTuple (args, "dd:Diagram.find_closest_handle",
                         &p.x, &p.y)) {
    return NULL;
  }

  dist = diagram_find_closest_handle (PYDIA_DIAGRAM (self), &handle, &obj, &p);
  ret = PyTuple_New (3);
  PyTuple_SetItem (ret, 0, PyFloat_FromDouble (dist));
  if (handle) {
    PyTuple_SetItem (ret, 1, PyDiaHandle_New (handle, obj));
  } else {
    Py_INCREF (Py_None);
    PyTuple_SetItem (ret, 1, Py_None);
  }

  if (obj) {
    PyTuple_SetItem (ret, 1, PyDiaObject_New (obj));
  } else {
    Py_INCREF(Py_None);
    PyTuple_SetItem (ret, 1, Py_None);
  }

  return ret;
}


static PyObject *
PyDiaDiagram_FindClosestConnectionPoint (PyDiaDiagram *self, PyObject *args)
{
  Point p;
  double dist;
  ConnectionPoint *cpoint;
  PyObject *ret;
  PyDiaObject *obj = NULL;

  if (!PyArg_ParseTuple (args, "dd|O!:Diagram.find_closest_connectionpoint",
                         &p.x, &p.y, PyDiaObject_Type, &obj)) {
    return NULL;
  }

  dist = diagram_find_closest_connectionpoint (PYDIA_DIAGRAM (self),
                                               &cpoint,
                                               &p,
                                               obj ? obj->object : NULL);

  ret = PyTuple_New (2);
  PyTuple_SetItem (ret, 0, PyFloat_FromDouble (dist));

  if (cpoint) {
    PyTuple_SetItem(ret, 1, PyDiaConnectionPoint_New (cpoint));
  } else {
    Py_INCREF (Py_None);
    PyTuple_SetItem (ret, 1, Py_None);
  }

  return ret;
}


static PyObject *
PyDiaDiagram_GroupSelected (PyDiaDiagram *self, PyObject *args)
{
  if (!PyArg_ParseTuple(args, ":Diagram.group_selected")) {
    return NULL;
  }

  diagram_group_selected (PYDIA_DIAGRAM (self));

  Py_RETURN_NONE;
}


static PyObject *
PyDiaDiagram_UngroupSelected (PyDiaDiagram *self, PyObject *args)
{
  if (!PyArg_ParseTuple (args, ":Diagram.ungroup_selected")) {
    return NULL;
  }

  diagram_ungroup_selected (PYDIA_DIAGRAM (self));

  Py_RETURN_NONE;
}


static PyObject *
PyDiaDiagram_Save (PyDiaDiagram *self, PyObject *args)
{
  DiaContext *ctx;
  char *filename = PYDIA_DIAGRAM (self)->filename;
  int ret;

  if (!PyArg_ParseTuple(args, "|s:Diagram.save", &filename)) {
    return NULL;
  }

  ctx = dia_context_new ("PyDia Save");
  dia_context_set_filename (ctx, filename);
  ret = diagram_save (PYDIA_DIAGRAM (self), filename, ctx);
  /* FIXME: throwing away possible error messages */
  dia_context_reset (ctx);
  dia_context_release (ctx);

  return PyLong_FromLong (ret);
}


static PyObject *
PyDiaDiagram_Display (PyDiaDiagram *self, PyObject *args)
{
  DDisplay *disp;

  if (!PyArg_ParseTuple (args, ":Diagram.display")) {
    return NULL;
  }

  disp = new_display (PYDIA_DIAGRAM (self));

  return PyDiaDisplay_New(disp);
}


/*
 *  Callback for "removed" signal, used by the connect_after method,
 *  it's a proxy for the python function, creating the values it needs.
 *  Params are those of the "removed" signal on the Diagram object.
 *  @param Diagram The Diagram that emitted the signal
 *  @param user_data The python function to be called by the callback.
 */
static void
PyDiaDiagram_CallbackRemoved (Diagram *dia, void *user_data)
{
  /* Check that we got a function */
  PyObject *diaobj, *res, *arg;
  PyObject *func = user_data;

  if (!func || !PyCallable_Check (func)) {
    g_warning ("Callback called without valid callback function.");
    return;
  }

  /*
   * Create a new PyDiaDiagram object. This really should reuse the object
   * that we connected to. We'll do that later.
   */
  if (dia) {
    diaobj = PyDiaDiagram_New (dia);
  } else {
    diaobj = Py_None;
    Py_INCREF (diaobj);
  }

  Py_INCREF (func);

  /* Call the callback. */
  arg = Py_BuildValue ("(O)", diaobj);
  if (arg) {
    res = PyObject_CallObject (func, arg);
    ON_RES (res, FALSE);
  }
  Py_XDECREF (arg);

  Py_DECREF (func);
  Py_XDECREF (diaobj);
}


/*
 *  Callback for "selection_changed" signal, used by the connect_after method,
 *  it's a proxy for the python function, creating the values it needs.
 *  Params are those of the "selection_changed" signal on the Diagram object.
 *  @param Diagram The Diagram that emitted the signal
 *  @param sel Number of selected objects??
 *  @param user_data The python function to be called by the callback.
 */
static void
PyDiaDiagram_CallbackSelectionChanged (Diagram *dia, int sel, void *user_data)
{
  /* Check that we got a function */
  PyObject *dgm, *res, *arg;
  PyObject *func = user_data;

  if (!func || !PyCallable_Check (func)) {
    g_warning ("Callback called without valid callback function.");
    return;
  }

  /*
   * Create a new PyDiaDiagram object. This really should reuse the object
   * that we connected to. We'll do that later.
   */
  if (dia) {
    dgm = PyDiaDiagram_New (dia);
  } else {
    dgm = Py_None;
    Py_INCREF (dgm);
  }

  Py_INCREF (func);

  /* Call the callback. */
  arg = Py_BuildValue ("(Oi)", dgm, sel);
  if (arg) {
    res = PyObject_CallObject (func, arg);
    ON_RES (res, FALSE);
  }
  Py_XDECREF (arg);

  Py_DECREF (func);
  Py_XDECREF (dgm);
}


/** Connects a python function to a signal.
 *  @param self The PyDiaDiagram this is a method of.
 *  @param args A tuple containing the arguments, a str for signal name
 *  and a callable object (like a function)
 */
static PyObject *
PyDiaDiagram_ConnectAfter (PyDiaDiagram *self, PyObject *args)
{
  PyObject *func;
  char *signal;

  /* Check arguments */
  if (!PyArg_ParseTuple (args, "sO:connect_after", &signal, &func)) {
    return NULL;
  }

  /* Check that the arg is callable */
  if (!PyCallable_Check (func)) {
    PyErr_SetString (PyExc_TypeError, "Second parameter must be callable");
    return NULL;
  }

  /* check if the signals name is valid */
  if (g_strcmp0 ("removed", signal) == 0 ||
      g_strcmp0 ("selection_changed", signal) == 0) {

    Py_INCREF (func); /* stay alive, where to kill ?? */

    /* connect to signal after by signal name */
    if (g_strcmp0 ("removed", signal) == 0) {
      g_signal_connect_after (PYDIA_DIAGRAM (self),
                              "removed",
                              G_CALLBACK (PyDiaDiagram_CallbackRemoved),
                              func);
    }

    if (strcmp ("selection_changed", signal) == 0) {
      g_signal_connect_after (PYDIA_DIAGRAM (self),
                              "selection_changed",
                              G_CALLBACK (PyDiaDiagram_CallbackSelectionChanged),
                              func);
    }

    Py_RETURN_NONE;
  } else {
    PyErr_SetString (PyExc_TypeError, "Wrong signal name");
    return NULL;
  }
}

static PyMethodDef PyDiaDiagram_Methods[] = {
    {"select", (PyCFunction)PyDiaDiagram_Select, METH_VARARGS,
     "select(Object: o) -> None.  Add the given object to the selection."},
    {"is_selected", (PyCFunction)PyDiaDiagram_IsSelected, METH_VARARGS,
     "is_selected(Object: o) -> bool.  True if the given object is already selected."},
    {"unselect", (PyCFunction)PyDiaDiagram_Unselect, METH_VARARGS,
     "unselect(Object: o) -> None.  Remove the given object from the selection)"},
    {"remove_all_selected", (PyCFunction)PyDiaDiagram_RemoveAllSelected, METH_VARARGS,
     "remove_all_selected() -> None.  Delete all selected objects."},
    {"update_extents", (PyCFunction)PyDiaDiagram_UpdateExtents, METH_VARARGS,
     "update_extents() -> None.  Force recaculation of the diagram extents."},
    {"get_sorted_selected", (PyCFunction)PyDiaDiagram_GetSortedSelected, METH_VARARGS,
     "get_sorted_selected() -> list.  Return the current selection sorted by Z-Order."},
    {"get_sorted_selected_remove",
     (PyCFunction)PyDiaDiagram_GetSortedSelectedRemove, METH_VARARGS,
     "get_sorted_selected_remove() -> list."
     "  Return sorted selection and remove it from the diagram."},
    {"add_update", (PyCFunction)PyDiaDiagram_AddUpdate, METH_VARARGS,
     "add_update(real: top, real: left, real: bottom, real: right) -> None."
     "  Add the given rectangle to the update queue."},
    {"add_update_all", (PyCFunction)PyDiaDiagram_AddUpdateAll, METH_VARARGS,
     "add_update_all() -> None.  Add the diagram visible area to the update queue."},
    {"update_connections", (PyCFunction)PyDiaDiagram_UpdateConnections, METH_VARARGS,
     "update_connections(Object: o) -> None."
     "  Update all connections of the given object. Might move connected objects."},
    {"flush", (PyCFunction)PyDiaDiagram_Flush, METH_VARARGS,
     "flush() -> None.  If no display update is queued, queue update."},
    {"find_clicked_object", (PyCFunction)PyDiaDiagram_FindClickedObject, METH_VARARGS,
     "find_clicked_object(real[2]: point, real: distance) -> Object."
     "  Find an object in the given distance of the given point."},
    {"find_closest_handle", (PyCFunction)PyDiaDiagram_FindClosestHandle, METH_VARARGS,
     "find_closest_handle(real[2]: point) -> (real: distance, Handle: h, Object: o)."
     "  Find the closest handle from point and return a tuple of the search results. "
     " Handle and Object might be None."},
    {"find_closest_connectionpoint",
     (PyCFunction)PyDiaDiagram_FindClosestConnectionPoint, METH_VARARGS,
     "find_closest_connectionpoint(real: x, real: y[, Object: o]) -> (real: dist, ConnectionPoint: cp)."
     "  Given a point and an optional object to exclude, return the distance and closest connection point or None."},
    {"group_selected", (PyCFunction)PyDiaDiagram_GroupSelected, METH_VARARGS,
     "group_selected() -> None.  Turn the current selection into a group object."},
    {"ungroup_selected", (PyCFunction)PyDiaDiagram_UngroupSelected, METH_VARARGS,
     "ungroup_selected() -> None.  Split all groups in the current selection into single objects."},
    {"save", (PyCFunction)PyDiaDiagram_Save, METH_VARARGS,
     "save(string: filename) -> None.  Save the diagram under the given filename."},
    {"display", (PyCFunction)PyDiaDiagram_Display, METH_VARARGS,
     "display() -> Display.  Create a new display of the diagram."},
    {"connect_after", (PyCFunction)PyDiaDiagram_ConnectAfter, METH_VARARGS,
     "connect_after(string: signal_name, Callback: func) -> None.  Listen to diagram events in ['removed', 'selection_changed']."},
    {NULL, 0, 0, NULL}
};


static PyObject *
PyDiaDiagram_GetData (PyDiaDiagram *self, void *closure)
{
  Py_INCREF (self);

  g_warning ("Use of <PyDiaDiagram>.data. PyDiaDiagram is PyDiaDiagramData, use directly");

  return (PyObject *) self;
}


static PyObject *
PyDiaDiagram_GetDisplays (PyDiaDiagram *self, void *closure)
{
  PyObject *ret;
  GSList *tmp;
  int i;

  ret = PyTuple_New (g_slist_length (PYDIA_DIAGRAM (self)->displays));

  for (i = 0, tmp = PYDIA_DIAGRAM (self)->displays; tmp; i++, tmp = tmp->next) {
    PyTuple_SetItem (ret, i, PyDiaDisplay_New ((DDisplay *) tmp->data));
  }

  return ret;
}


static PyObject *
PyDiaDiagram_GetFilename (PyDiaDiagram *self, void *closure)
{
  return PyUnicode_FromString (PYDIA_DIAGRAM (self)->filename);
}


static PyObject *
PyDiaDiagram_GetModified (PyDiaDiagram *self, void *closure)
{
  return PyBool_FromLong (diagram_is_modified (PYDIA_DIAGRAM (self)));
}


static PyObject *
PyDiaDiagram_GetSelected (PyDiaDiagram *self, void *closure)
{
  guint i, len = g_list_length (DIA_DIAGRAM_DATA (PYDIA_DIAGRAM (self))->selected);
  PyObject *ret = PyTuple_New (len);
  GList *tmp;

  for (i = 0, tmp = DIA_DIAGRAM_DATA (PYDIA_DIAGRAM (self))->selected; tmp; i++, tmp = tmp->next) {
    PyTuple_SetItem (ret, i, PyDiaObject_New (DIA_OBJECT (tmp->data)));
  }

  return ret;
}


static PyObject *
PyDiaDiagram_GetUnsaved (PyDiaDiagram *self, void *closure)
{
  return PyBool_FromLong (PYDIA_DIAGRAM (self)->unsaved);
}


static PyGetSetDef PyDiaDiagram_GetSetters[] = {
  { "data", (getter) PyDiaDiagram_GetData, NULL,
    "Backward-compatible base-class access", NULL },
  { "displays", (getter) PyDiaDiagram_GetDisplays, NULL,
    "The list of current displays of this diagram.", NULL },
  { "filename", (getter) PyDiaDiagram_GetFilename, NULL,
    "Filename in utf-8 encoding.", NULL },
  { "modified", (getter) PyDiaDiagram_GetModified, NULL,
    "Modification state.", NULL },
  { "selected", (getter) PyDiaDiagram_GetSelected, NULL,
    "The current object selection.", NULL },
  { "unsaved", (getter) PyDiaDiagram_GetUnsaved, NULL,
    "True if the diagram was not saved yet.", NULL },
  { NULL }
};


PyTypeObject PyDiaDiagram_Type = {
  PyVarObject_HEAD_INIT (NULL, 0)
  .tp_name = "dia.Diagram",
  .tp_basicsize = sizeof (PyDiaDiagram),
  .tp_flags = Py_TPFLAGS_DEFAULT,
  .tp_dealloc = (destructor) PyDiaDiagram_Dealloc,
  .tp_str = (reprfunc) PyDiaDiagram_Str,
  .tp_doc = "Subclass of dia.DiagramData adding interfacing the GUI "
            "elements.",
  .tp_methods = PyDiaDiagram_Methods,
  .tp_getset = PyDiaDiagram_GetSetters,
};
