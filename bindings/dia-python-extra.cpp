/* 
 * some helper function not to pollute the multilingual dia.swig file 
 *
 * Implementation Note: this file should not deal with Dia's Type's but only with some
 * conversions from complex Python types to std::types 
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

#include <Python.h>
#include <assert.h>

#include "dia-python-extra.h"

/* 
 * \brief Try to convert a 'nested' Python Object into a flat vector.
 *
 * With templates not working that well betwenn SWIG and msvc6 here's some manual conversion.
 * Basically this is 'unrolling' tuples and lists to a flat std::vector and delegating that 
 * to the C++ object, which than converts again to the concrete properties type.
 * As a short-cut int is converted to double if the first or last element is a double.
 * Yes I know this code is ugly. Let's see if other language binders do better ;)
  */
bool 
DiaPythonExtra_ToVector (PyObject* v, std::vector<double>& vec)
{
    if ((PyList_Check(v) && PyList_Size(v) > 0) || (PyTuple_Check(v) && PyTuple_Size(v) > 0)) {
        bool isList = PyList_Check(v);
	int size = isList ? PyList_Size(v) : PyTuple_Size (v);
	
	for (int i = 0; i < size; ++i) {
	    PyObject *a = isList ? PyList_GetItem(v, i) : PyTuple_GetItem (v, i);
	    if (PyFloat_Check(a) || PyInt_Check(a))
	        vec.push_back (PyFloat_Check(a) ? PyFloat_AsDouble(a) : PyInt_AsLong(a));
	    else if (PyTuple_Check (a)) {
	        for (int j = 0; j < PyTuple_Size (a); ++j) {
	            PyObject* b = PyTuple_GetItem (a, j);
		    if (!PyFloat_Check(b) && !PyInt_Check (b))
		        return false; // wrong type
	            vec.push_back (PyFloat_Check(b) ? PyFloat_AsDouble(b) : PyInt_AsLong(b));
	        }
	    } else {
	        return false; // only one level of nesting supported
	    }
	}
	return true;
    }
    return false;
}

/*!
 * \brief recursive function doing the real conversion work for tree<IProperty*>
 *
 * \return false on no vector input
 */
static bool
_ToVector (PyObject* v, std::vector<dia::IProperty*>& vec)
{
    bool isList = PyList_Check(v);
    bool isTuple = PyTuple_Check(v);
 
    if (isList || isTuple) {
	int len = isList ? PyList_Size(v): PyTuple_Size(v);

	for (int i = 0; i < len; ++i) {
            PyObject *o = isList ? PyList_GetItem(v, i) : PyTuple_GetItem(v, i);
	    if (PyString_Check (o))
		vec.push_back (new dia::Property<char*>(PyString_AsString (o)));
	    else if (PyUnicode_Check (o)) 
	    {
		PyObject *uval = PyUnicode_AsUTF8String (o);
		vec.push_back (new dia::Property<char*>(PyString_AsString(uval)));
		Py_DECREF (uval); 
	    }
	    else if (PyList_Check(o) || PyTuple_Check(o)) {
	        // dive into
	        dia::Property< std::vector<dia::IProperty*> >* inner = 
			new dia::Property< std::vector<dia::IProperty*> >();
	        _ToVector (o, *inner); 
		vec.push_back (inner);
	    } else if (PyInt_Check(o))
		vec.push_back (new dia::Property<int>(PyInt_AsLong (o)));
	    else if (PyFloat_Check (o))
		vec.push_back (new dia::Property<double>(PyFloat_AsDouble (o)));
	    else if (Py_None == o)
		vec.push_back (0);
	    else
	    {
		PyObject* s = PyObject_Str (o);
		printf ("_ToVector() : no conversion for '%s'\n", PyString_AsString (s));
	    }
	}
	return true;
    }
    return false;
}

/*!
 * \brief Generic way to create a property vector/tree
 *
 * To allow the assignment of complex properties built from native language
 * types some binding language specific support is needed.
 */
bool 
DiaPythonExtra_ToVector (PyObject* v, std::vector<dia::IProperty*>& vec)
{
    // we can deal handle empty lists/tuples as well
    if (PyList_Check(v) || PyTuple_Check(v))
        return _ToVector (v, vec);
    return false;
}
