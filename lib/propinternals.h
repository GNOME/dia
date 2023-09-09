/* Dia -- a diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * Property system for dia objects/shapes.
 * Copyright (C) 2000 James Henstridge
 * Copyright (C) 2001 Cyrille Chepelov
 * Major restructuration done in August 2001 by C. Chepelov
 *
 * propinternals.h: prototypes for routines which might be of use to other 
 * sections of the property support code (but whose use by external code is 
 * strongly discouraged !)
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
#ifndef PROPINTERNALS_H
#define PROPINTERNALS_H

#include <glib.h>

#include <gtk/gtk.h>
#include "properties.h" /* **after** gtk.h, please */

/* returns TRUE if two property descriptors describe properties 
   which can be merged. */
gboolean propdescs_can_be_merged(const PropDescription *p1, 
                                 const PropDescription *p2);

/* returns FALSE if error is set cause of messed up format */
gboolean prop_list_load(GPtrArray *props, DataNode data, DiaContext *ctx);
void prop_list_save(GPtrArray *props, DataNode data, DiaContext *ctx);

#define struct_member(sp, off, tp) (*(tp *)(((char *)sp) + off))

Property *find_prop_by_name(const GPtrArray *props, const gchar *name);
Property *find_prop_by_name_and_type(const GPtrArray *props, const gchar *name,
                                     PropertyType type);

/* ************************ */
/*  Property dialog methods */

/* Add a widget without a title. This will restart a new title+widget table 
   afterwards. */
void prop_dialog_add_raw(PropDialog *dialog, GtkWidget *widget);
void prop_dialog_add_raw_with_flags(PropDialog *dialog, GtkWidget *widget,
				    gboolean expand, gboolean fill);
/* Register a new container widget (which won't be automatically added) */
void prop_dialog_container_push(PropDialog *dialog, GtkWidget *container);
/* De-register the last container of the stack. */
GtkWidget *prop_dialog_container_pop(PropDialog *dialog);
void prop_dialog_add_property(PropDialog *dialog, Property *prop);

PropDialog *prop_dialog_new(GList *objects, gboolean is_default);
PropDialog *prop_dialog_from_widget(GtkWidget *dialog_widget);
void prop_dialog_destroy(PropDialog *dialog);
void prop_get_data_from_widgets(PropDialog *dialog);
WIDGET *prop_dialog_get_widget(const PropDialog *dialog);

/* properyhandler_connect but for notify:: signals. */
void prophandler_connect_notify(const Property *prop,
                                GObject *object,
                                const gchar *signal);
void prophandler_connect(const Property *prop, GObject *object, 
                         const gchar *signal);

/* Stuff in propoffsets.c: */
void prop_offset_list_calculate_quarks(PropOffset *offsets);
void do_set_props_from_offsets(void *base, GPtrArray *props, 
                               const PropOffset *offsets);
void do_get_props_from_offsets(void *base, GPtrArray *props, 
                               const PropOffset *offsets);


#include "prop_basic.h"
#include "prop_inttypes.h"
#include "prop_geomtypes.h"
#include "prop_attr.h"
#include "prop_text.h"
#include "prop_widgets.h"
#include "prop_sdarray.h"
#include "prop_dict.h"
#include "prop_pattern.h"
#include "prop_pixbuf.h"
#include "prop_matrix.h"

#endif /* PROPINTERNALS_H */

