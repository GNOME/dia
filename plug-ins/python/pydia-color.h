#ifndef PYDIA_COLOR_H
#define PYDIA_COLOR_H

#include <Python.h>
#include "color.h"

typedef struct {
    PyObject_HEAD
    Color color;
} PyDiaColor;

extern PyTypeObject PyDiaColor_Type;

PyObject* PyDiaColor_New (Color* color);

#endif
