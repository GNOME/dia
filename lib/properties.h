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

#include <glib.h>

#include "geometry.h"
#include "arrow.h"
#include "color.h"
#include "font.h"

typedef enum {
  PROP_TYPE_INVALID = 0,
  PROP_TYPE_CHAR,
  PROP_TYPE_BOOL,
  PROP_TYPE_INT,
  PROP_TYPE_ENUM,
  PROP_TYPE_REAL,
  PROP_TYPE_STRING
  PROP_TYPE_POINT,
  PROP_TYPE_POINTARRAY,
  PROP_TYPE_RECTANGLE,
  PROP_TYPE_ARROW,
  PROP_TYPE_COLOUR,
  PROP_TYPE_FONT,

  PROP_LAST
} PropType;

#define PROP_IS_OTHER(ptype) ((ptype) >= PROP_LAST)

typedef struct _PropDescription PropDescription;
struct _PropDescription {
  const gchar *name;
  PropertyType type;
  const gchar *description;

  GQuark quark; /* quark for property name -- helps speed up lookups. */
};

typedef struct _Property Property;
struct _Property {
  const gchar *name;
  PropertyType type;
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
    Arrow arrow_data;
    Color colour_data;
    Font *font_data;
    
    gpointer other_data;
  } d;
};

#define PROP_VALUE_CHAR(prop)       ((prop).d.char_data)
#define PROP_VALUE_BOOL(prop)       ((prop).d.bool_dat)
#define PROP_VALUE_INT(prop)        ((prop).d.int_data)
#define PROP_VALUE_ENUM(prop)       ((prop).d.int_data)
#define PROP_VALUE_REAL(prop)       ((prop).d.real_data)
#define PROP_VALUE_STRING(prop)     ((prop).d.string_data)
#define PROP_VALUE_POINT(prop)      ((prop).d.point_data)
#define PROP_VALUE_POINTARRAY(prop) ((prop).d.ptarray_data)
#define PROP_VALUE_RECT(prop)       ((prop).d.rect_data)
#define PROP_VALUE_ARROW(prop)      ((prop).d.arrow_data)
#define PROP_VALUE_COLOUR(prop)     ((prop).d.colour_data)
#define PROP_VALUE_FONT(prop)       ((prop).d.font_data)
#define PROP_VALUE_OTHER(prop)      ((prop).d.other_data)

G_INLINE_FUNC void prop_list_calculate_quarks(PropDescription *plist);
#ifdef G_CAN_INLINE
G_INLINE_FUNC void
prop_list_calculate_quarks(PropDescription *plist)
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
G_INLINE_FUNC PropDescription *prop_list_find_prop(PropDescription *plist,
						   const gchar *name);
#define G_CAN_INLINE
G_INLINE_FUNC PropDescription
prop_list_find_prop(PropDescription *plist, const gchar *name)
{
  gint i = 0;
  GQuark *name_quark = g_quark_from_string(name);

  while (plist[i].name != NULL) {
    if (plist[i].quark == name_quark)
      return &plist[i];
    i++;
  }
  return NULL;
}
#endif

PropDescription *prop_lists_union(GList *plists);
PropDescription *prop_lists_intersection(GList *plists);

#endif
