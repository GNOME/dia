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
 * A prototype for text effects with the standard renderer interface.
 * It should be possible to do all those fancy text effects like outline, rotate, ...
 * by converting text to pathes before rendering.
 */
#include <config.h>

#include "object.h"
#include "diarenderer.h"
#include "attributes.h"
#include "properties.h"
#include "boundingbox.h"

#include "tool-icons.h"

#ifdef HAVE_CAIRO
#include <cairo.h>

#ifdef CAIRO_HAS_SVG_SURFACE 
#include <cairo-svg.h>
#endif

#define NUM_HANDLES 2
/* Object definition */
typedef struct _Outline {
  DiaObject object;
  
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
outline_load(ObjectNode obj_node, int version, const char *filename);

static ObjectTypeOps outline_type_ops =
{
  (CreateFunc)outline_create,
  (LoadFunc)  outline_load,
  (SaveFunc)  object_save_using_properties, /* outline_save */
  (GetDefaultsFunc)   NULL,
  (ApplyDefaultsFunc) NULL
};

static DiaObjectType outline_type =
{
  "Standard - Outline",   /* name */
  0,                      /* version */
  (char **) outline_icon, /* pixmap */
  
  &outline_type_ops,      /* ops */
  NULL,                   /* pixmap_file */
  0                       /* default_user_data */
};

/* make accesible from the outside for type regristation */
DiaObjectType *_outline_type = (DiaObjectType *) &outline_type;

/* Class definition */
static ObjectChange* outline_move_handle (Outline *outline,
                                          Handle *handle,
					  Point *to, ConnectionPoint *cp,
					  HandleMoveReason reason, ModifierKeys modifiers);
static ObjectChange* outline_move (Outline *outline, Point *to);
static void outline_select(Outline *outline, Point *clicked_point,
			   DiaRenderer *interactive_renderer);
static void outline_draw(Outline *outline, DiaRenderer *renderer);
static real outline_distance_from (Outline *outline, Point *point);
static void outline_update_data (Outline *outline);
static void outline_destroy (Outline *outline);
static DiaObject *outline_copy (Outline *outline);
static DiaMenu *outline_get_object_menu(Outline *outline,
					Point *clickedpoint);
static PropDescription *outline_describe_props(Outline *outline);
static void outline_get_props(Outline *outline, GPtrArray *props);
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
  (DescribePropsFunc)   outline_describe_props,
  (GetPropsFunc)        outline_get_props,
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
outline_load(ObjectNode obj_node, int version, const char *filename)
{
  return object_load_using_properties(&outline_type,
                                      obj_node,version,filename);
}

/* Class/Object implementation */
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
outine_update_handles(Outline *outline)
{
  DiaObject *obj = &outline->object;

  g_return_if_fail (obj->handles != NULL);
  obj->handles[0]->id = HANDLE_RESIZE_NW;
  obj->handles[0]->pos = outline->ink_rect[0];

  obj->handles[1]->id = HANDLE_RESIZE_SE;
  obj->handles[1]->pos = outline->ink_rect[2];
}
/*! Not in the object interface but very important anyway. Used to recalculate the object data after a change  */
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
#ifdef CAIRO_HAS_SVG_SURFACE
  surface = cairo_svg_surface_create_for_stream (write_nul, NULL, 100, 100);
#else
  /* if only I could remember why I have choosen the svg surface in the first place */
  surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 100, 100);
#endif
  cr = cairo_create (surface);
  cairo_surface_destroy (surface); /* in fact: unref() */
  style = dia_font_get_style (outline->font);
  /* not exact matching but almost the best we can do with the toy api */
  cairo_select_font_face (cr, dia_font_get_family (outline->font), 
                          DIA_FONT_STYLE_GET_SLANT (style) == DIA_FONT_NORMAL ? CAIRO_FONT_SLANT_NORMAL : CAIRO_FONT_SLANT_ITALIC,
                          DIA_FONT_STYLE_GET_WEIGHT (style) < DIA_FONT_MEDIUM ? CAIRO_FONT_SLANT_NORMAL : CAIRO_FONT_WEIGHT_BOLD);
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

  outine_update_handles (outline),

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
static void 
outline_draw(Outline *outline, DiaRenderer *renderer)
{
  DiaObject *obj = &outline->object;
  int i, n = 0, total;
  BezPoint *pts;
  real x, y;
  Point ps = {0, 0}; /* silence gcc */
  
  if (!outline->path)
    return;
  DIA_RENDERER_GET_CLASS (renderer)->set_linewidth (renderer, outline->line_width);
  DIA_RENDERER_GET_CLASS (renderer)->set_linestyle (renderer, LINESTYLE_SOLID);
  DIA_RENDERER_GET_CLASS (renderer)->set_linejoin(renderer, LINEJOIN_MITER);
  DIA_RENDERER_GET_CLASS (renderer)->set_linecaps(renderer, LINECAPS_ROUND);

  /* the cairo text path is position independent, Dia's bezier are not */
  x = obj->position.x;
  y = obj->position.y;

  /* count Dia BezPoints required */
  total = 0;
  for (i=0; i < outline->path->num_data; i += outline->path->data[i].header.length) {
    ++total;
  }

  pts = g_alloca (sizeof(BezPoint)*(total));
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

  if (DIA_RENDERER_GET_CLASS (renderer)->is_capable_to(renderer, RENDER_HOLES)) {
    if (outline->show_background)
      DIA_RENDERER_GET_CLASS (renderer)->fill_bezier(renderer, pts, total, &outline->fill_color);
    DIA_RENDERER_GET_CLASS (renderer)->draw_bezier (renderer, pts, total, &outline->line_color);
    return; /* that was easy ;) */
  }
  /* otherwise split the path data into piece which can be handled by Dia's standard bezier rendering */
  if (outline->show_background) {
    /* first draw the fills */
    int s1 = 0, n1 = 0;
    int s2 = 0;
    for (i = 1; i < total; ++i) {
      if (BEZ_MOVE_TO == pts[i].type) {
        /* check whether the start point of the second outline is within the first outline. 
	 * If so it need to be subtracted - currently blanked. */
	real dist = distance_bez_shape_point (&pts[s1], 
	  n1 > 0 ? n1 : i - s1, 0, &pts[i].p1);
	if (s2 > s1) { /* blanking the previous one */
	  n = i - s2 - 1;
          DIA_RENDERER_GET_CLASS (renderer)->fill_bezier (renderer, &pts[s2], n, &color_white);
	} else { /* fill the outer shape */
	  n1 = n = i - s1;
          DIA_RENDERER_GET_CLASS (renderer)->fill_bezier (renderer, &pts[s1], n, &outline->fill_color);
	}
	if (dist > 0) { /* remember as new outer outline */
	  s1 = i;
	  n1 = 0;
	  s2 = 0;
	} else {
	  s2 = i;
	}
      }
    }
    /* the last one is not drawn yet */
    if (s2 > s1) { /* blanking the previous one */
      if (s2 - i - 1 > 1) /* depending on the above we may be ready */
        DIA_RENDERER_GET_CLASS (renderer)->fill_bezier (renderer, &pts[s2], s2 - i - 1, &color_white);
    } else {
      if (s1 - i - 1 > 1)
        DIA_RENDERER_GET_CLASS (renderer)->fill_bezier (renderer, &pts[s1], s1 - i - 1, &outline->fill_color);
    }
  } /* show_background */
  n = 0;
  for (i = 1; i < total; ++i) {
    if (BEZ_MOVE_TO == pts[i].type) {
      DIA_RENDERER_GET_CLASS (renderer)->draw_bezier (renderer, &pts[n], i - n, &outline->line_color);
      n = i;
    }
  }
  /* the last one, if there is one */
  if (i - n - 1 > 0)
    DIA_RENDERER_GET_CLASS (renderer)->draw_bezier (renderer, &pts[n], i - n - 1, &outline->line_color);
}
/* returning used to crash Dia, the method needed to be NULL if there is nothing to do */
static DiaMenu *
outline_get_object_menu(Outline *outline, Point *clickedpoint)
{
  return NULL;
}

/*! A standard props compliant object needs to describe its parameters */
static PropDescription *
outline_describe_props (Outline *outline)
{
  if (outline_props[0].quark == 0)
    prop_desc_list_calculate_quarks(outline_props);
  return outline_props;
}
static void 
outline_get_props (Outline *outline, GPtrArray *props)
{
  object_get_props_from_offsets(&outline->object, outline_offsets, props);
}
static void 
outline_set_props (Outline *outline, GPtrArray *props)
{
  object_set_props_from_offsets(&outline->object, outline_offsets, props);
  outline_update_data (outline);
}
static real
outline_distance_from (Outline *outline, Point *point)
{
  return distance_polygon_point (&outline->ink_rect[0], 4, outline->line_width, point);
}
static ObjectChange* 
outline_move_handle (Outline *outline,
                     Handle *handle,
		     Point *to, ConnectionPoint *cp,
		     HandleMoveReason reason, ModifierKeys modifiers)
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
  default :
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
static ObjectChange* 
outline_move (Outline *outline, Point *to)
{
  DiaObject *obj = &outline->object;

  obj->position = *to;

  outline_update_data (outline);
  return NULL;
}
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
static void 
outline_destroy (Outline *outline)
{
  if (outline->path)
    cairo_path_destroy (outline->path);
  g_free (outline->name);
  object_destroy(&outline->object);
  /* but not the object itself? */
}
static void 
outline_select (Outline *outline, Point *clicked_point,
		DiaRenderer *interactive_renderer)
{
  outine_update_handles (outline);
}

#endif /* HAVE_CAIRO */

