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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <math.h>

#include "intl.h"
#include "object.h"
#include "element.h"
#include "connectionpoint.h"
#include "diarenderer.h"
#include "attributes.h"
#include "widgets.h"
#include "message.h"
#include "properties.h"
#include "diagramdata.h"
#include "parent.h"
#include "newgroup.h"

#include "../objects/Misc/pixmaps/newgroup.xpm"

#define NUM_CONNECTIONS 9

#define DEFAULT_WIDTH 2
#define DEFAULT_HEIGHT 2

typedef struct _Group NewGroup;

struct _Group {
  Element element;

  gboolean is_open;
  ConnectionPoint connections[NUM_CONNECTIONS];
};

static real newgroup_distance_from(NewGroup *group, Point *point);
static void newgroup_select(NewGroup *group, Point *clicked_point,
		       DiaRenderer *interactive_renderer);
static ObjectChange* newgroup_move_handle(NewGroup *group, Handle *handle,
			    Point *to, ConnectionPoint *cp,
				     HandleMoveReason reason, 
			    ModifierKeys modifiers);
static ObjectChange* newgroup_move(NewGroup *group, Point *to);
static void newgroup_draw(NewGroup *group, DiaRenderer *renderer);
static void newgroup_update_data(NewGroup *group);
static DiaObject *newgroup_create(Point *startpoint,
			  void *user_data,
			  Handle **handle1,
			  Handle **handle2);
static void newgroup_destroy(NewGroup *group);
static DiaObject *newgroup_copy(NewGroup *group);

static PropDescription *newgroup_describe_props(NewGroup *group);
static void newgroup_get_props(NewGroup *group, GPtrArray *props);
static void newgroup_set_props(NewGroup *group, GPtrArray *props);

static void newgroup_save(NewGroup *group, ObjectNode obj_node, const char *filename);
static DiaObject *newgroup_load(ObjectNode obj_node, int version, const char *filename);

static ObjectTypeOps newgroup_type_ops =
{
  (CreateFunc) newgroup_create,
  (LoadFunc)   newgroup_load,
  (SaveFunc)   newgroup_save,
  (GetDefaultsFunc)   NULL,
  (ApplyDefaultsFunc) NULL
};

DiaObjectType newgroup_type =
{
  "Misc - NewGroup",  /* name */
  0,                 /* version */
  (char **) newgroup_icon, /* pixmap */

  &newgroup_type_ops      /* ops */
};

DiaObjectType *_newgroup_type = (DiaObjectType *) &newgroup_type;

static ObjectOps newgroup_ops = {
  (DestroyFunc)         newgroup_destroy,
  (DrawFunc)            newgroup_draw,
  (DistanceFunc)        newgroup_distance_from,
  (SelectFunc)          newgroup_select,
  (CopyFunc)            newgroup_copy,
  (MoveFunc)            newgroup_move,
  (MoveHandleFunc)      newgroup_move_handle,
  (GetPropertiesFunc)   object_create_props_dialog,
  (ApplyPropertiesDialogFunc) object_apply_props_from_dialog,
  (ObjectMenuFunc)      NULL,
  (DescribePropsFunc)   newgroup_describe_props,
  (GetPropsFunc)        newgroup_get_props,
  (SetPropsFunc)        newgroup_set_props,
  (TextEditFunc) 0,
#ifdef __GNUC__
  #warning NewGroup requires a function in the vtable to apply props
#else
  #pragma message("warning: NewGroup requires a function in the vtable to apply props")
#endif
};

static PropDescription newgroup_props[] = {
  ELEMENT_COMMON_PROPERTIES,
  { "open", PROP_TYPE_BOOL, PROP_FLAG_VISIBLE,
    N_("Open group"), NULL, NULL},

  PROP_DESC_END
};

static PropDescription *
newgroup_describe_props(NewGroup *group)
{
  if (newgroup_props[0].quark == 0)
    prop_desc_list_calculate_quarks(newgroup_props);
  return newgroup_props;
}

static PropOffset newgroup_offsets[] = {
  ELEMENT_COMMON_PROPERTIES_OFFSETS,
  { "open", PROP_TYPE_BOOL, offsetof(NewGroup, is_open) },
  { NULL, 0, 0 }
};

static void
newgroup_get_props(NewGroup *group, GPtrArray *props)
{
  object_get_props_from_offsets(&group->element.object, 
                                newgroup_offsets, props);
}

static void
newgroup_set_props(NewGroup *group, GPtrArray *props)
{
  object_set_props_from_offsets(&group->element.object, 
                                newgroup_offsets, props);
  newgroup_update_data(group);
}

static real
newgroup_distance_from(NewGroup *group, Point *point)
{
  Element *elem = &group->element;
  Rectangle rect;

  rect.left = elem->corner.x;
  rect.right = elem->corner.x + elem->width;
  rect.top = elem->corner.y;
  rect.bottom = elem->corner.y + elem->height;
  return distance_rectangle_point(&rect, point);
}

static void
newgroup_select(NewGroup *group, Point *clicked_point,
	   DiaRenderer *interactive_renderer)
{
  element_update_handles(&group->element);
}

static ObjectChange*
newgroup_move_handle(NewGroup *group, Handle *handle,
		Point *to, ConnectionPoint *cp,
		HandleMoveReason reason, ModifierKeys modifiers)
{
  assert(group!=NULL);
  assert(handle!=NULL);
  assert(to!=NULL);

  element_move_handle(&group->element, handle->id, to, cp, reason, modifiers);

  newgroup_update_data(group);

  return NULL;
}

static ObjectChange*
newgroup_move(NewGroup *group, Point *to)
{
  group->element.corner = *to;
  
  newgroup_update_data(group);

  return NULL;
}

static void
newgroup_draw(NewGroup *group, DiaRenderer *renderer)
{
  Point lr_corner;
  Element *elem;
  DiaRendererClass *renderer_ops = DIA_RENDERER_GET_CLASS (renderer);

  
  assert(group != NULL);
  assert(renderer != NULL);

  elem = &group->element;

  lr_corner.x = elem->corner.x + elem->width;
  lr_corner.y = elem->corner.y + elem->height;

  renderer_ops->set_linewidth(renderer, 0.01);
  renderer_ops->set_linestyle(renderer, LINESTYLE_SOLID);
  renderer_ops->set_linejoin(renderer, LINEJOIN_MITER);

  renderer_ops->draw_rect(renderer, 
			  &elem->corner,
			  &lr_corner, 
			  &color_black);
}

static void
newgroup_update_data(NewGroup *group)
{
  Element *elem = &group->element;
  /* ElementBBExtras *extra = &elem->extra_spacing; */
  DiaObject *obj = &elem->object;

  /* Update connections: */
  group->connections[0].pos.x = elem->corner.x;
  group->connections[0].pos.y = elem->corner.y;
  group->connections[1].pos.x = elem->corner.x + elem->width / 2.0;
  group->connections[1].pos.y = elem->corner.y;
  group->connections[2].pos.x = elem->corner.x + elem->width;
  group->connections[2].pos.y = elem->corner.y;
  group->connections[3].pos.x = elem->corner.x;
  group->connections[3].pos.y = elem->corner.y + elem->height / 2.0;
  group->connections[4].pos.x = elem->corner.x + elem->width;
  group->connections[4].pos.y = elem->corner.y + elem->height / 2.0;
  group->connections[5].pos.x = elem->corner.x;
  group->connections[5].pos.y = elem->corner.y + elem->height;
  group->connections[6].pos.x = elem->corner.x + elem->width / 2.0;
  group->connections[6].pos.y = elem->corner.y + elem->height;
  group->connections[7].pos.x = elem->corner.x + elem->width;
  group->connections[7].pos.y = elem->corner.y + elem->height;
  group->connections[8].pos.x = elem->corner.x + elem->width / 2.0;
  group->connections[8].pos.y = elem->corner.y + elem->height / 2.0;

  group->connections[0].directions = DIR_NORTH|DIR_WEST;
  group->connections[1].directions = DIR_NORTH;
  group->connections[2].directions = DIR_NORTH|DIR_EAST;
  group->connections[3].directions = DIR_WEST;
  group->connections[4].directions = DIR_EAST;
  group->connections[5].directions = DIR_SOUTH|DIR_WEST;
  group->connections[6].directions = DIR_SOUTH;
  group->connections[7].directions = DIR_SOUTH|DIR_EAST;
  group->connections[8].directions = DIR_ALL;

  element_update_boundingbox(elem);
  
  obj->position = elem->corner;
  
  element_update_handles(elem);

  if (group->is_open) {
    obj->flags &= ~DIA_OBJECT_GRABS_CHILD_INPUT;
  } else {
    gboolean newlySet = FALSE;
    Layer *layer;
    if (!object_flags_set(obj, DIA_OBJECT_GRABS_CHILD_INPUT)) {
      newlySet = TRUE;
    }
    obj->flags |= DIA_OBJECT_GRABS_CHILD_INPUT;
    if (newlySet) {
      layer = dia_object_get_parent_layer(obj);
      if (layer != NULL) { /* Placed in diagram already */
	GList *children = g_list_prepend(NULL, obj);
	children = parent_list_affected(children);
	/* Remove the group object that stayed at the start of the list,
	   leaving only the children */
	children = g_list_remove_link(children, children);
#if 0 /* this introduces a crircular dependency, does not work on win32 and is bad style everywhere */
	diagram_unselect_objects(layer_get_parent_diagram(layer), children);
#else
	g_warning ("used to call diagram_unselect_objects()");
#endif
	g_list_free(children);
      }
    }
  }
}

static DiaObject *
newgroup_create(Point *startpoint,
	   void *user_data,
	   Handle **handle1,
	   Handle **handle2)
{
  NewGroup *group;
  Element *elem;
  DiaObject *obj;
  int i;

  group = g_malloc0(sizeof(NewGroup));
  elem = &group->element;
  obj = &elem->object;
  
  obj->type = &newgroup_type;

  obj->ops = &newgroup_ops;
  obj->flags |= DIA_OBJECT_CAN_PARENT|DIA_OBJECT_GRABS_CHILD_INPUT;

  elem->corner = *startpoint;
  elem->width = DEFAULT_WIDTH;
  elem->height = DEFAULT_HEIGHT;  

  element_init(elem, 8, NUM_CONNECTIONS);

  for (i=0;i<NUM_CONNECTIONS;i++) {
    obj->connections[i] = &group->connections[i];
    group->connections[i].object = obj;
    group->connections[i].connected = NULL;
  }
  group->connections[8].flags = CP_FLAGS_MAIN;

  newgroup_update_data(group);

  if (handle1 != NULL) {
    *handle1 = NULL;
  }
  if (handle2 != NULL) {
    *handle2 = obj->handles[7];  
  }
  return &group->element.object;
}

static void
newgroup_destroy(NewGroup *group)
{
  element_destroy(&group->element);
}

static DiaObject *
newgroup_copy(NewGroup *group)
{
  int i;
  NewGroup *newgroup;
  Element *elem, *newelem;
  DiaObject *newobj;
  
  elem = &group->element;
  
  newgroup = g_malloc0(sizeof(NewGroup));
  newelem = &newgroup->element;
  newobj = &newelem->object;

  element_copy(elem, newelem);
  
  for (i=0;i<NUM_CONNECTIONS;i++) {
    newobj->connections[i] = &newgroup->connections[i];
    newgroup->connections[i].object = newobj;
    newgroup->connections[i].connected = NULL;
    newgroup->connections[i].pos = group->connections[i].pos;
    newgroup->connections[i].last_pos = group->connections[i].last_pos;
    newgroup->connections[i].flags = group->connections[i].flags;
  }

  return &newgroup->element.object;
}

static void
newgroup_save(NewGroup *group, ObjectNode obj_node, const char *filename)
{
  element_save(&group->element, obj_node);
}

static DiaObject *
newgroup_load(ObjectNode obj_node, int version, const char *filename)
{
  NewGroup *group;
  Element *elem;
  DiaObject *obj;
  int i;

  group = g_malloc0(sizeof(NewGroup));
  elem = &group->element;
  obj = &elem->object;
  
  obj->type = &newgroup_type;
  obj->ops = &newgroup_ops;

  element_load(elem, obj_node);
  element_init(elem, 8, NUM_CONNECTIONS);

  for (i=0;i<NUM_CONNECTIONS;i++) {
    obj->connections[i] = &group->connections[i];
    group->connections[i].object = obj;
    group->connections[i].connected = NULL;
  }
  group->connections[8].flags = CP_FLAGS_MAIN;

  newgroup_update_data(group);

  return &group->element.object;
}
