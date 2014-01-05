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

#include <structmember.h> /* PyMemberDef */

#include "app/load_save.h"
#include "app/connectionpoint_ops.h"

PyObject *
PyDiaDiagram_New(Diagram *dia)
{
    PyDiaDiagram *self;

    self = PyObject_NEW(PyDiaDiagram, &PyDiaDiagram_Type);

    if (!self) return NULL;
    g_object_ref(dia);
    self->dia = dia;
    return (PyObject *)self;
}

static void
PyDiaDiagram_Dealloc(PyDiaDiagram *self)
{
    g_object_unref(self->dia);
    PyObject_DEL(self);
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
PyDiaDiagram_Select(PyDiaDiagram *self, PyObject *args)
{
    PyDiaObject *obj;

    if (!PyArg_ParseTuple(args, "O!:Diagram.select",
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

    if (!PyArg_ParseTuple(args, "O!:Diagram.is_selected",
			  &PyDiaObject_Type, &obj))
	return NULL;
    return PyInt_FromLong(diagram_is_selected(self->dia, obj->object));
}

static PyObject *
PyDiaDiagram_Unselect(PyDiaDiagram *self, PyObject *args)
{
    PyDiaObject *obj;

    if (!PyArg_ParseTuple(args, "O!:Diagram.unselect",
			  &PyDiaObject_Type, &obj))
	return NULL;
    diagram_unselect_object(self->dia, obj->object);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
PyDiaDiagram_RemoveAllSelected(PyDiaDiagram *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ":Diagram.remove_all_selected"))
	return NULL;
    diagram_remove_all_selected(self->dia, TRUE);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
PyDiaDiagram_UpdateExtents(PyDiaDiagram *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ":Diagram.update_extents"))
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

    if (!PyArg_ParseTuple(args, ":Diagram.get_sorted_selected"))
	return NULL;
    list = diagram_get_sorted_selected(self->dia);

    len = g_list_length (list);
    ret = PyTuple_New(len);

    for (i = 0, tmp = list; tmp; i++, tmp = tmp->next)
	PyTuple_SetItem(ret, i, PyDiaObject_New((DiaObject *)tmp->data));
    g_list_free(list);
    return ret;
}

static PyObject *
PyDiaDiagram_GetSortedSelectedRemove(PyDiaDiagram *self, PyObject *args)
{
    GList *list, *tmp;
    PyObject *ret;
    guint i, len;

    if (!PyArg_ParseTuple(args, ":Diagram.get_sorted_selected_remove"))
	return NULL;
    list = diagram_get_sorted_selected_remove(self->dia);

    len = g_list_length (list);
    ret = PyTuple_New(len);

    for (i = 0, tmp = list; tmp; i++, tmp = tmp->next)
	PyTuple_SetItem(ret, i, PyDiaObject_New((DiaObject *)tmp->data));
    g_list_free(list);
    return ret;
}

static PyObject *
PyDiaDiagram_AddUpdate(PyDiaDiagram *self, PyObject *args)
{
    Rectangle r;

    if (!PyArg_ParseTuple(args, "dddd:Diagram.add_update", &r.top,
			  &r.left, &r.bottom, &r.right))
	return NULL;
    diagram_add_update(self->dia, &r);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
PyDiaDiagram_AddUpdateAll(PyDiaDiagram *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ":Diagram.add_update_all"))
	return NULL;
    diagram_add_update_all(self->dia);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
PyDiaDiagram_UpdateConnections(PyDiaDiagram *self, PyObject *args)
{
    PyDiaObject *obj;

    if (!PyArg_ParseTuple(args, "O!:Diagram.update_connections",
                          &PyDiaObject_Type, &obj))
	return NULL;
    diagram_update_connections_object(self->dia, obj->object, TRUE);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
PyDiaDiagram_Flush(PyDiaDiagram *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ":Diagram.flush"))
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
    DiaObject *obj;

    if (!PyArg_ParseTuple(args, "(dd)d:Diagram.find_clicked_object",
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
    DiaObject *obj;
    PyObject *ret;

    if (!PyArg_ParseTuple(args, "dd:Diagram.find_closest_handle",
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
    PyDiaObject *obj = NULL;

    if (!PyArg_ParseTuple(args, "dd|O!:Diagram.find_closest_connectionpoint",
			  &p.x, &p.y, PyDiaObject_Type, &obj))
	return NULL;
    dist = diagram_find_closest_connectionpoint(self->dia, &cpoint, &p, 
						obj ? obj->object : NULL);
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
    if (!PyArg_ParseTuple(args, ":Diagram.group_selected"))
	return NULL;
    diagram_group_selected(self->dia);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
PyDiaDiagram_UngroupSelected(PyDiaDiagram *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ":Diagram.ungroup_selected"))
	return NULL;
    diagram_ungroup_selected(self->dia);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
PyDiaDiagram_Save(PyDiaDiagram *self, PyObject *args)
{
    DiaContext *ctx;
    gchar *filename = self->dia->filename;
    int ret;

    if (!PyArg_ParseTuple(args, "|s:Diagram.save", &filename))
	return NULL;
    ctx = dia_context_new ("PyDia Save");
    dia_context_set_filename (ctx, filename);
    ret = diagram_save(self->dia, filename, ctx);
    /* FIXME: throwing away possible error messages */
    dia_context_reset (ctx);
    dia_context_release (ctx);
    return PyInt_FromLong(ret);
}

static PyObject *
PyDiaDiagram_Display(PyDiaDiagram *self, PyObject *args)
{
    DDisplay *disp;

    if (!PyArg_ParseTuple(args, ":Diagram.display"))
	return NULL;
    disp = new_display(self->dia);
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
PyDiaDiagram_CallbackRemoved(Diagram *dia,void *user_data)
{
    /* Check that we got a function */
    PyObject *diaobj,*res,*arg;
    PyObject *func = user_data;
    
    if (!func || !PyCallable_Check (func)) {
        g_warning ("Callback called without valid callback function.");
        return;
    }
      
    /* Create a new PyDiaDiagram object. This really should reuse the object that we connected to. 
     * We'll do that later.
     */
    if (dia)
        diaobj = PyDiaDiagram_New (dia);
    else {
        diaobj = Py_None;
        Py_INCREF (diaobj);
    }
      
    Py_INCREF(func);

    /* Call the callback. */
    arg = Py_BuildValue ("(O)", diaobj);
    if (arg) {
      res = PyEval_CallObject (func, arg);
      ON_RES(res, FALSE);
    }
    Py_XDECREF (arg);

    Py_DECREF(func);
    Py_XDECREF(diaobj);
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
PyDiaDiagram_CallbackSelectionChanged(Diagram *dia,int sel,void *user_data)
{
    /* Check that we got a function */
    PyObject *dgm,*res,*arg;
    PyObject *func = user_data;
    
    if (!func || !PyCallable_Check (func)) {
        g_warning ("Callback called without valid callback function.");
        return;
    }
      
    /* Create a new PyDiaDiagram object. This really should reuse the object that we connected to. 
     * We'll do that later.
     */
    if (dia)
        dgm = PyDiaDiagram_New (dia);
    else {
        dgm = Py_None;
        Py_INCREF (dgm);
    }
    
      
    Py_INCREF(func);

    /* Call the callback. */
    arg = Py_BuildValue ("(Oi)", dgm,sel);
    if (arg) {
      res = PyEval_CallObject (func, arg);
      ON_RES(res, FALSE);
    }
    Py_XDECREF (arg);

    Py_DECREF(func);
    Py_XDECREF(dgm);
}

/** Connects a python function to a signal.
 *  @param self The PyDiaDiagram this is a method of.
 *  @param args A tuple containing the arguments, a str for signal name
 *  and a callable object (like a function)
 */
static PyObject *
PyDiaDiagram_ConnectAfter(PyDiaDiagram *self, PyObject *args)
{
    PyObject *func;
    char *signal;

    /* Check arguments */
    if (!PyArg_ParseTuple(args, "sO:connect_after",&signal,&func))
        return NULL;

    /* Check that the arg is callable */
    if (!PyCallable_Check(func)) {
        PyErr_SetString(PyExc_TypeError, "Second parameter must be callable");
        return NULL;
    }

    /* check if the signals name is valid */
    if ( strcmp("removed",signal) == 0 || strcmp("selection_changed",signal) == 0) {

        Py_INCREF(func); /* stay alive, where to kill ?? */

        /* connect to signal after by signal name */
        if (strcmp("removed",signal) == 0)
        {
            g_signal_connect_after(DIA_DIAGRAM(self->dia),"removed",G_CALLBACK(PyDiaDiagram_CallbackRemoved), func);
        }
        
        if (strcmp("selection_changed",signal) == 0)
        {
            g_signal_connect_after(DIA_DIAGRAM(self->dia),"selection_changed",G_CALLBACK(PyDiaDiagram_CallbackSelectionChanged), func);
        }
 
        Py_INCREF(Py_None);
        return Py_None;
    }
    else {
            PyErr_SetString(PyExc_TypeError, "Wrong signal name");
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

#define T_INVALID -1 /* can't allow direct access due to pyobject->handle indirection */
static PyMemberDef PyDiaDiagram_Members[] = {
    { "data", T_INVALID, 0, RESTRICTED|READONLY, /* can't allow direct access due to pyobject->dia indirection */
      "Backward-compatible base-class access" },
    { "displays", T_INVALID, 0, RESTRICTED|READONLY,
      "The list of current displays of this diagram." },
    { "filename", T_INVALID, 0, RESTRICTED|READONLY,
      "Filename in utf-8 encoding." },
    { "modified", T_INVALID, 0, RESTRICTED|READONLY,
      "Modification state." },
    { "selected", T_INVALID, 0, RESTRICTED|READONLY,
      "The current object selection." },
    { "unsaved", T_INVALID, 0, RESTRICTED|READONLY,
      "True if the diagram was not saved yet." },
    { NULL }
};

static PyObject *
PyDiaDiagram_GetAttr(PyDiaDiagram *self, gchar *attr)
{
    if (!strcmp(attr, "__members__"))
	return Py_BuildValue("[sssss]",
			     "data", "displays", "filename",
			     "modified", "selected", "unsaved");
    else if (!strcmp(attr, "data"))
        return PyDiaDiagramData_New (self->dia->data);
    else if (!strcmp(attr, "filename"))
	return PyString_FromString(self->dia->filename);
    else if (!strcmp(attr, "unsaved"))
	return PyInt_FromLong(self->dia->unsaved);
    else if (!strcmp(attr, "modified"))
	return PyInt_FromLong(diagram_is_modified(self->dia));
    else if (!strcmp(attr, "selected")) {
	guint i, len = g_list_length (self->dia->data->selected);
	PyObject *ret = PyTuple_New(len);
	GList *tmp;

	for (i = 0, tmp = self->dia->data->selected; tmp; i++, tmp = tmp->next)
	    PyTuple_SetItem(ret, i, PyDiaObject_New((DiaObject *)tmp->data));
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
    "dia.Diagram",
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
    (getattrofunc)0,
    (setattrofunc)0,
    (PyBufferProcs *)0,
    0L, /* Flags */
    "Subclass of dia.DiagramData (at least in the C implementation) adding interfacing the GUI elements.",
    (traverseproc)0,
    (inquiry)0,
    (richcmpfunc)0,
    0, /* tp_weakliszoffset */
    (getiterfunc)0,
    (iternextfunc)0,
    PyDiaDiagram_Methods, /* tp_methods */
    PyDiaDiagram_Members, /* tp_members */
    0
};
