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

  if (font)
    self->font = dia_font_ref (font);
  else
    self->font = NULL;

  return (PyObject *)self;
}

/*
 * Dealloc
 */
static void
PyDiaFont_Dealloc(PyDiaFont *self)
{
  if (self->font)
    dia_font_unref (self->font);
  PyObject_DEL(self);
}

/*
 * Compare
 */
static int
PyDiaFont_Compare(PyDiaFont *self,
                  PyDiaFont *other)
{
  int ret;

  if (self->font == other->font)
    return 0;
  else if (!self->font)
    return 1;
  else if (!other->font)
    return -1;

  ret = strcmp (dia_font_get_family (self->font), 
                dia_font_get_family (other->font));
  if (ret != 0)
    return ret;

  ret = dia_font_get_style (self->font) - dia_font_get_style (other->font);
  return ret > 0 ? 1 : (ret < 0 ? -1 : 0);
}

/*
 * Hash
 */
static long
PyDiaFont_Hash(PyObject *self)
{
  return (long)self;
}

/*
 * GetAttr
 */
static PyObject *
PyDiaFont_GetAttr(PyDiaFont *self, gchar *attr)
{
  if (!strcmp(attr, "__members__"))
    return Py_BuildValue("[sss]", "family", "name", "style");
  else if (!strcmp(attr, "name"))
    return PyString_FromString(dia_font_get_legacy_name (self->font));
  else if (!strcmp(attr, "family"))
    return PyString_FromString(dia_font_get_family (self->font));
  else if (!strcmp(attr, "style"))
    return PyInt_FromLong (dia_font_get_style (self->font));

  PyErr_SetString(PyExc_AttributeError, attr);
  return NULL;
}

/*
 * Repr / _Str
 */
static PyObject *
PyDiaFont_Str(PyDiaFont *self)
{
  PyObject *ret;
  gchar *s = self->font ? g_strdup_printf ("%s %s %s",
  	dia_font_get_family (self->font),
	dia_font_get_weight_string (self->font),
	dia_font_get_slant_string (self->font)) : g_strdup("<DiaFont NULL>");

  ret = PyString_FromString(s);
  g_free (s);
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
 * Python objetc
 */
PyTypeObject PyDiaFont_Type = {
    PyObject_HEAD_INIT(NULL)
    0,
    "dia.Font",
    sizeof(PyDiaFont),
    0,
    (destructor)PyDiaFont_Dealloc,
    (printfunc)0,
    (getattrfunc)PyDiaFont_GetAttr,
    (setattrfunc)0,
    (cmpfunc)PyDiaFont_Compare,
    (reprfunc)0,
    0,
    0,
    0,
    (hashfunc)PyDiaFont_Hash,
    (ternaryfunc)0,
    (reprfunc)PyDiaFont_Str,
    (getattrofunc)0,
    (setattrofunc)0,
    (PyBufferProcs *)0,
    0L, /* Flags */
    "Provides access to some objects font property.",
    (traverseproc)0,
    (inquiry)0,
    (richcmpfunc)0,
    0, /* tp_weakliszoffset */
    (getiterfunc)0,
    (iternextfunc)0,
    0, /* tp_methods */
    PyDiaFont_Members, /* tp_members */
    0
};
