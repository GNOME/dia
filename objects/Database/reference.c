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
 * File:    reference.c
 *
 * Purpose: This file contains implementation of the database "reference"
 *          code.
 */

#include "config.h"

#include <glib/gi18n-lib.h>

#include "database.h"
#include "arrows.h"
#include "attributes.h"
#include "properties.h"
#include "diarenderer.h"
#include "pixmaps/reference.xpm"

/* ------------------------------------------------------------------------ */

static DiaObject * reference_create (Point *, void *, Handle **, Handle **);
static void reference_destroy (TableReference *);
static void reference_draw (TableReference *, DiaRenderer *);
static real reference_distance_from (TableReference *, Point *);
static void reference_select (TableReference *, Point *, DiaRenderer *);
static DiaObjectChange *reference_move           (TableReference   *,
                                                  Point            *);
static DiaObjectChange *reference_move_handle    (TableReference   *,
                                                  Handle           *,
                                                  Point            *,
                                                  ConnectionPoint  *,
                                                  HandleMoveReason,
                                                  ModifierKeys);
static PropDescription * reference_describe_props (TableReference *);
static void reference_get_props (TableReference *, GPtrArray *);
static void reference_set_props (TableReference *, GPtrArray *);
static void reference_update_data (TableReference *);
static DiaObject * reference_load (ObjectNode obj_node, int version,DiaContext *ctx);
static void             update_desc_data         (Point            *,
                                                  DiaAlignment     *,
                                                  Point            *,
                                                  Point            *,
                                                  Orientation,
                                                  double,
                                                  double);
static void             get_desc_bbox            (DiaRectangle     *,
                                                  char             *,
                                                  double,
                                                  Point            *,
                                                  DiaAlignment,
                                                  DiaFont          *,
                                                  double);
static DiaObjectChange *reference_add_segment_cb (DiaObject        *,
                                                  Point            *,
                                                  gpointer);
static DiaObjectChange *reference_del_segment_cb (DiaObject        *,
                                                  Point            *,
                                                  gpointer);
static DiaMenu * reference_object_menu(TableReference *, Point *);

/* ------------------------------------------------------------------------ */

static ObjectTypeOps reference_type_ops =
{
  (CreateFunc) reference_create,
  (LoadFunc)   reference_load,
  (SaveFunc)   object_save_using_properties,
  (GetDefaultsFunc)   NULL,
  (ApplyDefaultsFunc) NULL
};

DiaObjectType reference_type =
{
  "Database - Reference", /* name */
  0,                   /* version */
  reference_xpm,        /* pixmap */
  &reference_type_ops,     /* ops */
  NULL,            /* pixmap_file */
  NULL       /* default_user_data */
};

static ObjectOps reference_ops = {
  (DestroyFunc)         reference_destroy,
  (DrawFunc)            reference_draw,
  (DistanceFunc)        reference_distance_from,
  (SelectFunc)          reference_select,
  (CopyFunc)            object_copy_using_properties,
  (MoveFunc)            reference_move,
  (MoveHandleFunc)      reference_move_handle,
  (GetPropertiesFunc)   object_create_props_dialog,
  (ApplyPropertiesDialogFunc) object_apply_props_from_dialog,
  (ObjectMenuFunc)      reference_object_menu,
  (DescribePropsFunc)   reference_describe_props,
  (GetPropsFunc)        reference_get_props,
  (SetPropsFunc)        reference_set_props,
  (TextEditFunc) 0,
  (ApplyPropertiesListFunc) object_apply_props,
};

static PropNumData reference_corner_radius_data = { 0.0, 10.0, 0.1 };

static PropDescription reference_props[] =
  {
    ORTHCONN_COMMON_PROPERTIES,
    PROP_STD_TEXT_COLOUR_OPTIONS(PROP_FLAG_VISIBLE|PROP_FLAG_STANDARD|PROP_FLAG_OPTIONAL),
    PROP_STD_LINE_COLOUR_OPTIONAL,
    PROP_STD_LINE_WIDTH_OPTIONAL,
    PROP_STD_LINE_STYLE_OPTIONAL,
    { "corner_radius", PROP_TYPE_REAL, PROP_FLAG_VISIBLE,
      N_("Corner radius"), NULL, &reference_corner_radius_data },
    PROP_STD_END_ARROW,
    { "start_point_desc", PROP_TYPE_STRING, PROP_FLAG_VISIBLE,
      N_("Start description"), NULL, NULL },
    { "end_point_desc", PROP_TYPE_STRING, PROP_FLAG_VISIBLE,
      N_("End description"), NULL, NULL },
    PROP_MULTICOL_BEGIN("reference"),
    PROP_MULTICOL_COLUMN("font"),
    { "normal_font", PROP_TYPE_FONT, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
      N_("Font"), NULL, NULL },
    PROP_MULTICOL_COLUMN("height"),
    { "normal_font_height", PROP_TYPE_REAL, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
      N_(" "), NULL, NULL },
    PROP_MULTICOL_END("reference"),
    PROP_DESC_END
  };

static PropOffset reference_offsets[] =
  {
    ORTHCONN_COMMON_PROPERTIES_OFFSETS,
    { "text_colour", PROP_TYPE_COLOUR, offsetof(TableReference, text_color) },
    { "line_colour", PROP_TYPE_COLOUR, offsetof(TableReference, line_color) },
    { PROP_STDNAME_LINE_WIDTH, PROP_STDTYPE_LINE_WIDTH, offsetof(TableReference, line_width) },
    { "line_style", PROP_TYPE_LINESTYLE, offsetof(TableReference, line_style),
      offsetof(TableReference, dashlength) },
    { "end_arrow", PROP_TYPE_ARROW, offsetof(TableReference, end_arrow) },
    { "corner_radius", PROP_TYPE_REAL, offsetof(TableReference, corner_radius) },
    { "normal_font", PROP_TYPE_FONT, offsetof(TableReference, normal_font) },
    { "normal_font_height", PROP_TYPE_REAL, offsetof(TableReference, normal_font_height) },
    { "start_point_desc", PROP_TYPE_STRING, offsetof(TableReference, start_point_desc) },
    { "end_point_desc", PROP_TYPE_STRING, offsetof(TableReference, end_point_desc) },
    { NULL, 0, 0 }
  };

static DiaMenuItem reference_menu_items[] =
  {
    { N_("Add segment"), reference_add_segment_cb, NULL, DIAMENU_ACTIVE },
    { N_("Delete segment"), reference_del_segment_cb, NULL, DIAMENU_ACTIVE },
    ORTHCONN_COMMON_MENUS
  };

static DiaMenu reference_menu =
  {
    N_("Reference"),
    sizeof (reference_menu_items) / sizeof (DiaMenuItem),
    reference_menu_items,
    NULL
  };

/* ------------------------------------------------------------------------ */

static DiaObject *
reference_create (Point *startpoint,
                  void *user_data,
                  Handle **handle1,
                  Handle **handle2)
{
  TableReference * ref;
  OrthConn * orth;
  DiaObject * obj;

  ref = g_new0 (TableReference, 1);
  orth = &ref->orth;
  obj = &orth->object;

  obj->type = &reference_type;
  obj->ops = &reference_ops;

  orthconn_init (orth, startpoint);

  ref->normal_font =
    dia_font_new_from_style(DIA_FONT_MONOSPACE, 0.6);
  ref->normal_font_height = 0.6;
  ref->line_width = attributes_get_default_linewidth ();
  attributes_get_default_line_style (&ref->line_style, &ref->dashlength);
  ref->text_color = color_black;
  ref->line_color = attributes_get_foreground ();
  ref->end_arrow = attributes_get_default_end_arrow ();
  ref->corner_radius = 0.0;
  ref->start_point_desc = g_strdup ("1");
  ref->end_point_desc = g_strdup ("n");

  *handle1 = orth->handles[0];
  *handle2 = orth->handles[orth->numpoints-2];

  reference_update_data (ref);

  return &ref->orth.object;
}


static void
reference_destroy (TableReference * ref)
{
  orthconn_destroy (&ref->orth);

  g_clear_pointer (&ref->start_point_desc, g_free);
  g_clear_pointer (&ref->end_point_desc, g_free);
}


static DiaObject *
reference_load (ObjectNode obj_node, int version, DiaContext *ctx)
{
  DiaObject *obj = object_load_using_properties (&reference_type,
                                                 obj_node,
                                                 version,
                                                 ctx);

  return obj;
}


static void
reference_draw (TableReference *ref, DiaRenderer *renderer)
{
  OrthConn * orth = &ref->orth;
  Point * points;
  int num_points;

  points = &orth->points[0];
  num_points = orth->numpoints;

  dia_renderer_set_linewidth (renderer, ref->line_width);
  dia_renderer_set_linestyle (renderer, ref->line_style, ref->dashlength);
  dia_renderer_set_linejoin (renderer, DIA_LINE_JOIN_MITER);
  dia_renderer_set_linecaps (renderer, DIA_LINE_CAPS_BUTT);

  dia_renderer_draw_rounded_polyline_with_arrows (renderer,
                                                  points,
                                                  num_points,
                                                  ref->line_width,
                                                  &ref->line_color,
                                                  NULL,
                                                  &ref->end_arrow,
                                                  ref->corner_radius);

  dia_renderer_set_font (renderer, ref->normal_font, ref->normal_font_height);

  if (IS_NOT_EMPTY (ref->start_point_desc)) {
    dia_renderer_draw_string (renderer,
                              ref->start_point_desc,
                              &ref->sp_desc_pos,
                              ref->sp_desc_text_align,
                              &ref->text_color);
  }

  if (IS_NOT_EMPTY (ref->end_point_desc)) {
    dia_renderer_draw_string (renderer,
                              ref->end_point_desc,
                              &ref->ep_desc_pos,
                              ref->ep_desc_text_align,
                              &ref->text_color);
  }
}


static double
reference_distance_from (TableReference *ref, Point *point)
{
  DiaRectangle rect;
  OrthConn *orth;
  double dist;

  orth = &ref->orth;
  dist = orthconn_distance_from (orth, point, ref->line_width);

  if (IS_NOT_EMPTY (ref->start_point_desc)) {
    get_desc_bbox (&rect,
                   ref->start_point_desc,
                   ref->sp_desc_width,
                   &ref->sp_desc_pos,
                   ref->sp_desc_text_align,
                   ref->normal_font,
                   ref->normal_font_height);

    dist = MIN (distance_rectangle_point (&rect, point), dist);

    if (dist < 0.000001) {
      return 0.0;
    }
  }

  if (IS_NOT_EMPTY (ref->start_point_desc)) {
    get_desc_bbox (&rect,
                   ref->end_point_desc,
                   ref->ep_desc_width,
                   &ref->ep_desc_pos,
                   ref->ep_desc_text_align,
                   ref->normal_font,
                   ref->normal_font_height);
    dist = MIN (distance_rectangle_point (&rect, point), dist);
  }

  return dist;
}


static void
reference_select (TableReference *ref,
                  Point          *clicked_point,
                  DiaRenderer    *interactive_renderer)
{
  orthconn_update_data (&ref->orth);
}


static DiaObjectChange *
reference_move (TableReference *ref, Point *to)
{
  DiaObjectChange *change;

  change = orthconn_move (&ref->orth, to);
  reference_update_data (ref);

  return change;
}


static DiaObjectChange *
reference_move_handle (TableReference   *ref,
                       Handle           *handle,
                       Point            *to,
                       ConnectionPoint  *cp,
                       HandleMoveReason  reason,
                       ModifierKeys      modifiers)
{
  DiaObjectChange *change;

  change = orthconn_move_handle (&ref->orth,
                                 handle,
                                 to,
                                 cp,
                                 reason,
                                 modifiers);
  reference_update_data (ref);

  return change;
}


static PropDescription *
reference_describe_props (TableReference *ref)
{
  if (reference_props[0].quark == 0)
    prop_desc_list_calculate_quarks (reference_props);
  return reference_props;
}

static void
reference_get_props (TableReference *ref, GPtrArray *props)
{
  object_get_props_from_offsets(&ref->orth.object,
                                reference_offsets,
                                props);
}

static void
reference_set_props (TableReference *ref, GPtrArray *props)
{
  object_set_props_from_offsets(&ref->orth.object,
                                reference_offsets,
                                props);
  reference_update_data (ref);
}

static void
reference_update_data (TableReference * ref)
{
  OrthConn * orth = &ref->orth;
  DiaRectangle rect;
  PolyBBExtras *extra = &orth->extra_spacing;

  orthconn_update_data (orth);

  /* account for line_width in bounding box calculation */
  extra->start_trans =
    extra->start_long =
    extra->middle_trans =
    extra->end_trans =
    extra->end_long = ref->line_width/2.0;
  orthconn_update_boundingbox (orth);

  /* compute the position of the start point description */
  if (IS_NOT_EMPTY(ref->start_point_desc))
    {
      gint p_index = 0;
      Point * pos = &orth->points[p_index];
      Point * next_pos = &orth->points[p_index+1];
      Orientation orient = orth->orientation[p_index];

      /* if pos and next_pos are the same take the next point following
         next_pos */
      if (pos->x == next_pos->x && pos->y == next_pos->y)
        {
          next_pos = &orth->points[p_index+2];
          orient = (pos->y == next_pos->y) ? HORIZONTAL : VERTICAL;
        }

      ref->sp_desc_width = dia_font_string_width (ref->start_point_desc,
                                                  ref->normal_font,
                                                  ref->normal_font_height);

      update_desc_data (&ref->sp_desc_pos, &ref->sp_desc_text_align,
                        pos, next_pos, orient, ref->line_width,
                        ref->normal_font_height);

      get_desc_bbox (&rect, ref->start_point_desc, ref->sp_desc_width,
                     &ref->sp_desc_pos, ref->sp_desc_text_align,
                     ref->normal_font, ref->normal_font_height);
      rectangle_union (&orth->object.bounding_box, &rect);
    }
  else
    {
      ref->sp_desc_width = 0.0;
    }

  /* compute the position of the start point description */
  if (IS_NOT_EMPTY(ref->end_point_desc))
    {
      gint p_index = orth->numpoints - 1;
      Point * pos = &orth->points[p_index];
      Point * next_pos = &orth->points[p_index-1];
      Orientation orient = orth->orientation[orth->numorient-1];

      /* if pos and next_pos are the same take the next point before
         next_pos */
      if (pos->x == next_pos->x && pos->y == next_pos->y)
        {
          next_pos = &orth->points[p_index-2];
          orient = (pos->y == next_pos->y) ? HORIZONTAL : VERTICAL;
        }

      ref->ep_desc_width = dia_font_string_width (ref->end_point_desc,
                                                  ref->normal_font,
                                                  ref->normal_font_height);

      update_desc_data (&ref->ep_desc_pos, &ref->ep_desc_text_align,
                        pos, next_pos, orient, ref->line_width,
                        ref->normal_font_height);

      get_desc_bbox (&rect, ref->end_point_desc, ref->ep_desc_width,
                     &ref->ep_desc_pos, ref->ep_desc_text_align,
                     ref->normal_font, ref->normal_font_height);
      rectangle_union (&orth->object.bounding_box, &rect);
    }
  else
    {
      ref->ep_desc_width = 0.0;
    }
  /* finally the end arrow */
  arrow_bbox (&ref->end_arrow, ref->line_width,
              &orth->points[orth->numpoints - 1], &orth->points[orth->numpoints - 2], &rect);
  rectangle_union (&orth->object.bounding_box, &rect);
}


static void
update_desc_data (Point        *desc_pos,
                  DiaAlignment *desc_align,
                  Point        *end_point,
                  Point        *nearest_point,
                  Orientation   orientation,
                  double        line_width,
                  double        font_height)
{
  double dist = font_height/4.0 + line_width/2.0;

  *desc_pos = *end_point;

  switch (orientation) {
    case HORIZONTAL:
      /* for horizontal lines the label is above the line */
      desc_pos->y -= dist;
      if (end_point->x <= nearest_point->x) {
        desc_pos->x += dist;
        *desc_align = DIA_ALIGN_LEFT;
      } else {
        desc_pos->x -= dist;
        *desc_align = DIA_ALIGN_RIGHT;
      }
      break;

    case VERTICAL:
      /* for vertical lines the label is at the right side of it */
      desc_pos->x += dist;
      *desc_align = DIA_ALIGN_LEFT;
      if (end_point->y <= nearest_point->y)
        desc_pos->y += font_height;
      else
        desc_pos->y -= dist;
      break;

    default:
      g_return_if_reached ();
  }
}


static void
get_desc_bbox (DiaRectangle *r,
               char         *string,
               double        string_width,
               Point        *pos,
               DiaAlignment  align,
               DiaFont      *font,
               double        font_height)
{
  double width;

  g_assert (r != NULL);
  g_assert (string != NULL);
  g_assert (pos != NULL);

  width = string_width;

  g_return_if_fail (align == DIA_ALIGN_LEFT || align == DIA_ALIGN_RIGHT);

  if (align == DIA_ALIGN_LEFT) {
      r->left = pos->x;
      r->right = r->left + width;
  } else {
    r->right = pos->x;
    r->left = r->right - width;
  }

  r->top = pos->y;
  r->top -= dia_font_ascent (string, font, font_height);
  r->bottom = r->top + font_height;
}


static DiaMenu *
reference_object_menu (TableReference *tbl, Point *clicked)
{
  OrthConn *orth;

  orth = &tbl->orth;
  /* Set entries sensitive/selected etc here */
  reference_menu_items[0].active = orthconn_can_add_segment (orth, clicked);
  reference_menu_items[1].active = orthconn_can_delete_segment (orth, clicked);
  orthconn_update_object_menu (orth, clicked, &reference_menu_items[2]);

  return &reference_menu;
}


static DiaObjectChange *
reference_add_segment_cb (DiaObject *obj, Point *clicked, gpointer data)
{
  DiaObjectChange *change;

  change = orthconn_add_segment ((OrthConn *)obj, clicked);
  reference_update_data ((TableReference *) obj);

  return change;
}


static DiaObjectChange *
reference_del_segment_cb (DiaObject *obj, Point *clicked, gpointer data)
{
  DiaObjectChange *change;

  change = orthconn_delete_segment ((OrthConn *) obj, clicked);
  reference_update_data ((TableReference *) obj);

  return change;
}
