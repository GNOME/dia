/* -*- Mode: C; c-basic-offset: 4 -*- */
/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * xfig-import.c: xfig import filter for dia
 * Copyright (C) 2001 Lars Clausen
 * based on the dxf-import code
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
 
#include <stdio.h>
/* Information used here is taken from the FIG Format 3.2 specification
   <URL:http://www.xfig.org/userman/fig-format.html>
   Some items left unspecified in the specifications are taken from the
   XFig source v. 3.2.3c
 */

#include <string.h>
#include <math.h>
#include <glib.h>
#include <stdlib.h>
#include <errno.h>
#include <locale.h>

#include "config.h"
#include "intl.h"
#include "message.h"
#include "geometry.h"
#include "render.h"
#include "filter.h"
#include "object.h"
#include "properties.h"
#include "../app/group.h"

#define BUFLEN 512
#define FIG_MAX_USER_COLORS 512
/* 1200 PPI */
#define FIG_UNIT 472.440944881889763779527559055118
/* 1/80 inch */
#define FIG_ALT_UNIT 31.496062992125984251968503937007

static Color fig_default_colors[32] =
{ { 0x00, 0x00, 0x00}, {0x00, 0x00, 0xff}, {0x00, 0xff, 0x00}, {0x00, 0xff, 0xff}, 
  { 0xff, 0x00, 0x00}, {0xff, 0x00, 0xff}, {0xff, 0xff, 0x00}, {0xff, 0xff, 0xff},
  { 0x00, 0x00, 0x8f}, {0x00, 0x00, 0xb0}, {0x00, 0x00, 0xd1}, {0x87, 0xcf, 0xff},
  { 0x00, 0x8f, 0x00}, {0x00, 0xb0, 0x00}, {0x00, 0xd1, 0x00}, {0x00, 0x8f, 0x8f},
  { 0x00, 0xb0, 0xb0}, {0x00, 0xd1, 0xd1}, {0x8f, 0x00, 0x00}, {0xb0, 0x00, 0x00},
  { 0xd1, 0x00, 0x00}, {0x8f, 0x00, 0x8f}, {0xb0, 0x00, 0xb0}, {0xd1, 0x00, 0xd1},
  { 0x7f, 0x30, 0x00}, {0xa1, 0x3f, 0x00}, {0xbf, 0x61, 0x00}, {0xff, 0x7f, 0x7f},
  { 0xff, 0xa1, 0xa1}, {0xff, 0xbf, 0xbf}, {0xff, 0xe1, 0xe1}, {0xff, 0xd7, 0x00}};
static Color fig_colors[FIG_MAX_USER_COLORS];
static char *fig_fonts[] =
{
    "Times-Roman",
    "Times-Italic",
    "Times-Bold",
    "Times-BoldItalic",
    "AvantGarde-Book",
    "AvantGarde-BookOblique",
    "AvantGarde-Demi",
    "AvantGarde-DemiOblique",
    "Bookman-Light",
    "Bookman-LightItalic",
    "Bookman-Demi",
    "Bookman-DemiItalic",
    "Courier",
    "Courier-Oblique",
    "Courier-Bold",
    "Courier-BoldOblique",
    "Helvetica",
    "Helvetica-Oblique",
    "Helvetica-Bold",
    "Helvetica-BoldOblique",
    "Helvetica-Narrow",
    "Helvetica-Narrow-Oblique",
    "Helvetica-Narrow-Bold",
    "Helvetica-Narrow-BoldOblique",
    "NewCenturySchoolbook-Roman",
    "NewCenturySchoolbook-Italic",
    "NewCenturySchoolbook-Bold",
    "NewCenturySchoolbook-BoldItalic",
    "Palatino-Roman",
    "Palatino-Italic",
    "Palatino-Bold",
    "Palatino-BoldItalic",
    "Symbol",
    "ZapfChancery-MediumItalic",
    "ZapfDingbats"
};
gboolean import_fig(gchar *filename, DiagramData *dia, void* user_data);


#if 0
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
#endif

static int
skip_comments(FILE *file) {
  int ch;
  char buf[BUFLEN];

  while (!feof(file)) {
    if ((ch = fgetc(file)) == EOF) {
      return FALSE;
    }
    
    if (ch == '\n') continue;
    else if (ch == '#') {
      do {
	if (fgets(buf, BUFLEN, file) == NULL) {
	  return FALSE;
	}
	if (buf[strlen(buf)-1] == '\n') return TRUE;
      } while (!feof(file));
      return FALSE;
    } else {
      ungetc(ch, file);
      return TRUE;
    }
  }
  return TRUE;
}

static Object *
create_standard_text(real xpos, real ypos, char *text,
		     DiagramData *dia) {
    ObjectType *otype = object_get_type("Standard - Text");
    Object *new_obj;
    Handle *h1, *h2;
    Point point;
    Property props[4];

    point.x = xpos;
    point.y = ypos;

    new_obj = otype->ops->create(&point, otype->default_user_data,
				 &h1, &h2);
    layer_add_object(dia->active_layer, new_obj);

    message_warning("Creating text '%s'\n", text);

    props[0].name = "text";
    props[0].type = PROP_TYPE_STRING;
    PROP_VALUE_STRING(props[0]) = strdup(text);
    new_obj->ops->set_props(new_obj, props, 1);

    return new_obj;
}

static Object *
create_standard_ellipse(real xpos, real ypos, real width, real height,
			DiagramData *dia) {
    ObjectType *otype = object_get_type("Standard - Ellipse");
    Object *new_obj;
    Handle *h1, *h2;
    Property props[4];
    Point point;

    message_warning("Creating standard ellipse at %lf, %lf, size %lf x %lf\n", 
		    xpos, ypos, width, height);

    point.x = xpos;
    point.y = ypos;

    new_obj = otype->ops->create(&point, otype->default_user_data,
				 &h1, &h2);
    layer_add_object(dia->active_layer, new_obj);
  
    props[0].name = "elem_corner";
    props[0].type = PROP_TYPE_POINT;
    PROP_VALUE_POINT(props[0]).x = xpos;
    PROP_VALUE_POINT(props[0]).y = ypos;
    props[1].name = "elem_width";
    props[1].type = PROP_TYPE_REAL;
    PROP_VALUE_REAL(props[1]) = width;
    props[2].name = "elem_height";
    props[2].type = PROP_TYPE_REAL;
    PROP_VALUE_REAL(props[2]) = height;

    new_obj->ops->set_props(new_obj, props, 3);

    return new_obj;
}


static Object *
create_standard_box(real xpos, real ypos, real width, real height,
		    DiagramData *dia) {
  ObjectType *otype = object_get_type("Standard - Box");
  Object *new_obj;
  Handle *h1, *h2;
  Point point;
  Property props[4];

  point.x = xpos;
  point.y = ypos;

  new_obj = otype->ops->create(&point, otype->default_user_data,
			       &h1, &h2);
  layer_add_object(dia->active_layer, new_obj);
  
  props[0].name = "elem_corner";
  props[0].type = PROP_TYPE_POINT;
  PROP_VALUE_POINT(props[0]).x = xpos;
  PROP_VALUE_POINT(props[0]).y = ypos;
  props[1].name = "elem_width";
  props[1].type = PROP_TYPE_REAL;
  PROP_VALUE_REAL(props[1]) = width;
  props[2].name = "elem_height";
  props[2].type = PROP_TYPE_REAL;
  PROP_VALUE_REAL(props[2]) = height;

  new_obj->ops->set_props(new_obj, props, 3);

  return new_obj;
}

static Object *
create_standard_image(real xpos, real ypos, real width, real height,
		      char *file, DiagramData *dia) {
    ObjectType *otype = object_get_type("Standard - Image");
    Object *new_obj;
    Handle *h1, *h2;
    Point point;
    Property props[4];

    point.x = xpos;
    point.y = ypos;

    new_obj = otype->ops->create(&point, otype->default_user_data,
				 &h1, &h2);
    layer_add_object(dia->active_layer, new_obj);
    
    props[0].name = "elem_corner";
    props[0].type = PROP_TYPE_POINT;
    PROP_VALUE_POINT(props[0]).x = xpos;
    PROP_VALUE_POINT(props[0]).y = ypos;
    props[1].name = "elem_width";
    props[1].type = PROP_TYPE_REAL;
    PROP_VALUE_REAL(props[1]) = width;
    props[2].name = "elem_height";
    props[2].type = PROP_TYPE_REAL;
    PROP_VALUE_REAL(props[2]) = height;
    props[3].name = "image_file";
    props[3].type = PROP_TYPE_FILE;
    PROP_VALUE_FILE(props[3]) = file;
    new_obj->ops->set_props(new_obj, props, 4);

    return new_obj;
}

static int
fig_read_n_points(FILE *file, int n, Point **points) {
  int i;
  Point *new_points;

  new_points = (Point*)g_malloc(sizeof(Point)*n);

  for (i = 0; i < n; i++) {
      int x,y;
      if (fscanf(file, " %d %d ", &x, &y) != 2) {
	  message_error(_("Error while reading %dth of %d points: %s\n"),
			i, n, strerror(errno));
	  free(new_points);
	  return FALSE;
      }
      new_points[i].x = x/FIG_UNIT;
      new_points[i].y = y/FIG_UNIT;
  }
  fscanf(file, "\n");
  *points = new_points;
  return TRUE;
}

static void
fig_read_arrow(FILE *file) {
    int arrow_type, style;
    real thickness, width, height;
    if (fscanf(file, "%d %d %lf %lf %lf\n",
	       &arrow_type, &style, &thickness, &width, &height) != 5) {
	message_error(_("Error while reading arrowhead\n"));
    }
}

static Color
fig_color(int color_index) {
    if (color_index == -1) return color_black; /* Default color */
    if (color_index < 32) return fig_default_colors[color_index];
    else return fig_colors[color_index-32];
}


static void
fig_fix_text(char *text) {
    int i, j;
    int asciival;

    for (i = 0, j = 0; text[i] != 0; i++, j++) {
	if (text[i] == '\\') {
	    sscanf(text+i+1, "%3o", &asciival);
	    text[j] = asciival;
	    i+=3;
	} else {
	    text[j] = text[i];
	}
    }
    /* Strip final newline */
    text[j-1] = 0;
    if (text[j-2] == '\001') {
	text[j-2] = 0;
    }
}

static char *
fig_read_text_line(FILE *file) {
    char *text_buf;
    int text_alloc, text_len;

    getc(file);
    text_alloc = 80;
    text_buf = (char *)g_malloc(text_alloc*sizeof(char));
    text_len = 0;
    while (fgets(text_buf+text_len, text_alloc-text_len, file) != NULL) {
	if (strlen(text_buf) < text_alloc-1) break;
	text_len = text_alloc;
	text_alloc *= 2;
	text_buf = (char *)g_realloc(text_buf, text_alloc*sizeof(char));
    }

    fig_fix_text(text_buf);

    return text_buf;
}

static Object *
fig_read_ellipse(FILE *file, DiagramData *dia) {
    int sub_type;
    int line_style;
    int thickness;
    int pen_color;
    int fill_color;
    int depth;
    int pen_style;
    int area_fill;
    real style_val;
    int direction;
    real angle;
    int center_x, center_y;
    int radius_x, radius_y;
    int start_x, start_y;
    int end_x, end_y;
    Property props[5];
    Object *newobj = NULL;
    int num_props = 0;

    if (fscanf(file, "%d %d %d %d %d %d %d %d %lf %d %lf %d %d %d %d %d %d %d %d\n",
	       &sub_type,
	       &line_style,
	       &thickness,
	       &pen_color,
	       &fill_color,
	       &depth,
	       &pen_style,
	       &area_fill,
	       &style_val,
	       &direction,
	       &angle,
	       &center_x, &center_y,
	       &radius_x, &radius_y,
	       &start_x, &start_y,
	       &end_x, &end_y) != 19) {
	message_error(_("Couldn't read ellipse info: %s\n"), strerror(errno));
	return NULL;
    }
    
    /* Curiously, the sub_type doesn't matter, as all info can be
       extracted this way */
    newobj = create_standard_ellipse((center_x-radius_x)/FIG_UNIT,
				     (center_y-radius_y)/FIG_UNIT,
				     (2*radius_x)/FIG_UNIT,
				     (2*radius_y)/FIG_UNIT,
				     dia);

    num_props = 0;
    if (line_style != -1) {
	props[num_props].name = "line_style";
	props[num_props].type = PROP_TYPE_ENUM;
	switch (line_style) {
	case 0:
	    PROP_VALUE_ENUM(props[num_props]) = LINESTYLE_SOLID;
	    break;
	case 1:
	    PROP_VALUE_ENUM(props[num_props]) = LINESTYLE_DASHED;
	    break;
	case 2:
	    PROP_VALUE_ENUM(props[num_props]) = LINESTYLE_DOTTED;
	    break;
	case 3:
	    PROP_VALUE_ENUM(props[num_props]) = LINESTYLE_DASH_DOT;
	    break;
	case 4:
	    PROP_VALUE_ENUM(props[num_props]) = LINESTYLE_DASH_DOT_DOT;
	    break;
	case 5:
	default:
	    message_error(_("Line style %d not supported\n"), line_style);
	    PROP_VALUE_ENUM(props[num_props]) = LINESTYLE_SOLID;
	}
	num_props++;
    }
    props[num_props].name = "line_width";
    props[num_props].type = PROP_TYPE_REAL;
    PROP_VALUE_REAL(props[num_props]) = thickness/FIG_ALT_UNIT;
    num_props++;
    props[num_props].name = "line_color";
    props[num_props].type = PROP_TYPE_COLOUR;
    PROP_VALUE_COLOUR(props[num_props]) = fig_color(pen_color);
    num_props++;
    props[num_props].name = "background_color";
    props[num_props].type = PROP_TYPE_COLOUR;
    PROP_VALUE_COLOUR(props[num_props]) = fig_color(pen_color);
    num_props++;
    /* Depth field */
    /* Pen style field (not used) */
    /* Area_fill field */
    /* Style_val (size of dots and dashes) in 1/80 inch */
    /* Direction (not used) */
    /* Angle -- can't rotate yet */

    newobj->ops->set_props(newobj, props, num_props);
    
    return newobj;
}

static Object *
fig_read_polyline(FILE *file, DiagramData *dia) {
     int sub_type;
     int line_style;
     int thickness;
     int pen_color;
     int fill_color;
     int depth;
     int pen_style;
     int area_fill;
     real style_val;
     int join_style;
     int cap_style;
     int radius;
     int forward_arrow;
     int backward_arrow;
     int npoints;
     Point *points;
     Property props[5];
     Object *newobj = NULL;
     int num_props = 0;
     int flipped = 0;
     char *image_file = NULL;

     if (fscanf(file, "%d %d %d %d %d %d %d %d %lf %d %d %d %d %d %d\n",
		&sub_type,
		&line_style,
		&thickness,
		&pen_color,
		&fill_color,
		&depth,
		&pen_style,
		&area_fill,
		&style_val,
		&join_style,
		&cap_style,
		&radius,
		&forward_arrow,
		&backward_arrow,
		&npoints) != 15) {
       message_error(_("Couldn't read polyline info: %s\n"), strerror(errno));
       return NULL;
     }

     if (forward_arrow == 1) {
       fig_read_arrow(file);
     }

     if (backward_arrow == 1) {
       fig_read_arrow(file);
     }

     if (sub_type == 5) { /* image has image name before npoints */
	                  /* Despite what the specs say */
	 if (fscanf(file, " %d", &flipped) != 1) {
	     message_error(_("Couldn't read flipped bit: %s\n"), strerror(errno));
	     return NULL;
	 }

	 image_file = fig_read_text_line(file);

     }

     if (!fig_read_n_points(file, npoints, &points)) {
       return NULL;
     }
     
     switch (sub_type) {
     case 2: /* box */
       newobj = create_standard_box(points[0].x, points[0].y,
				    points[2].x-points[0].x,
				    points[2].y-points[0].y, dia);
       break;
     case 4: /* arc-box */
	 newobj = create_standard_box(points[0].x, points[0].y,
				      points[2].x-points[0].x,
				      points[2].y-points[0].y, dia);
	 if (radius < 0) {
	     message_warning("Negative corner radius %lf\n", radius);
	 } else {
	     props[0].name = "corner_radius";
	     props[0].type = PROP_TYPE_REAL;
	     PROP_VALUE_REAL(props[0]) = radius/FIG_ALT_UNIT;
	     newobj->ops->set_props(newobj, props, 1);
	 }
       break;
     case 5: /* imported-picture bounding-box) */
	 newobj = create_standard_image(points[0].x, points[0].y,
					points[2].x-points[0].x,
					points[2].y-points[0].y,
					image_file, dia);
	 break;
     case 1: /* polyline */
     case 3: /* polygon */
	 /*
	   newobj = create_standard_polygon(points, dia);
	 */
     default: 
       message_error(_("Unknown polyline subtype: %d\n"), sub_type);
       return NULL;
     }
     g_free(image_file);
     num_props = 0;
     if (line_style != -1) {
	 props[num_props].name = "line_style";
	 props[num_props].type = PROP_TYPE_ENUM;
	 switch (line_style) {
	 case 0:
	     PROP_VALUE_ENUM(props[num_props]) = LINESTYLE_SOLID;
	     break;
	 case 1:
	     PROP_VALUE_ENUM(props[num_props]) = LINESTYLE_DASHED;
	     break;
	 case 2:
	     PROP_VALUE_ENUM(props[num_props]) = LINESTYLE_DOTTED;
	     break;
	 case 3:
	     PROP_VALUE_ENUM(props[num_props]) = LINESTYLE_DASH_DOT;
	     break;
	 case 4:
	     PROP_VALUE_ENUM(props[num_props]) = LINESTYLE_DASH_DOT_DOT;
	     break;
	 case 5:
	 default:
	     message_error(_("Line style %d not supported\n"), line_style);
	     PROP_VALUE_ENUM(props[num_props]) = LINESTYLE_SOLID;
	 }
	 num_props++;
     }
     props[num_props].name = "line_width";
     props[num_props].type = PROP_TYPE_REAL;
     PROP_VALUE_REAL(props[num_props]) = thickness/FIG_ALT_UNIT;
     num_props++;
     props[num_props].name = "line_color";
     props[num_props].type = PROP_TYPE_COLOUR;
     PROP_VALUE_COLOUR(props[num_props]) = fig_color(pen_color);
     num_props++;
     props[num_props].name = "background_color";
     props[num_props].type = PROP_TYPE_COLOUR;
     PROP_VALUE_COLOUR(props[num_props]) = fig_color(pen_color);
     num_props++;
     if (area_fill == -1) {
	 props[num_props].name = "show_background";
	 props[num_props].type = PROP_TYPE_BOOL;
	 PROP_VALUE_BOOL(props[num_props]) = FALSE;
	 num_props++;
     } else {
	 /* Do stuff with fill patterns */
     }
     /* Depth field */
     /* Pen style field (not used) */
     /* Style_val (size of dots and dashes) in 1/80 inch*/
     /* Join style */
     /* Cap style */
     newobj->ops->set_props(newobj, props, num_props);
     
     return newobj;
}

static Object *
fig_read_text(FILE *file, DiagramData *dia) {
    Point *points;
    Property props[5];
    int num_props = 0;
    Object *newobj = NULL;
    int sub_type;
    int color;
    int depth;
    int pen_style;
    int font;
    real font_size;
    real angle;
    int font_flags;
    real height;
    real length;
    int x, y;
    char *text_buf;

    if (fscanf(file, " %d %d %d %d %d %lf %lf %d %lf %lf %d %d",
	       &sub_type,
	       &color,
	       &depth,
	       &pen_style,
	       &font,
	       &font_size,
	       &angle,
	       &font_flags,
	       &height,
	       &length,
	       &x,
	       &y) != 12) {
	message_error(_("Couldn't read text info: %s\n"), strerror(errno));
	return NULL;
    }
    /* Skip one space exactly */
    text_buf = fig_read_text_line(file);

    newobj = create_standard_text(x/FIG_UNIT, y/FIG_UNIT, text_buf, dia);
    /*    g_free(text_buf);*/

    props[num_props].name = "text_color";
    props[num_props].type = PROP_TYPE_COLOUR;
    PROP_VALUE_COLOUR(props[num_props]) = fig_color(color);
    num_props++;
    props[num_props].name = "text_alignment";
    props[num_props].type = PROP_TYPE_ENUM;
    PROP_VALUE_ENUM(props[num_props]) = sub_type;
    num_props++;
    props[num_props].name = "text_height";
    props[num_props].type = PROP_TYPE_REAL;
    PROP_VALUE_REAL(props[num_props]) = font_size*2.54/72.0;
    num_props++;
    /* Can't do the angle */
    /* Height and length are ignored */
    /* Flags */
    /* Font */
    props[num_props].name = "text_font";
    props[num_props].type = PROP_TYPE_FONT;
    PROP_VALUE_FONT(props[num_props]) = font_getfont(fig_fonts[font]);
    num_props++;

    newobj->ops->set_props(newobj, props, num_props);

    return newobj;
}

static int
fig_read_object(FILE *file, DiagramData *dia) {
  int objecttype;
  static GSList *compound_stack = NULL;
  Object *item = NULL;
  Layer *layer = dia->active_layer;

  if (fscanf(file, "%d ", &objecttype) != 1) {
    if (!feof(file)) {
      message_error(_("Couldn't identify FIG object: %s\n"), strerror(errno));
    }
    return FALSE;
  }

  

  switch (objecttype) {
  case -6: { /* End of compound */
      /* Pop off compound_stack till we reach a null */
      Object *subitem;
      GList *items = NULL;

      if (compound_stack == NULL) {
	  message_error(_("Compound end outside compound\n"));
	  return FALSE;
      }

      /* Make group item with these items */
      do {
	  subitem = (Object *)compound_stack->data;
	  compound_stack = g_slist_next(compound_stack);
	  if (subitem != NULL) { /* Add object to group */
	      items = g_list_prepend(items, subitem);
	  }
      } while (subitem != NULL);
      item = group_create(items);
      layer_add_object(layer, item);
      break;
  }
  case 0: { /* Color pseudo-object. */
    int colornumber;
    int colorvalues;
    Color color;

    if (fscanf(file, " %d #%xd", &colornumber, &colorvalues) != 2) {
      if (!feof(file)) {
	message_error(_("Couldn't read color: %s\n"), strerror(errno));
      }
      return FALSE;
    }

    color.red = (colorvalues & 0x00ff0000)>>16;
    color.green = (colorvalues & 0x0000ff00)>>8;
    color.blue = colorvalues & 0x000000ff;

    fig_colors[colornumber-32] = color;
    break;
  }
  case 1: { /* Ellipse which is a generalization of circle. */
      item = fig_read_ellipse(file, dia);
      if (item == NULL) return FALSE;
      break;
  }
  case 2: /* Polyline which includes polygon and box. */
      item = fig_read_polyline(file, dia);
      /*      if (item == NULL) return FALSE;*/
      break;
  case 4: /* Text. */
      item = fig_read_text(file, dia);
      /*      if (item == NULL) return FALSE;*/
      break;
  case 6: {/* Compound object which is composed of one or more objects. */
      int dummy;
      if (fscanf(file, " %d %d %d %d\n", &dummy, &dummy, &dummy, &dummy) != 4) {
	  if (!feof(file)) {
	      message_error(_("Couldn't read group extend: %s\n"), strerror(errno));
	  }
	  return FALSE;
      }
      /* Group extends don't really matter */
      compound_stack = g_slist_prepend(compound_stack, NULL);
      break;
  }
  case 5: /* Arc. */
  case 3: /* Spline which includes closed/open control/interpolated spline. */
    message_warning(_("Object type %d not implemented yet\n"), objecttype);
    return FALSE;
  default:
    message_error(_("Unknown object type %d\n"), objecttype);
    return FALSE;
  }
  if (compound_stack != NULL) /* We're building a compound */
    compound_stack = g_slist_prepend(compound_stack, item);
  return TRUE;
}

static int
fig_read_line_choice(FILE *file, char *choice1, char *choice2) {
  char buf[BUFLEN];

  if (!fgets(buf, BUFLEN, file)) {
    return -1;
  }

  buf[strlen(buf)-1] = 0; /* Remove trailing newline */
  if (!strcmp(buf, choice1)) return 0;
  if (!strcmp(buf, choice2)) return 1;
  message_warning(_("`%s' is not one of `%s' or `%s'\n"), buf, choice1, choice2);
  return 0;
}

static int
fig_read_paper_size(FILE *file, DiagramData *dia) {
  char buf[BUFLEN];
  int paper;

  if (!fgets(buf, BUFLEN, file)) {
    message_error(_("Error reading paper size: %s\n"), strerror(errno));
    return FALSE;
  }

  buf[strlen(buf)-1] = 0; /* Remove trailing newline */
  if ((paper = find_paper(buf)) != -1) {
    get_paper_info(&dia->paper, paper);
    return TRUE;
  }

  message_warning("Unknown paper size `%s', using default\n", buf);
  return TRUE;
}

int figversion;

static int
fig_read_meta_data(FILE *file, DiagramData *dia) {
  if (figversion >= 300) { /* Might exist earlier */
    int portrait;

    if ((portrait = fig_read_line_choice(file, "Portrait", "Landscape")) == -1) {
      message_error(_("Error reading paper orientation: %s\n"), strerror(errno));
      return FALSE;
    }
    dia->paper.is_portrait = portrait;
  }

  if (figversion >= 300) { /* Might exist earlier */
    int justify;

    if ((justify = fig_read_line_choice(file, "Center", "Flush Left")) == -1) {
      message_error(_("Error reading justification: %s\n"), strerror(errno));
      return FALSE;
    }
    /* Don't know what to do with this */
  }

  if (figversion >= 300) { /* Might exist earlier */
    int units;

    if ((units = fig_read_line_choice(file, "Metric", "Inches")) == -1) {
      message_error(_("Error reading units: %s\n"), strerror(errno));
      return FALSE;
    }
    /* Don't know what to do with this */
  }

  if (figversion >= 302) {
    if (!fig_read_paper_size(file, dia)) return FALSE;
  }

  {
    real mag;

    if (fscanf(file, "%lf\n", &mag) != 1) {
      message_error(_("Error reading magnification: %s\n"), strerror(errno));
      return FALSE;
    }
    
    dia->paper.scaling = mag/100;
  }

  if (figversion >= 302) {
    int multiple;

    if ((multiple = fig_read_line_choice(file, "Single", "Multiple")) == -1) {
      message_error(_("Error reading multipage indicator: %s\n"), strerror(errno));
      return FALSE;
    }

    /* Don't know what to do with this */
  }

  {
    int transparent;

    if (fscanf(file, "%d\n", &transparent) != 1) {
      message_error(_("Error reading transparent color: %s\n"), strerror(errno));
      return FALSE;
    }
    
    /* Don't know what to do with this */
  }

  if (!skip_comments(file)) {
    if (!feof(file)) {
      message_error(_("Error reading FIG file: %s\n"), strerror(errno));
    } else {
      message_error(_("Premature end of FIG file\n"), strerror(errno));
    }
    return FALSE;
  }

  {
    int resolution, coord_system;

    if (fscanf(file, "%d %d\n", &resolution, &coord_system) != 2) {
      message_error(_("Error reading resolution: %s\n"), strerror(errno));
      return FALSE;
    }
    
    /* Don't know what to do with this */
  }
  return TRUE;
}

/* imports the given fig-file, returns TRUE if successful */
gboolean 
import_fig(gchar *filename, DiagramData *dia, void* user_data) {
  FILE *figfile;
  char buf[BUFLEN];
  int figmajor, figminor;	
  int i;

  for (i = 0; i < FIG_MAX_USER_COLORS; i++) {
    fig_colors[i] = color_black;
  }

  figfile = fopen(filename,"r");
  if(figfile == NULL){
    message_error(_("Couldn't open: '%s' for reading.\n"), filename);
    return FALSE;
  }
  
  /* First check magic bytes */
  if (fscanf(figfile, "#FIG %d.%d\n", &figmajor, &figminor) != 2) {
    message_error(_("Doesn't look like a Fig file: %s\n"), strerror(errno));
    fclose(figfile);
    return FALSE;
  }
	
  if (figmajor != 3 || figminor != 2) {
    message_warning(_("This is a FIG version %d.%d file, I may not understand it\n"), figmajor, figminor);
  }

  figversion = figmajor*100+figminor;

  if (!skip_comments(figfile)) {
    if (!feof(figfile)) {
      message_error(_("Error reading FIG file: %s\n"), strerror(errno));
    } else {
      message_error(_("Premature end of FIG file\n"), strerror(errno));
    }
    fclose(figfile);
    return FALSE;
  }

  if (!fig_read_meta_data(figfile, dia)) {
    fclose(figfile);
    return TRUE;
  }
  
  do {
    if (!fig_read_object(figfile, dia)) {
      fclose(figfile);
      return feof(figfile);
    }
  } while (TRUE);
}

/* interface from filter.h */

static const gchar *extensions[] = {"fig", NULL };
DiaImportFilter xfig_import_filter = {
	N_("XFig File Format"),
	extensions,
	import_fig
};
