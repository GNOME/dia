/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
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

/* DO NOT USE THIS OBJECT AS A BASIS FOR A NEW OBJECT. */

#include "config.h"

#include <glib/gi18n-lib.h>

#include <math.h>
#include <string.h>

#include "object.h"
#include "element.h"
#include "connectionpoint.h"
#include "diarenderer.h"
#include "attributes.h"
#include "properties.h"

#include "pixmaps/relationship.xpm"

#define DEFAULT_WIDTH 2.0
#define DEFAULT_HEIGHT 1.0
#define IDENTIFYING_BORDER_WIDTH 0.4
#define TEXT_BORDER_WIDTH_X 1.0
#define DIAMOND_RATIO 0.6
#define FONT_HEIGHT 0.8
#define CARDINALITY_DISTANCE 0.3

#define NUM_CONNECTIONS 9

typedef struct _Relationship Relationship;

struct _Relationship {
  Element element;

  DiaFont *font;
  double font_height;
  char *name;
  char *left_cardinality;
  char *right_cardinality;
  double name_width;
  double left_card_width;
  double right_card_width;

  gboolean identifying;
  gboolean rotate;

  ConnectionPoint connections[NUM_CONNECTIONS];

  double border_width;
  Color border_color;
  Color inner_color;
};


static double relationship_distance_from(Relationship *relationship, Point *point);
static void relationship_select(Relationship *relationship, Point *clicked_point,
		       DiaRenderer *interactive_renderer);
static DiaObjectChange* relationship_move_handle(Relationship *relationship, Handle *handle,
					      Point *to, ConnectionPoint *cp,
					      HandleMoveReason reason, ModifierKeys modifiers);
static DiaObjectChange* relationship_move(Relationship *relationship, Point *to);
static void relationship_draw(Relationship *relationship, DiaRenderer *renderer);
static void relationship_update_data(Relationship *relationship);
static DiaObject *relationship_create(Point *startpoint,
			  void *user_data,
			  Handle **handle1,
			  Handle **handle2);
static void relationship_destroy(Relationship *relationship);
static DiaObject *relationship_copy(Relationship *relationship);

static void relationship_save(Relationship *relationship,
			      ObjectNode obj_node, DiaContext *ctx);
static DiaObject *relationship_load(ObjectNode obj_node, int version,DiaContext *ctx);
static PropDescription *
relationship_describe_props(Relationship *relationship);
static void
relationship_get_props(Relationship *relationship, GPtrArray *props);
static void
relationship_set_props(Relationship *relationship, GPtrArray *props);

static ObjectTypeOps relationship_type_ops =
{
  (CreateFunc) relationship_create,
  (LoadFunc)   relationship_load,
  (SaveFunc)   relationship_save
};

DiaObjectType relationship_type =
{
  "ER - Relationship",  /* name */
  0,                 /* version */
  relationship_xpm,   /* pixmap */
  &relationship_type_ops /* ops */
};

DiaObjectType *_relationship_type = (DiaObjectType *) &relationship_type;

static ObjectOps relationship_ops = {
  (DestroyFunc)         relationship_destroy,
  (DrawFunc)            relationship_draw,
  (DistanceFunc)        relationship_distance_from,
  (SelectFunc)          relationship_select,
  (CopyFunc)            relationship_copy,
  (MoveFunc)            relationship_move,
  (MoveHandleFunc)      relationship_move_handle,
  (GetPropertiesFunc)   object_create_props_dialog,
  (ApplyPropertiesDialogFunc) object_apply_props_from_dialog,
  (ObjectMenuFunc)      NULL,
  (DescribePropsFunc)   relationship_describe_props,
  (GetPropsFunc)        relationship_get_props,
  (SetPropsFunc)        relationship_set_props,
  (TextEditFunc) 0,
  (ApplyPropertiesListFunc) object_apply_props,
};

static PropDescription relationship_props[] = {
  ELEMENT_COMMON_PROPERTIES,
  { "name", PROP_TYPE_STRING, PROP_FLAG_VISIBLE,
    N_("Name:"), NULL, NULL },
  { "left_cardinality", PROP_TYPE_STRING, PROP_FLAG_VISIBLE,
    N_("Left Cardinality:"), NULL, NULL },
  { "right_cardinality", PROP_TYPE_STRING, PROP_FLAG_VISIBLE,
    N_("Right Cardinality:"), NULL, NULL },
  { "rotate", PROP_TYPE_BOOL, PROP_FLAG_VISIBLE,
    N_("Rotate:"), NULL, NULL },
  { "identifying", PROP_TYPE_BOOL, PROP_FLAG_VISIBLE,
    N_("Identifying:"), NULL, NULL },
  PROP_STD_LINE_WIDTH,
  PROP_STD_LINE_COLOUR,
  PROP_STD_FILL_COLOUR,
  PROP_STD_TEXT_FONT,
  PROP_STD_TEXT_HEIGHT,
  PROP_DESC_END
};

static PropDescription *
relationship_describe_props (Relationship *relationship)
{
  if (relationship_props[0].quark == 0) {
    prop_desc_list_calculate_quarks (relationship_props);
  }

  return relationship_props;
}


static PropOffset relationship_offsets[] = {
  ELEMENT_COMMON_PROPERTIES_OFFSETS,
  { "name", PROP_TYPE_STRING, offsetof (Relationship, name) },
  { "left_cardinality", PROP_TYPE_STRING, offsetof (Relationship, left_cardinality) },
  { "right_cardinality", PROP_TYPE_STRING, offsetof (Relationship, right_cardinality) },
  { "rotate", PROP_TYPE_BOOL, offsetof (Relationship, rotate) },
  { "identifying", PROP_TYPE_BOOL, offsetof (Relationship, identifying) },
  { PROP_STDNAME_LINE_WIDTH, PROP_STDTYPE_LINE_WIDTH, offsetof (Relationship, border_width) },
  { "line_colour", PROP_TYPE_COLOUR, offsetof (Relationship, border_color) },
  { "fill_colour", PROP_TYPE_COLOUR, offsetof (Relationship, inner_color) },
  { "text_font", PROP_TYPE_FONT, offsetof (Relationship, font) },
  { PROP_STDNAME_TEXT_HEIGHT, PROP_STDTYPE_TEXT_HEIGHT, offsetof (Relationship, font_height) },
  { NULL, 0, 0}
};


static void
relationship_get_props (Relationship *relationship, GPtrArray *props)
{
  object_get_props_from_offsets (&relationship->element.object,
                                 relationship_offsets, props);
}


static void
relationship_set_props (Relationship *relationship, GPtrArray *props)
{
  object_set_props_from_offsets (&relationship->element.object,
                                 relationship_offsets, props);
  relationship_update_data (relationship);
}


static double
relationship_distance_from (Relationship *relationship, Point *point)
{
  Element *elem = &relationship->element;
  DiaRectangle rect;

  rect.left = elem->corner.x - relationship->border_width / 2;
  rect.right = elem->corner.x + elem->width + relationship->border_width / 2;
  rect.top = elem->corner.y - relationship->border_width / 2;
  rect.bottom = elem->corner.y + elem->height + relationship->border_width / 2;

  return distance_rectangle_point (&rect, point);
}


static void
relationship_select (Relationship *relationship,
                     Point        *clicked_point,
                     DiaRenderer  *interactive_renderer)
{
  element_update_handles (&relationship->element);
}


static DiaObjectChange *
relationship_move_handle (Relationship     *relationship,
                          Handle           *handle,
                          Point            *to,
                          ConnectionPoint  *cp,
                          HandleMoveReason  reason,
                          ModifierKeys      modifiers) {
  g_return_val_if_fail (relationship != NULL, NULL);
  g_return_val_if_fail (handle != NULL, NULL);
  g_return_val_if_fail (to != NULL, NULL);

  element_move_handle (&relationship->element, handle->id, to, cp, reason, modifiers);

  relationship_update_data (relationship);

  return NULL;
}


static DiaObjectChange *
relationship_move (Relationship *relationship, Point *to)
{
  relationship->element.corner = *to;

  relationship_update_data (relationship);

  return NULL;
}


static void
relationship_draw (Relationship *relationship, DiaRenderer *renderer)
{
  Point corners[4];
  Point lc, rc;
  Point p;
  Element *elem;
  double diff;
  DiaAlignment left_align;

  g_return_if_fail (relationship != NULL);
  g_return_if_fail (renderer != NULL);

  elem = &relationship->element;

  corners[0].x = elem->corner.x;
  corners[0].y = elem->corner.y + elem->height / 2;
  corners[1].x = elem->corner.x + elem->width / 2;
  corners[1].y = elem->corner.y;
  corners[2].x = elem->corner.x + elem->width;
  corners[2].y = elem->corner.y + elem->height / 2;
  corners[3].x = elem->corner.x + elem->width / 2;
  corners[3].y = elem->corner.y + elem->height;

  dia_renderer_set_fillstyle (renderer, DIA_FILL_STYLE_SOLID);

  dia_renderer_set_linewidth (renderer, relationship->border_width);
  dia_renderer_set_linestyle (renderer, DIA_LINE_STYLE_SOLID, 0.0);
  dia_renderer_set_linejoin (renderer, DIA_LINE_JOIN_MITER);

  dia_renderer_draw_polygon (renderer,
                             corners,
                             4,
                             &relationship->inner_color,
                             &relationship->border_color);

  if (relationship->rotate) {
    lc.x = corners[1].x + 0.2;
    lc.y = corners[1].y - 0.3;
    rc.x = corners[3].x + 0.2;
    rc.y = corners[3].y + 0.3 + relationship->font_height;
    left_align = DIA_ALIGN_LEFT;
  } else {
    lc.x = corners[0].x - CARDINALITY_DISTANCE;
    lc.y = corners[0].y - 0.3;
    rc.x = corners[2].x + CARDINALITY_DISTANCE;
    rc.y = corners[2].y - 0.3;
    left_align = DIA_ALIGN_RIGHT;
  }

  if (relationship->identifying) {
    diff = IDENTIFYING_BORDER_WIDTH;
    corners[0].x += diff;
    corners[1].y += diff * DIAMOND_RATIO;
    corners[2].x -= diff;
    corners[3].y -= diff * DIAMOND_RATIO;

    dia_renderer_draw_polygon (renderer,
                               corners,
                               4,
                               NULL,
                               &relationship->border_color);
  }

  dia_renderer_set_font (renderer,
                         relationship->font,
                         relationship->font_height);
  dia_renderer_draw_string (renderer,
                            relationship->left_cardinality,
                            &lc,
                            left_align,
                            &color_black);
  dia_renderer_draw_string (renderer,
                            relationship->right_cardinality,
                            &rc,
                            DIA_ALIGN_LEFT,
                            &color_black);

  p.x = elem->corner.x + elem->width / 2.0;
  p.y = elem->corner.y + (elem->height - relationship->font_height) / 2.0 +
        dia_font_ascent (relationship->name,
                         relationship->font,
                         relationship->font_height);

  dia_renderer_draw_string (renderer,
                            relationship->name,
                            &p,
                            DIA_ALIGN_CENTRE,
                            &color_black);
}


static void
relationship_update_data(Relationship *relationship)
{
  Element *elem = &relationship->element;
  DiaObject *obj = &elem->object;
  ElementBBExtras *extra = &elem->extra_spacing;
  DiaRectangle *bbox = &obj->bounding_box;

  relationship->name_width = dia_font_string_width (relationship->name,
                                                    relationship->font,
                                                    relationship->font_height);

  elem->width = relationship->name_width + 2 * TEXT_BORDER_WIDTH_X;
  elem->height = elem->width * DIAMOND_RATIO;

  /* Update connections: */
  /*
       	2
       	*
    1  / \  3
      *	  *
     /	   \
  0 *	* 8 * 4
     \	   /
      *	  *
    7  \ /  5
        *
        6
   */

  connpoint_update (&relationship->connections[0],
                    elem->corner.x,
                    elem->corner.y + elem->height / 2.0,
                    DIR_WEST);

  connpoint_update (&relationship->connections[1],
                    elem->corner.x + elem->width / 4.0,
                    elem->corner.y + elem->height / 4.0,
                    DIR_NORTHWEST);

  connpoint_update (&relationship->connections[2],
                    elem->corner.x + elem->width / 2.0,
                    elem->corner.y,
                    DIR_NORTH);

  connpoint_update (&relationship->connections[3],
                    elem->corner.x + 3.0 * elem->width / 4.0,
                    elem->corner.y + elem->height / 4.0,
                    DIR_NORTHEAST);

  connpoint_update (&relationship->connections[4],
                    elem->corner.x + elem->width,
                    elem->corner.y + elem->height / 2.0,
                    DIR_EAST);

  connpoint_update (&relationship->connections[5],
                    elem->corner.x + 3.0 * elem->width / 4.0,
                    elem->corner.y + 3.0 * elem->height / 4.0,
                    DIR_SOUTHEAST);

  connpoint_update (&relationship->connections[6],
                    elem->corner.x + elem->width / 2.0,
                    elem->corner.y + elem->height,
                    DIR_SOUTH);

  connpoint_update (&relationship->connections[7],
                    elem->corner.x + elem->width / 4.0,
                    elem->corner.y + 3.0 * elem->height / 4.0,
                    DIR_SOUTHWEST);

  connpoint_update (&relationship->connections[8],
                    elem->corner.x + elem->width / 2.0,
                    elem->corner.y + elem->height / 2.0,
                    DIR_ALL);

  extra->border_trans = relationship->border_width / 2.0;
  element_update_boundingbox (elem);

  /* font_string_width is not very precise, adding a 5% extra margin avoids
   * clipping glitches
   */
  relationship->left_card_width =
      1.05 * dia_font_string_width (relationship->left_cardinality,
                                    relationship->font,
                                    relationship->font_height);
  relationship->right_card_width =
      1.05 * dia_font_string_width (relationship->right_cardinality,
                                    relationship->font,
                                    relationship->font_height);

  /* fix boundingrelationship for line_width: */
  if (relationship->rotate) {
    bbox->top -= relationship->font_height + 0.2 + CARDINALITY_DISTANCE;
    bbox->bottom += relationship->font_height + 0.2 + CARDINALITY_DISTANCE;
    bbox->right += (bbox->left + bbox->right) / 2.0 + CARDINALITY_DISTANCE
                   + MAX (relationship->right_card_width,
                          relationship->left_card_width);
  } else {
    bbox->left -= CARDINALITY_DISTANCE + relationship->left_card_width;
    bbox->right += CARDINALITY_DISTANCE + relationship->right_card_width;
  }
  obj->position = elem->corner;

  element_update_handles (elem);
}


static DiaObject *
relationship_create (Point   *startpoint,
                     void    *user_data,
                     Handle **handle1,
                     Handle **handle2)
{
  Relationship *relationship;
  Element *elem;
  DiaObject *obj;
  int i;

  relationship = g_new0 (Relationship, 1);
  elem = &relationship->element;
  obj = &elem->object;

  obj->type = &relationship_type;
  obj->ops = &relationship_ops;

  elem->corner = *startpoint;
  elem->width = DEFAULT_WIDTH;
  elem->height = DEFAULT_HEIGHT;

  relationship->border_width = attributes_get_default_linewidth ();
  relationship->border_color = attributes_get_foreground ();
  relationship->inner_color = attributes_get_background ();

  element_init (elem, 8, NUM_CONNECTIONS);

  for (i = 0; i < NUM_CONNECTIONS; i++) {
    obj->connections[i] = &relationship->connections[i];
    relationship->connections[i].object = obj;
    relationship->connections[i].connected = NULL;
  }
  relationship->connections[8].flags = CP_FLAGS_MAIN;

  relationship->font = dia_font_new_from_style (DIA_FONT_MONOSPACE,
                                                FONT_HEIGHT);
  relationship->font_height = FONT_HEIGHT;
  relationship->name = g_strdup (_("Relationship"));
  relationship->left_cardinality = g_strdup ("");
  relationship->right_cardinality = g_strdup ("");
  relationship->identifying = FALSE;
  relationship->rotate = FALSE;

  relationship_update_data (relationship);

  for (i = 0; i < 8; i++) {
    obj->handles[i]->type = HANDLE_NON_MOVABLE;
  }

  *handle1 = NULL;
  *handle2 = obj->handles[0];

  return &relationship->element.object;
}


static void
relationship_destroy (Relationship *relationship)
{
  g_clear_object (&relationship->font);
  element_destroy (&relationship->element);
  g_clear_pointer (&relationship->name, g_free);
  g_clear_pointer (&relationship->left_cardinality, g_free);
  g_clear_pointer (&relationship->right_cardinality, g_free);
}


static DiaObject *
relationship_copy (Relationship *relationship)
{
  int i;
  Relationship *newrelationship;
  Element *elem, *newelem;
  DiaObject *newobj;

  elem = &relationship->element;

  newrelationship = g_new0 (Relationship, 1);
  newelem = &newrelationship->element;
  newobj = &newelem->object;

  element_copy (elem, newelem);

  newrelationship->border_width = relationship->border_width;
  newrelationship->border_color = relationship->border_color;
  newrelationship->inner_color = relationship->inner_color;

  for (i = 0; i < NUM_CONNECTIONS; i++) {
    newobj->connections[i] = &newrelationship->connections[i];
    newrelationship->connections[i].object = newobj;
    newrelationship->connections[i].connected = NULL;
    newrelationship->connections[i].pos = relationship->connections[i].pos;
  }

  newrelationship->font = g_object_ref (relationship->font);
  newrelationship->font_height = relationship->font_height;
  newrelationship->name = g_strdup (relationship->name);
  newrelationship->left_cardinality =
      g_strdup (relationship->left_cardinality);
  newrelationship->right_cardinality =
      g_strdup (relationship->right_cardinality);
  newrelationship->name_width = relationship->name_width;
  newrelationship->left_card_width = relationship->left_card_width;
  newrelationship->right_card_width = relationship->right_card_width;

  newrelationship->identifying = relationship->identifying;
  newrelationship->rotate = relationship->rotate;

  return &newrelationship->element.object;
}


static void
relationship_save (Relationship *relationship,
                   ObjectNode    obj_node,
                   DiaContext   *ctx)
{
  element_save (&relationship->element, obj_node, ctx);

  data_add_real (new_attribute (obj_node, "border_width"),
                 relationship->border_width, ctx);
  data_add_color (new_attribute (obj_node, "border_color"),
                  &relationship->border_color, ctx);
  data_add_color (new_attribute (obj_node, "inner_color"),
                  &relationship->inner_color, ctx);
  data_add_string (new_attribute (obj_node, "name"),
                   relationship->name, ctx);
  data_add_string (new_attribute (obj_node, "left_card"),
                   relationship->left_cardinality, ctx);
  data_add_string (new_attribute (obj_node, "right_card"),
                   relationship->right_cardinality, ctx);
  data_add_boolean (new_attribute (obj_node, "identifying"),
                    relationship->identifying, ctx);
  data_add_boolean (new_attribute (obj_node, "rotated"),
                    relationship->rotate, ctx);
  data_add_font (new_attribute (obj_node, "font"),
                 relationship->font, ctx);
  data_add_real (new_attribute (obj_node, "font_height"),
                 relationship->font_height, ctx);
}


static DiaObject *
relationship_load (ObjectNode obj_node, int version, DiaContext *ctx)
{
  Relationship *relationship;
  Element *elem;
  DiaObject *obj;
  int i;
  AttributeNode attr;

  relationship = g_new0 (Relationship, 1);
  elem = &relationship->element;
  obj = &elem->object;

  obj->type = &relationship_type;
  obj->ops = &relationship_ops;

  element_load (elem, obj_node, ctx);

  relationship->border_width = 0.1;
  attr = object_find_attribute (obj_node, "border_width");
  if (attr != NULL) {
    relationship->border_width = data_real (attribute_first_data (attr),
                                            ctx);
  }

  relationship->border_color = color_black;
  attr = object_find_attribute (obj_node, "border_color");
  if (attr != NULL) {
    data_color (attribute_first_data (attr),
                &relationship->border_color, ctx);
  }

  relationship->inner_color = color_white;
  attr = object_find_attribute (obj_node, "inner_color");
  if (attr != NULL) {
    data_color (attribute_first_data (attr),
                &relationship->inner_color, ctx);
  }

  relationship->name = NULL;
  attr = object_find_attribute (obj_node, "name");
  if (attr != NULL) {
    relationship->name = data_string (attribute_first_data (attr), ctx);
  }

  relationship->left_cardinality = NULL;
  attr = object_find_attribute (obj_node, "left_card");
  if (attr != NULL) {
    relationship->left_cardinality =
        data_string (attribute_first_data (attr), ctx);
  }

  relationship->right_cardinality = NULL;
  attr = object_find_attribute (obj_node, "right_card");
  if (attr != NULL) {
    relationship->right_cardinality =
        data_string (attribute_first_data (attr), ctx);
  }

  attr = object_find_attribute (obj_node, "identifying");
  if (attr != NULL) {
    relationship->identifying =
        data_boolean (attribute_first_data (attr), ctx);
  }

  attr = object_find_attribute (obj_node, "rotated");
  if (attr != NULL) {
    relationship->rotate = data_boolean (attribute_first_data (attr), ctx);
  }

  relationship->font = NULL;
  attr = object_find_attribute (obj_node, "font");
  if (attr != NULL) {
    relationship->font = data_font (attribute_first_data (attr), ctx);
  }

  relationship->font_height = FONT_HEIGHT;
  attr = object_find_attribute (obj_node, "font_height");
  if (attr != NULL) {
    relationship->font_height = data_real (attribute_first_data (attr), ctx);
  }

  element_init (elem, 8, NUM_CONNECTIONS);

  for (i = 0; i < NUM_CONNECTIONS; i++) {
    obj->connections[i] = &relationship->connections[i];
    relationship->connections[i].object = obj;
    relationship->connections[i].connected = NULL;
  }
  relationship->connections[8].flags = CP_FLAGS_MAIN;

  if (relationship->font == NULL) {
    relationship->font = dia_font_new_from_style (DIA_FONT_MONOSPACE,
                                                  FONT_HEIGHT);
  }

  relationship_update_data (relationship);

  for (i = 0; i < 8; i++) {
    obj->handles[i]->type = HANDLE_NON_MOVABLE;
  }

  return &relationship->element.object;
}
