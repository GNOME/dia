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
#include "pydia-geometry.h"

#include <structmember.h> /* PyMemberDef */

/* Implements wrappers for Point, DiaRectangle, BezPoint */

/*
 * New
 */
PyObject* PyDiaPoint_New (Point* pt)
{
  PyDiaPoint *self;

  self = PyObject_NEW(PyDiaPoint, &PyDiaPoint_Type);
  if (!self) return NULL;

  self->pt = *pt;

  return (PyObject *)self;
}

PyObject*
PyDiaPointTuple_New (Point* pts, int num)
{
  PyObject* ret;
  int i;

  ret = PyTuple_New (num);
  if (ret) {
    for (i = 0; i < num; i++)
      PyTuple_SetItem(ret, i, PyDiaPoint_New(&(pts[i])));
  }

  return ret;
}


/* one of the parameters needs to be NULL, the other is created */
PyObject *
PyDiaRectangle_New (DiaRectangle *r)
{
  PyDiaRectangle *self;

  self = PyObject_NEW (PyDiaRectangle, &PyDiaRectangle_Type);

  if (!self) {
    return NULL;
  }

  self->r = *r;

  return (PyObject *) self;
}


PyObject *
PyDiaRectangle_New_FromPoints (Point *ul, Point *lr)
{
  PyDiaRectangle *self;

  self = PyObject_NEW (PyDiaRectangle, &PyDiaRectangle_Type);

  if (!self) {
    return NULL;
  }

  self->r.left = ul->x;
  self->r.top = ul->y;
  self->r.right = lr->x;
  self->r.bottom = lr->y;

  return (PyObject *) self;
}


PyObject* PyDiaBezPoint_New (BezPoint* bpn)
{
  PyDiaBezPoint *self;

  self = PyObject_NEW(PyDiaBezPoint, &PyDiaBezPoint_Type);
  if (!self) return NULL;

  self->bpn = *bpn;

  return (PyObject *)self;
}

PyObject*
PyDiaBezPointTuple_New (BezPoint* pts, int num)
{
  PyObject* ret;
  int i;

  ret = PyTuple_New (num);
  if (ret) {
    for (i = 0; i < num; i++)
      PyTuple_SetItem(ret, i, PyDiaBezPoint_New(&(pts[i])));
  }

  return ret;
}

PyObject* PyDiaArrow_New (Arrow* arrow)
{
  PyDiaArrow *self;

  self = PyObject_NEW(PyDiaArrow, &PyDiaArrow_Type);
  if (!self) return NULL;

  self->arrow = *arrow;

  return (PyObject *)self;
}

PyObject*
PyDiaMatrix_New (DiaMatrix *matrix)
{
  PyDiaMatrix *self;

  self = PyObject_NEW(PyDiaMatrix, &PyDiaMatrix_Type);
  if (!self) return NULL;

  if (matrix)
    self->matrix = *matrix;
  else {
    /* identity matrix */
    self->matrix.xx = self->matrix.yy = 1.0;
    self->matrix.xy = self->matrix.yx = self->matrix.x0 = self->matrix.y0 = 0.0;
  }

  return (PyObject *)self;
}


/*
 * Dealloc
 */
static void
PyDiaGeometry_Dealloc (PyObject *self)
{
  PyObject_DEL (self);
}


/*
 * Compare ?
 */
static PyObject *
PyDiaPoint_RichCompare (PyObject *self,
                        PyObject *other,
                        int       op)
{
  Point *point_a = &((PyDiaPoint *) self)->pt;
  Point *point_b = &((PyDiaPoint *) other)->pt;

  switch (op) {
    case Py_EQ:
      if (fabs (point_a->x - point_b->x) < 0.0001 &&
          fabs (point_a->y - point_b->y) < 0.0001) {
        Py_RETURN_TRUE;
      } else {
        Py_RETURN_FALSE;
      }
      break;
    case Py_NE:
      if (fabs (point_a->x - point_b->x) >= 0.0001 &&
          fabs (point_a->y - point_b->y) >= 0.0001) {
        Py_RETURN_TRUE;
      } else {
        Py_RETURN_FALSE;
      }
      break;
    case Py_LT:
    case Py_GT:
    case Py_LE:
    case Py_GE:
    default:
      Py_RETURN_NOTIMPLEMENTED;
  }
}


static PyObject *
PyDiaRectangle_RichCompare (PyObject *self,
                            PyObject *other,
                            int       op)
{
  DiaRectangle *rect_a = &((PyDiaRectangle *) self)->r;
  DiaRectangle *rect_b = &((PyDiaRectangle *) other)->r;

  switch (op) {
    case Py_EQ:
      if (fabs (rect_a->top - rect_b->top) < 0.0001 &&
          fabs (rect_a->left - rect_b->left) < 0.0001 &&
          fabs (rect_a->bottom - rect_b->bottom) < 0.0001 &&
          fabs (rect_a->right - rect_b->right) < 0.0001) {
        Py_RETURN_TRUE;
      } else {
        Py_RETURN_FALSE;
      }
      break;
    case Py_NE:
      if (fabs (rect_a->top - rect_b->top) >= 0.0001 &&
          fabs (rect_a->left - rect_b->left) >= 0.0001 &&
          fabs (rect_a->bottom - rect_b->bottom) >= 0.0001 &&
          fabs (rect_a->right - rect_b->right) >= 0.0001) {
        Py_RETURN_TRUE;
      } else {
        Py_RETURN_FALSE;
      }
      break;
    case Py_LT:
    case Py_GT:
    case Py_LE:
    case Py_GE:
    default:
      Py_RETURN_NOTIMPLEMENTED;
  }
}


static PyObject *
PyDiaBezPoint_RichCompare (PyObject *a,
                           PyObject *b,
                           int       op)
{
  PyDiaBezPoint *self = (PyDiaBezPoint *) a;
  PyDiaBezPoint *other = (PyDiaBezPoint *) b;
  int cmp = memcmp (&self->bpn, &other->bpn, sizeof (BezPoint));
  PyObject *ret;

  switch (op) {
    case Py_EQ:
      ret = cmp == 0 ? Py_True : Py_False;
      break;
    case Py_NE:
      ret = cmp != 0 ? Py_True : Py_False;
      break;
    case Py_LE:
      ret = cmp <= 0 ? Py_True : Py_False;
      break;
    case Py_GE:
      ret = cmp >= 0 ? Py_True : Py_False;
      break;
    case Py_LT:
      ret = cmp < 0 ? Py_True : Py_False;
      break;
    case Py_GT:
      ret = cmp > 0 ? Py_True : Py_False;
      break;
    default:
      ret = Py_NotImplemented;
      break;
  }

  Py_INCREF (ret);

  return ret;
}


static PyObject *
PyDiaArrow_RichCompare (PyObject *a,
                        PyObject *b,
                        int       op)
{
  PyDiaArrow *self = (PyDiaArrow *) a;
  PyDiaArrow *other = (PyDiaArrow *) b;
  int cmp = memcmp (&self->arrow, &other->arrow, sizeof (Arrow));
  PyObject *ret;

  switch (op) {
    case Py_EQ:
      ret = cmp == 0 ? Py_True : Py_False;
      break;
    case Py_NE:
      ret = cmp != 0 ? Py_True : Py_False;
      break;
    case Py_LE:
      ret = cmp <= 0 ? Py_True : Py_False;
      break;
    case Py_GE:
      ret = cmp >= 0 ? Py_True : Py_False;
      break;
    case Py_LT:
      ret = cmp < 0 ? Py_True : Py_False;
      break;
    case Py_GT:
      ret = cmp > 0 ? Py_True : Py_False;
      break;
    default:
      ret = Py_NotImplemented;
      break;
  }

  Py_INCREF (ret);

  return ret;
}


static PyObject *
PyDiaMatrix_RichCompare (PyObject *a,
                         PyObject *b,
                         int       op)
{
  PyDiaMatrix *self = (PyDiaMatrix *) a;
  PyDiaMatrix *other = (PyDiaMatrix *) b;
  int cmp = memcmp (&self->matrix, &other->matrix, sizeof (DiaMatrix));
  PyObject *ret;

  switch (op) {
    case Py_EQ:
      ret = cmp == 0 ? Py_True : Py_False;
      break;
    case Py_NE:
      ret = cmp != 0 ? Py_True : Py_False;
      break;
    case Py_LE:
      ret = cmp <= 0 ? Py_True : Py_False;
      break;
    case Py_GE:
      ret = cmp >= 0 ? Py_True : Py_False;
      break;
    case Py_LT:
      ret = cmp < 0 ? Py_True : Py_False;
      break;
    case Py_GT:
      ret = cmp > 0 ? Py_True : Py_False;
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
PyDiaGeometry_Hash (PyObject *self)
{
  return (long) self;
}


static PyObject *
PyDiaRectangle_GetAttr (PyObject *obj, PyObject *arg)
{
  PyDiaRectangle *self;
  const char *attr;

  if (PyUnicode_Check (arg)) {
    attr = PyUnicode_AsUTF8 (arg);
  } else {
    goto generic;
  }

  self = (PyDiaRectangle *) obj;

#define I_OR_F(v) PyFloat_FromDouble (self->r.v)

  if (!g_strcmp0 (attr, "__members__")) {
    return Py_BuildValue("[ssss]", "top", "left", "right", "bottom" );
  } else if (!g_strcmp0 (attr, "top")) {
    return I_OR_F (top);
  } else if (!g_strcmp0 (attr, "left")) {
    return I_OR_F (left);
  } else if (!g_strcmp0 (attr, "right")) {
    return I_OR_F (right);
  } else if (!g_strcmp0 (attr, "bottom")) {
    return I_OR_F (bottom);
  }

generic:
  return PyObject_GenericGetAttr (obj, arg);

#undef I_O_F
}


static PyObject *
PyDiaBezPoint_GetAttr (PyObject *obj, PyObject *arg)
{
  PyDiaBezPoint *self;
  const char *attr;

  if (PyUnicode_Check (arg)) {
    attr = PyUnicode_AsUTF8 (arg);
  } else {
    goto generic;
  }

  self = (PyDiaBezPoint *) obj;

  if (!g_strcmp0 (attr, "__members__")) {
    return Py_BuildValue ("[ssss]", "type", "p1", "p2", "p3");
  } else if (!g_strcmp0 (attr, "type")) {
    return PyLong_FromLong (self->bpn.type);
  } else if (!g_strcmp0 (attr, "p1")) {
    return PyDiaPoint_New (&(self->bpn.p1));
  } else if (!g_strcmp0 (attr, "p2")) {
    return PyDiaPoint_New (&(self->bpn.p2));
  } else if (!g_strcmp0 (attr, "p3")) {
    return PyDiaPoint_New (&(self->bpn.p3));
  }

generic:
  return PyObject_GenericGetAttr (obj, arg);
}

/*
 * SetAttr
 */

/* XXX */

/*
 * Repr / _Str
 */
static PyObject *
PyDiaPoint_Str (PyObject *self)
{
  PyObject *py_s;

#ifndef _DEBUG /* gives crashes with nan */
  char *s = g_strdup_printf ("(%f,%f)",
                             (float) ((PyDiaPoint *) self)->pt.x,
                             (float) ((PyDiaPoint *) self)->pt.y);
#else
  char *s = g_strdup_printf ("(%e,%e)",
                             (float) ((PyDiaPoint *) self)->pt.x),
                             (float) ((PyDiaPoint *) self)->pt.y));
#endif

  py_s = PyUnicode_FromString (s);
  g_clear_pointer (&s, g_free);

  return py_s;
}


static PyObject *
PyDiaRectangle_Str (PyObject *self)
{
  PyObject *py_s;
  char *s;

  {
#ifndef _DEBUG /* gives crashes with nan */
    s = g_strdup_printf ("((%f,%f),(%f,%f))",
                         (float) (((PyDiaRectangle *) self)->r.left),
                         (float) (((PyDiaRectangle *) self)->r.top),
                         (float) (((PyDiaRectangle *) self)->r.right),
                         (float) (((PyDiaRectangle *) self)->r.bottom));
#else
    s = g_strdup_printf ("((%e,%e),(%e,%e))",
                         (float) (((PyDiaRectangle *) self)->r.left),
                         (float) (((PyDiaRectangle *) self)->r.top),
                         (float) (((PyDiaRectangle *) self)->r.right),
                         (float) (((PyDiaRectangle *) self)->r.bottom));
#endif
  }

  py_s = PyUnicode_FromString (s);
  g_clear_pointer (&s, g_free);

  return py_s;
}


static PyObject *
PyDiaBezPoint_Str (PyObject *self)
{
  PyObject *py_s;
#if 0 /* FIXME: this is causing bad crashes with unintialized points.
       * Probably a glib and a Dia problem ... */
  char* s = g_strdup_printf ("((%f,%f),(%f,%f),(%f,%f),%s)",
                             (float)(self->bpn.p1.x), (float)(self->bpn.p1.y),
                             (float)(self->bpn.p2.x), (float)(self->bpn.p2.y),
                             (float)(self->bpn.p3.x), (float)(self->bpn.p3.y),
                             (self->bpn.type == BEZ_MOVE_TO ? "MOVE_TO" :
                             (self->bpn.type == BEZ_LINE_TO ? "LINE_TO" : "CURVE_TO")));
#else
  char* s = g_strdup_printf ("%s",
                             (((PyDiaBezPoint *) self)->bpn.type == BEZ_MOVE_TO ? "MOVE_TO" :
                             (((PyDiaBezPoint *) self)->bpn.type == BEZ_LINE_TO ? "LINE_TO" : "CURVE_TO")));
#endif

  py_s = PyUnicode_FromString (s);
  g_clear_pointer (&s, g_free);

  return py_s;
}


static PyObject *
PyDiaArrow_Str (PyObject *self)
{
  PyObject *py_s;
  char *s = g_strdup_printf ("(%f,%f, %d)",
                             (float) (((PyDiaArrow *) self)->arrow.width),
                             (float) (((PyDiaArrow *) self)->arrow.length),
                             (int) (((PyDiaArrow *) self)->arrow.type));

  py_s = PyUnicode_FromString (s);
  g_clear_pointer (&s, g_free);

  return py_s;
}


static PyObject *
PyDiaMatrix_Str (PyObject *self)
{
  PyObject *py_s;
  char *s = g_strdup_printf ("(%f, %f, %f, %f, %f, %f)",
                             (float) (((PyDiaMatrix *) self)->matrix.xx),
                             (float) (((PyDiaMatrix *) self)->matrix.yx),
                             (float) (((PyDiaMatrix *) self)->matrix.xy),
                             (float) (((PyDiaMatrix *) self)->matrix.yy),
                             (float) (((PyDiaMatrix *) self)->matrix.x0),
                             (float) (((PyDiaMatrix *) self)->matrix.y0));

  py_s = PyUnicode_FromString (s);
  g_clear_pointer (&s, g_free);

  return py_s;
}


/*
 * sequence interface (query only)
 */
/* Point */
static gssize
point_length (PyObject *self)
{
  return 2;
}


static PyObject *
point_item (PyObject* o, gssize i)
{
  PyDiaPoint* self = (PyDiaPoint*)o;

  switch (i) {
    case 0:
      return PyFloat_FromDouble (self->pt.x);
    case 1:
      return PyFloat_FromDouble (self->pt.y);
    default :
      PyErr_SetString (PyExc_IndexError, "PyDiaPoint index out of range");
      return NULL;
  }
}


static PyObject *
point_slice (PyObject *o, gssize i, gssize j)
{
  PyObject *ret;

  /* j maybe negative */
  if (j <= 0) {
    j = 1 + j;
  }
  /* j may be rather huge [:] ^= 0:0x7FFFFFFF */
  if (j > 1) {
    j = 1;
  }
  ret = PyTuple_New (j - i + 1);
  if (ret) {
    for (gssize k = i; k <= j && k < 2; k++) {
      PyTuple_SetItem (ret, k - i, point_item (o, k));
    }
  }
  return ret;
}


static PySequenceMethods point_as_sequence = {
  point_length,   /*sq_length*/
  (binaryfunc) 0, /*sq_concat*/
  0,              /*sq_repeat*/
  point_item,     /*sq_item*/
  point_slice,    /*sq_slice*/
  0,              /*sq_ass_item*/
  0,              /*sq_ass_slice*/
  (objobjproc) 0  /*sq_contains*/
};


/* Rect */
static gssize
rect_length (PyObject *o)
{
  return 4;
}


static PyObject *
rect_item (PyObject *o, gssize i)
{
  PyDiaRectangle *self = (PyDiaRectangle *) o;

  switch (i) {
    case 0:
      return PyFloat_FromDouble (self->r.left);
    case 1:
      return PyFloat_FromDouble (self->r.top);
    case 2:
      return PyFloat_FromDouble (self->r.right);
    case 3:
      return PyFloat_FromDouble (self->r.bottom);
    default :
      PyErr_SetString (PyExc_IndexError, "PyDiaRectangle index out of range");
      return NULL;
  }
}


static PySequenceMethods rect_as_sequence = {
  .sq_length = rect_length,
  .sq_item = rect_item,
};


static PyMemberDef PyDiaPoint_Members[] = {
  { "x", T_DOUBLE, offsetof (PyDiaPoint, pt.x), 0,
    "double: coordinate horizontal part" },
  { "y", T_DOUBLE, offsetof (PyDiaPoint, pt.y), 0,
    "double: coordinate vertical part" },
  { NULL }
};


/*
 * Python objetcs
 */
PyTypeObject PyDiaPoint_Type = {
  PyVarObject_HEAD_INIT (NULL, 0)
  .tp_name = "dia.Point",
  .tp_basicsize = sizeof (PyDiaPoint),
  .tp_dealloc = PyDiaGeometry_Dealloc,
  .tp_richcompare = PyDiaPoint_RichCompare,
  .tp_as_sequence = &point_as_sequence,
  .tp_hash = PyDiaGeometry_Hash,
  .tp_str = PyDiaPoint_Str,
  .tp_getattro = PyObject_GenericGetAttr,
  .tp_doc = "The dia.Point does not only provide access trough it's members "
            "but also via a sequence interface.",
  .tp_members = PyDiaPoint_Members,
};


#define T_INVALID -1 /* can't allow direct access due to pyobject->is_int */
static PyMemberDef PyDiaRect_Members[] = {
    { "top", T_INVALID, 0, RESTRICTED|READONLY,
      "int or double: upper edge y coordinate" },
    { "left", T_INVALID, 0, RESTRICTED|READONLY,
      "int or double: left edge x coordinate" },
    { "bottom", T_INVALID, 0, RESTRICTED|READONLY,
      "int or double: lower edge y coordinate" },
    { "right", T_INVALID, 0, RESTRICTED|READONLY,
      "int or double: right edge x coordinate" },
    { NULL }
};


PyTypeObject PyDiaRectangle_Type = {
  PyVarObject_HEAD_INIT (NULL, 0)
  .tp_name = "dia.Rectangle",
  .tp_basicsize = sizeof (PyDiaRectangle),
  .tp_dealloc = PyDiaGeometry_Dealloc,
  .tp_getattro = PyDiaRectangle_GetAttr,
  .tp_richcompare = PyDiaRectangle_RichCompare,
  .tp_as_sequence = &rect_as_sequence,
  .tp_hash = PyDiaGeometry_Hash,
  .tp_str = PyDiaRectangle_Str,
  .tp_doc = "The dia.Rectangle does not only provide access trough it's "
            "members but also via a sequence interface.",
  .tp_members = PyDiaRect_Members,
};


static PyMemberDef PyDiaBezPoint_Members[] = {
    { "type", T_INT, offsetof(PyDiaBezPoint, bpn.type), 0,
      "int: MOVETO, LINETO using p1 only;  CURVETO all 3 points" },
    { "p1", T_INVALID, offsetof(PyDiaBezPoint, bpn.p1), 0, /* T_INVALID for Python not knowing Point */
      "Point: first control point for CURVETO" },
    { "p2", T_INVALID, offsetof(PyDiaBezPoint, bpn.p2), 0,
      "Point: second control point for CURVETO" },
    { "p3", T_INVALID, offsetof(PyDiaBezPoint, bpn.p3), 0,
      "Point: target point for CURVETO" },
    { NULL }
};


PyTypeObject PyDiaBezPoint_Type = {
  PyVarObject_HEAD_INIT (NULL, 0)
  .tp_name = "dia.BezPoint",
  .tp_basicsize = sizeof (PyDiaBezPoint),
  .tp_dealloc = PyDiaGeometry_Dealloc,
  .tp_getattro = PyDiaBezPoint_GetAttr,
  .tp_richcompare = PyDiaBezPoint_RichCompare,
  .tp_hash = PyDiaGeometry_Hash,
  .tp_str = PyDiaBezPoint_Str,
  .tp_doc = "A dia.Point, a bezier type and two control points (dia.Point) "
            "make a bezier point.",
  .tp_members = PyDiaBezPoint_Members,
};


static PyMemberDef PyDiaArrow_Members[] = {
    { "type", T_INT, offsetof(PyDiaArrow, arrow.type), 0,
      "int: the shape of the arrow" },
    { "width", T_DOUBLE, offsetof(PyDiaPoint, pt.x), 0,
      "double: corresponding to line width" },
    { "length", T_DOUBLE, offsetof(PyDiaPoint, pt.y), 0,
      "double: length along the line" },
    { NULL }
};


PyTypeObject PyDiaArrow_Type = {
  PyVarObject_HEAD_INIT (NULL, 0)
  .tp_name = "dia.Arrow",
  .tp_basicsize = sizeof (PyDiaArrow),
  .tp_dealloc = PyDiaGeometry_Dealloc,
  .tp_richcompare = PyDiaArrow_RichCompare,
  .tp_hash = PyDiaGeometry_Hash,
  .tp_str = PyDiaArrow_Str,
  .tp_getattro = PyObject_GenericGetAttr,
  .tp_doc = "Dia's line objects usually ends with an dia.Arrow",
  .tp_members = PyDiaArrow_Members,
};


static PyMemberDef PyDiaMatrix_Members[] = {
    { "xx", T_DOUBLE, offsetof(PyDiaMatrix, matrix.xx), 0, "double" },
    { "xy", T_DOUBLE, offsetof(PyDiaMatrix, matrix.xy), 0, "double" },
    { "yx", T_DOUBLE, offsetof(PyDiaMatrix, matrix.yx), 0, "double" },
    { "yy", T_DOUBLE, offsetof(PyDiaMatrix, matrix.yy), 0, "double" },
    { "x0", T_DOUBLE, offsetof(PyDiaMatrix, matrix.x0), 0, "double" },
    { "y0", T_DOUBLE, offsetof(PyDiaMatrix, matrix.y0), 0, "double" },
    { NULL }
};


PyTypeObject PyDiaMatrix_Type = {
  PyVarObject_HEAD_INIT (NULL, 0)
  .tp_name = "dia.Matrix",
  .tp_basicsize = sizeof (PyDiaMatrix),
  .tp_dealloc = PyDiaGeometry_Dealloc,
  .tp_richcompare = PyDiaMatrix_RichCompare,
  .tp_hash = PyDiaGeometry_Hash,
  .tp_str = PyDiaMatrix_Str,
  .tp_getattro = PyObject_GenericGetAttr,
  .tp_doc = "Dia's matrix to do affine transformation",
  .tp_members = PyDiaMatrix_Members,
};
