#ifndef PYDIA_ERROR_H
#define PYDIA_ERROR_H

#include <Python.h>
#include <glib.h>

typedef struct {
    PyObject_HEAD
    GString* str;
} PyDiaError;

extern PyTypeObject PyDiaError_Type;

PyObject* PyDiaError_New (const char* s, gboolean unbuffered);

#endif