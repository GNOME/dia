#ifndef PYDIA_MENUITEM_H
#define PYDIA_MENUITEM_H

#include <Python.h>
#include "object.h"

typedef struct {
    PyObject_HEAD
    const DiaMenuItem *menuitem;
} PyDiaMenuitem;


extern PyTypeObject PyDiaMenuitem_Type;

PyObject *PyDiaMenuitem_New(const DiaMenuItem *layer);

#endif /* PYDIA_MENUITEM_H */
