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
#include "pydia-text.h"

#include "pydia-color.h"
#include "pydia-font.h"
#include "pydia-geometry.h"

#include <structmember.h> /* PyMemberDef */


/*
 * New
 */
PyObject *
PyDiaText_New (char *text_data, TextAttributes *attr)
{
  PyDiaText *self;

  self = PyObject_NEW (PyDiaText, &PyDiaText_Type);

  if (!self) return NULL;

  self->text_data = g_strdup (text_data);
  self->attr = *attr;

  return (PyObject *)self;
}


/*
 * Dealloc
 */
static void
PyDiaText_Dealloc (PyObject *self)
{
  g_clear_pointer (&((PyDiaText *) self)->text_data, g_free);
  PyObject_DEL (self);
}


/*
 * Compare
 */
static PyObject *
PyDiaText_RichCompare (PyObject *a,
                       PyObject *b,
                       int       op)
{
  PyDiaText *self = (PyDiaText *) a;
  PyDiaText *other = (PyDiaText *) b;
  int text_cmp = strcmp (self->text_data, other->text_data);
  int attr_cmp = memcmp (&(self->attr), &(other->attr), sizeof (TextAttributes));
  PyObject *ret;

  switch (op) {
    case Py_EQ:
      ret = (text_cmp == 0 && attr_cmp == 0) ? Py_True : Py_False;
      break;
    case Py_NE:
      ret = (text_cmp != 0 || attr_cmp != 0) ? Py_True : Py_False;
      break;
    case Py_LE:
      ret = (text_cmp <= 0 && attr_cmp <= 0) ? Py_True : Py_False;
      break;
    case Py_GE:
      ret = (text_cmp >= 0 && attr_cmp >= 0) ? Py_True : Py_False;
      break;
    case Py_LT:
      ret = (text_cmp < 0 && attr_cmp < 0) ? Py_True : Py_False;
      break;
    case Py_GT:
      ret = (text_cmp > 0 && attr_cmp > 0) ? Py_True : Py_False;
      break;
    default:
      ret = Py_NotImplemented;
      break;
  }

  Py_INCREF (ret);

  return ret;
}


/*
 * Hash
 */
static Py_hash_t
PyDiaText_Hash (PyObject *self)
{
  return (long) self;
}

/*
 * GetAttr
 */
static PyObject *
PyDiaText_GetAttr (PyObject *obj, PyObject *arg)
{
  PyDiaText *self;
  const char *attr;

  if (PyUnicode_Check (arg)) {
    attr = PyUnicode_AsUTF8 (arg);
  } else {
    goto generic;
  }

  self = (PyDiaText *) obj;

  if (!g_strcmp0 (attr, "__members__")) {
    return Py_BuildValue ("[sssss]",
                          "text", "font", "height", "position", "color",
                          "alignment");
  } else if (!g_strcmp0 (attr, "text")) {
    return PyUnicode_FromString (self->text_data);
  } else if (!g_strcmp0 (attr, "font")) {
    return PyDiaFont_New (self->attr.font);
  } else if (!g_strcmp0 (attr, "height")) {
    return PyFloat_FromDouble (self->attr.height);
  } else if (!g_strcmp0 (attr, "position")) {
    return PyDiaPoint_New (&self->attr.position);
  } else if (!g_strcmp0 (attr, "color")) {
    return PyDiaColor_New (&self->attr.color);
  } else if (!g_strcmp0 (attr, "alignment")) {
    return PyLong_FromLong (self->attr.alignment);
  }

generic:
  return PyObject_GenericGetAttr (obj, arg);
}


/*
 * Repr / _Str
 */
static PyObject *
PyDiaText_Str (PyObject *self)
{
  char *strname = g_strdup_printf ("<DiaText \"%s\" at %lx>",
                                   ((PyDiaText *) self)->attr.font ?
                                      dia_font_get_family (((PyDiaText *) self)->attr.font) : "none",
                                   (long) self);
  PyObject *ret = PyUnicode_FromString (strname);

  g_clear_pointer (&strname, g_free);

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
  PyVarObject_HEAD_INIT (NULL, 0)
  .tp_name = "dia.Text",
  .tp_basicsize = sizeof (PyDiaText),
  .tp_dealloc = PyDiaText_Dealloc,
  .tp_getattro = PyDiaText_GetAttr,
  .tp_richcompare = PyDiaText_RichCompare,
  .tp_hash = PyDiaText_Hash,
  .tp_str = PyDiaText_Str,
  .tp_doc = "Many objects (dia.Object) having text to display provide this "
            "property.",
  .tp_members = PyDiaText_Members,
};
