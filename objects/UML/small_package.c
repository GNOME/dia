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

#include "config.h"

#include <glib/gi18n-lib.h>

#include <math.h>
#include <string.h>

#include "object.h"
#include "element.h"
#include "dia-text.h"
#include "diarenderer.h"
#include "attributes.h"
#include "properties.h"

#include "stereotype.h"

#include "pixmaps/smallpackage.xpm"

#define NUM_CONNECTIONS 9

typedef struct _SmallPackage SmallPackage;
struct _SmallPackage {
  Element element;

  ConnectionPoint connections[NUM_CONNECTIONS];

  char *stereotype;
  DiaText *text;

  char *st_stereotype;

  double line_width;
  DiaColour line_color;
  DiaColour fill_color;
};

/* The old border width, kept for compatibility with dia files created with
 * older versions.
 */
#define SMALLPACKAGE_BORDERWIDTH 0.1
#define SMALLPACKAGE_TOPHEIGHT 0.9
#define SMALLPACKAGE_TOPWIDTH 1.5
#define SMALLPACKAGE_MARGIN_X 0.3
#define SMALLPACKAGE_MARGIN_Y 0.3

static real smallpackage_distance_from(SmallPackage *pkg, Point *point);
static void smallpackage_select(SmallPackage *pkg, Point *clicked_point,
				DiaRenderer *interactive_renderer);
static DiaObjectChange* smallpackage_move_handle(SmallPackage *pkg, Handle *handle,
					      Point *to, ConnectionPoint *cp,
					      HandleMoveReason reason, ModifierKeys modifiers);
static DiaObjectChange* smallpackage_move(SmallPackage *pkg, Point *to);
static void smallpackage_draw(SmallPackage *pkg, DiaRenderer *renderer);
static DiaObject *smallpackage_create(Point *startpoint,
				   void *user_data,
				   Handle **handle1,
				   Handle **handle2);
static void smallpackage_destroy(SmallPackage *pkg);
static DiaObject *smallpackage_load(ObjectNode obj_node, int version,DiaContext *ctx);

static PropDescription *smallpackage_describe_props(SmallPackage *smallpackage);
static void smallpackage_get_props(SmallPackage *smallpackage, GPtrArray *props);
static void smallpackage_set_props(SmallPackage *smallpackage, GPtrArray *props);

static void smallpackage_update_data(SmallPackage *pkg);

static ObjectTypeOps smallpackage_type_ops =
{
  (CreateFunc) smallpackage_create,
  (LoadFunc)   smallpackage_load,/*using_properties*/     /* load */
  (SaveFunc)   object_save_using_properties,      /* save */
  (GetDefaultsFunc)   NULL,
  (ApplyDefaultsFunc) NULL
};

DiaObjectType smallpackage_type =
{
  "UML - SmallPackage", /* name */
  0,                 /* version */
  smallpackage_xpm,   /* pixmap */
  &smallpackage_type_ops /* ops */
};

static ObjectOps smallpackage_ops = {
  (DestroyFunc)         smallpackage_destroy,
  (DrawFunc)            smallpackage_draw,
  (DistanceFunc)        smallpackage_distance_from,
  (SelectFunc)          smallpackage_select,
  (CopyFunc)            object_copy_using_properties,
  (MoveFunc)            smallpackage_move,
  (MoveHandleFunc)      smallpackage_move_handle,
  (GetPropertiesFunc)   object_create_props_dialog,
  (ApplyPropertiesDialogFunc) object_apply_props_from_dialog,
  (ObjectMenuFunc)      NULL,
  (DescribePropsFunc)   smallpackage_describe_props,
  (GetPropsFunc)        smallpackage_get_props,
  (SetPropsFunc)        smallpackage_set_props,
  (ApplyPropertiesListFunc) object_apply_props,
};

static PropDescription smallpackage_props[] = {
  ELEMENT_COMMON_PROPERTIES,
  { "stereotype", PROP_TYPE_STRING, PROP_FLAG_VISIBLE,
  N_("Stereotype"), NULL, NULL },
  PROP_STD_TEXT_FONT,
  PROP_STD_TEXT_HEIGHT,
  PROP_STD_TEXT_COLOUR,
  { "text", PROP_TYPE_TEXT, 0, N_("Text"), NULL, NULL },
  PROP_STD_LINE_WIDTH_OPTIONAL,
  PROP_STD_LINE_COLOUR_OPTIONAL,
  PROP_STD_FILL_COLOUR_OPTIONAL,
  PROP_DESC_END
};

static PropDescription *
smallpackage_describe_props(SmallPackage *smallpackage)
{
  if (smallpackage_props[0].quark == 0) {
    prop_desc_list_calculate_quarks(smallpackage_props);
  }
  return smallpackage_props;
}

static PropOffset smallpackage_offsets[] = {
  ELEMENT_COMMON_PROPERTIES_OFFSETS,
  {"stereotype", PROP_TYPE_STRING, offsetof(SmallPackage , stereotype) },
  {"text",PROP_TYPE_TEXT,offsetof(SmallPackage,text)},
  {"text_font",PROP_TYPE_FONT,offsetof(SmallPackage,text), DIA_TEXT_FONT_OFFSET},
  {PROP_STDNAME_TEXT_HEIGHT,PROP_STDTYPE_TEXT_HEIGHT,offsetof(SmallPackage,text), DIA_TEXT_HEIGHT_OFFSET},
  {"text_colour",PROP_TYPE_COLOUR,offsetof(SmallPackage,text), DIA_TEXT_COLOUR_OFFSET},
  { PROP_STDNAME_LINE_WIDTH, PROP_STDTYPE_LINE_WIDTH, offsetof(SmallPackage, line_width) },
  {"line_colour", PROP_TYPE_COLOUR, offsetof(SmallPackage, line_color) },
  {"fill_colour", PROP_TYPE_COLOUR, offsetof(SmallPackage, fill_color) },
  { NULL, 0, 0 },
};

static void
smallpackage_get_props(SmallPackage * smallpackage,
                       GPtrArray *props)
{
  object_get_props_from_offsets(&smallpackage->element.object,
                                smallpackage_offsets,props);
}


static void
smallpackage_set_props (SmallPackage *smallpackage,
                        GPtrArray    *props)
{
  object_set_props_from_offsets (&smallpackage->element.object,
                                 smallpackage_offsets,
                                 props);
  g_clear_pointer (&smallpackage->st_stereotype, g_free);
  smallpackage_update_data(smallpackage);
}


static double
smallpackage_distance_from (SmallPackage *pkg, Point *point)
{
  /* need to calculate the distance from both rects to make the autogap
   * use the right boundaries
   */
  Element *elem = &pkg->element;
  real x = elem->corner.x;
  real y = elem->corner.y;
  DiaRectangle r1 = { x, y, x + elem->width, y + elem->height };
  DiaRectangle r2 = { x, y - SMALLPACKAGE_TOPHEIGHT, x + SMALLPACKAGE_TOPWIDTH, y };
  real d1 = distance_rectangle_point(&r1, point);
  real d2 = distance_rectangle_point(&r2, point);

  /* always return  the value closest to zero or below */
  return MIN(d1, d2);
}


static void
smallpackage_select (SmallPackage *pkg,
                     Point        *clicked_point,
                     DiaRenderer  *interactive_renderer)
{
  dia_text_set_cursor (pkg->text, clicked_point, interactive_renderer);
  dia_text_grab_focus (pkg->text, &pkg->element.object);
  element_update_handles (&pkg->element);
}


static DiaObjectChange *
smallpackage_move_handle (SmallPackage     *pkg,
                          Handle           *handle,
                          Point            *to,
                          ConnectionPoint  *cp,
                          HandleMoveReason  reason,
                          ModifierKeys      modifiers)
{
  g_return_val_if_fail (pkg != NULL, NULL);
  g_return_val_if_fail (handle != NULL, NULL);
  g_return_val_if_fail (to != NULL, NULL);

  g_return_val_if_fail (handle->id < 8, NULL);

  return NULL;
}


static DiaObjectChange*
smallpackage_move(SmallPackage *pkg, Point *to)
{
  Point p;

  pkg->element.corner = *to;

  p = *to;
  p.x += SMALLPACKAGE_MARGIN_X;
  p.y += dia_text_get_ascent (pkg->text) + SMALLPACKAGE_MARGIN_Y;
  dia_text_set_position (pkg->text, &p);

  smallpackage_update_data(pkg);

  return NULL;
}

static void
smallpackage_draw (SmallPackage *pkg, DiaRenderer *renderer)
{
  Element *elem;
  double x, y, w, h;
  Point p1, p2;

  g_return_if_fail (pkg != NULL);
  g_return_if_fail (renderer != NULL);

  elem = &pkg->element;

  x = elem->corner.x;
  y = elem->corner.y;
  w = elem->width;
  h = elem->height;

  dia_renderer_set_fillstyle (renderer, DIA_FILL_STYLE_SOLID);
  dia_renderer_set_linewidth (renderer, pkg->line_width);
  dia_renderer_set_linestyle (renderer, DIA_LINE_STYLE_SOLID, 0.0);

  p1.x = x; p1.y = y;
  p2.x = x+w; p2.y = y+h;

  dia_renderer_draw_rect (renderer,
                          &p1,
                          &p2,
                          &pkg->fill_color,
                          &pkg->line_color);

  p1.x= x; p1.y = y-SMALLPACKAGE_TOPHEIGHT;
  p2.x = x+SMALLPACKAGE_TOPWIDTH; p2.y = y;

  dia_renderer_draw_rect (renderer,
                          &p1,
                          &p2,
                          &pkg->fill_color,
                          &pkg->line_color);

  dia_text_draw (pkg->text, renderer);

  if ((pkg->st_stereotype != NULL) && (pkg->st_stereotype[0] != '\0')) {
    DiaColour text_colour;

    dia_renderer_set_font (renderer,
                           dia_text_get_font (pkg->text),
                           dia_text_get_height (pkg->text));

    dia_text_get_position (pkg->text, &p1);
    p1.y -= dia_text_get_height (pkg->text);

    dia_text_get_colour (pkg->text, &text_colour);
    dia_renderer_draw_string (renderer,
                              pkg->st_stereotype,
                              &p1,
                              DIA_ALIGN_LEFT,
                              &text_colour);
  }
}

static void
smallpackage_update_data(SmallPackage *pkg)
{
  Element *elem = &pkg->element;
  DiaObject *obj = &elem->object;
  Point p;
  DiaFont *font;

  pkg->stereotype = remove_stereotype_from_string(pkg->stereotype);
  if (!pkg->st_stereotype) {
    pkg->st_stereotype =  string_to_stereotype(pkg->stereotype);
  }

  dia_text_calc_boundingbox (pkg->text, NULL);
  elem->width = dia_text_get_max_width (pkg->text) +
    (2 * SMALLPACKAGE_MARGIN_X);
  elem->width = MAX (elem->width, SMALLPACKAGE_TOPWIDTH + 1.0);
  elem->height =
    (dia_text_get_height (pkg->text) * dia_text_get_n_lines (pkg->text)) +
      (2 * SMALLPACKAGE_MARGIN_Y);

  p = elem->corner;
  p.x += SMALLPACKAGE_MARGIN_X;
  p.y += SMALLPACKAGE_MARGIN_Y + dia_text_get_ascent (pkg->text);

  if ((pkg->stereotype != NULL) && (pkg->stereotype[0] != '\0')) {
    font = dia_text_get_font (pkg->text);
    elem->height += dia_text_get_height (pkg->text);
    elem->width = MAX (elem->width,
                       dia_font_string_width (pkg->st_stereotype,
                                              font,
                                              dia_text_get_height (pkg->text)) +
                       2 * SMALLPACKAGE_MARGIN_X);
    p.y += dia_text_get_height (pkg->text);
  }

  dia_text_set_position (pkg->text, &p);

  /* Update connections: */
  element_update_connections_rectangle(elem, pkg->connections);

  element_update_boundingbox(elem);
  /* fix boundingbox for top rectangle: */
  obj->bounding_box.top -= SMALLPACKAGE_TOPHEIGHT;

  obj->position = elem->corner;

  element_update_handles(elem);
}

static DiaObject *
smallpackage_create(Point *startpoint,
		    void *user_data,
		    Handle **handle1,
		    Handle **handle2)
{
  SmallPackage *pkg;
  Element *elem;
  DiaObject *obj;
  Point p;
  DiaFont *font;
  int i;

  pkg = g_new0 (SmallPackage, 1);
  elem = &pkg->element;
  obj = &elem->object;

  obj->type = &smallpackage_type;

  obj->ops = &smallpackage_ops;

  elem->corner = *startpoint;

  font = dia_font_new_from_style(DIA_FONT_MONOSPACE, 0.8);
  p = *startpoint;
  p.x += SMALLPACKAGE_MARGIN_X;
  p.y += SMALLPACKAGE_MARGIN_Y+ dia_font_ascent("A",font, 0.8);

  pkg->text = dia_text_new ("",
                            font,
                            0.8,
                            &p,
                            &DIA_COLOUR_BLACK,
                            DIA_ALIGN_LEFT);
  g_clear_object (&font);

  element_init (elem, 8, NUM_CONNECTIONS);

  for (i=0;i<NUM_CONNECTIONS;i++) {
    obj->connections[i] = &pkg->connections[i];
    pkg->connections[i].object = obj;
    pkg->connections[i].connected = NULL;
  }
  pkg->connections[8].flags = CP_FLAGS_MAIN;
  pkg->line_width = attributes_get_default_linewidth();

  elem->extra_spacing.border_trans = pkg->line_width/2.0;

  pkg->line_color = attributes_get_foreground();
  pkg->fill_color = attributes_get_background();

  pkg->stereotype = NULL;
  pkg->st_stereotype = NULL;
  smallpackage_update_data(pkg);

  for (i=0;i<8;i++) {
    obj->handles[i]->type = HANDLE_NON_MOVABLE;
  }

  *handle1 = NULL;
  *handle2 = NULL;
  return &pkg->element.object;
}


static void
smallpackage_destroy (SmallPackage *pkg)
{
  dia_clear_part (&pkg->text);

  g_clear_pointer (&pkg->stereotype, g_free);
  g_clear_pointer (&pkg->st_stereotype, g_free);

  element_destroy (&pkg->element);
}


static DiaObject *
smallpackage_load(ObjectNode obj_node, int version,DiaContext *ctx)
{
  DiaObject *obj = object_load_using_properties(&smallpackage_type,
                                                obj_node,version,ctx);
  AttributeNode attr;
  /* For compatibility with previous dia files. If no line_width, use
   * SMALLPACKAGE_BORDERWIDTH, that was the previous line width.
   */
  attr = object_find_attribute(obj_node, PROP_STDNAME_LINE_WIDTH);
  if (attr == NULL)
    ((SmallPackage*)obj)->line_width = SMALLPACKAGE_BORDERWIDTH;

  return obj;
}


