#ifndef PYDIA_EXPORT_H
#define PYDIA_EXPORT_H

#include <Python.h>

#include "filter.h"

typedef struct {
    PyObject_HEAD
    DiaExportFilter *filter;
} PyDiaExportFilter;


extern PyTypeObject PyDiaExportFilter_Type;

PyObject *PyDiaExportFilter_New(DiaExportFilter *filter);

/* first callback for file exports */
gboolean PyDia_export_data(DiagramData *data, DiaContext *ctx,
			   const gchar *filename, const gchar *diafilename, void *user_data);

#endif
