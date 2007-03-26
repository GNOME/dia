/*
 * C++ interface of Dia Connections - just to have something clean to wrap
 *
 * Copyright 2007, Hans Breuer, GPL, see COPYING
 */
#ifndef DIA__CONNECTIONS_H
#define DIA__CONNECTIONS_H

#include "dia-diagramdata.h"

namespace dia {

// forward declare
class Object;

/*!
 * \brief Where a connection and an object meet there is a ConnectionPoint
 *
 * The ConnectionPoint is one part to do a connection. The other part is Handle.
 */
class ConnectionPoint
{
public :
    //! construct from underlying data type
    ConnectionPoint (::ConnectionPoint* p);
    //! destruct 
    ~ConnectionPoint ();

    //! FIXME: trying to be compatible
    //! read-only attribute;
    const dia::Objects connected;
    //! read-only attribute: placement
    _Point pos () const;
    //! the object this is belonging to. To follow a connection you go from Handle.connected_to -> ConnectionPoint.object
    Object* get_object () const;
    
    //! get on the underlying object
    ::ConnectionPoint* Self() const { return self; }
private :
    ::ConnectionPoint* self;
};

/*!
 * \brief A container for ConnectionPoints, provides array access to them
 *
 */
class Connections
{
public :
    //! construct from owning Object
    Connections (::DiaObject*);
    //! destroy the wrapper, not the underlying type
    ~Connections ();
    /* accessors like Python sequence */
    //! how many contained
    int len () const;
    //! return back a single property
    ConnectionPoint* getitem (int) const;

private :
    ::DiaObject* object; //!< the owner of the connections
};
/*!
 * \brief With a Handle you have a grip on an Object
 *
 * Together with ConnectionPoint a Handle allows to connect objects.
 */
class Handle
{
public :
    //! construct from wrapped
    Handle (_Handle* h, _DiaObject* o) : self(h), owner(o) {}
    //! indirectly delivers the object this is connected to, if at all
    const ConnectionPoint* get_connected_to() const;
    //! establish a connection to another object
    void connect (ConnectionPoint*);

    //! convert to underlying
    ::_Handle* Self() const { return self; }    
private :
    ::_Handle* self;
    ::_DiaObject* owner;
};
/*!
 * \brief Collection of Handle
 */
class Handles
{
public :
    //! construct from owner
    Handles (::DiaObject*);
    //! destroy
    ~Handles ();

    /* array access */
    //! how many contained
    int len () const;
    //! return back a single one
    Handle* getitem (int) const;
    
private :
    ::DiaObject* object;
};

} // namespace dia
#endif /* DIA__CONNECTIONS_H */
