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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <math.h>

#include "intl.h"
#include "object.h"
#include "connectionpoint.h"
#include "diarenderer.h"
#include "font.h"
#include "text.h"
#include "attributes.h"
#include "widgets.h"
#include "properties.h"

#include "pixmaps/text.xpm"

#define HANDLE_TEXT HANDLE_CUSTOM1

typedef struct _Textobj Textobj;
struct _Textobj {
  Object object;
  
  Handle text_handle;

  Text *text;
  TextAttributes attrs;
};

static struct _TextobjProperties {
  Alignment alignment;
} default_properties = { ALIGN_LEFT } ;

static real textobj_distance_from(Textobj *textobj, Point *point);
static void textobj_select(Textobj *textobj, Point *clicked_point,
			   DiaRenderer *interactive_renderer);
static void textobj_move_handle(Textobj *textobj, Handle *handle,
				Point *to, HandleMoveReason reason, ModifierKeys modifiers);
static void textobj_move(Textobj *textobj, Point *to);
static void textobj_draw(Textobj *textobj, DiaRenderer *renderer);
static void textobj_update_data(Textobj *textobj);
static Object *textobj_create(Point *startpoint,
			      void *user_data,
			      Handle **handle1,
			      Handle **handle2);
static void textobj_destroy(Textobj *textobj);

static PropDescription *textobj_describe_props(Textobj *textobj);
static void textobj_get_props(Textobj *textobj, GPtrArray *props);
static void textobj_set_props(Textobj *textobj, GPtrArray *props);

static void textobj_save(Textobj *textobj, ObjectNode obj_node,
			 const char *filename);
static Object *textobj_load(ObjectNode obj_node, int version,
			    const char *filename);

static ObjectTypeOps textobj_type_ops =
{
  (CreateFunc) textobj_create,
  (LoadFunc)   textobj_load,
  (SaveFunc)   textobj_save,
  (GetDefaultsFunc)   NULL, 
  (ApplyDefaultsFunc) NULL
};

ObjectType textobj_type =
{
  "Standard - Text",   /* name */
  0,                   /* version */
  (char **) text_xpm,  /* pixmap */

  &textobj_type_ops    /* ops */
};

ObjectType *_textobj_type = (ObjectType *) &textobj_type;

static ObjectOps textobj_ops = {
  (DestroyFunc)         textobj_destroy,
  (DrawFunc)            textobj_draw,
  (DistanceFunc)        textobj_distance_from,
  (SelectFunc)          textobj_select,
  (CopyFunc)            object_copy_using_properties,
  (MoveFunc)            textobj_move,
  (MoveHandleFunc)      textobj_move_handle,
  (GetPropertiesFunc)   object_create_props_dialog,
  (ApplyPropertiesFunc) object_apply_props_from_dialog,
  (ObjectMenuFunc)      NULL,
  (DescribePropsFunc)   textobj_describe_props,
  (GetPropsFunc)        textobj_get_props,
  (SetPropsFunc)        textobj_set_props,
};

static PropDescription textobj_props[] = {
  OBJECT_COMMON_PROPERTIES,
  PROP_STD_TEXT_ALIGNMENT,
  PROP_STD_TEXT_FONT,
  PROP_STD_TEXT_HEIGHT,
  PROP_STD_TEXT_COLOUR,
  PROP_STD_SAVED_TEXT,
  PROP_DESC_END
};

static PropDescription *
textobj_describe_props(Textobj *textobj)
{
  if (textobj_props[0].quark == 0)
    prop_desc_list_calculate_quarks(textobj_props);
  return textobj_props;
}

static PropOffset textobj_offsets[] = {
  OBJECT_COMMON_PROPERTIES_OFFSETS,
  {"text",PROP_TYPE_TEXT,offsetof(Textobj,text)},
  {"text_font",PROP_TYPE_FONT,offsetof(Textobj,attrs.font)},
  {"text_height",PROP_TYPE_REAL,offsetof(Textobj,attrs.height)},
  {"text_colour",PROP_TYPE_COLOUR,offsetof(Textobj,attrs.color)},
  {"text_alignment",PROP_TYPE_ENUM,offsetof(Textobj,attrs.alignment)},
  { NULL, 0, 0 }
};

static void
textobj_get_props(Textobj *textobj, GPtrArray *props)
{
  text_get_attributes(textobj->text,&textobj->attrs);
  object_get_props_from_offsets(&textobj->object,textobj_offsets,props);
}

static void
textobj_set_props(Textobj *textobj, GPtrArray *props)
{
  object_set_props_from_offsets(&textobj->object,textobj_offsets,props);
  apply_textattr_properties(props,textobj->text,"text",&textobj->attrs);
  textobj_update_data(textobj);
}

static real
textobj_distance_from(Textobj *textobj, Point *point)
{
  return text_distance_from(textobj->text, point); 
}

static void
textobj_select(Textobj *textobj, Point *clicked_point,
	       DiaRenderer *interactive_renderer)
{
  text_set_cursor(textobj->text, clicked_point, interactive_renderer);
  text_grab_focus(textobj->text, &textobj->object);
}

static void
textobj_move_handle(Textobj *textobj, Handle *handle,
		    Point *to, HandleMoveReason reason, ModifierKeys modifiers)
{
  assert(textobj!=NULL);
  assert(handle!=NULL);
  assert(to!=NULL);

  if (handle->id == HANDLE_TEXT) {
    text_set_position(textobj->text, to);
  }
  
  textobj_update_data(textobj);
}

static void
textobj_move(Textobj *textobj, Point *to)
{
  text_set_position(textobj->text, to);
  
  textobj_update_data(textobj);
}

static void
textobj_draw(Textobj *textobj, DiaRenderer *renderer)
{
  assert(textobj != NULL);
  assert(renderer != NULL);

  text_draw(textobj->text, renderer);
}

static void
textobj_update_data(Textobj *textobj)
{
  Object *obj = &textobj->object;
  
  obj->position = textobj->text->position;
  
  text_calc_boundingbox(textobj->text, &obj->bounding_box);
  
  textobj->text_handle.pos = textobj->text->position;
}

static Object *
textobj_create(Point *startpoint,
	       void *user_data,
	       Handle **handle1,
	       Handle **handle2)
{
  Textobj *textobj;
  Object *obj;
  Color col;
  DiaFont *font = NULL;
  real font_height;
  
  textobj = g_malloc0(sizeof(Textobj));
  obj = &textobj->object;
  
  obj->type = &textobj_type;

  obj->ops = &textobj_ops;

  col = attributes_get_foreground();
  attributes_get_default_font(&font, &font_height);
  textobj->text = new_text("", font, font_height,
			   startpoint, &col, default_properties.alignment );
  text_get_attributes(textobj->text,&textobj->attrs);
  dia_font_unref(font);
  
  object_init(obj, 1, 0);

  obj->handles[0] = &textobj->text_handle;
  textobj->text_handle.id = HANDLE_TEXT;
  textobj->text_handle.type = HANDLE_MAJOR_CONTROL;
  textobj->text_handle.connect_type = HANDLE_CONNECTABLE;
  textobj->text_handle.connected_to = NULL;

  textobj_update_data(textobj);

  *handle1 = NULL;
  *handle2 = obj->handles[0];
  return &textobj->object;
}

static void
textobj_destroy(Textobj *textobj)
{
  text_destroy(textobj->text);
  object_destroy(&textobj->object);
}

static void
textobj_save(Textobj *textobj, ObjectNode obj_node, const char *filename)
{
  object_save(&textobj->object, obj_node);

  data_add_text(new_attribute(obj_node, "text"),
		textobj->text);
}

static Object *
textobj_load(ObjectNode obj_node, int version, const char *filename)
{
  Textobj *textobj;
  Object *obj;
  AttributeNode attr;
  Point startpoint = {0.0, 0.0};

  textobj = g_malloc0(sizeof(Textobj));
  obj = &textobj->object;
  
  obj->type = &textobj_type;
  obj->ops = &textobj_ops;

  object_load(obj, obj_node);

  attr = object_find_attribute(obj_node, "text");
  if (attr != NULL) {
	  textobj->text = data_text( attribute_first_data(attr) );
  } else {
      DiaFont* font = dia_font_new_from_style(DIA_FONT_MONOSPACE,1.0);
      textobj->text = new_text("", font, 1.0,
                               &startpoint, &color_black, ALIGN_CENTER);
      dia_font_unref(font);
  }

  object_init(obj, 1, 0);

  obj->handles[0] = &textobj->text_handle;
  textobj->text_handle.id = HANDLE_TEXT;
  textobj->text_handle.type = HANDLE_MINOR_CONTROL;
  textobj->text_handle.connect_type = HANDLE_CONNECTABLE;
  textobj->text_handle.connected_to = NULL;

  textobj_update_data(textobj);

  return &textobj->object;
}
