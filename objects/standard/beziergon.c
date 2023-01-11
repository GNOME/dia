/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1999 Alexander Larsson
 *
 * beziergon.c - a beziergon shape
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

#include "config.h"

#include <glib/gi18n-lib.h>

#include <math.h>

#include "object.h"
#include "beziershape.h"
#include "connectionpoint.h"
#include "diarenderer.h"
#include "diainteractiverenderer.h"
#include "attributes.h"
#include "diamenu.h"
#include "properties.h"
#include "create.h"
#include "pattern.h"

#define DEFAULT_WIDTH 0.15

/*!
 * \brief Standard - Beziergon: closed shape with bezier
 * \extends _BezierShape
 * \ingroup StandardObjects
 */
typedef struct _Beziergon {
  BezierShape bezier;

  Color line_color;
  DiaLineStyle line_style;
  DiaLineJoin line_join;
  Color inner_color;
  gboolean show_background;
  double dashlength;
  double line_width;
  DiaPattern *pattern;
} Beziergon;

typedef struct _BeziergonProperties BeziergonProperties;

static struct _BeziergonProperties {
  gboolean show_background;
} default_properties = { TRUE };


static DiaObjectChange* beziergon_move_handle       (Beziergon        *beziergon,
                                                     Handle           *handle,
                                                     Point            *to,
                                                     ConnectionPoint  *cp,
                                                     HandleMoveReason  reason,
                                                     ModifierKeys      modifiers);
static DiaObjectChange* beziergon_move              (Beziergon        *beziergon,
                                                     Point            *to);
static void beziergon_select(Beziergon *beziergon, Point *clicked_point,
			     DiaRenderer *interactive_renderer);
static void beziergon_draw(Beziergon *beziergon, DiaRenderer *renderer);
static DiaObject *beziergon_create(Point *startpoint,
				void *user_data,
				Handle **handle1,
				Handle **handle2);
static real beziergon_distance_from(Beziergon *beziergon, Point *point);
static void beziergon_update_data(Beziergon *beziergon);
static void beziergon_destroy(Beziergon *beziergon);
static DiaObject *beziergon_copy(Beziergon *beziergon);

static void beziergon_set_props(Beziergon *beziergon, GPtrArray *props);

static void beziergon_save(Beziergon *beziergon, ObjectNode obj_node,
			   DiaContext *ctx);
static DiaObject *beziergon_load(ObjectNode obj_node, int version, DiaContext *ctx);
static DiaMenu *beziergon_get_object_menu(Beziergon *beziergon,
					  Point *clickedpoint);
static gboolean beziergon_transform(Beziergon *beziergon, const DiaMatrix *m);

static ObjectTypeOps beziergon_type_ops =
{
  (CreateFunc)beziergon_create,   /* create */
  (LoadFunc)  beziergon_load,     /* load */
  (SaveFunc)  beziergon_save,      /* save */
  (GetDefaultsFunc)   NULL,
  (ApplyDefaultsFunc) NULL
};

static PropDescription beziergon_props[] = {
  BEZSHAPE_COMMON_PROPERTIES,
  PROP_STD_LINE_WIDTH,
  PROP_STD_LINE_COLOUR,
  PROP_STD_LINE_STYLE,
  PROP_STD_LINE_JOIN_OPTIONAL,
  PROP_STD_FILL_COLOUR,
  PROP_STD_SHOW_BACKGROUND,
  PROP_STD_PATTERN,
  PROP_DESC_END
};

static PropOffset beziergon_offsets[] = {
  BEZSHAPE_COMMON_PROPERTIES_OFFSETS,
  { PROP_STDNAME_LINE_WIDTH, PROP_STDTYPE_LINE_WIDTH, offsetof(Beziergon, line_width) },
  { "line_colour", PROP_TYPE_COLOUR, offsetof(Beziergon, line_color) },
  { "line_style", PROP_TYPE_LINESTYLE,
    offsetof(Beziergon, line_style), offsetof(Beziergon, dashlength) },
  { "line_join", PROP_TYPE_ENUM, offsetof(Beziergon, line_join) },
  { "fill_colour", PROP_TYPE_COLOUR, offsetof(Beziergon, inner_color) },
  { "show_background", PROP_TYPE_BOOL, offsetof(Beziergon, show_background) },
  { "pattern", PROP_TYPE_PATTERN, offsetof(Beziergon, pattern) },
  { NULL, 0, 0 }
};

static DiaObjectType beziergon_type =
{
  "Standard - Beziergon",   /* name */
  0,                         /* version */
  (const char **) "res:/org/gnome/Dia/objects/standard/beziergon.png",
  &beziergon_type_ops,      /* ops */
  NULL,                     /* pixmap_file */
  0,                        /* default_user_data */
  beziergon_props,
  beziergon_offsets
};

DiaObjectType *_beziergon_type = (DiaObjectType *) &beziergon_type;


static ObjectOps beziergon_ops = {
  (DestroyFunc)         beziergon_destroy,
  (DrawFunc)            beziergon_draw,
  (DistanceFunc)        beziergon_distance_from,
  (SelectFunc)          beziergon_select,
  (CopyFunc)            beziergon_copy,
  (MoveFunc)            beziergon_move,
  (MoveHandleFunc)      beziergon_move_handle,
  (GetPropertiesFunc)   object_create_props_dialog,
  (ApplyPropertiesDialogFunc) object_apply_props_from_dialog,
  (ObjectMenuFunc)      beziergon_get_object_menu,
  (DescribePropsFunc)   object_describe_props,
  (GetPropsFunc)        object_get_props,
  (SetPropsFunc)        beziergon_set_props,
  (TextEditFunc) 0,
  (ApplyPropertiesListFunc) object_apply_props,
  (TransformFunc)       beziergon_transform,
};

static void
beziergon_set_props(Beziergon *beziergon, GPtrArray *props)
{
  object_set_props_from_offsets(&beziergon->bezier.object, beziergon_offsets,
				props);
  beziergon_update_data(beziergon);
}

static real
beziergon_distance_from(Beziergon *beziergon, Point *point)
{
  return beziershape_distance_from(&beziergon->bezier, point,
				   beziergon->line_width);
}

static int
beziergon_closest_segment(Beziergon *beziergon, Point *point)
{
  return beziercommon_closest_segment(&beziergon->bezier.bezier, point,
				      beziergon->line_width);
}

static void
beziergon_select(Beziergon *beziergon, Point *clicked_point,
		  DiaRenderer *interactive_renderer)
{
  beziershape_update_data(&beziergon->bezier);
}


static DiaObjectChange *
beziergon_move_handle (Beziergon        *beziergon,
                       Handle           *handle,
                       Point            *to,
                       ConnectionPoint  *cp,
                       HandleMoveReason  reason,
                       ModifierKeys      modifiers)
{
  g_return_val_if_fail (beziergon != NULL, NULL);
  g_return_val_if_fail (handle != NULL, NULL);
  g_return_val_if_fail (to != NULL, NULL);

  beziershape_move_handle (&beziergon->bezier, handle, to, cp, reason, modifiers);
  beziergon_update_data (beziergon);

  return NULL;
}


static DiaObjectChange *
beziergon_move (Beziergon *beziergon, Point *to)
{
  beziershape_move (&beziergon->bezier, to);
  beziergon_update_data (beziergon);

  return NULL;
}


static void
beziergon_draw (Beziergon *beziergon, DiaRenderer *renderer)
{
  BezierShape *bez = &beziergon->bezier;
  BezPoint *points;
  int n;

  points = &bez->bezier.points[0];
  n = bez->bezier.num_points;

  dia_renderer_set_linewidth (renderer, beziergon->line_width);
  dia_renderer_set_linestyle (renderer, beziergon->line_style, beziergon->dashlength);
  dia_renderer_set_linejoin (renderer, beziergon->line_join);
  dia_renderer_set_linecaps (renderer, DIA_LINE_CAPS_BUTT);

  if (beziergon->show_background) {
    Color fill = beziergon->inner_color;
    if (beziergon->pattern) {
      dia_pattern_get_fallback_color (beziergon->pattern, &fill);
      if (dia_renderer_is_capable_of (renderer, RENDER_PATTERN)) {
        dia_renderer_set_pattern (renderer, beziergon->pattern);
      }
    }
    dia_renderer_draw_beziergon (renderer, points, n, &fill, &beziergon->line_color);
    if (dia_renderer_is_capable_of (renderer, RENDER_PATTERN)) {
      dia_renderer_set_pattern (renderer, NULL);
    }
  } else { /* still to be closed */
    dia_renderer_draw_beziergon (renderer, points, n, NULL, &beziergon->line_color);
  }
  /* these lines should only be displayed when object is selected.
   * Unfortunately the draw function is not aware of the selected
   * state.  This is a compromise until I fix this properly. */
  if (DIA_IS_INTERACTIVE_RENDERER (renderer) &&
      dia_object_is_selected (DIA_OBJECT (beziergon))) {
    bezier_draw_control_lines (beziergon->bezier.bezier.num_points, beziergon->bezier.bezier.points, renderer);
  }
}

static DiaObject *
beziergon_create(Point *startpoint,
		  void *user_data,
		  Handle **handle1,
		  Handle **handle2)
{
  Beziergon *beziergon;
  BezierShape *bez;
  DiaObject *obj;
  Point defaultx = { 1.0, 0.0 };
  Point defaulty = { 0.0, 1.0 };

  beziergon = g_new0(Beziergon, 1);
  beziergon->bezier.object.enclosing_box = g_new0 (DiaRectangle, 1);
  bez = &beziergon->bezier;
  obj = &bez->object;

  obj->type = &beziergon_type;
  obj->ops = &beziergon_ops;

  if (user_data == NULL) {
    beziershape_init(bez, 3);

    bez->bezier.points[0].p1 = *startpoint;
    bez->bezier.points[0].p3 = *startpoint;
    bez->bezier.points[2].p3 = *startpoint;

    bez->bezier.points[1].p1 = *startpoint;
    point_add(&bez->bezier.points[1].p1, &defaultx);
    bez->bezier.points[2].p2 = *startpoint;
    point_sub(&bez->bezier.points[2].p2, &defaultx);

    bez->bezier.points[1].p3 = *startpoint;
    point_add(&bez->bezier.points[1].p3, &defaulty);
    bez->bezier.points[1].p2 = bez->bezier.points[1].p3;
    point_add(&bez->bezier.points[1].p2, &defaultx);
    bez->bezier.points[2].p1 = bez->bezier.points[1].p3;
    point_sub(&bez->bezier.points[2].p1, &defaultx);
  } else {
    BezierCreateData *bcd = (BezierCreateData*)user_data;

    beziershape_init(bez, bcd->num_points);
    beziercommon_set_points (&bez->bezier, bcd->num_points, bcd->points);
  }
  beziergon->line_width =  attributes_get_default_linewidth();
  beziergon->line_color = attributes_get_foreground();
  beziergon->inner_color = attributes_get_background();
  attributes_get_default_line_style (&beziergon->line_style,
                                     &beziergon->dashlength);
  beziergon->line_join = DIA_LINE_JOIN_MITER;
  beziergon->show_background = default_properties.show_background;

  beziergon_update_data(beziergon);

  *handle1 = bez->object.handles[5];
  *handle2 = bez->object.handles[2];
  return &beziergon->bezier.object;
}


static void
beziergon_destroy(Beziergon *beziergon)
{
  g_clear_object (&beziergon->pattern);
  g_clear_pointer (&beziergon->bezier.object.enclosing_box, g_free);
  beziershape_destroy (&beziergon->bezier);
}


static DiaObject *
beziergon_copy(Beziergon *beziergon)
{
  Beziergon *newbeziergon;
  BezierShape *bezier, *newbezier;

  bezier = &beziergon->bezier;

  newbeziergon = g_new0 (Beziergon, 1);
  newbeziergon->bezier.object.enclosing_box = g_new0 (DiaRectangle, 1);
  newbezier = &newbeziergon->bezier;

  beziershape_copy(bezier, newbezier);

  newbeziergon->line_color = beziergon->line_color;
  newbeziergon->line_width = beziergon->line_width;
  newbeziergon->line_style = beziergon->line_style;
  newbeziergon->line_join = beziergon->line_join;
  newbeziergon->dashlength = beziergon->dashlength;
  newbeziergon->inner_color = beziergon->inner_color;
  newbeziergon->show_background = beziergon->show_background;
  if (beziergon->pattern)
    newbeziergon->pattern = g_object_ref (beziergon->pattern);

  return &newbeziergon->bezier.object;
}

static void
beziergon_update_data(Beziergon *beziergon)
{
  BezierShape *bez = &beziergon->bezier;
  DiaObject *obj = &bez->object;
  ElementBBExtras *extra = &bez->extra_spacing;

  beziershape_update_data(bez);

  extra->border_trans = beziergon->line_width / 2.0;
  beziershape_update_boundingbox(bez);

  /* update the enclosing box using the control points */
  {
    int i, num_points = bez->bezier.num_points;
    g_assert (obj->enclosing_box != NULL);
    *obj->enclosing_box = obj->bounding_box;
    for (i = 0; i < num_points; ++i) {
      if (bez->bezier.points[i].type != BEZ_CURVE_TO)
        continue;
      rectangle_add_point(obj->enclosing_box, &bez->bezier.points[i].p1);
      rectangle_add_point(obj->enclosing_box, &bez->bezier.points[i].p2);
    }
  }
  obj->position = bez->bezier.points[0].p1;
}

static void
beziergon_save(Beziergon *beziergon, ObjectNode obj_node,
	       DiaContext *ctx)
{
  beziershape_save(&beziergon->bezier, obj_node, ctx);

  if (!color_equals(&beziergon->line_color, &color_black))
    data_add_color(new_attribute(obj_node, "line_color"),
		   &beziergon->line_color, ctx);

  if (beziergon->line_width != 0.1)
    data_add_real(new_attribute(obj_node, PROP_STDNAME_LINE_WIDTH),
		  beziergon->line_width, ctx);

  if (!color_equals(&beziergon->inner_color, &color_white))
    data_add_color(new_attribute(obj_node, "inner_color"),
		   &beziergon->inner_color, ctx);

  data_add_boolean(new_attribute(obj_node, "show_background"),
		   beziergon->show_background, ctx);

  if (beziergon->line_style != DIA_LINE_STYLE_SOLID)
    data_add_enum(new_attribute(obj_node, "line_style"),
		  beziergon->line_style, ctx);

  if (beziergon->line_style != DIA_LINE_STYLE_SOLID &&
      beziergon->dashlength != DEFAULT_LINESTYLE_DASHLEN)
    data_add_real(new_attribute(obj_node, "dashlength"),
		  beziergon->dashlength, ctx);

  if (beziergon->line_join != DIA_LINE_JOIN_MITER) {
    data_add_enum (new_attribute (obj_node, "line_join"),
                   beziergon->line_join,
                   ctx);
  }

  if (beziergon->pattern)
    data_add_pattern(new_attribute(obj_node, "pattern"),
		     beziergon->pattern, ctx);
}

static DiaObject *
beziergon_load(ObjectNode obj_node, int version, DiaContext *ctx)
{
  Beziergon *beziergon;
  BezierShape *bez;
  DiaObject *obj;
  AttributeNode attr;

  beziergon = g_new0 (Beziergon, 1);
  beziergon->bezier.object.enclosing_box = g_new0 (DiaRectangle, 1);

  bez = &beziergon->bezier;
  obj = &bez->object;

  obj->type = &beziergon_type;
  obj->ops = &beziergon_ops;

  beziershape_load(bez, obj_node, ctx);

  beziergon->line_color = color_black;
  attr = object_find_attribute(obj_node, "line_color");
  if (attr != NULL)
    data_color(attribute_first_data(attr), &beziergon->line_color, ctx);

  beziergon->line_width = 0.1;
  attr = object_find_attribute(obj_node, PROP_STDNAME_LINE_WIDTH);
  if (attr != NULL)
    beziergon->line_width = data_real(attribute_first_data(attr), ctx);

  beziergon->inner_color = color_white;
  attr = object_find_attribute(obj_node, "inner_color");
  if (attr != NULL)
    data_color(attribute_first_data(attr), &beziergon->inner_color, ctx);

  beziergon->show_background = TRUE;
  attr = object_find_attribute(obj_node, "show_background");
  if (attr != NULL)
    beziergon->show_background = data_boolean(attribute_first_data(attr), ctx);

  beziergon->line_style = DIA_LINE_STYLE_SOLID;
  attr = object_find_attribute(obj_node, "line_style");
  if (attr != NULL)
    beziergon->line_style = data_enum(attribute_first_data(attr), ctx);

  beziergon->line_join = DIA_LINE_JOIN_MITER;
  attr = object_find_attribute (obj_node, "line_join");
  if (attr != NULL) {
    beziergon->line_join = data_enum (attribute_first_data (attr), ctx);
  }

  beziergon->dashlength = DEFAULT_LINESTYLE_DASHLEN;
  attr = object_find_attribute(obj_node, "dashlength");
  if (attr != NULL)
    beziergon->dashlength = data_real(attribute_first_data(attr), ctx);

  attr = object_find_attribute(obj_node, "pattern");
  if (attr != NULL)
    beziergon->pattern = data_pattern(attribute_first_data(attr), ctx);

  beziergon_update_data(beziergon);

  return &beziergon->bezier.object;
}


static DiaObjectChange *
beziergon_add_segment_callback (DiaObject *obj, Point *clicked, gpointer data)
{
  Beziergon *bezier = (Beziergon*) obj;
  int segment;
  DiaObjectChange *change;

  segment = beziergon_closest_segment(bezier, clicked);
  change = beziershape_add_segment(&bezier->bezier, segment, clicked);

  beziergon_update_data(bezier);
  return change;
}


static DiaObjectChange *
beziergon_delete_segment_callback (DiaObject *obj, Point *clicked, gpointer data)
{
  int seg_nr;
  Beziergon *bezier = (Beziergon*) obj;
  DiaObjectChange *change;

  seg_nr = beziergon_closest_segment(bezier, clicked);
  change = beziershape_remove_segment(&bezier->bezier, seg_nr+1);

  beziergon_update_data(bezier);
  return change;
}


static DiaObjectChange *
beziergon_set_corner_type_callback (DiaObject *obj, Point *clicked, gpointer data)
{
  Handle *closest;
  Beziergon *beziergon = (Beziergon *) obj;
  DiaObjectChange *change;

  closest = beziershape_closest_major_handle(&beziergon->bezier, clicked);
  change = beziershape_set_corner_type(&beziergon->bezier, closest,
				       GPOINTER_TO_INT(data));

  beziergon_update_data(beziergon);
  return change;
}

static DiaMenuItem beziergon_menu_items[] = {
  { N_("Add Segment"), beziergon_add_segment_callback, NULL, 1 },
  { N_("Delete Segment"), beziergon_delete_segment_callback, NULL, 1 },
  { NULL, NULL, NULL, 1 },
  { N_("Symmetric control"), beziergon_set_corner_type_callback,
    GINT_TO_POINTER(BEZ_CORNER_SYMMETRIC), 1 },
  { N_("Smooth control"), beziergon_set_corner_type_callback,
    GINT_TO_POINTER(BEZ_CORNER_SMOOTH), 1 },
  { N_("Cusp control"), beziergon_set_corner_type_callback,
    GINT_TO_POINTER(BEZ_CORNER_CUSP), 1 }
};

static DiaMenu beziergon_menu = {
  "Beziergon",
  sizeof(beziergon_menu_items)/sizeof(DiaMenuItem),
  beziergon_menu_items,
  NULL
};

static DiaMenu *
beziergon_get_object_menu(Beziergon *beziergon, Point *clickedpoint)
{
  /* Set entries sensitive/selected etc here */
  beziergon_menu_items[0].active = 1;
  beziergon_menu_items[1].active = beziergon->bezier.bezier.num_points > 3;
  return &beziergon_menu;
}

static gboolean
beziergon_transform(Beziergon *beziergon, const DiaMatrix *m)
{
  int i;

  g_return_val_if_fail (m != NULL, FALSE);

  for (i = 0; i < beziergon->bezier.bezier.num_points; i++)
    transform_bezpoint (&beziergon->bezier.bezier.points[i], m);

  beziergon_update_data(beziergon);
  return TRUE;
}
