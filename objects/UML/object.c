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
#include <gtk/gtk.h>
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

#include "pixmaps/object.xpm"

typedef struct _Objet Objet;
typedef struct _ObjetState ObjetState;
typedef struct _ObjetPropertiesDialog ObjetPropertiesDialog;

struct _ObjetState {
  ObjectState obj_state;

  char *stereotype;
  char *exstate;  /* used for explicit state */
  char *attributes;
  
  int is_active;
  int show_attributes;
  int is_multiple;  
};
 
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
};

struct _ObjetPropertiesDialog {
  GtkWidget *dialog;
  
  GtkEntry *name;
  GtkEntry *stereotype;
  GtkWidget *attribs;
  GtkToggleButton *show_attrib;
  GtkToggleButton *active;
  GtkToggleButton *multiple;
};

#define OBJET_BORDERWIDTH 0.1
#define OBJET_ACTIVEBORDERWIDTH 0.2
#define OBJET_LINEWIDTH 0.05
#define OBJET_MARGIN_X 0.5
#define OBJET_MARGIN_Y 0.5
#define OBJET_MARGIN_M 0.4
#define OBJET_FONTHEIGHT 0.8

static ObjetPropertiesDialog* properties_dialog = NULL;

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
static Object *objet_copy(Objet *ob);
static void objet_save(Objet *ob, ObjectNode obj_node,
		       const char *filename);
static Object *objet_load(ObjectNode obj_node, int version,
			  const char *filename);
static PropDescription *objet_describe_props(Objet *objet);
static void objet_get_props(Objet *objet, Property *props, guint nprops);
static void objet_set_props(Objet *objet, Property *props, guint nprops);
static void objet_update_data(Objet *ob);
static GtkWidget *objet_get_properties(Objet *ob);
static ObjectChange *objet_apply_properties(Objet *ob);

static ObjetState *objet_get_state(Objet *ob);
static void objet_set_state(Objet *ob,
			    ObjetState *state);

static ObjectTypeOps objet_type_ops =
{
  (CreateFunc) objet_create,
  (LoadFunc)   objet_load,
  (SaveFunc)   objet_save
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
  (CopyFunc)            objet_copy,
  (MoveFunc)            objet_move,
  (MoveHandleFunc)      objet_move_handle,
  (GetPropertiesFunc)   objet_get_properties,
  (ApplyPropertiesFunc) objet_apply_properties,
  (ObjectMenuFunc)      NULL,
  (DescribePropsFunc)   objet_describe_props,
  (GetPropsFunc)        objet_get_props,
  (SetPropsFunc)        objet_set_props,
};


static PropDescription objet_props[] = {
  ELEMENT_COMMON_PROPERTIES,
  PROP_STD_TEXT_ALIGNMENT,
  PROP_STD_TEXT_FONT,
  PROP_STD_TEXT_HEIGHT,
  PROP_STD_TEXT_COLOUR,
  PROP_STD_TEXT,
  { "stereotype", PROP_TYPE_STRING, PROP_FLAG_VISIBLE,
    N_("Stereotype"), NULL, NULL },
  { "name", PROP_TYPE_STRING, PROP_FLAG_VISIBLE,
    N_("Class name"), NULL, NULL },

  /*
   * XXX Lists: "attributes" "operations" "templates"
   */

  { NULL, 0, 0, NULL, NULL, NULL, 0}
};

static PropDescription *
objet_describe_props(Objet *ob)
{
  if (objet_props[0].quark == 0)
    prop_desc_list_calculate_quarks(objet_props);
  return objet_props;
}

static PropOffset objet_offsets[] = {
  ELEMENT_COMMON_PROPERTIES_OFFSETS,
  { "name", PROP_TYPE_STRING, offsetof(Objet, exstate) },
  { "stereotype", PROP_TYPE_STRING, offsetof(Objet, stereotype) },
  { NULL, 0, 0 },
};

static struct { const gchar *name; GQuark q; } quarks[] = {
  { "text_alignment" },
  { "text_font" },
  { "text_height" },
  { "text_colour" },
  { "text" }
};

static void
objet_get_props(Objet * objet, Property *props, guint nprops)
{
  guint i;

  if (object_get_props_from_offsets(&objet->element.object, 
                                    objet_offsets, props, nprops))
    return;

  if (quarks[0].q == 0)
    for (i = 0; i < sizeof(quarks)/sizeof(*quarks); i++)
      quarks[i].q = g_quark_from_static_string(quarks[i].name);
 
  for (i = 0; i < nprops; i++) {
    GQuark pquark = g_quark_from_string(props[i].name);

    if (pquark == quarks[0].q) {
      props[i].type = PROP_TYPE_ENUM;
      PROP_VALUE_ENUM(props[i]) = objet->text->alignment;
    } else if (pquark == quarks[1].q) {
      props[i].type = PROP_TYPE_FONT;
      PROP_VALUE_FONT(props[i]) = objet->text->font;
    } else if (pquark == quarks[2].q) {
      props[i].type = PROP_TYPE_REAL;
      PROP_VALUE_REAL(props[i]) = objet->text->height;
    } else if (pquark == quarks[3].q) {
      props[i].type = PROP_TYPE_COLOUR;
      PROP_VALUE_COLOUR(props[i]) = objet->text->color;
    } else if (pquark == quarks[4].q) {
      props[i].type = PROP_TYPE_STRING;
      g_free(PROP_VALUE_STRING(props[i]));
      PROP_VALUE_STRING(props[i]) = text_get_string_copy(objet->text);
    }
  }

}

static void
objet_set_props(Objet *objet, Property *props, guint nprops)
{
  if (!object_set_props_from_offsets(&objet->element.object, objet_offsets,
		     props, nprops)) {
    guint i;

    if (quarks[0].q == 0)
      for (i = 0; i < sizeof(quarks)/sizeof(*quarks); i++)
        quarks[i].q = g_quark_from_static_string(quarks[i].name);

    for (i = 0; i < nprops; i++) {
      GQuark pquark = g_quark_from_string(props[i].name);

      if (pquark == quarks[0].q && props[i].type == PROP_TYPE_ENUM) {
	text_set_alignment(objet->text, PROP_VALUE_ENUM(props[i]));
      } else if (pquark == quarks[1].q && props[i].type == PROP_TYPE_FONT) {
        text_set_font(objet->text, PROP_VALUE_FONT(props[i]));
      } else if (pquark == quarks[2].q && props[i].type == PROP_TYPE_REAL) {
        text_set_height(objet->text, PROP_VALUE_REAL(props[i]));
      } else if (pquark == quarks[3].q && props[i].type == PROP_TYPE_COLOUR) {
        text_set_color(objet->text, &PROP_VALUE_COLOUR(props[i]));
      } else if (pquark == quarks[4].q && props[i].type == PROP_TYPE_STRING) {
        text_set_string(objet->text, PROP_VALUE_STRING(props[i]));
      }
    }
  }
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

  if (ob->stereotype != NULL) {
      renderer->ops->draw_string(renderer,
				 ob->stereotype,
				 &ob->st_pos, ALIGN_CENTER,
				 &color_black);
  }

  if (ob->exstate != NULL) {
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
  
  font = ob->text->font;
  h = elem->corner.y + OBJET_MARGIN_Y;

  if (ob->is_multiple) {
    h += OBJET_MARGIN_M;
  }
    
  if (ob->stereotype != NULL) {
      w = font_string_width(ob->stereotype, font, OBJET_FONTHEIGHT);
      h += OBJET_FONTHEIGHT;
      ob->st_pos.y = h;
      h += OBJET_MARGIN_Y/2.0;
  }

  w = MAX(w, ob->text->max_width);
  p1.y = h + ob->text->ascent;  /* position of text */

  h += ob->text->height*ob->text->numlines;

  if (ob->exstate != NULL) {
      w = MAX(w, font_string_width(ob->exstate, font, OBJET_FONTHEIGHT));
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

  font = font_getfont("Helvetica");
  
  ob->show_attributes = FALSE;
  ob->is_active = FALSE;
  ob->is_multiple = FALSE;

  ob->exstate = NULL;
  ob->stereotype = NULL;

  /* The text position is recalculated later */
  p.x = 0.0;
  p.y = 0.0;
  ob->attributes = new_text("", font, 0.8, &p, &color_black, ALIGN_LEFT);
  ob->text = new_text("", font, 0.8, &p, &color_black, ALIGN_CENTER);
  
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

  if (ob->stereotype != NULL)
      g_free(ob->stereotype);

  if (ob->exstate != NULL)
      g_free(ob->exstate);

  text_destroy(ob->attributes);

  element_destroy(&ob->element);
}

static Object *
objet_copy(Objet *ob)
{
  int i;
  Objet *newob;
  Element *elem, *newelem;
  Object *newobj;
  
  elem = &ob->element;
  
  newob = g_malloc0(sizeof(Objet));
  newelem = &newob->element;
  newobj = &newelem->object;

  element_copy(elem, newelem);

  newob->text = text_copy(ob->text);
  
  for (i=0;i<8;i++) {
    newobj->connections[i] = &newob->connections[i];
    newob->connections[i].object = newobj;
    newob->connections[i].connected = NULL;
    newob->connections[i].pos = ob->connections[i].pos;
    newob->connections[i].last_pos = ob->connections[i].last_pos;
  }

  newob->stereotype = (ob->stereotype != NULL) ? 
      strdup(ob->stereotype): NULL;

  newob->exstate = (ob->exstate != NULL) ? 
      strdup(ob->exstate): NULL;

  newob->attributes = text_copy(ob->attributes);

  newob->is_active = ob->is_active;
  newob->show_attributes = ob->show_attributes;
  newob->is_multiple = ob->is_multiple;
  
  objet_update_data(newob);
  
  return &newob->element.object;
}

static void
objet_state_free(ObjectState *ostate)
{
  ObjetState *state = (ObjetState *)ostate;
  g_free(state->stereotype);
  g_free(state->exstate);
  g_free(state->attributes);
}

static ObjetState *
objet_get_state(Objet *ob)
{
  ObjetState *state = g_new0(ObjetState, 1);

  state->obj_state.free = objet_state_free;

  state->stereotype = g_strdup(ob->stereotype);
  state->exstate = g_strdup(ob->exstate);
  state->attributes = text_get_string_copy(ob->attributes);

  state->is_active = ob->is_active;
  state->show_attributes = ob->show_attributes;
  state->is_multiple = ob->is_multiple;

  return state;
}

static void
objet_set_state(Objet *ob, ObjetState *state)
{
  g_free(ob->stereotype);
  ob->stereotype = state->stereotype;
  g_free(ob->exstate);
  ob->exstate = state->exstate;
  text_set_string(ob->attributes, state->attributes);
  g_free(state->attributes);
		  
  ob->is_active = state->is_active;
  ob->show_attributes = state->show_attributes;
  ob->is_multiple = state->is_multiple;

  g_free(state);
  
  objet_update_data(ob);
}

static void
objet_save(Objet *ob, ObjectNode obj_node, const char *filename)
{
  element_save(&ob->element, obj_node);

  data_add_text(new_attribute(obj_node, "text"),
		ob->text);

  data_add_string(new_attribute(obj_node, "stereotype"),
		  ob->stereotype);

  data_add_string(new_attribute(obj_node, "exstate"),
		  ob->exstate);

  data_add_text(new_attribute(obj_node, "attrib"),
		ob->attributes);
    
  data_add_boolean(new_attribute(obj_node, "is_active"),
		   ob->is_active);
  
  data_add_boolean(new_attribute(obj_node, "show_attribs"),
		   ob->show_attributes);

  data_add_boolean(new_attribute(obj_node, "multiple"),
		   ob->is_multiple);
}

static Object *
objet_load(ObjectNode obj_node, int version, const char *filename)
{
  Objet *ob;
  AttributeNode attr;
  Element *elem;
  Object *obj;
  int i;
  
  ob = g_malloc0(sizeof(Objet));
  elem = &ob->element;
  obj = &elem->object;
  
  obj->type = &objet_type;
  obj->ops = &objet_ops;

  element_load(elem, obj_node);
  
  ob->text = NULL;
  attr = object_find_attribute(obj_node, "text");
  if (attr != NULL)
    ob->text = data_text(attribute_first_data(attr));

  ob->stereotype = NULL;
  attr = object_find_attribute(obj_node, "stereotype");
  if (attr != NULL)
      ob->stereotype = data_string(attribute_first_data(attr));

  ob->exstate = NULL;
  attr = object_find_attribute(obj_node, "exstate");
  if (attr != NULL)
      ob->exstate = data_string(attribute_first_data(attr));

  ob->attributes = NULL;
  attr = object_find_attribute(obj_node, "attrib");
  if (attr != NULL)
    ob->attributes = data_text(attribute_first_data(attr));

  attr = object_find_attribute(obj_node, "is_active");
  if (attr != NULL)
    ob->is_active = data_boolean(attribute_first_data(attr));
  else
    ob->is_active = FALSE;

  attr = object_find_attribute(obj_node, "show_attribs");
  if (attr != NULL)
    ob->show_attributes = data_boolean(attribute_first_data(attr));
  else
    ob->show_attributes = FALSE;
  
  attr = object_find_attribute(obj_node, "multiple");
  if (attr != NULL)
    ob->is_multiple = data_boolean(attribute_first_data(attr));
  else
    ob->is_multiple = FALSE;

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

  return &ob->element.object;
}


static ObjectChange *
objet_apply_properties(Objet *ob)
{
  ObjetPropertiesDialog *prop_dialog;
  ObjectState *old_state;
  char *str;

  prop_dialog = properties_dialog;

  old_state = (ObjectState *)objet_get_state(ob);

  /* Read from dialog and put in object: */
  if (ob->exstate != NULL)
    g_free(ob->exstate);
  str = gtk_entry_get_text(prop_dialog->name);
  if (strlen(str) != 0) {
      ob->exstate = g_malloc(sizeof(char)*strlen(str)+2+1);
      sprintf(ob->exstate, "[%s]", str);
  } else
    ob->exstate = NULL;

  if (ob->stereotype != NULL)
    g_free(ob->stereotype);
  
  str = gtk_entry_get_text(prop_dialog->stereotype);
  
  if (strlen(str) != 0) {
    ob->stereotype = g_malloc(sizeof(char)*strlen(str)+2+1);
    ob->stereotype[0] = UML_STEREOTYPE_START;
    ob->stereotype[1] = 0;
    strcat(ob->stereotype, str);
    ob->stereotype[strlen(str)+1] = UML_STEREOTYPE_END;
    ob->stereotype[strlen(str)+2] = 0;
  } else {
    ob->stereotype = NULL;
  }

  ob->is_active = prop_dialog->active->active;
  ob->show_attributes = prop_dialog->show_attrib->active;
  ob->is_multiple = prop_dialog->multiple->active;

  
  text_set_string(ob->attributes,
		  gtk_editable_get_chars( GTK_EDITABLE(prop_dialog->attribs),
						 0, -1));

  objet_update_data(ob);
  
  return new_object_state_change(&ob->element.object, old_state, 
				 (GetStateFunc)objet_get_state,
				 (SetStateFunc)objet_set_state);
}

static void
fill_in_dialog(Objet *ob)
{
  ObjetPropertiesDialog *prop_dialog;
  char *str;
  
  prop_dialog = properties_dialog;

  if (ob->exstate != NULL) {
      str = strdup(ob->exstate+1);
      str[strlen(str)-1] = 0;
      gtk_entry_set_text(prop_dialog->name, str);
      g_free(str);
  } else 
    gtk_entry_set_text(prop_dialog->name, "");
  
  
  if (ob->stereotype != NULL) {
    str = strdup(ob->stereotype);
    strcpy(str, ob->stereotype+1);
    str[strlen(str)-1] = 0;
    gtk_entry_set_text(prop_dialog->stereotype, str);
    g_free(str);
  } else {
    gtk_entry_set_text(prop_dialog->stereotype, "");
  }

  gtk_toggle_button_set_active(prop_dialog->show_attrib, ob->show_attributes);
  gtk_toggle_button_set_active(prop_dialog->active, ob->is_active);
  gtk_toggle_button_set_active(prop_dialog->multiple, ob->is_multiple);


  gtk_text_freeze(GTK_TEXT(prop_dialog->attribs));
  gtk_text_set_point(GTK_TEXT(prop_dialog->attribs), 0);
  gtk_text_forward_delete( GTK_TEXT(prop_dialog->attribs), gtk_text_get_length(GTK_TEXT(prop_dialog->attribs)));
  gtk_text_insert( GTK_TEXT(prop_dialog->attribs),
		   NULL, NULL, NULL,
		   text_get_string_copy(ob->attributes),
		   -1);
  gtk_text_thaw(GTK_TEXT(prop_dialog->attribs));
}

static GtkWidget *
objet_get_properties(Objet *ob)
{
  ObjetPropertiesDialog *prop_dialog;
  GtkWidget *dialog;
  GtkWidget *checkbox;
  GtkWidget *entry;
  GtkWidget *hbox;
  GtkWidget *label;

  if (properties_dialog == NULL) {

    prop_dialog = g_new(ObjetPropertiesDialog, 1);
    properties_dialog = prop_dialog;

    dialog = gtk_vbox_new(FALSE, 0);
    gtk_object_ref(GTK_OBJECT(dialog));
    gtk_object_sink(GTK_OBJECT(dialog));
    prop_dialog->dialog = dialog;
    
    hbox = gtk_hbox_new(FALSE, 5);

    label = gtk_label_new(_("Explicit state:"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    entry = gtk_entry_new();
    prop_dialog->name = GTK_ENTRY(entry);
    gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, TRUE, 0);
    gtk_widget_show (label);
    gtk_widget_show (entry);
    gtk_box_pack_start (GTK_BOX (dialog), hbox, TRUE, TRUE, 0);
    gtk_widget_show(hbox);

    hbox = gtk_hbox_new(FALSE, 5);
    gtk_container_set_border_width (GTK_CONTAINER (hbox), 5);
    label = gtk_label_new(_("Stereotype:"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    entry = gtk_entry_new();
    prop_dialog->stereotype = GTK_ENTRY(entry);
    gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, TRUE, 0);
    gtk_widget_show (label);
    gtk_widget_show (entry);
    gtk_box_pack_start (GTK_BOX (dialog), hbox, TRUE, TRUE, 0);
    gtk_widget_show(hbox);
    
    label = gtk_label_new(_("Attributes:"));
    gtk_box_pack_start (GTK_BOX (dialog), label, FALSE, TRUE, 0);
    entry = gtk_text_new(NULL, NULL);
    prop_dialog->attribs = entry;
    gtk_text_set_editable(GTK_TEXT(entry), TRUE);
    gtk_box_pack_start (GTK_BOX (dialog), entry, TRUE, TRUE, 0);
    gtk_widget_show (label);
    gtk_widget_show (entry);
        
    hbox = gtk_hbox_new(FALSE, 5);
    checkbox = gtk_check_button_new_with_label(_("Show attributes"));
    gtk_box_pack_start (GTK_BOX (hbox), checkbox, FALSE, TRUE, 0);
    prop_dialog->show_attrib = GTK_TOGGLE_BUTTON( checkbox );
    gtk_widget_show(checkbox);
    checkbox = gtk_check_button_new_with_label(_("Active object"));
    gtk_box_pack_start (GTK_BOX (hbox), checkbox, FALSE, TRUE, 0);
    prop_dialog->active = GTK_TOGGLE_BUTTON( checkbox );
    gtk_widget_show(checkbox);
      
    checkbox = gtk_check_button_new_with_label(_("multiple instance"));
    gtk_box_pack_start (GTK_BOX (hbox), checkbox, FALSE, TRUE, 0);
    prop_dialog->multiple = GTK_TOGGLE_BUTTON( checkbox );
    gtk_widget_show(checkbox);
      
    gtk_widget_show(hbox);
    gtk_box_pack_start (GTK_BOX (dialog), hbox, TRUE, TRUE, 0);

  }
  
  fill_in_dialog(ob);
  gtk_widget_show (properties_dialog->dialog);

  return properties_dialog->dialog;
}

