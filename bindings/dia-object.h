/*
 * C++ interface of DiaObject - just to have something clean to wrap
 *
 * Copyright 2007, Hans Breuer, GPL, see COPYING
 */
#ifndef DIA__OBJECT_H
#define DIA__OBJECT_H

#include "dia-properties.h"
#include "dia-connections.h"

namespace dia {

//! forward declare
class Object;
class Renderer;

/*!
 * \brief ObjectType is the factoy to create Object
 *
 * To get on all registered ObjectType use dia::registered_types() to create one specific Object 
 * factory use dia::get_object_type()
 */
class ObjectType
{
public :
    //! one-time initialized read-only
    const char* name;
    //! instead of doing this I'd like better accessor support in SWIG
    const int version;

    //! \internal construct from underlying type
    ObjectType (::DiaObjectType* ot);
    //! create a default initialized object 
    Object* create (double x, double y, dia::Handle** h1 = 0, dia::Handle** h2 = 0) const;
    //! load an object from storage
    Object* load (ObjectNode node, int version, DiaContext *ctx) const;
    //! save an object to file filename
    void save (Object* o, ObjectNode node, const char* filename) const;
#if 0 // nothing useful
    //! OPTIONAL: open the defaults dialog
    GtkWidget* get_defaults () const;
#endif
    //! OPTIONAL: apply changed defaults
    void apply_defaults ();
private :
    ::DiaObjectType* self;     
};

/*!
 * \brief The basic building block of diagrams
 *
 * This is a proxy class to objects coming from Dia's object factory. The latter is populate by all the objecct
 * implementations from objects
 */
class Object
{
public :
    //! trying to be compatible (otherwise this would not be public)
    const Properties properties;
    //! access to this objects connection points
    const Connections connections;
    //! access to this objects Handles
    const Handles handles;
    //! the wrapped type (factory)
    const ObjectType type;

    //! direct property access for compatibility and convenience
    ::_Rectangle* bbox () const;
    
    //! \internal create an object wrapper - object previously registered from a plug-in
    Object (DiaObject*);
    //! \internal destroying the proxy, not the underlying object
    ~Object ();
    //! \internal not to be wrapped - just used internally
    DiaObject* Self() const { return self; }
    //! real destruction
    void destroy ();
    //! drawing the object on the renderer
    void draw (Renderer* r) const;
    //! calculate the distance betwenn the object and the point
    double distance_from (Point* p) const;
    //! react on selection (e.g. may change the drawing state)
    void select (Point* clicked_point, DiaRenderer* interactive_renderer);
    //! create a deep copy
    Object* copy () const;
    //! change position of the whole object
    ObjectChange* move (double x, double y);
    //! change position of an object handle - this is usually  a resize (Attention: does *not* match the C prototype)
    ObjectChange* move_handle (Handle* h, double x, double y, HandleMoveReason reason, ModifierKeys modifiers);

    //! OPTIONAL: provide a property dialog to change the object proeprties
    GtkWidget* get_properties (bool is_default) const;
    //! OPTIONAL: apply the properties changed in the dialog
    ObjectChange* apply_properties (GtkWidget*);
    //! OPTIONAL: provide a context menu to change the object states
    DiaMenu* get_object_menu (Point* pos) const;
    
    //! StdProps: descibe properties accessible via standard properties API
    const PropDescription* describe_props () const;
    //! StdProps: return a list of properties describing the object state
    void get_props (GPtrArray *props) const;
    //! StdProps: apply a list of object properties to change the objects state
    void set_props (GPtrArray *props);
private :
   DiaObject* self;
};

//! factory function to get on the ObjectType
ObjectType* get_object_type (const char* name);

} // namespace dia
#endif /* DIA__OBJECT_H */
