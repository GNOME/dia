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

#include <glib.h>

#include "pydia-object.h"
#include "pydia-diagramdata.h"

#include "pydia-geometry.h"
#include "pydia-layer.h"
#include "pydia-color.h"


PyObject *
PyDiaDiagramData_New(DiagramData *dd)
{
    PyDiaDiagramData *self;

    self = PyObject_NEW(PyDiaDiagramData, &PyDiaDiagramData_Type);

    if (!self) return NULL;
    self->data = dd; /* FIXME: how long is it supposed to live ? */
    return (PyObject *)self;
}

static void
PyDiaDiagramData_Dealloc(PyDiaDiagramData *self)
{
     PyMem_DEL(self);
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
    gchar* s = g_strdup_printf ("<PyDiaDiagramData 0x%08x>", self);
    py_s = PyString_FromString(s);
    g_free(s);
    return py_s;
}

/*
 * "real" member function implementaion ?
 */

static PyMethodDef PyDiaDiagramData_Methods[] = {
    {NULL, 0, 0, NULL}
};

static PyObject *
PyDiaDiagramData_GetAttr(PyDiaDiagramData *self, gchar *attr)
{
    if (!strcmp(attr, "__members__"))
	return Py_BuildValue("[ssssssssss]", 
                           "extents", "bg_color", "paper",
                           "grid.width", "grid.visible", 
                           "hguides", "vguides",
                           "layers", "active_layer",
                           "selected" );
    else if (!strcmp(attr, "extents"))
      return PyDiaRectangle_New(&self->data->extents, NULL);
    else if (!strcmp(attr, "bg_color")) {
      return PyDiaColor_New (&(self->data->bg_color));
    }
    else if (!strcmp(attr, "paper")) {
      /* XXX */
      return NULL;
    }
    else if (!strcmp(attr, "grid.width")) 
      return Py_BuildValue("(dd)", self->data->grid.width_x, self->data->grid.width_y);
    else if (!strcmp(attr, "grid.visible")) 
      return Py_BuildValue("(ii)", self->data->grid.visible_x, self->data->grid.visible_y);
    else if (!strcmp(attr, "hguides")) {
      int len = self->data->guides.nhguides;
      PyObject *ret = PyTuple_New(len);
      int i;
      for (i = 0; i < len; i++)
        PyTuple_SetItem(ret, i, PyFloat_FromDouble(self->data->guides.hguides[i]));
      return ret;
    }
    else if (!strcmp(attr, "vguides")) {
      int len = self->data->guides.nvguides;
      PyObject *ret = PyTuple_New(len);
      int i;
      for (i = 0; i < len; i++)
        PyTuple_SetItem(ret, i, PyFloat_FromDouble(self->data->guides.vguides[i]));
      return ret;
    }
    else if (!strcmp(attr, "layers")) {
	guint i, len = self->data->layers->len;
	PyObject *ret = PyTuple_New(len);

	for (i = 0; i < len; i++)
	    PyTuple_SetItem(ret, i, PyDiaLayer_New(
			g_ptr_array_index(self->data->layers, i)));
	return ret;
    }
    else if (!strcmp(attr, "active_layer")) {
      return PyDiaLayer_New(self->data->active_layer);
    }
    else if (!strcmp(attr, "selected")) {
	PyObject *ret;
	GList *tmp;
	gint i;

	ret = PyTuple_New(g_list_length(self->data->selected));
	for (i = 0, tmp = self->data->selected; tmp; i++, tmp = tmp->next)
	    PyTuple_SetItem(ret, i, PyDiaObject_New((Object *)tmp->data));
	return ret;
    }

    return Py_FindMethod(PyDiaDiagramData_Methods, (PyObject *)self, attr);
}

PyTypeObject PyDiaDiagramData_Type = {
    PyObject_HEAD_INIT(&PyType_Type)
    0,
    "DiagramData",
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
    0L,0L,0L,0L,
    NULL
};
