/* Dia -- a diagram creation/manipulation program -*- c -*-
 * Copyright (C) 1998 Alexander Larsson
 *
 * Property system for dia objects/shapes.
 * Copyright (C) 2000 James Henstridge
 * Copyright (C) 2001 Cyrille Chepelov
 * Major restructuration done in August 2001 by C. Chepelov
 *
 * Basic Property types definition. 
 *
 * This program is free software; you can redistribute it and/or modify
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

#ifndef PROP_BASIC_H
#define PROP_BASIC_H

#include "properties.h"
#include "dia_xml.h"

void initialize_property(Property *prop, const PropDescription *pdesc,
                         PropDescToPropPredicate reason);
void copy_init_property(Property *dest, const Property *src);

typedef struct {
  Property common;
} NoopProperty;

NoopProperty *noopprop_new(const PropDescription *pdesc, 
                           PropDescToPropPredicate reason);
void noopprop_free(NoopProperty *prop);
NoopProperty *noopprop_copy(NoopProperty *src);
WIDGET *noopprop_get_widget(NoopProperty *prop, PropDialog *dialog);
void noopprop_reset_widget(NoopProperty *prop, WIDGET *widget);
void noopprop_set_from_widget(NoopProperty *prop, WIDGET *widget);
void noopprop_load(NoopProperty *prop, AttributeNode attr, DataNode data, DiaContext *ctx);
void noopprop_save(NoopProperty *prop, AttributeNode attr, DiaContext *ctx);
gboolean noopprop_can_merge(const PropDescription *pd1, 
                            const PropDescription *pd2);
gboolean noopprop_cannot_merge(const PropDescription *pd1, 
                               const PropDescription *pd2);
void noopprop_get_from_offset(const NoopProperty *prop,
                              void *base, guint offset, guint offset2);
void noopprop_set_from_offset(NoopProperty *prop,
                              void *base, guint offset, guint offset2);

typedef struct {
  Property common;
} InvalidProperty;

InvalidProperty *invalidprop_new(const PropDescription *pdesc, 
                                 PropDescToPropPredicate reason);
void invalidprop_free(InvalidProperty *prop);
InvalidProperty *invalidprop_copy(InvalidProperty *src);
WIDGET *invalidprop_get_widget(InvalidProperty *prop, PropDialog *dialog);
void invalidprop_reset_widget(InvalidProperty *prop, WIDGET *widget);
void invalidprop_set_from_widget(InvalidProperty *prop, WIDGET *widget);
void invalidprop_load(InvalidProperty *prop, AttributeNode attr, 
                      DataNode data, DiaContext *ctx);
void invalidprop_save(InvalidProperty *prop, AttributeNode attr, DiaContext *ctx);
gboolean invalidprop_can_merge(const PropDescription *pd1, 
                               const PropDescription *pd2);
void invalidprop_get_from_offset(const InvalidProperty *prop,
                                 void *base, guint offset, guint offset2);
void invalidprop_set_from_offset(InvalidProperty *prop,
                                 void *base, guint offset, guint offset2);
typedef struct {
  Property common;
} UnimplementedProperty;

UnimplementedProperty *unimplementedprop_new(const PropDescription *pdesc, 
                                             PropDescToPropPredicate reason);
void unimplementedprop_free(UnimplementedProperty *prop);
UnimplementedProperty *unimplementedprop_copy(UnimplementedProperty *src);
WIDGET *unimplementedprop_get_widget(UnimplementedProperty *prop, 
                                     PropDialog *dialog);
void unimplementedprop_reset_widget(UnimplementedProperty *prop, 
                                    WIDGET *widget); 
void unimplementedprop_set_from_widget(UnimplementedProperty *prop, 
                                       WIDGET *widget);
void unimplementedprop_load(UnimplementedProperty *prop, 
                            AttributeNode attr, DataNode data, DiaContext *ctx); 
void unimplementedprop_save(UnimplementedProperty *prop, AttributeNode attr, DiaContext *ctx);
gboolean unimplementedprop_can_merge(const PropDescription *pd1, 
                                     const PropDescription *pd2);
void unimplementedprop_get_from_offset(const UnimplementedProperty *prop,
                                       void *base, 
                                       guint offset, guint offset2); 

void unimplementedprop_set_from_offset(UnimplementedProperty *prop,
                                       void *base, 
                                       guint offset, guint offset2);
void prop_basic_register(void);

#endif
