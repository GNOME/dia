#ifndef PYDIA_GEOMETRY_H
#define PYDIA_GEOMETRY_H

#include <Python.h>
#include "geometry.h"
#include "arrows.h"

typedef struct {
  PyObject_HEAD

  DiaRectangle r;
} PyDiaRectangle;

extern PyTypeObject PyDiaRectangle_Type;

PyObject *PyDiaRectangle_New (DiaRectangle* r);
PyObject *PyDiaRectangle_New_FromPoints (Point* ul, Point* lr);

typedef struct {
    PyObject_HEAD
    Point pt;
} PyDiaPoint;

extern PyTypeObject PyDiaPoint_Type;

PyObject* PyDiaPoint_New (Point* pt);
PyObject* PyDiaPointTuple_New (Point* pts, int num);

typedef struct {
    PyObject_HEAD
    BezPoint bpn;
} PyDiaBezPoint;

extern PyTypeObject PyDiaBezPoint_Type;

PyObject* PyDiaBezPoint_New (BezPoint* bpn);
PyObject* PyDiaBezPointTuple_New (BezPoint* pts, int num);

typedef struct {
    PyObject_HEAD
    Arrow arrow;
} PyDiaArrow;

extern PyTypeObject PyDiaArrow_Type;

PyObject* PyDiaArrow_New (Arrow* arrow);

typedef struct {
    PyObject_HEAD
    DiaMatrix matrix;
} PyDiaMatrix;

extern PyTypeObject PyDiaMatrix_Type;

PyObject* PyDiaMatrix_New (DiaMatrix* arrow);

#endif
