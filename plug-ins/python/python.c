#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <Python.h>
#include <stdio.h>
#include <glib.h>

int
get_version(void)
{
    return 0;
}

void
register_objects(void)
{
    gchar *filename;
    FILE *fp;

    Py_SetProgramName("dia");
    Py_Initialize();

    initdia();
    if (PyErr_Occurred()) {
	PyErr_Print();
	return;
    }

    filename = g_strdup("/home/james/gnomecvs/dia/plug-ins/python/test.py");
    fp = fopen(filename, "r");
    if (fp)
	PyRun_SimpleFile(fp, filename);
    g_free(filename);

    return;
}

void
register_sheets(void)
{
}

