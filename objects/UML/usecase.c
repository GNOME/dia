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
#include <string.h>

#include "intl.h"
#include "object.h"
#include "element.h"
#include "render.h"
#include "attributes.h"
#include "text.h"
#include "properties.h"

#include "pixmaps/case.xpm"

typedef struct _UsecasePropertiesDialog UsecasePropertiesDialog;
typedef struct _Usecase Usecase;
typedef struct _UsecaseState UsecaseState;

struct _Usecase {
  Element element;

  ConnectionPoint connections[8];

  Text *text;
  int text_outside;
  int collaboration;
  TextAttributes attrs;
};


struct _UsecasePropertiesDialog {
  GtkWidget *dialog;
  
  GtkToggleButton *text_out;
  GtkToggleButton *collaboration;
};


#define USECASE_WIDTH 3.25
#define USECASE_HEIGHT 2
#define USECASE_MIN_RATIO 1.5
#define USECASE_MAX_RATIO 3
#define USECASE_LINEWIDTH 0.1
#define USECASE_MARGIN_Y 0.3

static real usecase_distance_from(Usecase *usecase, Point *point);
static void usecase_select(Usecase *usecase, Point *clicked_point,
			   Renderer *interactive_renderer);
static void usecase_move_handle(Usecase *usecase, Handle *handle,
				Point *to, HandleMoveReason reason, ModifierKeys modifiers);
static void usecase_move(Usecase *usecase, Point *to);
static void usecase_draw(Usecase *usecase, Renderer *renderer);
static Object *usecase_create(Point *startpoint,
			      void *user_data,
			      Handle **handle1,
			      Handle **handle2);
static void usecase_destroy(Usecase *usecase);
static Object *usecase_load(ObjectNode obj_node, int version,
			    const char *filename);
static void usecase_update_data(Usecase *usecase);
static PropDescription *usecase_describe_props(Usecase *usecase);
static void usecase_get_props(Usecase *usecase, GPtrArray *props);
static void usecase_set_props(Usecase *usecase, GPtrArray *props);

static ObjectTypeOps usecase_type_ops =
{
  (CreateFunc) usecase_create,
  (LoadFunc)   usecase_load,/*using_properties*/     /* load */
  (SaveFunc)   object_save_using_properties,      /* save */
  (GetDefaultsFunc)   NULL, 
  (ApplyDefaultsFunc) NULL
};

ObjectType usecase_type =
{
  "UML - Usecase",   /* name */
  0,                      /* version */
  (char **) case_xpm,  /* pixmap */
  
  &usecase_type_ops       /* ops */
};

static ObjectOps usecase_ops = {
  (DestroyFunc)         usecase_destroy,
  (DrawFunc)            usecase_draw,
  (DistanceFunc)        usecase_distance_from,
  (SelectFunc)          usecase_select,
  (CopyFunc)            object_copy_using_properties,
  (MoveFunc)            usecase_move,
  (MoveHandleFunc)      usecase_move_handle,
  (GetPropertiesFunc)   object_create_props_dialog,
  (ApplyPropertiesFunc) object_apply_props_from_dialog,
  (ObjectMenuFunc)      NULL,
  (DescribePropsFunc)   usecase_describe_props,
  (GetPropsFunc)        usecase_get_props,
  (SetPropsFunc)        usecase_set_props
};

static PropDescription usecase_props[] = {
  ELEMENT_COMMON_PROPERTIES,
  { "text_outside", PROP_TYPE_BOOL, PROP_FLAG_VISIBLE,
  N_("Text outside"), NULL, NULL },
  { "collaboration", PROP_TYPE_BOOL, PROP_FLAG_VISIBLE,
  N_("Collaboration"), NULL, NULL },
  PROP_STD_TEXT_FONT,
  PROP_STD_TEXT_HEIGHT,
  PROP_STD_TEXT_COLOUR,
  { "text", PROP_TYPE_TEXT, 0, N_("Text"), NULL, NULL }, 
  
  PROP_DESC_END
};

static PropDescription *
usecase_describe_props(Usecase *usecase)
{
  return usecase_props;
}

static PropOffset usecase_offsets[] = {
  ELEMENT_COMMON_PROPERTIES_OFFSETS,
  {"text_outside", PROP_TYPE_BOOL, offsetof(Usecase, text_outside) },
  {"collaboration", PROP_TYPE_BOOL, offsetof(Usecase, collaboration) },
  {"text",PROP_TYPE_TEXT,offsetof(Usecase,text)},
  {"text_font",PROP_TYPE_FONT,offsetof(Usecase,attrs.font)},
  {"text_height",PROP_TYPE_REAL,offsetof(Usecase,attrs.height)},
  {"text_colour",PROP_TYPE_COLOUR,offsetof(Usecase,attrs.color)},
  { NULL, 0, 0 },
};

static void
usecase_get_props(Usecase * usecase, GPtrArray *props)
{
  text_get_attributes(usecase->text,&usecase->attrs);
  object_get_props_from_offsets(&usecase->element.object,
                                usecase_offsets,props);
}

static void
usecase_set_props(Usecase *usecase, GPtrArray *props)
{
  object_set_props_from_offsets(&usecase->element.object,
                                usecase_offsets,props);
  apply_textattr_properties(props,usecase->text,"text",&usecase->attrs);
  usecase_update_data(usecase);
}

static real
usecase_distance_from(Usecase *usecase, Point *point)
{
  Object *obj = &usecase->element.object;
  return distance_rectangle_point(&obj->bounding_box, point);
}

static void
usecase_select(Usecase *usecase, Point *clicked_point,
	       Renderer *interactive_renderer)
{
  text_set_cursor(usecase->text, clicked_point, interactive_renderer);
  text_grab_focus(usecase->text, &usecase->element.object);
  element_update_handles(&usecase->element);
}

static void
usecase_move_handle(Usecase *usecase, Handle *handle,
		 Point *to, HandleMoveReason reason, ModifierKeys modifiers)
{
  assert(usecase!=NULL);
  assert(handle!=NULL);
  assert(to!=NULL);

  assert(handle->id < 8);
}

static void
usecase_move(Usecase *usecase, Point *to)
{
  real h;
  Point p;
  
  usecase->element.corner = *to;
  h = usecase->text->height*usecase->text->numlines;
  
  p = *to;
  p.x += usecase->element.width/2.0;
  if (usecase->text_outside) {
      p.y += usecase->element.height - h + usecase->text->ascent;
  } else {
      p.y += (usecase->element.height - h)/2.0 + usecase->text->ascent;
  }
  text_set_position(usecase->text, &p);
  usecase_update_data(usecase);
}

static void
usecase_draw(Usecase *usecase, Renderer *renderer)
{
  Element *elem;
  real x, y, w, h;
  Point c;

  assert(usecase != NULL);
  assert(renderer != NULL);

  elem = &usecase->element;

  x = elem->corner.x;
  y = elem->corner.y;

  if (usecase->text_outside) {
      w = USECASE_WIDTH;
      h = USECASE_HEIGHT;
      c.x = x + elem->width/2.0;
      c.y = y + USECASE_HEIGHT / 2.0;     
   } else {
      w = elem->width;
      h = elem->height;
      c.x = x + w/2.0;
      c.y = y + h/2.0;    
  }


  renderer->ops->set_fillstyle(renderer, FILLSTYLE_SOLID);
  renderer->ops->set_linewidth(renderer, USECASE_LINEWIDTH);

  if (usecase->collaboration)
	  renderer->ops->set_linestyle(renderer, LINESTYLE_DASHED);
  else 
	  renderer->ops->set_linestyle(renderer, LINESTYLE_SOLID);

  renderer->ops->fill_ellipse(renderer, 
			     &c,
			     w, h,
			     &color_white);
  renderer->ops->draw_ellipse(renderer, 
			     &c,
			     w, h,
			     &color_black);  
  
  text_draw(usecase->text, renderer);
}


static void
usecase_update_data(Usecase *usecase)
{
  real w, h, ratio;
  Point c, half, r,p;
  
  Element *elem = &usecase->element;
  Object *obj = &elem->object;
  
  text_calc_boundingbox(usecase->text, NULL);
  w = usecase->text->max_width;
  h = usecase->text->height*usecase->text->numlines;

  if (!usecase->text_outside) { 
      ratio = w/h;

      if (ratio > USECASE_MAX_RATIO) 
	  ratio = USECASE_MAX_RATIO;
  
      if (ratio < USECASE_MIN_RATIO) {
	  ratio = USECASE_MIN_RATIO;
	  r.y = w / ratio + h;
	  r.x = r.y * ratio;
      } else {
	  r.x = ratio*h + w;
	  r.y = r.x / ratio;
      }
      if (r.x < USECASE_WIDTH) 
	      r.x = USECASE_WIDTH;
      if (r.y < USECASE_HEIGHT)
	      r.y = USECASE_HEIGHT;
  } else {
      r.x = USECASE_WIDTH;
      r.y = USECASE_HEIGHT;
  }

  elem->width = r.x;
  elem->height = r.y;

  if (usecase->text_outside) { 
	  elem->width = MAX(elem->width, w);
	  elem->height += h + USECASE_MARGIN_Y;
  }

  r.x /= 2.0;
  r.y /= 2.0;
  c.x = elem->corner.x + elem->width / 2.0;
  c.y = elem->corner.y + r.y;
  half.x = r.x * M_SQRT1_2;
  half.y = r.y * M_SQRT1_2;

  /* Update connections: */
  usecase->connections[0].pos.x = c.x - half.x;
  usecase->connections[0].pos.y = c.y - half.y;
  usecase->connections[1].pos.x = c.x;
  usecase->connections[1].pos.y = elem->corner.y;
  usecase->connections[2].pos.x = c.x + half.x;
  usecase->connections[2].pos.y = c.y - half.y;
  usecase->connections[3].pos.x = c.x - r.x;
  usecase->connections[3].pos.y = c.y;
  usecase->connections[4].pos.x = c.x + r.x;
  usecase->connections[4].pos.y = c.y;
  
  if (usecase->text_outside) { 
      usecase->connections[5].pos.x = elem->corner.x;
      usecase->connections[5].pos.y = elem->corner.y + elem->height;
      usecase->connections[6].pos.x = c.x;
      usecase->connections[6].pos.y = elem->corner.y + elem->height;
      usecase->connections[7].pos.x = elem->corner.x + elem->width;
      usecase->connections[7].pos.y = elem->corner.y + elem->height;
  } else {
      usecase->connections[5].pos.x = c.x - half.x;
      usecase->connections[5].pos.y = c.y + half.y;
      usecase->connections[6].pos.x = c.x;
      usecase->connections[6].pos.y = elem->corner.y + elem->height;
      usecase->connections[7].pos.x = c.x + half.x;
      usecase->connections[7].pos.y = c.y + half.y;
  }

  h = usecase->text->height*usecase->text->numlines;
  p = usecase->element.corner;
  p.x += usecase->element.width/2.0;
  if (usecase->text_outside) {
      p.y += usecase->element.height - h + usecase->text->ascent;
  } else {
      p.y += (usecase->element.height - h)/2.0 + usecase->text->ascent;
  }
  text_set_position(usecase->text, &p);

  element_update_boundingbox(elem);

  obj->position = elem->corner;

  element_update_handles(elem);

}

static Object *
usecase_create(Point *startpoint,
	       void *user_data,
  	       Handle **handle1,
	       Handle **handle2)
{
  Usecase *usecase;
  Element *elem;
  Object *obj;
  Point p;
  DiaFont *font;
  int i;
  
  usecase = g_malloc0(sizeof(Usecase));
  elem = &usecase->element;
  obj = &elem->object;
  
  obj->type = &usecase_type;
  obj->ops = &usecase_ops;
  elem->corner = *startpoint;
  elem->width = USECASE_WIDTH;
  elem->height = USECASE_HEIGHT;

  /* choose default font name for your locale. see also font_data structure
     in lib/font.c. */
  font = font_getfont (_("Helvetica"));
  p = *startpoint;
  p.x += USECASE_WIDTH/2.0;
  p.y += USECASE_HEIGHT/2.0;
  
  usecase->text = new_text("", font, 0.8, &p, &color_black, ALIGN_CENTER);
  text_get_attributes(usecase->text,&usecase->attrs);
  usecase->text_outside = 0;
  usecase->collaboration = 0;
  element_init(elem, 8, 8);
  
  for (i=0;i<8;i++) {
    obj->connections[i] = &usecase->connections[i];
    usecase->connections[i].object = obj;
    usecase->connections[i].connected = NULL;
  }
  elem->extra_spacing.border_trans = 0.0;
  usecase_update_data(usecase);

  for (i=0;i<8;i++) {
    obj->handles[i]->type = HANDLE_NON_MOVABLE;
  }

  *handle1 = NULL;
  *handle2 = NULL;
  return &usecase->element.object;
}

static void
usecase_destroy(Usecase *usecase)
{
  text_destroy(usecase->text);

  element_destroy(&usecase->element);
}

static Object *
usecase_load(ObjectNode obj_node, int version, const char *filename)
{
  return object_load_using_properties(&usecase_type,
                                      obj_node,version,filename);
}


