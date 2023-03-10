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
PyDiaProperties_Dealloc (PyObject *self)
{
  ((PyDiaObject *) self)->object = NULL; /* XXX: should dec ref */
  PyObject_DEL (self);
}


/*
 * Compare
 */
static PyObject *
PyDiaProperties_RichCompare (PyObject *self,
                             PyObject *other,
                             int       op)
{
  Py_RETURN_RICHCOMPARE (((PyDiaProperties *) self)->object,
                         ((PyDiaProperties *) other)->object,
                         op);
}


/*
 * Hash
 */
static Py_hash_t
PyDiaProperties_Hash (PyObject *self)
{
  return (long) self;
}


/*
 * Repr / _Str
 */
static PyObject *
PyDiaProperties_Str (PyObject *self)
{
  PyObject* py_s;
  char* s = g_strdup_printf ("<DiaProperties at %p of DiaObject at %p>",
                             self, ((PyDiaObject *) self)->object);

  py_s = PyUnicode_FromString (s);
  g_clear_pointer (&s, g_free);

  return py_s;
}


/*
 * Implement the dictionary interface for the Properties object
 */
static PyObject *
PyDiaProperties_Get (PyObject *self, PyObject *args)
{
  PyObject *key;
  PyObject *failobj = Py_None;
  PyObject *val = NULL;

  if (!PyArg_ParseTuple (args, "O|O:get", &key, &failobj))
    return NULL;

  if (((PyDiaObject *) self)->object->ops->get_props != NULL) {
    Property *p;
    const char *name = PyUnicode_AsUTF8 (key);

    p = object_prop_by_name (((PyDiaObject *) self)->object, name);

    if (p) {
      val = PyDiaProperty_New (p); /* makes a copy */
      p->ops->free(p);
    }
  }

  if (val == NULL) {
    val = failobj;
    Py_INCREF (val);
  }

  return val;
}


static PyObject *
PyDiaProperties_HasKey (PyDiaProperties *self, PyObject *args)
{
  PyObject *key;
  long ok = 0;

  if (!PyArg_ParseTuple (args, "O:has_key", &key)) {
    return NULL;
  }

  if (!PyUnicode_Check (key)) {
    ok = 0; /* is this too drastic? */
  }

  if (self->object->ops->get_props != NULL) {
    Property *p;
    const char *name;

    name = PyUnicode_AsUTF8 (key);

    p = object_prop_by_name (self->object, name);
    ok = (NULL != p);
    if (p) {
      p->ops->free(p);
    }
  }

  return PyLong_FromLong (ok);
}


static PyObject *
PyDiaProperties_Keys (PyDiaProperties *self, PyObject *args)
{
  PyObject *list;
  const PropDescription *desc = NULL;

  list = PyList_New (0);

  // TODO: Why the check?
  if (self->object->ops->describe_props) {
    desc = dia_object_describe_properties (self->object);
  }

  if (desc) {
    for (int i = 0; desc[i].name; i++) {
      /* at the moment I see no use case to access widgets from Python, PROP_FLAG_LOAD_ONLY compatibility not anted here */
      if ((desc[i].flags & (PROP_FLAG_WIDGET_ONLY|PROP_FLAG_LOAD_ONLY)) == 0) {
        PyList_Append (list, PyUnicode_FromString (desc[i].name));
      }
    }
  }

  return list;
}


static Py_ssize_t
PyDiaProperties_Length (PyObject *obj)
{
  PyDiaProperties *self = (PyDiaProperties *) obj;

  if (self->nprops < 0) {
    const PropDescription *desc = NULL;

    if (self->object->ops->describe_props) {
      desc = dia_object_describe_properties (self->object);
    }

    self->nprops = 0;
    if (desc) {
      int i;
      for (i = 0; desc[i].name; i++) {
        self->nprops++;
      }
    }
  }

  return self->nprops;
}


static PyObject *
PyDiaProperties_Subscript (PyObject *obj, PyObject *key)
{
  PyDiaProperties *self = (PyDiaProperties *) obj;
  PyObject *v = NULL;

  if (!self->object->ops->describe_props) {
    PyErr_SetObject (PyExc_KeyError, key);
    return NULL;
  } else {
    Property *p;
    const char *name;

    name = PyUnicode_AsUTF8 (key);
    p = object_prop_by_name (self->object, name);

    if (p) {
      v = PyDiaProperty_New (p); /* makes a copy */
      p->ops->free (p);
    }
  }

  if (v == NULL)
    PyErr_SetObject(PyExc_KeyError, key);

  return v;
}


static int
PyDiaProperties_AssSub (PyObject *obj, PyObject *key, PyObject *val)
{
  PyDiaProperties *self = (PyDiaProperties *) obj;
  int ret = -1;

  if (val == NULL) {
    PyErr_SetString (PyExc_TypeError, "can not delete properties.");
  } else {
    Property *p;
    const char *name;

    name = PyUnicode_AsUTF8 (key);
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


static PyObject *
PyDiaProperties_Item (PyObject *o, gssize i)
{
  PyDiaProperties *self = (PyDiaProperties *) o;
  PyObject *v = NULL;
  Property *p;
  GPtrArray *props;

  if (i > self->nprops || i < 0) {
    PyErr_SetString (PyExc_IndexError, "PyDiaProperties index out of range");
    return NULL;
  }

  props = g_ptr_array_new ();

  object_get_props (self->object, props);

  p = g_ptr_array_index (props, i);

  if (p) {
    v = PyDiaProperty_New (p); /* makes a copy */
    p->ops->free (p);
  }

  g_ptr_array_unref (props);

  return v;
}


static PyMappingMethods PyDiaProperties_AsMapping = {
  .mp_length = PyDiaProperties_Length,
  .mp_subscript = PyDiaProperties_Subscript,
  .mp_ass_subscript = PyDiaProperties_AssSub,
};


static PySequenceMethods PyDiaProperties_AsSequence = {
  .sq_length = PyDiaProperties_Length,
  .sq_item = PyDiaProperties_Item,
};


static PyMethodDef PyDiaProperties_Methods[] = {
  /* duck-typing dictionary */
  {"get",     (PyCFunction) PyDiaProperties_Get,    METH_VARARGS},
  {"has_key", (PyCFunction) PyDiaProperties_HasKey, METH_VARARGS},
  {"keys",    (PyCFunction) PyDiaProperties_Keys,   METH_NOARGS},
  {NULL, NULL}
};


#define T_INVALID -1 /* can't allow direct access due to pyobject->handle indirection */
static PyMemberDef PyDiaProperties_Members[] = {
  { NULL }
};


/*
 * Python object
 */
PyTypeObject PyDiaProperties_Type = {
  PyVarObject_HEAD_INIT (NULL, 0)
  .tp_name = "dia.Properties",
  .tp_basicsize = sizeof (PyDiaProperties),
  .tp_dealloc = PyDiaProperties_Dealloc,
  .tp_getattro = PyObject_GenericGetAttr,
  .tp_richcompare = PyDiaProperties_RichCompare,
  .tp_as_mapping = &PyDiaProperties_AsMapping,
  .tp_as_sequence = &PyDiaProperties_AsSequence,
  .tp_hash = PyDiaProperties_Hash,
  .tp_str = PyDiaProperties_Str,
  .tp_doc = "A dictionary interface to dia.Object's standard properties. "
            "Many properties can be get and set through this. If there is "
            "a specific method to change an objects property like o.move() "
            "or o.move_handle() use that instead.",
  .tp_methods = PyDiaProperties_Methods,
  .tp_members = PyDiaProperties_Members,
};
