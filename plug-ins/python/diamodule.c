/* -*- Mode: C; c-basic-offset: 4 -*- */
/* Python plug-in for dia
 * Copyright (C) 1999  James Henstridge
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

#include <config.h>

#include <Python.h>
#include <locale.h>

#include "pydia-diagram.h"
#include "pydia-display.h"
#include "pydia-layer.h"
#include "pydia-object.h"
#include "pydia-cpoint.h"
#include "pydia-handle.h"
#include "pydia-export.h"
#include "pydia-geometry.h"
#include "pydia-diagramdata.h"
#include "pydia-font.h"
#include "pydia-color.h"
#include "pydia-image.h"
#include "pydia-properties.h"
#include "pydia-error.h"
#include "pydia-text.h"
#include "pydia-paperinfo.h"
#include "pydia-menuitem.h"
#include "pydia-sheet.h"

#include "lib/dialib.h"
#include "lib/object.h"
#include "lib/group.h"
#include "app/diagram.h"
#include "app/display.h"
#include "app/load_save.h"

#include "lib/message.h"

#include "lib/plug-ins.h"

static PyObject *
PyDia_GroupCreate(PyObject *self, PyObject *args)
{
    int i, len;
    GList *list = NULL;
    PyObject *lst;
    PyObject *ret;

    if (!PyArg_ParseTuple(args, "O!:dia.group_create",
                          &PyList_Type, &lst))
	return NULL;

    len = PyList_Size(lst);
    for (i = 0; i < len; i++)
      {
        PyObject *o = PyList_GetItem(lst, i);

        if (0 && !PyDiaObject_Check(o))
          {
            PyErr_SetString(PyExc_TypeError, "Only DiaObjects can be grouped.");
            g_list_free(list);
            return NULL;
          }

        list = g_list_append(list, ((PyDiaObject *)o)->object);
      }

    if (list)
      ret = PyDiaObject_New(group_create(list));
    else
      {
        Py_INCREF(Py_None);
        ret = Py_None;
      }
    /* Urgh : group_create() eats list */
    /* NOT: g_list_free(list); */

    return ret;
}

static PyObject *
PyDia_Diagrams(PyObject *self, PyObject *args)
{
    GList *tmp;
    PyObject *ret;

    if (!PyArg_ParseTuple(args, ":dia.diagrams"))
	return NULL;
    ret = PyList_New(0);
    for (tmp = dia_open_diagrams(); tmp; tmp = tmp->next)
	PyList_Append(ret, PyDiaDiagram_New((Diagram *)tmp->data));
    return ret;
}

static PyObject *
PyDia_Load(PyObject *self, PyObject *args)
{
    gchar *filename;
    Diagram *dia;

    if (!PyArg_ParseTuple(args, "s:dia.load", &filename))
	return NULL;
    dia = diagram_load(filename, NULL);
    if (dia)
	return PyDiaDiagram_New(dia);
    PyErr_SetString(PyExc_IOError, "could not load diagram");
    return NULL;
}

static PyObject *
PyDia_New(PyObject *self, PyObject *args)
{
    Diagram *dia;
    gchar *filename;

    if (!PyArg_ParseTuple(args, "s:dia.new", &filename))
	return NULL;

    dia = new_diagram(filename);
    if (dia)
	return PyDiaDiagram_New(dia);
    PyErr_SetString(PyExc_IOError, "could not create diagram");
    return NULL;
}

static PyObject *
PyDia_GetObjectType(PyObject *self, PyObject *args)
{
    gchar *name;
    DiaObjectType *otype;

    if (!PyArg_ParseTuple(args, "s:dia.get_object_type", &name))
	return NULL;
    otype = object_get_type(name);
    if (otype)
	return PyDiaObjectType_New(otype);
    PyErr_SetString(PyExc_KeyError, "unknown object type");
    return NULL;
}

/*
 * A dictionary interface to all registered object(-types)
 */
static void
_ot_item (gpointer key,
          gpointer value,
          gpointer user_data)
{
    gchar *name = (gchar *)key;
    DiaObjectType *type = (DiaObjectType *)value;
    PyObject *dict = (PyObject *)user_data;
    PyObject *k, *v;

    k = PyString_FromString(name);
    v = PyDiaObjectType_New(type);
    if (k && v)
        PyDict_SetItem(dict, k, v);
    Py_XDECREF(k);
    Py_XDECREF(v);
}

static PyObject *
PyDia_RegisterPlugin(PyObject *self, PyObject *args)
{
    gchar *filename;

    if (!PyArg_ParseTuple(args, "s:dia.register_plugin", &filename))
	return NULL;
    dia_register_plugin (filename);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
PyDia_RegisteredTypes(PyObject *self, PyObject *args)
{
    PyObject *dict;

    if (!PyArg_ParseTuple(args, ":dia.registered_types"))
	return NULL;

    dict = PyDict_New();

    object_registry_foreach(_ot_item, dict);

    return dict;
}

static PyObject *
PyDia_RegisteredSheets(PyObject *self, PyObject *args)
{
    PyObject *list;
    GSList *items;

    if (!PyArg_ParseTuple(args, ":dia.registered_sheets"))
	return NULL;

    list = PyList_New(0);

    for (items = get_sheets_list (); items != NULL; items = items->next)
	PyList_Append (list, PyDiaSheet_New (items->data));

    return list;
}

static PyObject *
PyDia_ActiveDisplay(PyObject *self, PyObject *args)
{
    DDisplay *disp;

    if (!PyArg_ParseTuple(args, ":dia.active_display"))
	return NULL;
    disp = ddisplay_active();
    if (disp)
	return PyDiaDisplay_New(disp);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
PyDia_UpdateAll(PyObject *self, PyObject *args)
{
    GList *tmp;

    if (!PyArg_ParseTuple(args, ":dia.update_all"))
	return NULL;
    for (tmp = dia_open_diagrams(); tmp; tmp = tmp->next)
	diagram_add_update_all((Diagram *)tmp->data);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
PyDia_RegisterExport(PyObject *self, PyObject *args)
{
    gchar *name;
    gchar *ext;
    PyObject *obj;
    DiaExportFilter *filter;
    PyObject* renderer;

    if (!PyArg_ParseTuple(args, "ssO:dia.register_export",
			  &name, &ext, &renderer))
	return NULL;

    Py_INCREF(renderer); /* stay alive, where to kill ?? */

    filter = g_new0 (DiaExportFilter, 1);
    filter->description = g_strdup (name);
    /* the following is usually declared as a static const string array, but we can't
     * cause there needs to be one for every export filter defined in Python. Silence gcc. 
     */
    filter->extensions = (const gchar**)g_new (gchar*, 2);
    filter->extensions[0] = g_strdup (ext);
    filter->extensions[1] = NULL;
    filter->export_func = &PyDia_export_data;
    filter->user_data = renderer;
    filter->unique_name = g_strdup_printf ("%s-py", ext);
    filter->hints = FILTER_DONT_GUESS;
    obj = PyDiaExportFilter_New(filter);

    filter_register_export(filter);

    return obj;
}


/*
 * This function gets called by Dia as a reaction to file import.
 * It needs to be registered before via Python function 
 * dia.register_import
 */
static gboolean
PyDia_import_data (const gchar* filename, DiagramData *dia, DiaContext *ctx, void *user_data)
{
    PyObject *diaobj, *arg, *func = user_data;
    char* old_locale;
    gboolean bRet = FALSE;

    if (!func || !PyCallable_Check (func)) {
        dia_context_add_message (ctx, "Import called without valid callback function.");
        return FALSE;
    }
    if (dia)
        diaobj = PyDiaDiagramData_New (dia);
    else {
        diaobj = Py_None;
        Py_INCREF (diaobj);
    }
      
    Py_INCREF(func);

    /* Python tries to guarantee this, make it work for these plugins too */
    old_locale = setlocale(LC_NUMERIC, "C");

    arg = Py_BuildValue ("(sO)", filename, diaobj);
    if (arg) {
      PyObject *res = PyEval_CallObject (func, arg);
      ON_RES(res, TRUE);
      bRet = !!res;
    }
    Py_XDECREF (arg);

    Py_DECREF(func);
    Py_XDECREF(diaobj);

    setlocale(LC_NUMERIC, old_locale);

    return bRet;
}

static PyObject *
PyDia_RegisterImport(PyObject *self, PyObject *args)
{
    gchar *name;
    gchar *ext;
    DiaImportFilter *filter;
    PyObject* func;

    if (!PyArg_ParseTuple(args, "ssO:dia.register_import",
			  &name, &ext, &func))
	return NULL;

    Py_INCREF(func); /* stay alive, where to kill ?? */

    filter = g_new0 (DiaImportFilter, 1);
    filter->description = g_strdup (name);
    /* expects const gchar** but we can't really do that, silence gcc. */
    filter->extensions = (const gchar**)g_new (gchar*, 2);
    filter->extensions[0] = g_strdup (ext);
    filter->extensions[1] = NULL;
    filter->import_func = &PyDia_import_data;
    filter->user_data = func;
    filter->unique_name = g_strdup_printf ("%s-py", ext);
    filter->hints = FILTER_DONT_GUESS;

    filter_register_import(filter);

    Py_INCREF(Py_None);
    return Py_None;
}

/*
 * This function gets called by Dia as a reaction to a menu item.
 * It needs to be registered before via Python function 
 * dia.register_action (or dia.register_callback)
 */
static ObjectChange *
PyDia_callback_func (DiagramData *dia, const gchar *filename, guint flags, void *user_data)
{
    PyObject *diaobj, *res, *arg, *func = user_data;
    if (!func || !PyCallable_Check (func)) {
        g_warning ("Callback called without valid callback function.");
        return NULL;
    }
  
    if (dia)
        diaobj = PyDiaDiagramData_New (dia);
    else {
        diaobj = Py_None;
        Py_INCREF (diaobj);
    }
      
    Py_INCREF(func);

    arg = Py_BuildValue ("(Oi)", diaobj, flags);
    if (arg) {
      res = PyEval_CallObject (func, arg);
      ON_RES(res, TRUE);
    }
    Py_XDECREF (arg);

    Py_DECREF(func);
    Py_XDECREF(diaobj);

    return NULL;
}

static PyObject *
_RegisterAction (gchar *action,
                 gchar *desc,
                 gchar *menupath,
                 PyObject *func);

static gchar *
_strip_non_alphanum (gchar* in)
{
  int i, o;
  int len = strlen (in);
  gchar *out = g_new (gchar, len);
  
  for (i = 0, o = 0; i < len; ++i) {
    if (g_ascii_isalnum (in[i])) {
      out[o] = in[i];
      ++o;
    }
  }
  out[o] = '\0';
  return out;
}

static PyObject *
PyDia_RegisterCallback(PyObject *self, PyObject *args)
{
    gchar *desc;
    gchar *menupath;
    gchar *path;
    PyObject *func;
    gchar *action;
    PyObject *ret;

    if (!PyArg_ParseTuple(args, "ssO:dia.register_callback",
			  &desc, &menupath, &func))
	return NULL;

    /* if root node name does not match : <Display> -> /DisplayMenu */
    if (strstr (menupath, "<Display>") == menupath)
        path = g_strdup_printf ("/DisplayMenu%s", menupath + strlen("<Display>"));
    else if (strstr (menupath, "<Toolbox>") == menupath)
        path = g_strdup_printf ("/ToolboxMenu%s", menupath + strlen("<Toolbox>"));
    else {
        /* no need for g_warning here, we'll get one when entering into the GtkUiManager */
	path = g_strdup (menupath);
    }
    action = _strip_non_alphanum (path);
#if 1
    if (strrchr (path, '/') - path < strlen(path))
      *(strrchr (path, '/')) = '\0';
#endif
    ret = _RegisterAction (action, desc, path, func);
    g_free (path);
    g_free (action);

    return ret;
}

static PyObject *
PyDia_RegisterAction (PyObject *self, PyObject *args)
{
    gchar *action;
    gchar *desc;
    gchar *menupath;
    PyObject *func;

    if (!PyArg_ParseTuple(args, "sssO:dia.register_action",
			  &action, &desc, &menupath, &func))
	return NULL;

    return _RegisterAction (action, desc, menupath, func);
}

static PyObject *
_RegisterAction (gchar *action,
                 gchar *desc,
                 gchar *menupath,
                 PyObject *func)
{
	DiaCallbackFilter *filter;

    if (!PyCallable_Check(func)) {
        PyErr_SetString(PyExc_TypeError, "third parameter must be callable");
        return NULL;
    }

    Py_INCREF(func); /* stay alive, where to kill ?? */

    filter = g_new0 (DiaCallbackFilter, 1);
    filter->action = g_strdup (action);
    filter->description = g_strdup (desc);
    filter->menupath = g_strdup (menupath);
    filter->callback = &PyDia_callback_func;
    filter->user_data = func;

    filter_register_callback(filter);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
PyDia_Message (PyObject *self, PyObject *args)
{
    int type = 0;
    char *text = "Huh?";

    if (!PyArg_ParseTuple(args, "is:dia.message",
			  &type, &text))
	return NULL;

    if (0 == type)
	message_notice ("%s", text);
    else if (1 == type)
	message_warning ("%s", text);
    else
	message_error ("%s", text);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyMethodDef dia_methods[] = {
    { "group_create", PyDia_GroupCreate, METH_VARARGS,
      "group_create(List of Object: objs) -> Object."
      "  Create a group containing the given list of dia.Object(s)" },
    { "diagrams", PyDia_Diagrams, METH_VARARGS,
      "diagrams() -> List of Diagram.  Returns the list of currently open diagrams" },
    { "load", PyDia_Load, METH_VARARGS,
      "load(string: name) -> Diagram.  Loads a diagram from the given filename" },
    { "message", PyDia_Message, METH_VARARGS,
      "message(int: type, string: msg) -> None.  Popup a dialog with given message" },
    { "new", PyDia_New, METH_VARARGS,
      "new(string: name) -> Diagram.  Create an empty diagram" },
    { "get_object_type", PyDia_GetObjectType, METH_VARARGS,
      "get_object_type(string: type) -> ObjectType."
      "  From a type name like \"Standard - Line\" return the factory to create objects of that type, see: DiaObjectType" },
    { "registered_types", PyDia_RegisteredTypes, METH_VARARGS,
      "registered_types() -> Dict of ObjectType indexed by their name."
      "  A dictionary of all registered object factories, aka. DiaObjectType" },
    { "registered_sheets", PyDia_RegisteredSheets, METH_VARARGS,
      "registered_sheets() -> List of registered sheets."
      "  A list of all registered sheets." },
    { "active_display", PyDia_ActiveDisplay, METH_VARARGS,
      "active_display() -> Display.  Delivers the currently active display 'dia.Display' or None" },
    { "update_all", PyDia_UpdateAll, METH_VARARGS,
      "update_all() -> None.  Force an asynchronous update of all existing diagram displays" },
    { "register_export", PyDia_RegisterExport, METH_VARARGS,
      "register_export(string: name, string: extension, Renderer: r) -> None."
      "  Allows to register an export filter written in Python. It needs to conform to the DiaRenderer interface." },
    { "register_import", PyDia_RegisterImport, METH_VARARGS,
      "register_import(string: name, string: extension, Callback: func) -> None."
      "  Allows to register an import filter written in Python, that is mainly a callback function which fills the"
      "given DiaDiagramData from the given filename" },
    { "register_callback", PyDia_RegisterCallback, METH_VARARGS,
      "register_callback(string: description, string: menupath, Callback: func) -> None."
      "  Register a callback function which appears in the menu. Depending on the menu path used during registration"
      "the callback gets called with the current DiaDiagramData object" },
    { "register_action", PyDia_RegisterAction, METH_VARARGS,
      "register_action(string: action, string: description, string: menupath, Callback: func) -> None."
      "  Register a callback function which appears in the menu. Depending on the menu path used during registration"
      "the callback gets called with the current DiaDiagramData object" },
    { "register_plugin", PyDia_RegisterPlugin, METH_VARARGS,
      "register_plugin(string: filename) -> None."
      "  Registers a single plug-in given its filename, that is load a dynamic module." },
    { NULL, NULL }
};

PyDoc_STRVAR(dia_module_doc,
"The dia module allows to write Python plug-ins for Dia [http://live.gnome.org/Dia/Python]\n"
"\n"
"This modules is designed to run Python scripts embedded in Dia. To make your script accessible\n"
"to Dia you have to put it into $HOME/.dia/python and let it call one of the register_*() functions.\n"
"It is possible to write import filters [register_import()] and export filters [register_export()], "
"as well as scripts to manipulate existing diagrams or create new ones [register_action()].\n"
"\n"
"For stand-alone Python bindings to Dia see http://mail.gnome.org/archives/dia-list/2007-March/msg00092.html");

DL_EXPORT(void) initdia(void);

DL_EXPORT(void)
initdia(void)
{
    PyObject *m, *d;

    /* see: Python FAQ 3.24 "Initializer not a constant." */
    /* https://docs.python.org/2/c-api/typeobj.html#c.PyObject.ob_type */
    PyDiaConnectionPoint_Type.ob_type = &PyType_Type;
    PyDiaDiagram_Type.ob_type = &PyType_Type;
    PyDiaDisplay_Type.ob_type = &PyType_Type;
    PyDiaHandle_Type.ob_type = &PyType_Type;
    PyDiaLayer_Type.ob_type = &PyType_Type;
    PyDiaObject_Type.ob_type = &PyType_Type;
    PyDiaObjectType_Type.ob_type = &PyType_Type;

    PyDiaExportFilter_Type.ob_type = &PyType_Type;
    PyDiaDiagramData_Type.ob_type = &PyType_Type;

    PyDiaPoint_Type.ob_type = &PyType_Type;
    PyDiaRectangle_Type.ob_type = &PyType_Type;
    PyDiaBezPoint_Type.ob_type = &PyType_Type;
    PyDiaArrow_Type.ob_type = &PyType_Type;
    PyDiaMatrix_Type.ob_type = &PyType_Type;

    PyDiaFont_Type.ob_type = &PyType_Type;
    PyDiaColor_Type.ob_type = &PyType_Type;
    PyDiaImage_Type.ob_type = &PyType_Type;
    PyDiaProperty_Type.ob_type = &PyType_Type;
    PyDiaProperties_Type.ob_type = &PyType_Type;
    PyDiaError_Type.ob_type = &PyType_Type;
    PyDiaText_Type.ob_type = &PyType_Type;
    PyDiaPaperinfo_Type.ob_type = &PyType_Type;
    PyDiaMenuitem_Type.ob_type = &PyType_Type;
    PyDiaSheet_Type.ob_type = &PyType_Type;

    m = Py_InitModule3("dia", dia_methods, dia_module_doc);
    d = PyModule_GetDict(m);

    /* 
     * The postfix 'Type' should not be there (one obvious exception, but there
     * it isn't a Postfix). That is: names here and in the respective 
     * PyTypeObject must match.
     * The extra namespacing (prefix 'Dia') isn't necessary either, we use the
     * pythonesque namespacing instead.
     */
    /* instead to cast to PyObject* we use void* just to silence the ugly
     * warning: dereferencing type-punned pointer will break strict-aliasing rules
     */
    PyDict_SetItemString(d, "Diagram",
			 (void *)&PyDiaDiagram_Type);
    PyDict_SetItemString(d, "Display",
			 (void *)&PyDiaDisplay_Type);
    PyDict_SetItemString(d, "Layer",
			 (void *)&PyDiaLayer_Type);
    PyDict_SetItemString(d, "Object",
			 (void *)&PyDiaObject_Type);
    PyDict_SetItemString(d, "ObjectType",
			 (void *)&PyDiaObjectType_Type);
    PyDict_SetItemString(d, "ConnectionPoint",
			 (void *)&PyDiaConnectionPoint_Type);
    PyDict_SetItemString(d, "Handle",
			 (void *)&PyDiaHandle_Type);
    PyDict_SetItemString(d, "ExportFilter",
			 (void *)&PyDiaExportFilter_Type);
    PyDict_SetItemString(d, "DiagramData",
			 (void *)&PyDiaDiagramData_Type);
    PyDict_SetItemString(d, "Point",
			 (void *)&PyDiaPoint_Type);
    PyDict_SetItemString(d, "Rectangle",
			 (void *)&PyDiaRectangle_Type);
    PyDict_SetItemString(d, "BezPoint",
			 (void *)&PyDiaBezPoint_Type);
    PyDict_SetItemString(d, "Font",
			 (void *)&PyDiaFont_Type);
    PyDict_SetItemString(d, "Color",
			 (void *)&PyDiaColor_Type);
    PyDict_SetItemString(d, "Image",
			 (void *)&PyDiaImage_Type);
    PyDict_SetItemString(d, "Property",
			 (void *)&PyDiaProperty_Type);
    PyDict_SetItemString(d, "Properties",
			 (void *)&PyDiaProperties_Type);
    PyDict_SetItemString(d, "Error",
			 (void *)&PyDiaError_Type);
    PyDict_SetItemString(d, "Arrow",
			 (void *)&PyDiaArrow_Type);
    PyDict_SetItemString(d, "Matrix",
			 (void *)&PyDiaMatrix_Type);
    PyDict_SetItemString(d, "Text",
			 (void *)&PyDiaText_Type);
    PyDict_SetItemString(d, "Paperinfo",
			 (void *)&PyDiaPaperinfo_Type);
    PyDict_SetItemString(d, "Menuitem",
			 (void *)&PyDiaMenuitem_Type);
    PyDict_SetItemString(d, "Sheet",
			 (void *)&PyDiaSheet_Type);

    if (PyErr_Occurred())
	Py_FatalError("can't initialize module dia");
    else {
      /* should all be no-ops when used embedded */
#if !GLIB_CHECK_VERSION(2,36,0)
      g_type_init ();
#endif
      libdia_init (DIA_MESSAGE_STDERR);
    }
}
