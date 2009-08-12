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

#include <stdio.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <assert.h>
#include <math.h>
#include <string.h>
#include <glib.h>

#include <fcntl.h>
#include <string.h>
#include <glib.h>
#include <errno.h>

#include "intl.h"
#include "object.h"
#include "element.h"
#include "connectionpoint.h"
#include "diarenderer.h"
#include "attributes.h"
#include "text.h"
#include "widgets.h"
#include "message.h"
#include "connpoint_line.h"
#include "color.h"


#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xmlmemory.h>
#include "dia_xml_libxml.h"
#include "dia_xml.h"

#include "../../app/load_save.h"

#include "site.h"

#define DEFAULT_WIDTH  1.0
#define DEFAULT_HEIGHT 1.0
#define TEXT_FONT (DIA_FONT_SANS|DIA_FONT_BOLD)
#define TEXT_FONT_HEIGHT 0.6
#define TEXT_HEIGHT (2.0)
#define NUM_CONNECTIONS 9

static Color color_green_clear = { 0.87, 0.98f, 0.91, 1.0f };

#ifdef G_OS_WIN32
#include <io.h>
#define mkstemp(s) _open(_mktemp(s), O_CREAT | O_TRUNC | O_WRONLY | _O_BINARY, 0644)
#define fchmod(f,m) (0)
#endif
#ifdef __EMX__
#define mkstemp(s) _open(_mktemp(s), O_CREAT | O_TRUNC | O_WRONLY | O_BINARY, 0644)
#define fchmod(f,m) (0)
#endif


static ObjectTypeOps site_type_ops =
{
  (CreateFunc) site_create,
  (LoadFunc)   site_load,
  (SaveFunc)   object_sissi_save,
  (GetDefaultsFunc)   NULL,
  (ApplyDefaultsFunc) NULL,
};

DiaObjectType site_type =
{
  "SISSI - site",           /* name */
  0,                            /* version */
  (char **) sissi_object_xpm, /* this is the default pixmap */
  &site_type_ops      /* ops */
};


static ObjectOps object_sissi_ops = {
  (DestroyFunc)         object_sissi_destroy,
  (DrawFunc)            object_sissi_draw,
  (DistanceFunc)        object_sissi_distance_from,
  (SelectFunc)          object_sissi_select,
  (CopyFunc)            object_sissi_copy_using_properties,
  (MoveFunc)            object_sissi_move,
  (MoveHandleFunc)      object_sissi_move_handle,
  (GetPropertiesFunc)   object_sissi_get_properties,
  (ApplyPropertiesDialogFunc) object_sissi_apply_props_from_dialog,
  (ObjectMenuFunc)      NULL,
  (DescribePropsFunc)   site_describe_props,
  (GetPropsFunc)        site_get_props,
  (SetPropsFunc)        site_set_props,
  (TextEditFunc) 0,
  (ApplyPropertiesListFunc) object_apply_props,
};

static PropDescription sissi_object_props[] = {
  ELEMENT_COMMON_PROPERTIES,
/*      { "confidentialite", PROP_TYPE_ENUM, PROP_FLAG_VISIBLE, N_("Confidentiality :"), N_("confidentiality "), property_confidentialite_data},*/
    PROP_DESC_END
};

static PropOffset sissi_object_offsets[] = {
  ELEMENT_COMMON_PROPERTIES_OFFSETS,
/*    { "confidentialite", PROP_TYPE_ENUM, offsetof(ObjetSISSI,confidentialite)},*/
  {NULL}
};

static PropDescription *site_describe_props(ObjetSISSI *site)
{
  if (sissi_object_props[0].quark == 0) {
    prop_desc_list_calculate_quarks(sissi_object_props);
  }
  return sissi_object_props;
}


static void site_set_props(ObjetSISSI *object_sissi, GPtrArray *props)
{
/*//   object_set_props_from_offsets(&object_sissi->element.object, sissi_object_offsets,props);

//   object_sissi_update_data(object_sissi, ANCHOR_MIDDLE, ANCHOR_MIDDLE);
*/
}

static void site_get_props(ObjetSISSI *object_sissi, GPtrArray *props)
{
  object_get_props_from_offsets(&object_sissi->element.object, sissi_object_offsets,props);
}

/***** create ***********/
static DiaObject *site_create(Point *startpoint,  void *user_data, Handle **handle1, Handle **handle2)
{
  ObjetSISSI *object_sissi;
  Element *elem;
  DiaObject *obj;
  int i,num;
  DiaFont* action_font;
  Point pos;
  xmlDocPtr doc;
  /* DiagramData *data; */
  xmlNodePtr diagramdata,composite;
  AttributeNode attr;
  SISSI_Property_Menace *properties_menaces;
  SISSI_Property *properties_others;
  Url_Docs *url_doc;

  
  object_sissi = g_malloc0(sizeof(ObjetSISSI));
  elem = &object_sissi->element;
  obj = &elem->object;

  obj->type = &site_type;
  obj->ops = &object_sissi_ops;

  elem->corner = *startpoint;
  elem->width = DEFAULT_WIDTH;
  elem->height = DEFAULT_HEIGHT;

 element_init(elem, 8, NUM_CONNECTIONS);

  for (i=0;i<NUM_CONNECTIONS;i++) {
    obj->connections[i] = &object_sissi->connections[i];
    object_sissi->connections[i].object = obj;
    object_sissi->connections[i].connected = NULL;
  }
  object_sissi->connections[8].flags = CP_FLAGS_MAIN;

   object_sissi->border_color=color_white;
   object_sissi->fill_colour=color_green_clear;
 
    if (object_sissi->image) {
      elem->width = (elem->width*(float)dia_image_width(object_sissi->image))/
	(float)dia_image_height(object_sissi->image);
    }
  object_sissi->type_element = g_strdup("site");

  object_sissi->entity=g_strdup("");
   object_sissi->entity_type=g_strdup("PHYSIC");

  if (GPOINTER_TO_INT(user_data)!=0) {	
    doc = sissi_read_object_from_xml(GPOINTER_TO_INT(user_data));
    if (!doc) {
      g_free(object_sissi);
      return NULL;
    }
    
    diagramdata = find_node_named (doc->xmlRootNode->xmlChildrenNode, "object");
    
	/* load paper information from diagram object section */
	attr = composite_find_attribute(diagramdata, "nb_others_fixes");
	if (attr != NULL) {
	object_sissi->nb_others_fixes = data_int ( attribute_first_data(attr) );
	}
	
	/**** read the other properties *******/
	attr = object_find_attribute(diagramdata, "properties");
	num = attribute_num_data(attr);
	composite = attribute_first_data(attr);
	object_sissi->properties_others = NULL;
	for (i=0;i<num;i++) {
	properties_others = property_other_read(composite);
	object_sissi->properties_others = g_list_append(object_sissi->properties_others, properties_others);
	composite = data_next(composite);
	}
	
	/******** read the menaces properties ************/
	attr = object_find_attribute(diagramdata, "menaces");
	num = attribute_num_data(attr);
	composite = attribute_first_data(attr);
	object_sissi->properties_menaces = NULL;
	for (i=0;i<num;i++) {
	properties_menaces = property_menace_read(composite);
	object_sissi->properties_menaces = g_list_append(object_sissi->properties_menaces, properties_menaces);
	composite = data_next(composite);
	}
	
	/********* read the docs *************/
	attr = object_find_attribute(diagramdata, "documentation");
	num = attribute_num_data(attr);
	composite = attribute_first_data(attr);
	object_sissi->url_docs = NULL;
	for (i=0;i<num;i++) {
	url_doc = url_doc_read(composite);
	object_sissi->url_docs = g_list_append(object_sissi->url_docs, url_doc);
	composite = data_next(composite);
	}
}

  action_font = dia_font_new_from_style(TEXT_FONT,TEXT_FONT_HEIGHT); 
  object_sissi->text = new_text("",action_font, TEXT_FONT_HEIGHT, &pos, &color_black, ALIGN_LEFT);


  object_sissi->nb_others_fixes=8;
  object_sissi_update_data(object_sissi, ANCHOR_MIDDLE, ANCHOR_MIDDLE);

  *handle1 = NULL;
  *handle2 = obj->handles[7];
  
   return &object_sissi->element.object;
}
 
DiaObject *site_load(ObjectNode obj_node, int version, const char *filename)
{
  ObjetSISSI *object_sissi;
  Element *elem;
  DiaObject *obj;
  DiaFont* action_font;
  Point pos;
  
  
  object_sissi = g_malloc0(sizeof(ObjetSISSI));
  elem = &object_sissi->element;
  obj = &elem->object;

  obj->type = &site_type;
  obj->ops = &object_sissi_ops;
  
  action_font = dia_font_new_from_style(TEXT_FONT,TEXT_FONT_HEIGHT); 
  object_sissi->text = new_text("",action_font, TEXT_FONT_HEIGHT, &pos, &color_black, ALIGN_LEFT);

  object_sissi=object_sissi_load(obj_node, version, filename, object_sissi,elem,obj);
  
   object_sissi->border_color=color_white;
   object_sissi->fill_colour=color_green_clear;
  
    if (object_sissi->image) {
      elem->width = (elem->width*(float)dia_image_width(object_sissi->image))/
	(float)dia_image_height(object_sissi->image);
    }

  object_sissi_update_data(object_sissi, ANCHOR_MIDDLE, ANCHOR_MIDDLE);

  return &object_sissi->element.object;

}
