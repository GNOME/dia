/* Python plug-in for dia
 * Copyright (C) 1999  James Henstridge
 * Copyright (C) 2000  Hans Breuer
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <Python.h>
#include <stdio.h>
#include <glib.h>

#include "intl.h"
#include "plug-ins.h"
#include "dia_dirs.h"
#include "message.h"

#include "pydia-error.h"

DIA_PLUGIN_CHECK_INIT

void initdia(void);

static gboolean
on_error_report (void)
{
    if (PyErr_Occurred()) {
#ifndef G_OS_WIN32
        PyErr_Print();
#else
        /* On Win32 there is not necessary a console attached to the
         * program, so delegate the stack dump to the error object,
         * which finally uses g_print()
         */
        PyObject *exc, *v, *tb, *ef;
        PyErr_Fetch (&exc, &v, &tb);
        ef = PyDiaError_New ("Initialization Error:", FALSE);
        PyFile_WriteObject (exc, ef, 0);
        PyFile_WriteObject (v, ef, 0);
        PyTraceBack_Print(tb, ef);
        Py_DECREF (ef);

        Py_XDECREF (exc);
        Py_XDECREF (v);
        Py_XDECREF (tb);
#endif
        return TRUE;
    }
    else
        return FALSE; 
}

static gboolean
dia_py_plugin_can_unload (PluginInfo *info)
{
    /* until we can properly clean up the python plugin, it is not safe to
     * unload it from a dia */
    return FALSE; /* TRUE */
}

static void
dia_py_plugin_unload (PluginInfo *info)
{
    /* should call filter_unregister_export () */
    Py_Finalize ();
}

PluginInitResult
dia_plugin_init(PluginInfo *info)
{
    gchar *python_argv[] = { "dia-python", NULL };
    gchar *startup_file;
    FILE *fp;
    PyObject *__main__, *__file__;

    if (!dia_plugin_info_init(info, "Python",
			      _("Python scripting support"),
			      dia_py_plugin_can_unload, 
			      dia_py_plugin_unload))
	return DIA_PLUGIN_INIT_ERROR;

    Py_SetProgramName("dia");
    Py_Initialize();

    PySys_SetArgv(1, python_argv);

    if (on_error_report())
	return DIA_PLUGIN_INIT_ERROR;

    initdia();
    if (on_error_report())
	return DIA_PLUGIN_INIT_ERROR;

#ifdef G_OS_WIN32
    PySys_SetObject("stderr", PyDiaError_New (NULL, TRUE));
#endif

    startup_file = dia_get_data_directory("python-startup.py");
    if (!startup_file) {
	g_warning("could not find python-startup.py");
	return DIA_PLUGIN_INIT_ERROR;
    }

    /* set __file__ in __main__ so that python-startup.py knows where it is */
    __main__ = PyImport_AddModule("__main__");
    __file__ = PyString_FromString(startup_file);
    PyObject_SetAttrString(__main__, "__file__", __file__);
    Py_DECREF(__file__);

    fp = fopen(startup_file, "r");
    if (!fp) {
	g_free(startup_file);
	return DIA_PLUGIN_INIT_ERROR;
    }
    PyRun_SimpleFile(fp, startup_file);
    g_free(startup_file);

    if (on_error_report())
	return DIA_PLUGIN_INIT_ERROR;

    return DIA_PLUGIN_INIT_OK;
}
