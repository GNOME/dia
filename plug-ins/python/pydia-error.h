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

#define ON_RES(r) \
if (!r) { \
  PyObject *exc, *v, *tb, *ef; \
  PyErr_Fetch (&exc, &v, &tb); \
  PyErr_NormalizeException(&exc, &v, &tb); \
\
  ef = PyDiaError_New(G_GNUC_PRETTY_FUNCTION " Error:", FALSE); \
  PyFile_WriteObject (exc, ef, 0); \
  PyFile_WriteObject (v, ef, 0); \
  PyTraceBack_Print(tb, ef); \
\
  Py_DECREF (ef); \
  Py_XDECREF(exc); \
  Py_XDECREF(v); \
  Py_XDECREF(tb); \
} \
else \
  Py_DECREF (res)
      
#endif