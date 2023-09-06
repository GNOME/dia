/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * GRAFCET charts support for Dia
 * Copyright (C) 2000, 2001 Cyrille Chepelov
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "object.h"
#include "element.h"
#include "connectionpoint.h"
#include "diarenderer.h"
#include "attributes.h"
#include "properties.h"

#include "grafcet.h"

#include "pixmaps/etape.xpm"

#define STEP_FONT (DIA_FONT_SANS|DIA_FONT_BOLD)
#define STEP_FONT_HEIGHT 1
#define STEP_LINE_WIDTH GRAFCET_GENERAL_LINE_WIDTH
#define STEP_WIDTH 3.0
#define STEP_DECLAREDWIDTH 4.0
#define STEP_HEIGHT 4.0
#define STEP_DOT_RADIUS .35

#define HANDLE_NORTH HANDLE_CUSTOM1
#define HANDLE_SOUTH HANDLE_CUSTOM2


typedef enum {
  STEP_NORMAL,
  STEP_INITIAL,
  STEP_MACROENTRY,
  STEP_MACROEXIT,
  STEP_MACROCALL,
  STEP_SUBPCALL} StepType;


typedef struct _Step {
  Element element;

  ConnectionPoint connections[4];

  char *id;
  int active;
  StepType type;

  DiaFont *font;
  real font_size;
  Color font_color;

  Handle north, south;
  Point SD1, SD2, NU1, NU2;

  /* These are useful points for drawing.
     Must be in sequence, A first, Z last. */
  Point A, B, C, D, E, F, G, H, I, J, Z;
} Step;


static double           step_distance_from  (Step              *step,
                                             Point             *point);
static void             step_select         (Step              *step,
                                             Point             *clicked_point,
                                             DiaRenderer       *interactive_renderer);
static DiaObjectChange *step_move_handle    (Step              *step,
                                             Handle            *handle,
                                             Point             *to,
                                             ConnectionPoint   *cp,
                                             HandleMoveReason   reason,
                                             ModifierKeys       modifiers);
static DiaObjectChange *step_move           (Step              *step,
                                             Point             *to);
static void             step_draw           (Step              *step,
                                             DiaRenderer       *renderer);
static void             step_update_data    (Step              *step);
static DiaObject       *step_create         (Point             *startpoint,
                                             void              *user_data,
                                             Handle           **handle1,
                                             Handle           **handle2);
static void             step_destroy        (Step              *step);
static void             step_been_renamed   (const gchar       *sid);
static DiaObject       *step_load           (ObjectNode         obj_node,
                                             int                version,
                                             DiaContext        *ctx);
static PropDescription *step_describe_props (Step              *step);
static void             step_get_props      (Step              *step,
                                             GPtrArray         *props);
static void             step_set_props      (Step              *step,
                                             GPtrArray         *props);


static ObjectTypeOps step_type_ops = {
  (CreateFunc) step_create,
  (LoadFunc)   step_load,
  (SaveFunc)   object_save_using_properties,
  (GetDefaultsFunc)   NULL,
  (ApplyDefaultsFunc) NULL,
};


DiaObjectType step_type = {
  "GRAFCET - Step", /* name */
  0,             /* version */
  etape_xpm,      /* pixmap */
  &step_type_ops,    /* ops */
  NULL, /* pixmap_file */
  NULL, /* default_user_data */
  NULL, /* prop_descs */
  NULL, /* prop_offsets */
  DIA_OBJECT_HAS_VARIANTS /* flags */
};


static ObjectOps step_ops = {
  (DestroyFunc)         step_destroy,
  (DrawFunc)            step_draw,
  (DistanceFunc)        step_distance_from,
  (SelectFunc)          step_select,
  (CopyFunc)            object_copy_using_properties,
  (MoveFunc)            step_move,
  (MoveHandleFunc)      step_move_handle,
  (GetPropertiesFunc)   object_create_props_dialog,
  (ApplyPropertiesDialogFunc) object_apply_props_from_dialog,
  (ObjectMenuFunc)      NULL,
  (DescribePropsFunc)   step_describe_props,
  (GetPropsFunc)        step_get_props,
  (SetPropsFunc)        step_set_props,
  (TextEditFunc) 0,
  (ApplyPropertiesListFunc) object_apply_props,
};

PropEnumData step_style[] = {
  { N_("Regular step"),STEP_NORMAL },
  { N_("Initial step"),STEP_INITIAL },
  { N_("Macro entry step"),STEP_MACROENTRY },
  { N_("Macro exit step"),STEP_MACROEXIT },
  { N_("Macro call step"),STEP_MACROCALL },
  { N_("Subprogram call step"), STEP_SUBPCALL },
  { NULL }
};


static PropDescription step_props[] = {
  ELEMENT_COMMON_PROPERTIES,
  { "id", PROP_TYPE_STRING,
    PROP_FLAG_VISIBLE|PROP_FLAG_NO_DEFAULTS|PROP_FLAG_DONT_MERGE,
    N_("Step name"),N_("The name of the step")},
  { "type", PROP_TYPE_ENUM,
    PROP_FLAG_VISIBLE|PROP_FLAG_NO_DEFAULTS|PROP_FLAG_DONT_MERGE,
    N_("Step type"),N_("The kind of step"),step_style},
  { "active", PROP_TYPE_BOOL, PROP_FLAG_VISIBLE,
    N_("Active"), N_("Shows a red dot to figure the step's activity")},
  { "font", PROP_TYPE_FONT, PROP_FLAG_VISIBLE,
    N_("Font"),NULL},
  { "font_size", PROP_TYPE_FONTSIZE, PROP_FLAG_VISIBLE,
    N_("Font size"), NULL, &prop_std_text_height_data},
  { "font_color", PROP_TYPE_COLOUR, PROP_FLAG_VISIBLE,
    N_("Text color"), NULL },
  { "north_pos", PROP_TYPE_POINT, 0},
  { "south_pos", PROP_TYPE_POINT, 0},
  PROP_DESC_END
};


static PropDescription *
step_describe_props (Step *step)
{
  if (step_props[0].quark == 0) {
    prop_desc_list_calculate_quarks (step_props);
  }
  return step_props;
}


static PropOffset step_offsets[] = {
  ELEMENT_COMMON_PROPERTIES_OFFSETS,
  { "id", PROP_TYPE_STRING, offsetof (Step, id)},
  { "type", PROP_TYPE_ENUM, offsetof (Step, type)},
  { "active", PROP_TYPE_BOOL, offsetof (Step, active)},
  { "font", PROP_TYPE_FONT, offsetof (Step, font)},
  { "font_size", PROP_TYPE_FONTSIZE, offsetof (Step, font_size)},
  { "font_color", PROP_TYPE_COLOUR, offsetof (Step, font_color)},
  { "north_pos", PROP_TYPE_POINT, offsetof (Step, north.pos)},
  { "south_pos", PROP_TYPE_POINT, offsetof (Step, south.pos)},
  { NULL,0,0 }
};


static void
step_get_props (Step *step, GPtrArray *props)
{
  object_get_props_from_offsets (DIA_OBJECT (step),
                                 step_offsets,
                                 props);
}


static void
step_set_props (Step *step, GPtrArray *props)
{
  object_set_props_from_offsets (DIA_OBJECT (step),
                                 step_offsets,
                                 props);
  step_been_renamed (step->id);
  step_update_data (step);
}


/* the following two functions try to be clever when allocating
   step numbers */

static int __stepnum = 0;
static int __Astyle = 0;


static gchar *
new_step_name (void)
{
  char snum[16];
  char *p = snum;

  if (__Astyle) {
    *p++ = 'A';
  }

  g_snprintf (p, sizeof (snum)-2, "%d", __stepnum++);
  return g_strdup (snum);
}


static void
step_been_renamed (const gchar *sid)
{
  gchar *endptr;
  long int snum;
  if (!sid) return;
  if (*sid == 'A') {
    sid++; /* for the "A01" numbering style */
    __Astyle = 1;
  } else {
    __Astyle = 0;
  }
  endptr = NULL;
  snum = strtol (sid, &endptr, 10);
  if (*endptr == '\0') {
    __stepnum = snum + 1;
  }
}


static Color color_red = { 1.0f, 0.0f, 0.0f, 1.0f };


static real
step_distance_from (Step *step, Point *point)
{
  Element *elem = &step->element;
  DiaRectangle rect;
  real dist;

  dist = distance_line_point (&step->north.pos, &step->NU1,
                              STEP_LINE_WIDTH, point);
  dist = MIN (dist,
              distance_line_point (&step->NU1, &step->NU2,
                                   STEP_LINE_WIDTH, point));
  dist = MIN (dist,
              distance_line_point (&step->NU2, &step->A,
                                   STEP_LINE_WIDTH, point));
  dist = MIN (dist,
              distance_line_point (&step->D, &step->SD1,
                                   STEP_LINE_WIDTH, point));
  dist = MIN (dist,
              distance_line_point (&step->SD1, &step->SD2,
                                   STEP_LINE_WIDTH, point));
  dist = MIN (dist,
              distance_line_point (&step->SD2, &step->south.pos,
                                   STEP_LINE_WIDTH, point));

  rect.left = elem->corner.x;
  rect.right = elem->corner.x + elem->width;
  rect.top = elem->corner.y;
  rect.bottom = elem->corner.y + elem->height;
  dist = MIN (dist, distance_rectangle_point (&rect, point));
  return dist;
}


static void
step_select (Step        *step,
             Point       *clicked_point,
             DiaRenderer *interactive_renderer)
{
  element_update_handles (&step->element);
}


static DiaObjectChange *
step_move_handle (Step             *step,
                  Handle           *handle,
                  Point            *to,
                  ConnectionPoint  *cp,
                  HandleMoveReason  reason,
                  ModifierKeys      modifiers)
{
  g_return_val_if_fail (step != NULL, NULL);
  g_return_val_if_fail (handle != NULL, NULL);
  g_return_val_if_fail (to != NULL, NULL);

  switch(handle->id) {
    case HANDLE_NORTH:
      step->north.pos = *to;
      if (step->north.pos.y > step->A.y) {
        step->north.pos.y = step->A.y;
      }
      break;
    case HANDLE_SOUTH:
      step->south.pos = *to;
      if (step->south.pos.y < step->D.y) {
        step->south.pos.y = step->D.y;
      }
      break;
    case HANDLE_MOVE_STARTPOINT:
    case HANDLE_MOVE_ENDPOINT:
    case HANDLE_RESIZE_NW:
    case HANDLE_RESIZE_N:
    case HANDLE_RESIZE_NE:
    case HANDLE_RESIZE_W:
    case HANDLE_RESIZE_E:
    case HANDLE_RESIZE_SW:
    case HANDLE_RESIZE_S:
    case HANDLE_RESIZE_SE:
    case HANDLE_CUSTOM3:
    case HANDLE_CUSTOM4:
    case HANDLE_CUSTOM5:
    case HANDLE_CUSTOM6:
    case HANDLE_CUSTOM7:
    case HANDLE_CUSTOM8:
    case HANDLE_CUSTOM9:
    default:
      element_move_handle (&step->element,
                           handle->id,
                           to,
                           cp,
                           reason,
                           modifiers);
  }

  step_update_data (step);

  return NULL;
}


static DiaObjectChange *
step_move (Step *step, Point *to)
{
  Point delta = *to;
  point_sub (&delta, &step->element.corner);
  step->element.corner = *to;
  point_add (&step->north.pos, &delta);
  point_add (&step->south.pos, &delta);

  step_update_data (step);

  return NULL;
}


static void
step_draw (Step *step, DiaRenderer *renderer)
{
  Point pts[4];
  g_return_if_fail (step != NULL);
  g_return_if_fail (renderer != NULL);

  dia_renderer_set_fillstyle (renderer, DIA_FILL_STYLE_SOLID);
  dia_renderer_set_linewidth (renderer, STEP_LINE_WIDTH);
  dia_renderer_set_linestyle (renderer, DIA_LINE_STYLE_SOLID, 0);
  dia_renderer_set_linejoin (renderer, DIA_LINE_JOIN_MITER);

  pts[0] = step->north.pos;
  pts[1] = step->NU1;
  pts[2] = step->NU2;
  pts[3] = step->A;
  dia_renderer_draw_polyline (renderer,
                              pts,
                              sizeof(pts)/sizeof(pts[0]),
                              &color_black);
  pts[0] = step->D;
  pts[1] = step->SD1;
  pts[2] = step->SD2;
  pts[3] = step->south.pos;
  dia_renderer_draw_polyline (renderer,
                              pts,
                              sizeof(pts)/sizeof(pts[0]),
                              &color_black);

  if ((step->type == STEP_INITIAL) ||
      (step->type == STEP_MACROCALL) ||
      (step->type == STEP_SUBPCALL)) {
    dia_renderer_draw_rect (renderer, &step->I, &step->J, &color_white, &color_black);
    dia_renderer_draw_rect (renderer, &step->E, &step->F, NULL, &color_black);
  } else {
    dia_renderer_draw_rect (renderer, &step->E, &step->F, &color_white, &color_black);
  }

  if (step->type != STEP_MACROENTRY) {
    dia_renderer_draw_line (renderer,&step->A,&step->B,&color_black);
  }
  if (step->type != STEP_MACROEXIT) {
    dia_renderer_draw_line (renderer,&step->C,&step->D,&color_black);
  }

  dia_renderer_set_font (renderer, step->font, step->font_size);

  dia_renderer_draw_string (renderer,
                            step->id,
                            &step->G,
                            DIA_ALIGN_CENTRE,
                            &step->font_color);
  if (step->active) {
    dia_renderer_draw_ellipse (renderer,
                               &step->H,
                               STEP_DOT_RADIUS,STEP_DOT_RADIUS,
                               &color_red,
                               NULL);
  }
}


static void
step_update_data (Step *step)
{
  Element *elem = &step->element;
  DiaObject *obj = &elem->object;
  ElementBBExtras *extra = &elem->extra_spacing;
  Point *p,ulc;

  ulc = elem->corner;
  ulc.x += ((STEP_DECLAREDWIDTH - STEP_WIDTH) / 2.0); /* we cheat a little */

  step->A.x = 0.0 + (STEP_WIDTH / 2.0); step->A.y = 0.0;
  step->D.x = 0.0 + (STEP_WIDTH / 2.0); step->D.y = 0.0 + STEP_HEIGHT;

  step->E.x = 0.0; step->E.y = 0.5;
  step->F.x = STEP_WIDTH; step->F.y = STEP_HEIGHT- 0.5;


  switch (step->type) {
    case STEP_INITIAL:
      step->I.x = step->E.x - 2 * STEP_LINE_WIDTH;
      step->I.y = step->E.y - 2 * STEP_LINE_WIDTH;
      step->J.x = step->F.x + 2 * STEP_LINE_WIDTH;
      step->J.y = step->F.y + 2 * STEP_LINE_WIDTH;

      step->B.x = step->A.x; step->B.y = step->I.y;
      step->C.x = step->D.x; step->C.y = step->J.y;
      step->Z.x = step->J.x; step->Z.y = STEP_HEIGHT / 2;
      break;
    case STEP_MACROCALL:
      step->I.x = step->E.x;
      step->I.y = step->E.y - 2 * STEP_LINE_WIDTH;
      step->J.x = step->F.x;
      step->J.y = step->F.y + 2 * STEP_LINE_WIDTH;

      step->B.x = step->A.x; step->B.y = step->I.y;
      step->C.x = step->D.x; step->C.y = step->J.y;
      step->Z.x = step->J.x; step->Z.y = STEP_HEIGHT / 2;
      break;
    case STEP_SUBPCALL:
      step->I.x = step->E.x - 2 * STEP_LINE_WIDTH;
      step->I.y = step->E.y;
      step->J.x = step->F.x + 2 * STEP_LINE_WIDTH;
      step->J.y = step->F.y;

      step->B.x = step->A.x; step->B.y = step->I.y;
      step->C.x = step->D.x; step->C.y = step->J.y;
      step->Z.x = step->J.x; step->Z.y = STEP_HEIGHT / 2;
      break;
    case STEP_NORMAL:
    case STEP_MACROENTRY:
    case STEP_MACROEXIT:
    default: /* regular or macro end steps */
      step->B.x = step->A.x; step->B.y = step->E.y;
      step->C.x = step->D.x; step->C.y = step->F.y;
      step->Z.x = step->F.x; step->Z.y = STEP_HEIGHT / 2;
  }

  step->G.x = step->A.x;
  step->G.y = (STEP_HEIGHT / 2)  + (.3 * step->font_size);
  step->H.x = step->E.x + (1.2 * STEP_DOT_RADIUS);
  step->H.y = step->F.y - (1.2 * STEP_DOT_RADIUS);

  for (p = &(step->A); p <= &(step->Z); p++) {
    point_add (p, &ulc);
  }

  /* Update handles: */
  if (step->north.pos.x == -65536.0) {
    step->north.pos = step->A;
    step->south.pos = step->D;
  }
  step->NU1.x = step->north.pos.x;
  step->NU2.x = step->A.x;
  step->NU1.y = step->NU2.y = (step->north.pos.y + step->A.y) / 2.0;
  step->SD1.x = step->D.x;
  step->SD2.x = step->south.pos.x;
  step->SD1.y = step->SD2.y = (step->south.pos.y + step->D.y) / 2.0;

  /* Update connections: */
  step->connections[0].pos = step->A;
  step->connections[0].directions = DIR_NORTH;
  step->connections[1].pos = step->D;
  step->connections[1].directions = DIR_SOUTH;
  step->connections[2].pos = step->Z;
  step->connections[2].directions = DIR_EAST;
  step->connections[3].pos = step->H;
  step->connections[3].directions = DIR_WEST;

  /* recalc the bounding box : */
  if ((step->type == STEP_INITIAL) || (step->type == STEP_SUBPCALL)) {
    extra->border_trans = 2.5 * STEP_LINE_WIDTH;
  } else {
    extra->border_trans = STEP_LINE_WIDTH / 2;
  }

  element_update_boundingbox (elem);
  rectangle_add_point (&obj->bounding_box, &step->north.pos);
  rectangle_add_point (&obj->bounding_box, &step->south.pos);

  obj->position = elem->corner;

  element_update_handles (elem);
}


static DiaObject *
step_create (Point   *startpoint,
             void    *user_data,
             Handle **handle1,
             Handle **handle2)
{
  Step *step;
  Element *elem;
  DiaObject *obj;
  int i;
  int type;

  step = g_new0 (Step, 1);
  elem = &step->element;
  obj = &elem->object;

  obj->type = &step_type;
  obj->ops = &step_ops;

  elem->corner = *startpoint;
  elem->width = STEP_DECLAREDWIDTH;
  elem->height = STEP_HEIGHT;

  element_init (elem, 10, 4);

  for (i = 0; i < 4; i++) {
    obj->connections[i] = &step->connections[i];
    step->connections[i].object = obj;
    step->connections[i].connected = NULL;
  }

  step->id = new_step_name ();
  step->active = 0;
  step->font = dia_font_new_from_style (STEP_FONT, STEP_FONT_HEIGHT);
  step->font_size = STEP_FONT_HEIGHT;
  step->font_color = color_black;

  type = GPOINTER_TO_INT (user_data);
  switch (type) {
    case STEP_NORMAL:
    case STEP_INITIAL:
    case STEP_MACROENTRY:
    case STEP_MACROEXIT:
    case STEP_MACROCALL:
    case STEP_SUBPCALL:
      step->type = type;
      break;
    default:
      step->type = STEP_NORMAL;
  }


  for (i = 0; i < 8; i++) {
    obj->handles[i]->type = HANDLE_NON_MOVABLE;
  }
  obj->handles[8] = &step->north;
  obj->handles[9] = &step->south;
  step->north.connect_type = HANDLE_CONNECTABLE;
  step->north.type = HANDLE_MAJOR_CONTROL;
  step->north.id = HANDLE_NORTH;
  step->south.connect_type = HANDLE_CONNECTABLE;
  step->south.type = HANDLE_MAJOR_CONTROL;
  step->south.id = HANDLE_SOUTH;
  step->north.pos.x = -65536.0; /* magic */

  step_update_data (step);

  *handle1 = NULL;
  *handle2 = obj->handles[0];

  return &step->element.object;
}


static void
step_destroy (Step *step)
{
  g_clear_object (&step->font);
  g_clear_pointer (&step->id, g_free);
  element_destroy (&step->element);
}


static DiaObject *
step_load (ObjectNode obj_node, int version, DiaContext *ctx)
{
  return object_load_using_properties (&step_type,
                                       obj_node,
                                       version,
                                       ctx);
}
