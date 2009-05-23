/*
 * C++ wrapper of DiagramData - just to have something clean to wrap
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
#include "diagramdata.h"
#include <assert.h>

#include "dia-object.h"

#include "dia-diagramdata.h"

//! how many contained
int 
dia::Objects::len () const
{
    if (list)
        return g_list_length(*list);
    return 0;
}
//! \brief return back a single one
//! Together with len() getitem() allows list access. The code generator needs some
//! extra help to not making it an endless list. At least for Python the NULL return
//! was turned into None objects, we want to have only as much list elements 
//! as there are real objects.
dia::Object* 
dia::Objects::getitem (int n) const
{
    if (list && n >= 0 && n < g_list_length(*list))
        return new dia::Object((::DiaObject*)g_list_nth_data(*list, n));
    return 0;
}

//! construct with underlying list of objects
//! \todo check ownership/lifetime of that list!
dia::Layer::Layer (::Layer* layer) : self(layer), _found(0) 
{
    assert (self);
    // todo: when we allow to change the name this needs to change
    name = g_strdup (self->name);
    objects = new dia::Objects(&(layer->objects));
}
dia::Layer::~Layer ()
{
    if (_found)
        delete _found;
    if (name)
        g_free (const_cast<gchar*>(name));
    if (objects)
      delete const_cast<dia::Objects*>(objects);
}
void
dia::Layer::add_object (Object* o)
{
    g_return_if_fail (self != NULL);
    layer_add_object (self, o->Self());
}
void
dia::Layer::update_extents ()
{
    g_return_if_fail (self != NULL);
    layer_update_extents(self);
}
//! the object next to given point but within maxdist
dia::Object* 
dia::Layer::find_closest_object (::Point* pos, double maxdist) const
{
    g_return_val_if_fail (self != NULL, 0);
    ::DiaObject* o = layer_find_closest_object (self, pos, maxdist);
    if (o)
      return new dia::Object (o);
    return 0;
}
//! a list of Object in the given rectangle
dia::Objects& 
dia::Layer::find_objects_in_rectangle (::Rectangle* rect) const
{
    static GList* list = layer_find_objects_in_rectangle (self, rect);
    if (_found)
        delete _found;
    const_cast<Layer*>(this)->_found = new dia::Objects (&list);
    return *_found;
}
 //! objects are kept in an ordered list, this is the index of the given object
int 
dia::Layer::object_index (Object* o) const
{
    g_return_val_if_fail (self != NULL, -1);
    return layer_object_get_index (self, o->Self());
}

int
dia::Layers::len () const
{
    if (list)
        return (*list)->len;
    return 0;
}
dia::Layer*
dia::Layers::getitem (int n) const
{
  if (list && n >= 0 && n < (*list)->len)
    return new dia::Layer(static_cast< ::Layer* >(g_ptr_array_index (*list, n)));
  return 0;
}

/*!
 * DiagramData is the low level Diagram, i.e. everything without an UI
 */
dia::DiagramData::DiagramData () : self(0), active_layer(0), _selected(0)
{
    // the usual GObjetc cast DIA_DIAGRAM_DATA does not work when playing games with namespaces ;)
    self = static_cast< ::DiagramData* > (g_object_new (DIA_TYPE_DIAGRAM_DATA, NULL));
    //FIXME: grumpf
    if (self->active_layer)
      active_layer = new Layer(self->active_layer);
    else if (self->layers->len > 0)
      active_layer = new Layer((::Layer*)g_ptr_array_index(self->layers, 0));
    layers = new Layers(&self->layers);
}
dia::DiagramData::~DiagramData ()
{
    g_object_unref (self);
    if (_selected)
        delete _selected;
    if (layers)
      delete const_cast< dia::Layers* >(layers);
}

dia::Layer*
dia::DiagramData::add_layer (const char* name)
{
    int pos = -1; //TODO: make this a parameter
    g_return_val_if_fail (self != NULL, 0);
    ::Layer* layer = new_layer(g_strdup(name),self);
    if (pos != -1)
	data_add_layer_at(self, layer, pos);
    else
	data_add_layer(self, layer);

    dia::Layer* dl = new dia::Layer (layer);
    return dl;
}
void 
dia::DiagramData::update_extents ()
{
    g_return_if_fail(self != NULL);
    data_update_extents(self);
    // conceptionally const, i.e. read-only
    *const_cast< ::Rectangle* >(&extents) = self->extents;
}
dia::Objects&
dia::DiagramData::get_sorted_selected () const
{
    if (_selected)
        delete _selected;

    static GList* list = data_get_sorted_selected(self);
    const_cast< DiagramData* >(this)->_selected = new dia::Objects (&list);
    return *_selected;
}

