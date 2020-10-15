#ifndef PYDIA_PROPERTIES_H
#define PYDIA_PROPERTIES_H

#include <Python.h>
#include "lib/object.h"
#include "lib/properties.h"

typedef struct {
    PyObject_HEAD
    Property* property;
} PyDiaProperty;

extern PyTypeObject PyDiaProperty_Type;
PyObject* PyDiaProperty_New (Property* property);

typedef struct {
    PyObject_HEAD
    DiaObject* object;
    int nprops;
} PyDiaProperties;

extern PyTypeObject PyDiaProperties_Type;
PyObject* PyDiaProperties_New (DiaObject* obj);

int PyDiaProperty_ApplyToObject (DiaObject *object, const char *key, Property *prop, PyObject *val);

#define PyDiaProperty_Check(o) ((o)->ob_type == &PyDiaProperty_Type)

#endif
