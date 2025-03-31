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

#define G_LOG_DOMAIN "PyDia"

#include "config.h"

#include <glib/gi18n-lib.h>

#include <Python.h>
#include <stdio.h>
#include <glib.h>

#include "plug-ins.h"
#include "dia_dirs.h"

#include "pydia.h"
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
dia_plugin_init (PluginInfo *info)
{
  char *python_argv[] = { "dia-python", NULL };
  char *startup_file;
  FILE *fp;
  PyStatus status;
  PyConfig config;
  PyObject *__main__, *__file__;

  if (Py_IsInitialized ()) {
    g_warning ("Dia's Python embedding is not designed for concurrency.");
    return DIA_PLUGIN_INIT_ERROR;
  }

  if (!dia_plugin_info_init (info,
                             "Python",
                             _("Python scripting support"),
                             dia_py_plugin_can_unload,
                             dia_py_plugin_unload)) {
    return DIA_PLUGIN_INIT_ERROR;
  }

  PyImport_AppendInittab ("dia", &PyInit_dia);

  PyConfig_InitPythonConfig (&config);

  status = PyConfig_SetString(&config, &config.program_name, L"dia");
  if (PyStatus_Exception (status)) {
      goto failed;
  }

  status = PyConfig_SetBytesArgv (&config, 1, python_argv);
  if (PyStatus_Exception (status)) {
      goto failed;
  }

  status = Py_InitializeFromConfig (&config);
  if (PyStatus_Exception (status)) {
      goto failed;
  }
  PyConfig_Clear (&config);


  /* Sanitize sys.path */
  PyRun_SimpleString ("import sys; sys.path = list(filter(None, sys.path))");

  if (on_error_report()) {
    return DIA_PLUGIN_INIT_ERROR;
  }

#ifdef G_OS_WIN32
  PySys_SetObject("stderr", PyDiaError_New (NULL, TRUE));
#endif
  if (g_getenv ("DIA_PYTHON_PATH")) {
    startup_file = g_build_filename (g_getenv ("DIA_PYTHON_PATH"), "python-startup.py", NULL);
  } else {
    startup_file = dia_get_data_directory ("python-startup.py");
  }

  if (!startup_file) {
    g_warning ("could not find python-startup.py");
    return DIA_PLUGIN_INIT_ERROR;
  }

  /* set __file__ in __main__ so that python-startup.py knows where it is */
  __main__ = PyImport_AddModule ("__main__");
  __file__ = PyUnicode_FromString (startup_file);
  PyObject_SetAttrString (__main__, "__file__", __file__);
  Py_DECREF (__file__);
#if defined(G_OS_WIN32) && (PY_VERSION_HEX >= 0x02040000)
  /* this code should work for every supported Python version, but it is needed
    * on win32 since Python 2.4 due to mixed runtime issues, i.e.
    * crashing in PyRun_SimpleFile for python2(5|6)/msvcr(71|90)
    * It is not enabled by default yet, because I could not get PyGtk using
    * plug-ins to work at all with 2.5/2.6 */
  {
    char *startup_string = NULL;
    gsize i, length = 0;
    GError *error = NULL;
    if (!g_file_get_contents(startup_file, &startup_string, &length, &error)) {
        g_warning("Python: Couldn't find startup file %s\n%s\n",
            startup_file, error->message);
        g_clear_error (&error);
        g_clear_pointer (&startup_file, g_free);
        return DIA_PLUGIN_INIT_ERROR;
    }
    /* PyRun_SimpleString does not like the windows format */
    for (i = 0; i < length; ++i) {
      if (startup_string[i] == '\r') {
        startup_string[i] = '\n';
      }
    }


    if (PyRun_SimpleString(startup_string) != 0) {
        g_warning("Python: Couldn't run startup file %s\n", startup_file);
    }

    g_clear_pointer (&startup_string, g_free);
  }
#else
  fp = fopen (startup_file, "r");
  if (!fp) {
    g_warning ("Python: Couldn't find startup file %s\n", startup_file);
    g_clear_pointer (&startup_file, g_free);
    return DIA_PLUGIN_INIT_ERROR;
  }
  PyRun_SimpleFile (fp, startup_file);
#endif
  g_clear_pointer (&startup_file, g_free);

  if (on_error_report()) {
    return DIA_PLUGIN_INIT_ERROR;
  }

  return DIA_PLUGIN_INIT_OK;

failed:
  PyConfig_Clear (&config);

  g_critical ("Can't start: %s: %s", status.func, status.err_msg);

  return DIA_PLUGIN_INIT_ERROR;
}
