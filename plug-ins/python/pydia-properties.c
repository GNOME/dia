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
#include "pydia-properties.h"

#include <structmember.h> /* PyMemberDef */

/*
 * New
 */
PyObject* 
PyDiaProperties_New (DiaObject* obj)
{
  PyDiaProperties *self;
  
  self = PyObject_NEW(PyDiaProperties, &PyDiaProperties_Type);
  if (!self) return NULL;
  
  self->object = obj;  /* XXX: should be ref counted */
  self->nprops = -1;
  
  return (PyObject *)self;
}

/*
 * Dealloc
 */
static void
PyDiaProperties_Dealloc(PyDiaObject *self)
{
  self->object = NULL; /* XXX: should dec ref */
  PyObject_DEL(self);
}

/*
 * Compare
 */
static int
PyDiaProperties_Compare(PyDiaProperties *self,
                      PyDiaProperties *other)
{
  return memcmp(&(self->object), &(other->object), sizeof(DiaObject*));
}

/*
 * Hash
 */
static long
PyDiaProperties_Hash(PyObject *self)
{
  return (long)self;
}

/*
 * Repr / _Str
 */
static PyObject *
PyDiaProperties_Str(PyDiaProperties *self)
{
  PyObject* py_s;
  gchar* s = g_strdup_printf("<DiaProperties at %p of DiaObject at %p>",
                             self, self->object );
  py_s = PyString_FromString(s);
  g_free (s);
  return py_s;
}

/*
 * Implement the dictionary interface for the Properties object
 */
static PyObject *
PyDiaProperties_Get(PyDiaProperties *self, PyObject *args)
{
  PyObject *key;
  PyObject *failobj = Py_None;
  PyObject *val = NULL;

  if (!PyArg_ParseTuple(args, "O|O:get", &key, &failobj))
    return NULL;

  if (self->object->ops->get_props != NULL) {
    Property *p;
    char* name = PyString_AsString(key);
    p = object_prop_by_name (self->object, name);  
    if (p) {
      val = PyDiaProperty_New(p); /* makes a copy */
      p->ops->free(p);
    }
  }
 
  if (val == NULL) { 
    val = failobj;
    Py_INCREF(val);
  }

  return val;
}

static PyObject *
PyDiaProperties_HasKey(PyDiaProperties *self, PyObject *args)
{
  PyObject *key;
  long ok = 0;

  if (!PyArg_ParseTuple(args, "O:has_key", &key))
    return NULL;

  if (!PyString_Check(key))
    ok = 0; /* is this too drastic? */

  if (self->object->ops->get_props != NULL) {
    Property *p;
    char     *name;

    name = PyString_AsString(key);

    p = object_prop_by_name (self->object, name);  
    ok = (NULL != p);
    if (p)
      p->ops->free(p);
  }

  return PyInt_FromLong(ok);
}

static PyObject *
PyDiaProperties_Keys(PyDiaProperties *self, PyObject *args)
{
  PyObject *list;
  const PropDescription *desc = NULL;

  if (!PyArg_NoArgs(args))
    return NULL;

  list = PyList_New(0);

  if (self->object->ops->describe_props)
    desc = self->object->ops->describe_props(self->object);
  if (desc) {
    int i;
    for (i = 0; desc[i].name; i++) {
      /* at the moment I see no use case to access widgets from Python, PROP_FLAG_LOAD_ONLY compatibility not anted here */
      if ((desc[i].flags & (PROP_FLAG_WIDGET_ONLY|PROP_FLAG_LOAD_ONLY)) == 0)
        PyList_Append(list, PyString_FromString(desc[i].name));
    }
  }

  return list;
}

static int
PyDiaProperties_Length (PyDiaProperties* self)
{
  if (self->nprops < 0) {
    const PropDescription *desc = NULL;

    if (self->object->ops->describe_props)
      desc = self->object->ops->describe_props(self->object);

    self->nprops = 0;
    if (desc) {
      int i;
      for (i = 0; desc[i].name; i++)
        self->nprops++;
    }
  }

  return self->nprops;
}

static PyObject *
PyDiaProperties_Subscript (PyDiaProperties *self, register PyObject *key)
{
  PyObject *v = NULL;

  if (!self->object->ops->describe_props) {
    PyErr_SetObject(PyExc_KeyError, key);
    return NULL;
  }
  else {
    Property *p;
    char     *name;  

    name = PyString_AsString(key);
    p = object_prop_by_name (self->object, name);  

    if (p) {
      v = PyDiaProperty_New(p); /* makes a copy */
      p->ops->free (p);
    }
  }

  if (v == NULL)
    PyErr_SetObject(PyExc_KeyError, key);

  return v;
}

static int
PyDiaProperties_AssSub (PyDiaProperties* self, PyObject *key, PyObject *val)
{
  int ret = -1;

  if (val == NULL) {
    PyErr_SetString(PyExc_TypeError, "can not delete properties.");
  }
  else {
    Property *p;
    char     *name;  

    name = PyString_AsString(key);
    p = object_prop_by_name (self->object, name);  

    /* g_print ("AssSub(key: '%s', type <%s>)\n", name, (p ? p->type : "none")); */
    if (p) {
      if (0 == PyDiaProperty_ApplyToObject(self->object, name, p, val)) {
        /* if applied the property is deleted */
        ret = 0;
      }
      else {
        p->ops->free (p);
        PyErr_SetString(PyExc_TypeError, "prop type mis-match.");
      }
    }
    else {
      PyErr_SetObject(PyExc_KeyError, key);
    }
  }

  return ret;
}

static PyMappingMethods PyDiaProperties_AsMapping = {
	(inquiry)PyDiaProperties_Length, /*mp_length*/
	(binaryfunc)PyDiaProperties_Subscript, /*mp_subscript*/
	(objobjargproc)PyDiaProperties_AssSub, /*mp_ass_subscript*/
};

static PyMethodDef PyDiaProperties_Methods[] = {
	/* duck-typing dictionary */
	{"get",     (PyCFunction)PyDiaProperties_Get,     METH_VARARGS},
	{"has_key", (PyCFunction)PyDiaProperties_HasKey,  METH_VARARGS},
	{"keys",    (PyCFunction)PyDiaProperties_Keys},
	{NULL,		NULL}		/* sentinel */
};

#define T_INVALID -1 /* can't allow direct access due to pyobject->handle indirection */
static PyMemberDef PyDiaProperties_Members[] = {
	{ NULL }
};

/*
 * GetAttr
 */
static PyObject*
PyDiaProperties_GetAttr(PyDiaProperties *self, gchar *attr)
{
  return Py_FindMethod(PyDiaProperties_Methods, (PyObject *)self, attr);
}

/*
 * Python object
 */
PyTypeObject PyDiaProperties_Type = {
    PyObject_HEAD_INIT(&PyType_Type)
    0,
    "dia.Properties",
    sizeof(PyDiaProperties),
    0,
    (destructor)PyDiaProperties_Dealloc,
    (printfunc)0,
    (getattrfunc)PyDiaProperties_GetAttr,
    (setattrfunc)0,
    (cmpfunc)PyDiaProperties_Compare,
    (reprfunc)0,
    0,
    0,
    &PyDiaProperties_AsMapping, /* PyMappingMethods */
    (hashfunc)PyDiaProperties_Hash,
    (ternaryfunc)0,
    (reprfunc)PyDiaProperties_Str,
    (getattrofunc)0,
    (setattrofunc)0,
    (PyBufferProcs *)0,
    0L, /* Flags */
    "A dictionary interface to dia.Object's standard properties. Many properties"
    "can be get and set through this. If there is a specific method to change an"
    "objects property like o.move() or o.move_handle() use that instead.",
    (traverseproc)0,
    (inquiry)0,
    (richcmpfunc)0,
    0, /* tp_weakliszoffset */
    (getiterfunc)0,
    (iternextfunc)0,
    PyDiaProperties_Methods, /* tp_methods */
    PyDiaProperties_Members, /* tp_members */
    0
};
