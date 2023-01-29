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

#define G_LOG_DOMAIN "DiaPython"

#include "config.h"

#include <glib/gi18n-lib.h>

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
PyDiaProperty_Dealloc (PyObject *obj)
{
  PyDiaProperty *self = (PyDiaProperty *) obj;
  self->property->ops->free (self->property);
  PyObject_DEL (self);
}


/*
 * Compare
 */
static PyObject *
PyDiaProperty_RichCompare (PyObject *a,
                           PyObject *b,
                           int       op)
{
  PyDiaProperty *self = (PyDiaProperty *) a;
  PyDiaProperty *other = (PyDiaProperty *) b;
  int cmp = memcmp (&self->property, &other->property, sizeof (Property));
  PyObject *ret;

  switch (op) {
    case Py_EQ:
      ret = cmp == 0 ? Py_True : Py_False;
      break;
    case Py_NE:
      ret = cmp != 0 ? Py_True : Py_False;
      break;
    case Py_LE:
      ret = cmp <= 0 ? Py_True : Py_False;
      break;
    case Py_GE:
      ret = cmp >= 0 ? Py_True : Py_False;
      break;
    case Py_LT:
      ret = cmp < 0 ? Py_True : Py_False;
      break;
    case Py_GT:
      ret = cmp > 0 ? Py_True : Py_False;
      break;
    default:
      ret = Py_NotImplemented;
      break;
  }

  Py_INCREF (ret);

  return ret;
}


/*
 * Hash
 */
static Py_hash_t
PyDiaProperty_Hash (PyObject *self)
{
  return (long) self;
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
PyDia_get_Char (Property *prop)
{
  return PyLong_FromLong (((CharProperty *) prop)->char_data);
}


static PyObject *
PyDia_get_Bool (Property *prop)
{
  return PyLong_FromLong (((BoolProperty *) prop)->bool_data);
}


static PyObject *
PyDia_get_Int (Property *prop)
{
  return PyLong_FromLong (((IntProperty *) prop)->int_data);
}


static PyObject *
PyDia_get_IntArray (Property *uncasted_prop)
{
  IntarrayProperty *prop = (IntarrayProperty *) uncasted_prop;
  PyObject *ret;
  int i, num;

  num = prop->intarray_data->len;
  ret = PyTuple_New (num);

  for (i = 0; i < num; i++) {
    PyTuple_SetItem (ret, i,
                     PyLong_FromLong (g_array_index (prop->intarray_data, int, i)));
  }

  return ret;
}


static PyObject *
PyDia_get_Enum (Property *prop)
{
  return PyLong_FromLong (((EnumProperty *) prop)->enum_data);
}


static PyObject *
PyDia_get_LineStyle (Property *uncasted_prop)
{
  LinestyleProperty *prop = (LinestyleProperty *) uncasted_prop;
  PyObject *ret;

  ret = PyTuple_New (2);
  PyTuple_SetItem (ret, 0, PyLong_FromLong (prop->style));
  PyTuple_SetItem (ret, 1, PyFloat_FromDouble (prop->dash));
  return ret;
}


static PyObject *
PyDia_get_Real (Property *prop)
{
  return PyFloat_FromDouble (((RealProperty *) prop)->real_data);
}


static PyObject *
PyDia_get_Length (Property *prop)
{
  return PyFloat_FromDouble (((LengthProperty *) prop)->length_data);
}


static PyObject *
PyDia_get_Fontsize (Property *prop)
{
  return PyFloat_FromDouble (((FontsizeProperty *) prop)->fontsize_data);
}


static PyObject *
PyDia_get_String (Property *uncasted_prop)
{
  StringProperty *prop = (StringProperty *) uncasted_prop;

  if (NULL == prop->string_data) {
    return PyUnicode_FromString ("(NULL)");
  } else if (1 == prop->num_lines) {
    return PyUnicode_FromString (prop->string_data);
  } else {
    /* FIXME: MULTISTRING ?  */
    return PyUnicode_FromString (prop->string_data);
  }
}


static PyObject *
PyDia_get_StringList (Property *prop)
{
  GList *tmp;
  PyObject *ret = PyList_New (0);

  for (tmp = ((StringListProperty *) prop)->string_list; tmp; tmp = tmp->next) {
    PyList_Append (ret, PyUnicode_FromString (tmp->data));
  }
  return ret;
}


static PyObject *
PyDia_get_Text (Property *uncasted_prop)
{
  TextProperty *prop = (TextProperty *) uncasted_prop;

  return PyDiaText_New (prop->text_data, &prop->attr);
}


static PyObject *
PyDia_get_Point (Property *prop)
{
  return PyDiaPoint_New (&((PointProperty *) prop)->point_data);
}


static PyObject *
PyDia_get_PointArray (Property *uncasted_prop)
{
  PointarrayProperty *prop = (PointarrayProperty *) uncasted_prop;
  PyObject *ret;
  int i, num;

  num = prop->pointarray_data->len;
  ret = PyTuple_New (num);

  for (i = 0; i < num; i++) {
    PyTuple_SetItem (ret, i,
                     PyDiaPoint_New (&g_array_index(prop->pointarray_data,
                                                    Point, i)));
  }

  return ret;
}


static PyObject *
PyDia_get_BezPoint (Property *prop)
{
  return PyDiaBezPoint_New (&((BezPointProperty *) prop)->bezpoint_data);
}


static PyObject *
PyDia_get_BezPointArray (Property *uncasted_prop)
{
  BezPointarrayProperty *prop = (BezPointarrayProperty *) uncasted_prop;
  PyObject *ret;
  int num;

  num = prop->bezpointarray_data->len;
  ret = PyTuple_New (num);

  for (int i = 0; i < num; i++) {
    PyTuple_SetItem (ret, i,
                     PyDiaBezPoint_New (&g_array_index (prop->bezpointarray_data,
                                                        BezPoint,
                                                        i)));
  }

  return ret;
}


static PyObject *
PyDia_get_Rect (Property *prop)
{
  return PyDiaRectangle_New (&((RectProperty *) prop)->rect_data);
}


static PyObject *
PyDia_get_Arrow (Property *prop)
{
  return PyDiaArrow_New (&((ArrowProperty *) prop)->arrow_data);
}


static PyObject *
PyDia_get_Matrix (Property *prop)
{
  return PyDiaMatrix_New (((MatrixProperty *) prop)->matrix);
}


static PyObject *
PyDia_get_Color (Property *prop)
{
  return PyDiaColor_New (&((ColorProperty *) prop)->color_data);
}


static PyObject *
PyDia_get_Font (Property *prop)
{
  return PyDiaFont_New (((FontProperty *) prop)->font_data);
}


static PyObject *PyDia_get_Array (Property *prop);
static int PyDia_set_Array (Property *, PyObject *);

static PyObject *PyDia_get_Dict (Property *prop);
static int PyDia_set_Dict (Property *prop, PyObject *val);

static PyObject *PyDia_get_Pixbuf (Property *prop);
static int PyDia_set_Pixbuf (Property *prop, PyObject *val);

static int
PyDia_set_Bool (Property *prop, PyObject *val)
{
  BoolProperty *p = (BoolProperty *) prop;

  if (PyLong_Check (val)) {
    p->bool_data = !!PyLong_AsLong (val);
    return 0;
  }

  return -1;
}


static int
PyDia_set_Int (Property *prop, PyObject *val)
{
  IntProperty *p = (IntProperty *) prop;

  if (PyLong_Check (val)) {
    p->int_data = PyLong_AsLong (val);
    return 0;
  }

  return -1;
}


static int
PyDia_set_IntArray (Property *prop, PyObject *val)
{
  IntarrayProperty *p = (IntarrayProperty *) prop;

  if (PyTuple_Check (val)) {
    int len = PyTuple_Size (val);
    g_array_set_size (p->intarray_data, len);

    for (int i = 0; i < len; i++) {
      PyObject *o = PyTuple_GetItem (val, i);
      g_array_index (p->intarray_data, int, i) = PyLong_Check (o) ? PyLong_AsLong (o) : 0;
    }

    return 0;
  } else if (PyList_Check (val)) {
    int len = PyList_Size (val);
    g_array_set_size (p->intarray_data, len);

    for (int i = 0; i < len; i++) {
      PyObject *o = PyList_GetItem(val, i);
      g_array_index (p->intarray_data, int, i) = PyLong_Check (o) ? PyLong_AsLong (o) : 0;
    }

    return 0;
  }

  return -1;
}


static int
PyDia_set_Enum (Property *prop, PyObject *val)
{
  EnumProperty *p = (EnumProperty *) prop;

  /* XXX a range check would not hurt */
  if (PyLong_Check (val)) {
    p->enum_data = PyLong_AsLong (val);
    return 0;
  }

  return -1;
}


static int
PyDia_set_Arrow (Property *prop, PyObject *val)
{
  ArrowProperty *p = (ArrowProperty *)prop;

  if (val->ob_type == &PyDiaArrow_Type) {
    p->arrow_data = ((PyDiaArrow *) val)->arrow;
    return 0;
  } else if (PyTuple_Check (val)) {
    int len = PyTuple_Size (val);
    PyObject *o;
    if (len < 3)
      return -1;
    if ((o = PyTuple_GetItem(val, 0)) != NULL && PyLong_Check (o))
      p->arrow_data.type = PyLong_AsLong (o);
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
      p->matrix = g_new0 (DiaMatrix, 1);
    *p->matrix = ((PyDiaMatrix *) val)->matrix;
    return 0;
  }
  return -1;
}


static int
PyDia_set_Color (Property *prop, PyObject *val)
{
  ColorProperty *p = (ColorProperty*) prop;

  if (PyUnicode_Check (val)) {
    const char *str = PyUnicode_AsUTF8 (val);
    PangoColor color;

    if (pango_color_parse (&color, str)) {
      p->color_data.red = color.red / 65535.0;
      p->color_data.green = color.green / 65535.0;
      p->color_data.blue = color.blue / 65535.0;
      p->color_data.alpha = 1.0;
      return 0;
    } else {
      g_debug ("%s: Failed to parse color string '%s'", G_STRLOC, str);
    }
  } else if (PyTuple_Check (val)) {
    int len = PyTuple_Size (val);
    double f[3];

    if (len < 3) {
      return -1;
    }
    for (int i = 0; i < 3; i++) {
      PyObject *o = PyTuple_GetItem (val, i);

      if (PyFloat_Check (o)) {
        f[i] = PyFloat_AsDouble (o);
      } else if (PyLong_Check (o)) {
        f[i] = PyLong_AsLong (o) / 65535.;
      } else {
        f[i] = 0.0;
      }
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
PyDia_set_LineStyle (Property *prop, PyObject *val)
{
  LinestyleProperty *p = (LinestyleProperty *) prop;

  if (PyTuple_Check (val) && PyTuple_Size (val) == 2) {
    p->style = PyLong_AsLong (PyTuple_GetItem (val, 0));
    p->dash  = PyFloat_Check (PyTuple_GetItem (val, 1)) ?
          PyFloat_AsDouble (PyTuple_GetItem (val, 1)) :
                PyLong_AsLong (PyTuple_GetItem (val, 1));
    return 0;
  }

  return -1;
}


static int
PyDia_set_Real (Property *prop, PyObject *val)
{
  RealProperty *p = (RealProperty *) prop;

  if (PyFloat_Check (val)) {
    p->real_data = PyFloat_AsDouble (val);
    return 0;
  } else if (PyLong_Check (val)) {
    /* be tolerant for up-casting */
    p->real_data = PyLong_AsLong (val);
    return 0;
  }

  return -1;
}


/* as of this writing the only difference between Real-, Length- and Fontsize-property
 * is the widget representing them. But that may change so here are the 'type-safe'
 * accessors.
 */
static int
PyDia_set_Length (Property *prop, PyObject *val)
{
  LengthProperty *p = (LengthProperty *) prop;

  if (PyFloat_Check (val)) {
    p->length_data = PyFloat_AsDouble (val);
    return 0;
  } else if (PyLong_Check (val)) {
    /* be tolerant for up-casting */
    p->length_data = PyLong_AsLong (val);
    return 0;
  }

  return -1;
}


static int
PyDia_set_Fontsize (Property *prop, PyObject *val)
{
  FontsizeProperty *p = (FontsizeProperty *) prop;

  if (PyFloat_Check (val)) {
    p->fontsize_data = PyFloat_AsDouble (val);
    return 0;
  } else if (PyLong_Check (val)) {
    /* be tolerant for up-casting */
    p->fontsize_data = PyLong_AsLong (val);
    return 0;
  }

  return -1;
}


static int
PyDia_set_String (Property *prop, PyObject *val)
{
  StringProperty *p = (StringProperty *) prop;

  if (Py_None == val) {
    /* XXX: maybe a little dangerous, string = NULL not handle in everystring prop */
    g_clear_pointer (&p->string_data, g_free);
    p->num_lines = 0;
    return 0;
  } else if (PyUnicode_Check (val)) {
    const char *str = PyUnicode_AsUTF8 (val);
    g_clear_pointer (&p->string_data, g_free);
    p->string_data = g_strdup (str);
    p->num_lines = 1;
    return 0;
  }

  return -1;
}


static int
PyDia_set_Text (Property *prop, PyObject *val)
{
  TextProperty *p = (TextProperty *)prop;

  if (PyUnicode_Check (val)) {
    const char *str = PyUnicode_AsUTF8 (val);
    g_clear_pointer (&p->text_data, g_free);
    p->text_data = g_strdup (str);
    /* XXX: update size calculation ? */
    return 0;
  }

  return -1;
}


static int
PyDia_set_Point (Property *prop, PyObject *val)
{
  PointProperty *p = (PointProperty*) prop;

  if (PyTuple_Check(val) && PyTuple_Size(val) == 2) {
    p->point_data.x = PyFloat_AsDouble(PyTuple_GetItem(val, 0));
    p->point_data.y = PyFloat_AsDouble(PyTuple_GetItem(val, 1));
    return 0;
  }

  return -1;
}


static int
PyDia_set_PointArray (Property *prop, PyObject *val)
{
  /* accept either tuple or list */
  PointarrayProperty *ptp = (PointarrayProperty *) prop;

  if (PyTuple_Check (val) || PyList_Check (val)) {
    Point pt;
    gboolean is_list = !PyTuple_Check (val);
    int len = is_list ? PyList_Size (val) : PyTuple_Size (val);
    gboolean is_flat = TRUE;
    g_array_set_size(ptp->pointarray_data,len);
    for (int i = 0; i < len; i++) {
      PyObject *o = is_list ? PyList_GetItem (val, i) : PyTuple_GetItem (val, i);
      if (PyTuple_Check(o)) {
        pt.x = PyFloat_AsDouble (PyTuple_GetItem (o, 0));
        pt.y = PyFloat_AsDouble (PyTuple_GetItem (o, 1));
        g_array_index (ptp->pointarray_data, Point, i) = pt;
        is_flat = FALSE;
      } else {
        if (i % 2) {
          pt.x = PyFloat_AsDouble (PyTuple_GetItem (val, i - 1));
          pt.y = PyFloat_AsDouble (PyTuple_GetItem (val, i));
          g_array_index (ptp->pointarray_data, Point, i / 2) = pt;
        }
      }
    }

    if (is_flat) {
      g_array_set_size (ptp->pointarray_data, len / 2);
    }

    return 0;
  }

  return -1;
}


static int
PyDia_set_BezPointArray (Property *prop, PyObject *val)
{
  /* accept either tuple or list */
  BezPointarrayProperty *ptp = (BezPointarrayProperty *) prop;

  if (PyTuple_Check (val) || PyList_Check (val)) {
    BezPoint bpt;
    gboolean is_list = !PyTuple_Check (val);
    int len = is_list ? PyList_Size (val) : PyTuple_Size (val);
    int numpts = 0;

    g_array_set_size (ptp->bezpointarray_data, len);

    for (int i = 0; i < len; i++) {
      /* a tuple of at least (int,double,double) */
      PyObject *o = is_list ? PyList_GetItem (val, i) : PyTuple_GetItem (val, i);
      int tp = PyLong_AsLong (PyTuple_GetItem (o, 0));

      bpt.p1.x = PyFloat_AsDouble (PyTuple_GetItem (o, 1));
      bpt.p1.y = PyFloat_AsDouble (PyTuple_GetItem (o, 2));

      if (BEZ_CURVE_TO == tp) {
        bpt.type = BEZ_CURVE_TO;
        bpt.p2.x = PyFloat_AsDouble (PyTuple_GetItem (o, 3));
        bpt.p2.y = PyFloat_AsDouble (PyTuple_GetItem (o, 4));
        bpt.p3.x = PyFloat_AsDouble (PyTuple_GetItem (o, 5));
        bpt.p3.y = PyFloat_AsDouble (PyTuple_GetItem (o, 6));
      } else {
        if (0 == i && tp != BEZ_MOVE_TO) {
          g_debug ("%s: First bezpoint must be BEZ_MOVE_TO", G_STRLOC);
        }

        if (0 < i && tp != BEZ_LINE_TO) {
          g_debug ("%s: Further bezpoint must be BEZ_LINE_TO or BEZ_CURVE_TO",
                   G_STRLOC);
        }

        bpt.type = (0 == i) ? BEZ_MOVE_TO : BEZ_LINE_TO;
        /* not strictly needed */
        bpt.p2 = bpt.p3 = bpt.p1;
      }
      g_array_index (ptp->bezpointarray_data, BezPoint, i) = bpt;
      ++numpts;
    }

    /* rather than crashing Dia with too few point handle it here */
    if (numpts < 2) {
      PyErr_Warn (PyExc_RuntimeWarning, "Too few BezPoints!");
      return -1;
    }

    /* only count valid points */
    g_array_set_size (ptp->bezpointarray_data, numpts);

    return 0;
  }

  return -1;
}


static int
PyDia_set_Rect (Property *prop, PyObject *val)
{
  RectProperty *p = (RectProperty*) prop;

  if (PyTuple_Check (val) && PyTuple_Size (val) == 4) {
    p->rect_data.left = PyFloat_AsDouble (PyTuple_GetItem (val, 0));
    p->rect_data.top = PyFloat_AsDouble (PyTuple_GetItem (val, 1));
    p->rect_data.right = PyFloat_AsDouble (PyTuple_GetItem (val, 2));
    p->rect_data.bottom = PyFloat_AsDouble (PyTuple_GetItem (val, 3));
    return 0;
  }

  return -1;
}

struct {
  char *type;
  PyObject *(*propget)(Property*);
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
ensure_quarks (void)
{
  static gboolean type_quarks_calculated = FALSE;

  if (!type_quarks_calculated) {
    for (int i = 0; i < G_N_ELEMENTS (prop_type_map); i++) {
      prop_type_map[i].quark = g_quark_from_string (prop_type_map[i].type);
    }
    type_quarks_calculated = TRUE;
  }
}


static PyObject *
PyDia_get_Array (Property *uncasted_prop)
{
  ArrayProperty *prop = (ArrayProperty *) uncasted_prop;
  PyObject *ret;
  int num, num_props;

  num_props = prop->ex_props->len;
  num = prop->records->len;
  ret = PyTuple_New (num);

  /* fill it with tuples or single types */
  if (num > 0) {
    PyDiaPropGetFunc *getters = g_new0 (PyDiaPropGetFunc, num_props);

    /* resolve the getter functions once */
    for (int i = 0; i < num_props; i++) {
      for (int j = 0; j < G_N_ELEMENTS (prop_type_map); j++) {
        Property *inner = g_ptr_array_index (prop->ex_props, i);
        if (prop_type_map[j].quark == inner->type_quark) {
          getters[i] = (PyDiaPropGetFunc)prop_type_map[j].propget;
        }
      }
    }

    for (int i = 0; i < num; i++) {
      PyObject *o;
      GPtrArray *p = g_ptr_array_index (prop->records, i);
      int j = 0;

      if (1 == num_props) {
        Property *sub = g_ptr_array_index (p, j);
        o = getters[j](sub);
      } else {
        o = PyTuple_New (num_props);
        for (j = 0; j < num_props; j++) {
          Property *sub = g_ptr_array_index (p,j);
          PyTuple_SetItem (o, j, getters[j](sub));
        }
      }

      PyTuple_SetItem (ret, i, o);
    }

    g_clear_pointer (&getters, g_free);
  }

  return ret;
}


static int
PyDia_set_Array (Property *prop, PyObject *val)
{
  ArrayProperty *p = (ArrayProperty *)prop;
  guint num_props = p->ex_props->len;
  PyDiaPropSetFunc *setters = g_new0 (PyDiaPropSetFunc, num_props);
  int ret = 0;

  /* resolve the getter functions once */
  for (int i = 0; i < num_props; i++) {
    Property *ex = g_ptr_array_index (p->ex_props, i);

    for (int j = 0; j < G_N_ELEMENTS (prop_type_map); j++) {
      if (prop_type_map[j].quark == ex->type_quark) {
        setters[i] = (PyDiaPropSetFunc) prop_type_map[j].propset;
      }
    }

    if (!setters[i]) {
      g_debug ("%s: No setter for '%s'", G_STRLOC, ex->descr->type);
      g_clear_pointer (&setters, g_free);
      return -1;
    }
  }

  /* tuple or list containing tuples */
  if (PyTuple_Check (val) || PyList_Check (val)) {
    gboolean is_list = !PyTuple_Check (val);
    guint len = is_list ? PyList_Size (val) : PyTuple_Size (val);

    for (int i = 0; i < p->records->len; i++) {
      GPtrArray *record = g_ptr_array_index (p->records, i);

      for (int j = 0; j < num_props; j++) {
        Property *inner =g_ptr_array_index (record, j);
        inner->ops->free (inner);
      }
      g_ptr_array_free (record, TRUE);
    }

    g_ptr_array_set_size (p->records, 0);

    for (int i = 0; i < len; i++) {
      PyObject *t = is_list ? PyList_GetItem (val, i) : PyTuple_GetItem (val, i);
      GPtrArray *record = g_ptr_array_new ();

      if (!PyTuple_Check (t) || PyTuple_Size (t) != num_props) {
        g_debug ("%s: PyDia_set_Array: %s.",
                 G_STRLOC,
                 !PyTuple_Check (t) ? "no tuple" : " wrong size");
        ret = -1;
        break;
      }

      g_ptr_array_set_size (record, 0);

      for (int j = 0; j < num_props; j++) {
        Property *ex = g_ptr_array_index (p->ex_props, j);
        Property *inner = ex->ops->copy (ex);
        PyObject *v = PyTuple_GetItem (t, j);

        if (0 != setters[j] (inner, v)) {
          if (Py_None == v) {
                  /* use just the defaults, setters don't need to handle this */
          } else {
            g_debug ("%s: Failed to set '%s::%s' from '%s'",
                     G_STRLOC,
                     p->common.descr->name,
                     inner->descr->name,
                     v->ob_type->tp_name);
            inner->ops->free (inner);
            ret = -1;
            break;
          }
        }

        g_ptr_array_add (record, inner);
      }

      g_ptr_array_add (p->records, record);

      if (ret != 0) {
        break;
      }
    }
  }

  g_clear_pointer (&setters, g_free);

  return ret;
}


static void
_keyvalue_get (gpointer key,
               gpointer value,
               gpointer user_data)
{
  char *name = (char *) key;
  char *val = (char *) value;
  PyObject *dict = (PyObject *)user_data;
  PyObject *k, *v;

  k = PyUnicode_FromString (name);
  v = PyUnicode_FromString (val);

  if (k && v) {
    PyDict_SetItem (dict, k, v);
  }

  Py_XDECREF (k);
  Py_XDECREF (v);
}


static PyObject *
PyDia_get_Dict (Property *uncasted_prop)
{
  DictProperty *prop = (DictProperty *) uncasted_prop;
  PyObject *dict = PyDict_New();

  if (prop->dict) {
    g_hash_table_foreach (prop->dict, _keyvalue_get, dict);
  }

  return dict;
}


static int
PyDia_set_Dict (Property *prop, PyObject *val)
{
  DictProperty *p = (DictProperty *)prop;

  if (PyDict_Check (val)) {
    Py_ssize_t i = 0; /* not to be modified! */
    PyObject *key, *value;

    if (!p->dict) {
      p->dict = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);
    }

    while (PyDict_Next (val, &i, &key, &value)) {
      /* CHECK semantics: replace or add? */
      g_hash_table_insert (p->dict, g_strdup (PyUnicode_AsUTF8 (key)),
      g_strdup (PyUnicode_AsUTF8 (value)));
    }

    return 0;
  }

  return -1;
}


static PyObject *
PyDia_get_Pixbuf (Property *uncasted_prop)
{
  PixbufProperty *prop = (PixbufProperty *) uncasted_prop;
  PyObject *pb;

  if (!prop->pixbuf) {
    Py_INCREF(Py_None);
    return Py_None;
  }
  pb = PyCapsule_New (prop->pixbuf, "pixbuf", NULL);

  return pb;
}


static int
PyDia_set_Pixbuf (Property *prop, PyObject *val)
{
  PixbufProperty *p = (PixbufProperty *) prop;

  if (PyCapsule_IsValid (val, "pixbuf")) {
    gpointer pp = PyCapsule_GetPointer (val, "pixbuf");

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
PyDiaProperty_GetAttr (PyObject *obj, PyObject *arg)
{
  PyDiaProperty *self;
  const char *attr;

  if (PyUnicode_Check (arg)) {
    attr = PyUnicode_AsUTF8 (arg);
  } else {
    goto generic;
  }

  self = (PyDiaProperty *) obj;

  if (!g_strcmp0 (attr, "__members__")) {
    return Py_BuildValue ("[ssss]",
                          "name", "type", "value", "visible", "description",
                          "tooltip");
  } else if (!g_strcmp0 (attr, "name")) {
    return PyUnicode_FromString (self->property->descr->name);
  } else if (!g_strcmp0 (attr, "type")) {
    return PyUnicode_FromString (self->property->descr->type);
  } else if (!g_strcmp0 (attr, "description")) {
    return PyUnicode_FromString (self->property->descr->description);
  } else if (!g_strcmp0 (attr, "tooltip")) {
    return PyUnicode_FromString (self->property->descr->tooltip);
  } else if (!g_strcmp0 (attr, "visible")) {
    return PyLong_FromLong (0 != (self->property->descr->flags & PROP_FLAG_VISIBLE));
  } else if (!g_strcmp0 (attr, "value")) {
    int i;

    ensure_quarks ();
    for (i = 0; i < G_N_ELEMENTS (prop_type_map); i++) {
      if (prop_type_map[i].quark == self->property->type_quark) {
        return prop_type_map[i].propget (self->property);
      }
    }

    if (0 == (PROP_FLAG_WIDGET_ONLY & self->property->descr->flags)) {
      g_debug ("%s: No handler for type '%s'",
               G_STRLOC,
               self->property->descr->type);
    }

    Py_INCREF (Py_None);
    return Py_None;
  }

generic:
  return PyObject_GenericGetAttr (obj, arg);
}


/*
 * Similar to SetAttr but the property is directly applied
 * to the DiaObject
 */
int
PyDiaProperty_ApplyToObject (DiaObject  *object,
                             const char *key,
                             Property   *prop,
                             PyObject   *val)
{
  int ret = -1;

  if (PyDiaProperty_Check (val)) {
    /* must be a Property object ? Or PyDiaRect etc ? */
    Property *inprop = ((PyDiaProperty *) val)->property;

    if (g_strcmp0 (prop->descr->type, inprop->descr->type) == 0) {
      GPtrArray *plist;
      /* apply it */
      prop->ops->free (prop); /* release this one */
      prop = inprop->ops->copy (inprop);
      /* apply property to object */
      plist = prop_list_from_single (prop);
      dia_object_set_properties (object, plist);
      prop_list_free (plist);
      return 0;
    } else {
      g_debug ("%s: PyDiaProperty_ApplyToObject : no property conversion %s -> %s",
               G_STRLOC,
               inprop->descr->type,
               prop->descr->type);
    }
  } else {
    int i;
    ensure_quarks ();
    for (i = 0; i < G_N_ELEMENTS (prop_type_map); i++) {
      if (prop_type_map[i].quark == prop->type_quark) {
        if (!prop_type_map[i].propset) {
          g_debug ("%s: Setter for '%s' not implemented.",
                   G_STRLOC,
                   prop_type_map[i].type);
        } else if (0 == prop_type_map[i].propset (prop, val)) {
          ret = 0;
        }
        break;
      }
    }
    if (ret != 0) {
      g_debug ("%s: PyDiaProperty_ApplyToObject : no conversion %s -> %s",
               G_STRLOC,
               key,
               prop->descr->type);
    }
  }

  if (0 == ret) {
    /* apply property to object */
    GPtrArray *plist = prop_list_from_single (prop);
    dia_object_set_properties (object, plist);
    prop_list_free (plist);
  }

  return ret;
}


/*
 * Repr / _Str
 */
static PyObject *
PyDiaProperty_Str (PyObject *obj)
{
  PyDiaProperty *self = (PyDiaProperty *) obj;
  PyObject* py_s;
  char* s;

  s = g_strdup_printf ("<DiaProperty at %p, \"%s\", %s>",
                       self,
                       self->property->descr->name,
                       self->property->descr->type);

  py_s = PyUnicode_FromString (s);
  g_clear_pointer (&s, g_free);

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
  PyVarObject_HEAD_INIT (NULL, 0)
  .tp_name = "dia.Property",
  .tp_basicsize = sizeof (PyDiaProperty),
  .tp_dealloc = PyDiaProperty_Dealloc,
  .tp_getattro = PyDiaProperty_GetAttr,
  .tp_richcompare = PyDiaProperty_RichCompare,
  .tp_hash = PyDiaProperty_Hash,
  .tp_str = PyDiaProperty_Str,
  .tp_doc = "Interface to so called StdProps, the mechanism to control "
            "most of Dia's canvas objects properties. ",
  .tp_members = PyDiaProperty_Members,
};
