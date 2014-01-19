/*
 * C++ interface of DiagramData - just to have something clean to wrap
 *
 * Copyright 2007, Hans Breuer, GPL, see COPYING
 */
#ifndef DIA__DIAGRAMDATA_H
#define DIA__DIAGRAMDATA_H

#include "dia-object.h"

//! every wrapping object is in the 'dia' namespace
//! the C-Objects are only namespaced in the binding 
namespace dia {

class Object;

/*!
 * \brief A collection of objects
 *
 * Maybe this should be turned into a template class cause it would need different owners
 * depending on its use. The strongest connection is between Object and Layer but there
 * are other collections of of Objects like DiagramData::selected or even Group
 */
class Objects
{
public :
    //! don't use
    Objects () : list(0) {}

    //! construct from Object owning objects list
    Objects (GList** o) : list(o) {}
    
    /* array access */
    //! how many contained
    int len () const;
    //! return back a single one
    Object* getitem (int) const;
private :
    GList** list;
};

/*!
 * \brief Proxy to the Layer object
 *
 * A Layer is the connection between DiagramData and Object. 
 * DiagramData has Layers and a Layer has Objects.
 */
class Layer
{
public :
    //! construct from underlying type: can not be created stand-alone yet.
    explicit Layer (::Layer* layer);
    //! destroy caches etc.
    ~Layer ();
    //! the underlying type - only for implementation, not visible in th elanguage binding 
    ::Layer* Self () const { return self; }
    //! collection of contained Object, read-only!
    const Objects* objects;
    //! add an object to our collection
    void add_object (Object* o);
    //! recalculation of every Object size and position to update the Layer extents
    void update_extents ();
    //! the object next to given point but within maxdist
    Object* find_closest_object (::Point* pos, double maxdist) const;
    //! a list of Object in the given rectangle
    Objects& find_objects_in_rectangle (::Rectangle* rect) const;
    //! objects are kept in an ordered list, this is the index of the given object
    int object_index (Object* o) const;

    //! again I would like easy accessor mapping in SWIG
    const char* name;
private :
    ::Layer* self;
    Objects* _found; //< backing store for find_objects_in_rectangle
};

//! \brief container for layer, needed due to msvc6 lacking partial specialization
class Layers
{
public :
    //! don't use
    Layers () : list(0) {}
    //! construct from Dia's container
    Layers (GPtrArray** os) : list(os) {}
    /* array access */
    //! how many contained
    int len () const;
    //! return back a single one
    Layer* getitem (int) const;
private :
    GPtrArray** list;
};

/*!
 * \brief DiagramData is the low level Diagram, i.e. everything without an UI
 *
 * The base class for Diagram. The top-level object for libdia containing
 * all the other diagram objects.
 */
class DiagramData
{
public :
    //! the size of the diagram, from last DiagramData::update_extents();
#ifdef SWIG
    const // this is read only from Python , too. But gcc does not like it that way 
          // error: uninitialized member 'dia::DiagramData::extents' with 'const' type 'const Rectangle'
#endif
    ::Rectangle extents;
    //! trying to be compatible, read-only
    const Layer* active_layer;
    //! the read-only list of layers, \todo typemap it in SWIG
    const Layers* layers;
    //! can be created as stand-alone object
    DiagramData ();
    //! done with it
    ~DiagramData ();
    //! get the underlying representation
    ::DiagramData* Self () const { return self; }
    //! factory function for Layer. They can't stand alone. TODO: decide if this should change the active_layer
    Layer* add_layer (const char* name);
    //! recalculation of every object to update the diagram extents
    void update_extents ();
    //! the currently selected objects
    Objects& get_sorted_selected () const;
private :
    ::DiagramData* self;
    Objects* _selected; //!< backing store for get_sorted_selected
};

} // namespace dia
#endif /* DIA__DIAGRAMDATA_H */
