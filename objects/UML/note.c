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

#include "pixmaps/note.xpm"

#define NUM_CONNECTIONS 9

typedef struct _Note Note;
struct _Note {
  Element element;

  ConnectionPoint connections[NUM_CONNECTIONS];

  Text *text;

  real line_width;
  Color line_color;
  Color fill_color;
};

#define NOTE_BORDERWIDTH 0.1
#define NOTE_CORNER 0.6
#define NOTE_MARGIN_X 0.3
#define NOTE_MARGIN_Y 0.3

static real note_distance_from(Note *note, Point *point);
static void note_select(Note *note, Point *clicked_point,
			DiaRenderer *interactive_renderer);
static DiaObjectChange* note_move_handle(Note *note, Handle *handle,
				      Point *to, ConnectionPoint *cp,
				      HandleMoveReason reason, ModifierKeys modifiers);
static DiaObjectChange* note_move(Note *note, Point *to);
static void note_draw(Note *note, DiaRenderer *renderer);
static DiaObject *note_create(Point *startpoint,
			   void *user_data,
			   Handle **handle1,
			   Handle **handle2);
static void note_destroy(Note *note);
static DiaObject *note_load(ObjectNode obj_node, int version,DiaContext *ctx);

static PropDescription *note_describe_props(Note *note);
static void note_get_props(Note *note, GPtrArray *props);
static void note_set_props(Note *note, GPtrArray *props);

static void note_update_data(Note *note);

static ObjectTypeOps note_type_ops =
{
  (CreateFunc) note_create,
  (LoadFunc)   note_load,/*using_properties*/     /* load */
  (SaveFunc)   object_save_using_properties,      /* save */
  (GetDefaultsFunc)   NULL,
  (ApplyDefaultsFunc) NULL
};

DiaObjectType note_type =
{
  "UML - Note", /* name */
  0,         /* version */
  note_xpm,   /* pixmap */
  &note_type_ops /* ops */
};

static ObjectOps note_ops = {
  (DestroyFunc)         note_destroy,
  (DrawFunc)            note_draw,
  (DistanceFunc)        note_distance_from,
  (SelectFunc)          note_select,
  (CopyFunc)            object_copy_using_properties,
  (MoveFunc)            note_move,
  (MoveHandleFunc)      note_move_handle,
  (GetPropertiesFunc)   object_create_props_dialog,
  (ApplyPropertiesDialogFunc) object_apply_props_from_dialog,
  (ObjectMenuFunc)      NULL,
  (DescribePropsFunc)   note_describe_props,
  (GetPropsFunc)        note_get_props,
  (SetPropsFunc)        note_set_props,
  (TextEditFunc) 0,
  (ApplyPropertiesListFunc) object_apply_props,
};

static PropDescription note_props[] = {
  ELEMENT_COMMON_PROPERTIES,
  PROP_STD_TEXT_FONT,
  PROP_STD_TEXT_HEIGHT,
  PROP_STD_TEXT_COLOUR_OPTIONAL,
  { "text", PROP_TYPE_TEXT, 0, N_("Text"), NULL, NULL },
  PROP_STD_LINE_WIDTH_OPTIONAL,
  PROP_STD_LINE_COLOUR_OPTIONAL,
  PROP_STD_FILL_COLOUR_OPTIONAL,
  PROP_DESC_END
};

static PropDescription *
note_describe_props(Note *note)
{
  if (note_props[0].quark == 0) {
    prop_desc_list_calculate_quarks(note_props);
  }
  return note_props;
}

static PropOffset note_offsets[] = {
  ELEMENT_COMMON_PROPERTIES_OFFSETS,
  {"text",PROP_TYPE_TEXT,offsetof(Note,text)},
  {"text_font",PROP_TYPE_FONT,offsetof(Note,text),offsetof(Text,font)},
  {PROP_STDNAME_TEXT_HEIGHT,PROP_STDTYPE_TEXT_HEIGHT,offsetof(Note,text),offsetof(Text,height)},
  {"text_colour",PROP_TYPE_COLOUR,offsetof(Note,text),offsetof(Text,color)},
  { PROP_STDNAME_LINE_WIDTH, PROP_STDTYPE_LINE_WIDTH, offsetof(Note, line_width)},
  {"line_colour",PROP_TYPE_COLOUR,offsetof(Note,line_color)},
  {"fill_colour",PROP_TYPE_COLOUR,offsetof(Note,fill_color)},
  { NULL, 0, 0 },
};

static void
note_get_props(Note * note, GPtrArray *props)
{
  object_get_props_from_offsets(&note->element.object,
                                note_offsets,props);
}

static void
note_set_props(Note *note, GPtrArray *props)
{
  object_set_props_from_offsets(&note->element.object,
                                note_offsets,props);
  note_update_data(note);
}

static real
note_distance_from(Note *note, Point *point)
{
  Element *elem = &note->element;
  real x = elem->corner.x;
  real y = elem->corner.y;
  real w = elem->width;
  real h = elem->height;
  Point pts[] = {
    { x, y },
    { x + w - NOTE_CORNER, y },
    { x + w, y + NOTE_CORNER },
    { x + w, y + h },
    { x, y + h }
  };
  /* not using the line_width parameter to the arrow on the line */
  return distance_polygon_point(pts, G_N_ELEMENTS(pts), 0.0, point);
}

static void
note_select(Note *note, Point *clicked_point,
	       DiaRenderer *interactive_renderer)
{
  text_set_cursor(note->text, clicked_point, interactive_renderer);
  text_grab_focus(note->text, &note->element.object);
  element_update_handles(&note->element);
}


static DiaObjectChange *
note_move_handle (Note             *note,
                  Handle           *handle,
                  Point            *to,
                  ConnectionPoint  *cp,
                  HandleMoveReason  reason,
                  ModifierKeys      modifiers)
{
  g_return_val_if_fail (note != NULL, NULL);
  g_return_val_if_fail (handle != NULL, NULL);
  g_return_val_if_fail (to != NULL, NULL);

  g_return_val_if_fail (handle->id < 8, NULL);

  return NULL;
}


static DiaObjectChange*
note_move(Note *note, Point *to)
{
  note->element.corner = *to;

  note_update_data(note);

  return NULL;
}


static void
note_draw (Note *note, DiaRenderer *renderer)
{
  Element *elem;
  double x, y, w, h;
  Point poly[5];

  g_return_if_fail (note != NULL);
  g_return_if_fail (renderer != NULL);

  elem = &note->element;

  x = elem->corner.x;
  y = elem->corner.y;
  w = elem->width;
  h = elem->height;

  dia_renderer_set_fillstyle (renderer, DIA_FILL_STYLE_SOLID);
  dia_renderer_set_linewidth (renderer, note->line_width);
  dia_renderer_set_linestyle (renderer, DIA_LINE_STYLE_SOLID, 0.0);

  poly[0].x = x;
  poly[0].y = y;
  poly[1].x = x+w-NOTE_CORNER;
  poly[1].y = y;
  poly[2].x = x+w;
  poly[2].y = y+NOTE_CORNER;
  poly[3].x = x+w;
  poly[3].y = y+h;
  poly[4].x = x;
  poly[4].y = y+h;

  dia_renderer_draw_polygon (renderer,
                             poly,
                             5,
                             &note->fill_color,
                             &note->line_color);

  poly[0] = poly[1];
  poly[1].x = x + w - NOTE_CORNER;
  poly[1].y = y + NOTE_CORNER;
  poly[2] = poly[2];

  dia_renderer_set_linewidth (renderer, note->line_width / 2);
  dia_renderer_draw_polyline (renderer,
                              poly,
                              3,
                              &note->line_color);

  text_draw (note->text, renderer);
}

static void
note_update_data(Note *note)
{
  Element *elem = &note->element;
  DiaObject *obj = &elem->object;
  Point p;

  text_calc_boundingbox(note->text, NULL);

  elem->width = note->text->max_width + NOTE_MARGIN_X + NOTE_CORNER;
  elem->height =
    note->text->height*note->text->numlines + NOTE_MARGIN_Y + NOTE_CORNER;

  p = elem->corner;
  p.x += note->line_width/2.0 + NOTE_MARGIN_X;
  p.y += note->line_width/2.0 + NOTE_CORNER + note->text->ascent;
  text_set_position(note->text, &p);

  /* Update connections: */
  element_update_connections_rectangle(elem, note->connections);

  element_update_boundingbox(elem);

  obj->position = elem->corner;
  element_update_handles(elem);
}

static DiaObject *
note_create(Point *startpoint,
	       void *user_data,
  	       Handle **handle1,
	       Handle **handle2)
{
  Note *note;
  Element *elem;
  DiaObject *obj;
  Point p;
  DiaFont *font;
  int i;

  note = g_new0 (Note, 1);
  elem = &note->element;
  obj = &elem->object;

  obj->type = &note_type;

  obj->ops = &note_ops;

  elem->corner = *startpoint;

  note->line_width = attributes_get_default_linewidth();
  note->line_color = attributes_get_foreground();
  note->fill_color = attributes_get_background();

  font = dia_font_new_from_style (DIA_FONT_MONOSPACE, 0.8);
  p = *startpoint;
  p.x += note->line_width/2.0 + NOTE_MARGIN_X;
  p.y += note->line_width/2.0 + NOTE_CORNER + dia_font_ascent("A",font, 0.8);

  note->text = new_text ("", font, 0.8, &p, &color_black, DIA_ALIGN_LEFT);
  g_clear_object (&font);

  element_init(elem, 8, NUM_CONNECTIONS);

  for (i=0;i<NUM_CONNECTIONS;i++) {
    obj->connections[i] = &note->connections[i];
    note->connections[i].object = obj;
    note->connections[i].connected = NULL;
  }
  note->connections[NUM_CONNECTIONS-1].flags = CP_FLAGS_MAIN;

  elem->extra_spacing.border_trans = note->line_width/2.0;
  note_update_data(note);

  for (i=0;i<8;i++) {
    obj->handles[i]->type = HANDLE_NON_MOVABLE;
  }

  *handle1 = NULL;
  *handle2 = NULL;
  return &note->element.object;
}

static void
note_destroy(Note *note)
{
  text_destroy(note->text);

  element_destroy(&note->element);
}

static DiaObject *
note_load(ObjectNode obj_node, int version,DiaContext *ctx)
{
  DiaObject * obj = object_load_using_properties(&note_type,
                                                 obj_node,version,ctx);
  AttributeNode attr;
  /* For compatibility with previous dia files. If no line_width, use
   * NOTE_BORDERWIDTH, that was the previous line width.
   */
  attr = object_find_attribute(obj_node, PROP_STDNAME_LINE_WIDTH);
  if (attr == NULL)
    ((Note*)obj)->line_width = NOTE_BORDERWIDTH;

  return obj;
}


