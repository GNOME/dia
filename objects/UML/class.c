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

#include "intl.h"
#include "render.h"
#include "attributes.h"
#include "properties.h"

#include "class.h"

#include "pixmaps/umlclass.xpm"

#define UMLCLASS_BORDER 0.1
#define UMLCLASS_UNDERLINEWIDTH 0.05

static real umlclass_distance_from(UMLClass *umlclass, Point *point);
static void umlclass_select(UMLClass *umlclass, Point *clicked_point,
			    Renderer *interactive_renderer);
static void umlclass_move_handle(UMLClass *umlclass, Handle *handle,
				 Point *to, HandleMoveReason reason, ModifierKeys modifiers);
static void umlclass_move(UMLClass *umlclass, Point *to);
static void umlclass_draw(UMLClass *umlclass, Renderer *renderer);
static Object *umlclass_create(Point *startpoint,
			       void *user_data,
			       Handle **handle1,
			       Handle **handle2);
static void umlclass_destroy(UMLClass *umlclass);
static Object *umlclass_copy(UMLClass *umlclass);

static void umlclass_save(UMLClass *umlclass, ObjectNode obj_node,
			  const char *filename);
static Object *umlclass_load(ObjectNode obj_node, int version,
			     const char *filename);

static PropDescription *umlclass_describe_props(UMLClass *umlclass);
static void umlclass_get_props(UMLClass *umlclass, Property *props, guint nprops);
static void umlclass_set_props(UMLClass *umlclass, Property *props, guint nprops);

static ObjectTypeOps umlclass_type_ops =
{
  (CreateFunc) umlclass_create,
  (LoadFunc)   umlclass_load,
  (SaveFunc)   umlclass_save
};

ObjectType umlclass_type =
{
  "UML - Class",   /* name */
  0,                      /* version */
  (char **) umlclass_xpm,  /* pixmap */
  
  &umlclass_type_ops       /* ops */
};

static ObjectOps umlclass_ops = {
  (DestroyFunc)         umlclass_destroy,
  (DrawFunc)            umlclass_draw,
  (DistanceFunc)        umlclass_distance_from,
  (SelectFunc)          umlclass_select,
  (CopyFunc)            umlclass_copy,
  (MoveFunc)            umlclass_move,
  (MoveHandleFunc)      umlclass_move_handle,
  (GetPropertiesFunc)   umlclass_get_properties,
  (ApplyPropertiesFunc) umlclass_apply_properties,
  (ObjectMenuFunc)      NULL,
  (DescribePropsFunc)   umlclass_describe_props,
  (GetPropsFunc)        umlclass_get_props,
  (SetPropsFunc)        umlclass_set_props
};

static PropDescription umlclass_props[] = {
  ELEMENT_COMMON_PROPERTIES,
  PROP_STD_LINE_COLOUR,
  PROP_STD_FILL_COLOUR,
  { "name", PROP_TYPE_STRING, PROP_FLAG_VISIBLE,
  N_("Name"), NULL, NULL },
  { "stereotype", PROP_TYPE_STRING, PROP_FLAG_VISIBLE,
  N_("Stereotype"), NULL, NULL },
  { "abstract", PROP_TYPE_INT, PROP_FLAG_VISIBLE,
  N_("Abstract"), NULL, NULL },
  { "suppress_attributes", PROP_TYPE_INT, PROP_FLAG_VISIBLE,
  N_("Suppress Attributes"), NULL, NULL },
  { "suppress_operations", PROP_TYPE_INT, PROP_FLAG_VISIBLE,
  N_("Suppress Operations"), NULL, NULL },
  { "visible_attributes", PROP_TYPE_INT, PROP_FLAG_VISIBLE,
  N_("Visible Attributes"), NULL, NULL },
  { "visible_operations", PROP_TYPE_INT, PROP_FLAG_VISIBLE,
  N_("Visible Operations"), NULL, NULL },
  
  /* Attributes: XXX */
  /* Operators: XXX */

  { "template", PROP_TYPE_INT, PROP_FLAG_VISIBLE,
  N_("Template"), NULL, NULL },

  /* formal_params XXX */

  PROP_DESC_END
};

static PropDescription *
umlclass_describe_props(UMLClass *umlclass)
{
  if (umlclass_props[0].quark == 0)
    prop_desc_list_calculate_quarks(umlclass_props);
  return umlclass_props;
}

static PropOffset umlclass_offsets[] = {
  ELEMENT_COMMON_PROPERTIES_OFFSETS,
  { "line_colour", PROP_TYPE_COLOUR, offsetof(UMLClass, color_foreground) },
  { "fill_colour", PROP_TYPE_COLOUR, offsetof(UMLClass, color_background) },
  { "name", PROP_TYPE_STRING, offsetof(UMLClass, name) },
  { "stereotype", PROP_TYPE_STRING, offsetof(UMLClass, stereotype) },
  { "abstract", PROP_TYPE_INT, offsetof(UMLClass , abstract) },
  { "suppress_attributes", PROP_TYPE_INT, offsetof(UMLClass , suppress_attributes) },
  { "visible_attributes", PROP_TYPE_INT, offsetof(UMLClass , visible_attributes) },
  { "suppress_operations", PROP_TYPE_INT, offsetof(UMLClass , suppress_operations) },
  { "visible_operations", PROP_TYPE_INT, offsetof(UMLClass , visible_operations) },

  { NULL, 0, 0 },
};

static void
umlclass_get_props(UMLClass * umlclass, Property *props, guint nprops)
{
  object_get_props_from_offsets(&umlclass->element.object, 
                                umlclass_offsets, props, nprops);
}

static void
umlclass_set_props(UMLClass *umlclass, Property *props, guint nprops)
{
  object_set_props_from_offsets(&umlclass->element.object, umlclass_offsets,
                                props, nprops);
  umlclass_update_data(umlclass);
}

static real
umlclass_distance_from(UMLClass *umlclass, Point *point)
{
  Object *obj = &umlclass->element.object;
  return distance_rectangle_point(&obj->bounding_box, point);
}

static void
umlclass_select(UMLClass *umlclass, Point *clicked_point,
	       Renderer *interactive_renderer)
{
  element_update_handles(&umlclass->element);
}

static void
umlclass_move_handle(UMLClass *umlclass, Handle *handle,
		     Point *to, HandleMoveReason reason, ModifierKeys modifiers)
{
  assert(umlclass!=NULL);
  assert(handle!=NULL);
  assert(to!=NULL);

  assert(handle->id < 8);
}

static void
umlclass_move(UMLClass *umlclass, Point *to)
{
  umlclass->element.corner = *to;
  umlclass_update_data(umlclass);
}

static void
umlclass_draw(UMLClass *umlclass, Renderer *renderer)
{
  Element *elem;
  real x,y;
  Point p, p1, p2, p3;
  Font *font;
  int i;
  GList *list;
  
  assert(umlclass != NULL);
  assert(renderer != NULL);

  elem = &umlclass->element;

  x = elem->corner.x;
  y = elem->corner.y;

  renderer->ops->set_fillstyle(renderer, FILLSTYLE_SOLID);
  renderer->ops->set_linewidth(renderer, UMLCLASS_BORDER);
  renderer->ops->set_linestyle(renderer, LINESTYLE_SOLID);

  p1.x = x;
  p2.x = x + elem->width;
  p1.y = y;
  y += umlclass->namebox_height;
  p2.y = y;
  
  renderer->ops->fill_rect(renderer, 
			   &p1, &p2,
			   &umlclass->color_background);
  renderer->ops->draw_rect(renderer, 
			   &p1, &p2,
			   &umlclass->color_foreground);

  /* stereotype: */
  font = umlclass->normal_font;
  p.x = x + elem->width / 2.0;
  p.y = p1.y;

  if (umlclass->stereotype != NULL) {
    p.y += 0.1 + umlclass->font_ascent;
    renderer->ops->set_font(renderer, font, umlclass->font_height);
    renderer->ops->draw_string(renderer,
			       umlclass->stereotype_string,
			       &p, ALIGN_CENTER, 
			       &umlclass->color_foreground);
  }

  /* name: */
  if (umlclass->abstract) 
    font = umlclass->abstract_classname_font;
  else 
    font = umlclass->classname_font;

  p.y += umlclass->classname_font_height;

  renderer->ops->set_font(renderer, font, umlclass->classname_font_height);
  renderer->ops->draw_string(renderer,
			     umlclass->name,
			     &p, ALIGN_CENTER, 
			     &umlclass->color_foreground);
  
  if (umlclass->visible_attributes) {
    p1.x = x;
    p1.y = y;
    y += umlclass->attributesbox_height;
    p2.y = y;
  
    renderer->ops->fill_rect(renderer, 
			     &p1, &p2,
			     &umlclass->color_background);
    renderer->ops->draw_rect(renderer, 
			     &p1, &p2,
			     &umlclass->color_foreground);

    if (!umlclass->suppress_attributes) {
      p.x = x + UMLCLASS_BORDER/2.0 + 0.1;
      p.y = p1.y + 0.1 + umlclass->font_ascent;

      i = 0;
      list = umlclass->attributes;
      while (list != NULL) {
	UMLAttribute *attr = (UMLAttribute *)list->data;
	if (attr->abstract) 
	  font = umlclass->abstract_font;
	else
	  font = umlclass->normal_font;

	renderer->ops->set_font(renderer, font, umlclass->font_height);
	renderer->ops->draw_string(renderer,
				   umlclass->attributes_strings[i],
				   &p, ALIGN_LEFT, 
				   &umlclass->color_foreground);
	if (attr->class_scope) {
	  p1 = p; 
	  p1.y += umlclass->font_height*0.1;
	  p3 = p1;
	  p3.x += font_string_width(umlclass->attributes_strings[i],
				    font, umlclass->font_height);
	  renderer->ops->set_linewidth(renderer, UMLCLASS_UNDERLINEWIDTH);
	  renderer->ops->draw_line(renderer, &p1, &p3,  &umlclass->color_foreground);
	  renderer->ops->set_linewidth(renderer, UMLCLASS_BORDER);
	}
	
	
	p.y += umlclass->font_height;
	
	list = g_list_next(list);
	i++;
      }
    }
  }

  if (umlclass->visible_operations) {
    p1.x = x;
    p1.y = y;
    y += umlclass->operationsbox_height;
    p2.y = y;
    
    renderer->ops->fill_rect(renderer, 
			     &p1, &p2,
			     &umlclass->color_background);
    renderer->ops->draw_rect(renderer, 
			     &p1, &p2,
			     &umlclass->color_foreground);
    if (!umlclass->suppress_operations) {
      p.x = x + UMLCLASS_BORDER/2.0 + 0.1;
      p.y = p1.y + 0.1 + umlclass->font_ascent;

      i = 0;
      list = umlclass->operations;
      while (list != NULL) {
	UMLOperation *op = (UMLOperation *)list->data;
	if (op->abstract) 
	  font = umlclass->abstract_font;
	else
	  font = umlclass->normal_font;

	renderer->ops->set_font(renderer, font, umlclass->font_height);
	renderer->ops->draw_string(renderer,
				   umlclass->operations_strings[i],
				   &p, ALIGN_LEFT, 
				   &umlclass->color_foreground);

	if (op->class_scope) {
	  p1 = p; 
	  p1.y += umlclass->font_height*0.1;
	  p3 = p1;
	  p3.x += font_string_width(umlclass->operations_strings[i],
				    font, umlclass->font_height);
	  renderer->ops->set_linewidth(renderer, UMLCLASS_UNDERLINEWIDTH);
	  renderer->ops->draw_line(renderer, &p1, &p3,  &umlclass->color_foreground);
	  renderer->ops->set_linewidth(renderer, UMLCLASS_BORDER);
	}

	p.y += umlclass->font_height;

	list = g_list_next(list);
	i++;
      }
    }
  }

  if (umlclass->template) {
    x = elem->corner.x + elem->width - 2.3; 
    y = elem->corner.y - umlclass->templates_height + 0.3; 

    p1.x = x;
    p1.y = y;
    p2.x = x + umlclass->templates_width;
    p2.y = y + umlclass->templates_height;
    
    renderer->ops->fill_rect(renderer, 
			     &p1, &p2,
			     &umlclass->color_background);

    renderer->ops->set_linestyle(renderer, LINESTYLE_DASHED);
    renderer->ops->set_dashlength(renderer, 0.3);
    
    renderer->ops->draw_rect(renderer, 
			     &p1, &p2,
			     &umlclass->color_foreground);

    p.x = x + 0.3;
    p.y = y + 0.1 + font_ascent(umlclass->normal_font, umlclass->font_height);

    renderer->ops->set_font(renderer, umlclass->normal_font,
			    umlclass->font_height);
    i = 0;
    list = umlclass->formal_params;
    while (list != NULL) {
      /*UMLFormalParameter *param = (UMLFormalParameter *)list->data;*/

      renderer->ops->draw_string(renderer,
				 umlclass->templates_strings[i],
				 &p, ALIGN_LEFT, 
				 &umlclass->color_foreground);
      
      p.y += umlclass->font_height;

      list = g_list_next(list);
      i++;
    }
  }
}

void
umlclass_update_data(UMLClass *umlclass)
{
  Element *elem = &umlclass->element;
  Object *obj = &elem->object;
  real x,y;
  GList *list;

  x = elem->corner.x;
  y = elem->corner.y;
  
  /* Update connections: */
  umlclass->connections[0].pos = elem->corner;
  umlclass->connections[1].pos.x = x + elem->width / 2.0;
  umlclass->connections[1].pos.y = y;
  umlclass->connections[2].pos.x = x + elem->width;
  umlclass->connections[2].pos.y = y;
  umlclass->connections[3].pos.x = x;
  umlclass->connections[3].pos.y = y + umlclass->namebox_height/2.0;
  umlclass->connections[4].pos.x = x + elem->width;
  umlclass->connections[4].pos.y = y + umlclass->namebox_height/2.0;
  umlclass->connections[5].pos.x = x;
  umlclass->connections[5].pos.y = y + elem->height;
  umlclass->connections[6].pos.x = x + elem->width / 2.0;
  umlclass->connections[6].pos.y = y + elem->height;
  umlclass->connections[7].pos.x = x + elem->width;
  umlclass->connections[7].pos.y = y + elem->height;

  y += umlclass->namebox_height + 0.1 + umlclass->font_height/2;

  list = umlclass->attributes;
  while (list != NULL) {
    UMLAttribute *attr = (UMLAttribute *)list->data;

    attr->left_connection->pos.x = x;
    attr->left_connection->pos.y = y;
    attr->right_connection->pos.x = x + elem->width;
    attr->right_connection->pos.y = y;

    y += umlclass->font_height;

    list = g_list_next(list);
  }

  y = elem->corner.y + umlclass->namebox_height +
    umlclass->attributesbox_height + 0.1 + umlclass->font_height/2;
    
  list = umlclass->operations;
  while (list != NULL) {
    UMLOperation *op = (UMLOperation *)list->data;

    op->left_connection->pos.x = x;
    op->left_connection->pos.y = y;
    op->right_connection->pos.x = x + elem->width;
    op->right_connection->pos.y = y;

    y += umlclass->font_height;

    list = g_list_next(list);
  }
  
  element_update_boundingbox(elem);

  if (umlclass->template) {
    /* fix boundingumlclass for templates: */
    obj->bounding_box.top -= (umlclass->templates_height - 0.3) ;
    obj->bounding_box.right += (umlclass->templates_width - 2.3);
  }
  
  obj->position = elem->corner;

  element_update_handles(elem);
}

void
umlclass_calculate_data(UMLClass *umlclass)
{
  real font_height;
  int i;
  GList *list;
  real maxwidth;
  real width;
  
  font_height = umlclass->font_height;
  umlclass->font_ascent = font_ascent(umlclass->normal_font, font_height);

  /* name box: */

  if (umlclass->abstract) 
    maxwidth = font_string_width(umlclass->name,
				 umlclass->abstract_classname_font,
				 umlclass->classname_font_height);
  else 
    maxwidth = font_string_width(umlclass->name,
				 umlclass->classname_font,
				 umlclass->classname_font_height);

  umlclass->namebox_height = umlclass->classname_font_height + 4*0.1;
  if (umlclass->stereotype_string != NULL) {
    g_free(umlclass->stereotype_string);
  }
  if (umlclass->stereotype != NULL) {
    umlclass->namebox_height += font_height;
    umlclass->stereotype_string =
      g_malloc(sizeof(char)*(strlen(umlclass->stereotype)+2+1));
    
    umlclass->stereotype_string[0] = UML_STEREOTYPE_START;
    umlclass->stereotype_string[1] = 0;
    
    strcat(umlclass->stereotype_string, umlclass->stereotype);

    i = strlen(umlclass->stereotype_string);
    umlclass->stereotype_string[i] = UML_STEREOTYPE_END;
    umlclass->stereotype_string[i+1] = 0;
    
    width = font_string_width(umlclass->stereotype_string, umlclass->normal_font, font_height);
    maxwidth = MAX(width, maxwidth);
  } else {
    umlclass->stereotype_string = NULL;
  }

  /* attributes box: */
  if (umlclass->attributes_strings != NULL) {
    for (i=0;i<umlclass->num_attributes;i++) {
      g_free(umlclass->attributes_strings[i]);
    }
    g_free(umlclass->attributes_strings);
  }
  umlclass->num_attributes = g_list_length(umlclass->attributes);
  umlclass->attributesbox_height = font_height * umlclass->num_attributes + 2*0.1;

  if ((umlclass->attributesbox_height<0.4) ||
      umlclass->suppress_attributes )
      umlclass->attributesbox_height = 0.4;

  if (!umlclass->visible_attributes )
    umlclass->attributesbox_height = 0.0;

  umlclass->attributes_strings = NULL;
  if (umlclass->num_attributes != 0) {
    umlclass->attributes_strings =
      g_malloc(sizeof(char *)*umlclass->num_attributes);
    i = 0;
    list = umlclass->attributes;
    while (list != NULL) {
      UMLAttribute *attr;

      attr = (UMLAttribute *) list->data;
      umlclass->attributes_strings[i] = uml_get_attribute_string(attr);

      if (attr->abstract)
	width = font_string_width(umlclass->attributes_strings[i], umlclass->abstract_font, font_height);
      else
	width = font_string_width(umlclass->attributes_strings[i], umlclass->normal_font, font_height);
      maxwidth = MAX(width, maxwidth);

      i++;
      list = g_list_next(list);
    }
  }

  /* operations box: */
  if (umlclass->operations_strings != NULL) {
    for (i=0;i<umlclass->num_operations;i++) {
      g_free(umlclass->operations_strings[i]);
    }
    g_free(umlclass->operations_strings);
  }
  umlclass->num_operations = g_list_length(umlclass->operations);

  umlclass->operationsbox_height = font_height * umlclass->num_operations + 2*0.1;
  if ((umlclass->operationsbox_height<0.4) ||
      umlclass->suppress_operations )
      umlclass->operationsbox_height = 0.4;
  
  umlclass->operations_strings = NULL;
  if (umlclass->num_operations != 0) {
    umlclass->operations_strings =
      g_malloc(sizeof(char *)*umlclass->num_operations);
    i = 0;
    list = umlclass->operations;
    while (list != NULL) {
      UMLOperation *op;

      op = (UMLOperation *) list->data;
      umlclass->operations_strings[i] = uml_get_operation_string(op);
      
      if (op->abstract)
	width = font_string_width(umlclass->operations_strings[i], umlclass->abstract_font, font_height);
      else
	width = font_string_width(umlclass->operations_strings[i], umlclass->normal_font, font_height);
      maxwidth = MAX(width, maxwidth);

      i++;
      list = g_list_next(list);
    }
  }

  umlclass->element.width = maxwidth + 2*0.3;

  umlclass->element.height = umlclass->namebox_height;
  if (umlclass->visible_attributes)
    umlclass->element.height += umlclass->attributesbox_height;
  if (umlclass->visible_operations)
    umlclass->element.height += umlclass->operationsbox_height;

  /* templates box: */
  if (umlclass->templates_strings != NULL) {
    for (i=0;i<umlclass->num_templates;i++) {
      g_free(umlclass->templates_strings[i]);
    }
    g_free(umlclass->templates_strings);
  }
  umlclass->num_templates = g_list_length(umlclass->formal_params);

  umlclass->templates_height = font_height * umlclass->num_templates + 2*0.1;
  umlclass->templates_height = MAX(umlclass->templates_height, 1.0);


  umlclass->templates_strings = NULL;
  maxwidth = 2.3;
  if (umlclass->num_templates != 0) {
    umlclass->templates_strings =
      g_malloc(sizeof(char *)*umlclass->num_templates);
    i = 0;
    list = umlclass->formal_params;
    while (list != NULL) {
      UMLFormalParameter *param;

      param = (UMLFormalParameter *) list->data;
      umlclass->templates_strings[i] = uml_get_formalparameter_string(param);
      
      width = font_string_width(umlclass->templates_strings[i],
				umlclass->normal_font, font_height);
      maxwidth = MAX(width, maxwidth);

      i++;
      list = g_list_next(list);
    }
  }
  umlclass->templates_width = maxwidth + 2*0.2;
}

static void
fill_in_fontdata(UMLClass *umlclass)
{
  umlclass->font_height = 0.8;
  umlclass->classname_font_height = 1.0;
  umlclass->normal_font = font_getfont("Courier");
  umlclass->abstract_font = font_getfont("Courier-Oblique");
  umlclass->classname_font = font_getfont("Helvetica-Bold");
  umlclass->abstract_classname_font = font_getfont("Helvetica-BoldOblique");
}

static Object *
umlclass_create(Point *startpoint,
	       void *user_data,
  	       Handle **handle1,
	       Handle **handle2)
{
  UMLClass *umlclass;
  Element *elem;
  Object *obj;
  int i;

  umlclass = g_malloc(sizeof(UMLClass));
  elem = &umlclass->element;
  obj = &elem->object;
  
  obj->type = &umlclass_type;

  obj->ops = &umlclass_ops;

  elem->corner = *startpoint;

  element_init(elem, 8, 8); /* No attribs or ops => 0 extra connectionpoints. */

  umlclass->properties_dialog = NULL;
  fill_in_fontdata(umlclass);

  umlclass->name = strdup(_("Class"));
  umlclass->stereotype = NULL;
  
  umlclass->abstract = FALSE;

  umlclass->suppress_attributes = FALSE;
  umlclass->suppress_operations = FALSE;

  umlclass->visible_attributes = TRUE;
  umlclass->visible_operations = TRUE;

  umlclass->attributes = NULL;

  umlclass->operations = NULL;
  
  umlclass->template = (GPOINTER_TO_INT(user_data)==1);
  umlclass->formal_params = NULL;
  
  umlclass->stereotype_string = NULL;
  umlclass->attributes_strings = NULL;
  umlclass->operations_strings = NULL;
  umlclass->templates_strings = NULL;
  
  umlclass->color_foreground = color_black;
  umlclass->color_background = color_white;

  umlclass_calculate_data(umlclass);
  
  for (i=0;i<8;i++) {
    obj->connections[i] = &umlclass->connections[i];
    umlclass->connections[i].object = obj;
    umlclass->connections[i].connected = NULL;
  }
  elem->extra_spacing.border_trans = UMLCLASS_BORDER/2.0;
  umlclass_update_data(umlclass);

  for (i=0;i<8;i++) {
    obj->handles[i]->type = HANDLE_NON_MOVABLE;
  }

  *handle1 = NULL;
  *handle2 = NULL;
  return &umlclass->element.object;
}

static void
umlclass_destroy(UMLClass *umlclass)
{
  int i;
  GList *list;
  UMLAttribute *attr;
  UMLOperation *op;
  UMLFormalParameter *param;
  
  element_destroy(&umlclass->element);
  
  g_free(umlclass->name);
  if (umlclass->stereotype != NULL)
    g_free(umlclass->stereotype);

  list = umlclass->attributes;
  while (list != NULL) {
    attr = (UMLAttribute *)list->data;
    g_free(attr->left_connection);
    g_free(attr->right_connection);
    uml_attribute_destroy(attr);
    list = g_list_next(list);
  }
  g_list_free(umlclass->attributes);
  
  list = umlclass->operations;
  while (list != NULL) {
    op = (UMLOperation *)list->data;
    g_free(op->left_connection);
    g_free(op->right_connection);
    uml_operation_destroy(op);
    list = g_list_next(list);
  }
  g_list_free(umlclass->operations);

  list = umlclass->formal_params;
  while (list != NULL) {
    param = (UMLFormalParameter *)list->data;
    uml_formalparameter_destroy(param);
    list = g_list_next(list);
  }
  g_list_free(umlclass->formal_params);

  if (umlclass->stereotype_string != NULL) {
    g_free(umlclass->stereotype_string);
  }

  if (umlclass->attributes_strings != NULL) {
    for (i=0;i<umlclass->num_attributes;i++) {
      g_free(umlclass->attributes_strings[i]);
    }
    g_free(umlclass->attributes_strings);
  }

  if (umlclass->operations_strings != NULL) {
    for (i=0;i<umlclass->num_operations;i++) {
      g_free(umlclass->operations_strings[i]);
    }
    g_free(umlclass->operations_strings);
  }

  if (umlclass->templates_strings != NULL) {
    for (i=0;i<umlclass->num_templates;i++) {
      g_free(umlclass->templates_strings[i]);
    }
    g_free(umlclass->templates_strings);
  }

  if (umlclass->properties_dialog != NULL) {
    gtk_widget_destroy(umlclass->properties_dialog->dialog);
    g_list_free(umlclass->properties_dialog->deleted_connections);
    g_free(umlclass->properties_dialog);
  }
}

static Object *
umlclass_copy(UMLClass *umlclass)
{
  int i;
  UMLClass *newumlclass;
  Element *elem, *newelem;
  Object *newobj;
  GList *list;
  UMLAttribute *attr;
  UMLAttribute *newattr;
  UMLOperation *op;
  UMLOperation *newop;
  UMLFormalParameter *param;
  
  elem = &umlclass->element;
  
  newumlclass = g_malloc(sizeof(UMLClass));
  newelem = &newumlclass->element;
  newobj = &newelem->object;

  element_copy(elem, newelem);

  newumlclass->font_height = umlclass->font_height;
  newumlclass->classname_font_height = umlclass->classname_font_height;
  newumlclass->normal_font = umlclass->normal_font;
  newumlclass->abstract_font = umlclass->abstract_font;
  newumlclass->classname_font = umlclass->classname_font;
  newumlclass->abstract_classname_font = umlclass->abstract_classname_font;

  newumlclass->name = strdup(umlclass->name);
  if (umlclass->stereotype != NULL)
    newumlclass->stereotype = strdup(umlclass->stereotype);
  else
    newumlclass->stereotype = NULL;
  newumlclass->abstract = umlclass->abstract;
  newumlclass->suppress_attributes = umlclass->suppress_attributes;
  newumlclass->suppress_operations = umlclass->suppress_operations;
  newumlclass->visible_attributes = umlclass->visible_attributes;
  newumlclass->visible_operations = umlclass->visible_operations;
  newumlclass->color_foreground = umlclass->color_foreground;
  newumlclass->color_background = umlclass->color_background;

  newumlclass->attributes = NULL;
  list = umlclass->attributes;
  while (list != NULL) {
    attr = (UMLAttribute *)list->data;
    newattr = uml_attribute_copy(attr);
    
    newattr->left_connection = g_new(ConnectionPoint,1);
    *newattr->left_connection = *attr->left_connection;
    newattr->left_connection->object = newobj;
    newattr->left_connection->connected = NULL;
    
    newattr->right_connection = g_new(ConnectionPoint,1);
    *newattr->right_connection = *attr->right_connection;
    newattr->right_connection->object = newobj;
    newattr->right_connection->connected = NULL;
    
    newumlclass->attributes = g_list_prepend(newumlclass->attributes,
					     newattr);
    list = g_list_next(list);
  }

  newumlclass->operations = NULL;
  list = umlclass->operations;
  while (list != NULL) {
    op = (UMLOperation *)list->data;
    newop = uml_operation_copy(op);
    newop->left_connection = g_new(ConnectionPoint,1);
    *newop->left_connection = *op->left_connection;
    newop->left_connection->object = newobj;
    newop->left_connection->connected = NULL;

    newop->right_connection = g_new(ConnectionPoint,1);
    *newop->right_connection = *op->right_connection;
    newop->right_connection->object = newobj;
    newop->right_connection->connected = NULL;
    
    newumlclass->operations = g_list_prepend(newumlclass->operations,
					     newop);
    list = g_list_next(list);
  }

  newumlclass->template = umlclass->template;
  
  newumlclass->formal_params = NULL;
  list = umlclass->formal_params;
  while (list != NULL) {
    param = (UMLFormalParameter *)list->data;
    newumlclass->formal_params =
      g_list_prepend(newumlclass->formal_params,
		     uml_formalparameter_copy(param));
    list = g_list_next(list);
  }

  newumlclass->properties_dialog = NULL;
     
  newumlclass->stereotype_string = NULL;
  newumlclass->attributes_strings = NULL;
  newumlclass->operations_strings = NULL;
  newumlclass->templates_strings = NULL;

  for (i=0;i<8;i++) {
    newobj->connections[i] = &newumlclass->connections[i];
    newumlclass->connections[i].object = newobj;
    newumlclass->connections[i].connected = NULL;
    newumlclass->connections[i].pos = umlclass->connections[i].pos;
    newumlclass->connections[i].last_pos = umlclass->connections[i].last_pos;
  }

  umlclass_calculate_data(newumlclass);

  i = 8;
  if ( (newumlclass->visible_attributes) &&
       (!newumlclass->suppress_attributes)) {
    list = newumlclass->attributes;
    while (list != NULL) {
      attr = (UMLAttribute *)list->data;
      newobj->connections[i++] = attr->left_connection;
      newobj->connections[i++] = attr->right_connection;
      
      list = g_list_next(list);
    }
  }
  
  if ( (newumlclass->visible_operations) &&
       (!newumlclass->suppress_operations)) {
    list = newumlclass->operations;
    while (list != NULL) {
      op = (UMLOperation *)list->data;
      newobj->connections[i++] = op->left_connection;
      newobj->connections[i++] = op->right_connection;
      
      list = g_list_next(list);
    }
  }

  umlclass_update_data(newumlclass);
  
  return &newumlclass->element.object;
}


static void
umlclass_save(UMLClass *umlclass, ObjectNode obj_node,
	      const char *filename)
{
  UMLAttribute *attr;
  UMLOperation *op;
  UMLFormalParameter *formal_param;
  GList *list;
  AttributeNode attr_node;
  
  element_save(&umlclass->element, obj_node);

  /* Class info: */
  data_add_string(new_attribute(obj_node, "name"),
		  umlclass->name);
  data_add_string(new_attribute(obj_node, "stereotype"),
		  umlclass->stereotype);
  data_add_boolean(new_attribute(obj_node, "abstract"),
		   umlclass->abstract);
  data_add_boolean(new_attribute(obj_node, "suppress_attributes"),
		   umlclass->suppress_attributes);
  data_add_boolean(new_attribute(obj_node, "suppress_operations"),
		   umlclass->suppress_operations);
  data_add_boolean(new_attribute(obj_node, "visible_attributes"),
		   umlclass->visible_attributes);
  data_add_boolean(new_attribute(obj_node, "visible_operations"),
		   umlclass->visible_operations);
  data_add_color(new_attribute(obj_node, "foreground_color"), 
		   &umlclass->color_foreground);		   
  data_add_color(new_attribute(obj_node, "background_color"), 
		   &umlclass->color_background);		   

  /* Attribute info: */
  attr_node = new_attribute(obj_node, "attributes");
  list = umlclass->attributes;
  while (list != NULL) {
    attr = (UMLAttribute *) list->data;
    uml_attribute_write(attr_node, attr);
    list = g_list_next(list);
  }
  
  /* Operations info: */
  attr_node = new_attribute(obj_node, "operations");
  list = umlclass->operations;
  while (list != NULL) {
    op = (UMLOperation *) list->data;
    uml_operation_write(attr_node, op);
    list = g_list_next(list);
  }

  /* Template info: */
  data_add_boolean(new_attribute(obj_node, "template"),
		   umlclass->template);
  
  attr_node = new_attribute(obj_node, "templates");
  list = umlclass->formal_params;
  while (list != NULL) {
    formal_param = (UMLFormalParameter *) list->data;
    uml_formalparameter_write(attr_node, formal_param);
    list = g_list_next(list);
  }
}

static Object *umlclass_load(ObjectNode obj_node, int version,
			     const char *filename)
{
  UMLClass *umlclass;
  Element *elem;
  Object *obj;
  UMLAttribute *attr;
  UMLOperation *op;
  UMLFormalParameter *formal_param;
  AttributeNode attr_node;
  DataNode composite;
  int i;
  int num, num_attr, num_ops;
  GList *list;
  
  umlclass = g_malloc(sizeof(UMLClass));
  elem = &umlclass->element;
  obj = &elem->object;
  
  obj->type = &umlclass_type;
  obj->ops = &umlclass_ops;

  element_load(elem, obj_node);
  
  /* Class info: */
  umlclass->name = NULL;
  attr_node = object_find_attribute(obj_node, "name");
  if (attr_node != NULL)
    umlclass->name = data_string(attribute_first_data(attr_node));
  
  umlclass->stereotype = NULL;
  attr_node = object_find_attribute(obj_node, "stereotype");
  if (attr_node != NULL)
    umlclass->stereotype = data_string(attribute_first_data(attr_node));
  
  umlclass->abstract = FALSE;
  attr_node = object_find_attribute(obj_node, "abstract");
  if (attr_node != NULL)
    umlclass->abstract = data_boolean(attribute_first_data(attr_node));
  
  umlclass->suppress_attributes = FALSE;
  attr_node = object_find_attribute(obj_node, "suppress_attributes");
  if (attr_node != NULL)
    umlclass->suppress_attributes = data_boolean(attribute_first_data(attr_node));
  
  umlclass->suppress_operations = FALSE;
  attr_node = object_find_attribute(obj_node, "suppress_operations");
  if (attr_node != NULL)
    umlclass->suppress_operations = data_boolean(attribute_first_data(attr_node));
  
  umlclass->visible_attributes = FALSE;
  attr_node = object_find_attribute(obj_node, "visible_attributes");
  if (attr_node != NULL)
    umlclass->visible_attributes = data_boolean(attribute_first_data(attr_node));
  
  umlclass->visible_operations = FALSE;
  attr_node = object_find_attribute(obj_node, "visible_operations");
  if (attr_node != NULL)
    umlclass->visible_operations = data_boolean(attribute_first_data(attr_node));
  
  umlclass->color_foreground = color_black;
  attr_node = object_find_attribute(obj_node, "foreground_color");
  if(attr_node != NULL)
    data_color(attribute_first_data(attr_node), &umlclass->color_foreground); 
  
  umlclass->color_background = color_white;
  attr_node = object_find_attribute(obj_node, "background_color");
  if(attr_node != NULL)
    data_color(attribute_first_data(attr_node), &umlclass->color_background); 
    
  /* Attribute info: */
  attr_node = object_find_attribute(obj_node, "attributes");
  num_attr = num = attribute_num_data(attr_node);
  composite = attribute_first_data(attr_node);
  umlclass->attributes = NULL;
  for (i=0;i<num;i++) {
    attr = uml_attribute_read(composite);

    attr->left_connection = g_new(ConnectionPoint,1);
    attr->left_connection->object = obj;
    attr->left_connection->connected = NULL;

    attr->right_connection = g_new(ConnectionPoint,1);
    attr->right_connection->object = obj;
    attr->right_connection->connected = NULL;

    umlclass->attributes = g_list_append(umlclass->attributes, attr);
    composite = data_next(composite);
  }

  /* Operations info: */
  attr_node = object_find_attribute(obj_node, "operations");
  num_ops = num = attribute_num_data(attr_node);
  composite = attribute_first_data(attr_node);
  umlclass->operations = NULL;
  for (i=0;i<num;i++) {
    op = uml_operation_read(composite);

    op->left_connection = g_new(ConnectionPoint,1);
    op->left_connection->object = obj;
    op->left_connection->connected = NULL;

    op->right_connection = g_new(ConnectionPoint,1);
    op->right_connection->object = obj;
    op->right_connection->connected = NULL;
    
    umlclass->operations = g_list_append(umlclass->operations, op);
    composite = data_next(composite);
  }

  /* Template info: */
  umlclass->template = FALSE;
  attr_node = object_find_attribute(obj_node, "template");
  if (attr_node != NULL)
    umlclass->template = data_boolean(attribute_first_data(attr_node));

  attr_node = object_find_attribute(obj_node, "templates");
  num = attribute_num_data(attr_node);
  composite = attribute_first_data(attr_node);
  umlclass->formal_params = NULL;
  for (i=0;i<num;i++) {
    formal_param = uml_formalparameter_read(composite);
    umlclass->formal_params = g_list_append(umlclass->formal_params, formal_param);
    composite = data_next(composite);
  }

  if ( (!umlclass->visible_attributes) ||
       (umlclass->suppress_attributes))
    num_attr = 0;
  
  if ( (!umlclass->visible_operations) ||
       (umlclass->suppress_operations))
    num_ops = 0;
  
  element_init(elem, 8, 8 + num_attr*2 + num_ops*2);

  umlclass->properties_dialog = NULL;
  fill_in_fontdata(umlclass);
  
  umlclass->stereotype_string = NULL;
  umlclass->attributes_strings = NULL;
  umlclass->operations_strings = NULL;
  umlclass->templates_strings = NULL;

  umlclass_calculate_data(umlclass);
  
  for (i=0;i<8;i++) {
    obj->connections[i] = &umlclass->connections[i];
    umlclass->connections[i].object = obj;
    umlclass->connections[i].connected = NULL;
  }

  i = 8;

  if ( (umlclass->visible_attributes) &&
       (!umlclass->suppress_attributes)) {
    list = umlclass->attributes;
    while (list != NULL) {
      attr = (UMLAttribute *)list->data;
      obj->connections[i++] = attr->left_connection;
      obj->connections[i++] = attr->right_connection;
      
      list = g_list_next(list);
    }
  }
  
  if ( (umlclass->visible_operations) &&
       (!umlclass->suppress_operations)) {
    list = umlclass->operations;
    while (list != NULL) {
      op = (UMLOperation *)list->data;
      obj->connections[i++] = op->left_connection;
      obj->connections[i++] = op->right_connection;
      
      list = g_list_next(list);
    }
  }
  elem->extra_spacing.border_trans = UMLCLASS_BORDER/2.0;
  umlclass_update_data(umlclass);

  
  for (i=0;i<8;i++) {
    obj->handles[i]->type = HANDLE_NON_MOVABLE;
  }

  return &umlclass->element.object;
}







