#ifndef PYDIA_PROPERTIES_H
#define PYDIA_PROPERTIES_H

#include <Python.h>
#include "object.h"
#include "properties.h"

typedef struct {
    PyObject_HEAD
    Property property; 
} PyDiaProperty;

extern PyTypeObject PyDiaProperty_Type;
PyObject* PyDiaProperty_New (Property* property);

typedef struct {
    PyObject_HEAD
    Object* object;
    int nprops;
} PyDiaProperties;

extern PyTypeObject PyDiaProperties_Type;
PyObject* PyDiaProperties_New (Object* obj);

#endif