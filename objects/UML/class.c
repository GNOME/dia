/* xxxxxx -- an diagram creation/manipulation program
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

#include "render.h"
#include "attributes.h"
#include "files.h"
#include "sheet.h"

#include "class.h"

#include "pixmaps/umlclass.xpm"
#include "pixmaps/umlclass_template.xpm"

#define UMLCLASS_BORDER 0.1
#define UMLCLASS_UNDERLINEWIDTH 0.05

static real umlclass_distance_from(UMLClass *umlclass, Point *point);
static void umlclass_select(UMLClass *umlclass, Point *clicked_point,
			    Renderer *interactive_renderer);
static void umlclass_move_handle(UMLClass *umlclass, Handle *handle,
				 Point *to, HandleMoveReason reason);
static void umlclass_move(UMLClass *umlclass, Point *to);
static void umlclass_draw(UMLClass *umlclass, Renderer *renderer);
static Object *umlclass_create(Point *startpoint,
			       void *user_data,
			       Handle **handle1,
			       Handle **handle2);
static void umlclass_destroy(UMLClass *umlclass);
static Object *umlclass_copy(UMLClass *umlclass);

static void umlclass_save(UMLClass *umlclass, int fd);
static Object *umlclass_load(int fd, int version);

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

SheetObject umlclass_sheetobj =
{
  &umlclass_type,             /* type */
  "Create a class",           /* description */
  (char **) umlclass_xpm,     /* pixmap */

  GINT_TO_POINTER(0)          /* user_data */
};

SheetObject umlclass_template_sheetobj =
{
  &umlclass_type,             /* type */
  "Create a template class",  /* description */
  (char **) umlclass_template_xpm,     /* pixmap */

  GINT_TO_POINTER(1)          /* user_data */
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
  (IsEmptyFunc)         object_return_false
};

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
		     Point *to, HandleMoveReason reason)
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
			   &color_white);
  renderer->ops->draw_rect(renderer, 
			   &p1, &p2,
			   &color_black);

  /* stereotype: */
  font = umlclass->normal_font;
  p.x = x + elem->width / 2.0;
  p.y = p1.y + 0.1 + umlclass->font_ascent;

  if (umlclass->stereotype != NULL) {
    renderer->ops->set_font(renderer, font, umlclass->font_height);
    renderer->ops->draw_string(renderer,
			       umlclass->stereotype_string,
			       &p, ALIGN_CENTER, 
			       &color_black);
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
			     &color_black);
  
  if (umlclass->visible_attributes) {
    p1.x = x;
    p1.y = y;
    y += umlclass->attributesbox_height;
    p2.y = y;
  
    renderer->ops->fill_rect(renderer, 
			     &p1, &p2,
			     &color_white);
    renderer->ops->draw_rect(renderer, 
			     &p1, &p2,
			     &color_black);

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
				   &color_black);
	if (attr->class_scope) {
	  p1 = p; 
	  p1.y += umlclass->font_height*0.1;
	  p3 = p1;
	  p3.x += font_string_width(umlclass->attributes_strings[i],
				    font, umlclass->font_height);
	  renderer->ops->set_linewidth(renderer, UMLCLASS_UNDERLINEWIDTH);
	  renderer->ops->draw_line(renderer, &p1, &p3,  &color_black);
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
			     &color_white);
    renderer->ops->draw_rect(renderer, 
			     &p1, &p2,
			     &color_black);
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
				   &color_black);

	if (op->class_scope) {
	  p.y += umlclass->font_height*0.1;
	  p1 = p;
	  p1.x += font_string_width(umlclass->operations_strings[i],
				    font, umlclass->font_height);
	  renderer->ops->set_linewidth(renderer, UMLCLASS_UNDERLINEWIDTH);
	  renderer->ops->draw_line(renderer, &p, &p1,  &color_black);
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
			     &color_white);

    renderer->ops->set_linestyle(renderer, LINESTYLE_DASHED);
    renderer->ops->set_dashlength(renderer, 0.3);
    
    renderer->ops->draw_rect(renderer, 
			     &p1, &p2,
			     &color_black);

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
				 &color_black);
      
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
  Object *obj = (Object *)umlclass;
  
  /* Update connections: */
  umlclass->connections[0].pos = elem->corner;
  umlclass->connections[1].pos.x = elem->corner.x + elem->width / 2.0;
  umlclass->connections[1].pos.y = elem->corner.y;
  umlclass->connections[2].pos.x = elem->corner.x + elem->width;
  umlclass->connections[2].pos.y = elem->corner.y;
  umlclass->connections[3].pos.x = elem->corner.x;
  umlclass->connections[3].pos.y = elem->corner.y + elem->height / 2.0;
  umlclass->connections[4].pos.x = elem->corner.x + elem->width;
  umlclass->connections[4].pos.y = elem->corner.y + elem->height / 2.0;
  umlclass->connections[5].pos.x = elem->corner.x;
  umlclass->connections[5].pos.y = elem->corner.y + elem->height;
  umlclass->connections[6].pos.x = elem->corner.x + elem->width / 2.0;
  umlclass->connections[6].pos.y = elem->corner.y + elem->height;
  umlclass->connections[7].pos.x = elem->corner.x + elem->width;
  umlclass->connections[7].pos.y = elem->corner.y + elem->height;

  element_update_boundingbox(elem);
  /* fix boundingumlclass for line_width: */
  obj->bounding_box.top -= UMLCLASS_BORDER/2.0;
  obj->bounding_box.left -= UMLCLASS_BORDER/2.0;
  obj->bounding_box.bottom += UMLCLASS_BORDER/2.0;
  obj->bounding_box.right += UMLCLASS_BORDER/2.0;


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

  umlclass->namebox_height =
    font_height + umlclass->classname_font_height + 2*0.1;
  if (umlclass->stereotype_string != NULL) {
    g_free(umlclass->stereotype_string);
  }
  if (umlclass->stereotype != NULL) {
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
  umlclass->classname_font_height = 1.2;
  umlclass->normal_font = font_getfont("Courier");
  umlclass->abstract_font = font_getfont("Courier-Oblique");
  umlclass->classname_font = font_getfont("Courier-Bold");
  umlclass->abstract_classname_font = font_getfont("Courier-BoldOblique");
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
  obj = (Object *) umlclass;
  
  obj->type = &umlclass_type;

  obj->ops = &umlclass_ops;

  elem->corner = *startpoint;

  element_init(elem, 8, 8);

  umlclass->properties_dialog = NULL;
  fill_in_fontdata(umlclass);

  umlclass->name = strdup("Class");
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
  
  umlclass_calculate_data(umlclass);
  
  for (i=0;i<8;i++) {
    obj->connections[i] = &umlclass->connections[i];
    umlclass->connections[i].object = obj;
    umlclass->connections[i].connected = NULL;
  }
  umlclass_update_data(umlclass);

  for (i=0;i<8;i++) {
    obj->handles[i]->type = HANDLE_NON_MOVABLE;
  }

  *handle1 = NULL;
  *handle2 = NULL;
  return (Object *)umlclass;
}

static void
umlclass_destroy(UMLClass *umlclass)
{
  int i;
  GList *list;
  UMLAttribute *attr;
  UMLOperation *op;
  UMLFormalParameter *param;
  
  g_free(umlclass->name);
  if (umlclass->stereotype != NULL)
    g_free(umlclass->stereotype);

  list = umlclass->attributes;
  while (list != NULL) {
    attr = (UMLAttribute *)list->data;
    uml_attribute_destroy(attr);
    list = g_list_next(list);
  }
  g_list_free(umlclass->attributes);
  
  list = umlclass->operations;
  while (list != NULL) {
    op = (UMLOperation *)list->data;
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
    g_free(umlclass->properties_dialog);
  }

  element_destroy(&umlclass->element);
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
  UMLOperation *op;
  UMLFormalParameter *param;
  
  elem = &umlclass->element;
  
  newumlclass = g_malloc(sizeof(UMLClass));
  newelem = &newumlclass->element;
  newobj = (Object *) newumlclass;

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

  newumlclass->attributes = NULL;
  list = umlclass->attributes;
  while (list != NULL) {
    attr = (UMLAttribute *)list->data;
    newumlclass->attributes = g_list_prepend(newumlclass->attributes,
					     uml_attribute_copy(attr));
    list = g_list_next(list);
  }

  newumlclass->operations = NULL;
  list = umlclass->operations;
  while (list != NULL) {
    op = (UMLOperation *)list->data;
    newumlclass->operations = g_list_prepend(newumlclass->operations,
					     uml_operation_copy(op));
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

  umlclass_update_data(newumlclass);
  
  return (Object *)newumlclass;
}


static void
umlclass_save(UMLClass *umlclass, int fd)
{
  UMLAttribute *attr;
  UMLOperation *op;
  UMLFormalParameter *formal_param;
  GList *list;
  
  element_save(&umlclass->element, fd);

  /* Class info: */
  write_string(fd, umlclass->name);
  write_string(fd, umlclass->stereotype);
  write_int32(fd, umlclass->abstract);
  write_int32(fd, umlclass->suppress_attributes);
  write_int32(fd, umlclass->suppress_operations);
  write_int32(fd, umlclass->visible_attributes);
  write_int32(fd, umlclass->visible_operations);

  /* Attribute info: */
  write_int32(fd, g_list_length(umlclass->attributes));
  list = umlclass->attributes;
  while (list != NULL) {
    attr = (UMLAttribute *) list->data;
    uml_attribute_write(fd, attr);
    list = g_list_next(list);
  }

  /* Operations info: */
  write_int32(fd, g_list_length(umlclass->operations));
  list = umlclass->operations;
  while (list != NULL) {
    op = (UMLOperation *) list->data;
    uml_operation_write(fd, op);
    list = g_list_next(list);
  }

  /* Template info: */
  write_int32(fd, umlclass->template);
  write_int32(fd, g_list_length(umlclass->formal_params));
  list = umlclass->formal_params;
  while (list != NULL) {
    formal_param = (UMLFormalParameter *) list->data;
    uml_formalparameter_write(fd, formal_param);
    list = g_list_next(list);
  }
}

static Object *umlclass_load(int fd, int version)
{
  UMLClass *umlclass;
  Element *elem;
  Object *obj;
  UMLAttribute *attr;
  UMLOperation *op;
  UMLFormalParameter *formal_param;
  int i;
  int num;
  
  umlclass = g_malloc(sizeof(UMLClass));
  elem = &umlclass->element;
  obj = (Object *) umlclass;
  
  obj->type = &umlclass_type;
  obj->ops = &umlclass_ops;

  element_load(elem, fd);
  
  /* Class info: */
  umlclass->name = read_string(fd);
  umlclass->stereotype = read_string(fd);
  umlclass->abstract = read_int32(fd);
  umlclass->suppress_attributes = read_int32(fd);
  umlclass->suppress_operations = read_int32(fd);
  umlclass->visible_attributes = read_int32(fd);
  umlclass->visible_operations = read_int32(fd);

  /* Attribute info: */
  num = read_int32(fd);
  umlclass->attributes = NULL;
  for (i=0;i<num;i++) {
    attr = uml_attribute_read(fd);
    umlclass->attributes = g_list_append(umlclass->attributes, attr);
  }

  /* Operations info: */
  num = read_int32(fd);
  umlclass->operations = NULL;
  for (i=0;i<num;i++) {
    op = uml_operation_read(fd);
    umlclass->operations = g_list_append(umlclass->operations, op);
  }

  /* Attribute info: */
  umlclass->template = read_int32(fd);
  num = read_int32(fd);
  umlclass->formal_params = NULL;
  for (i=0;i<num;i++) {
    formal_param = uml_formalparameter_read(fd);
    umlclass->formal_params = g_list_append(umlclass->formal_params, formal_param);
  }

  element_init(elem, 8, 8);

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
  umlclass_update_data(umlclass);

  for (i=0;i<8;i++) {
    obj->handles[i]->type = HANDLE_NON_MOVABLE;
  }

  return (Object *)umlclass;
}




