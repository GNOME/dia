#ifndef PYDIA_PAPERINFO_H
#define PYDIA_PAPERINFO_H

#include <Python.h>
#include "paper.h"

typedef struct {
    PyObject_HEAD
    const PaperInfo *paper;
} PyDiaPaperinfo;


extern PyTypeObject PyDiaPaperinfo_Type;

PyObject *PyDiaPaperinfo_New(const PaperInfo *layer);

#endif /* PYDIA_PAPERINFO_H */
