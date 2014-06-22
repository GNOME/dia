#ifndef PYDIA_SHEET_H
#define PYDIA_SHEET_H

#include <Python.h>
#include "sheet.h"

typedef struct {
    PyObject_HEAD
    Sheet *sheet;
} PyDiaSheet;

extern PyTypeObject PyDiaSheet_Type;

PyObject* PyDiaSheet_New (Sheet* sheet);

#endif
