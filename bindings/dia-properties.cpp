/*
 * C++ implementation of Dia Properties - just to have something clean to wrap
 *
 * Copyright (C) 2007, Hans Breuer, <Hans@Breuer.Org>
 *
 * This is free software; you can redistribute it and/or modify
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
#include "properties.h"
// need access to property internals
#include "prop_inttypes.h"
#include "prop_geomtypes.h"
#include "prop_attr.h"
#include "prop_text.h"
#include "prop_sdarray.h"

#include "object.h"
#include <assert.h>
#include <cstring>

#include "dia-properties.h"
#include "dia-object.h"

dia::Property< ::Property* >::Property (::Property* p) : self(p) 
{
}

//! read-only attribute; the name of the property; at least unique within an object, but supposed to be unique global 
const char* 
dia::Property< ::Property* >::get_name () const
{
    if (self)
        return self->descr->name;
    return "<null>::name";
}
//! read-only attribute giving the data type represented
const char* 
dia::Property< ::Property* >::get_type () const
{
    if (self)
        return self->descr->type;
    return "<null>::type";
}
/*!
 * \brief getter depending on the type of the property
 *
 * This is supposed to deliver very different types - which is not possible in a strongly type language.
 * We could either have various accessors like:
 *     template<class T> bool Property::get_value<T>(T*);
 * an wrap them into one Property.value for runtime typed languages or ... ?
 */
bool 
dia::Property< ::Property* >::get (int* v) const
{
    g_return_val_if_fail (self != NULL, false);
    bool ret = true;
    
    if (strcmp (self->descr->type, PROP_TYPE_BOOL) == 0)
	*v = ((BoolProperty *)self)->bool_data;
    else if (strcmp (self->descr->type, PROP_TYPE_INT) == 0)
	*v = ((IntProperty *)self)->int_data;
    else if (strcmp (self->descr->type, PROP_TYPE_ENUM) == 0)
	*v = ((EnumProperty *)self)->enum_data;
    else
        ret = false;

    return ret;
}
//! getter for double
bool 
dia::Property< ::Property* >::get (double* v) const
{
    g_return_val_if_fail (self != NULL, false);
    if (strcmp (self->descr->type, PROP_TYPE_REAL) == 0) {
	*v = ((RealProperty *)self)->real_data;
	return true;
    }
    return false;
}
//! getter for string
bool 
dia::Property< ::Property* >::get (const char** v) const
{
    g_return_val_if_fail (self != NULL, false);
    if (strcmp (self->descr->type, PROP_TYPE_STRING) == 0) {
	*v = ((StringProperty*)self)->string_data;
	if (*v == NULL)
	  *v = "";
	return true;
    }
    return false;
}
//! Now it starts to become ugly: isn't there a better way with SWIG to map one to may types?
bool 
dia::Property< ::Property* >::get (::_Point* v) const 
{ 
    g_return_val_if_fail (self != NULL, false);
    if (strcmp (self->descr->type, PROP_TYPE_POINT) == 0) {
	*v = ((PointProperty *)self)->point_data;
	return true;
    }
    return false; 
}
//! almost complete ;)
bool
dia::Property< ::Property* >::get (::_Rectangle* v) const 
{ 
    g_return_val_if_fail (self != NULL, false);
    if (strcmp (self->descr->type, PROP_TYPE_RECT) == 0) {
	*v = ((RectProperty *)self)->rect_data;
	return true;
    }
    return false; 
}
//! the final one
bool
dia::Property< ::Property* >::get (const std::vector<IProperty*>** v) const 
{ 
    g_return_val_if_fail (self != NULL, false);
    if (strcmp (self->descr->type, PROP_TYPE_DARRAY) == 0) {
	// remove everything from an earlier call
	std::vector<IProperty*>::iterator it;
	// remove const from our cache
	std::vector<IProperty*>& vec = const_cast< std::vector<IProperty*>& >(_vec);
	for (it = vec.begin(); it != vec.end(); ++it)
	    delete *it;
	vec.clear();

	::ArrayProperty *prop = (::ArrayProperty *)self;

	// now build with new values
	int num_props = prop->ex_props->len;
	for (int i = 0; i < prop->records->len; ++i) {
	  ::Property* p = 0;
	  vec.push_back (new dia::Property <Property*> (0));
	}
	*v = &vec;
	return true;
    }
    return false; 
}

dia::Property< ::Property* >::~Property ()
{
    std::vector<IProperty*>::iterator it;
    for (it = _vec.begin(); it != _vec.end(); ++it)
	delete *it;
}

/*!
 * If there is still a property conversion missing it needs to be added
 * in four places. In the implementation dia-properties.cpp, in the
 * interface of this class and IProperty: dia-properties.h and finally 
 * in the .swig file.
 */
bool
dia::Property< ::Property* >::get (::_Color* v) const 
{ 
    g_return_val_if_fail (self != NULL, false);
    if (strcmp (self->descr->type, PROP_TYPE_COLOUR) == 0) {
	*v = ((ColorProperty *)self)->color_data;
	return true;
    }
    return false; 
}

//! if the property is to be shown (in a dialog)
bool 
dia::Property< ::Property* >::visible () const
{
    g_return_val_if_fail (self != NULL, false);
    return !!(self->descr->flags & PROP_FLAG_VISIBLE);
}

dia::Properties::Properties (::DiaObject* o) : object(o)
{
}
dia::Properties::~Properties ()
{
}

//! is there a property of this name
bool 
dia::Properties::has_key (const char* name) const
{
    g_return_val_if_fail (object != NULL, false);
    if (!object->ops->get_props)
        return false;
    ::Property *p = object_prop_by_name (object, name);
    bool ret = (p != NULL);
    if (p)
      p->ops->free(p);
    return ret;
}
static bool
set_prop (::Property* p, int v)
{
    if (strcmp (p->descr->type, PROP_TYPE_BOOL) == 0)
	((BoolProperty *)p)->bool_data = (v != 0);
    else if (strcmp (p->descr->type, PROP_TYPE_INT) == 0)
	((IntProperty *)p)->int_data = v;
    else if (strcmp (p->descr->type, PROP_TYPE_ENUM) == 0)
	((EnumProperty *)p)->enum_data = v;
    else
        return false;
    return true;
}
static bool
set_prop (::Property* p, double v)
{
    if (strcmp (p->descr->type, PROP_TYPE_REAL) == 0)
	((RealProperty *)p)->real_data = v;
    else
        return false;
    return true;
}
static bool
set_prop (::Property* p, const char* v)
{
    if (strcmp (p->descr->type, PROP_TYPE_COLOUR) == 0) {
	PangoColor color;
	if (pango_color_parse(&color, v)) {
            ((ColorProperty*)p)->color_data.red = color.red / 65535.0; 
            ((ColorProperty*)p)->color_data.green = color.green / 65535.0; 
            ((ColorProperty*)p)->color_data.blue = color.blue / 65535.0;
            ((ColorProperty*)p)->color_data.alpha = 1.0;
        }
    }
    else if (strcmp (p->descr->type, PROP_TYPE_STRING) == 0) {
	g_free (((StringProperty*)p)->string_data);
        ((StringProperty*)p)->string_data = v ? g_strdup (v) : g_strdup ("");
        ((StringProperty*)p)->num_lines = 1;
    }
    else if (strcmp (p->descr->type, PROP_TYPE_TEXT) == 0) {
	g_free (((TextProperty*)p)->text_data);
        ((TextProperty*)p)->text_data = v ? g_strdup (v) : g_strdup ("");
        /* XXX: update size calculation ? */
    }
    else
        return false;
    return true;
}
static bool
set_prop (::Property* p, char* v)
{
    return set_prop (p, (const char*)v);
}

static bool
set_prop (GPtrArray* to, ::ArrayProperty* kinds, int num, const std::vector<dia::IProperty*>&v)
{
    for (int i = 0; i < v.size() && i < num; ++i) { 
        ::Property *ex = (::Property*)g_ptr_array_index(kinds->ex_props, i);
	::Property* inner = ex->ops->copy(ex);
	dia::IProperty *from = v[i];
	int vi;
	double vd;
	const char* vcs;
	char* vs;
	const std::vector<dia::IProperty*>* vv;
	
	if (!from)
	    /* nothing to do, alread initialized */;
	else if (from->get (&vi))
	    set_prop (inner, vi);
	else if (from->get (&vd))
	    set_prop (inner, vi);
	else if (from->get (&vcs))
	    set_prop (inner, vcs);
	else if (from->get (&vs))
	    set_prop (inner, vs);
	else if (from->get (&vv)) {
	    ArrayProperty* ap = (ArrayProperty*)inner;
	    GPtrArray* record = g_ptr_array_new ();
	    if (!ap->ex_props) { //FIXME: bug in bindings?
	        // not sure if this is the right thing to do, reusing kinds
		if (!set_prop (to, kinds, vv->size(), *vv))
		{
		    g_warning ("Type mismatch vector<>[%d] '%s'", vv->size(), ap->common.descr->name);
		    g_ptr_array_free (record, TRUE);
		    return false;
		}
		return true;
	    } else {
	        set_prop (record, ap, ap->ex_props->len, *vv);
	        g_ptr_array_add(ap->records, record);
	    }
	}
        g_ptr_array_add(to, inner);
    }
    return true;
}
//! add a property of type int
int 
dia::Properties::setitem (const char* s, int n)
{
    g_return_val_if_fail (object != NULL, -1);
    ::Property *p = object_prop_by_name (object, s);
    if (p) {
        bool apply = true;
        if (!set_prop (p, n))
            printf ("dia::Properties::setitem (%s, %d) type mismatch (%s)\n", s, n, p->descr->type), apply = false;
	if (apply) {
	    GPtrArray *plist = prop_list_from_single (p);
	    object->ops->set_props(object, plist);
            prop_list_free (plist);
	}
	return 0;
    }
    printf ("dia::Properties::setitem (%s, %d) none such\n", s, n);
    return -1;
}
//! add a property of type double
int
dia::Properties::setitem (const char* s, double n)
{
    g_return_val_if_fail (object != NULL, -1);
    ::Property *p = object_prop_by_name (object, s);
    if (p) {
        bool apply = true;
        if (!set_prop (p, n))
            printf ("dia::Properties::setitem (%s, %f) type mismatch (%s)\n", s, n, p->descr->type), apply = false;
	if (apply) {
	    GPtrArray *plist = prop_list_from_single (p);
	    object->ops->set_props(object, plist);
            prop_list_free (plist);
	}
	return 0;
    }
    printf ("dia::Properties::setitem (%s, %f) none such\n", s, n);
    return -1;
}
//! add a property of type string
int 
dia::Properties::setitem (const char* s, const char* v)
{
    g_return_val_if_fail (object != NULL, -1);
    //printf ("0x%08X->properties['%s'] = '%s'\n", object, s, v);
    ::Property *p = object_prop_by_name (object, s);
    if (p) {
        bool apply = true;
        if (!set_prop (p, v))
            printf ("dia::Properties::setitem (%s, %s) type mismatch (%s)\n", s, v, p->descr->type), apply = false;

	if (apply) {
	    GPtrArray *plist = prop_list_from_single (p);
	    object->ops->set_props(object, plist);
            prop_list_free (plist);
	}
	return 0;
    }
    printf ("dia::Properties::setitem (%s, %s) none such\n", s, v);
    return -1;
}
//! add a property of type double list
int 
dia::Properties::setitem (const char* s, const std::vector<double>& v)
{
    g_return_val_if_fail (object != NULL, -1);
    ::Property *p = object_prop_by_name (object, s);
    if (p) {
        bool apply = true;
        if (strcmp (p->descr->type, PROP_TYPE_COLOUR) == 0 && v.size() == 4) {
            ((ColorProperty*)p)->color_data.red   = v[0]; 
            ((ColorProperty*)p)->color_data.green = v[1]; 
            ((ColorProperty*)p)->color_data.blue  = v[2];
            ((ColorProperty*)p)->color_data.alpha = v[3];
	}
	else if (strcmp (p->descr->type, PROP_TYPE_LINESTYLE) == 0 && v.size() == 2) {
	    ((LinestyleProperty *)p)->style = (::LineStyle)(int)v[0];
	    ((LinestyleProperty *)p)->dash = v[1];
	}
	else if (strcmp (p->descr->type, PROP_TYPE_POINT) == 0 && v.size() == 2) {
	    ((PointProperty *)p)->point_data.x = v[0];
	    ((PointProperty *)p)->point_data.y = v[1];
	}
	else if (strcmp (p->descr->type, PROP_TYPE_POINTARRAY) == 0) {
	    PointarrayProperty *ptp = (PointarrayProperty *)p;
	    int len = v.size() / 2;
            g_array_set_size(ptp->pointarray_data,len);
	    for (int i = 0; i < len; ++i) {
	        ::Point pt = {v[(i<<1)], v[(i<<1)+1]};
		g_array_index(ptp->pointarray_data,Point,i) = pt;
	    }
	}
	else if (strcmp (p->descr->type, PROP_TYPE_BEZPOINTARRAY) == 0) {
            BezPointarrayProperty *ptp = (BezPointarrayProperty *)p;
	    // in the folded list the (variable size!) data is packed. So we need to calculate the real length during unpacking
	    int i = 0;
	    g_array_set_size(ptp->bezpointarray_data, v.size() / 3); // worst case
	    std::vector<double>::const_iterator it;
	    for (it = v.begin(); it != v.end(); ++it) {
	        ::BezPoint bpt;
		g_return_val_if_fail ((int)(*it) >= ::BezPoint::BEZ_MOVE_TO && (int)(*it) <= ::BezPoint::BEZ_CURVE_TO, -1);
		bpt.type = (int)(*it) == ::BezPoint::BEZ_MOVE_TO ? ::BezPoint::BEZ_MOVE_TO :
		           (int)(*it) == ::BezPoint::BEZ_LINE_TO ? ::BezPoint::BEZ_LINE_TO : ::BezPoint::BEZ_CURVE_TO;
		++it; bpt.p1.x = *it;
		++it; bpt.p1.y = *it;
	        if (bpt.type == ::BezPoint::BEZ_CURVE_TO) {
		    ++it; bpt.p2.x = *it;
		    ++it; bpt.p2.y = *it;
		    ++it; bpt.p3.x = *it;
		    ++it; bpt.p3.y = *it;
		}
		else {
		    if (0 == i && bpt.type != ::BezPoint::BEZ_MOVE_TO)
		        g_warning("First bezpoint must be BEZ_MOVE_TO");
                    if (0 < i && bpt.type != ::BezPoint::BEZ_LINE_TO)
                        g_warning("Further bezpoint must be BEZ_LINE_TO or BEZ_CURVE_TO");
		    bpt.p2 = bpt.p3 = bpt.p1; // not strictly needed
		}
		g_array_index(ptp->bezpointarray_data,BezPoint,i) = bpt;
		++i;
	    }
	    g_array_set_size(ptp->bezpointarray_data, i); // worst case
	}
	else
            printf ("dia::Properties::setitem (%s, std::vector<double>) type or size (%d) mismatch (%s)\n", 
	            s, static_cast<int>(v.size()), p->descr->type), apply = false;
	if (apply) {
	    GPtrArray *plist = prop_list_from_single (p);
	    object->ops->set_props(object, plist);
            prop_list_free (plist);
	}
        return 0;
    }
    printf ("dia::Properties::setitem (%s, std::vector<double>&) none such\n", s);
    return -1;
}
/*!
 * \brief set a list of IProperty* which in itself may have std::vector<dia::IProperty*>
 *
 */
int
dia::Properties::setitem (const char* s, const std::vector<dia::IProperty*>& vec)
{
    g_return_val_if_fail (object != NULL, -1);
    ::ArrayProperty *pvec = (ArrayProperty *)object_prop_by_name (object, s);
    if (pvec) {
        guint i, num_props = pvec->ex_props->len;
	// clean the previos records
	for (i = 0; i < pvec->records->len; ++i) {
	    GPtrArray* record = (GPtrArray*)g_ptr_array_index(pvec->records, i);
            for (int j = 0; j < num_props; ++j) {
	        ::Property* inner = (::Property*)g_ptr_array_index(record,j);
	        inner->ops->free(inner);
            }
            g_ptr_array_free(record, TRUE);
        }
        g_ptr_array_set_size(pvec->records, 0);
        // and fill with new values
        for (i = 0; i < vec.size(); ++i) {
	    // we can only get lists here, every list gives a record and must match in size
	    const std::vector<dia::IProperty*>* vv;
	     
	    if (vec[i]->get (&vv)) {
	        if (vv->size() != num_props)
	            g_warning ("dia::Properties::setitem() inner list of wrong size");
		GPtrArray *record = g_ptr_array_new();
		set_prop (record, pvec, num_props, *vv);
                g_ptr_array_add(pvec->records, record);
	     }
	}
	GPtrArray *plist = prop_list_from_single ((::Property*)pvec);
        object->ops->set_props(object, plist);
        prop_list_free (plist);

	return true;
    }
    printf ("dia::Properties::setitem (%s, std::vector<std::IProperty*>&) none such\n", s);
    return -1;
}
/*!
 * \brief dictionary lookup for a property given by name
 *
 * It may be desired to make all these properties directly
 * available as attribute, so instead of (Python)
 * \code
 *   w = o.properties["elem_width"].value
 * \endcode
 * one could write
 * \code
 *   w = o.elem_height
 * \endcode
 * OTOH this would remove the possibility to do reflection on 
 * the properties, e.g. asking them for type and name. Also it 
 * may produce namespace clashes for more common property names
 * like 'name'.
 */
dia::IProperty* 
dia::Properties::getitem (const char* name) const
{
    g_return_val_if_fail (object != NULL, 0);
    ::Property *p = object_prop_by_name (object, name);
    if (p)
        return new dia::Property< ::Property* >(p);
    return 0;
}

//! give all the keys in this map
std::vector<char*>
dia::Properties::keys () const
{
    g_return_val_if_fail (object != NULL, _keys);
    
    if (!_keys.empty())
        return _keys;
    if (object->ops->describe_props) {
        const PropDescription *desc = object->ops->describe_props(object);
        for (int i = 0; desc[i].name; i++) {
             if ((desc[i].flags & PROP_FLAG_WIDGET_ONLY) == 0)
	         // conceptionally const, just building our cache
	         const_cast<Properties*>(this)->_keys.push_back (const_cast<char*>(desc[i].name));
	}
    }
    return _keys;
}

