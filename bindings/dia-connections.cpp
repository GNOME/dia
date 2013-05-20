/*
 * C++ implementation of Dia Connections - just proxies
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
#include "object.h"

#include "dia-object.h"

#include "dia-connections.h"
#include "dia-diagramdata.h"

#include "diagramdata.h"

dia::ConnectionPoint::ConnectionPoint (::ConnectionPoint* p) : self(p), connected(&p->connected) 
{
}

dia::ConnectionPoint::~ConnectionPoint () 
{ 
}
//! read-only attribute: placement
::_Point 
dia::ConnectionPoint::pos () const
{
    ::_Point dummy = {0, 0};
    g_return_val_if_fail (self, dummy);
    return self->pos;
}
//! the object this is belonging to. To follow a connection 
//! you go from Handle.connected_to -> ConnectionPoint.object
dia::Object* 
dia::ConnectionPoint::get_object () const
{
    g_return_val_if_fail (self, 0);
    return new dia::Object(self->object);
}

dia::Connections::Connections (::DiaObject* o) : object(o)
{
}
dia::Connections::~Connections ()
{
}
//! is there a property of this name
int 
dia::Connections::len () const
{
    if (object)
        return object->num_connections;
    return 0;
}
//! return back a single property
dia::ConnectionPoint* 
dia::Connections::getitem (int n) const
{
    g_return_val_if_fail (object, 0);
    if (n < 0 || n >= object->num_connections)
        return 0;
    return new dia::ConnectionPoint (object->connections[n]);
}
//! \todo find a way to SWIG pick up the accessors as attributes w/o
//! remaning the accessor and
//! %attribute(dia::Handle, dia::ConnectionPoint*, connected_to, get_connected_to);
const dia::ConnectionPoint* 
dia::Handle::get_connected_to() const
{
    g_return_val_if_fail (self, 0);
    if (!self->connected_to)
        return 0;
    return new ConnectionPoint(self->connected_to);
}
void
dia::Handle::connect (dia::ConnectionPoint* cp)
{
    g_return_if_fail (self != NULL);
    g_return_if_fail (owner != NULL);
    object_connect (owner, self, cp->Self());
}
//! construct from owner
dia::Handles::Handles (::DiaObject* o) : object(o)
{
}
//! destroy
dia::Handles::~Handles ()
{
}
//! how many contained
int 
dia::Handles::len () const
{
    g_return_val_if_fail (object != NULL, 0);
    return object->num_handles;
}
//! return back a single property
dia::Handle* 
dia::Handles::getitem (int n) const
{
    g_return_val_if_fail (object, 0);
    if (n < 0 || n >= object->num_handles)
        return 0;
    return new dia::Handle (object->handles[n], object);
}
