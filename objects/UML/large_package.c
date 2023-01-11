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
#include "diarenderer.h"
#include "attributes.h"
#include "text.h"
#include "properties.h"

#include "uml.h"
#include "stereotype.h"

#include "pixmaps/largepackage.xpm"

typedef struct _LargePackage LargePackage;

#define NUM_CONNECTIONS 9

struct _LargePackage {
  Element element;

  ConnectionPoint connections[NUM_CONNECTIONS];

  char *name;
  char *stereotype; /* Can be NULL, including << and >> */
  char *st_stereotype;

  DiaFont *font;

  real line_width;

  Color text_color;
  Color line_color;
  Color fill_color;

  real     font_height;

  real topwidth;
  real topheight;
};

/* The old border width, kept for compatibility with dia files created with
 * older versions.
 */
#define LARGEPACKAGE_BORDERWIDTH 0.1

static real largepackage_distance_from(LargePackage *pkg, Point *point);
static void largepackage_select(LargePackage *pkg, Point *clicked_point,
				DiaRenderer *interactive_renderer);
static DiaObjectChange* largepackage_move_handle(LargePackage *pkg, Handle *handle,
					      Point *to, ConnectionPoint *cp,
					      HandleMoveReason reason, ModifierKeys modifiers);
static DiaObjectChange* largepackage_move(LargePackage *pkg, Point *to);
static void largepackage_draw(LargePackage *pkg, DiaRenderer *renderer);
static DiaObject *largepackage_create(Point *startpoint,
				   void *user_data,
				   Handle **handle1,
				   Handle **handle2);
static void largepackage_destroy(LargePackage *pkg);

static void largepackage_update_data(LargePackage *pkg);

static PropDescription *largepackage_describe_props(LargePackage *largepackage);
static void largepackage_get_props(LargePackage *largepackage, GPtrArray *props);
static void largepackage_set_props(LargePackage *largepackage, GPtrArray *props);
static DiaObject *largepackage_load(ObjectNode obj_node, int version,DiaContext *ctx);

static ObjectTypeOps largepackage_type_ops =
{
  (CreateFunc) largepackage_create,
  (LoadFunc)   largepackage_load,/*using_properties*/     /* load */
  (SaveFunc)   object_save_using_properties,      /* save */
  (GetDefaultsFunc)   NULL,
  (ApplyDefaultsFunc) NULL
};

DiaObjectType largepackage_type =
{
  "UML - LargePackage",  /* name */
  0,                  /* version */
  largepackage_xpm,    /* pixmap */
  &largepackage_type_ops, /* ops */
  NULL, /* pixmap_file: fallback if pixmap is NULL */
  NULL, /* default_user_data: use this if no user data is specified in the .sheet file */
  NULL, /* prop_descs: property descriptions */
  NULL, /* prop_offsets: DiaObject struct offsets */
  DIA_OBJECT_CAN_PARENT /* flags */
};

static ObjectOps largepackage_ops = {
  (DestroyFunc)         largepackage_destroy,
  (DrawFunc)            largepackage_draw,
  (DistanceFunc)        largepackage_distance_from,
  (SelectFunc)          largepackage_select,
  (CopyFunc)            object_copy_using_properties,
  (MoveFunc)            largepackage_move,
  (MoveHandleFunc)      largepackage_move_handle,
  (GetPropertiesFunc)   object_create_props_dialog,
  (ApplyPropertiesDialogFunc) object_apply_props_from_dialog,
  (ObjectMenuFunc)      NULL,
  (DescribePropsFunc)   largepackage_describe_props,
  (GetPropsFunc)        largepackage_get_props,
  (SetPropsFunc)        largepackage_set_props,
  (TextEditFunc) 0,
  (ApplyPropertiesListFunc) object_apply_props,
};

static PropDescription largepackage_props[] = {
  ELEMENT_COMMON_PROPERTIES,
  { "name", PROP_TYPE_STRING, PROP_FLAG_VISIBLE,
  N_("Name"), NULL, NULL },
  { "stereotype", PROP_TYPE_STRING, PROP_FLAG_VISIBLE,
  N_("Stereotype"), NULL, NULL },
  /* can't use PROP_STD_TEXT_COLOUR_OPTIONAL cause it has PROP_FLAG_DONT_SAVE. It is designed to fill the Text object - not some subset */
  PROP_STD_TEXT_FONT_OPTIONS(PROP_FLAG_VISIBLE|PROP_FLAG_STANDARD|PROP_FLAG_OPTIONAL),
  PROP_STD_TEXT_HEIGHT_OPTIONS(PROP_FLAG_VISIBLE|PROP_FLAG_STANDARD|PROP_FLAG_OPTIONAL),
  PROP_STD_TEXT_COLOUR_OPTIONS(PROP_FLAG_VISIBLE|PROP_FLAG_STANDARD|PROP_FLAG_OPTIONAL),
  PROP_STD_LINE_WIDTH_OPTIONAL,
  PROP_STD_LINE_COLOUR_OPTIONAL,
  PROP_STD_FILL_COLOUR_OPTIONAL,
  PROP_DESC_END
};

static PropDescription *
largepackage_describe_props(LargePackage *largepackage)
{
  if (largepackage_props[0].quark == 0) {
    prop_desc_list_calculate_quarks(largepackage_props);
  }
  return largepackage_props;
}

static PropOffset largepackage_offsets[] = {
  ELEMENT_COMMON_PROPERTIES_OFFSETS,
  {"stereotype", PROP_TYPE_STRING, offsetof(LargePackage , stereotype) },
  {"name", PROP_TYPE_STRING, offsetof(LargePackage , name) },
  { "text_font", PROP_TYPE_FONT, offsetof(LargePackage, font) },
  { PROP_STDNAME_TEXT_HEIGHT, PROP_STDTYPE_TEXT_HEIGHT, offsetof(LargePackage, font_height) },
  {"text_colour",PROP_TYPE_COLOUR,offsetof(LargePackage,text_color)},
  { PROP_STDNAME_LINE_WIDTH, PROP_STDTYPE_LINE_WIDTH, offsetof(LargePackage, line_width) },
  {"line_colour",PROP_TYPE_COLOUR,offsetof(LargePackage,line_color)},
  {"fill_colour",PROP_TYPE_COLOUR,offsetof(LargePackage,fill_color)},

  { NULL, 0, 0 },
};

static void
largepackage_get_props(LargePackage * largepackage, GPtrArray *props)
{
  object_get_props_from_offsets(&largepackage->element.object,
                                largepackage_offsets, props);
}


static void
largepackage_set_props (LargePackage *largepackage, GPtrArray *props)
{
  object_set_props_from_offsets (&largepackage->element.object,
                                 largepackage_offsets,
                                 props);
  g_clear_pointer (&largepackage->st_stereotype, g_free);
  largepackage_update_data (largepackage);
}


static real
largepackage_distance_from(LargePackage *pkg, Point *point)
{
  /* need to calculate the distance from both rects to make the autogap
   * use the right boundaries
   */
  Element *elem = &pkg->element;
  real x = elem->corner.x;
  real y = elem->corner.y;
  DiaRectangle r1 = { x, y, x + elem->width, y + elem->height };
  DiaRectangle r2 = { x, y - pkg->topheight, x + pkg->topwidth, y };
  real d1 = distance_rectangle_point(&r1, point);
  real d2 = distance_rectangle_point(&r2, point);

  /* always return  the value closest to zero or below */
  return MIN(d1, d2);
}

static void
largepackage_select(LargePackage *pkg, Point *clicked_point,
		    DiaRenderer *interactive_renderer)
{
  element_update_handles(&pkg->element);
}


static DiaObjectChange *
largepackage_move_handle (LargePackage     *pkg,
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

  element_move_handle (&pkg->element, handle->id, to, cp, reason, modifiers);
  largepackage_update_data (pkg);

  return NULL;
}


static DiaObjectChange*
largepackage_move(LargePackage *pkg, Point *to)
{
  pkg->element.corner = *to;

  largepackage_update_data(pkg);

  return NULL;
}


static void
largepackage_draw (LargePackage *pkg, DiaRenderer *renderer)
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

  p1.x= x; p1.y = y - pkg->topheight;
  p2.x = x + pkg->topwidth; p2.y = y;
  dia_renderer_draw_rect (renderer,
                          &p1,
                          &p2,
                          &pkg->fill_color,
                          &pkg->line_color);

  dia_renderer_set_font (renderer, pkg->font, pkg->font_height);

  p1.x = x + 0.1;
  p1.y = y - pkg->font_height -
      dia_font_descent (pkg->st_stereotype,
                        pkg->font,
                        pkg->font_height) - 0.1;



  if (pkg->st_stereotype && pkg->st_stereotype[0] != '\0') {
    dia_renderer_draw_string (renderer,
                              pkg->st_stereotype,
                              &p1,
                              DIA_ALIGN_LEFT,
                              &pkg->text_color);
  }
  p1.y += pkg->font_height;

  if (pkg->name) {
    dia_renderer_draw_string (renderer,
                              pkg->name,
                              &p1,
                              DIA_ALIGN_LEFT,
                              &pkg->text_color);
  }
}

static void
largepackage_update_data(LargePackage *pkg)
{
  Element *elem = &pkg->element;
  DiaObject *obj = &elem->object;

  pkg->stereotype = remove_stereotype_from_string(pkg->stereotype);
  if (!pkg->st_stereotype) {
    pkg->st_stereotype = string_to_stereotype(pkg->stereotype);
  }

  pkg->topheight = pkg->font_height + 0.1*2;

  pkg->topwidth = 2.0;
  if (pkg->name != NULL)
    pkg->topwidth = MAX(pkg->topwidth,
                        dia_font_string_width(pkg->name, pkg->font,
                                          pkg->font_height)+2*0.1);
  if (pkg->st_stereotype != NULL && pkg->st_stereotype[0] != '\0') {
    pkg->topwidth = MAX(pkg->topwidth,
                        dia_font_string_width(pkg->st_stereotype, pkg->font,
                                              pkg->font_height)+2*0.1);
    pkg->topheight += pkg->font_height;
  }

  if (elem->width < (pkg->topwidth + 0.2))
    elem->width = pkg->topwidth + 0.2;
  if (elem->height < 1.0)
    elem->height = 1.0;

  /* Update connections: */
  element_update_connections_rectangle(elem, pkg->connections);

  element_update_boundingbox(elem);
  /* fix boundingbox for top rectangle: */
  obj->bounding_box.top -= pkg->topheight;

  obj->position = elem->corner;

  element_update_handles(elem);
}

static DiaObject *
largepackage_create(Point *startpoint,
		    void *user_data,
		    Handle **handle1,
		    Handle **handle2)
{
  LargePackage *pkg;
  Element *elem;
  DiaObject *obj;
  int i;

  pkg = g_new0 (LargePackage, 1);
  elem = &pkg->element;
  obj = &elem->object;

  obj->type = &largepackage_type;

  obj->ops = &largepackage_ops;

  elem->corner = *startpoint;

  element_init(elem, 8, NUM_CONNECTIONS);

  elem->width = 4.0;
  elem->height = 4.0;

  pkg->line_width = attributes_get_default_linewidth();
  pkg->text_color = color_black;
  pkg->line_color = attributes_get_foreground();
  pkg->fill_color = attributes_get_background();
  /* old defaults */
  pkg->font_height = 0.8;
  pkg->font = dia_font_new_from_style(DIA_FONT_MONOSPACE, pkg->font_height);
  pkg->line_width = 0.1;

  pkg->stereotype = NULL;
  pkg->st_stereotype = NULL;
  pkg->name = g_strdup("");

  pkg->topwidth = 2.0;
  pkg->topheight = pkg->font_height*2 + 0.1*2;

  for (i=0;i<NUM_CONNECTIONS;i++) {
    obj->connections[i] = &pkg->connections[i];
    pkg->connections[i].object = obj;
    pkg->connections[i].connected = NULL;
  }
  pkg->connections[8].flags = CP_FLAGS_MAIN;
  pkg->element.extra_spacing.border_trans = pkg->line_width/2.0;
  largepackage_update_data(pkg);

  *handle1 = NULL;
  *handle2 = obj->handles[7];
  return &pkg->element.object;
}


static void
largepackage_destroy (LargePackage *pkg)
{
  g_clear_object (&pkg->font);
  g_clear_pointer (&pkg->stereotype, g_free);
  g_clear_pointer (&pkg->st_stereotype, g_free);
  g_clear_pointer (&pkg->name, g_free);

  element_destroy (&pkg->element);
}


static DiaObject *
largepackage_load(ObjectNode obj_node, int version,DiaContext *ctx)
{
  DiaObject *obj = object_load_using_properties(&largepackage_type,
                                                obj_node,version,ctx);
  AttributeNode attr;
  /* For compatibility with previous dia files. If no line_width, use
   * LARGEPACKAGE_BORDERWIDTH, that was the previous line width.
   */
  attr = object_find_attribute(obj_node, PROP_STDNAME_LINE_WIDTH);
  if (attr == NULL)
    ((LargePackage*)obj)->line_width = LARGEPACKAGE_BORDERWIDTH;

  return obj;
}


