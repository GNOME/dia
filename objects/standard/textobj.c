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

#include "tool-icons.h"

#define HANDLE_TEXT HANDLE_CUSTOM1


typedef enum _Valign Valign;
enum _Valign {
        VALIGN_TOP,
        VALIGN_BOTTOM,
        VALIGN_CENTER,
        VALIGN_FIRST_LINE
};
typedef struct _Textobj Textobj;
struct _Textobj {
  DiaObject object;
  
  Handle text_handle;

  Text *text;
  TextAttributes attrs;
  Valign vert_align;
};

static struct _TextobjProperties {
  Alignment alignment;
  Valign vert_align;
} default_properties = { ALIGN_LEFT, VALIGN_FIRST_LINE } ;

static real textobj_distance_from(Textobj *textobj, Point *point);
static void textobj_select(Textobj *textobj, Point *clicked_point,
			   DiaRenderer *interactive_renderer);
static ObjectChange* textobj_move_handle(Textobj *textobj, Handle *handle,
					 Point *to, ConnectionPoint *cp,
					 HandleMoveReason reason, ModifierKeys modifiers);
static ObjectChange* textobj_move(Textobj *textobj, Point *to);
static void textobj_draw(Textobj *textobj, DiaRenderer *renderer);
static void textobj_update_data(Textobj *textobj);
static DiaObject *textobj_create(Point *startpoint,
			      void *user_data,
			      Handle **handle1,
			      Handle **handle2);
static void textobj_destroy(Textobj *textobj);

static PropDescription *textobj_describe_props(Textobj *textobj);
static void textobj_get_props(Textobj *textobj, GPtrArray *props);
static void textobj_set_props(Textobj *textobj, GPtrArray *props);

static void textobj_save(Textobj *textobj, ObjectNode obj_node,
			 const char *filename);
static DiaObject *textobj_load(ObjectNode obj_node, int version,
			    const char *filename);
static void textobj_valign_point(Textobj *textobj, Point* p, real factor);

static ObjectTypeOps textobj_type_ops =
{
  (CreateFunc) textobj_create,
  (LoadFunc)   textobj_load,
  (SaveFunc)   textobj_save,
  (GetDefaultsFunc)   NULL, 
  (ApplyDefaultsFunc) NULL
};

/* Version history:
 * Version 1 added vertical alignment, and needed old objects to use the
 *     right alignment.
 */

DiaObjectType textobj_type =
{
  "Standard - Text",   /* name */
  1,                   /* version */
  (char **) text_icon,  /* pixmap */

  &textobj_type_ops    /* ops */
};

DiaObjectType *_textobj_type = (DiaObjectType *) &textobj_type;

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

PropEnumData prop_text_vert_align_data[] = {
  { N_("Bottom"), VALIGN_BOTTOM },
  { N_("Top"), VALIGN_TOP },
  { N_("Center"), VALIGN_CENTER },
  { N_("First Line"), VALIGN_FIRST_LINE },
  { NULL, 0 }
};
static PropDescription textobj_props[] = {
  OBJECT_COMMON_PROPERTIES,
  PROP_STD_TEXT_ALIGNMENT,
  { "text_vert_alignment", PROP_TYPE_ENUM, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL, \
    N_("Vertical text alignment"), NULL, prop_text_vert_align_data },
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
  {PROP_STDNAME_TEXT_HEIGHT,PROP_STDTYPE_TEXT_HEIGHT,offsetof(Textobj,attrs.height)},
  {"text_colour",PROP_TYPE_COLOUR,offsetof(Textobj,attrs.color)},
  {"text_alignment",PROP_TYPE_ENUM,offsetof(Textobj,attrs.alignment)},
  {"text_vert_alignment",PROP_TYPE_ENUM,offsetof(Textobj,vert_align)},
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



static ObjectChange*
textobj_move_handle(Textobj *textobj, Handle *handle,
		    Point *to, ConnectionPoint *cp,
		    HandleMoveReason reason, ModifierKeys modifiers)
{
  assert(textobj!=NULL);
  assert(handle!=NULL);
  assert(to!=NULL);

  if (handle->id == HANDLE_TEXT) {
          /*Point to2 = *to;
          point_add(&to2,&textobj->text->position);
          point_sub(&to2,&textobj->text_handle.pos);
          textobj_move(textobj, &to2);*/
          textobj_move(textobj, to);
          
  }

  return NULL;
}

static ObjectChange*
textobj_move(Textobj *textobj, Point *to)
{
  textobj->object.position = *to;

  textobj_update_data(textobj);

  return NULL;
}

static void
textobj_draw(Textobj *textobj, DiaRenderer *renderer)
{
  assert(textobj != NULL);
  assert(renderer != NULL);

  text_draw(textobj->text, renderer);
}

static void textobj_valign_point(Textobj *textobj, Point* p, real factor)
        /* factor should be 1 or -1 */
{
    Rectangle *bb  = &(textobj->object.bounding_box); 
    real offset ;
    switch (textobj->vert_align){
        case VALIGN_BOTTOM:
            offset = bb->bottom - textobj->object.position.y;
            p->y -= offset * factor; 
            break;
        case VALIGN_TOP:
            offset = bb->top - textobj->object.position.y;
            p->y -= offset * factor; 
            break;
        case VALIGN_CENTER:
            offset = (bb->bottom + bb->top)/2 - textobj->object.position.y;
            p->y -= offset * factor; 
            break;
        case VALIGN_FIRST_LINE:
            break;
        }
}
static void
textobj_update_data(Textobj *textobj)
{
  Point to2;
  DiaObject *obj = &textobj->object;
  
  text_set_position(textobj->text, &obj->position);
  text_calc_boundingbox(textobj->text, &obj->bounding_box);

  to2 = obj->position;
  textobj_valign_point(textobj, &to2, 1);
  text_set_position(textobj->text, &to2);
  text_calc_boundingbox(textobj->text, &obj->bounding_box);
  
  textobj->text_handle.pos = obj->position;
}

static DiaObject *
textobj_create(Point *startpoint,
	       void *user_data,
	       Handle **handle1,
	       Handle **handle2)
{
  Textobj *textobj;
  DiaObject *obj;
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
  /* need to initialize to object.position as well, it is used update data */
  obj->position = *startpoint;

  text_get_attributes(textobj->text,&textobj->attrs);
  dia_font_unref(font);
  textobj->vert_align = default_properties.vert_align;
  
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
  data_add_enum(new_attribute(obj_node, "valign"),
		  textobj->vert_align);
}

static DiaObject *
textobj_load(ObjectNode obj_node, int version, const char *filename)
{
  Textobj *textobj;
  DiaObject *obj;
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

  attr = object_find_attribute(obj_node, "valign");
  if (attr != NULL)
    textobj->vert_align = data_enum( attribute_first_data(attr) );
  else if (version == 0) {
    textobj->vert_align = VALIGN_FIRST_LINE;
  }

  object_init(obj, 1, 0);

  obj->handles[0] = &textobj->text_handle;
  textobj->text_handle.id = HANDLE_TEXT;
  textobj->text_handle.type = HANDLE_MAJOR_CONTROL;
  textobj->text_handle.connect_type = HANDLE_CONNECTABLE;
  textobj->text_handle.connected_to = NULL;

  textobj_update_data(textobj);

  return &textobj->object;
}
