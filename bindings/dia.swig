%module dia
%{
#include "object.h"
#include "diarenderer.h"
#include "diagramdata.h"
#include "filter.h"
#include "plug-ins.h"
#include "properties.h"

// helper functions
#ifdef SWIGPYTHON
#include "dia-python-extra.h"
#endif
#include "dia-extra.h"

// the wrapper headers
#include "dia-properties.h"
#include "dia-object.h"
#include "dia-renderer.h"
#include "dia-diagramdata.h"
%}

#define G_GNUC_CONST
#define G_BEGIN_DECLS
#define G_END_DECLS
#define G_INLINE_FUNC
// for SWIG these are the same types
typedef char gchar;

%ignore TYPES_H;
%include "diatypes.h"

// the renaming needs to be done before SWIG sees the declarations
%rename(Rectangle) _Rectangle;
// this should be merged with rectangle somehow
%rename(IntRectangle) _IntRectangle;
%rename(Point) _Point;
%rename(BezPoint) _BezPoint;

%ignore GEOMETRY_H;
// and a bunch of other stuff not yet needed
%ignore dia_acos;
%ignore dia_asin;
%ignore point_add;
%ignore point_sub;
%ignore point_dot;
%ignore point_len;
%ignore point_scale;
%ignore point_rotate;
%ignore point_normalize;
%ignore point_get_perp;
%ignore point_copy;
%ignore point_add_scaled;
%ignore point_convex;
%ignore point_get_normed;
%ignore point_copy_add_scaled;
%ignore point_in_rectangle;
%ignore rectangle_in_rectangle;
%ignore rectangle_union;
%ignore int_rectangle_union;
%ignore rectangle_intersection;
%ignore rectangle_intersects;
%ignore rectangle_add_point;
%ignore rectangle_equals;
%ignore closest_to;
%ignore distance_bez_seg_point;
%ignore distance_point_point;
%ignore distance_point_point_manhattan;
%ignore distance_rectangle_point;
%ignore distance_line_point;
%ignore distance_polygon_point;
%ignore distance_bez_line_point;
%ignore distance_bez_shape_point;
%ignore distance_ellipse_point;
%ignore identity_matrix;
%ignore mult_matrix;
%ignore translate_matrix;
%ignore rotate_matrix;
%ignore transform_point;
%ignore scale_matrix;
%ignore xshear_matrix;
%ignore yshear_matrix;
%ignore dot2;
%ignore line_coef;
%ignore line_to_point;
%ignore point_perp;
%ignore fillet;
%ignore point_cross;
%ignore calculate_object_edge;
%include "geometry.h"

// again using the original header
%nodefaultctor;        // Don't create default constructors
%rename(ImportFilter) _DiaImportFilter;
%rename(ExportFilter) _DiaExportFilter;
%rename(CallbackFilter) _DiaCallbackFilter;
%immutable; // the following are all read-only
%include "filter.h"
%clearnodefaultctor;   // Re-enable default constructors
%mutable;

%rename(Color) _Color;
// redefinition - the original header 'color.h' is too ugly to parse
struct _Color
{
   float red;
   float green;
   float blue;
   float alpha;
};

#if 0 /* works for vc7.1, but not with vc6 and *not* for const char* ? 
       * All the partial specialization needs to be avoided with vc6, no compiler support!
       */
%include "std_vector.i"
using namespace std;
namespace std {
    %template(vectorString) vector<char*>;
}
#else
#  ifdef SWIGPYTHON
// after hours of trying starting with 'const std::vector<const char*>&' this one seems to work ...
%typemap (out) std::vector<char*> {
    int size = $1.size();
    $result = PyList_New(size);
    for (int i=0; i<size; i++)
        PyList_SetItem($result,i,PyString_FromString ($1[i]));
}
#  endif
#endif

// hide the internal helper of all our wrapper clsses
%ignore *::Self;

%ignore dia::Property;
%rename(Property) dia::IProperty;
%include attribute.i
%attribute(dia::IProperty, char*, type, get_type);
%attribute(dia::IProperty, char*, name, get_name);
%nodefaultctor;        // Don't create default constructors
%include "dia-properties.h"
%clearnodefaultctor;   // Re-enable default constructors

// in general we can't handle any of our objects being NULL
%include "constraints.i"
%apply Pointer NONNULL { dia::IProperty * };
%apply Pointer NONNULL { dia::Object * };
%apply Pointer NONNULL { dia::Layer * };
%apply Pointer NONNULL { dia::DiagramData * };

// this is needed to make iteration via __getitem__ work, i.e. not returning None objects.
// It tells SWIG to check every such pointer return and set/do the appropriate error/return.
%typemap(ret) dia::Object*, dia::Handle*, dia::ConnectionPoint*, dia::Layer* {
  if ($1 == NULL) {
    PyErr_SetString (PyExc_IndexError, "not in list");
    return NULL;
  }
}
%typemap(ret) dia::IProperty*, dia::ObjectType* {
  if ($1 == NULL) {
    PyErr_SetString (PyExc_KeyError, "none such");
    return NULL;
  }
}

#ifdef SWIGPYTHON
%extend dia::IProperty {
   %immutable; 
   PyObject* value;
   %mutable;
};
%inline %{
PyObject* dia_IProperty_value_get (dia::IProperty* self) {
     int vi;
     if (self->get (&vi)) return PyInt_FromLong(vi);
     double vd;
     if (self->get (&vd)) return PyFloat_FromDouble(vd);
     const char* vcs;
     if (self->get (&vcs)) return PyString_FromString(vcs);
     char* vs; // not sure if this duplication const char* vs. char* is really needed
     if (self->get (&vs)) return PyString_FromString(vs);
     ::_Point vp;
     if (self->get (&vp)) return SWIG_NewPointerObj(&vp, SWIGTYPE_p__Point, 0);
     ::_Rectangle vr;
     if (self->get (&vr)) return SWIG_NewPointerObj(&vr, SWIGTYPE_p__Rectangle, 0);
     ::_Color vc;
     if (self->get (&vc)) return SWIG_NewPointerObj(&vc, SWIGTYPE_p__Color, 0);
     const std::vector<dia::IProperty*>* vecProp;
     if (self->get (&vecProp)) {
       const size_t n = vecProp->size();
       PyObject* ret = PyTuple_New(n);
       for (int i = 0; i < n; ++i)
         PyTuple_SetItem(ret, i, SWIG_NewPointerObj((*vecProp)[i], SWIGTYPE_p_dia__IProperty, 0));
       return ret;
     }

     // generic fallback - not of much use if one is interested in value
     return SWIG_NewPointerObj(self, SWIGTYPE_p_dia__IProperty, 0);
}
%}
#endif

// must do this 'by hand' cause all the std::map stuff doesn't work with msvc6
%extend dia::Properties {
  dia::IProperty* __getitem__(const char* s) { return self->getitem(s); }

  void __setitem__(const char* s, int v) { self->setitem(s, v); }
  void __setitem__(const char* s, double v) { self->setitem(s, v); }
  void __setitem__(const char* s, const char* v) { self->setitem(s, v); }
#ifdef SWIGPYTHON
  void __setitem__(const char* s, PyObject* v) {
    std::vector<double> vecDouble;
    std::vector<dia::IProperty*> vecProp;
    if (DiaPythonExtra_ToVector (v, vecDouble))
	self->setitem (s, vecDouble);
    else if (DiaPythonExtra_ToVector (v, vecProp))
	self->setitem (s, vecProp);
    else {
        PyObject* vs = PyObject_Str (v);
        printf ("NOT assigning '%s' = %s\n", s, PyString_AsString (vs));
    }
  }
#endif
};

%nodefaultctor;        // Don't create default constructors
%include "dia-connections.h"
%clearnodefaultctor;   // Re-enable default constructors
// again by hand: provide a Python compatible sequence interface
%extend dia::Connections {
  int __len__ () { return self->len(); }
  dia::ConnectionPoint* __getitem__ (int n) { return self->getitem(n); }
};
%extend dia::Handles {
  int __len__ () { return self->len(); }
  dia::Handle* __getitem__ (int n) { return self->getitem(n); }
};

%attribute(dia::Handle, dia::ConnectionPoint*, connected_to, get_connected_to);
%attribute(dia::ConnectionPoint, dia::Object*, object, get_object);
%extend dia::Objects {
  int __len__ () { return self->len(); }
  dia::Object* __getitem__ (int n) { return self->getitem(n); }
};

%nodefaultctor;        // Don't create default constructors
%ignore dia::Object::bbox;
%include "dia-object.h"
%clearnodefaultctor;   // Re-enable default constructors
#ifdef SWIGPYTHON
// there may be a target language independent way to do this but not w/o understanding typemaps ;)
%extend dia::ObjectType {
  // not only creates the object but also two handles to be returned
  PyObject* create (double x, double y) {
    PyObject* ret = PyTuple_New(3);
    dia::Handle *h1, *h2;
    dia::Object *o = self->create (x,y, &h1, &h2);
    PyTuple_SetItem(ret,0,SWIG_NewPointerObj(o, SWIGTYPE_p_dia__Object, 0));
    PyTuple_SetItem(ret,1,SWIG_NewPointerObj(h1, SWIGTYPE_p_dia__Handle, 0));
    PyTuple_SetItem(ret,2,SWIG_NewPointerObj(h2, SWIGTYPE_p_dia__Handle, 0));
    return ret;
  }
};
%include attribute.i
%attribute(dia::Object, _Rectangle, bounding_box, bbox);
%extend dia::Object {
  // convenience/compatibility taking the position as tuple
  PyObject* move_handle (dia::Handle* h, PyObject* pos, int reason, int modifiers)
  {
    double x = PyFloat_AsDouble(PyTuple_GetItem (pos, 0));
    double y = PyFloat_AsDouble(PyTuple_GetItem (pos, 1));
    self->move_handle (h, x, y, (HandleMoveReason)reason, (ModifierKeys)modifiers);
    Py_INCREF(Py_None);
    return Py_None;
  }
};
#endif

%nodefaultctor;        // Don't create default constructors
%include "dia-diagramdata.h"
%clearnodefaultctor;   // Re-enable default constructors

#if 0 // again no partial templates with msvc6
%include "std_vector.i"
%template(Layers) std::vector<const dia::Layer*>;
#else
%extend dia::Layers {
  int __len__ () { return self->len(); }
  dia::Layer* __getitem__ (int n) { return self->getitem(n); }
};
#endif

%nodefaultctor;        // Don't create default constructors
%include "dia-renderer.h"
%clearnodefaultctor;   // Re-enable default constructors

// afaik SWIG does not support cross language callbacks; so let's build one
// NOTE: although renamed above we need to use the old name here
%extend _DiaExportFilter {
  // don't use 'export' as this produces nothing w/o warning
  void do_export (dia::DiagramData *dia, const gchar *filename) {
    DiaContext *ctx = dia_context_new ("Export");
    dia_context_set_filename (ctx, filename);
    if (dia)
      self->export_func (dia->Self(), ctx, filename, filename, self->user_data);
    else
      g_warning ( "%s no data", G_STRFUNC);
    dia_context_release (ctx);
  }
};

// don't generate a non-working binding
%ignore dia::group_create;
%include "dia-extra.h"

%ignore _ot_item;
#ifdef SWIGPYTHON
%inline %{
// compatibility functions, just to leave the original plug-ins unchanged
void register_import (const char* name, const char* ext, PyObject* user)
{
    printf ("dia.register_import ('%s', '%s', ...) faked.\n", name, ext);
}
void register_callback (const char* desc, const char* menu, PyObject* user)
{
    printf ("dia.register_import ('%s', '%s', ...) faked.\n", desc, menu);
}
void register_action (const char* desc, const char* action, const char* menu, PyObject* user)
{
    printf ("dia.register_action ('%s', <%s>'%s', ...) faked.\n", desc, action, menu);
}
//! callback to be applied on every object in the map
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
    v = SWIG_NewPointerObj(type, SWIGTYPE_p_dia__ObjectType, 0);
    if (k && v)
        PyDict_SetItem(dict, k, v);
    Py_XDECREF(k);
    Py_XDECREF(v);
}
PyObject* registered_types () 
{
    PyObject *dict = PyDict_New();

    object_registry_foreach(_ot_item, dict);

    return dict;
}
//! overwrite group_create to work with a list of Python objects
dia::Object* group_create (PyObject *list)
{
    if (PyList_Check(list)) {
	GList *objs = NULL;
	for (int i = 0; i < PyList_Size(list); ++i) {
	    PyObject* obj1 = PyList_GetItem(list, i);
	    void* o;
	    if (SWIG_IsOK(SWIG_ConvertPtr(obj1, &o, SWIGTYPE_p_dia__Object, 0 )))
		objs = g_list_append (objs, reinterpret_cast<dia::Object*>(o)->Self());
	}
	dia::Objects ow(&objs);
	return dia::group_create (&ow);
    }
    return NULL;
}
%}
#endif

// as long as we are not dependending on app/* - which we dont want! - we need to 
// fake Display to some extent to make old pydia plug-ins work
%pythoncode %{
class Display :
    def add_update_all (self) :
        pass
class Diagram :
    def __init__ (self) :
        self.data = DiagramData ()
    def display (self) :
	return Display ()
    def update_extents (self) :
        self.data.update_extents ()
    def flush (self) :
        pass
def new (s) :
    return Diagram()
def active_display () :
    return None
%}
