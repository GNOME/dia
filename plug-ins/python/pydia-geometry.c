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
#include "pydia-geometry.h"


/* Implements wrappers for Point, Rectangle, IntRectangle, BezPoint */

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
PyObject* 
PyDiaRectangle_New (Rectangle* r, IntRectangle* ri)
{
  PyDiaRectangle *self;

  self = PyObject_NEW(PyDiaRectangle, &PyDiaRectangle_Type);
  if (!self) return NULL;

  self->is_int = (ri != NULL);
  if (self->is_int)
    self->r.ri = *ri;
  else
    self->r.rf = *r;

  return (PyObject *)self;
}

PyObject* PyDiaRectangle_New_FromPoints (Point* ul, Point* lr)
{
  PyDiaRectangle *self;

  self = PyObject_NEW(PyDiaRectangle, &PyDiaRectangle_Type);
  if (!self) return NULL;

  self->is_int = FALSE;
  self->r.rf.left = ul->x;
  self->r.rf.top = ul->y;
  self->r.rf.right = lr->x;
  self->r.rf.bottom = lr->y;

  return (PyObject *)self;
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

/*
 * Dealloc
 */
static void
PyDiaGeometry_Dealloc(void *self)
{
     PyMem_DEL(self);
}

/*
 * Compare ?
 */
static int
PyDiaPoint_Compare(PyDiaPoint *self,
			     PyDiaPoint *other)
{
#if 1
  return memcmp (&self->pt, &other->pt, sizeof(Point));
#else //?
  if (self->pt.x == other->pt.x && self->pt.x == other->pt.x) return 0;
#define SQR(pt) (pt.x*pt.y)
  if (SQR(self->pt) > SQR(other->pt)) return -1;
#undef  SQR 
  return 1;
#endif
}

static int
PyDiaRectangle_Compare(PyDiaRectangle *self,
			     PyDiaRectangle *other)
{
  /* this is not correct */
  return memcmp (&self->r, &other->r, sizeof(Rectangle));
}

static int
PyDiaBezPoint_Compare(PyDiaBezPoint *self,
			     PyDiaBezPoint *other)
{
  return memcmp (&self->bpn, &other->bpn, sizeof(BezPoint));
}

static int
PyDiaArrow_Compare(PyDiaArrow *self,
			 PyDiaArrow *other)
{
  return memcmp (&self->arrow, &other->arrow, sizeof(Arrow));
}

/*
 * Hash
 */
static long
PyDiaGeometry_Hash(PyObject *self)
{
    return (long)self;
}

/*
 * GetAttr
 */
static PyObject *
PyDiaPoint_GetAttr(PyDiaPoint *self, gchar *attr)
{
  if (!strcmp(attr, "__members__"))
    return Py_BuildValue("[ss]", "x", "y");
  else if (!strcmp(attr, "x"))
    return PyFloat_FromDouble(self->pt.x);
  else if (!strcmp(attr, "y"))
    return PyFloat_FromDouble(self->pt.y);

  PyErr_SetString(PyExc_AttributeError, attr);
  return NULL;
}

static PyObject *
PyDiaRectangle_GetAttr(PyDiaRectangle *self, gchar *attr)
{
#define I_OR_F(v) \
  (self->is_int ? \
   PyInt_FromLong(self->r.ri.##v) : PyFloat_FromDouble(self->r.rf.##v))

  if (!strcmp(attr, "__members__"))
    return Py_BuildValue("[ssss]", "top", "left", "right", "bottom" );
  else if (!strcmp(attr, "top"))
    return I_OR_F(top);
  else if (!strcmp(attr, "left"))
    return I_OR_F(left);
  else if (!strcmp(attr, "right"))
    return I_OR_F(right);
  else if (!strcmp(attr, "bottom"))
    return I_OR_F(bottom);

  PyErr_SetString(PyExc_AttributeError, attr);
  return NULL;

#undef I_O_F
}

static PyObject *
PyDiaBezPoint_GetAttr(PyDiaBezPoint *self, gchar *attr)
{
  if (!strcmp(attr, "__members__"))
    return Py_BuildValue("[ssss]", "type", "p1", "p2", "p3");
  else if (!strcmp(attr, "type"))
    return PyInt_FromLong(self->bpn.type);
  else if (!strcmp(attr, "p1"))
    return PyDiaPoint_New(&(self->bpn.p1));
  else if (!strcmp(attr, "p2"))
    return PyDiaPoint_New(&(self->bpn.p2));
  else if (!strcmp(attr, "p3"))
    return PyDiaPoint_New(&(self->bpn.p3));

  PyErr_SetString(PyExc_AttributeError, attr);
  return NULL;
}

static PyObject *
PyDiaArrow_GetAttr(PyDiaArrow *self, gchar *attr)
{
  if (!strcmp(attr, "__members__"))
    return Py_BuildValue("[sss]", "type", "width", "length");
  else if (!strcmp(attr, "type"))
    return PyInt_FromLong(self->arrow.type);
  else if (!strcmp(attr, "width"))
    return PyFloat_FromDouble(self->arrow.width);
  else if (!strcmp(attr, "length"))
    return PyFloat_FromDouble(self->arrow.length);

  PyErr_SetString(PyExc_AttributeError, attr);
  return NULL;
}

/*
 * SetAttr
 */

/* XXX */

/*
 * Repr / _Str
 */
static PyObject *
PyDiaPoint_Str(PyDiaPoint *self)
{
    PyObject* py_s;
    gchar* s = g_strdup_printf ("(%f,%f)",
                                (float)(self->pt.x), (float)(self->pt.y));
    py_s = PyString_FromString(s);
    g_free(s);
    return py_s;
}

static PyObject *
PyDiaRectangle_Str(PyDiaRectangle *self)
{
    PyObject* py_s;
    gchar* s;
    if (self->is_int)
      s = g_strdup_printf ("((%d,%d),(%d,%d))",
                           (self->r.ri.left), (self->r.ri.top),
                           (self->r.ri.right), (self->r.ri.bottom));
    else
      s = g_strdup_printf ("((%f,%f),(%f,%f))",
                           (float)(self->r.rf.left), (float)(self->r.rf.top),
                           (float)(self->r.rf.right), (float)(self->r.rf.bottom));
    py_s = PyString_FromString(s);
    g_free(s);
    return py_s;
}

static PyObject *
PyDiaBezPoint_Str(PyDiaBezPoint *self)
{
    PyObject* py_s;
#if 0 /* FIXME: this is causing bad crashes. Probably a glib and a Dia problem ... */
    gchar* s = g_strdup_printf ("((%f,%f),(%f,%f),(%f,%f),%s)",
                                (float)(self->bpn.p1.x), (float)(self->bpn.p1.y),
                                (float)(self->bpn.p2.x), (float)(self->bpn.p2.y),
                                (float)(self->bpn.p3.x), (float)(self->bpn.p3.y),
                                (self->bpn.type == BEZ_MOVE_TO ? "MOVE_TO" :
                                  (self->bpn.type == BEZ_LINE_TO ? "LINE_TO" : "CURVE_TO")));
#else
    gchar* s = g_strdup_printf ("%s",
                                (self->bpn.type == BEZ_MOVE_TO ? "MOVE_TO" :
                                  (self->bpn.type == BEZ_LINE_TO ? "LINE_TO" : "CURVE_TO")));
#endif
    py_s = PyString_FromString(s);
    g_free(s);
    return py_s;
}


static PyObject *
PyDiaArrow_Str(PyDiaArrow *self)
{
    PyObject* py_s;
    gchar* s = g_strdup_printf ("(%f,%f, %d)",
                                (float)(self->arrow.width), 
                                (float)(self->arrow.length),
                                (float)(self->arrow.type));
    py_s = PyString_FromString(s);
    g_free(s);
    return py_s;
}

/*
 * Python objetcs
 */
PyTypeObject PyDiaPoint_Type = {
    PyObject_HEAD_INIT(&PyType_Type)
    0,
    "DiaPoint",
    sizeof(PyDiaPoint),
    0,
    (destructor)PyDiaGeometry_Dealloc,
    (printfunc)0,
    (getattrfunc)PyDiaPoint_GetAttr,
    (setattrfunc)0,
    (cmpfunc)PyDiaPoint_Compare,
    (reprfunc)0,
    0,
    0,
    0,
    (hashfunc)PyDiaGeometry_Hash,
    (ternaryfunc)0,
    (reprfunc)PyDiaPoint_Str,
    0L,0L,0L,0L,
    NULL
};

PyTypeObject PyDiaRectangle_Type = {
    PyObject_HEAD_INIT(&PyType_Type)
    0,
    "DiaRectangle",
    sizeof(PyDiaRectangle),
    0,
    (destructor)PyDiaGeometry_Dealloc,
    (printfunc)0,
    (getattrfunc)PyDiaRectangle_GetAttr,
    (setattrfunc)0,
    (cmpfunc)PyDiaRectangle_Compare,
    (reprfunc)0,
    0,
    0,
    0,
    (hashfunc)PyDiaGeometry_Hash,
    (ternaryfunc)0,
    (reprfunc)PyDiaRectangle_Str,
    0L,0L,0L,0L,
    NULL
};

PyTypeObject PyDiaBezPoint_Type = {
    PyObject_HEAD_INIT(&PyType_Type)
    0,
    "DiaBezPoint",
    sizeof(PyDiaBezPoint),
    0,
    (destructor)PyDiaGeometry_Dealloc,
    (printfunc)0,
    (getattrfunc)PyDiaBezPoint_GetAttr,
    (setattrfunc)0,
    (cmpfunc)PyDiaBezPoint_Compare,
    (reprfunc)0,
    0,
    0,
    0,
    (hashfunc)PyDiaGeometry_Hash,
    (ternaryfunc)0,
    (reprfunc)PyDiaBezPoint_Str,
    0L,0L,0L,0L,
    NULL
};

PyTypeObject PyDiaArrow_Type = {
    PyObject_HEAD_INIT(&PyType_Type)
    0,
    "DiaArrow",
    sizeof(PyDiaArrow),
    0,
    (destructor)PyDiaGeometry_Dealloc,
    (printfunc)0,
    (getattrfunc)PyDiaArrow_GetAttr,
    (setattrfunc)0,
    (cmpfunc)PyDiaArrow_Compare,
    (reprfunc)0,
    0,
    0,
    0,
    (hashfunc)PyDiaGeometry_Hash,
    (ternaryfunc)0,
    (reprfunc)PyDiaArrow_Str,
    0L,0L,0L,0L,
    NULL
};
