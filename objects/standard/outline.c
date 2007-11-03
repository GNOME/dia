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

#include <cairo.h>
#include <cairo-svg.h>

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
static void outline_save (Outline *outline, ObjectNode obj_node, 
                          const char *filename);
static DiaObject *outline_load (ObjectNode obj_node, int version, 
                                const char *filename);

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
  PROP_STD_TEXT_FONT,
  PROP_STD_TEXT_HEIGHT,
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
  obj->handles[0]->id = HANDLE_CUSTOM1;
  obj->handles[0]->pos = outline->ink_rect[0];

  obj->handles[1]->id = HANDLE_CUSTOM2;
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
  surface = cairo_svg_surface_create_for_stream (write_nul, NULL, 100, 100);
  cr = cairo_create (surface);
  cairo_surface_destroy (surface); /* in fact: unref() */
  style = dia_font_get_style (outline->font);
  /* not exact matching but almost the best we can do with the toy api */
  cairo_select_font_face (cr, dia_font_get_family (outline->font), 
                          DIA_FONT_STYLE_GET_SLANT (style) == DIA_FONT_NORMAL ? CAIRO_FONT_SLANT_NORMAL : CAIRO_FONT_SLANT_ITALIC,
                          DIA_FONT_STYLE_GET_WEIGHT (style) < DIA_FONT_MEDIUM ? CAIRO_FONT_SLANT_NORMAL : CAIRO_FONT_WEIGHT_BOLD);
  cairo_set_font_size (cr, outline->font_height);
  cairo_text_extents (cr, outline->name, &extents);
#if 0
  /* unfortunately this has no effect on the returned path */
  cairo_rotate (cr, outline->rotation/(2*G_PI));
#endif
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
  cairo_text_path (cr, outline->name);
  outline->path = cairo_copy_path (cr);
}
static void 
outline_draw(Outline *outline, DiaRenderer *renderer)
{
  DiaObject *obj = &outline->object;
  int i, n = 0;
  BezPoint *pts;
  real x, y;
  
  if (!outline->path)
    return;
  DIA_RENDERER_GET_CLASS (renderer)->set_linewidth (renderer, outline->line_width);
  DIA_RENDERER_GET_CLASS (renderer)->set_linestyle (renderer, LINESTYLE_SOLID);
  DIA_RENDERER_GET_CLASS (renderer)->set_linejoin(renderer, LINEJOIN_MITER);
  DIA_RENDERER_GET_CLASS (renderer)->set_linecaps(renderer, LINECAPS_ROUND);

  /* the cairo text path is position independent, Dia's bezier are not */
  x = obj->position.x;
  y = obj->position.y;

  /* split the path data into piece which can be handled by Dia's bezier rendering */
  pts = g_alloca (sizeof(BezPoint)*(outline->path->num_data+1));
  for (i=0; i < outline->path->num_data; i += outline->path->data[i].header.length) {
    cairo_path_data_t *data = &outline->path->data[i];
    if (CAIRO_PATH_MOVE_TO == data->header.type) {
      /* Dia can't handle moveto except at start */
      if (0 != n) {
        DIA_RENDERER_GET_CLASS (renderer)->draw_bezier (renderer, pts, n, &outline->line_color);
	n = 0;
      }
      pts[n].type = BEZ_MOVE_TO;
      pts[n].p1.x = data[1].point.x * outline->mat.xx + data[1].point.y * outline->mat.xy + x;
      pts[n].p1.y = data[1].point.x * outline->mat.yx + data[1].point.y * outline->mat.yy + y;
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
      if (outline->show_background)
        DIA_RENDERER_GET_CLASS (renderer)->fill_bezier (renderer, pts, n, &outline->fill_color);
      /* always draw the outline after the fill */
      pts[n].type = BEZ_LINE_TO;
      pts[n].p1.x = pts[0].p1.x;
      pts[n].p1.y = pts[0].p1.y;
      ++n;
      DIA_RENDERER_GET_CLASS (renderer)->draw_bezier (renderer, pts, n, &outline->line_color);
      n = 0;
    }
  }
}
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
  /* setup transform? */
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
  to->show_background = from->show_background;
  /* the rest will be recalculated in update_data() */

  return &to->object;
}
static void 
outline_destroy (Outline *outline)
{
  if (outline->path)
    cairo_path_destroy (outline->path);
  g_free (outline->name);
  /* but not the object itself? */
}
static void 
outline_select (Outline *outline, Point *clicked_point,
		DiaRenderer *interactive_renderer)
{
  outine_update_handles (outline);
}

