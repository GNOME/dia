/*
 * C++ interface of Dia Properties - just to have something clean to wrap
 *
 * Copyright 2007, Hans Breuer, GPL, see COPYING
 */
#ifndef DIA__PROPERTIES_H
#define DIA__PROPERTIES_H

#include <vector>

#include "properties.h"

namespace dia {

/*!
 * \brief Property is the representation of one attribute of an Object
 *
 * The abstract interface is needed to build a vector of Property. Real properties
 * are implemented as template
 */
class IProperty
{
public :
    //! \internal to make delete delete the real thing
    virtual ~IProperty () {}
    //! getter depending on the type of the property
    virtual bool get (int*) const { return false; }
    //! the concrete implementation must deliver the underlying type
    virtual bool get (double*) const { return false; }
    //! and return true just for that one
    virtual bool get (const char**) const { return false; }
    //! \todo this seems to be needed due to SWIG's code generattion?
    //! It gives: dia-properties.h(30): Warning(509): Overloaded get(char **) const is shadowed by
    //! get(char const **) const at dia-properties.h:28.
    //! but without it the setting of UMLClass:attributes and members does not work.
    //! so before removing it check 'test.py --doc'
    virtual bool get (char**) const { return false; }
    //! Now it starts to become ugly: isn't there a better way with SWIG to map one to many types?
    virtual bool get (::_Point* p) const { return false; }
    //! one more ;)
    virtual bool get (::_Rectangle* p) const { return false; }
    //! and this one ...
    virtual bool get (::_Color* p) const { return false; }

    //! we can also handle vector<IProperty*>
    virtual bool get (const std::vector<IProperty*>**) const { return false; }
    
    //! read-only attribute; the name of the property; at least unique within an object, but supposed to be unique global 
    virtual const char* get_name () const = 0;
    //! read-only attribute giving the data type represented
    virtual const char* get_type () const = 0;
    //! if the property is to be shown
    virtual bool visible () const = 0;
};
/*!
 * \brief To construct any Property type from its underlying type.
 *
 * This class is used to construct a more complex property and put it into a container of 
 * properties without having access to the underlying ::Property type.
 */
template <class T>
class Property : public IProperty
{
public :
    //! construct from underlying data type
    Property<T> (T v) : self(v) {}
    //! the on get function which returns true, 
    //! because the underlying type matches
    virtual bool get (T* p) const { *p = self; return true; }
    
    //! read-only attribute; the name of the property; at least unique within an object, 
    //! but supposed to be unique globally 
    const char* get_name () const { return "<name>"; }
    //! read-only attribute giving the data type represented
    const char* get_type () const { return "<type>"; }
    //! if the property is to be shown
    bool visible () const { return false; }
private :
    T self;
};

template<> inline bool Property<int>::get (int* p) const 
{ 
  *p = self; return true; 
}
template<> inline bool Property<double>::get (double* p) const 
{ 
  *p = self; return true; 
}
template<> inline Property<const char*>::Property (const char* v) 
{ 
  self = v ? g_strdup (v) : g_strdup(""); 
}
template<> inline bool Property<const char*>::get (const char** p) const 
{ 
  *p = self; return true; 
}
template<> inline Property<char*>::Property (char* v) 
{ 
  self = v ? g_strdup (v) : g_strdup(""); 
}
template<> inline bool Property<char*>::get (char** p) const 
{ 
  *p = self; return true; 
}

/*!
 * \brief specialization for getter; a Property from underlying ::Property
 *
 * The Dia Property System can handle various kinds of properties from simple int
 * over Point and Rectangle up to whole Property lists and trees. This class is 
 * used to get or read a property from its underlying type.
 */
template<>
class Property< ::Property* > : public IProperty
{
public :
    //! construct from underlying object
    Property (::Property* p);
    //! destroy chache
    ~Property ();

    //! getter depending on the type of the property
    virtual bool get (int*) const;
    //! the concrete implementation must deliver the underlying type
    virtual bool get (double*) const;
    //! and return true just for that one
    virtual bool get (const char**) const;
    //! just one wrapper too much ;)
    virtual bool get (::_Point* p) const;
    //! one more ;)
    virtual bool get (::_Rectangle* p) const;
    //! and this one ...
    virtual bool get (::_Color* p) const;
    //! we can also handle vector<IProperty*>
    virtual bool get (const std::vector<IProperty*>**) const;


    //! read-only attribute; the name of the property; at least unique within an object, but supposed to be unique global 
    const char* get_name () const;
    //! read-only attribute giving the data type represented
    const char* get_type () const;
    //! if the property is to be shown
    bool visible () const;
private :
    ::Property* self;
    // this is ugly but otherwise we would have an unbound leak
    std::vector<IProperty*> _vec;
};
/*!
 * \brief a list of properties; can contain list again so in fact it can be a tree
 *
 * Some Object Properties consist of Property lists or even property trees. This is the basic 
 * building block for these complex properties. See 'UML - Class'::operations for an example.
 */
template<>
class Property< std::vector<IProperty*> > : public IProperty, public std::vector<IProperty*>
{
public :
    //! create an empty list
    Property () {}
    //! empty our list knd of 'deep delete'
    ~Property ()
    {
	std::vector<IProperty*>::iterator it;

	for (it = begin(); it != end(); ++it)
		delete *it;
    }
    //! get us self
    virtual bool get (const std::vector<IProperty*>** p) const 
    {
        *p = this;
	return true;
    }
    //! read-only attribute; the name of the property; at least unique within an object, but supposed to be unique global 
    const char* get_name () const { return ""; }
    //! read-only attribute giving the data type represented
    const char* get_type () const { return "vector<IProperty*>"; }
    //! if the property is to be shown
    bool visible () const { return false; }
};
/*!
 * \brief A container for Property, merely a fassade to make a group of properties more accessible
 *
 * In Python this is mapped to the dictionary type, accessing a specific property is done by
 * a key lookup with it's name.
 *
 * To modify an objects property it is needed to modify it's dictionary. This is due to the fact
 * that single properties don't have a connection to a specific object but the property map has.
 */
class Properties
{
public :
    //! construct with/from owning object
    Properties (::DiaObject*);
    //! destroy the wrapper
    ~Properties ();
    /* accessors like Python dictionary */
    //! is there a property of this name
    bool has_key (const char*) const;
    
    //! add a property of type int
    int setitem (const char*, int);
    //! add a property of type double
    int setitem (const char*, double);
    //! add a property of type string
    int setitem (const char*, const char*);
    //! add a property of type double list
    int setitem (const char*, const std::vector<double>&);
    //! add a property list - intented as final falback
    int setitem (const char* s, const std::vector<IProperty*>& vec);
    //! return back a single property
    IProperty* getitem (const char*) const;
    //! give all the keys in this map
    std::vector<char*> keys () const;
private :
    ::DiaObject* object; //!< the owner of the properties
    
    std::vector<char*> _keys; //!< lazy created
};

} // namespace dia
#endif /* DIA__PROPERTIES_H */
