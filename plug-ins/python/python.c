#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <Python.h>
#include <stdio.h>
#include <glib.h>

#include "intl.h"
#include "plug-ins.h"

DIA_PLUGIN_CHECK_INIT

void initdia(void);

PluginInitResult
dia_plugin_init(PluginInfo *info)
{
    gchar *filename;
    FILE *fp;

    if (!dia_plugin_info_init(info, "Python",
			      _("Python scripting support"),
			      NULL, NULL))
      return DIA_PLUGIN_INIT_ERROR;

    Py_SetProgramName("dia");
    Py_Initialize();

    initdia();
    if (PyErr_Occurred()) {
	PyErr_Print();
	return DIA_PLUGIN_INIT_ERROR;
    }

    filename = g_strdup("/home/james/gnomecvs/dia/plug-ins/python/test.py");
    fp = fopen(filename, "r");
    if (fp)
	PyRun_SimpleFile(fp, filename);
    g_free(filename);

    return DIA_PLUGIN_INIT_OK;
}

