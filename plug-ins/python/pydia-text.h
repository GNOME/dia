#ifndef PYDIA_TEXT_H
#define PYDIA_TEXT_H

#include <Python.h>
#include "color.h"

typedef struct {
    PyObject_HEAD
    gchar *text_data;
    TextAttributes attr;
} PyDiaText;

extern PyTypeObject PyDiaText_Type;

PyObject* PyDiaText_New (gchar *text_data, TextAttributes *attr);

#endif
