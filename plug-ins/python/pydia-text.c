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
#include "pydia-text.h"

#include "pydia-color.h"
#include "pydia-font.h"
#include "pydia-geometry.h"

#include <structmember.h> /* PyMemberDef */

/*
 * New
 */
PyObject* PyDiaText_New (gchar *text_data, TextAttributes *attr)
{
  PyDiaText *self;
  
  self = PyObject_NEW(PyDiaText, &PyDiaText_Type);
  if (!self) return NULL;
  
  self->text_data = g_strdup (text_data);
  self->attr = *attr;

  return (PyObject *)self;
}

/*
 * Dealloc
 */
static void
PyDiaText_Dealloc(PyDiaText *self)
{
  g_free (self->text_data);
  PyObject_DEL(self);
}

/*
 * Compare
 */
static int
PyDiaText_Compare(PyDiaText *self,
                  PyDiaText *other)
{
  int i;
  i = strcmp (self->text_data, other->text_data);
  if (i != 0)
    return i;
  return memcmp(&(self->attr), &(other->attr), sizeof(TextAttributes));
}

/*
 * Hash
 */
static long
PyDiaText_Hash(PyObject *self)
{
  return (long)self;
}

/*
 * GetAttr
 */
static PyObject *
PyDiaText_GetAttr(PyDiaText *self, gchar *attr)
{
  if (!strcmp(attr, "__members__"))
    return Py_BuildValue("[sssss]", "text", "font", "height", 
                         "position", "color", "alignment");
  else if (!strcmp(attr, "text"))
    return PyString_FromString(self->text_data);
  else if (!strcmp(attr, "font"))
    return PyDiaFont_New(self->attr.font);
  else if (!strcmp(attr, "height"))
    return PyFloat_FromDouble(self->attr.height);
  else if (!strcmp(attr, "position"))
    return PyDiaPoint_New(&self->attr.position);
  else if (!strcmp(attr, "color"))
    return PyDiaColor_New(&self->attr.color);
  else if (!strcmp(attr, "alignment"))
    return PyInt_FromLong(self->attr.alignment);

  PyErr_SetString(PyExc_AttributeError, attr);
  return NULL;
}

/*
 * Repr / _Str
 */
static PyObject *
PyDiaText_Str(PyDiaText *self)
{
  gchar *strname = g_strdup_printf("<DiaText \"%s\" at %lx>",
                                   self->attr.font ? dia_font_get_family (self->attr.font) : "none",
                                   (long)self);
  PyObject *ret = PyString_FromString(strname);

  g_free(strname);
  return ret;
}

#define T_INVALID -1 /* can't allow direct access due to pyobject->text indirection */
static PyMemberDef PyDiaText_Members[] = {
    { "text", T_INVALID, 0, RESTRICTED|READONLY,
      "string: text data" },
    { "font", T_INVALID, 0, RESTRICTED|READONLY,
      "Font: read-only reference to font used" },
    { "height", T_INVALID, 0, RESTRICTED|READONLY,
      "real: height of the font" },
    { "position", T_INVALID, 0, RESTRICTED|READONLY,
      "Point: alignment position of the text" },
    { "color", T_INVALID, 0, RESTRICTED|READONLY,
      "Color: color of the text" },
    { "alignment", T_INVALID, 0, RESTRICTED|READONLY,
      "int: alignment out of LEFT=0, CENTER=1, RIGHT=2" },
    { NULL }
};
/*
 * Python objetcs
 */
PyTypeObject PyDiaText_Type = {
    PyObject_HEAD_INIT(NULL)
    0,
    "dia.Text",
    sizeof(PyDiaText),
    0,
    (destructor)PyDiaText_Dealloc,
    (printfunc)0,
    (getattrfunc)PyDiaText_GetAttr,
    (setattrfunc)0,
    (cmpfunc)PyDiaText_Compare,
    (reprfunc)0,
    0,
    0,
    0,
    (hashfunc)PyDiaText_Hash,
    (ternaryfunc)0,
    (reprfunc)PyDiaText_Str,
    (getattrofunc)0,
    (setattrofunc)0,
    (PyBufferProcs *)0,
    0L, /* Flags */
    "Many objects (dia.Object) having text to display provide this property.",
    (traverseproc)0,
    (inquiry)0,
    (richcmpfunc)0,
    0, /* tp_weakliszoffset */
    (getiterfunc)0,
    (iternextfunc)0,
    0, /* tp_methods */
    PyDiaText_Members, /* tp_members */
    0
};
