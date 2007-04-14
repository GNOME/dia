/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * SISSI diagram -  adapted by Luc Cessieux
 * This class could draw the system security
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
#include "plug-ins.h"

#include "object.h"

#include "intl.h"
#include "sissi.h"
#include "sissi_dialog.h"

#include "element.h"
#include "connectionpoint.h"
#include "diarenderer.h"
#include "attributes.h"
#include "text.h"
#include "widgets.h"
#include "message.h"
#include "connpoint_line.h"
#include "color.h"
#include <string.h>


#define DEFAULT_WIDTH  1.0
#define DEFAULT_HEIGHT 1.0
#define TEXT_FONT (DIA_FONT_SANS|DIA_FONT_BOLD)
#define TEXT_FONT_HEIGHT 0.6
#define TEXT_HEIGHT (2.0)
#define NUM_CONNECTIONS 9


extern DiaObjectType sissi_object_type;
extern DiaObjectType room_type;
extern DiaObjectType faraday_type;
extern DiaObjectType area_type;
extern DiaObjectType site_type;

DIA_PLUGIN_CHECK_INIT

PluginInitResult
dia_plugin_init(PluginInfo *info)
{
  if (!dia_plugin_info_init(info, "SISSI", _("SISSI diagram"),
			    NULL, NULL))
    return DIA_PLUGIN_INIT_ERROR;

  object_register_type(&sissi_object_type);
  object_register_type(&room_type);
  object_register_type(&faraday_type);
  object_register_type(&area_type);
  object_register_type(&site_type);
      
  return DIA_PLUGIN_INIT_OK;
}

extern GtkWidget *object_sissi_get_properties_dialog(ObjetSISSI *object_sissi, gboolean is_default);
extern ObjectChange *object_sissi_apply_properties_dialog(ObjetSISSI *object_sissi);


void url_doc_write(AttributeNode attr_node, Url_Docs *url_docs)
{
  DataNode composite;

  composite = data_add_composite(attr_node, "url_doc");

  data_add_string(composite_add_attribute(composite, "label"), url_docs->label);
  data_add_string(composite_add_attribute(composite, "url"), url_docs->url);
  data_add_string(composite_add_attribute(composite, "description"), url_docs->description);

}

void property_menace_write(AttributeNode attr_node, SISSI_Property_Menace *properties_menaces)
{
  DataNode composite;

  composite = data_add_composite(attr_node, "property_menace");

  data_add_string(composite_add_attribute(composite, "label"), properties_menaces->label);
  data_add_real(composite_add_attribute(composite, "action"), properties_menaces->action);
  data_add_real(composite_add_attribute(composite, "detection"), properties_menaces->detection);
  data_add_real(composite_add_attribute(composite, "vulnerability"), properties_menaces->vulnerability);
  data_add_string(composite_add_attribute(composite, "comments"), properties_menaces->comments);
}

void property_other_write(AttributeNode attr_node, SISSI_Property *properties_other)
{
  DataNode composite;

  composite = data_add_composite(attr_node, "property_other");

  data_add_string(composite_add_attribute(composite, "label"), properties_other->label);
  data_add_string(composite_add_attribute(composite, "value"), properties_other->value);
  data_add_string(composite_add_attribute(composite, "description"), properties_other->description);
}

void property_url_write(AttributeNode attr_node, Url_Docs *url_docs)
{
  DataNode composite;

  composite = data_add_composite(attr_node, "property_url");

  data_add_string(composite_add_attribute(composite, "label"), url_docs->label);
  data_add_string(composite_add_attribute(composite, "url"), url_docs->url);
  data_add_string(composite_add_attribute(composite, "description"), url_docs->description);
}

void object_sissi_save(ObjetSISSI *object_sissi, ObjectNode obj_node, const char *filename)
{
  SISSI_Property_Menace *properties_menaces;
  SISSI_Property *properties_others;
  Url_Docs *url_doc;
  GList *list;
  AttributeNode menace_node;
  AttributeNode property_node;  
  AttributeNode doc_node;

  element_save(&object_sissi->element, obj_node);

  data_add_string(new_attribute(obj_node, "name"), object_sissi->name);
  data_add_int(new_attribute(obj_node, "subscribers"),object_sissi->subscribers);
  data_add_boolean(new_attribute(obj_node, "show_background"), object_sissi->show_background);
  data_add_boolean(new_attribute(obj_node, "draw_border"), object_sissi->draw_border);
  data_add_boolean(new_attribute(obj_node, "keep_aspect"), object_sissi->keep_aspect);
  data_add_color(new_attribute(obj_node, "fill_colour"), &object_sissi->fill_colour);
  data_add_color(new_attribute(obj_node, "line_colour"), &object_sissi->line_colour);
  data_add_color(new_attribute(obj_node, "border_color"), &object_sissi->border_color);
  data_add_real (new_attribute (obj_node, "radius"), object_sissi->radius);
  data_add_real (new_attribute (obj_node, "dashlength"), object_sissi->dashlength);
  data_add_real (new_attribute (obj_node, "border_width"), object_sissi->border_width);
  data_add_real (new_attribute (obj_node, "line_width"), object_sissi->line_width);
  data_add_string(new_attribute(obj_node, "file_image"), object_sissi->file);
  data_add_string(new_attribute(obj_node, "confidentiality"), object_sissi->confidentiality);
  data_add_string(new_attribute(obj_node, "integrity"), object_sissi->integrity);
  data_add_string(new_attribute(obj_node, "disponibility_level"), object_sissi->disponibility_level);
  data_add_real (new_attribute (obj_node, "disponibility_value"), object_sissi->disponibility_value);
  data_add_string(new_attribute(obj_node, "id_db"), object_sissi->id_db);
  data_add_string(new_attribute(obj_node, "entity"), object_sissi->entity);
  data_add_string(new_attribute(obj_node, "entity_type"), object_sissi->entity_type);
  data_add_string(new_attribute(obj_node, "type_element"), object_sissi->type_element);
  data_add_string(new_attribute(obj_node, "room"), object_sissi->room);
  data_add_string(new_attribute(obj_node, "site"), object_sissi->site);
  data_add_int(new_attribute(obj_node, "nb_others_fixes"),object_sissi->nb_others_fixes);  
    /******* save other properties *************/
  property_node = new_attribute(obj_node, "properties");
  list = object_sissi->properties_others;
  while (list != NULL) {
    properties_others = (SISSI_Property *) list->data;
    property_other_write(property_node, properties_others);
    list = g_list_next(list);
  }

  
  /******** save of menaces **********/
  menace_node = new_attribute(obj_node, "menaces");
  list = object_sissi->properties_menaces;
  while (list != NULL) {
    properties_menaces = (SISSI_Property_Menace *) list->data;
    property_menace_write(menace_node, properties_menaces);
    list = g_list_next(list);
  }
  
  /******** save of urls_doc *********/
  doc_node = new_attribute(obj_node, "documentation");
  list = object_sissi->url_docs;
  while (list != NULL) {
    url_doc = (Url_Docs *) list->data;
    property_url_write(doc_node, url_doc);
    list = g_list_next(list);
  }
}

 real object_sissi_distance_from(ObjetSISSI *object_sissi, Point *point)
{
  Element *elem = &object_sissi->element;
  Rectangle rect;
  rect.left = elem->corner.x - object_sissi->border_width;
  rect.right = elem->corner.x + elem->width + object_sissi->border_width;
  rect.top = elem->corner.y - object_sissi->border_width;
  rect.bottom = elem->corner.y + elem->height + object_sissi->border_width;
  return distance_rectangle_point(&rect, point);
}

 void object_sissi_select(ObjetSISSI *object_sissi, Point *clicked_point, DiaRenderer *interactive_renderer)
{
  element_update_handles(&object_sissi->element);
}

ObjectChange* object_sissi_move_handle(ObjetSISSI *object_sissi, Handle *handle, Point *to, ConnectionPoint *cp, HandleMoveReason reason, ModifierKeys modifiers)
{
  AnchorShape horiz = ANCHOR_MIDDLE, vert = ANCHOR_MIDDLE;
  assert(object_sissi!=NULL);
  assert(handle!=NULL);
  assert(to!=NULL);

  element_move_handle(&object_sissi->element, handle->id, to, cp, reason, modifiers);

  switch (handle->id) {
  case HANDLE_RESIZE_NW:
    horiz = ANCHOR_END; vert = ANCHOR_END; break;
  case HANDLE_RESIZE_N:
    vert = ANCHOR_END; break;
  case HANDLE_RESIZE_NE:
    horiz = ANCHOR_START; vert = ANCHOR_END; break;
  case HANDLE_RESIZE_E:
    horiz = ANCHOR_START; break;
  case HANDLE_RESIZE_SE:
    horiz = ANCHOR_START; vert = ANCHOR_START; break;
  case HANDLE_RESIZE_S:
    vert = ANCHOR_START; break;
  case HANDLE_RESIZE_SW:
    horiz = ANCHOR_END; vert = ANCHOR_START; break;
  case HANDLE_RESIZE_W:
    horiz = ANCHOR_END; break;
  default:
    break;
  }

  object_sissi_update_data(object_sissi, horiz, vert);

  return NULL;
}

 ObjectChange* object_sissi_move(ObjetSISSI *object_sissi, Point *to)
{
  object_sissi->element.corner = *to;

  object_sissi_update_data(object_sissi, ANCHOR_MIDDLE, ANCHOR_MIDDLE);
  return NULL;
}
/************ draw method ***********/
 void object_sissi_draw(ObjetSISSI *object_sissi, DiaRenderer *renderer)
{
 DiaRendererClass *renderer_ops = DIA_RENDERER_GET_CLASS (renderer);
 Point ul_corner, lr_corner, text_corner;
 Element *elem;
 real idfontheight;
 idfontheight=5.0;
  assert(object_sissi != NULL);
  assert(renderer != NULL);

  /********** computing positions ****************/
  elem = &object_sissi->element;

  lr_corner.x = elem->corner.x + elem->width + object_sissi->border_width/2;
  lr_corner.y = elem->corner.y + elem->height-TEXT_FONT_HEIGHT + object_sissi->border_width/2;
  
  ul_corner.x = elem->corner.x - object_sissi->border_width/2;
  ul_corner.y = elem->corner.y - object_sissi->border_width/2;

  text_corner.x = elem->corner.x - object_sissi->border_width/2;
  text_corner.y = elem->corner.y + elem->height + object_sissi->border_width/2;

  renderer_ops->draw_rect(renderer, 
		     &ul_corner,
		     &lr_corner, 
		     &object_sissi->border_color);
  
  renderer_ops->set_dashlength(renderer, object_sissi->dashlength);
  if (!object_sissi->show_background)
  {
   	renderer_ops->fill_rect(renderer, 
		       &elem->corner,
		       &lr_corner, 
		       &object_sissi->fill_colour);
  }
  if (object_sissi->image) {
    renderer_ops->draw_image(renderer, &elem->corner, elem->width, elem->height-TEXT_FONT_HEIGHT, object_sissi->image);
    renderer_ops->set_linejoin(renderer, LINEJOIN_MITER);
  } else {
/*
//     DiaImage broken = dia_image_get_broken();
//     renderer_ops->draw_image(renderer, &elem->corner, elem->width, elem->height, broken);
*/
  }
    text_set_position(object_sissi->text,&text_corner);
    text_set_string(object_sissi->text,object_sissi->name);
    text_draw(object_sissi->text, renderer);
}

 void object_sissi_update_data(ObjetSISSI *object_sissi, AnchorShape horiz, AnchorShape vert)
{
  Element *elem = &object_sissi->element;
  ElementBBExtras *extra = &elem->extra_spacing;
  DiaObject *obj = &elem->object;
  /* real width, height; */
  Point center, bottom_right;
/***   save starting points ***********/
  center = bottom_right = elem->corner;
  center.x += elem->width/2;
  bottom_right.x += elem->width;
  center.y += elem->height/2;
  bottom_right.y += elem->height;

  object_sissi->connections[0].pos = elem->corner;
  object_sissi->connections[1].pos.x = elem->corner.x + elem->width / 2.0;
  object_sissi->connections[1].pos.y = elem->corner.y;
  object_sissi->connections[2].pos.x = elem->corner.x + elem->width;
  object_sissi->connections[2].pos.y = elem->corner.y;
  object_sissi->connections[3].pos.x = elem->corner.x;
  object_sissi->connections[3].pos.y = elem->corner.y + (elem->height-TEXT_FONT_HEIGHT) / 2.0;
  object_sissi->connections[4].pos.x = elem->corner.x + elem->width;
  object_sissi->connections[4].pos.y = elem->corner.y + (elem->height-TEXT_FONT_HEIGHT) / 2.0;
  object_sissi->connections[5].pos.x = elem->corner.x;
  object_sissi->connections[5].pos.y = elem->corner.y + elem->height-TEXT_FONT_HEIGHT;
  object_sissi->connections[6].pos.x = elem->corner.x + elem->width / 2.0;
  object_sissi->connections[6].pos.y = elem->corner.y + elem->height-TEXT_FONT_HEIGHT;
  object_sissi->connections[7].pos.x = elem->corner.x + elem->width;
  object_sissi->connections[7].pos.y = elem->corner.y + elem->height-TEXT_FONT_HEIGHT;
  object_sissi->connections[8].pos.x = elem->corner.x + elem->width / 2.0;
  object_sissi->connections[8].pos.y = elem->corner.y + (elem->height-TEXT_FONT_HEIGHT) / 2.0;

  object_sissi->connections[0].directions = DIR_NORTH|DIR_WEST;
  object_sissi->connections[1].directions = DIR_NORTH;
  object_sissi->connections[2].directions = DIR_NORTH|DIR_EAST;
  object_sissi->connections[3].directions = DIR_WEST;
  object_sissi->connections[4].directions = DIR_EAST;
  object_sissi->connections[5].directions = DIR_SOUTH|DIR_WEST;
  object_sissi->connections[6].directions = DIR_SOUTH;
  object_sissi->connections[7].directions = DIR_SOUTH|DIR_EAST;
  object_sissi->connections[8].directions = DIR_ALL;

  switch (horiz) {
    case ANCHOR_MIDDLE:
      elem->corner.x = center.x - elem->width/2; break;
    case ANCHOR_END:
      elem->corner.x = bottom_right.x - elem->width; break;
    default:
      break;
  }

  switch (vert) {
    case ANCHOR_MIDDLE:
      elem->corner.y = center.y - elem->height/2; break;
    case ANCHOR_END:
      elem->corner.y = bottom_right.y - elem->height; break;
    default:
      break;
  }
  extra->border_trans = object_sissi->border_width / 2.0;
  element_update_boundingbox(elem);

  obj->position = elem->corner;
  
  element_update_handles(elem);
 }
 
 /*********** copy function **********/
 DiaObject *object_sissi_copy_using_properties(ObjetSISSI *object_sissi_origin)
 {
  ObjetSISSI *object_sissi;
  Element *elem,*elem_origin;
  DiaObject *obj;
  int i;
  GList *list;
  SISSI_Property_Menace *properties_menaces_aux;
  SISSI_Property *property_aux;
  Url_Docs *url_aux;
  DiaFont* action_font;
  Point defaultlen  = {1.0,0.0}, pos;
 
  elem_origin = &object_sissi_origin->element; 
  
  object_sissi = g_malloc0(sizeof(ObjetSISSI));
  elem = &object_sissi->element;
  obj = &elem->object;
  element_copy(elem_origin, elem);
  action_font = dia_font_new_from_style(TEXT_FONT,TEXT_FONT_HEIGHT); 
  object_sissi->text = new_text("",action_font, TEXT_FONT_HEIGHT, &pos, &color_black, ALIGN_LEFT);
  
  object_sissi->nb_others_fixes=object_sissi_origin->nb_others_fixes;

   object_sissi->border_color=object_sissi_origin->border_color;
   object_sissi->fill_colour=object_sissi_origin->fill_colour; 
   object_sissi->radius=object_sissi_origin->radius;
   object_sissi->subscribers=object_sissi_origin->subscribers; 
   object_sissi->show_background=object_sissi_origin->show_background;
   object_sissi->line_colour=object_sissi_origin->line_colour; 
   object_sissi->dashlength=object_sissi_origin->dashlength;
   object_sissi->line_width=object_sissi_origin->line_width; 
   object_sissi->border_width=object_sissi_origin->border_width;
   object_sissi->draw_border=object_sissi_origin->draw_border; 
   object_sissi->keep_aspect=object_sissi_origin->keep_aspect;

   object_sissi->confidentiality = g_strdup(object_sissi_origin->confidentiality);
   object_sissi->entity = g_strdup(object_sissi_origin->entity);
   object_sissi->entity_type = g_strdup(object_sissi_origin->entity_type);
   object_sissi->site = g_strdup(object_sissi_origin->site);
   object_sissi->room = g_strdup(object_sissi_origin->room);
   object_sissi->name = g_strdup(object_sissi_origin->name);
      
   object_sissi->type_element = g_strdup(object_sissi_origin->type_element);
   object_sissi->file =g_strdup(object_sissi_origin->file);
   object_sissi->image = dia_image_load(dia_get_data_directory(object_sissi->file));
  
  for (i=0;i<NUM_CONNECTIONS;i++) {
    obj->connections[i] = &object_sissi->connections[i];
    object_sissi->connections[i].object = obj;
    object_sissi->connections[i].connected = NULL;
  }
    
  /*** copy menaces list */
  object_sissi->properties_menaces = NULL;
  list = object_sissi_origin->properties_menaces;
  while (list != NULL) {
    properties_menaces_aux = (SISSI_Property_Menace *)list->data;   
    object_sissi->properties_menaces = g_list_prepend(object_sissi->properties_menaces, copy_menace(properties_menaces_aux->label,properties_menaces_aux->comments,properties_menaces_aux->action,properties_menaces_aux->detection,properties_menaces_aux->vulnerability));
    list = g_list_next(list);
  }
  
 /*********** copy the other propertie */
  object_sissi->properties_others = NULL;
  list = object_sissi_origin->properties_others;
  while (list != NULL) {
    property_aux = (SISSI_Property *)list->data;   
    object_sissi->properties_others = g_list_prepend(object_sissi->properties_others, create_new_property_other(property_aux->label,property_aux->description,property_aux->value));
    list = g_list_next(list);
  }
/*********** copy the list of documents */
  object_sissi->url_docs = NULL;
  list = object_sissi_origin->url_docs;
  while (list != NULL) {
    url_aux = (Url_Docs *)list->data;   
    object_sissi->properties_others = g_list_prepend(object_sissi->url_docs, create_new_url(url_aux->label,url_aux->description,url_aux->url));
    list = g_list_next(list);
  }
  
  object_sissi_update_data(object_sissi, ANCHOR_MIDDLE, ANCHOR_MIDDLE);
  
  return &object_sissi->element.object;
    
}
 
 void object_sissi_destroy(ObjetSISSI *object_sissi)
{
  g_free(object_sissi->file);  
  g_free(object_sissi->confidentiality);  
  g_free(object_sissi->integrity);  
  g_free(object_sissi->disponibility_level);
  g_free(object_sissi->entity);  
  g_free(object_sissi->entity_type);  
  g_free(object_sissi->type_element);  
  g_free(object_sissi->room);  
  g_free(object_sissi->site); 
  g_free(object_sissi->name); 
  
  if (object_sissi->properties_dialog!=NULL)
  	dialog_sissi_destroy(object_sissi->properties_dialog);
	
  object_sissi->properties_menaces=clear_list_property_menace(object_sissi->properties_menaces);
  object_sissi->properties_others=clear_list_property(object_sissi->properties_others);
  object_sissi->url_docs=clear_list_url_doc(object_sissi->url_docs); 
  element_destroy(&object_sissi->element);
}

void dialog_sissi_destroy(SISSIDialog *properties_dialog)
{
  gtk_widget_destroy(properties_dialog->dialog);
}

SISSI_Property_Menace *property_menace_read(DataNode composite)
{
  SISSI_Property_Menace *properties_menaces;
  AttributeNode attr_node;
  
  properties_menaces = g_malloc0(sizeof(SISSI_Property_Menace));

  properties_menaces->label = NULL;
  attr_node = composite_find_attribute(composite, "label");
  if (attr_node != NULL)
    properties_menaces->label =  data_string( attribute_first_data(attr_node) );

  properties_menaces->action = 0;
  attr_node = composite_find_attribute(composite, "action");
  if (attr_node != NULL)
     properties_menaces->action =  data_real( attribute_first_data(attr_node) );

  properties_menaces->detection = 0;
  attr_node = composite_find_attribute(composite, "detection");
  if (attr_node != NULL)
     properties_menaces->detection =  data_real( attribute_first_data(attr_node) );

  properties_menaces->vulnerability = 0;
  attr_node = composite_find_attribute(composite, "vulnerability");
  if (attr_node != NULL)
    properties_menaces->vulnerability =  data_real( attribute_first_data(attr_node) );

  properties_menaces->comments = NULL;
  attr_node = composite_find_attribute(composite, "comments");
  if (attr_node != NULL)
    properties_menaces->comments = data_string(attribute_first_data(attr_node));

  return properties_menaces;
}

SISSI_Property *property_other_read(DataNode composite)
{
  SISSI_Property *properties_others;
  AttributeNode attr_node;
  
  properties_others = g_malloc0(sizeof(SISSI_Property));

  properties_others->label = NULL;
  attr_node = composite_find_attribute(composite, "label");
  if (attr_node != NULL)
    properties_others->label = data_string( attribute_first_data(attr_node) );

  properties_others->value = NULL;
  attr_node = composite_find_attribute(composite, "value");
  if (attr_node != NULL)
    properties_others->value = data_string( attribute_first_data(attr_node) );

  properties_others->description = NULL;
  attr_node = composite_find_attribute(composite, "description");
  if (attr_node != NULL)
    properties_others->description = data_string( attribute_first_data(attr_node) );
  
  if (!strcmp(properties_others->description,""))
  	properties_others->description=NULL;
	
  return properties_others;
}

Url_Docs *url_doc_read(DataNode composite)
{
  Url_Docs *url_doc;
  AttributeNode attr_node;
  
  url_doc = g_malloc0(sizeof(SISSI_Property));

  url_doc->label = NULL;
  attr_node = composite_find_attribute(composite, "label");
  if (attr_node != NULL)
    url_doc->label =  data_string( attribute_first_data(attr_node) );

  url_doc->url = NULL;
  attr_node = composite_find_attribute(composite, "url");
  if (attr_node != NULL)
    url_doc->url =  data_string( attribute_first_data(attr_node) );

  url_doc->description = NULL;
  attr_node = composite_find_attribute(composite, "description");
  if (attr_node != NULL)
    url_doc->description =  data_string( attribute_first_data(attr_node) );

  return url_doc;
}


ObjetSISSI *object_sissi_load(ObjectNode obj_node, int version, const char *filename, ObjetSISSI *object_sissi,Element *elem,DiaObject *obj)
{
  SISSI_Property_Menace *properties_menaces;
  SISSI_Property *properties_others;
  Url_Docs *url_doc;
  AttributeNode attr_node;
  DataNode composite;
  int i;
  int num;

   elem = &object_sissi->element;
   obj = &elem->object;
  
   element_load(elem, obj_node);

    element_init(elem, 8, NUM_CONNECTIONS);
  for (i=0;i<NUM_CONNECTIONS;i++) {
      obj->connections[i] = &object_sissi->connections[i];
     object_sissi->connections[i].object = obj;
     object_sissi->connections[i].connected = NULL;
   }

  /* Initialisation of object */

  object_sissi->name = NULL;
  attr_node = object_find_attribute(obj_node, "name");
  if (attr_node != NULL)
    object_sissi->name =  data_string( attribute_first_data(attr_node) );
  
  object_sissi->file = NULL;
  attr_node = object_find_attribute(obj_node, "file_image");
  if (attr_node != NULL)
    object_sissi->file =  data_string( attribute_first_data(attr_node) );
 
  attr_node = object_find_attribute(obj_node, "subscribers");
  if (attr_node != NULL)
    object_sissi->subscribers = data_int(attribute_first_data(attr_node));

  object_sissi->show_background = FALSE;
  attr_node = object_find_attribute(obj_node, "show_background");
  if (attr_node != NULL)
    object_sissi->show_background = data_boolean(attribute_first_data(attr_node));

  object_sissi->show_background = FALSE;
  attr_node = object_find_attribute(obj_node, "draw_border");
  if (attr_node != NULL)
    object_sissi->draw_border = data_boolean(attribute_first_data(attr_node));

  object_sissi->keep_aspect = FALSE;
  attr_node = object_find_attribute(obj_node, "keep_aspect");
  if (attr_node != NULL)
    object_sissi->keep_aspect = data_boolean(attribute_first_data(attr_node));
 
  object_sissi->fill_colour = color_white;
  attr_node = object_find_attribute(obj_node, "fill_colour");
  if(attr_node != NULL)
    data_color(attribute_first_data(attr_node), &object_sissi->fill_colour); 
 
  object_sissi->line_colour = color_black;
  attr_node = object_find_attribute(obj_node, "line_colour");
  if(attr_node != NULL)
    data_color(attribute_first_data(attr_node), &object_sissi->line_colour); 

  object_sissi->border_color = color_black;
  attr_node = object_find_attribute(obj_node, "border_color");
  if(attr_node != NULL)
    data_color(attribute_first_data(attr_node), &object_sissi->border_color); 

  attr_node = object_find_attribute(obj_node, "radius");
  if (attr_node != NULL)
    object_sissi->radius = data_real(attribute_first_data(attr_node));
  
  attr_node = object_find_attribute(obj_node, "dashlength");
  if (attr_node != NULL)
    object_sissi->dashlength = data_real(attribute_first_data(attr_node));
  
  attr_node = object_find_attribute(obj_node, "border_width");
  if (attr_node != NULL)
    object_sissi->border_width = data_real(attribute_first_data(attr_node));
  
  attr_node = object_find_attribute(obj_node, "line_width");
  if (attr_node != NULL)
    object_sissi->line_width = data_real(attribute_first_data(attr_node));
    
  object_sissi->id_db = NULL;
  attr_node = object_find_attribute(obj_node, "id_db");
  if (attr_node != NULL)
    object_sissi->id_db =  data_string( attribute_first_data(attr_node) );
  
  object_sissi->confidentiality = NULL;
  attr_node = object_find_attribute(obj_node, "confidentiality");
  if (attr_node != NULL)
    object_sissi->confidentiality =  data_string( attribute_first_data(attr_node) );
  
  object_sissi->integrity = NULL;
  attr_node = object_find_attribute(obj_node, "integrity");
  if (attr_node != NULL)
    object_sissi->integrity =  data_string( attribute_first_data(attr_node) );
  
  object_sissi->disponibility_level = NULL;
  attr_node = object_find_attribute(obj_node, "disponibility_level");
  if (attr_node != NULL)
    object_sissi->disponibility_level =  data_string( attribute_first_data(attr_node) );
  
  attr_node = object_find_attribute(obj_node, "disponibility_value");
  if (attr_node != NULL)
    object_sissi->disponibility_value = data_real(attribute_first_data(attr_node));
  
  object_sissi->entity = NULL;
  attr_node = object_find_attribute(obj_node, "entity");
  if (attr_node != NULL)
    object_sissi->entity =  data_string( attribute_first_data(attr_node) );
  
  object_sissi->entity_type = NULL;
  attr_node = object_find_attribute(obj_node, "entity_type");
  if (attr_node != NULL)
    object_sissi->entity_type =  data_string( attribute_first_data(attr_node) );
  
  object_sissi->type_element = NULL;
  attr_node = object_find_attribute(obj_node, "type_element");
  if (attr_node != NULL)
    object_sissi->type_element =  data_string( attribute_first_data(attr_node) );
  
  object_sissi->room = NULL;
  attr_node = object_find_attribute(obj_node, "room");
  if (attr_node != NULL)
    object_sissi->room =  data_string( attribute_first_data(attr_node) );
  
  object_sissi->site = NULL;
  attr_node = object_find_attribute(obj_node, "site");
  if (attr_node != NULL)
    object_sissi->site =  data_string( attribute_first_data(attr_node) );

   attr_node = object_find_attribute(obj_node, "nb_others_fixes");
  if (attr_node != NULL)
    object_sissi->nb_others_fixes = data_int(attribute_first_data(attr_node));
 
  /**** read the other properties *******/
  attr_node = object_find_attribute(obj_node, "properties");
  num = attribute_num_data(attr_node);
  composite = attribute_first_data(attr_node);
  object_sissi->properties_others = NULL;
  for (i=0;i<num;i++) {
    properties_others = property_other_read(composite);
    object_sissi->properties_others = g_list_append(object_sissi->properties_others, properties_others);
    composite = data_next(composite);
  }

  
  /******** read the menaces properties ************/
  attr_node = object_find_attribute(obj_node, "menaces");
  num = attribute_num_data(attr_node);
  composite = attribute_first_data(attr_node);
  object_sissi->properties_menaces = NULL;
  for (i=0;i<num;i++) {
    properties_menaces = property_menace_read(composite);
    object_sissi->properties_menaces = g_list_append(object_sissi->properties_menaces, properties_menaces);
    composite = data_next(composite);
  }

  /********* read the docs *************/
  attr_node = object_find_attribute(obj_node, "documentation");
  num = attribute_num_data(attr_node);
  composite = attribute_first_data(attr_node);
  object_sissi->url_docs = NULL;
  for (i=0;i<num;i++) {
    url_doc = url_doc_read(composite);
    object_sissi->url_docs = g_list_append(object_sissi->url_docs, url_doc);
    composite = data_next(composite);
  }

  return object_sissi;
}

GtkWidget *object_sissi_get_properties(ObjetSISSI *object_sissi, gboolean is_default)
{
    return object_sissi_get_properties_dialog(object_sissi, is_default);
}

void object_sissi_apply_properties(ObjetSISSI *object_sissi)
{
    object_sissi_apply_properties_dialog(object_sissi);
}

SISSI_Property *create_new_property_other(gchar *label, gchar *description, gchar *value)
{
  SISSI_Property *property;
  property = g_new0(SISSI_Property, 1);

  if (property->label != NULL)
    g_free(property->label);
  if (label && label[0])
    property->label = g_strdup (label);
  else
    property->label = NULL;

  if (property->description != NULL)
    g_free(property->description);
  if (description && description[0])
    property->description = g_strdup (description);
  else
    property->description = NULL;

  if (property->value != NULL)
    g_free(property->value);
  if (value && value[0])
    property->value = g_strdup (value);
  else
    property->value = NULL;
  
  return property;
}

Url_Docs *create_new_url(gchar *label, gchar *description, gchar *url)
{
  Url_Docs *url_doc;
  url_doc = g_new0(Url_Docs, 1);

  if (url_doc->label != NULL)
    g_free(url_doc->label);
  if (label && label[0])
    url_doc->label = g_strdup (label);
  else
    url_doc->label = NULL;

  if (url_doc->description != NULL)
    g_free(url_doc->description);
  if (description && description[0])
    url_doc->description = g_strdup (description);
  else
    url_doc->description = NULL;

  if (url_doc->url != NULL)
    g_free(url_doc->url);
  if (url && url[0])
    url_doc->url = g_strdup (url);
  else
    url_doc->url = NULL;
  
  return url_doc;
}


GList *clear_list_property (GList *list)
{  
  while (list != NULL) {
    SISSI_Property *property = (SISSI_Property *) list->data;
    g_free(property->label);
    g_free(property->value);
    g_free(property->description);
    g_free(property);
    list = g_list_next(list);
  }
   g_list_free(list);
   list=NULL;
  return list;
}

GList *clear_list_url_doc (GList *list)
{  
  while (list != NULL) {
    Url_Docs *url_doc = (Url_Docs *) list->data;
    g_free(url_doc->label);
    g_free(url_doc->url);
    g_free(url_doc->description);
    g_free(url_doc);
    list = g_list_next(list);
  }
  g_list_free(list);
  list=NULL;
  return list;
}

xmlNodePtr
find_node_named (xmlNodePtr p, const char *name)
{
  while (p && 0 != strcmp(p->name, name))
    p = p->next;
  return p;
}
