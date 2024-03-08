/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * outline.c -- cairo based rotateable outline
 * Copyright (C) 2007, Hans Breuer, <Hans@Breuer.Org>
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

/*!
 * \file outline.c - finally rotated text, somwhat
 *
 * A prototype for text effects with the standard renderer interface.
 * It should be possible to do all those fancy text effects like outline, rotate, ...
 * by converting text to pathes before rendering.
 */

#include "config.h"

#include <glib/gi18n-lib.h>

#include "object.h"
#include "diarenderer.h"
#include "attributes.h"
#include "properties.h"
#include "boundingbox.h"
#include "standard-path.h"


#include <cairo.h>
#include <cairo-svg.h>

#define NUM_HANDLES 2

/*!
 * \brief Standard - Outline
 * \extends _DiaObject
 * \ingroup StandardObjects
 */
typedef struct _Outline {
  DiaObject object; /*!< inheritance */

  char *name;
  real rotation;

  DiaFont *font;
  real font_height;

  Color line_color;
  Color fill_color;
  gboolean show_background;
  real line_width;

  Handle handles[NUM_HANDLES];
  /* calculated data */
  Point ink_rect[4];
  cairo_path_t *path;
  struct _Matrix
  {
    real xx;
    real xy;
    real yx;
    real yy;
  } mat;
} Outline;

/* Type definition */
static DiaObject *outline_create (Point *startpoint,
				  void *user_data,
				  Handle **handle1,
				  Handle **handle2);
static DiaObject *
outline_load(ObjectNode obj_node, int version,DiaContext *ctx);

static ObjectTypeOps outline_type_ops =
{
  (CreateFunc)outline_create,
  (LoadFunc)  outline_load,
  (SaveFunc)  object_save_using_properties, /* outline_save */
  (GetDefaultsFunc)   NULL,
  (ApplyDefaultsFunc) NULL
};

static PropNumData _rotation_range = { 0.0f, 360.0f, 1.0f };
static PropDescription outline_props[] = {
  OBJECT_COMMON_PROPERTIES,
  { "name", PROP_TYPE_STRING,PROP_FLAG_VISIBLE|PROP_FLAG_DONT_MERGE,
    N_("Text content"),NULL },
  { "rotation", PROP_TYPE_REAL,PROP_FLAG_VISIBLE,
    N_("Rotation"), N_("Angle to rotate the outline"), &_rotation_range},
  /* the default PROP_STD_TEXT_FONT has PROP_FLAG_DONT_SAVE, we need saving */
  PROP_STD_TEXT_FONT_OPTIONS(PROP_FLAG_VISIBLE),
  PROP_STD_TEXT_HEIGHT_OPTIONS(PROP_FLAG_VISIBLE),
  PROP_STD_LINE_WIDTH,
  PROP_STD_LINE_COLOUR,
  PROP_STD_FILL_COLOUR,
  PROP_STD_SHOW_BACKGROUND,
  PROP_DESC_END
};

static PropOffset outline_offsets[] = {
  OBJECT_COMMON_PROPERTIES_OFFSETS,
  { "name", PROP_TYPE_STRING, offsetof(Outline, name) },
  { "rotation", PROP_TYPE_REAL, offsetof(Outline, rotation) },
  { "text_font",PROP_TYPE_FONT,offsetof(Outline,font) },
  { PROP_STDNAME_TEXT_HEIGHT,PROP_STDTYPE_TEXT_HEIGHT,offsetof(Outline, font_height) },
  { PROP_STDNAME_LINE_WIDTH, PROP_STDTYPE_LINE_WIDTH, offsetof(Outline, line_width) },
  { "line_colour", PROP_TYPE_COLOUR, offsetof(Outline, line_color) },
  { "fill_colour", PROP_TYPE_COLOUR, offsetof(Outline, fill_color) },
  { "show_background", PROP_TYPE_BOOL, offsetof(Outline, show_background) },
  { NULL, 0, 0 }
};

static DiaObjectType outline_type =
{
  "Standard - Outline",   /* name */
  0,                      /* version */
  (const char **) "res:/org/gnome/Dia/objects/standard/outline.png",
  &outline_type_ops,      /* ops */
  NULL,                   /* pixmap_file */
  0,                      /* default_user_data */
  outline_props,
  outline_offsets
};

/* make accessible from the outside for type registration */
DiaObjectType *_outline_type = (DiaObjectType *) &outline_type;

/* Class definition */
static DiaObjectChange* outline_move_handle (Outline *outline,
                                          Handle *handle,
					  Point *to, ConnectionPoint *cp,
					  HandleMoveReason reason, ModifierKeys modifiers);
static DiaObjectChange* outline_move (Outline *outline, Point *to);
static void outline_select(Outline *outline, Point *clicked_point,
			   DiaRenderer *interactive_renderer);
static void outline_draw(Outline *outline, DiaRenderer *renderer);
static real outline_distance_from (Outline *outline, Point *point);
static void outline_update_data (Outline *outline);
static void outline_destroy (Outline *outline);
static DiaObject *outline_copy (Outline *outline);
static DiaMenu *outline_get_object_menu(Outline *outline,
					Point *clickedpoint);
static void outline_set_props(Outline *outline, GPtrArray *props);

static ObjectOps outline_ops = {
  (DestroyFunc)         outline_destroy,
  (DrawFunc)            outline_draw,
  (DistanceFunc)        outline_distance_from,
  (SelectFunc)          outline_select,
  (CopyFunc)            outline_copy,
  (MoveFunc)            outline_move,
  (MoveHandleFunc)      outline_move_handle,
  (GetPropertiesFunc)   object_create_props_dialog,
  (ApplyPropertiesDialogFunc) object_apply_props_from_dialog,
  (ObjectMenuFunc)      outline_get_object_menu,
  (DescribePropsFunc)   object_describe_props,
  (GetPropsFunc)        object_get_props,
  (SetPropsFunc)        outline_set_props,
  (TextEditFunc) 0,
  (ApplyPropertiesListFunc) object_apply_props,
};

/* Type implementation */
static void
outline_init_handles (Outline *outline)
{
  DiaObject *obj = &outline->object;
  int i;

  for (i = 0; i < NUM_HANDLES; ++i) {
    obj->handles[i] = &outline->handles[i];
    obj->handles[i]->type = HANDLE_MAJOR_CONTROL;
    obj->handles[i]->connect_type = HANDLE_CONNECTABLE;
    obj->handles[i]->connected_to = NULL;
  }
}

/*! Factory function - create default object */
static DiaObject *
outline_create (Point *startpoint,
		void *user_data,
		Handle **handle1,
		Handle **handle2)
{
  Outline *outline;
  DiaObject *obj;

  outline = g_new0 (Outline,1);
  obj = &outline->object;
  obj->type = &outline_type;
  obj->ops = &outline_ops;

  object_init (obj, NUM_HANDLES, 0);
  obj->position = *startpoint;

  outline_init_handles (outline);

  attributes_get_default_font (&outline->font, &outline->font_height);

  outline->line_width = 0; /* Not: attributes_get_default_linewidth(); it looks ugly */
  outline->line_color = attributes_get_foreground();
  outline->fill_color = attributes_get_background();
  outline->show_background = FALSE;
  outline->name = g_strdup ("?");
  outline->rotation = 0;

  *handle1 = outline->object.handles[0];
  *handle2 = outline->object.handles[1];

  outline_update_data (outline);

  return obj;
}
static DiaObject *
outline_load(ObjectNode obj_node, int version,DiaContext *ctx)
{
  return object_load_using_properties(&outline_type,
                                      obj_node,version,ctx);
}

/* empty write function */
static cairo_status_t
write_nul (void                *closure,
           const unsigned char *data,
	   unsigned int	        length)
{
  return CAIRO_STATUS_SUCCESS;
}
/*! Not in the object interface but still required */
static void
outline_update_handles(Outline *outline)
{
  DiaObject *obj = &outline->object;

  g_return_if_fail (obj->handles != NULL);
  obj->handles[0]->id = HANDLE_RESIZE_NW;
  obj->handles[0]->pos = outline->ink_rect[0];

  obj->handles[1]->id = HANDLE_RESIZE_SE;
  obj->handles[1]->pos = outline->ink_rect[2];
}
/*!
 * \brief Object update function called after property change
 *
 * Not in the object interface but very important anyway.
 * Used to recalculate the object data after a change
 * \protected \memberof Outline
 */
static void
outline_update_data (Outline *outline)
{
  DiaObject *obj = &outline->object;
  /* calculate the ink_rect from two points and the given rotation */
  cairo_t *cr;
  cairo_surface_t *surface;
  cairo_text_extents_t extents;
  real x, y;
  DiaFontStyle style;

  if (outline->path)
    cairo_path_destroy (outline->path);
  outline->path = NULL;
  /* surface will not be used to render anything, it is just to create the cairo context */
  surface = cairo_svg_surface_create_for_stream (write_nul, NULL, 100, 100);
  cr = cairo_create (surface);
  cairo_surface_destroy (surface); /* in fact: unref() */
  style = dia_font_get_style (outline->font);
  /* not exact matching but almost the best we can do with the toy api */
  cairo_select_font_face (cr, dia_font_get_family (outline->font),
                          DIA_FONT_STYLE_GET_SLANT (style) == DIA_FONT_NORMAL ? CAIRO_FONT_SLANT_NORMAL : CAIRO_FONT_SLANT_ITALIC,
                          DIA_FONT_STYLE_GET_WEIGHT (style) < DIA_FONT_MEDIUM ? CAIRO_FONT_WEIGHT_NORMAL : CAIRO_FONT_WEIGHT_BOLD);
  cairo_set_font_size (cr, outline->font_height);
  cairo_text_extents (cr, outline->name, &extents);

  /* unfortunately this has no effect on the returned path? See below. */
  cairo_rotate (cr, outline->rotation/(2*G_PI));

  outline->mat.xx =  cos(G_PI*outline->rotation/180);
  outline->mat.xy =  sin(G_PI*outline->rotation/180);
  outline->mat.yx = -sin(G_PI*outline->rotation/180);
  outline->mat.yy =  cos(G_PI*outline->rotation/180);

  /* fix point */
  outline->ink_rect[0].x = x = obj->position.x;
  outline->ink_rect[0].y = y = obj->position.y;
  /* handle rotation */
  outline->ink_rect[1].x = x + extents.width * outline->mat.xx;
  outline->ink_rect[1].y = y + extents.width * outline->mat.yx;
  outline->ink_rect[2].x = x + extents.width * outline->mat.xx + extents.height * outline->mat.xy;
  outline->ink_rect[2].y = y + extents.width * outline->mat.yx + extents.height * outline->mat.yy;
  outline->ink_rect[3].x = x + extents.height * outline->mat.xy;
  outline->ink_rect[3].y = y + extents.height * outline->mat.yy;
  /* x_advance? */
  /* calculate bounding box */
  {
    PolyBBExtras bbex = {0, 0, outline->line_width/2, 0, 0 };
    polyline_bbox (&outline->ink_rect[0], 4, &bbex, TRUE, &obj->bounding_box);
  }

  outline_update_handles (outline),

  cairo_move_to (cr, -extents.x_bearing, -extents.y_bearing);

#if 0
  /* reset the matrix to not yet change the outline_draw method */
  outline->mat.xx =  1.0;
  outline->mat.xy =  0.0;
  outline->mat.yx =  0.0;
  outline->mat.yy =  1.0;
#endif

  cairo_text_path (cr, outline->name);
  /* reset the rotation to not have the path rotated back and forth: no effect */
  cairo_rotate (cr, 0.0);
  outline->path = cairo_copy_path (cr);
  /* the cairo context is only used in this fuinction */
  cairo_destroy (cr);
}
/*!
 * \brief Object drawing to the given renderer
 *
 * \memberof Outline
 */
static void
outline_draw (Outline *outline, DiaRenderer *renderer)
{
  DiaObject *obj = &outline->object;
  int i, n = 0, total;
  BezPoint *pts;
  real x, y;
  Point ps = {0, 0}; /* silence gcc */

  if (!outline->path)
    return;

  dia_renderer_set_linewidth (renderer, outline->line_width);
  dia_renderer_set_linestyle (renderer, DIA_LINE_STYLE_SOLID, 0.0);
  dia_renderer_set_linejoin (renderer, DIA_LINE_JOIN_MITER);
  dia_renderer_set_linecaps (renderer, DIA_LINE_CAPS_ROUND);

  /* the cairo text path is position independent, Dia's bezier are not */
  x = obj->position.x;
  y = obj->position.y;

  /* count Dia BezPoints required */
  total = 0;
  for (i=0; i < outline->path->num_data; i += outline->path->data[i].header.length) {
    ++total;
  }
  /* if there is nothing to draw exit early to avoid
   * Undefined allocation of 0 bytes (CERT MEM04-C; CWE-131)
   */
  if (total < 2)
    return;

  pts = g_newa (BezPoint, total);
  for (i=0; i < outline->path->num_data; i += outline->path->data[i].header.length) {
    cairo_path_data_t *data = &outline->path->data[i];
    if (CAIRO_PATH_MOVE_TO == data->header.type) {
      /* Dia can't handle moveto except at start: corrected below */
      pts[n].type = BEZ_MOVE_TO;
      ps.x = pts[n].p1.x = data[1].point.x * outline->mat.xx + data[1].point.y * outline->mat.xy + x;
      ps.y = pts[n].p1.y = data[1].point.x * outline->mat.yx + data[1].point.y * outline->mat.yy + y;
      ++n;
    } else if (CAIRO_PATH_LINE_TO == data->header.type) {
      pts[n].type = BEZ_LINE_TO;
      pts[n].p1.x = data[1].point.x * outline->mat.xx + data[1].point.y *  outline->mat.xy + x;
      pts[n].p1.y = data[1].point.x * outline->mat.yx + data[1].point.y *  outline->mat.yy + y;
      ++n;
    } else if (CAIRO_PATH_CURVE_TO == data->header.type) {
      pts[n].type = BEZ_CURVE_TO;
      pts[n].p1.x = data[1].point.x * outline->mat.xx + data[1].point.y *  outline->mat.xy + x;
      pts[n].p1.y = data[1].point.x * outline->mat.yx + data[1].point.y *  outline->mat.yy + y;
      pts[n].p2.x = data[2].point.x * outline->mat.xx + data[2].point.y *  outline->mat.xy + x;
      pts[n].p2.y = data[2].point.x * outline->mat.yx + data[2].point.y *  outline->mat.yy + y;
      pts[n].p3.x = data[3].point.x * outline->mat.xx + data[3].point.y *  outline->mat.xy + x;
      pts[n].p3.y = data[3].point.x * outline->mat.yx + data[3].point.y *  outline->mat.yy + y;
      ++n;
    } else if (CAIRO_PATH_CLOSE_PATH == data->header.type) {
      pts[n].type = BEZ_LINE_TO;
      pts[n].p1.x = ps.x;
      pts[n].p1.y = ps.y;
      ++n;
    }
  }
  if (pts[n-1].type == BEZ_MOVE_TO)
    --total; /* remove a potential last move which would otherwise be rendered as a dot */

  if (dia_renderer_is_capable_of (renderer, RENDER_HOLES)) {
    dia_renderer_draw_beziergon (renderer,
                                 pts,
                                 total,
                                 outline->show_background ? &outline->fill_color : NULL,
                                 &outline->line_color);
    return; /* that was easy ;) */
  }
  /* otherwise split the path data into piece which can be handled by Dia's standard bezier rendering */
  if (outline->show_background) {
    dia_renderer_bezier_fill (renderer, pts, total, &outline->fill_color);
  }
  dia_renderer_bezier_stroke (renderer, pts, total, &outline->line_color);
}
/*!
 * \brief Optionally deliver an object specific menu
 *
 * returning NULL used to crash Dia, the method itself needed to be NULL if there is nothing to do
 *
 * \memberof Outline
 */
static DiaMenu *
outline_get_object_menu(Outline *outline, Point *clickedpoint)
{
  return NULL;
}

/*!
 * \brief Set the object state from the given proeprty vector
 * \memberof Outline
 */
static void
outline_set_props (Outline *outline, GPtrArray *props)
{
  object_set_props_from_offsets(&outline->object, outline_offsets, props);
  outline_update_data (outline);
}


/**
 * outline_distance_from:
 *
 * Calculate the distance of the whole object to the given point
 */
static real
outline_distance_from (Outline *outline, Point *point)
{
  return distance_polygon_point (&outline->ink_rect[0], 4,
                                 outline->line_width, point);
}


/**
 * outline_move_handle:
 *
 * Move one of the objects handles
 */
static DiaObjectChange*
outline_move_handle (Outline          *outline,
                     Handle           *handle,
                     Point            *to,
                     ConnectionPoint  *cp,
                     HandleMoveReason  reason,
                     ModifierKeys      modifiers)
{
  DiaObject *obj = &outline->object;
  Point start = obj->position;
  Point end = outline->ink_rect[2];
  real dist, old_dist = distance_point_point (&start, &end);
  Point norm = end;
  point_sub (&norm, &start);
  point_normalize (&norm);

  /* we use this to modify angle and scale */
  switch (handle->id) {
    case HANDLE_RESIZE_NW :
      start = *to;
      break;
    case HANDLE_RESIZE_SE :
      end = *to;
      break;
    case HANDLE_RESIZE_N:
    case HANDLE_RESIZE_NE:
    case HANDLE_RESIZE_W:
    case HANDLE_RESIZE_E:
    case HANDLE_RESIZE_SW:
    case HANDLE_RESIZE_S:
    case HANDLE_MOVE_STARTPOINT:
    case HANDLE_MOVE_ENDPOINT:
    case HANDLE_CUSTOM1:
    case HANDLE_CUSTOM2:
    case HANDLE_CUSTOM3:
    case HANDLE_CUSTOM4:
    case HANDLE_CUSTOM5:
    case HANDLE_CUSTOM6:
    case HANDLE_CUSTOM7:
    case HANDLE_CUSTOM8:
    case HANDLE_CUSTOM9:
    default:
      g_warning ("Outline unknown handle");
  }

  dist = distance_point_point (&start, &end);
  /* disallow everything below a certain level, otherwise the font-size could become invalid */
  if (dist > 0.1) {
    obj->position = start;

    outline->font_height *= (dist / old_dist);

    outline_update_data (outline);
  }

  return NULL;
}


/*!
 * \brief Move the whole object to the given position
 *
 * If the object position does not change the whole object should not either.
 * This is used as a kludge to call the protected update_data() function
 *
 * \memberof Outline
 */
static DiaObjectChange*
outline_move (Outline *outline, Point *to)
{
  DiaObject *obj = &outline->object;

  obj->position = *to;

  outline_update_data (outline);
  return NULL;
}
/*!
 * \brief Create a deep copy of the object
 * \memberof Outline
 */
static DiaObject *
outline_copy (Outline *from)
{
  Outline *to;

  to = g_new0 (Outline, 1);
  object_copy (&from->object, &to->object);
  outline_init_handles (to);
  to->name = g_strdup (from->name);
  to->rotation = from->rotation;
  to->font = dia_font_copy (from->font);
  to->font_height = from->font_height;
  to->line_width = from->line_width;
  to->line_color = from->line_color;
  to->fill_color = from->fill_color;
  to->show_background = from->show_background;
  /* the rest will be recalculated in update_data() */
  outline_update_data (to);

  return &to->object;
}
/*!
 * \brief Destruction of the object
 * \memberof Outline
 */
static void
outline_destroy (Outline *outline)
{
  if (outline->path)
    cairo_path_destroy (outline->path);
  g_clear_pointer (&outline->name, g_free);
  object_destroy(&outline->object);
  /* but not the object itself? */
}
/*!
 * \brief Change the object state regarding selection
 * \memberof Outline
 */
static void
outline_select (Outline *outline, Point *clicked_point,
		DiaRenderer *interactive_renderer)
{
  outline_update_handles (outline);
}
