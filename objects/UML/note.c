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
#include <assert.h>
#include <gtk/gtk.h>
#include <math.h>
#include <string.h>

#include "config.h"
#include "intl.h"
#include "object.h"
#include "element.h"
#include "render.h"
#include "attributes.h"
#include "text.h"
#include "properties.h"

#include "pixmaps/note.xpm"

typedef struct _Note Note;
struct _Note {
  Element element;

  ConnectionPoint connections[8];

  Text *text;
};

#define NOTE_BORDERWIDTH 0.1
#define NOTE_CORNERWIDTH 0.05
#define NOTE_CORNER 0.6
#define NOTE_MARGIN_X 0.3
#define NOTE_MARGIN_Y 0.3

static real note_distance_from(Note *note, Point *point);
static void note_select(Note *note, Point *clicked_point,
			Renderer *interactive_renderer);
static void note_move_handle(Note *note, Handle *handle,
			     Point *to, HandleMoveReason reason, ModifierKeys modifiers);
static void note_move(Note *note, Point *to);
static void note_draw(Note *note, Renderer *renderer);
static Object *note_create(Point *startpoint,
			   void *user_data,
			   Handle **handle1,
			   Handle **handle2);
static void note_destroy(Note *note);
static Object *note_copy(Note *note);

static void note_save(Note *note, ObjectNode obj_node,
		      const char *filename);
static Object *note_load(ObjectNode obj_node, int version,
			 const char *filename);

static PropDescription *note_describe_props(Note *note);
static void note_get_props(Note *note, Property *props, guint nprops);
static void note_set_props(Note *note, Property *props, guint nprops);

static void note_update_data(Note *note);

static ObjectTypeOps note_type_ops =
{
  (CreateFunc) note_create,
  (LoadFunc)   note_load,
  (SaveFunc)   note_save
};

ObjectType note_type =
{
  "UML - Note",   /* name */
  0,                      /* version */
  (char **) note_xpm,  /* pixmap */
  
  &note_type_ops       /* ops */
};

static ObjectOps note_ops = {
  (DestroyFunc)         note_destroy,
  (DrawFunc)            note_draw,
  (DistanceFunc)        note_distance_from,
  (SelectFunc)          note_select,
  (CopyFunc)            note_copy,
  (MoveFunc)            note_move,
  (MoveHandleFunc)      note_move_handle,
  (GetPropertiesFunc)   object_return_null,
  (ApplyPropertiesFunc) object_return_void,
  (ObjectMenuFunc)      NULL,
  (DescribePropsFunc)   note_describe_props,
  (GetPropsFunc)        note_get_props,
  (SetPropsFunc)        note_set_props
};

static PropDescription note_props[] = {
  ELEMENT_COMMON_PROPERTIES,
  PROP_STD_TEXT_FONT,
  PROP_STD_TEXT_HEIGHT,
  PROP_STD_TEXT_COLOUR,
  PROP_STD_TEXT,
  
  PROP_DESC_END
};

static PropDescription *
note_describe_props(Note *note)
{
  if (note_props[0].quark == 0)
    prop_desc_list_calculate_quarks(note_props);
  return note_props;
}

static PropOffset note_offsets[] = {
  ELEMENT_COMMON_PROPERTIES_OFFSETS,
  { NULL, 0, 0 },
};

static struct { const gchar *name; GQuark q; } quarks[] = {
  { "text_font" },
  { "text_height" },
  { "text_colour" },
  { "text" }
};

static void
note_get_props(Note * note, Property *props, guint nprops)
{
  guint i;

  if (object_get_props_from_offsets((Object *)note, note_offsets, props, nprops))
    return;
  /* these props can't be handled as easily */
  if (quarks[0].q == 0)
    for (i = 0; i < 4; i++)
      quarks[i].q = g_quark_from_static_string(quarks[i].name);
  for (i = 0; i < nprops; i++) {
    GQuark pquark = g_quark_from_string(props[i].name);

    if (pquark == quarks[0].q) {
      props[i].type = PROP_TYPE_FONT;
      PROP_VALUE_FONT(props[i]) = note->text->font;
    } else if (pquark == quarks[1].q) {
      props[i].type = PROP_TYPE_REAL;
      PROP_VALUE_REAL(props[i]) = note->text->height;
    } else if (pquark == quarks[2].q) {
      props[i].type = PROP_TYPE_COLOUR;
      PROP_VALUE_COLOUR(props[i]) = note->text->color;
    } else if (pquark == quarks[3].q) {
      props[i].type = PROP_TYPE_STRING;
      g_free(PROP_VALUE_STRING(props[i]));
      PROP_VALUE_STRING(props[i]) = text_get_string_copy(note->text);
    }
  }
}

static void
note_set_props(Note *note, Property *props, guint nprops)
{
  if (!object_set_props_from_offsets((Object *)note, note_offsets,
		     props, nprops)) {
    guint i;

    if (quarks[0].q == 0)
      for (i = 0; i < 4; i++)
	quarks[i].q = g_quark_from_static_string(quarks[i].name);

    for (i = 0; i < nprops; i++) {
      GQuark pquark = g_quark_from_string(props[i].name);

      if (pquark == quarks[0].q && props[i].type == PROP_TYPE_FONT) {
	text_set_font(note->text, PROP_VALUE_FONT(props[i]));
      } else if (pquark == quarks[1].q && props[i].type == PROP_TYPE_REAL) {
	text_set_height(note->text, PROP_VALUE_REAL(props[i]));
      } else if (pquark == quarks[2].q && props[i].type == PROP_TYPE_COLOUR) {
	text_set_color(note->text, &PROP_VALUE_COLOUR(props[i]));
      } else if (pquark == quarks[3].q && props[i].type == PROP_TYPE_STRING) {
	text_set_string(note->text, PROP_VALUE_STRING(props[i]));
      }
    }
  }
  note_update_data(note);
}

static real
note_distance_from(Note *note, Point *point)
{
  Object *obj = &note->element.object;
  return distance_rectangle_point(&obj->bounding_box, point);
}

static void
note_select(Note *note, Point *clicked_point,
	       Renderer *interactive_renderer)
{
  text_set_cursor(note->text, clicked_point, interactive_renderer);
  text_grab_focus(note->text, (Object *)note);
  element_update_handles(&note->element);
}

static void
note_move_handle(Note *note, Handle *handle,
		 Point *to, HandleMoveReason reason, ModifierKeys modifiers)
{
  assert(note!=NULL);
  assert(handle!=NULL);
  assert(to!=NULL);

  assert(handle->id < 8);
}

static void
note_move(Note *note, Point *to)
{
  Point p;
  
  note->element.corner = *to;

  p = *to;
  p.x += NOTE_BORDERWIDTH/2.0 + NOTE_MARGIN_X;
  p.y += NOTE_BORDERWIDTH/2.0 + NOTE_CORNER + note->text->ascent;
  text_set_position(note->text, &p);
  note_update_data(note);
}

static void
note_draw(Note *note, Renderer *renderer)
{
  Element *elem;
  real x, y, w, h;
  Point poly[5];
  
  assert(note != NULL);
  assert(renderer != NULL);

  elem = &note->element;

  x = elem->corner.x;
  y = elem->corner.y;
  w = elem->width;
  h = elem->height;
  
  renderer->ops->set_fillstyle(renderer, FILLSTYLE_SOLID);
  renderer->ops->set_linewidth(renderer, NOTE_BORDERWIDTH);
  renderer->ops->set_linestyle(renderer, LINESTYLE_SOLID);

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

  renderer->ops->fill_polygon(renderer, 
			      poly, 5,
			      &color_white);
  renderer->ops->draw_polygon(renderer, 
			      poly, 5,
			      &color_black);

  poly[0] = poly[1];
  poly[1].x = x + w - NOTE_CORNER;
  poly[1].y = y + NOTE_CORNER;
  poly[2] = poly[2];
 
  renderer->ops->set_linewidth(renderer, NOTE_CORNERWIDTH);
  renderer->ops->draw_polyline(renderer, 
			   poly, 3,
			   &color_black);

  text_draw(note->text, renderer);
}

static void
note_update_data(Note *note)
{
  Element *elem = &note->element;
  Object *obj = (Object *) note;
  
  elem->width = note->text->max_width + NOTE_MARGIN_X + NOTE_CORNER;
  elem->height =
    note->text->height*note->text->numlines + NOTE_MARGIN_Y + NOTE_CORNER;

  /* Update connections: */
  note->connections[0].pos = elem->corner;
  note->connections[1].pos.x = elem->corner.x + elem->width / 2.0;
  note->connections[1].pos.y = elem->corner.y;
  note->connections[2].pos.x = elem->corner.x + elem->width;
  note->connections[2].pos.y = elem->corner.y;
  note->connections[3].pos.x = elem->corner.x;
  note->connections[3].pos.y = elem->corner.y + elem->height / 2.0;
  note->connections[4].pos.x = elem->corner.x + elem->width;
  note->connections[4].pos.y = elem->corner.y + elem->height / 2.0;
  note->connections[5].pos.x = elem->corner.x;
  note->connections[5].pos.y = elem->corner.y + elem->height;
  note->connections[6].pos.x = elem->corner.x + elem->width / 2.0;
  note->connections[6].pos.y = elem->corner.y + elem->height;
  note->connections[7].pos.x = elem->corner.x + elem->width;
  note->connections[7].pos.y = elem->corner.y + elem->height;
  
  element_update_boundingbox(elem);
  /* fix boundingnote for line_width: */
  obj->bounding_box.top -= NOTE_BORDERWIDTH/2.0;
  obj->bounding_box.left -= NOTE_BORDERWIDTH/2.0;
  obj->bounding_box.bottom += NOTE_BORDERWIDTH/2.0;
  obj->bounding_box.right += NOTE_BORDERWIDTH/2.0;

  obj->position = elem->corner;

  element_update_handles(elem);
}

static Object *
note_create(Point *startpoint,
	       void *user_data,
  	       Handle **handle1,
	       Handle **handle2)
{
  Note *note;
  Element *elem;
  Object *obj;
  Point p;
  Font *font;
  int i;
  
  note = g_malloc(sizeof(Note));
  elem = &note->element;
  obj = (Object *) note;
  
  obj->type = &note_type;

  obj->ops = &note_ops;

  elem->corner = *startpoint;

  font = font_getfont("Courier");
  p = *startpoint;
  p.x += NOTE_BORDERWIDTH/2.0 + NOTE_MARGIN_X;
  p.y += NOTE_BORDERWIDTH/2.0 + NOTE_CORNER + font_ascent(font, 0.8);
  
  note->text = new_text("", font, 0.8, &p, &color_black, ALIGN_LEFT);
  
  element_init(elem, 8, 8);
  
  for (i=0;i<8;i++) {
    obj->connections[i] = &note->connections[i];
    note->connections[i].object = obj;
    note->connections[i].connected = NULL;
  }
  note_update_data(note);

  for (i=0;i<8;i++) {
    obj->handles[i]->type = HANDLE_NON_MOVABLE;
  }

  *handle1 = NULL;
  *handle2 = NULL;
  return (Object *)note;
}

static void
note_destroy(Note *note)
{
  text_destroy(note->text);

  element_destroy(&note->element);
}

static Object *
note_copy(Note *note)
{
  int i;
  Note *newnote;
  Element *elem, *newelem;
  Object *newobj;
  
  elem = &note->element;
  
  newnote = g_malloc(sizeof(Note));
  newelem = &newnote->element;
  newobj = (Object *) newnote;

  element_copy(elem, newelem);

  newnote->text = text_copy(note->text);
  
  for (i=0;i<8;i++) {
    newobj->connections[i] = &newnote->connections[i];
    newnote->connections[i].object = newobj;
    newnote->connections[i].connected = NULL;
    newnote->connections[i].pos = note->connections[i].pos;
    newnote->connections[i].last_pos = note->connections[i].last_pos;
  }

  note_update_data(newnote);
  
  return (Object *)newnote;
}


static void
note_save(Note *note, ObjectNode obj_node, const char *filename)
{
  element_save(&note->element, obj_node);

  data_add_text(new_attribute(obj_node, "text"),
		note->text);
}

static Object *
note_load(ObjectNode obj_node, int version, const char *filename)
{
  Note *note;
  AttributeNode attr;
  Element *elem;
  Object *obj;
  int i;
  
  note = g_malloc(sizeof(Note));
  elem = &note->element;
  obj = (Object *) note;
  
  obj->type = &note_type;
  obj->ops = &note_ops;

  element_load(elem, obj_node);
  
  note->text = NULL;
  attr = object_find_attribute(obj_node, "text");
  if (attr != NULL)
    note->text = data_text(attribute_first_data(attr));

  element_init(elem, 8, 8);

  for (i=0;i<8;i++) {
    obj->connections[i] = &note->connections[i];
    note->connections[i].object = obj;
    note->connections[i].connected = NULL;
  }
  note_update_data(note);

  for (i=0;i<8;i++) {
    obj->handles[i]->type = HANDLE_NON_MOVABLE;
  }

  return (Object *)note;
}




