/* -*- Mode: C; c-basic-offset: 4 -*- */
/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * dxf-import.c: dxf import filter for dia
 * Copyright (C) 2000 Steffen Macke
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
#include <string.h>
#include <math.h>
#include <glib.h>
#include <stdlib.h>
#include <locale.h>

#include "intl.h"
#include "message.h"
#include "geometry.h"
#include "render.h"
#include "filter.h"
#include "object.h"
#include "properties.h"

/* maximum line length */
#define DXF_LINE_LENGTH 255

typedef struct _DxfData
{
	char code[DXF_LINE_LENGTH];
	char value[DXF_LINE_LENGTH];
} DxfData;

gboolean import_dxf(gchar *filename, DiagramData *dia, void* user_data);
gboolean read_dxf_codes(FILE *filedxf, DxfData *data);
void read_entity_line_dxf(FILE *filedxf, DxfData *data, DiagramData *dia);
void read_entity_circle_dxf(FILE *filedxf, DxfData *data, DiagramData *dia);
void read_entity_ellipse_dxf(FILE *filedxf, DxfData *data, DiagramData *dia);
void read_entity_text_dxf(FILE *filedxf, DxfData *data, DiagramData *dia);
void read_table_layer_dxf(FILE *filedxf, DxfData *data, DiagramData *dia);
void read_section_tables_dxf(FILE *filedxf, DxfData *data, DiagramData *dia);
void read_section_entities_dxf(FILE *filedxf, DxfData *data, DiagramData *dia);
Layer *layer_find_by_name(char *layername, DiagramData *dia);
LineStyle get_dia_linestyle_dxf(char *dxflinestyle);

/* returns the layer with the given name */
/* TODO: merge this with other layer code? */
Layer *layer_find_by_name(char *layername, DiagramData *dia) {
	  Layer *matching_layer, *layer;
	  int i;
	
	  matching_layer = dia->active_layer;
    for (i=0; i<dia->layers->len; i++) {
      layer = (Layer *)g_ptr_array_index(dia->layers, i);
      if(strcmp(layer->name, layername) == 0) {
      	matching_layer = layer;
      	break;
      }
    }
    return matching_layer;
}

/* returns the matching dia linestyle for a given dxf linestyle */
/* if no matching style is found, LINESTYLE solid is returned as a default */
LineStyle get_dia_linestyle_dxf(char *dxflinestyle) {
	if(strcmp(dxflinestyle, "DASH") == 0) {
		return LINESTYLE_DASHED;
	}
	if(strcmp(dxflinestyle, "DASHDOT") == 0){
		return LINESTYLE_DASH_DOT;
	}
	if(strcmp(dxflinestyle, "DOT") == 0){
		return LINESTYLE_DOTTED;
	}
	return LINESTYLE_SOLID;
}

/* reads a line entity from the dxf file and creates a line object in dia*/
void read_entity_line_dxf(FILE *filedxf, DxfData *data, DiagramData *dia){
	int codedxf;
	char *old_locale;

	/* line data */
	Point start, end;

	ObjectType *otype = object_get_type("Standard - Line");  
	Handle *h1, *h2;
  
	Object *line_obj;
	Color line_colour = { 0.0, 0.0, 0.0 };
	Property props[5];
	real line_width = 0.1;
	LineStyle style = LINESTYLE_SOLID;
	Layer *layer = NULL;
		
	do {
		if(read_dxf_codes(filedxf, data) == FALSE){
			return;
		}
		codedxf = atoi(data->code);
		switch(codedxf){
			case 6:	 style = get_dia_linestyle_dxf(data->value);
					 break;		
 			case  8: layer = layer_find_by_name(data->value, dia);
					 break;
			case 10:
			    old_locale = setlocale(LC_NUMERIC, "C");
			    start.x = atof(data->value);
			    setlocale(LC_NUMERIC, "C");
					 break;
			case 11: 
			    old_locale = setlocale(LC_NUMERIC, "C");
			    end.x = atof(data->value);
			    setlocale(LC_NUMERIC, "C");
					 break;
			case 20: 
			    old_locale = setlocale(LC_NUMERIC, "C");
			    start.y = (-1)*atof(data->value);
			    setlocale(LC_NUMERIC, "C");
					 break;
			case 21: 
			    old_locale = setlocale(LC_NUMERIC, "C");
			    end.y = (-1)*atof(data->value);
			    setlocale(LC_NUMERIC, "C");
					 break;
			case 39: 
			    old_locale = setlocale(LC_NUMERIC, "C");
			    line_width = atof(data->value)/10.0;		 		 		 
			    setlocale(LC_NUMERIC, "C");
					 break;
		}
		
	}while(codedxf != 0);

	line_obj = otype->ops->create(&start, otype->default_user_data,
				      &h1, &h2);
	layer_add_object(layer, line_obj);
		
	props[0].name = "start_point";
	props[0].type = PROP_TYPE_POINT;
	PROP_VALUE_POINT(props[0]).x = start.x;
	PROP_VALUE_POINT(props[0]).y = start.y;
	props[1].name = "end_point";
	props[1].type = PROP_TYPE_POINT;
	PROP_VALUE_POINT(props[1]).x = end.x;
	PROP_VALUE_POINT(props[1]).y = end.y;

	props[2].name = "line_colour";
	props[2].type = PROP_TYPE_COLOUR;
	PROP_VALUE_COLOUR(props[2]) = line_colour;
	props[3].name = "line_width";
	props[3].type = PROP_TYPE_REAL;
	PROP_VALUE_REAL(props[3]) = line_width;
	props[4].name = "line_style";
	props[4].type = PROP_TYPE_LINESTYLE;
	PROP_VALUE_LINESTYLE(props[4]).style = style;
	PROP_VALUE_LINESTYLE(props[4]).dash = 1.0;

	line_obj->ops->set_props(line_obj, props, 5);
}

/* reads a circle entity from the dxf file and creates a circle object in dia*/
void read_entity_circle_dxf(FILE *filedxf, DxfData *data, DiagramData *dia){
	int codedxf;
	char *old_locale;
	
	/* circle data */
	Point center;
	real radius = 1.0;

	ObjectType *otype = object_get_type("Standard - Ellipse");  
	Handle *h1, *h2;
  
	Object *ellipse_obj;
	Color line_colour = { 0.0, 0.0, 0.0 };
	Property props[5];
	real line_width = 0.1;
  Layer *layer = NULL;
		
	do {
		if(read_dxf_codes(filedxf, data) == FALSE){
			return;
		}
		codedxf = atoi(data->code);
		switch(codedxf){
			case  8: layer = layer_find_by_name(data->value, dia);
					 break;
			case 10: 
			    old_locale = setlocale(LC_NUMERIC, "C");
			    center.x = atof(data->value);
			    setlocale(LC_NUMERIC, "C");
					 break;
			case 20: 
			    old_locale = setlocale(LC_NUMERIC, "C");
			    center.y = (-1)*atof(data->value);
			    setlocale(LC_NUMERIC, "C");
					 break;
			case 39: 
			    old_locale = setlocale(LC_NUMERIC, "C");
			    line_width = atof(data->value)/10.0;
			    setlocale(LC_NUMERIC, "C");
					 break;
			case 40: 
			    old_locale = setlocale(LC_NUMERIC, "C");
			    radius = atof(data->value);
			    setlocale(LC_NUMERIC, "C");
					 break;
		}
		
	}while(codedxf != 0);

	center.x -= radius;
	center.y -= radius;
	ellipse_obj = otype->ops->create(&center, otype->default_user_data,
				      &h1, &h2);
	layer_add_object(layer, ellipse_obj);
	
	props[0].name = "elem_corner";
	props[0].type = PROP_TYPE_POINT;
	PROP_VALUE_POINT(props[0]).x = center.x;
	PROP_VALUE_POINT(props[0]).y = center.y;
	props[1].name = "elem_width";
	props[1].type = PROP_TYPE_REAL;
	PROP_VALUE_REAL(props[1]) = radius * 2.0;
    props[2].name = "elem_height";
    props[2].type = PROP_TYPE_REAL;
    PROP_VALUE_REAL(props[2]) = radius * 2.0;
	props[3].name = "line_colour";
	props[3].type = PROP_TYPE_COLOUR;
	PROP_VALUE_COLOUR(props[3]) = line_colour;
	props[4].name = "line_width";
	props[4].type = PROP_TYPE_REAL;
	PROP_VALUE_REAL(props[4]) = line_width;

	ellipse_obj->ops->set_props(ellipse_obj, props, 5);
}

/* reads an ellipse entity from the dxf file and creates an ellipse object in dia*/
void read_entity_ellipse_dxf(FILE *filedxf, DxfData *data, DiagramData *dia){
	int codedxf;
	char *old_locale;
	
	/* ellipse data */
	Point center;
	real width = 1.0;
	real ratio_width_height = 1.0;

	ObjectType *otype = object_get_type("Standard - Ellipse");
	Handle *h1, *h2;

	Object *ellipse_obj;
	Color line_colour = { 0.0, 0.0, 0.0 };
	Property props[5];
	real line_width = 0.1;
  Layer *layer = NULL;
		
	do {
		if(read_dxf_codes(filedxf, data) == FALSE){
			return;
		}
		codedxf = atoi(data->code);
		switch(codedxf){
			case  8: layer = layer_find_by_name(data->value, dia);
					 break;
			case 10: 
			    old_locale = setlocale(LC_NUMERIC, "C");
			    center.x = atof(data->value);
			    setlocale(LC_NUMERIC, "C");
					 break;
			case 11: 
			    old_locale = setlocale(LC_NUMERIC, "C");
			    ratio_width_height = atof(data->value);
			    setlocale(LC_NUMERIC, "C");
					 break;
			case 20: 
			    old_locale = setlocale(LC_NUMERIC, "C");
			    center.y = (-1)*atof(data->value);
			    setlocale(LC_NUMERIC, "C");
					 break;
			case 39: 
			    old_locale = setlocale(LC_NUMERIC, "C");
			    line_width = atof(data->value)/10.0;
			    setlocale(LC_NUMERIC, "C");
					 break;
			case 40: 
			    old_locale = setlocale(LC_NUMERIC, "C");
			    width = atof(data->value) * 2;
			    setlocale(LC_NUMERIC, "C");
					 break;
		}
		
	}while(codedxf != 0);

	center.x -= width;
	center.y -= (width*ratio_width_height);
	ellipse_obj = otype->ops->create(&center, otype->default_user_data,
				      &h1, &h2);
	layer_add_object(layer, ellipse_obj);
	
	props[0].name = "elem_corner";
	props[0].type = PROP_TYPE_POINT;
	PROP_VALUE_POINT(props[0]).x = center.x;
	PROP_VALUE_POINT(props[0]).y = center.y;
	props[1].name = "elem_width";
	props[1].type = PROP_TYPE_REAL;
	PROP_VALUE_REAL(props[1]) = width;
  props[2].name = "elem_height";
  props[2].type = PROP_TYPE_REAL;
  PROP_VALUE_REAL(props[2]) = width * ratio_width_height;
	props[3].name = "line_colour";
	props[3].type = PROP_TYPE_COLOUR;
	PROP_VALUE_COLOUR(props[3]) = line_colour;
	props[4].name = "line_width";
	props[4].type = PROP_TYPE_REAL;
	PROP_VALUE_REAL(props[4]) = line_width;

	ellipse_obj->ops->set_props(ellipse_obj, props, 5);
}

void read_entity_text_dxf(FILE *filedxf, DxfData *data, DiagramData *dia) {
  int codedxf;
  char *old_locale;

	/* text data */
	Point location;
	real height = 10.0;
	Alignment textalignment = ALIGN_LEFT;
	char *textvalue = NULL;

	ObjectType *otype = object_get_type("Standard - Text");
	Handle *h1, *h2;

	Object *text_obj;
	Color text_colour = { 0.0, 0.0, 0.0 };
	Property props[5];
  Layer *layer = NULL;
		
	do {
		if(read_dxf_codes(filedxf, data) == FALSE){
			return;
		}
		codedxf = atoi(data->code);
		switch(codedxf){
			case  1: textvalue = g_strdup(data->value);
					 break;
			case  8: layer = layer_find_by_name(data->value, dia);
					 break;
			case 10: 
			    old_locale = setlocale(LC_NUMERIC, "C");
			    location.x = atof(data->value);
			    setlocale(LC_NUMERIC, "C");
					 break;
			case 20:
			    old_locale = setlocale(LC_NUMERIC, "C");
			    location.y = (-1)*atof(data->value);
			    setlocale(LC_NUMERIC, "C");
					 break;
		  case 40: height = atof(data->value);
		 			 break;
		  case 72: switch(atoi(data->value)){
		  				  	case 0: textalignment = ALIGN_LEFT;
		  				  		break;
		  				  	case 1: textalignment = ALIGN_CENTER;
		  				  		break;
		  				    case 2: textalignment = ALIGN_RIGHT;
		  				    	break;
		  					} 			
		}
		
	}while(codedxf != 0);

	text_obj = otype->ops->create(&location, otype->default_user_data,
				      &h1, &h2);
	layer_add_object(layer, text_obj);
	
	props[0].name = "text_alignment";
	props[0].type = PROP_TYPE_ENUM;
	PROP_VALUE_ENUM(props[0]) = textalignment;
	props[1].name = "text_height";
	props[1].type = PROP_TYPE_REAL;
	PROP_VALUE_REAL(props[1]) = height;
  props[2].name = "text";
  props[2].type = PROP_TYPE_STRING;
  PROP_VALUE_STRING(props[2]) = textvalue;
	props[3].name = "text_colour";
	props[3].type = PROP_TYPE_COLOUR;
	PROP_VALUE_COLOUR(props[3]) = text_colour;
	props[4].name="text_font";
	props[4].type = PROP_TYPE_FONT;
	PROP_VALUE_FONT(props[4]) = font_getfont("Courier");


	text_obj->ops->set_props(text_obj, props, 5);

}

/* reads the layer table from the dxf file and creates the layers */
void read_table_layer_dxf(FILE *filedxf, DxfData *data, DiagramData *dia){
	int codedxf;
	Layer *layer;
	
	do {
		if(read_dxf_codes(filedxf, data) == FALSE){
			return;
		}
		else {
			 codedxf = atoi(data->code);
			 if(codedxf == 2){
			 	layer = new_layer(g_strdup(data->value));
			 	data_add_layer(dia, layer);	
			 }
		}
	}while((codedxf != 0) || (strcmp(data->value, "ENDTAB") != 0));
}

/* reads the tables section of the dxf file */
void read_section_tables_dxf(FILE *filedxf, DxfData *data, DiagramData *dia) {
	int codedxf;
	
	if(read_dxf_codes(filedxf, data) == FALSE){
		return;
	}
	do {
		codedxf = atoi(data->code);
		if((codedxf == 0) && (strcmp(data->value, "LAYER") == 0)) {
			read_table_layer_dxf(filedxf, data, dia);
		}
		else {
		 	if(read_dxf_codes(filedxf, data) == FALSE){
				return;
	        }
		}		
	}while((codedxf != 0) || (strcmp(data->value, "ENDSEC") != 0));
}

/* reads the entities section of the dxf file */
void read_section_entities_dxf(FILE *filedxf, DxfData *data, DiagramData *dia) {
	int codedxf;
	
	if(read_dxf_codes(filedxf, data) == FALSE){
			return;		
	}
	codedxf = atoi(data->code);
	do {

   		if((codedxf == 0) && (strcmp(data->value, "LINE") == 0)) {
   			read_entity_line_dxf(filedxf, data, dia);
   		}
   		else if((codedxf == 0) && (strcmp(data->value, "CIRCLE") == 0)) {
   			read_entity_circle_dxf(filedxf, data, dia);
   		}
   		else if((codedxf == 0) && (strcmp(data->value, "ELLIPSE") == 0)) {
   			read_entity_ellipse_dxf(filedxf, data, dia);
   		}
   		else if((codedxf == 0) && (strcmp(data->value, "TEXT") == 0)) {
   			read_entity_text_dxf(filedxf, data, dia);
   		}

   		else {
   			if(read_dxf_codes(filedxf, data) == FALSE) {
   				return;
   			}
   		}
		codedxf = atoi(data->code);		
	}while((codedxf != 0) || (strcmp(data->value, "ENDSEC") != 0));
}

/* imports the given dxf-file, returns TRUE if successful */
gboolean import_dxf(gchar *filename, DiagramData *dia, void* user_data){
	FILE *filedxf;
	DxfData *data;
	int codedxf;
	
	filedxf = fopen(filename,"r");
	if(filedxf == NULL){
		message_error(_("Couldn't open: '%s' for reading.\n"), filename);
		return FALSE;
	}
	                  
	data = g_new(DxfData, 1);
	
	do {
		if(read_dxf_codes(filedxf, data) == FALSE) {
			g_free(data);
  			return FALSE;
		}
		else {
			 codedxf = atoi(data->code);
			 if(codedxf == 2) {
			 	if(strcmp(data->value, "ENTITIES") == 0) {
			 		read_section_entities_dxf(filedxf, data, dia);
			 	}
			 	else if(strcmp(data->value, "TABLES") == 0) {
			 		read_section_tables_dxf(filedxf, data, dia);
			 	}
			 }
		}
	}while((codedxf != 0) || (strcmp(data->value, "EOF") != 0));
	
	g_free(data);
	
	return TRUE;
}

/* reads a code/value pair from the DXF file */
gboolean read_dxf_codes(FILE *filedxf, DxfData *data){
	int i;
	char *c;
	
	if(fgets(data->code, DXF_LINE_LENGTH, filedxf) == NULL){
		return FALSE;
	}
	if(fgets(data->value, DXF_LINE_LENGTH, filedxf) == NULL){
		return FALSE;
	}
    c=data->value;
	for(i=0; i < DXF_LINE_LENGTH; i++){		
		if((c[i] == '\n')||(c[i] == '\r')){
			c[i] = 0;
			break;
		}
	}
	return TRUE;	
}

/* interface from filter.h */

static const gchar *extensions[] = {"dxf", NULL };
DiaImportFilter dxf_import_filter = {
	N_("Drawing Interchange File"),
	extensions,
	import_dxf
};
