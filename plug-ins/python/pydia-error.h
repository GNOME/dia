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
void _pyerror_report_last (gboolean popup, const char* fn, const char* file, int line);

#define ON_RES(r,popup) \
if (!r) { \
  _pyerror_report_last (popup, G_STRFUNC, __FILE__, __LINE__); \
} \
else \
  Py_DECREF (r)
      
#endif
