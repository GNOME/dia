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

#include <Python.h>

#include "pydia-diagram.h"
#include "pydia-display.h"
#include "pydia-layer.h"
#include "pydia-object.h"
#include "pydia-cpoint.h"
#include "pydia-handle.h"

#include "object.h"
#include "app/diagram.h"
#include "app/display.h"
#include "app/load_save.h"

static PyObject *
PyDia_Diagrams(PyObject *self, PyObject *args)
{
    GList *tmp;
    PyObject *ret;

    if (!PyArg_ParseTuple(args, ":dia.diagrams"))
	return NULL;
    ret = PyList_New(0);
    for (tmp = open_diagrams; tmp; tmp = tmp->next)
	PyList_Append(ret, PyDiaDiagram_New((Diagram *)tmp->data));
    return ret;
}

static PyObject *
PyDia_Load(PyObject *self, PyObject *args)
{
    gchar *filename;
    Diagram *dia;

    if (!PyArg_ParseTuple(args, "s:dia.load", &filename))
	return NULL;
    dia = diagram_load(filename, NULL);
    if (dia)
	return PyDiaDiagram_New(dia);
    PyErr_SetString(PyExc_IOError, "could not load diagram");
    return NULL;
}

static PyObject *
PyDia_GetObjectType(PyObject *self, PyObject *args)
{
    gchar *name;
    ObjectType *otype;

    if (!PyArg_ParseTuple(args, "s:dia.get_object_type", &name))
	return NULL;
    otype = object_get_type(name);
    if (otype)
	return PyDiaObjectType_New(otype);
    PyErr_SetString(PyExc_KeyError, "unknown object type");
    return NULL;
}

static PyObject *
PyDia_ActiveDisplay(PyObject *self, PyObject *args)
{
    DDisplay *disp;

    if (!PyArg_ParseTuple(args, ":dia.active_display"))
	return NULL;
    disp = ddisplay_active();
    if (disp)
	return PyDiaDisplay_New(disp);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
PyDia_UpdateAll(PyObject *self, PyObject *args)
{
    GList *tmp;

    if (!PyArg_ParseTuple(args, ":dia.update_all"))
	return NULL;
    for (tmp = open_diagrams; tmp; tmp = tmp->next)
	diagram_add_update_all((Diagram *)tmp->data);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyMethodDef dia_methods[] = {
    { "diagrams", PyDia_Diagrams, 1 },
    { "load", PyDia_Load, 1 },
    { "get_object_type", PyDia_GetObjectType, 1 },
    { "active_display", PyDia_ActiveDisplay, 1 },
    { "update_all", PyDia_UpdateAll, 1 },
    { NULL, NULL }
};

void initdia(void);

DL_EXPORT(void)
initdia(void)
{
    PyObject *m, *d;

    m = Py_InitModule("dia", dia_methods);
    d = PyModule_GetDict(m);

    PyDict_SetItemString(d, "DiaDiagramType",
			 (PyObject *)&PyDiaDiagram_Type);
    PyDict_SetItemString(d, "DiaDisplayType",
			 (PyObject *)&PyDiaDisplay_Type);
    PyDict_SetItemString(d, "DiaLayerType",
			 (PyObject *)&PyDiaLayer_Type);
    PyDict_SetItemString(d, "DiaObjectType",
			 (PyObject *)&PyDiaObject_Type);
    PyDict_SetItemString(d, "DiaObjectTypeType",
			 (PyObject *)&PyDiaObjectType_Type);
    PyDict_SetItemString(d, "DiaConnectionPointType",
			 (PyObject *)&PyDiaConnectionPoint_Type);
    PyDict_SetItemString(d, "DiaHandleType",
			 (PyObject *)&PyDiaHandle_Type);

    if (PyErr_Occurred())
	Py_FatalError("can't initialise module dia");
}
