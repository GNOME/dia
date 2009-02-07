#ifndef PYDIA_IMAGE_H
#define PYDIA_IMAGE_H

#include <Python.h>
#include "dia_image.h"

typedef struct {
    PyObject_HEAD
    DiaImage *image;
} PyDiaImage;

extern PyTypeObject PyDiaImage_Type;

PyObject* PyDiaImage_New (DiaImage *image);

#endif
