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
#include <stdio.h>

#include "intl.h"
#include "object.h"
#include "element.h"
#include "render.h"
#include "attributes.h"
#include "text.h"
#include "properties.h"

#include "uml.h"
#include "stereotype.h"

#include "pixmaps/object.xpm"

typedef struct _Objet Objet;

struct _Objet {
  Element element;

  ConnectionPoint connections[8];
  
  char *stereotype;
  Text *text;
  char *exstate;  /* used for explicit state */
  Text *attributes;

  Point ex_pos, st_pos;
  int is_active;
  int show_attributes;
  int is_multiple;  
  
  char *attrib;
  
  char *st_stereotype;
};

#define OBJET_BORDERWIDTH 0.1
#define OBJET_ACTIVEBORDERWIDTH 0.2
#define OBJET_LINEWIDTH 0.05
#define OBJET_MARGIN_X 0.5
#define OBJET_MARGIN_Y 0.5
#define OBJET_MARGIN_M 0.4
#define OBJET_FONTHEIGHT 0.8

static real objet_distance_from(Objet *ob, Point *point);
static void objet_select(Objet *ob, Point *clicked_point,
			 Renderer *interactive_renderer);
static void objet_move_handle(Objet *ob, Handle *handle,
			      Point *to, HandleMoveReason reason, ModifierKeys modifiers);
static void objet_move(Objet *ob, Point *to);
static void objet_draw(Objet *ob, Renderer *renderer);
static Object *objet_create(Point *startpoint,
			    void *user_data,
			    Handle **handle1,
			    Handle **handle2);
static void objet_destroy(Objet *ob);
static Object *objet_load(ObjectNode obj_node, int version,
			  const char *filename);
static PropDescription *objet_describe_props(Objet *objet);
static void objet_get_props(Objet *objet, GPtrArray *props);
static void objet_set_props(Objet *objet, GPtrArray *props);
static void objet_update_data(Objet *ob);

static ObjectTypeOps objet_type_ops =
{
  (CreateFunc) objet_create,
  (LoadFunc)   objet_load,/*using_properties*/     /* load */
  (SaveFunc)   object_save_using_properties,      /* save */
  (GetDefaultsFunc)   NULL, 
  (ApplyDefaultsFunc) NULL
};

/* Non-nice typo, needed for backwards compatibility. */
ObjectType objet_type =
{
  "UML - Objet",   /* name */
  0,                      /* version */
  (char **) object_xpm,  /* pixmap */
  
  &objet_type_ops       /* ops */
};

ObjectType umlobject_type =
{
  "UML - Object",   /* name */
  0,                      /* version */
  (char **) object_xpm,  /* pixmap */
  
  &objet_type_ops       /* ops */
};

static ObjectOps objet_ops = {
  (DestroyFunc)         objet_destroy,
  (DrawFunc)            objet_draw,
  (DistanceFunc)        objet_distance_from,
  (SelectFunc)          objet_select,
  (CopyFunc)            object_copy_using_properties,
  (MoveFunc)            objet_move,
  (MoveHandleFunc)      objet_move_handle,
  (GetPropertiesFunc)   object_create_props_dialog,
  (ApplyPropertiesFunc) object_apply_props_from_dialog,
  (ObjectMenuFunc)      NULL,
  (DescribePropsFunc)   objet_describe_props,
  (GetPropsFunc)        objet_get_props,
  (SetPropsFunc)        objet_set_props,
};


static PropDescription objet_props[] = {
  ELEMENT_COMMON_PROPERTIES,
  /*PROP_STD_TEXT_ALIGNMENT,
  PROP_STD_TEXT_FONT,
  PROP_STD_TEXT_HEIGHT,
  PROP_STD_TEXT_COLOUR,*/
  { "text", PROP_TYPE_TEXT, 0, N_("Text"), NULL, NULL },
  { "stereotype", PROP_TYPE_STRING, PROP_FLAG_VISIBLE,
    N_("Stereotype"), NULL, NULL },
  { "exstate", PROP_TYPE_STRING, PROP_FLAG_VISIBLE,
    N_("Explicit state"),NULL, NULL },
  { "attribstr", PROP_TYPE_MULTISTRING, PROP_FLAG_VISIBLE|PROP_FLAG_DONT_SAVE,
    N_("Attributes"),NULL, GINT_TO_POINTER(6) },
  { "attrib", PROP_TYPE_TEXT, 0, NULL,NULL, NULL },
  { "is_active", PROP_TYPE_BOOL, PROP_FLAG_VISIBLE,
    N_("Active object"),NULL,NULL},
  { "show_attribs", PROP_TYPE_BOOL, PROP_FLAG_VISIBLE,
    N_("Show attributes"),NULL,NULL}, 
  { "multiple", PROP_TYPE_BOOL, PROP_FLAG_VISIBLE,
    N_("Multiple instance"),NULL,NULL}, 
  { NULL, 0, 0, NULL, NULL, NULL, 0}
};

static PropDescription *
objet_describe_props(Objet *ob)
{
  return objet_props;
}

static PropOffset objet_offsets[] = {
  ELEMENT_COMMON_PROPERTIES_OFFSETS,
  { "name", PROP_TYPE_STRING, offsetof(Objet, exstate) },
  { "stereotype", PROP_TYPE_STRING, offsetof(Objet, stereotype) },
  { "text", PROP_TYPE_TEXT, offsetof(Objet, text) },
  { "exstate", PROP_TYPE_STRING, offsetof(Objet, exstate) },
  { "attribstr", PROP_TYPE_MULTISTRING, offsetof(Objet, attrib)},
  { "attrib", PROP_TYPE_TEXT, offsetof(Objet, attributes)},
  { "is_active", PROP_TYPE_BOOL, offsetof(Objet,is_active)},
  { "show_attribs", PROP_TYPE_BOOL, offsetof(Objet, show_attributes)},
  { "multiple", PROP_TYPE_BOOL, offsetof(Objet, is_multiple)},
  { NULL, 0, 0 },
};

static void
objet_get_props(Objet * objet, GPtrArray *props)
{
  if (objet->attrib) g_free(objet->attrib);
  objet->attrib = text_get_string_copy(objet->attributes);

  object_get_props_from_offsets(&objet->element.object,
                                objet_offsets,props);
}

static void
objet_set_props(Objet *objet, GPtrArray *props)
{
  object_set_props_from_offsets(&objet->element.object,
                                objet_offsets,props);
  apply_textstr_properties(props,objet->attributes,"attrib",objet->attrib);
  g_free(objet->st_stereotype);
  objet->st_stereotype = NULL;
  objet_update_data(objet);
}

static real
objet_distance_from(Objet *ob, Point *point)
{
  Object *obj = &ob->element.object;
  return distance_rectangle_point(&obj->bounding_box, point);
}

static void
objet_select(Objet *ob, Point *clicked_point,
	       Renderer *interactive_renderer)
{
  text_set_cursor(ob->text, clicked_point, interactive_renderer);
  text_grab_focus(ob->text, &ob->element.object);
  element_update_handles(&ob->element);
}

static void
objet_move_handle(Objet *ob, Handle *handle,
			 Point *to, HandleMoveReason reason, ModifierKeys modifiers)
{
  assert(ob!=NULL);
  assert(handle!=NULL);
  assert(to!=NULL);

  assert(handle->id < 8);
}

static void
objet_move(Objet *ob, Point *to)
{
  ob->element.corner = *to;
  objet_update_data(ob);
}

static void
objet_draw(Objet *ob, Renderer *renderer)
{
  Element *elem;
  real bw, x, y, w, h;
  Point p1, p2;
  int i;
  
  assert(ob != NULL);
  assert(renderer != NULL);

  elem = &ob->element;

  x = elem->corner.x;
  y = elem->corner.y;
  w = elem->width;
  h = elem->height;
  
  bw = (ob->is_active) ? OBJET_ACTIVEBORDERWIDTH: OBJET_BORDERWIDTH;

  renderer->ops->set_fillstyle(renderer, FILLSTYLE_SOLID);
  renderer->ops->set_linewidth(renderer, bw);
  renderer->ops->set_linestyle(renderer, LINESTYLE_SOLID);


  p1.x = x; p1.y = y;
  p2.x = x+w; p2.y = y+h;

  if (ob->is_multiple) {
    p1.x += OBJET_MARGIN_M;
    p2.y -= OBJET_MARGIN_M;
    renderer->ops->fill_rect(renderer, 
			     &p1, &p2,
			     &color_white); 
    renderer->ops->draw_rect(renderer, 
			     &p1, &p2,
			     &color_black);
    p1.x -= OBJET_MARGIN_M;
    p1.y += OBJET_MARGIN_M;
    p2.x -= OBJET_MARGIN_M;
    p2.y += OBJET_MARGIN_M;
    y += OBJET_MARGIN_M;
  }
    
  renderer->ops->fill_rect(renderer, 
			   &p1, &p2,
			   &color_white);
  renderer->ops->draw_rect(renderer, 
			   &p1, &p2,
			   &color_black);

  
  text_draw(ob->text, renderer);

  if ((ob->st_stereotype != NULL) && (ob->st_stereotype[0] != '\0')) {
      renderer->ops->draw_string(renderer,
				 ob->st_stereotype,
				 &ob->st_pos, ALIGN_CENTER,
				 &color_black);
  }

  if ((ob->exstate != NULL) && (ob->exstate[0] != '\0')) {
      renderer->ops->draw_string(renderer,
				 ob->exstate,
				 &ob->ex_pos, ALIGN_CENTER,
				 &color_black);
  }

  /* Is there a better way to underline? */
  p1.x = x + (w - ob->text->max_width)/2;
  p1.y = ob->text->position.y + ob->text->descent;
  p2.x = p1.x + ob->text->max_width;
  p2.y = p1.y;
  
  renderer->ops->set_linewidth(renderer, OBJET_LINEWIDTH);
    
  for (i=0; i<ob->text->numlines; i++) { 
    p1.x = x + (w - ob->text->row_width[i])/2;
    p2.x = p1.x + ob->text->row_width[i];
    renderer->ops->draw_line(renderer,
			     &p1, &p2,
			     &color_black);
    p1.y = p2.y += ob->text->height;
  }

  if (ob->show_attributes) {
      p1.x = x; p2.x = x + w;
      p1.y = p2.y = ob->attributes->position.y - ob->attributes->ascent - OBJET_MARGIN_Y;
      
      renderer->ops->set_linewidth(renderer, bw);
      renderer->ops->draw_line(renderer,
			       &p1, &p2,
			       &color_black);

      text_draw(ob->attributes, renderer);
  }
}

static void
objet_update_data(Objet *ob)
{
  Element *elem = &ob->element;
  Object *obj = &elem->object;
  DiaFont *font;
  Point p1, p2;
  real h, w = 0;
  
  text_calc_boundingbox(ob->text, NULL);
  ob->stereotype = remove_stereotype_from_string(ob->stereotype);
  if (!ob->st_stereotype) {
    ob->st_stereotype =  string_to_stereotype(ob->stereotype);
  }

  font = ob->text->font;
  h = elem->corner.y + OBJET_MARGIN_Y;

  if (ob->is_multiple) {
    h += OBJET_MARGIN_M;
  }
    
  if ((ob->stereotype != NULL) && (ob->stereotype[0] != '\0')) {
      w = dia_font_string_width(ob->st_stereotype, font, OBJET_FONTHEIGHT);
      h += OBJET_FONTHEIGHT;
      ob->st_pos.y = h;
      h += OBJET_MARGIN_Y/2.0;
  }

  w = MAX(w, ob->text->max_width);
  p1.y = h + ob->text->ascent;  /* position of text */

  h += ob->text->height*ob->text->numlines;

  if ((ob->exstate != NULL) && (ob->exstate[0] != '\0')) {
      w = MAX(w, dia_font_string_width(ob->exstate, font, OBJET_FONTHEIGHT));
      h += OBJET_FONTHEIGHT;
      ob->ex_pos.y = h;
  }
  
  h += OBJET_MARGIN_Y;

  if (ob->show_attributes) {
      h += OBJET_MARGIN_Y + ob->attributes->ascent;
      p2.x = elem->corner.x + OBJET_MARGIN_X;
      p2.y = h;      
      text_set_position(ob->attributes, &p2);

      h += ob->attributes->height*ob->attributes->numlines; 

      w = MAX(w, ob->attributes->max_width);
  }

  w += 2*OBJET_MARGIN_X; 

  p1.x = elem->corner.x + w/2.0;
  text_set_position(ob->text, &p1);
  
  ob->ex_pos.x = ob->st_pos.x = p1.x;

  
  if (ob->is_multiple) {
    w += OBJET_MARGIN_M;
  }
    
  elem->width = w;
  elem->height = h - elem->corner.y;

  /* Update connections: */
  ob->connections[0].pos = elem->corner;
  ob->connections[1].pos.x = elem->corner.x + elem->width / 2.0;
  ob->connections[1].pos.y = elem->corner.y;
  ob->connections[2].pos.x = elem->corner.x + elem->width;
  ob->connections[2].pos.y = elem->corner.y;
  ob->connections[3].pos.x = elem->corner.x;
  ob->connections[3].pos.y = elem->corner.y + elem->height / 2.0;
  ob->connections[4].pos.x = elem->corner.x + elem->width;
  ob->connections[4].pos.y = elem->corner.y + elem->height / 2.0;
  ob->connections[5].pos.x = elem->corner.x;
  ob->connections[5].pos.y = elem->corner.y + elem->height;
  ob->connections[6].pos.x = elem->corner.x + elem->width / 2.0;
  ob->connections[6].pos.y = elem->corner.y + elem->height;
  ob->connections[7].pos.x = elem->corner.x + elem->width;
  ob->connections[7].pos.y = elem->corner.y + elem->height;
  
  element_update_boundingbox(elem);
  obj->position = elem->corner;
  element_update_handles(elem);
}

static Object *
objet_create(Point *startpoint,
		    void *user_data,
		    Handle **handle1,
		    Handle **handle2)
{
  Objet *ob;
  Element *elem;
  Object *obj;
  Point p;
  DiaFont *font;
  int i;
  
  ob = g_malloc0(sizeof(Objet));
  elem = &ob->element;
  obj = &elem->object;
  
  obj->type = &umlobject_type;

  obj->ops = &objet_ops;

  elem->corner = *startpoint;

  font = dia_font_new_from_style(DIA_FONT_SANS, OBJET_FONTHEIGHT);
  
  ob->show_attributes = FALSE;
  ob->is_active = FALSE;
  ob->is_multiple = FALSE;

  ob->exstate = NULL;
  ob->stereotype = NULL;
  ob->st_stereotype = NULL;

  /* The text position is recalculated later */
  p.x = 0.0;
  p.y = 0.0;
  ob->attributes = new_text("", font, 0.8, &p, &color_black, ALIGN_LEFT);
  ob->attrib = NULL;
  ob->text = new_text("", font, 0.8, &p, &color_black, ALIGN_CENTER);

  dia_font_unref(font);
  
  element_init(elem, 8, 8);
  
  for (i=0;i<8;i++) {
    obj->connections[i] = &ob->connections[i];
    ob->connections[i].object = obj;
    ob->connections[i].connected = NULL;
  }
  elem->extra_spacing.border_trans = OBJET_BORDERWIDTH/2.0;
  objet_update_data(ob);

  for (i=0;i<8;i++) {
    obj->handles[i]->type = HANDLE_NON_MOVABLE;
  }

  *handle1 = NULL;
  *handle2 = NULL;

  return &ob->element.object;
}

static void
objet_destroy(Objet *ob)
{
  text_destroy(ob->text);
  text_destroy(ob->attributes);

  g_free(ob->stereotype);
  g_free(ob->st_stereotype);
  g_free(ob->exstate);
  g_free(ob->attrib);

  element_destroy(&ob->element);
}

static Object *
objet_load(ObjectNode obj_node, int version, const char *filename)
{
  return object_load_using_properties(&objet_type,
                                      obj_node,version,filename);
}

