/* Python plug-in for dia
 * Copyright (C) 1999  James Henstridge
 * Copyright (C) 2014  Hans Breuer
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
#include "pydia-sheet.h"

#include <structmember.h> /* PyMemberDef */

PyObject *
PyDiaSheet_New(Sheet *sheet)
{
    PyDiaSheet *self;

    self = PyObject_NEW(PyDiaSheet, &PyDiaSheet_Type);

    if (!self) return NULL;
    self->sheet = sheet;
    return (PyObject *)self;
}

static void
PyDiaSheet_Dealloc(PyDiaSheet *self)
{
     PyObject_DEL(self);
}

static int
PyDiaSheet_Compare(PyDiaSheet *self, PyDiaSheet *other)
{
    if (self->sheet == other->sheet) return 0;
    if (self->sheet > other->sheet) return -1;
    return 1;
}

static long
PyDiaSheet_Hash(PyDiaSheet *self)
{
    return (long)self->sheet;
}

static PyObject *
PyDiaSheet_Str(PyDiaSheet *self)
{
    return PyString_FromString(self->sheet->description);
}

/*
 * "real" member function implementaion ?
 */

static PyMethodDef PyDiaSheet_Methods[] = {
    {NULL, 0, 0, NULL}
};

#define T_INVALID -1 /* can't allow direct access due to pyobject->data indirection */
static PyMemberDef PyDiaSheet_Members[] = {
    { "name", T_INVALID, 0, RESTRICTED|READONLY,
      "The name for the sheet." },
    { "description", T_INVALID, 0, RESTRICTED|READONLY,
      "The description for the sheet." },
    { "filename", T_INVALID, 0, RESTRICTED|READONLY,
      "The filename for the sheet." },
    { "user", T_INVALID, 0, RESTRICTED|READONLY,
      "The sheet scope is user provided, not system." },
    { "objects", T_INVALID, 0, RESTRICTED|READONLY,
      "The list of sheet objects referenced by the sheet." },
    { NULL }
};

static PyObject *
PyDiaSheet_GetAttr(PyDiaSheet *self, gchar *attr)
{
    if (!strcmp(attr, "__members__"))
	return Py_BuildValue("[ssss]", "name", "description", "filename", "objects");
    else if (!strcmp(attr, "name"))
	return PyString_FromString(self->sheet->name);
    else if (!strcmp(attr, "description"))
	return PyString_FromString(self->sheet->description);
    else if (!strcmp(attr, "filename"))
	return PyString_FromString(self->sheet->filename);
    else if (!strcmp(attr, "user"))
	return PyInt_FromLong(self->sheet->scope == SHEET_SCOPE_USER ? 1 : 0);
    else if (!strcmp(attr, "objects")) {
	/* Just returning tuples with information for now. Wrapping SheetObject
	 * looks like overkill for the time being.
	 *  - DiaObjectType or None
	 *  - description of the SheetObject
	 *  - filename of the icon file
	 */
	PyObject *ret = PyList_New(0);
	GSList *list;

	for (list = self->sheet->objects; list != NULL; list = list->next) {
	    SheetObject *so = list->data;
	    DiaObjectType *ot = object_get_type (so->object_type);

	    if (!ot)
		Py_INCREF(Py_None);
	    PyList_Append(ret, Py_BuildValue ("(Oss)",
						ot ? PyDiaObjectType_New (ot) : Py_None,
						PyString_FromString (so->description ? so->description : ""),
						PyString_FromString (so->pixmap_file ? so->pixmap_file : "")));
	}
	return ret;
    }

    return Py_FindMethod(PyDiaSheet_Methods, (PyObject *)self, attr);
}

PyTypeObject PyDiaSheet_Type = {
    PyObject_HEAD_INIT(NULL)
    0,
    "dia.Sheet",
    sizeof(PyDiaSheet),
    0,
    (destructor)PyDiaSheet_Dealloc,
    (printfunc)0,
    (getattrfunc)PyDiaSheet_GetAttr,
    (setattrfunc)0,
    (cmpfunc)PyDiaSheet_Compare,
    (reprfunc)0,
    0,
    0,
    0,
    (hashfunc)PyDiaSheet_Hash,
    (ternaryfunc)0,
    (reprfunc)PyDiaSheet_Str,
    (getattrofunc)0,
    (setattrofunc)0,
    (PyBufferProcs *)0,
    0L, /* Flags */
    "returned by dia.register_export() but not used otherwise yet.",
    (traverseproc)0,
    (inquiry)0,
    (richcmpfunc)0,
    0, /* tp_weakliszoffset */
    (getiterfunc)0,
    (iternextfunc)0,
    PyDiaSheet_Methods, /* tp_methods */
    PyDiaSheet_Members, /* tp_members */
    0
};
