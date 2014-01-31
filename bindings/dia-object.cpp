/*
 * C++ interface of DiaObject - just to have something clean to wrap
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
#include <assert.h>

#include "dia-object.h"

dia::ObjectType::ObjectType (DiaObjectType* ot) : self(ot), version(ot ? ot->version : -1)
{
    assert (self);
    // read-only which also wont change
    name = self->name;
}
/*! Factory function to create Object of a given type.
 *
 * \return not only the new Object* but also two 'appropriate' Handle to e.g. connect 
 * to some other Object
 */
dia::Object* 
dia::ObjectType::create(double x, double y, dia::Handle** h1, dia::Handle** h2) const
{
    ::_Handle *_h1, *_h2;
    dia::Object* o;
    Point p = {x, y};
    void* user_data = 0; //TODO: as parameter ???
    if (!self)
        return 0;
    o = new dia::Object (self->ops->create(&p, user_data ? user_data : self->default_user_data, &_h1, &_h2));
    if (h1) *h1 = new dia::Handle (_h1, o->Self());
    if (h2) *h2 = new dia::Handle (_h2, o->Self());
    return o;
}
/*! 
 * Allows to load an object from storage. Not sure if this becomes useful for language bindings.
 */
dia::Object* 
dia::ObjectType::load (ObjectNode node, int version, DiaContext *ctx) const
{
    assert (self);
    return new dia::Object (self->ops->load (node, version, ctx));
}
/*! Allows to save an Object to ObjectNode file filename.
 * . Not sure if this becomes useful for language bindings.
 */
void 
dia::ObjectType::save (Object* o, ObjectNode node, const char* filename) const
{
    assert (self);
    DiaContext *ctx = dia_context_new ("Object save");
    self->ops->save (o->Self(), node, ctx);
    dia_context_reset (ctx);
    dia_context_release (ctx);
}
#if 0
//! OPTIONAL: opens the defaults dialog
GtkWidget* 
dia::ObjectType::get_defaults () const
{
    assert (self);
    if (!self->ops->get_defaults)
        return 0;
    return self->ops->get_defaults ();
}
#endif
//! OPTIONAL: apply changed defaults
void 
dia::ObjectType::apply_defaults ()
{
    assert (self);
    if (!self->ops->apply_defaults)
	return;
    self->ops->apply_defaults ();
}

//! create an object wrapper - object previously registered from a plug-in
dia::Object::Object (DiaObject* o) : self(o), properties(o), connections(o), handles(o), type(o->type)
{
}
//! destroy the right thing - usually not the underlying object
dia::Object::~Object ()
{
}
//! accessor to deliver the objects bounding box
::_Rectangle*
dia::Object::bbox () const
{
    return &self->bounding_box;
}
//! real destruction
void 
dia::Object::destroy ()
{
    assert (self);
    self->ops->destroy (self);
}
//! drawing the object on the renderer
void 
dia::Object::draw (Renderer* r) const
{
    assert (self);
#if 0 //FIXME: need to think about delegation
    self->ops->draw (self, r);
#endif
}
//! calculate the distance betwenn the object and the point
double 
dia::Object::distance_from (Point* p) const
{
    assert (self);
    return self->ops->distance_from (self, p);
}
//! react on selection (e.g. may change the drawing state)
void
dia::Object::select (Point* clicked_point, DiaRenderer* interactive_renderer)
{
    assert (self);
    self->ops->selectf (self, clicked_point, interactive_renderer);
}
//! create a deep copy
dia::Object* dia::Object::copy () const
{
    assert (self);
    return new dia::Object (self->ops->copy (self));
}
//! change position of the whole object
ObjectChange* 
dia::Object::move (double x, double y)
{
    assert (self);
    Point p = {x, y};
    return self->ops->move (self, &p);
}
//! change position of an object handle - this is usually  a resize (Attention: does *not* match the C prototype)
ObjectChange* 
dia::Object::move_handle (dia::Handle* h, double x, double y, HandleMoveReason reason, ModifierKeys modifiers)
{
    assert (self);
    Point p = {x, y};
    return self->ops->move_handle (self, h->Self(), &p, NULL, reason, modifiers);
}
//! OPTIONAL: provide a property dialog to change the object proeprties
GtkWidget* 
dia::Object::get_properties (bool is_default) const
{
    assert (self);
    if (!self->ops->get_properties)
	return 0;
    return self->ops->get_properties (self, is_default);
}
//! OPTIONAL: apply the properties changed in the dialog
ObjectChange* 
dia::Object::apply_properties (GtkWidget* w)
{
    assert (self);
    if (!self->ops->apply_properties_from_dialog)
	return NULL;
    return self->ops->apply_properties_from_dialog (self, w);
}
//! OPTIONAL: provide a context menu to change the object states
DiaMenu* 
dia::Object::get_object_menu (Point* pos) const
{
    assert (self);
    if (!self->ops->get_object_menu)
	return 0;
    return self->ops->get_object_menu (self, pos);
}
    
//! StdProps: descibe properties accessible via standard properties API
const PropDescription* 
dia::Object::describe_props () const
{
    assert (self);
    return self->ops->describe_props (self);
}
//! StdProps: return a list of properties describing the object state
void 
dia::Object::get_props (GPtrArray *props) const
{
    assert (self);
    self->ops->get_props (self, props);
}
//! StdProps: apply a list of object properties to change the objects state
void 
dia::Object::set_props (GPtrArray *props)
{
    assert (self);
    self->ops->set_props (self, props);
}

/*!
 * \brief factory function for ObjectType (Object factories)
 *
 * Before this function can return anything useful the ObjectType registry needs to be filled,
 * e.g. by dia::register_plugins()
 */
dia::ObjectType*
dia::get_object_type (const char* name)
{
    ::DiaObjectType* ot = object_get_type(const_cast<char*>(name));
    if (ot)
        return new dia::ObjectType (ot);
    return 0;
}
