/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * Fork type for UML diagrams
 * Copyright (C) 2002 Alejandro Sierra <asierra@servidor.unam.mx>
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

#include "config.h"

#include <glib/gi18n-lib.h>

#include <math.h>
#include <string.h>

#include "object.h"
#include "element.h"
#include "diarenderer.h"
#include "attributes.h"
#include "text.h"
#include "properties.h"

#include "uml.h"

#include "pixmaps/fork.xpm"

typedef struct _Fork Fork;

/* earlier versions claimed to have 8 connections, but initialized only 6 */
#define NUM_CONNECTIONS 6

struct _Fork
{
  Element element;
  Color fill_color;
  ConnectionPoint connections[NUM_CONNECTIONS];
};

static const double FORK_BORDERWIDTH = 0.0;
static const double FORK_WIDTH = 4.0;
static const double FORK_HEIGHT = 0.4;
static const double FORK_MARGIN = 0.125;

static real fork_distance_from(Fork *branch, Point *point);
static void fork_select(Fork *branch, Point *clicked_point, DiaRenderer *interactive_renderer);
static DiaObjectChange* fork_move_handle(Fork *branch, Handle *handle,
				      Point *to, ConnectionPoint *cp,
				      HandleMoveReason reason, ModifierKeys modifiers);
static DiaObjectChange* fork_move(Fork *branch, Point *to);
static void fork_draw(Fork *branch, DiaRenderer *renderer);
static DiaObject *fork_create(Point *startpoint,
			     void *user_data,
			     Handle **handle1,
			     Handle **handle2);
static void fork_destroy(Fork *branch);
static DiaObject *fork_load(ObjectNode obj_node, int version,DiaContext *ctx);

static PropDescription *fork_describe_props(Fork *branch);
static void fork_get_props(Fork *branch, GPtrArray *props);
static void fork_set_props(Fork *branch, GPtrArray *props);

static void fork_update_data(Fork *branch);

static ObjectTypeOps fork_type_ops =
{
  (CreateFunc) fork_create,
  (LoadFunc)   fork_load,/*using_properties*/     /* load */
  (SaveFunc)   object_save_using_properties,      /* save */
  (GetDefaultsFunc)   NULL,
  (ApplyDefaultsFunc) NULL
};

DiaObjectType fork_type =
{
  "UML - Fork", /* name */
  0,         /* version */
  fork_xpm,   /* pixmap */
  &fork_type_ops /* ops */
};

static ObjectOps fork_ops =
{
  (DestroyFunc)         fork_destroy,
  (DrawFunc)            fork_draw,
  (DistanceFunc)        fork_distance_from,
  (SelectFunc)          fork_select,
  (CopyFunc)            object_copy_using_properties,
  (MoveFunc)            fork_move,
  (MoveHandleFunc)      fork_move_handle,
  (GetPropertiesFunc)   object_create_props_dialog,
  (ApplyPropertiesDialogFunc) object_apply_props_from_dialog,
  (ObjectMenuFunc)      NULL,
  (DescribePropsFunc)   fork_describe_props,
  (GetPropsFunc)        fork_get_props,
  (SetPropsFunc)        fork_set_props,
  (TextEditFunc) 0,
  (ApplyPropertiesListFunc) object_apply_props,
};

static PropDescription fork_props[] = {
  ELEMENT_COMMON_PROPERTIES,
  PROP_STD_FILL_COLOUR_OPTIONAL,
  PROP_DESC_END
};

static PropDescription *
fork_describe_props(Fork *branch)
{
  if (fork_props[0].quark == 0) {
    prop_desc_list_calculate_quarks(fork_props);
  }
  return fork_props;
}

static PropOffset fork_offsets[] = {
  ELEMENT_COMMON_PROPERTIES_OFFSETS,
  { "fill_colour",PROP_TYPE_COLOUR,offsetof(Fork, fill_color) },
  { NULL, 0, 0 },
};

static void
fork_get_props(Fork * branch, GPtrArray *props)
{
  object_get_props_from_offsets(&branch->element.object,
                                fork_offsets, props);
}

static void
fork_set_props(Fork *branch, GPtrArray *props)
{
  object_set_props_from_offsets(&branch->element.object,
                                fork_offsets, props);
  fork_update_data(branch);
}

static real
fork_distance_from(Fork *branch, Point *point)
{
  DiaObject *obj = &branch->element.object;
  return distance_rectangle_point(&obj->bounding_box, point);
}

static void
fork_select(Fork *branch, Point *clicked_point, DiaRenderer *interactive_renderer)
{
  element_update_handles(&branch->element);
}


static DiaObjectChange *
fork_move_handle (Fork             *branch,
                  Handle           *handle,
                  Point            *to,
                  ConnectionPoint  *cp,
                  HandleMoveReason  reason,
                  ModifierKeys      modifiers)
{
  double dx;
  Point c;

  g_return_val_if_fail (branch != NULL, NULL);
  g_return_val_if_fail (handle != NULL, NULL);
  g_return_val_if_fail (to != NULL, NULL);

  g_return_val_if_fail (handle->id < 8, NULL);

  /* Only orizontal E/W movement are allowed */
  if (handle->id==3 || handle->id==4) {
     c.y = to->y;
     c.x = branch->element.corner.x + branch->element.width / 2.;
     dx = fabs(to->x - c.x);
     to->x = c.x - dx;
     element_move_handle(&branch->element, 3, to, cp, reason, modifiers);
     to->x = c.x + dx;
     element_move_handle(&branch->element, 4, to, cp, reason, modifiers);
     fork_update_data(branch);
  }

  return NULL;
}

static DiaObjectChange*
fork_move(Fork *branch, Point *to)
{
  branch->element.corner = *to;
  fork_update_data(branch);

  return NULL;
}


static void
fork_draw (Fork *branch, DiaRenderer *renderer)
{
  Element *elem;
  double w, h;
  Point p1, p2;

  g_return_if_fail (branch != NULL);
  g_return_if_fail (renderer != NULL);

  elem = &branch->element;
  w = elem->width;
  h = elem->height;

  dia_renderer_set_fillstyle (renderer, DIA_FILL_STYLE_SOLID);
  dia_renderer_set_linewidth (renderer, FORK_BORDERWIDTH);
  dia_renderer_set_linestyle (renderer, DIA_LINE_STYLE_SOLID, 0.0);

  p1.x = elem->corner.x;
  p1.y = elem->corner.y;
  p2.x = elem->corner.x + w;
  p2.y = elem->corner.y + h;

  dia_renderer_draw_rect (renderer,
                          &p1,
                          &p2,
                          &branch->fill_color,
                          NULL);
}

static void
fork_update_data(Fork *branch)
{
  Element *elem = &branch->element;
  DiaObject *obj = &elem->object;

  /* Update connections: */
  branch->connections[0].pos.x = elem->corner.x + FORK_MARGIN*elem->width;
  branch->connections[0].pos.y = elem->corner.y;
  branch->connections[1].pos.x = elem->corner.x + elem->width / 2.;
  branch->connections[1].pos.y = elem->corner.y;
  branch->connections[2].pos.x = elem->corner.x + elem->width - FORK_MARGIN*elem->width;
  branch->connections[2].pos.y = elem->corner.y;
  branch->connections[3].pos.x = elem->corner.x + FORK_MARGIN*elem->width;
  branch->connections[3].pos.y = elem->corner.y + elem->height;
  branch->connections[4].pos.x = elem->corner.x + elem->width / 2.;
  branch->connections[4].pos.y = elem->corner.y + elem->height;
  branch->connections[5].pos.x = elem->corner.x + elem->width - FORK_MARGIN*elem->width;
  branch->connections[5].pos.y = elem->corner.y + elem->height;

  element_update_boundingbox(elem);
  obj->position = elem->corner;

  element_update_handles(elem);
}

static DiaObject *
fork_create(Point *startpoint, void *user_data, Handle **handle1, Handle **handle2)
{
  Fork *branch;
  Element *elem;
  DiaObject *obj;
  int i;

  branch = g_new0 (Fork, 1);
  elem = &branch->element;
  obj = &elem->object;

  obj->type = &fork_type;

  obj->ops = &fork_ops;

  elem->corner = *startpoint;
  elem->width = FORK_WIDTH;
  elem->height = FORK_HEIGHT;
  element_init(elem, 8, NUM_CONNECTIONS);

  branch->fill_color = attributes_get_foreground();

  for (i = 0; i < NUM_CONNECTIONS; ++i) {
    obj->connections[i] = &branch->connections[i];
    branch->connections[i].object = obj;
    branch->connections[i].connected = NULL;
    if (i < NUM_CONNECTIONS/2)
      branch->connections[i].directions = DIR_NORTH;
    else
      branch->connections[i].directions = DIR_SOUTH;
  }
  elem->extra_spacing.border_trans = FORK_BORDERWIDTH / 2.0;
  fork_update_data(branch);

  for (i=0;i<8;i++) {
     if (i!=3 && i!=4)
       obj->handles[i]->type = HANDLE_NON_MOVABLE;
  }

  *handle1 = NULL;
  *handle2 = obj->handles[0];
  return &branch->element.object;
}

static void
fork_destroy(Fork *branch)
{
  element_destroy(&branch->element);
}

static DiaObject *
fork_load(ObjectNode obj_node, int version,DiaContext *ctx)
{
  return object_load_using_properties(&fork_type,
                                      obj_node,version,ctx);
}





