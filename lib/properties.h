/* Dia -- a diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * properties.h: property system for dia objects/shapes.
 * Copyright (C) 2000 James Henstridge
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

#ifndef PROPERTIES_H
#define PROPERTIES_H

#include <gtk/gtk.h>

#include "geometry.h"
#include "render.h"
#include "arrows.h"
#include "color.h"
#include "font.h"
#include "object.h"

#ifndef _prop_typedefs_defined
#define _prop_typedefs_defined
typedef struct _PropDescription PropDescription;
typedef struct _Property Property;
#endif

typedef enum {
  PROP_TYPE_INVALID = 0,
  PROP_TYPE_CHAR,
  PROP_TYPE_BOOL,
  PROP_TYPE_INT,
  PROP_TYPE_ENUM,
  PROP_TYPE_REAL,
  PROP_TYPE_STRING,
  PROP_TYPE_POINT,
  PROP_TYPE_POINTARRAY,
  PROP_TYPE_RECT,
  PROP_TYPE_LINESTYLE,
  PROP_TYPE_ARROW,
  PROP_TYPE_COLOUR,
  PROP_TYPE_FONT,

  PROP_LAST
} PropType;

#define PROP_IS_OTHER(ptype) ((ptype) >= PROP_LAST)

struct _PropDescription {
  const gchar *name;
  PropType type;
  guint flags;
  const gchar *description;
  const gchar *tooltip;

  /* Holds some extra data whose meaning is dependent on the property type.
   * For example, int or float may use bounds for a spin button, and enums
   * may use a list of string names for enumeration values. */
  gpointer extra_data;

  GQuark quark; /* quark for property name -- helps speed up lookups. */
};

#define PROP_FLAG_VISIBLE   0x0001
#define PROP_FLAG_DONT_SAVE 0x0002

struct _Property {
  const gchar *name;
  PropType type;
  gpointer extra_data;
  union {
    gchar char_data;
    gboolean bool_data;
    gint int_data;
    real real_data;
    gchar *string_data; /* malloc'd string owned by Property structure */
    Point point_data;
    struct {
      Point *pts;
      guint npts;
    } ptarray_data;
    Rectangle rect_data;
    struct {
      LineStyle style;
      real dash;
    } linestyle_data;
    Arrow arrow_data;
    Color colour_data;
    Font *font_data;
    
    gpointer other_data;
  } d;
};

#define PROP_VALUE_CHAR(prop)       ((prop).d.char_data)
#define PROP_VALUE_BOOL(prop)       ((prop).d.bool_data)
#define PROP_VALUE_INT(prop)        ((prop).d.int_data)
#define PROP_VALUE_ENUM(prop)       ((prop).d.int_data)
#define PROP_VALUE_REAL(prop)       ((prop).d.real_data)
#define PROP_VALUE_STRING(prop)     ((prop).d.string_data)
#define PROP_VALUE_POINT(prop)      ((prop).d.point_data)
#define PROP_VALUE_POINTARRAY(prop) ((prop).d.ptarray_data)
#define PROP_VALUE_RECT(prop)       ((prop).d.rect_data)
#define PROP_VALUE_LINESTYLE(prop)  ((prop).d.linestyle_data)
#define PROP_VALUE_ARROW(prop)      ((prop).d.arrow_data)
#define PROP_VALUE_COLOUR(prop)     ((prop).d.colour_data)
#define PROP_VALUE_FONT(prop)       ((prop).d.font_data)
#define PROP_VALUE_OTHER(prop)      ((prop).d.other_data)

/* Copy the data member of the property
 * If NULL, then just copy data member straight */
typedef void (* PropType_Copy)(Property *dest, Property *src);
/* free the data member of the property -- must handle NULL values
 * If NULL, don't do anything special to free value */
typedef void (* PropType_Free)(Property *prop);
/* Create a widget capable of editing the property */
typedef GtkWidget *(* PropType_GetWidget)(Property *prop);
/* Set the value of the property from the current value of the widget */
typedef void (* PropType_SetProp)(Property *prop, GtkWidget *widget);

PropType prop_type_register(const gchar *name, PropType_Copy cfunc,
			    PropType_Free ffunc, PropType_GetWidget wfunc,
			    PropType_SetProp sfunc);

G_INLINE_FUNC void prop_desc_list_calculate_quarks(PropDescription *plist);
#ifdef G_CAN_INLINE
G_INLINE_FUNC void
prop_desc_list_calculate_quarks(PropDescription *plist)
{
  gint i = 0;
  while (plist[i].name != NULL) {
    if (plist[i].quark == 0)
      plist[i].quark = g_quark_from_static_string(plist[i].name);
    i++;
  }
}
#endif

/* plist must have all quarks calculated in advance */
G_INLINE_FUNC PropDescription *prop_desc_list_find_prop(PropDescription *plist,
							const gchar *name);
#ifdef G_CAN_INLINE
G_INLINE_FUNC PropDescription *
prop_desc_list_find_prop(PropDescription *plist, const gchar *name)
{
  gint i = 0;
  GQuark name_quark = g_quark_from_string(name);

  while (plist[i].name != NULL) {
    if (plist[i].quark == name_quark)
      return &plist[i];
    i++;
  }
  return NULL;
}
#endif

/* operations on lists of property description lists */
PropDescription *prop_desc_lists_union(GList *plists);
PropDescription *prop_desc_lists_intersection(GList *plists);

/* functions for manipulating an individual property structure */
void       prop_copy(Property *dest, Property *src);
void       prop_free(Property *prop);
GtkWidget *prop_get_widget(Property *prop);
void       prop_set_from_widget(Property *prop, GtkWidget *widget);

G_INLINE_FUNC Property *prop_list_from_matching_descs(PropDescription *plist,
						      guint flags,
						      guint *nprops);
#ifdef G_CAN_INLINE
G_INLINE_FUNC Property *
prop_list_from_matching_descs(PropDescription *plist, guint flags,
			      guint *nprops)
{
  Property *ret;
  guint count = 0, i;

  for (i = 0; plist[i].name != NULL; i++)
    if ((plist[i].flags & flags) == flags)
      count++;

  ret = g_new0(Property, count);
  if (nprops) *nprops = count;
  count = 0;
  for (i = 0; plist[i].name != NULL; i++)
    if ((plist[i].flags & flags) == flags) {
      ret[count].name = plist[i].name;
      ret[count].type = plist[i].type;
      ret[count].extra_data = plist[i].extra_data;
      count++;
    }
  return ret;
}
#endif

/* calculates the offset of a structure member within the structure */
#ifndef offsetof
#define offsetof(type, member) ( (int) & ((type*)0) -> member )
#endif
typedef struct _PropOffset PropOffset;
struct _PropOffset {
  const gchar *name;
  PropType type;
  int offset;
  int offset2; /* maybe for point lists, etc */
  GQuark name_quark;
};

/* not guaranteed to handle all property types */
gboolean object_get_props_from_offsets(Object *obj, PropOffset *offsets,
				       Property *props, guint nprops);
gboolean object_set_props_from_offsets(Object *obj, PropOffset *offsets,
				       Property *props, guint nprops);

/* apply some properties and return a corresponding object change */
ObjectChange *object_apply_props(Object *obj, Property *props, guint nprops);

/* standard properties dialogs that can be used for objects that
 * implement describe_props, get_props and set_props */
GtkWidget    *object_create_props_dialog     (Object *obj);
ObjectChange *object_apply_props_from_dialog (Object *obj, GtkWidget *table);

#endif
