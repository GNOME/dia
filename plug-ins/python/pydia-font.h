#ifndef PYDIA_FONT_H
#define PYDIA_FONT_H

#include <Python.h>
#include "font.h"

typedef struct {
    PyObject_HEAD
    DiaFont *font;
} PyDiaFont;

extern PyTypeObject PyDiaFont_Type;

PyObject* PyDiaFont_New (DiaFont* font);

#endif
