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
 *
 * File:    compound.c
 *
 * Purpose: This file contains implementation of the database "compound"
 *          code. It is a 'spider' like object with one connection point
 *          and many arms going from the connnection point.
 */

#include "config.h"

#include <glib/gi18n-lib.h>

#include <gtk/gtk.h> /* because of GtkWidgket */
#include "pixmaps/compound.xpm"
#include "object.h"
#include "connectionpoint.h"
#include "diarenderer.h"
#include "attributes.h"
#include "geometry.h"
#include "propinternals.h"

#include "debug.h"
#include "database.h"

/* ------------------------------------------------------------------------ */

#define DEFAULT_NUMARMS 2 /* must be >= 2 */
#define HANDLE_MOUNT_POINT (HANDLE_CUSTOM1)
#define HANDLE_ARM (HANDLE_CUSTOM2)
#define DEFAULT_ARM_X_DISTANCE 0.5
#define DEFAULT_ARM_Y_DISTANCE 0.5

#define DEBUG_DRAW_MP_DIRECTION 0
#define DEBUG_DRAW_BOUNDING_BOX 0

/* ------------------------------------------------------------------------ */

typedef struct _Compound Compound;
typedef struct _CompoundState CompoundState;
typedef struct _ArmHandleState ArmHandleState;
typedef struct _CompoundChange CompoundChange;
typedef struct _MountPointMoveChange MountPointMoveChange;

struct _Compound {
  DiaObject object; /**< inheritance */

  /* there is only one connection point to this object */
  ConnectionPoint mount_point;
  /* the first handle is the handle at the mount_point */
  Handle *handles; /* array of size is `num_arms + 1' */

  gint num_arms;
  real line_width;
  Color line_color;
};

struct _CompoundState {
  ArmHandleState * handle_states;
  gint num_handles; /* num_arms = num_handles-1 */
  real line_width;
  Color line_color;
};

struct _ArmHandleState {
  /* id, type, and connect_type are static for the arms of a
     Compound. there is no need to back them up */
  Point pos;
  ConnectionPoint * connected_to;
};


struct _DiaDBCompoundObjectChange {
  DiaObjectChange  obj_change;
  Compound        *obj;
  CompoundState   *saved_state;
};


DIA_DEFINE_OBJECT_CHANGE (DiaDBCompoundObjectChange,
                          dia_db_compound_object_change)


struct _DiaDBCompoundMountObjectChange {
  DiaObjectChange  obj_change;
  Compound        *obj;
  Point            saved_pos;
};


DIA_DEFINE_OBJECT_CHANGE (DiaDBCompoundMountObjectChange,
                          dia_db_compound_mount_object_change)


/* ------------------------------------------------------------------------ */

static CompoundState * compound_state_new (Compound *);
static void compound_state_free (CompoundState *);
static void compound_state_set (CompoundState *, Compound *);

static DiaObjectChange *compound_change_new           (Compound             *,
                                                       CompoundState        *);
static DiaObjectChange *mount_point_move_change_new   (Compound             *,
                                                       Point                *);

static DiaObject * compound_create (Point *, void *, Handle **, Handle **);
static DiaObject * compound_load (ObjectNode obj_node, int version,DiaContext *ctx);
static void compound_save (Compound *, ObjectNode, DiaContext *ctx);
static void compound_destroy (Compound *);
static void compound_draw (Compound *, DiaRenderer *);
static real compound_distance_from (Compound *, Point *);
static void compound_select (Compound *, Point *, DiaRenderer *);
static DiaObject * compound_copy (Compound *);
static DiaObjectChange *compound_move                    (Compound         *,
                                                          Point            *);
static DiaObjectChange *compound_move_handle             (Compound         *,
                                                          Handle           *,
                                                          Point            *,
                                                          ConnectionPoint  *,
                                                          HandleMoveReason,
                                                          ModifierKeys);
static DiaObjectChange *compound_apply_properties_dialog (Compound         *,
                                                          GtkWidget        *);
static PropDescription *compound_describe_props          (Compound         *);
static void             compound_get_props               (Compound         *,
                                                          GPtrArray        *);
static void             compound_set_props               (Compound         *,
                                                          GPtrArray        *);
static void             compound_update_object           (Compound         *);
static DiaObjectChange *compound_flip_arms_cb            (DiaObject        *,
                                                          Point            *,
                                                          gpointer);
static DiaObjectChange *compound_repos_mount_point_cb    (DiaObject        *,
                                                          Point            *,
                                                          gpointer);
static DiaMenu * compound_object_menu(DiaObject *, Point *);
static gint adjust_handle_count_to (Compound *, gint);
static void setup_mount_point (ConnectionPoint *, DiaObject *, Point *);
static void setup_handle (Handle *, HandleId, HandleType, HandleConnectType);
static void compound_sanity_check (Compound *, gchar *);
static void update_mount_point_directions (Compound *);
static void compound_apply_props (Compound *, GPtrArray *, gboolean);
static void compound_update_data (Compound *);

/* these two are responsible for setting the handles positions */
static void init_positions_for_handles_beginning_at_index (Compound *, gint);
static void init_default_handle_positions (Compound *c);



/* ------------------------------------------------------------------------ */

static ObjectTypeOps compound_type_ops =
  {
    (CreateFunc) compound_create,
    (LoadFunc) compound_load,
    (SaveFunc) compound_save
  };

DiaObjectType compound_type =
  {
    "Database - Compound",  /* name */
    0,                    /* version */
    compound_xpm,          /* pixmap */
    &compound_type_ops /* operations */
  };

static ObjectOps compound_ops =
  {
    (DestroyFunc)          compound_destroy,
    (DrawFunc)             compound_draw,
    (DistanceFunc)         compound_distance_from,
    (SelectFunc)           compound_select,
    (CopyFunc)             compound_copy,
    (MoveFunc)             compound_move,
    (MoveHandleFunc)       compound_move_handle,
    (GetPropertiesFunc)    object_create_props_dialog,
    (ApplyPropertiesDialogFunc)  compound_apply_properties_dialog,
    (ObjectMenuFunc)       compound_object_menu,
    (DescribePropsFunc)    compound_describe_props,
    (GetPropsFunc)         compound_get_props,
    (SetPropsFunc)         compound_set_props,
    (TextEditFunc) 0,
    (ApplyPropertiesListFunc) object_apply_props,
};

/* actually, there is no upper limit - used 10 just for fun */
static PropNumData compound_num_arms_prop_extra_data = {2.0, 10.0, 1.0};

static PropDescription compound_props[] =
  {
    OBJECT_COMMON_PROPERTIES,
    PROP_STD_LINE_COLOUR_OPTIONAL,
    PROP_STD_LINE_WIDTH_OPTIONAL,
    { "num_arms", PROP_TYPE_INT, PROP_FLAG_VISIBLE,
      N_("Number of arms"), NULL, &compound_num_arms_prop_extra_data},

    PROP_DESC_END
  };

static PropOffset compound_offsets[] =
  {
    OBJECT_COMMON_PROPERTIES_OFFSETS,
    { "line_colour", PROP_TYPE_COLOUR, offsetof(Compound, line_color) },
    { PROP_STDNAME_LINE_WIDTH, PROP_STDTYPE_LINE_WIDTH, offsetof(Compound, line_width) },
    { "num_arms", PROP_TYPE_INT, offsetof(Compound, num_arms) },

    { NULL, 0, 0 }
  };

#define CENTER_BOTH 1
#define CENTER_VERTICAL 2
#define CENTER_HORIZONTAL 3

#define FLIP_VERTICAL 1
#define FLIP_HORIZONTAL 2

static DiaMenuItem compound_menu_items[] = {
  { N_("Flip arms vertically"), compound_flip_arms_cb,
    GINT_TO_POINTER(1), DIAMENU_ACTIVE },
  { N_("Flip arms horizontally"), compound_flip_arms_cb,
    GINT_TO_POINTER(2), DIAMENU_ACTIVE },
  { N_("Center mount point vertically"), compound_repos_mount_point_cb,
    GINT_TO_POINTER (CENTER_VERTICAL), DIAMENU_ACTIVE },
  { N_("Center mount point horizontally"), compound_repos_mount_point_cb,
    GINT_TO_POINTER (CENTER_HORIZONTAL), DIAMENU_ACTIVE },
  { N_("Center mount point"), compound_repos_mount_point_cb,
    GINT_TO_POINTER (CENTER_BOTH), DIAMENU_ACTIVE }
};

static DiaMenu compound_menu = {
  N_("Compound"),
  sizeof (compound_menu_items) / sizeof (DiaMenuItem),
  compound_menu_items,
  NULL
};

/* ------------------------------------------------------------------------ */

static CompoundState *
compound_state_new (Compound * c)
{
  CompoundState * state;
  DiaObject * obj;
  gint i, num_handles;

  state = g_new0 (CompoundState, 1);
  obj = &c->object;

  num_handles = c->object.num_handles;
  state->num_handles = num_handles;
  state->line_width = c->line_width;
  state->line_color = c->line_color;
  state->handle_states = g_new (ArmHandleState, num_handles);
  for (i = 0; i < num_handles; i++)
    {
      state->handle_states[i].pos = obj->handles[i]->pos;
      state->handle_states[i].connected_to = obj->handles[i]->connected_to;
    }

  return state;
}

static void
compound_state_set (CompoundState * state, Compound * comp)
{
  gint i, num_handles;
  Handle * h;
  DiaObject * obj;
  ArmHandleState * ahs;

  obj = &comp->object;
  comp->line_width = state->line_width;
  comp->line_color = state->line_color;
  adjust_handle_count_to (comp, state->num_handles);
  num_handles = comp->object.num_handles;
  for (i = 0; i < num_handles; i++)
    {
      h = &comp->handles[i];
      ahs = &state->handle_states[i];

      h->pos = ahs->pos;
      if (h->connected_to != ahs->connected_to)
        {
          if (h->connected_to)
            object_unconnect (obj, h);
          if (ahs->connected_to)
            object_connect (obj, h, ahs->connected_to);
        }
    }
  comp->mount_point.pos = comp->handles[0].pos;
  compound_update_data (comp);
  compound_sanity_check (comp, "Restored state");
}


static void
compound_state_free (CompoundState * state)
{
  g_clear_pointer (&state->handle_states, g_free);
  g_free (state);
}


static void
compound_change_apply (DiaDBCompoundObjectChange *change, DiaObject *obj)
{
  CompoundState *old_state;

  old_state = compound_state_new (change->obj);

  compound_state_set (change->saved_state, change->obj);
  compound_state_free (change->saved_state);

  change->saved_state = old_state;
}


static void
dia_db_compound_object_change_apply (DiaObjectChange *self, DiaObject *obj)
{
  compound_change_apply (DIA_DB_COMPOUND_OBJECT_CHANGE (self), obj);
}


static void
dia_db_compound_object_change_revert (DiaObjectChange *self, DiaObject *obj)
{
  compound_change_apply (DIA_DB_COMPOUND_OBJECT_CHANGE (self), obj);
}


static void
dia_db_compound_object_change_free (DiaObjectChange *self)
{
  DiaDBCompoundObjectChange *change = DIA_DB_COMPOUND_OBJECT_CHANGE (self);

  compound_state_free (change->saved_state);
}


static DiaObjectChange *
compound_change_new (Compound *comp, CompoundState *state)
{
  DiaDBCompoundObjectChange *change;

  change = dia_object_change_new (DIA_DB_TYPE_COMPOUND_OBJECT_CHANGE);

  change->obj = comp;
  change->saved_state = state;

  return DIA_OBJECT_CHANGE (change);
}


static void
mount_point_move_change_apply (DiaDBCompoundMountObjectChange *change, DiaObject *obj)
{
  Compound *comp = change->obj;
  Point old_pos = comp->handles[0].pos;

  comp->handles[0].pos = change->saved_pos;
  comp->mount_point.pos = change->saved_pos;
  compound_update_data (comp);

  change->saved_pos = old_pos;

  compound_sanity_check (comp, "After applying mount point move change");
}


static void
dia_db_compound_mount_object_change_apply (DiaObjectChange *self, DiaObject *obj)
{
  mount_point_move_change_apply (DIA_DB_COMPOUND_MOUNT_OBJECT_CHANGE (self),
                                 obj);
}


static void
dia_db_compound_mount_object_change_revert (DiaObjectChange *self, DiaObject *obj)
{
  mount_point_move_change_apply (DIA_DB_COMPOUND_MOUNT_OBJECT_CHANGE (self),
                                 obj);
}


static void
dia_db_compound_mount_object_change_free (DiaObjectChange *change)
{
  /* currently there is nothing to be done here */
}


static DiaObjectChange *
mount_point_move_change_new (Compound *comp, Point *pos)
{
  DiaDBCompoundMountObjectChange *change;

  change = dia_object_change_new (DIA_DB_TYPE_COMPOUND_MOUNT_OBJECT_CHANGE);

  change->obj = comp;
  change->saved_pos = *pos;

  return DIA_OBJECT_CHANGE (change);
}


/* ------------------------------------------------------------------------ */

static DiaObject *
compound_create (Point * start_point, void * user_data,
                 Handle **handle1, Handle **handle2)
{
  Compound * comp;
  DiaObject * obj;
  Handle * handle;
  gint num_handles;
  gint i;

  comp = g_new0 (Compound, 1);
  obj = &comp->object;

  obj->type = &compound_type;
  obj->ops = &compound_ops;

  comp->num_arms = DEFAULT_NUMARMS;
  comp->line_width = attributes_get_default_linewidth ();
  comp->line_color = attributes_get_foreground ();
  /* init our mount-point */
  setup_mount_point (&comp->mount_point, obj, start_point);

  num_handles = comp->num_arms + 1;
  /* init the inherited object */
  object_init (obj, num_handles, 1);
  obj->connections[0] = &comp->mount_point;

  comp->handles = g_new0 (Handle, num_handles);
  /* init the handle on the mount-point */
  obj->handles[0] = &comp->handles[0];
  setup_handle (obj->handles[0], HANDLE_MOUNT_POINT,
                HANDLE_MAJOR_CONTROL, HANDLE_NONCONNECTABLE);
  /* now init the rest of the handles */
  for (i = 1; i < num_handles; i++)
    {
      handle = &comp->handles[i];
      obj->handles[i] = handle;
      setup_handle (handle, HANDLE_ARM,
                    HANDLE_MINOR_CONTROL, HANDLE_CONNECTABLE_NOBREAK);
    }
  init_default_handle_positions (comp);

  compound_update_data (comp);
  compound_sanity_check (comp, "Created");

  *handle1 = &comp->handles[0];
  *handle2 = &comp->handles[1];

  return &comp->object;
}

static DiaObject *
compound_load (ObjectNode obj_node, int version, DiaContext *ctx)
{
  Compound * comp;
  DiaObject * obj;
  AttributeNode attr;
  DataNode data;
  gint i, num_handles;

  comp = g_new0 (Compound, 1);
  obj = &comp->object;
  /* load the objects position and bounding box */
  object_load (obj, obj_node, ctx);
  /* init the object */
  obj->type = &compound_type;
  obj->ops = &compound_ops;

  /* load the object's handles and its positions */
  attr = object_find_attribute (obj_node, "comp_points");
  g_assert (attr != NULL);
  num_handles = attribute_num_data (attr);
  g_assert (num_handles >= 3);
  /* allocate space for object handles and connectionpoints point arrays */
  object_init (obj, num_handles, 1);
  data = attribute_first_data (attr);
  /* init our mount_point */
  setup_mount_point (&comp->mount_point, obj, NULL);
  data_point (data, &comp->mount_point.pos, ctx);
  obj->connections[0] = &comp->mount_point;
  /* now init the handles */
  comp->num_arms = num_handles-1;
  comp->handles = g_new0 (Handle, num_handles);
  setup_handle (&comp->handles[0], HANDLE_MOUNT_POINT,
                HANDLE_MAJOR_CONTROL, HANDLE_NONCONNECTABLE);
  comp->handles[0].pos = comp->mount_point.pos;
  obj->handles[0] = &comp->handles[0];
  data = data_next (data);
  for (i = 1; i < num_handles; i++)
    {
      obj->handles[i] = &comp->handles[i];
      setup_handle (obj->handles[i], HANDLE_ARM,
                    HANDLE_MINOR_CONTROL, HANDLE_CONNECTABLE_NOBREAK);
      data_point (data, &obj->handles[i]->pos, ctx);
      data = data_next (data);
    }

  /* load remainding properties */
  attr = object_find_attribute (obj_node, PROP_STDTYPE_LINE_WIDTH);
  if (attr == NULL)
    comp->line_width = 0.1;
  else
    comp->line_width = data_real (attribute_first_data (attr), ctx);
  attr = object_find_attribute (obj_node, "line_colour");
  if (attr == NULL)
    comp->line_color = color_black;
  else
    data_color (attribute_first_data (attr), &comp->line_color, ctx);

  compound_update_data (comp);
  compound_sanity_check (comp, "Loaded");
  return &comp->object;
}

static void
compound_save (Compound *comp, ObjectNode obj_node, DiaContext *ctx)
{
  gint i;
  AttributeNode attr;
  DiaObject * obj = &comp->object;

  compound_sanity_check (comp, "Saving");

  object_save (&comp->object, obj_node, ctx);

  attr = new_attribute(obj_node, "comp_points");
  for (i = 0; i < obj->num_handles; i++)
    {
      Handle *h = obj->handles[i];
      data_add_point (attr, &h->pos, ctx);
    }

  attr = new_attribute (obj_node, PROP_STDNAME_LINE_WIDTH);
  data_add_real (attr, comp->line_width, ctx);
  attr = new_attribute (obj_node, "line_color");
  data_add_color (attr, &comp->line_color, ctx);
}


static void
compound_destroy (Compound * comp)
{
  compound_sanity_check (comp, "Destroying");

  object_destroy (&comp->object);
  g_clear_pointer (&comp->handles, g_free);
}


static void
compound_draw (Compound * comp, DiaRenderer * renderer)
{
  gint num_handles;
  Point * mount_point_pos;
  gint i;

  num_handles = comp->object.num_handles;
  mount_point_pos = &comp->mount_point.pos;

  dia_renderer_set_linewidth (renderer, comp->line_width);

  for (i = 1; i < num_handles; i++) {
    dia_renderer_draw_line (renderer,
                            mount_point_pos,
                            &comp->handles[i].pos,
                            &comp->line_color);
#if DEBUG_DRAW_BOUNDING_BOX
 {
   Point ul, br;
   ul.x = comp->object.bounding_box.left;
   ul.y = comp->object.bounding_box.top;
   br.x = comp->object.bounding_box.right;
   br.y = comp->object.bounding_box.bottom;

   dia_renderer_draw_rect (renderer, &ul, &br, &comp->line_color);
 }
#endif
  }

#if DEBUG_DRAW_MP_DIRECTION
 {
   Point p = comp->mount_point.pos;
   Color red = { 1.0, 0.0, 0.0, 1.0 };
   gchar dirs = comp->mount_point.directions;
   if (dirs & DIR_NORTH)
     p.y -= 1.0;
   if (dirs & DIR_SOUTH)
     p.y += 1.0;
   if (dirs & DIR_WEST)
     p.x -= 1.0;
   if (dirs & DIR_EAST)
     p.x += 1.0;

   dia_renderer_draw_line (renderer, &comp->mount_point.pos, &p, &red);
 }
#endif

}

static real
compound_distance_from (Compound * comp, Point * point)
{
  real dist = 0.0;
  gint num_handles = comp->object.num_handles;
  Point * mount_point_pos = &comp->mount_point.pos;
  gint i;

  dist = distance_line_point (mount_point_pos,
                              &comp->handles[1].pos,
                              comp->line_width,
                              point);
  if (dist < 0.000001)
    return 0.0;

  for (i = 2; i < num_handles; i++)
    {
      dist = MIN(distance_line_point (mount_point_pos,
                                      &comp->handles[i].pos,
                                      comp->line_width,
                                      point),
                 dist);
      if (dist < 0.000001)
        return 0.0;
    }
  return dist;
}

static void
compound_select (Compound * comp, Point * clicked,
                 DiaRenderer * interactive_renderer)
{
  compound_update_data (comp);
}

static DiaObject *
compound_copy (Compound * comp)
{
  Compound * copy;
  Handle * ch, * oh;
  DiaObject * copy_obj, * comp_obj;
  gint i, num_handles;

  comp_obj = &comp->object;
  num_handles = comp->object.num_handles;

  g_assert (comp->num_arms >= 2);
  g_assert (comp->num_arms+1 == num_handles);

  copy = g_new0 (Compound, 1);
  copy_obj = &copy->object;
  /* copy the properties */
  copy->num_arms = comp->num_arms;
  copy->line_width = comp->line_width;
  copy->line_color = comp->line_color;

  /* this will allocate the object's pointer arrays for handles and
     connection_points */
  object_copy (comp_obj, copy_obj);
  /* copy the handles */
  copy->handles = g_new (Handle, num_handles);
  for (i = 0; i < num_handles; i++)
    {
      ch = &copy->handles[i];
      oh = &comp->handles[i];
      setup_handle (ch, oh->id, oh->type, oh->connect_type);
      ch->pos = oh->pos;
      copy_obj->handles[i] = ch;
    }
  /* copy the connection/mount point */
  copy_obj->connections[0] = &copy->mount_point;
  setup_mount_point (copy_obj->connections[0],
                     copy_obj,
                     &copy_obj->handles[0]->pos);

  compound_update_data (comp);
  compound_sanity_check (copy, "Copied");
  return &copy->object;
}


static DiaObjectChange *
compound_move (Compound *comp, Point *to)
{
  Point diff;
  Handle * h;
  int i, num_handles;

  diff.x = to->x - comp->object.position.x;
  diff.y = to->y - comp->object.position.y;

  num_handles = comp->object.num_handles;
  for (i = 0; i < num_handles; i++) {
    h = &comp->handles[i];
    point_add (&h->pos, &diff);
  }
  point_add (&comp->mount_point.pos, &diff);

  compound_update_data (comp);
  return NULL;
}


static DiaObjectChange *
compound_move_handle (Compound         *comp,
                      Handle           *handle,
                      Point            *to,
                      ConnectionPoint  *cp,
                      HandleMoveReason  reason,
                      ModifierKeys      modifiers)
{
  if (handle->id == HANDLE_MOUNT_POINT)
    {
      g_assert (handle == &comp->handles[0]);
      comp->mount_point.pos = *to;
    }
  else
    {
      /** ATTENTION: the idea is that while the handles are connected to
       *  an object and that object moves than this compound also moves
       *  (as a whole)! unfortunatelly, this method gets called for each
       *  handle connected and there is not method to register which
       *  would be called after all the handles of this object has been
       *  processed by this routine.  see also
       *  app/connectionpoint_ops.c:diagram_update_connections_object
       *
       *  as a work around we say that when the first handle is moved,
       *  only then we update the position of the mount_point. This
       *  solves the problem for compounds whose handles (all) are
       *  connected to some (even more than one) objects.
       */
      if (reason == HANDLE_MOVE_CONNECTED
          && handle == &comp->handles[1])
        {
          Point diff = *to;
          point_sub (&diff, &handle->pos);

          point_add (&comp->handles[0].pos, &diff);
          point_add (&comp->mount_point.pos, &diff);
        }
    }
  handle->pos = *to;
  compound_update_data (comp);
  return NULL;
}


static DiaObjectChange *
compound_apply_properties_dialog (Compound * comp, GtkWidget * dialog_widget)
{
  CompoundState * state;
  PropDialog *dialog = prop_dialog_from_widget(dialog_widget);

  state = compound_state_new (comp);

  prop_get_data_from_widgets(dialog);
  compound_apply_props (comp, dialog->props, FALSE);

  return compound_change_new (comp, state);
}


static PropDescription *
compound_describe_props (Compound * comp)
{
  if (compound_props[0].quark == 0)
    prop_desc_list_calculate_quarks (compound_props);
  return compound_props;
}

static void
compound_get_props (Compound * comp, GPtrArray * props)
{
  object_get_props_from_offsets(&comp->object, compound_offsets, props);
}

/**
 * Everything I know about is initialize so that this function get called
 * only when the user is applying the default dialog.
 */
static void
compound_set_props (Compound * comp, GPtrArray * props)
{
  compound_apply_props (comp, props, TRUE);
}

static void
compound_apply_props (Compound * comp, GPtrArray * props, gboolean is_default)
{
  gint change_count;
  object_set_props_from_offsets (&comp->object, compound_offsets, props);
  /* comp->num_arms has already been set by
     the call to object_set_props_from_offsets () */
  change_count = adjust_handle_count_to (comp, comp->num_arms+1);
  if (change_count)
    {
      /* if there has been some arms added to the default object
         reinitialize all handles' position */
      if (change_count > 0)
        {
          if (is_default)
            init_default_handle_positions (comp);
          else
            {
              gint new_index = comp->object.num_handles - change_count;
              init_positions_for_handles_beginning_at_index (comp, new_index);
            }
        }
    }
  compound_update_data (comp);
  compound_sanity_check (comp, "After setting properties");
}


/**
 * Determine the bounding box of the compound object and store it in the
 * object. Also update the object's position to the upper left corner of
 * the bounding box.
 */
static void compound_update_object (Compound * comp)
{
  DiaRectangle * bb;
  Handle * h;
  gint i;
  gint num_handles = comp->object.num_handles;

  h = &comp->handles[0];
  bb = &comp->object.bounding_box;
  bb->right = bb->left = h->pos.x;
  bb->top = bb->bottom = h->pos.y;
  for (i = 1; i < num_handles; i++)
    {
      h = &comp->handles[i];
      bb->left = MIN (h->pos.x, bb->left);
      bb->right = MAX(h->pos.x, bb->right);
      bb->top = MIN(h->pos.y, bb->top);
      bb->bottom = MAX(h->pos.y, bb->bottom);
    }
  comp->object.position.x = bb->left;
  comp->object.position.y = bb->top;

#if DEBUG_DRAW_MP_DIRECTION
  /* make the bounding box a bit larger because drawing the direction of
   * the mount point can go beyond the current bounding box.
   */
  bb->left -= 1.0;
  bb->right += 1.0;
  bb->top -= 1.0;
  bb->bottom += 1.0;
#endif
}


/**
 * Add or remove handles, so that comp->object.num_handles will equal to
 * the specified value. When there have been no changes done 0 is
 * returned, otherwise the number of handles removed (negative value) or
 * the number of added handles (positive value) will be returned.
 */
static int
adjust_handle_count_to (Compound * comp, gint to)
{
  DiaObject * obj = &comp->object;
  int old_count = obj->num_handles;
  int new_count = to;
  Handle * h;
  int diff = 0;

  /* we require to have always at least two arms! */
  g_assert (new_count >= 3);

  if (new_count == old_count)
    return diff; /* every thing is ok */

  obj->handles = g_renew (Handle *, obj->handles, new_count);
  obj->num_handles = new_count;
  comp->num_arms = new_count-1;

  if (new_count < old_count) {
    /* removing */
    for (int i = new_count; i < old_count; i++) {
      object_unconnect (obj, &comp->handles[i]);
    }
    comp->handles = g_renew (Handle, comp->handles, new_count);
  } else {
    /* adding */
    comp->handles = g_renew (Handle, comp->handles, new_count);
    for (int i = old_count; i < new_count; i++) {
      h = &comp->handles[i];
      setup_handle (h, HANDLE_ARM,
                    HANDLE_MINOR_CONTROL, HANDLE_CONNECTABLE_NOBREAK);
    }
  }

  for (int i = 0; i < new_count; i++) {
    obj->handles[i] = &comp->handles[i];
  }

  return new_count - old_count;
}


static void
setup_mount_point (ConnectionPoint * mp, DiaObject * obj, Point * pos)
{
  if (pos != NULL)
    mp->pos = *pos;
  mp->object = obj;
  mp->connected = NULL;
  mp->directions = DIR_ALL;
  mp->flags = 0;
}

static void
setup_handle (Handle *h, HandleId id, HandleType type, HandleConnectType ctype)
{
  g_assert (h != NULL);

  h->id = id;
  h->type = type;
  h->pos.x = h->pos.y = 0.0;
  h->connect_type = ctype;
  h->connected_to = NULL;
}

/*
 * Update the directions fields of our mount point, so that when an
 * object is connected to it, let's say an OrthConn object, the
 * connected object will automatically be placed at that side which
 * contains no arms. If there is no free space left, DIR_ALL is assigned
 * which allows new object to connect from any side.
 */
static void
update_mount_point_directions (Compound *c)
{
  Point *p;
  Point mp_pos;
  gint i, below_index;
  gchar used_dir = DIR_NONE;

  below_index = c->object.num_handles;
  mp_pos = c->mount_point.pos;
  for (i = 1; i < below_index; i++)
    {
      p = &c->object.handles[i]->pos;
      used_dir |= (p->x <= mp_pos.x) ? DIR_WEST : DIR_EAST;
      used_dir |= (p->y <= mp_pos.y) ? DIR_NORTH : DIR_SOUTH;
    }

  used_dir = (used_dir ^ DIR_ALL)&DIR_ALL;
  if ((used_dir&DIR_ALL) == DIR_NONE)
    used_dir = DIR_ALL;
  c->mount_point.directions = used_dir;
}

/**
 * Reposition the handles to a default position. The handles will be on
 * one vertical line equally distributed to the left of the mount point.
 * The first handle will be set to the position of the mount_point! The
 * mount point's position must be initialized before this function is
 * called.
 */
static void
init_default_handle_positions (Compound *c)
{
  Handle * h;
  Point run_point;
  gint i, num_handles;

  num_handles = c->object.num_handles;
  h = c->object.handles[0];
  h->pos = c->mount_point.pos;
  run_point = h->pos;
  /* XXX: as you can see the code positions the points on a vertical line
     to the left of the mount point; if you like to change this ...
     here is the right place to do so.
   */
  run_point.x -= DEFAULT_ARM_X_DISTANCE;
  run_point.y -= ((num_handles-2)*DEFAULT_ARM_Y_DISTANCE)/2.0;
  for (i = 1; i < num_handles; i++)
    {
      h = c->object.handles[i];
      h->pos = run_point;
      run_point.y += DEFAULT_ARM_Y_DISTANCE;
    }
}

/**
 * This routine will initialize the position of handles with index >=
 * HINDEX.
 */
static void
init_positions_for_handles_beginning_at_index (Compound *c, gint hindex)
{
  Handle * h;
  Point tmp;
  gint i, num_handles, num_new_gaps;
  real x_inc;
  real y_inc;

  num_handles = c->object.num_handles;
  g_assert (hindex < num_handles);
  num_new_gaps = (num_handles - hindex)-1;

  /* XXX: the following is too simple! there is no code for checking the
     new position not colliding with any other line of this object.
     however, it is sufficient for now, but feel free to improve this
     code if you would like to do so.
  */

  tmp = c->mount_point.pos;
  switch (c->mount_point.directions)
    {
    case DIR_NORTH:
      tmp.y -= DEFAULT_ARM_Y_DISTANCE;
      tmp.x -= (DEFAULT_ARM_X_DISTANCE*num_new_gaps)/2.0;
      x_inc = DEFAULT_ARM_X_DISTANCE;
      y_inc = 0.0;
      break;
    case DIR_EAST:
      tmp.x += DEFAULT_ARM_X_DISTANCE;
      tmp.y -= (DEFAULT_ARM_Y_DISTANCE*num_new_gaps)/2.0;
      x_inc = 0.0;
      y_inc = DEFAULT_ARM_Y_DISTANCE;
      break;
    case DIR_SOUTH:
      tmp.y += DEFAULT_ARM_Y_DISTANCE;
      tmp.x -= (DEFAULT_ARM_X_DISTANCE*num_new_gaps)/2.0;
      x_inc = DEFAULT_ARM_X_DISTANCE;
      y_inc = 0.0;
      break;
    case DIR_WEST:
      tmp.x -= DEFAULT_ARM_X_DISTANCE;
      tmp.y -= (DEFAULT_ARM_Y_DISTANCE*num_new_gaps)/2.0;
      x_inc = 0.0;
      y_inc = DEFAULT_ARM_Y_DISTANCE;
      break;
    default:
      /* XXX this could be more intelligent */
      x_inc = DEFAULT_ARM_X_DISTANCE;
      y_inc = DEFAULT_ARM_Y_DISTANCE;
      tmp.x += x_inc;
      tmp.y += y_inc;
      break;
    }

  for (i = hindex; i < num_handles; i++)
    {
      h = c->object.handles[i];
      h->pos = tmp;
      tmp.x += x_inc;
      tmp.y += y_inc;
    }
}

/**
 * Perform the default updating of dynamic data. In particular this is:
 * - update the object's position and bounding box depending on the
 *   handles position.
 *
 * - make sure that the compound's num_arms correctly corresponds to the
 *   object's number of handles. If not create or remove handles.
 *
 * - update the mount points directions field depending on the position
 *   of the object's handles.
 */
static void
compound_update_data (Compound *c)
{
  /* make sure the number of handles is correct */
  adjust_handle_count_to (c, c->num_arms+1);
  /* this will update the object's position and bounding box */
  compound_update_object (c);
  /* this will compute the mount point's directions field depending on
     the position of the handles */
  update_mount_point_directions (c);
}

static DiaMenu *
compound_object_menu(DiaObject *obj, Point *p)
{
  Compound * comp = (Compound *) obj;
  gchar dir = comp->mount_point.directions;

  if (dir == DIR_ALL)
    {
      compound_menu_items[0].active = 0;
      compound_menu_items[1].active = 0;
    }
  else
    {
      compound_menu_items[0].active = ((dir & DIR_NORTH) == DIR_NORTH
                                       || ((dir & DIR_SOUTH) == DIR_SOUTH))
        ? DIAMENU_ACTIVE
        : 0;
      compound_menu_items[1].active = ((dir & DIR_WEST) == DIR_WEST
                                       || (dir & DIR_EAST) == DIR_EAST)
        ? DIAMENU_ACTIVE
        : 0;
    }

  return &compound_menu;
}


static DiaObjectChange *
compound_flip_arms_cb (DiaObject *obj, Point *pos, gpointer data)
{
  Compound * comp = (Compound *) obj;
  int direction = GPOINTER_TO_INT (data);
  Point * mppos = &comp->mount_point.pos;
  CompoundState * state;
  Handle * h;
  Point * p;
  int num_handles, i;

  state = compound_state_new (comp);

  num_handles = obj->num_handles;
  for (i = 1; i < num_handles; i++)
    {
      h = obj->handles[i];
      object_unconnect (obj, h);
      p = &h->pos;
      if (direction == FLIP_VERTICAL)
        {
          p->y -= mppos->y;
          p->y *= -1.0;
          p->y += mppos->y;
        }
      else
        {
          p->x -= mppos->x;
          p->x *= -1.0;
          p->x += mppos->x;
        }
    }

  compound_update_data (comp);
  compound_sanity_check (comp, "After flipping sides");

  return compound_change_new (comp, state);
}


static DiaObjectChange *
compound_repos_mount_point_cb (DiaObject * obj, Point *clicked, gpointer data)
{
  Compound * comp = (Compound *) obj;
  Handle * h;
  Point old_pos;
  Point pos;
  int what_todo;
  int i, num_handles;

  old_pos = comp->mount_point.pos;
  what_todo = GPOINTER_TO_INT (data);

  h = comp->object.handles[1];
  pos = h->pos;
  num_handles = comp->object.num_handles;
  for (i = 2; i < num_handles; i++)
    {
      h = comp->object.handles[i];
      point_add (&pos, &h->pos);
    }
  switch (what_todo)
    {
    case CENTER_BOTH:
      pos.x /= (real) (num_handles - 1);
      pos.y /= (real) (num_handles - 1);
      break;
    case CENTER_VERTICAL:
      pos.y /= (real) (num_handles - 1);
      pos.x = comp->handles[0].pos.x;
      break;
    case CENTER_HORIZONTAL:
      pos.x /= (real) (num_handles - 1);
      pos.y = comp->handles[0].pos.y;
      break;
    default:
      g_assert (FALSE);
    }

  comp->handles[0].pos = pos;
  comp->mount_point.pos = pos;
  compound_update_data (comp);

  return mount_point_move_change_new (comp, &old_pos);
}

/**
 * The only purpose is to check whether the passed compound object is in
 * a consitent state. If it's not, abort the program with a message!
 */
static void
compound_sanity_check (Compound * c, gchar *msg)
{
  DiaObject * obj;
  Point * ph, * pc;
  gint i;

  obj = &c->object;

  dia_object_sanity_check (obj, msg);

  dia_assert_true (obj->num_connections == 1,
                   "%s: Compound %p has not exactly one connection but %d!\n",
                   msg, c, obj->num_connections);

  dia_assert_true (obj->connections[0] == &c->mount_point,
                   "%s: Compound %p connection mismatch %p != %p!\n",
                   msg, c, obj->connections[0], &c->mount_point);

  dia_assert_true (obj->num_handles >= 3,
                   "%s: Object %p has only %d handles, but at least %d are required!\n",
                   msg, c, obj->num_handles, 3);

  dia_assert_true (obj->num_handles == (c->num_arms+1),
                   "%s: Compound %p has %d handles and %d arms. The number of arms must be the number of handles decreased by one!\n",
                   msg, c, obj->num_handles, c->num_arms);

  for (i = 0; i < obj->num_handles; i++)
    dia_assert_true (obj->handles[i] == &c->handles[i],
                     "%s: Compound %p handles mismatch at %d: %p != %p!\n",
                     msg, c, i, obj->handles[i], &c->handles[i]);

  ph = &obj->handles[0]->pos;
  pc = &c->mount_point.pos;
  dia_assert_true (ph->x == pc->x && ph->y == pc->y,
                   "%s: Compound %p handle[0]/mount_point position mismatch: (%f, %f) != (%f, %f)!\n",
                   msg, c, ph->x, ph->y, pc->x, pc->y);
}
