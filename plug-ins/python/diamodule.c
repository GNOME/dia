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
#include "pydia-export.h"
#include "pydia-geometry.h"
#include "pydia-diagramdata.h"
#include "pydia-font.h"
#include "pydia-color.h"
#include "pydia-image.h"
#include "pydia-properties.h"
#include "pydia-error.h"

#include "object.h"
#include "app/diagram.h"
#include "app/display.h"
#include "app/load_save.h"

#ifdef G_OS_WIN32
#pragma message("FIXME: open_diagrams")
#define open_diagrams NULL
#endif

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

static PyObject *
PyDia_RegisterExport(PyObject *self, PyObject *args)
{
    gchar *name;
    gchar *ext;
    PyObject *obj;
    DiaExportFilter *filter;
    PyObject* renderer;

    if (!PyArg_ParseTuple(args, "ssO:dia.register_export",
			  &name, &ext, &renderer))
	return NULL;

    Py_INCREF(renderer); /* stay alive, where to kill ?? */

    filter = g_new (DiaExportFilter, 1);
    filter->description = g_strdup (name);
    filter->extensions = g_new (gchar*, 2);
    filter->extensions[0] = g_strdup (ext);
    filter->extensions[1] = NULL;
    filter->export = &PyDia_export_data;
    filter->user_data = renderer;
    obj = PyDiaExportFilter_New(filter);

    filter_register_export(filter);

    return obj;
}


static PyMethodDef dia_methods[] = {
    { "diagrams", PyDia_Diagrams, 1 },
    { "load", PyDia_Load, 1 },
    { "get_object_type", PyDia_GetObjectType, 1 },
    { "active_display", PyDia_ActiveDisplay, 1 },
    { "update_all", PyDia_UpdateAll, 1 },
    { "register_export", PyDia_RegisterExport, 1 },
    { NULL, NULL }
};

void initdia(void);

DL_EXPORT(void)
initdia(void)
{
    PyObject *m, *d;

#if defined (_MSC_VER)
     /* see: Python FAQ 3.24 "Initializer not a constant." */
    PyDiaConnectionPoint_Type.ob_type = &PyType_Type;
    PyDiaDiagram_Type.ob_type = &PyType_Type;
    PyDiaDisplay_Type.ob_type = &PyType_Type;
    PyDiaHandle_Type.ob_type = &PyType_Type;
    PyDiaLayer_Type.ob_type = &PyType_Type;
    PyDiaObject_Type.ob_type = &PyType_Type;
    PyDiaObjectType_Type.ob_type = &PyType_Type;

    PyDiaExportFilter_Type.ob_type = &PyType_Type;
    PyDiaDiagramData_Type.ob_type = &PyType_Type;
    PyDiaPoint_Type.ob_type = &PyType_Type;
    PyDiaRectangle_Type.ob_type = &PyType_Type;
    PyDiaBezPoint_Type.ob_type = &PyType_Type;

    PyDiaFont_Type.ob_type = &PyType_Type;
    PyDiaColor_Type.ob_type = &PyType_Type;
    PyDiaImage_Type.ob_type = &PyType_Type;
    PyDiaProperty_Type.ob_type = &PyType_Type;
    PyDiaProperties_Type.ob_type = &PyType_Type;
    PyDiaError_Type.ob_type = &PyType_Type;
    PyDiaArrow_Type.ob_type = &PyType_Type;
#endif

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
    PyDict_SetItemString(d, "DiaExportFilter",
			 (PyObject *)&PyDiaExportFilter_Type);
    PyDict_SetItemString(d, "DiaDiagramData",
			 (PyObject *)&PyDiaDiagramData_Type);
    PyDict_SetItemString(d, "DiaPoint",
			 (PyObject *)&PyDiaPoint_Type);
    PyDict_SetItemString(d, "DiaRectangle",
			 (PyObject *)&PyDiaRectangle_Type);
    PyDict_SetItemString(d, "DiaBezPoint",
			 (PyObject *)&PyDiaBezPoint_Type);
    PyDict_SetItemString(d, "DiaFont",
			 (PyObject *)&PyDiaFont_Type);
    PyDict_SetItemString(d, "DiaColor",
			 (PyObject *)&PyDiaColor_Type);
    PyDict_SetItemString(d, "DiaImage",
			 (PyObject *)&PyDiaImage_Type);
    PyDict_SetItemString(d, "DiaProperty",
			 (PyObject *)&PyDiaProperty_Type);
    PyDict_SetItemString(d, "DiaProperties",
			 (PyObject *)&PyDiaProperties_Type);
    PyDict_SetItemString(d, "DiaError",
			 (PyObject *)&PyDiaError_Type);
    PyDict_SetItemString(d, "DiaArrow",
			 (PyObject *)&PyDiaArrow_Type);

    if (PyErr_Occurred())
	Py_FatalError("can't initialise module dia");
}
