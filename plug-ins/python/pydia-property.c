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

#include <config.h>

#include <glib.h>

#include <pango/pango-attributes.h>

#include "pydia-object.h"
#include "pydia-properties.h"
#include "pydia-geometry.h"
#include "pydia-font.h"
#include "pydia-color.h"
#include "pydia-text.h"

/*#include "propinternals.h" /* include mess: needs tree.h, we don't */
#include "prop_inttypes.h"
#include "prop_geomtypes.h"
#include "prop_attr.h"
#include "prop_text.h"


/*
 * New
 */
PyObject* PyDiaProperty_New (Property* property)
{
  PyDiaProperty *self;
  
  self = PyObject_NEW(PyDiaProperty, &PyDiaProperty_Type);
  if (!self) return NULL;
  
  self->property = property->ops->copy (property);

  return (PyObject *)self;
}

/*
 * Dealloc
 */
static void
PyDiaProperty_Dealloc(PyDiaProperty *self)
{
  self->property->ops->free(self->property);
  PyMem_DEL(self);
}

/*
 * Compare
 */
static int
PyDiaProperty_Compare(PyDiaProperty *self,
                      PyDiaProperty *other)
{
  /* ??? */
  return memcmp(&(self->property), &(other->property), sizeof(Property));
}

/*
 * Hash
 */
static long
PyDiaProperty_Hash(PyObject *self)
{
  return (long)self;
}

/*
 * Still not convinced that this is better than an integral
 * property->type and some casting ...
 * It is trading a straightforward 40 lines switch statement to
 * this nice 'type safe' function mapping (about 125 lines)
 *                                                        --hb
 */
typedef PyObject * (*PyDiaPropGetFunc) (Property*);
typedef int (*PyDiaPropSetFunc) (Property*, PyObject *val);

static PyObject *
PyDia_get_tuple (GPtrArray *pa, PyDiaPropGetFunc subfn)
{
  PyObject* ret;
  int i, num;

  num = pa->len;
  ret = PyTuple_New (num);
  if (ret) {
    for (i = 0; i < num; i++)
      PyTuple_SetItem(ret, i, subfn(g_ptr_array_index(pa,i)));
  }
  else {
    Py_INCREF(Py_None);
    ret = Py_None;
  }
  return ret;
}

static PyObject * PyDia_get_Char (CharProperty *prop) 
{ return PyInt_FromLong(prop->char_data); }
static PyObject * PyDia_get_Bool (BoolProperty *prop) 
{ return PyInt_FromLong(prop->bool_data); }
static PyObject * PyDia_get_Int (IntProperty *prop) 
{ return PyInt_FromLong(prop->int_data); }
static PyObject * PyDia_get_IntArray (IntarrayProperty *prop) 
{
  PyObject *ret;
  int i, num;

  num = prop->intarray_data->len;
  ret = PyTuple_New (num);

  for (i = 0; i < num; i++)
    PyTuple_SetItem(ret, i, 
                    PyInt_FromLong(g_array_index(prop->intarray_data, gint, i)));
  return ret;
}
static PyObject * PyDia_get_Enum (EnumProperty *prop) 
{ return PyInt_FromLong(prop->enum_data); }
static PyObject * PyDia_get_LineStyle (LinestyleProperty *prop) 
{ Py_INCREF(Py_None); return Py_None; }
static PyObject * PyDia_get_Real (RealProperty *prop) 
{ return PyFloat_FromDouble(prop->real_data); }
static PyObject * PyDia_get_String (StringProperty *prop) 
{ 
  if (NULL == prop->string_data)
    return PyString_FromString("(NULL)");
  else if (1 == prop->num_lines)
    return PyString_FromString(prop->string_data);
  else /* FIXME: MULTISTRING ?  */
    return PyString_FromString(prop->string_data);
}
static PyObject * PyDia_get_StringList (StringListProperty *prop) 
{ 
  GList *tmp;
  PyObject *ret = PyList_New(0);

  for (tmp = prop->string_list; tmp; tmp = tmp->next)
    PyList_Append(ret, PyString_FromString(tmp->data));
  return ret;  
}
static PyObject * PyDia_get_Text (TextProperty *prop)
{ return PyDiaText_New (prop->text_data, &prop->attr); }
static PyObject * PyDia_get_Point (PointProperty *prop) 
{ return PyDiaPoint_New (&prop->point_data); }
static PyObject * PyDia_get_PointArray (PointarrayProperty *prop) 
{
  PyObject *ret;
  int i, num;

  num = prop->pointarray_data->len;
  ret = PyTuple_New (num);

  for (i = 0; i < num; i++)
    PyTuple_SetItem(ret, i, 
                    PyDia_get_Point(&g_array_index(prop->pointarray_data,
                                    PointProperty, i)));
  return ret;
}
static PyObject * PyDia_get_BezPoint (BezPointProperty *prop) 
{ return PyDiaBezPoint_New (&prop->bezpoint_data); }
static PyObject * PyDia_get_BezPointArray (BezPointarrayProperty *prop)
{
  PyObject *ret;
  int i, num;

  num = prop->bezpointarray_data->len;
  ret = PyTuple_New (num);

  for (i = 0; i < num; i++)
    PyTuple_SetItem(ret, i, 
                    PyDia_get_BezPoint(&g_array_index(prop->bezpointarray_data,
                                       BezPointProperty, i)));
  return ret;
}
static PyObject * PyDia_get_Rect (RectProperty *prop)
{ return PyDiaRectangle_New (&prop->rect_data, NULL); }
static PyObject * PyDia_get_Arrow (ArrowProperty *prop)
{ return PyDiaArrow_New (&prop->arrow_data); }
static PyObject * PyDia_get_Color (ColorProperty *prop)
{ return PyDiaColor_New (&prop->color_data); }
static PyObject * PyDia_get_Font (FontProperty *prop)
{ return PyDiaFont_New (prop->font_data); }

struct {
  char *type;
  PyObject *(*propget)();
  GQuark quark;
} prop_type_map [] =
{
  { PROP_TYPE_CHAR, PyDia_get_Char },
  { PROP_TYPE_BOOL, PyDia_get_Bool },
  { PROP_TYPE_INT,  PyDia_get_Int },
  { PROP_TYPE_INTARRAY, PyDia_get_IntArray },
  { PROP_TYPE_ENUM, PyDia_get_Enum },
  { PROP_TYPE_ENUMARRAY, PyDia_get_IntArray }, /* Enum == Int */
  { PROP_TYPE_LINESTYLE, PyDia_get_LineStyle },
  { PROP_TYPE_REAL, PyDia_get_Real },
  { PROP_TYPE_STRING, PyDia_get_String },
  { PROP_TYPE_STRINGLIST, PyDia_get_StringList },
  { PROP_TYPE_FILE, PyDia_get_String },
  { PROP_TYPE_MULTISTRING, PyDia_get_String },
  { PROP_TYPE_TEXT, PyDia_get_Text },
  { PROP_TYPE_POINT, PyDia_get_Point },
  { PROP_TYPE_POINTARRAY, PyDia_get_PointArray },
  { PROP_TYPE_BEZPOINT, PyDia_get_BezPoint },
  { PROP_TYPE_BEZPOINTARRAY, PyDia_get_BezPointArray },
  { PROP_TYPE_RECT, PyDia_get_Rect },
  { PROP_TYPE_ARROW, PyDia_get_Arrow },
  { PROP_TYPE_COLOUR, PyDia_get_Color },
  { PROP_TYPE_FONT, PyDia_get_Font }
};

/*
 * GetAttr
 */
static PyObject *
PyDiaProperty_GetAttr(PyDiaProperty *self, gchar *attr)
{
  static gboolean type_quarks_calculated = FALSE;

  if (!strcmp(attr, "__members__"))
    return Py_BuildValue("[sss]", "name", "type", "value", "visible");
  else if (!strcmp(attr, "name"))
    return PyString_FromString(self->property->name);
  else if (!strcmp(attr, "type"))
    return PyString_FromString(self->property->type);
  else if (!strcmp(attr, "visible"))
    return PyInt_FromLong(0 != (self->property->descr->flags & PROP_FLAG_VISIBLE));
  else if (!strcmp(attr, "value")) {
    int i;

    if (!type_quarks_calculated) {
      for (i = 0; i < G_N_ELEMENTS(prop_type_map); i++) {
        prop_type_map[i].quark = g_quark_from_string (prop_type_map[i].type);
      }
      type_quarks_calculated = TRUE;
    }

    for (i = 0; i < G_N_ELEMENTS(prop_type_map); i++)
      if (prop_type_map[i].quark == self->property->type_quark)
        return prop_type_map[i].propget(self->property);
    if (0 == PROP_FLAG_WIDGET_ONLY & self->property->descr->flags)
      g_warning ("No handler for type '%s'", self->property->type);

    Py_INCREF(Py_None);
    return Py_None;
  }

  PyErr_SetString(PyExc_AttributeError, attr);
  return NULL;
}

/*
 * Similar to SetAttr but the property is directly applied
 * to the DiaObject
 */
int PyDiaProperty_ApplyToObject (Object   *object, 
                                 gchar    *key, 
                                 Property *prop, 
                                 PyObject *val)
{
  /* XXX: implement by means of prop_type_map or ... */
  PyObject *obj = NULL;
  int ret = -1;

  if PyDiaProperty_Check(val) {
    /* must be a Property object ? Or PyDiaRect etc ? */
    Property* inprop = ((PyDiaProperty*)val)->property;

    if (0 == strcmp (prop->type, inprop->type)) 
      {
        GPtrArray *plist;
        /* apply it */
        prop->ops->free (prop); /* release this one */
        prop = inprop->ops->copy(inprop);
        /* apply property to object */
        plist = prop_list_from_single (prop);
        object->ops->set_props(object, plist);
        prop_list_free (plist);

        return 0;
      }
    /* XXX: conversions ??? */
  } else if (PyString_Check (val)) {
    gchar    *str = PyString_AsString (val);

    if (   0 == strcmp (PROP_TYPE_STRING, prop->type)
        || 0 == strcmp (PROP_TYPE_FILE, prop->type)
        || 0 == strcmp (PROP_TYPE_MULTISTRING, prop->type)) 
      {
        StringProperty *p = (StringProperty *)prop;
        g_free (p->string_data);
        p->string_data = g_strdup (str);
        p->num_lines = 1;
        ret = 0; /* everything fine */
      } 
    else if (0 == strcmp (PROP_TYPE_TEXT, prop->type)) 
      {
        TextProperty *p = (TextProperty *)prop;
        g_free (p->text_data);
        p->text_data = g_strdup (str);
        /* XXX: update size calculation ? */
        ret = 0; /* got it */
      }
    else if (0 == strcmp (PROP_TYPE_COLOUR, prop->type))
      {
        PangoColor color;
        if (pango_color_parse(&color, str))
          {
            ColorProperty *p = (ColorProperty*)prop;
            p->color_data.red = color.red / 65535.0; 
            p->color_data.green = color.green / 65535.0; 
            p->color_data.blue = color.blue / 65535.0;
            ret = 0;
          }
        else
          g_warning("Failed to parse color string '%s'", str);
      }
  } else if (PyFloat_Check (val)) {
    if (0 == strcmp(PROP_TYPE_REAL, prop->type))
      {
        RealProperty *p = (RealProperty*)prop;
        p->real_data = PyFloat_AsDouble(val);
        ret = 0;
      }
    else if (0 == strcmp(PROP_TYPE_INT, prop->type))
      {
        IntProperty *p = (IntProperty*)prop;
        p->int_data = (int)PyFloat_AsDouble(val);
        ret = 0;
      }
  } else if (PyInt_Check(val)) {
    if (0 == strcmp(PROP_TYPE_BOOL, prop->type))
      {
        BoolProperty *p = (BoolProperty*)prop;
        p->bool_data = !!PyInt_AsLong(val);
        ret = 0;
      }
    else if (0 == strcmp(PROP_TYPE_ENUM, prop->type))
      {
        EnumProperty *p = (EnumProperty*)prop;
        p->enum_data = PyInt_AsLong(val);
        ret = 0;
      }
    else if (0 == strcmp(PROP_TYPE_INT, prop->type))
      {
        IntProperty *p = (IntProperty*)prop;
        p->int_data = (int)PyInt_AsLong(val);
        ret = 0;
      }
  } else if (PyTuple_Check (val)) {
    int i, len = PyTuple_Size(val);
    double *p = g_new(real, len);

    for (i = 0; i < len; i++)
      {
         PyObject *o = PyTuple_GetItem(val, i);
         if (PyInt_Check(o))
           p[i] = PyInt_AsLong(o);
         else if (PyFloat_Check(o))
           p[i] = PyFloat_AsDouble(o);
         else
           {
             g_warning("Tuple(%d) with type '%s' ignoring",
                       i, ((PyTypeObject*)PyObject_Type(o))->tp_name);
             p[i] = 0.0;
           }
      }
    if (2 >= len && 0 == strcmp (PROP_TYPE_POINT, prop->type))
      {
        PointProperty *ptp = (PointProperty*)prop;
        ptp->point_data.x = p[0];
        ptp->point_data.y = p[1];
        ret = 0;
      }
    else if (3 == len && 0 == strcmp(PROP_TYPE_COLOUR, prop->type))
      {
        ColorProperty *ctp = (ColorProperty*)prop;
        ctp->color_data.red   = p[0];
        ctp->color_data.green = p[1];
        ctp->color_data.blue  = p[2];
        ret = 0;
      }
    else if (4 >= len && 0 == strcmp(PROP_TYPE_RECT, prop->type))
      {
        RectProperty* rtp = (RectProperty*)prop;
        rtp->rect_data.left   = p[0];
        rtp->rect_data.top    = p[1];
        rtp->rect_data.right  = p[2];
        rtp->rect_data.bottom = p[3];
        ret = 0;
      }
    else
      {
         //? PROP_TYPE_POINTARRAY, PROP_TYPE_BEZPOINTARRAY, ...
         g_warning("PyDiaProperty_ApplyToObject : no tuple conversion %s -> %s",
	    key, prop->type);
     }
    g_free (p);
  } else if PyList_Check(val) {
    int i, len = PyList_Size(val);

    if (0 == strcmp(PROP_TYPE_POINTARRAY, prop->type))
      {
        PointarrayProperty *ptp = (PointarrayProperty *)prop;
        Point pt;
        g_array_set_size(ptp->pointarray_data,len);
        for (i = 0; i < len; i++)
          {
            PyObject *o = PyList_GetItem(val, i);

            pt.x = PyFloat_AsDouble(PyTuple_GetItem(o, 0));
            pt.y = PyFloat_AsDouble(PyTuple_GetItem(o, 1));
            g_array_index(ptp->pointarray_data,Point,i) = pt;
          }
        ret = 0;
      }
    else if (0 == strcmp(PROP_TYPE_BEZPOINTARRAY, prop->type))
      {
        BezPointarrayProperty *ptp = (BezPointarrayProperty *)prop;
        BezPoint bpt;
        g_array_set_size(ptp->bezpointarray_data,len);
        for (i = 0; i < len; i++)
          {
            /* a tuple of at least (int,double,double) */
            PyObject *o = PyList_GetItem(val, i);
            int tp = PyInt_AsLong(PyTuple_GetItem(o, 0));

            bpt.p1.x = PyFloat_AsDouble(PyTuple_GetItem(o, 1));
            bpt.p1.y = PyFloat_AsDouble(PyTuple_GetItem(o, 2));
            if (BEZ_CURVE_TO == tp)
              {
                 bpt.type = BEZ_CURVE_TO;
                 bpt.p2.x = PyFloat_AsDouble(PyTuple_GetItem(o, 3));
                 bpt.p2.y = PyFloat_AsDouble(PyTuple_GetItem(o, 4));
                 bpt.p3.x = PyFloat_AsDouble(PyTuple_GetItem(o, 5));
                 bpt.p3.y = PyFloat_AsDouble(PyTuple_GetItem(o, 6));
              }
            else
              {
                 if (0 == i && tp != BEZ_MOVE_TO)
                   g_warning("First bezpoint must be BEZ_MOVE_TO");
                 if (0 < i && tp != BEZ_LINE_TO)
                   g_warning("Further bezpoint must be BEZ_LINE_TO or BEZ_CURVE_TO");

                 bpt.type = (0 == i) ? BEZ_MOVE_TO : BEZ_LINE_TO;
                 /* not strictly needed */
                 bpt.p2 = bpt.p3 = bpt.p1;
              }

            g_array_index(ptp->bezpointarray_data,BezPoint,i) = bpt;
          }
        ret = 0;
      }
  } else {
    g_warning("PyDiaProperty_ApplyToObject : no conversion %s -> %s",
	key, prop->type);
  }

  if (0 == ret) {
    /* apply property to object */
    GPtrArray *plist = prop_list_from_single (prop);
    object->ops->set_props(object, plist);
    prop_list_free (plist);
  }

  return ret;
}

/*
 * Repr / _Str
 */
static PyObject *
PyDiaProperty_Str(PyDiaProperty *self)
{
  PyObject* py_s;
  gchar* tname = "OTHER";
  gchar* s;

  s = g_strdup_printf("<DiaProperty at 0x%08x, \"%s\", %s>",
                      self,
                      self->property->name,
                      self->property->type);

  py_s = PyString_FromString(s);
  g_free (s);
  return py_s;
}

/*
 * Python object
 */
PyTypeObject PyDiaProperty_Type = {
    PyObject_HEAD_INIT(&PyType_Type)
    0,
    "DiaProperty",
    sizeof(PyDiaProperty),
    0,
    (destructor)PyDiaProperty_Dealloc,
    (printfunc)0,
    (getattrfunc)PyDiaProperty_GetAttr,
    (setattrfunc)0,
    (cmpfunc)PyDiaProperty_Compare,
    (reprfunc)0,
    0,
    0,
    0,
    (hashfunc)PyDiaProperty_Hash,
    (ternaryfunc)0,
    (reprfunc)PyDiaProperty_Str,
    0L,0L,0L,0L,
    NULL
};
