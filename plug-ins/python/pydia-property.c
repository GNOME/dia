/* Python plug-in for dia
 * Copyright (C) 1999  James Henstridge
 * Copyright (C) 2000-2005  Hans Breuer
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
#include <glib.h>

#include <pango/pango-attributes.h>

#include "pydia-object.h"
#include "pydia-properties.h"
#include "pydia-geometry.h"
#include "pydia-font.h"
#include "pydia-color.h"
#include "pydia-text.h"

#include <structmember.h> /* PyMemberDef */

/* include mess: propinternals.h needs tree.h, we don't */
#include "prop_inttypes.h"
#include "prop_geomtypes.h"
#include "prop_attr.h"
#include "prop_text.h"
#include "prop_sdarray.h"
#include "prop_dict.h"
#include "prop_matrix.h"
#include "prop_pixbuf.h"

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
  PyObject_DEL(self);
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
{ 
  PyObject *ret;

  ret = PyTuple_New (2);
  PyTuple_SetItem(ret, 0, PyInt_FromLong(prop->style));
  PyTuple_SetItem(ret, 1, PyFloat_FromDouble(prop->dash));
  return ret;
}
static PyObject * PyDia_get_Real (RealProperty *prop) 
{ return PyFloat_FromDouble(prop->real_data); }
static PyObject * PyDia_get_Length (LengthProperty *prop) 
{ return PyFloat_FromDouble(prop->length_data); }
static PyObject * PyDia_get_Fontsize (FontsizeProperty *prop) 
{ return PyFloat_FromDouble(prop->fontsize_data); }
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
                    PyDiaPoint_New(&g_array_index(prop->pointarray_data,
                                   Point, i)));
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
                    PyDiaBezPoint_New(&g_array_index(prop->bezpointarray_data,
                                      BezPoint, i)));
  return ret;
}
static PyObject * PyDia_get_Rect (RectProperty *prop)
{ return PyDiaRectangle_New (&prop->rect_data, NULL); }
static PyObject * PyDia_get_Arrow (ArrowProperty *prop)
{ return PyDiaArrow_New (&prop->arrow_data); }
static PyObject * PyDia_get_Matrix (MatrixProperty *prop)
{ return PyDiaMatrix_New (prop->matrix); }
static PyObject * PyDia_get_Color (ColorProperty *prop)
{ return PyDiaColor_New (&prop->color_data); }
static PyObject * PyDia_get_Font (FontProperty *prop)
{ return PyDiaFont_New (prop->font_data); }

static PyObject * PyDia_get_Array (ArrayProperty *prop);

static PyObject * PyDia_get_Dict (DictProperty *prop);
static int PyDia_set_Dict (Property *prop, PyObject *val);

static PyObject * PyDia_get_Pixbuf (PixbufProperty *prop);
static int PyDia_set_Pixbuf (Property *prop, PyObject *val);

static int
PyDia_set_Bool (Property *prop, PyObject *val)
{
  BoolProperty *p = (BoolProperty *)prop;
  if (PyInt_Check(val)) {
    p->bool_data = !!PyInt_AsLong(val);
    return 0;
  }
  return -1;
}
static int
PyDia_set_Int (Property *prop, PyObject *val)
{
  IntProperty *p = (IntProperty *)prop;
  if (PyInt_Check(val)) {
    p->int_data = PyInt_AsLong(val);
    return 0;
  }
  return -1;
}
static int
PyDia_set_IntArray(Property *prop, PyObject *val)
{
  IntarrayProperty *p = (IntarrayProperty *)prop;
  if (PyTuple_Check(val)) {
    int i, len = PyTuple_Size(val);
    g_array_set_size(p->intarray_data, len);
    for (i = 0; i < len; i++) {
      PyObject *o = PyTuple_GetItem(val, i);
      g_array_index(p->intarray_data, gint, i) = PyInt_Check(o) ? PyInt_AsLong(o) : 0;
    }
    return 0;
  } else if (PyList_Check(val)) {
    int i, len = PyList_Size(val);
    g_array_set_size(p->intarray_data, len);
    for (i = 0; i < len; i++) {
      PyObject *o = PyList_GetItem(val, i);
      g_array_index(p->intarray_data, gint, i) = PyInt_Check(o) ? PyInt_AsLong(o) : 0;
    }
    return 0;
  }
  return -1;
}
static int
PyDia_set_Enum (Property *prop, PyObject *val)
{
  EnumProperty *p = (EnumProperty *)prop;
  /* XXX a range check would not hurt */
  if (PyInt_Check(val)) {
    p->enum_data = PyInt_AsLong(val);
    return 0;
  }
  return -1;
}
static int
PyDia_set_Arrow (Property *prop, PyObject *val)
{
  ArrowProperty *p = (ArrowProperty *)prop;
  
  if (val->ob_type == &PyDiaArrow_Type) {
    p->arrow_data = ((PyDiaArrow *)val)->arrow;
    return 0;
  } else if (PyTuple_Check (val)) {
    int len = PyTuple_Size(val);
    PyObject *o;
    if (len < 3)
      return -1;
    if ((o = PyTuple_GetItem(val, 0)) != NULL && PyInt_Check(o))
      p->arrow_data.type = PyInt_AsLong(o);
    else
      p->arrow_data.type = ARROW_NONE;
    if ((o = PyTuple_GetItem(val, 1)) != NULL && PyFloat_Check(o))
      p->arrow_data.length = PyFloat_AsDouble(o);
    else
      p->arrow_data.length = DEFAULT_ARROW_SIZE;
    if ((o = PyTuple_GetItem(val, 2)) != NULL && PyFloat_Check(o))
      p->arrow_data.width = PyFloat_AsDouble(o);
    else
      p->arrow_data.width = DEFAULT_ARROW_SIZE;
    return 0;
  }
  return -1;
}
static int
PyDia_set_Matrix (Property *prop, PyObject *val)
{
  MatrixProperty *p = (MatrixProperty *)prop;
  
  if (val->ob_type == &PyDiaMatrix_Type) {
    if (!p->matrix)
      p->matrix = g_new (DiaMatrix, 1);
    *p->matrix = ((PyDiaMatrix *)val)->matrix;
    return 0;
  }
  return -1;
}
static int
PyDia_set_Color (Property *prop, PyObject *val)
{
  ColorProperty *p = (ColorProperty*)prop;
  if (PyString_Check(val)) {
    gchar *str = PyString_AsString(val);
    PangoColor color;
    if (pango_color_parse(&color, str)) {
      p->color_data.red = color.red / 65535.0; 
      p->color_data.green = color.green / 65535.0; 
      p->color_data.blue = color.blue / 65535.0;
      p->color_data.alpha = 1.0;
      return 0;
    } else
      g_debug("Failed to parse color string '%s'", str);
  } else if (PyTuple_Check (val)) {
    int i, len = PyTuple_Size(val);
    real f[3];
    if (len < 3)
      return -1;
    for (i = 0; i < 3; i++) {
      PyObject *o = PyTuple_GetItem(val, i);
      if (PyFloat_Check(o))
	f[i] = PyFloat_AsDouble(o);
      else if (PyInt_Check(o))
        f[i] = PyInt_AsLong(o) / 65535.;
      else
	f[i] = 0.0;
    }
    p->color_data.red = f[0]; 
    p->color_data.green = f[1]; 
    p->color_data.blue = f[2];
    p->color_data.alpha = 1.0;
    return 0;
  }
  /* also convert char/255 ? */
  return -1;
}
static int
PyDia_set_LineStyle(Property *prop, PyObject *val)
{
  LinestyleProperty *p = (LinestyleProperty*)prop;
  if (PyTuple_Check(val) && PyTuple_Size(val) == 2) {
    p->style = PyInt_AsLong(PyTuple_GetItem(val, 0));
    p->dash  = PyFloat_Check(PyTuple_GetItem(val, 1)) ? PyFloat_AsDouble(PyTuple_GetItem(val, 1)) : PyInt_AsLong(PyTuple_GetItem(val, 1));
    return 0;
  }
  return -1;
}
static int
PyDia_set_Real(Property *prop, PyObject *val)
{
  RealProperty *p = (RealProperty *)prop;
  if (PyFloat_Check(val)) {
    p->real_data = PyFloat_AsDouble(val);
    return 0;
  } else if (PyInt_Check(val)) {
    /* be tolerant for up-casting */
    p->real_data = PyInt_AsLong(val);
    return 0;
  }
  return -1;
}
/* as of this writing the only difference between Real-, Length- and Fontsize-property 
 * is the widget representing them. But that may change so here are the 'type-safe' 
 * accessors.
 */
static int
PyDia_set_Length(Property *prop, PyObject *val)
{
  LengthProperty *p = (LengthProperty *)prop;
  if (PyFloat_Check(val)) {
    p->length_data = PyFloat_AsDouble(val);
    return 0;
  } else if (PyInt_Check(val)) {
    /* be tolerant for up-casting */
    p->length_data = PyInt_AsLong(val);
    return 0;
  }
  return -1;
}
static int
PyDia_set_Fontsize(Property *prop, PyObject *val)
{
  FontsizeProperty *p = (FontsizeProperty *)prop;
  if (PyFloat_Check(val)) {
    p->fontsize_data = PyFloat_AsDouble(val);
    return 0;
  } else if (PyInt_Check(val)) {
    /* be tolerant for up-casting */
    p->fontsize_data = PyInt_AsLong(val);
    return 0;
  }
  return -1;
}

static int
PyDia_set_String(Property *prop, PyObject *val)
{
  StringProperty *p = (StringProperty *)prop;
  if (Py_None == val) {
    /* XXX: maybe a little dangerous, string = NULL not handle in everystring prop */
    g_free (p->string_data);
    p->string_data = NULL;
    p->num_lines = 0;
    return 0;
  } else if (PyString_Check (val)) {
    gchar *str = PyString_AsString (val);
    g_free (p->string_data);
    p->string_data = g_strdup (str);
    p->num_lines = 1;
    return 0;
  } else if (PyUnicode_Check (val)) {
    PyObject *uval = PyUnicode_AsUTF8String (val);
    gchar *str = PyString_AsString(uval);
    g_free (p->string_data);
    p->string_data = g_strdup (str);
    p->num_lines = 1;
    Py_DECREF (uval); 
    return 0;
  }
  return -1;
}
static int
PyDia_set_Text(Property *prop, PyObject *val)
{
  TextProperty *p = (TextProperty *)prop;
  if (PyString_Check (val)) {
    gchar *str = PyString_AsString (val);
    g_free (p->text_data);
    p->text_data = g_strdup (str);
    /* XXX: update size calculation ? */
    return 0;
  } else if (PyUnicode_Check (val)) {
    PyObject *uval = PyUnicode_AsUTF8String (val);
    gchar *str = PyString_AsString(uval);
    g_free (p->text_data);
    p->text_data = g_strdup (str);
    Py_DECREF (uval); 
    return 0;
  }
  return -1;
}
static int
PyDia_set_Point(Property *prop, PyObject *val)
{
  PointProperty *p = (PointProperty*)prop;
  if (PyTuple_Check(val) && PyTuple_Size(val) == 2) {
    p->point_data.x = PyFloat_AsDouble(PyTuple_GetItem(val, 0));
    p->point_data.y = PyFloat_AsDouble(PyTuple_GetItem(val, 1));
    return 0;
  }
  return -1;
}
static int
PyDia_set_PointArray(Property *prop, PyObject *val)
{
  /* accept either tuple or list */
  PointarrayProperty *ptp = (PointarrayProperty *)prop;
  if (PyTuple_Check(val) || PyList_Check(val)) {
    Point pt;
    gboolean is_list = !PyTuple_Check(val);
    int i, len = is_list ? PyList_Size(val) : PyTuple_Size(val);
    gboolean is_flat = TRUE;
    g_array_set_size(ptp->pointarray_data,len);
    for (i = 0; i < len; i++) {
      PyObject *o = is_list ? PyList_GetItem(val, i) : PyTuple_GetItem(val, i);
      if (PyTuple_Check(o)) {
	pt.x = PyFloat_AsDouble(PyTuple_GetItem(o, 0));
	pt.y = PyFloat_AsDouble(PyTuple_GetItem(o, 1));
        g_array_index(ptp->pointarray_data,Point,i) = pt;
	is_flat = FALSE;
      } else {
	if (i % 2) {
	  pt.x = PyFloat_AsDouble(PyTuple_GetItem(val, i-1));
	  pt.y = PyFloat_AsDouble(PyTuple_GetItem(val, i));
          g_array_index(ptp->pointarray_data,Point,i/2) = pt;
	}
      }
    }
    if (is_flat)
      g_array_set_size(ptp->pointarray_data,len/2);
    return 0;
  }
  return -1;
}
static int
PyDia_set_BezPointArray(Property *prop, PyObject *val)
{
  /* accept either tuple or list */
  BezPointarrayProperty *ptp = (BezPointarrayProperty *)prop;
  if (PyTuple_Check(val) || PyList_Check(val)) {
    BezPoint bpt;
    gboolean is_list = !PyTuple_Check(val);
    int i, len = is_list ? PyList_Size(val) : PyTuple_Size(val);
    int numpts = 0;
    g_array_set_size(ptp->bezpointarray_data,len);
    for (i = 0; i < len; i++) {
      /* a tuple of at least (int,double,double) */
      PyObject *o = is_list ? PyList_GetItem(val, i) : PyTuple_GetItem(val, i);
      int tp = PyInt_AsLong(PyTuple_GetItem(o, 0));

      bpt.p1.x = PyFloat_AsDouble(PyTuple_GetItem(o, 1));
      bpt.p1.y = PyFloat_AsDouble(PyTuple_GetItem(o, 2));
      if (BEZ_CURVE_TO == tp) {
        bpt.type = BEZ_CURVE_TO;
        bpt.p2.x = PyFloat_AsDouble(PyTuple_GetItem(o, 3));
        bpt.p2.y = PyFloat_AsDouble(PyTuple_GetItem(o, 4));
        bpt.p3.x = PyFloat_AsDouble(PyTuple_GetItem(o, 5));
        bpt.p3.y = PyFloat_AsDouble(PyTuple_GetItem(o, 6));
      } else {
        if (0 == i && tp != BEZ_MOVE_TO)
          g_debug("First bezpoint must be BEZ_MOVE_TO");
        if (0 < i && tp != BEZ_LINE_TO)
          g_debug("Further bezpoint must be BEZ_LINE_TO or BEZ_CURVE_TO");

        bpt.type = (0 == i) ? BEZ_MOVE_TO : BEZ_LINE_TO;
        /* not strictly needed */
        bpt.p2 = bpt.p3 = bpt.p1;
      }
      g_array_index(ptp->bezpointarray_data,BezPoint,i) = bpt;
      ++numpts;
    }
    /* rather than crashing Dia with too few point handle it here */
    if (numpts < 2) {
      PyErr_Warn (PyExc_RuntimeWarning, "Too few BezPoints!");
      return -1;
    }
    /* only count valid points */
    g_array_set_size(ptp->bezpointarray_data,numpts);
    return 0;
  }
  return -1;
}
static int
PyDia_set_Rect(Property *prop, PyObject *val)
{
  RectProperty *p = (RectProperty*)prop;
  if (PyTuple_Check(val) && PyTuple_Size(val) == 4) {
    p->rect_data.left = PyFloat_AsDouble(PyTuple_GetItem(val, 0));
    p->rect_data.top = PyFloat_AsDouble(PyTuple_GetItem(val, 1));
    p->rect_data.right = PyFloat_AsDouble(PyTuple_GetItem(val, 2));
    p->rect_data.bottom = PyFloat_AsDouble(PyTuple_GetItem(val, 3));
    return 0;
  }
  return -1;
}

static int PyDia_set_Array(Property*, PyObject*);

struct {
  char *type;
  PyObject *(*propget)();
  int (*propset)(Property*, PyObject*);
  GQuark quark;
} prop_type_map [] =
{
  { PROP_TYPE_CHAR, PyDia_get_Char },
  { PROP_TYPE_BOOL, PyDia_get_Bool, PyDia_set_Bool },
  { PROP_TYPE_INT,  PyDia_get_Int, PyDia_set_Int },
  { PROP_TYPE_INTARRAY, PyDia_get_IntArray, PyDia_set_IntArray },
  { PROP_TYPE_ENUM, PyDia_get_Enum, PyDia_set_Enum },
  { PROP_TYPE_ENUMARRAY, PyDia_get_IntArray, PyDia_set_IntArray }, /* Enum == Int */
  { PROP_TYPE_LINESTYLE, PyDia_get_LineStyle, PyDia_set_LineStyle },
  { PROP_TYPE_REAL, PyDia_get_Real, PyDia_set_Real },
  { PROP_TYPE_LENGTH, PyDia_get_Length, PyDia_set_Length },
  { PROP_TYPE_FONTSIZE, PyDia_get_Fontsize, PyDia_set_Fontsize },
  { PROP_TYPE_STRING, PyDia_get_String, PyDia_set_String },
  { PROP_TYPE_STRINGLIST, PyDia_get_StringList },
  { PROP_TYPE_FILE, PyDia_get_String, PyDia_set_String },
  { PROP_TYPE_MULTISTRING, PyDia_get_String, PyDia_set_String },
  { PROP_TYPE_TEXT, PyDia_get_Text, PyDia_set_Text },
  { PROP_TYPE_POINT, PyDia_get_Point, PyDia_set_Point },
  { PROP_TYPE_POINTARRAY, PyDia_get_PointArray, PyDia_set_PointArray },
  { PROP_TYPE_BEZPOINT, PyDia_get_BezPoint },
  { PROP_TYPE_BEZPOINTARRAY, PyDia_get_BezPointArray, PyDia_set_BezPointArray },
  { PROP_TYPE_RECT, PyDia_get_Rect, PyDia_set_Rect },
  { PROP_TYPE_ARROW, PyDia_get_Arrow, PyDia_set_Arrow },
  { PROP_TYPE_MATRIX, PyDia_get_Matrix, PyDia_set_Matrix },
  { PROP_TYPE_COLOUR, PyDia_get_Color, PyDia_set_Color },
  { PROP_TYPE_FONT, PyDia_get_Font },
  { PROP_TYPE_SARRAY, PyDia_get_Array, PyDia_set_Array },
  { PROP_TYPE_DARRAY, PyDia_get_Array, PyDia_set_Array },
  { PROP_TYPE_DICT, PyDia_get_Dict, PyDia_set_Dict },
  { PROP_TYPE_PIXBUF, PyDia_get_Pixbuf, PyDia_set_Pixbuf }
};

static void
ensure_quarks(void)
{
  static gboolean type_quarks_calculated = FALSE;
  int i;
  if (!type_quarks_calculated) {
    for (i = 0; i < G_N_ELEMENTS(prop_type_map); i++) {
      prop_type_map[i].quark = g_quark_from_string (prop_type_map[i].type);
    }
    type_quarks_calculated = TRUE;
  }
}

static PyObject * 
PyDia_get_Array (ArrayProperty *prop)
{
  PyObject *ret;
  int num, num_props;

  num_props = prop->ex_props->len;
  num = prop->records->len;
  ret = PyTuple_New (num);

  /* fill it with tuples or single types */
  if (num > 0) {
    PyDiaPropGetFunc *getters = g_new0(PyDiaPropGetFunc, num_props);
    int i;

    /* resolve the getter functions once */
    for (i = 0; i < num_props; i++) {
      int j;
      for (j = 0; j < G_N_ELEMENTS(prop_type_map); j++) {
        Property *inner = g_ptr_array_index(prop->ex_props, i);
        if (prop_type_map[j].quark == inner->type_quark)
          getters[i] = (PyDiaPropGetFunc)prop_type_map[j].propget;
      }
    }
    for (i = 0; i < num; i++) {
      PyObject *o;
      GPtrArray *p = g_ptr_array_index(prop->records, i);
      int j = 0;

      if (1 == num_props) {
        Property *sub = g_ptr_array_index(p,j);
        o = getters[j](sub);
      } else {
        o = PyTuple_New (num_props);
        for (j = 0; j < num_props; j++) {
          Property *sub = g_ptr_array_index(p,j);
          PyTuple_SetItem(o, j, getters[j](sub));
        }
      }
      PyTuple_SetItem(ret, i, o);
    }
    g_free(getters);
  }

  return ret;
}

static int
PyDia_set_Array (Property *prop, PyObject *val)
{
  ArrayProperty *p = (ArrayProperty *)prop;
  guint i, num_props = p->ex_props->len;
  PyDiaPropSetFunc *setters = g_new0(PyDiaPropSetFunc, num_props);
  int ret = 0;

  /* resolve the getter functions once */
  for (i = 0; i < num_props; i++) {
    Property *ex = g_ptr_array_index(p->ex_props, i);
    int j;
    for (j = 0; j < G_N_ELEMENTS(prop_type_map); j++) {
      if (prop_type_map[j].quark == ex->type_quark)
	setters[i] = (PyDiaPropSetFunc)prop_type_map[j].propset;
    }
    if (!setters[i]) {
      g_debug("No setter for '%s'", ex->descr->type);
      g_free(setters);
      return -1;
    }
  }
  /* tuple or list containing tuples */
  if (PyTuple_Check(val) || PyList_Check(val)) {
    gboolean is_list = !PyTuple_Check(val);
    guint len = is_list ? PyList_Size(val) : PyTuple_Size(val);
    for (i = 0; i < p->records->len; i++) {
      GPtrArray *record = g_ptr_array_index(p->records, i);
      guint j;
      for (j = 0; j < num_props; j++) {
	Property *inner =g_ptr_array_index(record,j);
	inner->ops->free(inner);
      }
      g_ptr_array_free(record, TRUE);
    }
    g_ptr_array_set_size(p->records, 0);
    for (i = 0; i < len; i++) {
      PyObject *t = is_list ? PyList_GetItem(val, i) : PyTuple_GetItem(val, i);
      GPtrArray *record = g_ptr_array_new();
      guint j;
      if (!PyTuple_Check(t) || PyTuple_Size(t) != num_props) {
        g_debug("PyDia_set_Array: %s.", !PyTuple_Check(t) ? "no tuple" : " wrong size");
	ret = -1;
	break;
      }
      g_ptr_array_set_size(record, 0);
      for (j = 0; j < num_props; j++) {
	Property *ex = g_ptr_array_index(p->ex_props, j);
	Property *inner = ex->ops->copy(ex);
	PyObject *v = PyTuple_GetItem(t, j);

	if (0 != setters[j] (inner, v)) {
	  if (Py_None == v) {
            /* use just the defaults, setters don't need to handle this */
	  } else {
	    g_debug ("Failed to set '%s::%s' from '%s'", 
	      p->common.descr->name, inner->descr->name, v->ob_type->tp_name);
	    inner->ops->free(inner);
	    ret = -1;
	    break;
	  }
	}
	g_ptr_array_add(record, inner);
      }
      g_ptr_array_add(p->records, record);
      if (ret != 0)
        break;
    }
  }
  g_free(setters);
  return ret;
}

static void
_keyvalue_get (gpointer key,
               gpointer value,
               gpointer user_data)
{
  gchar *name = (gchar *)key;
  gchar *val = (gchar *)value;
  PyObject *dict = (PyObject *)user_data;
  PyObject *k, *v;
  
  k = PyString_FromString(name);
  v = PyString_FromString(val);
  if (k && v)
    PyDict_SetItem(dict, k, v);
  Py_XDECREF(k);
  Py_XDECREF(v);
}
static PyObject *
PyDia_get_Dict (DictProperty *prop)
{
  PyObject *dict = PyDict_New();
  if (prop->dict)
    g_hash_table_foreach (prop->dict, _keyvalue_get, dict);
  return dict;
}
static int
PyDia_set_Dict (Property *prop, PyObject *val)
{
  DictProperty *p = (DictProperty *)prop;

  if PyDict_Check(val) {
    int i = 0; /* not to be modified! */
    PyObject *key, *value;


    if (!p->dict)
      p->dict = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);
    while (PyDict_Next(val, &i, &key, &value)) {
      /* CHECK semantics: replace or add? */
      g_hash_table_insert (p->dict, 
	                   g_strdup (PyString_AsString (key)), 
			   g_strdup (PyString_AsString (value)));
    }
    return 0;
  }
  return -1;
}

static PyObject *
PyDia_get_Pixbuf (PixbufProperty *prop)
{
  PyObject *pb;

  if (!prop->pixbuf) {
    Py_INCREF(Py_None);
    return Py_None;
  }
  pb = PyCObject_FromVoidPtrAndDesc (prop->pixbuf, NULL, NULL);
  
  return pb;
}
static int
PyDia_set_Pixbuf (Property *prop, PyObject *val)
{
  PixbufProperty *p = (PixbufProperty *)prop;

  if PyCObject_Check(val) {
    gpointer pp = PyCObject_AsVoidPtr (val);

    /* FIXME: refernce counting? */
    p->pixbuf = pp;
    return 0;
  }
  return -1;
}

/*
 * GetAttr
 */
static PyObject *
PyDiaProperty_GetAttr(PyDiaProperty *self, gchar *attr)
{

  if (!strcmp(attr, "__members__"))
    return Py_BuildValue("[ssss]", "name", "type", "value", "visible", "description", "tooltip");
  else if (!strcmp(attr, "name"))
    return PyString_FromString(self->property->descr->name);
  else if (!strcmp(attr, "type"))
    return PyString_FromString(self->property->descr->type);
  else if (!strcmp(attr, "description"))
    return PyString_FromString(self->property->descr->description);
  else if (!strcmp(attr, "tooltip"))
    return PyString_FromString(self->property->descr->tooltip);
  else if (!strcmp(attr, "visible"))
    return PyInt_FromLong(0 != (self->property->descr->flags & PROP_FLAG_VISIBLE));
  else if (!strcmp(attr, "value")) {
    int i;

    ensure_quarks();
    for (i = 0; i < G_N_ELEMENTS(prop_type_map); i++)
      if (prop_type_map[i].quark == self->property->type_quark)
        return prop_type_map[i].propget(self->property);
    if (0 == (PROP_FLAG_WIDGET_ONLY & self->property->descr->flags))
      g_debug ("No handler for type '%s'", self->property->descr->type);

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
int PyDiaProperty_ApplyToObject (DiaObject   *object, 
                                 gchar    *key, 
                                 Property *prop, 
                                 PyObject *val)
{
  int ret = -1;

  if PyDiaProperty_Check(val) {
    /* must be a Property object ? Or PyDiaRect etc ? */
    Property* inprop = ((PyDiaProperty*)val)->property;

    if (0 == strcmp (prop->descr->type, inprop->descr->type)) {
      GPtrArray *plist;
      /* apply it */
      prop->ops->free (prop); /* release this one */
      prop = inprop->ops->copy(inprop);
      /* apply property to object */
      plist = prop_list_from_single (prop);
      object->ops->set_props(object, plist);
      prop_list_free (plist);
      return 0;
    } else {
      g_debug("PyDiaProperty_ApplyToObject : no property conversion %s -> %s",
	             inprop->descr->type, prop->descr->type);
    }
  } else {
    int i;
    ensure_quarks();
    for (i = 0; i < G_N_ELEMENTS(prop_type_map); i++) {
      if (prop_type_map[i].quark == prop->type_quark) {
	if (!prop_type_map[i].propset)
	  g_debug("Setter for '%s' not implemented.", prop_type_map[i].type);
	else if (0 == prop_type_map[i].propset(prop, val))
	  ret = 0;
	break;
      }
    }
    if (ret != 0)
      g_debug("PyDiaProperty_ApplyToObject : no conversion %s -> %s",
	             key, prop->descr->type);
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
  gchar* s;

  s = g_strdup_printf("<DiaProperty at %p, \"%s\", %s>",
                      self,
                      self->property->descr->name,
                      self->property->descr->type);

  py_s = PyString_FromString(s);
  g_free (s);
  return py_s;
}

#define T_INVALID -1 /* can't allow direct access due to pyobject->property indirection */
static PyMemberDef PyDiaProperty_Members[] = {
    { "name", T_INVALID, 0, RESTRICTED|READONLY,
      "string: the name of the property" },
    { "type", T_INVALID, 0, RESTRICTED|READONLY,
      "string: the type name of the object" },
    { "value", T_INVALID, 0, RESTRICTED|READONLY,
      "various: the value is of type char, bool, dict, int, real, string, Text, Point, BezPoint, "
      "Rect, Arrow, Color or a tuple or list, possibly containing tuple of varying types." },
    { "visible", T_INVALID, 0, RESTRICTED|READONLY,
      "bool: visibility of the property" },
    { NULL }
};
/*
 * Python object
 */
PyTypeObject PyDiaProperty_Type = {
    PyObject_HEAD_INIT(NULL)
    0,
    "dia.Property",
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
    (getattrofunc)0,
    (setattrofunc)0,
    (PyBufferProcs *)0,
    0L, /* Flags */
    "Interface to so called StdProps, the mechanism to control "
    "most of Dia's canvas objects properties. ",
    (traverseproc)0,
    (inquiry)0,
    (richcmpfunc)0,
    0, /* tp_weakliszoffset */
    (getiterfunc)0,
    (iternextfunc)0,
    0, /* tp_methods */
    PyDiaProperty_Members, /* tp_members */
    0
};
