#ifndef PYDIA_DIAGRAMDATA_H
#define PYDIA_DIAGRAMDATA_H

#include <Python.h>
#include "diagramdata.h"

typedef struct {
    PyObject_HEAD
    DiagramData* data;
} PyDiaDiagramData;

extern PyTypeObject PyDiaDiagramData_Type;

PyObject *PyDiaDiagramData_New(DiagramData *dd);

#endif
