#ifndef PYDIA_COLOR_H
#define PYDIA_COLOR_H

#include <Python.h>

typedef struct {
    PyObject_HEAD
    GdkRGBA color;
} PyDiaColor;

extern PyTypeObject PyDiaColor_Type;

PyObject* PyDiaColor_New (GdkRGBA* color);

#endif
