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
#include "pydia-font.h"

#include <structmember.h> /* PyMemberDef */

/*
 * New
 */
PyObject* PyDiaFont_New (DiaFont* font)
{
  PyDiaFont *self;

  self = PyObject_NEW(PyDiaFont, &PyDiaFont_Type);
  if (!self) return NULL;

  if (font) {
    self->font = g_object_ref (font);
  } else {
    self->font = NULL;
  }

  return (PyObject *) self;
}

/*
 * Dealloc
 */
static void
PyDiaFont_Dealloc (PyObject *self)
{
  g_clear_object (&((PyDiaFont *) self)->font);

  PyObject_DEL (self);
}

/*
 * Compare
 */
static PyObject *
PyDiaFont_RichCompare (PyObject *a,
                       PyObject *b,
                       int       op)
{
  PyDiaFont *self = (PyDiaFont *) a;
  PyDiaFont *other = (PyDiaFont *) b;

  /* Hmm, not sure about this at all */

  switch (op) {
    case Py_EQ:
      if ((self->font && other->font) &&
          (g_strcmp0 (dia_font_get_family (self->font),
                      dia_font_get_family (other->font)) == 0
          && dia_font_get_style (self->font) == dia_font_get_style (other->font))) {
        Py_RETURN_TRUE;
      }
      break;
    case Py_NE:
      if ((self->font && other->font) &&
          (g_strcmp0 (dia_font_get_family (self->font),
                      dia_font_get_family (other->font)) != 0
          || dia_font_get_style (self->font) != dia_font_get_style (other->font))) {
        Py_RETURN_TRUE;
      }
      break;
    case Py_LT:
      if ((self->font && other->font) && g_strcmp0 (dia_font_get_family (self->font),
                                                    dia_font_get_family (other->font)) < 0) {
        Py_RETURN_TRUE;
      }
      break;
    case Py_GT:
      if ((self->font && other->font) && g_strcmp0 (dia_font_get_family (self->font),
                                                    dia_font_get_family (other->font)) > 0) {
        Py_RETURN_TRUE;
      }
      break;
    case Py_LE:
      if ((self->font && other->font) && g_strcmp0 (dia_font_get_family (self->font),
                                                    dia_font_get_family (other->font)) <= 0) {
        Py_RETURN_TRUE;
      }
      break;
    case Py_GE:
      if ((self->font && other->font) && g_strcmp0 (dia_font_get_family (self->font),
                                                    dia_font_get_family (other->font)) >= 0) {
        Py_RETURN_TRUE;
      }
      break;
    default:
      Py_RETURN_NOTIMPLEMENTED;
  }

  Py_RETURN_FALSE;
}


/*
 * Hash
 */
static Py_hash_t
PyDiaFont_Hash (PyObject *self)
{
  return (long) self;
}


/*
 * GetAttr
 */
static PyObject *
PyDiaFont_GetAttr (PyObject *obj, PyObject *arg)
{
  PyDiaFont *self;
  const char *attr;

  if (PyUnicode_Check (arg)) {
    attr = PyUnicode_AsUTF8 (arg);
  } else {
    goto generic;
  }

  self = (PyDiaFont *) obj;

  if (!strcmp (attr, "__members__")) {
    return Py_BuildValue ("[sss]", "family", "name", "style");
  } else if (!strcmp (attr, "name")) {
    return PyUnicode_FromString (dia_font_get_legacy_name (self->font));
  } else if (!strcmp (attr, "family")) {
    return PyUnicode_FromString (dia_font_get_family (self->font));
  } else if (!strcmp (attr, "style")) {
    return PyLong_FromLong (dia_font_get_style (self->font));
  }

generic:
  return PyObject_GenericGetAttr (obj, arg);
}


/*
 * Repr / _Str
 */
static PyObject *
PyDiaFont_Str (PyObject *obj)
{
  PyDiaFont *self = (PyDiaFont *) obj;
  PyObject *ret;
  char *s;

  if (self->font) {
    s = g_strdup_printf ("%s %s %s",
                         dia_font_get_family (self->font),
                         dia_font_get_weight_string (self->font),
                         dia_font_get_slant_string (self->font));
  } else {
    s = g_strdup ("<DiaFont NULL>");
  }

  ret = PyUnicode_FromString (s);
  g_clear_pointer (&s, g_free);

  return ret;
}


#define T_INVALID -1 /* can't allow direct access due to pyobject->cpoint indirection */
static PyMemberDef PyDiaFont_Members[] = {
    { "family", T_INVALID, 0, RESTRICTED|READONLY,
      "string: family name of the font" },
    { "name", T_INVALID, 0, RESTRICTED|READONLY,
      "string: legacy name of the font" },
    { "style", T_INVALID, 0, RESTRICTED|READONLY,
      "int: style flags" },
    { NULL }
};


/*
 * Python object
 */
PyTypeObject PyDiaFont_Type = {
  PyVarObject_HEAD_INIT (NULL, 0)
  .tp_name = "dia.Font",
  .tp_basicsize = sizeof (PyDiaFont),
  .tp_dealloc = PyDiaFont_Dealloc,
  .tp_getattro = PyDiaFont_GetAttr,
  .tp_richcompare = PyDiaFont_RichCompare,
  .tp_hash = PyDiaFont_Hash,
  .tp_str = PyDiaFont_Str,
  .tp_doc = "Provides access to some objects font property.",
  .tp_members = PyDiaFont_Members,
};
