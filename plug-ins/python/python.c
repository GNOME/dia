#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <Python.h>
#include <stdio.h>
#include <glib.h>
#ifdef HAVE_DIRENT_H
#include <dirent.h>
#endif

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

gboolean
dia_py_plugin_can_unload (PluginInfo *info)
{
    return TRUE;
}

void
dia_py_plugin_unload (PluginInfo *info)
{
    /* should call filter_unregister_export () */
    Py_Finalize ();
}

PluginInitResult
dia_plugin_init(PluginInfo *info)
{
    DIR *dp;
    struct dirent *dirp;
    gchar* path;

    if (!dia_plugin_info_init(info, "Python",
			      _("Python scripting support"),
			      dia_py_plugin_can_unload, 
                        dia_py_plugin_unload))
      return DIA_PLUGIN_INIT_ERROR;

    Py_SetProgramName("dia");
    Py_Initialize();
    if (on_error_report())
	  return DIA_PLUGIN_INIT_ERROR;

    initdia();
    if (on_error_report())
	  return DIA_PLUGIN_INIT_ERROR;

#ifdef G_OS_WIN32
    PySys_SetObject("stderr", PyDiaError_New (NULL, TRUE));
#endif
    path = dia_get_lib_directory("dia");
    dp = opendir (path);

    while ((dirp = readdir(dp)) != NULL) {
        gchar *ext = strrchr(dirp->d_name, '.');
        if (ext && 0 == g_strcasecmp(ext, ".py")) {
            gchar *filename = g_strconcat(path, G_DIR_SEPARATOR_S,
    						dirp->d_name, NULL);
            FILE *fp = fopen(filename, "r");
            if (fp)
                PyRun_SimpleFile(fp, filename);
            g_free(filename);

            if (on_error_report())
                continue;
        }
    }
    closedir(dp);
    g_free (path);

    return DIA_PLUGIN_INIT_OK;
}
