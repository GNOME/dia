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
 
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <glib.h>
#include <stdlib.h>

#include "config.h"
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

gboolean import_dxf(gchar *filename, DiagramData *dia);
gboolean read_dxf_codes(FILE *filedxf, DxfData *data);
void read_entity_line_dxf(FILE *filedxf, DxfData *data, DiagramData *dia);
void read_table_layer_dxf(FILE *filedxf, DxfData *data, DiagramData *dia);
void read_section_tables_dxf(FILE *filedxf, DxfData *data, DiagramData *dia);
void read_section_entities_dxf(FILE *filedxf, DxfData *data, DiagramData *dia);

/* reads a line entity from the dxf file and creates a line object in dia*/
void read_entity_line_dxf(FILE *filedxf, DxfData *data, DiagramData *dia){
	int codedxf;
	
	/* line data */
	Point start, end;

	ObjectType *otype = object_get_type("Standard - Line");  
	Handle *h1, *h2;
  
	Object *line_obj;
	Color line_colour = { 1.0, 0.0, 0.0 };
	Property props[4];

		
	do {
		if(read_dxf_codes(filedxf, data) == FALSE){
			return;
		}
		codedxf = atoi(data->code);
		switch(codedxf){
			case 10: start.x = atof(data->value);
					 break;
			case 20: start.y = atof(data->value);
					 break;
			case 11: end.x = atof(data->value);
					 break;
			case 21: end.y = atof(data->value);
					 break;		 		 
		}
		
	}while(codedxf != 0);

	line_obj = otype->ops->create(&start, otype->default_user_data,
				      &h1, &h2);
	layer_add_object(dia->active_layer, line_obj);

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
	PROP_VALUE_REAL(props[3]) = 0.4;

	line_obj->ops->set_props(line_obj, props, 4);
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
	printf("entering entities section\n");
	do {
		if(read_dxf_codes(filedxf, data) == FALSE){
			return;
		}
		else {
			 codedxf = atoi(data->code);
			 if((codedxf == 0) && (strcmp(data->value, "LINE") == 0)) {
			 	read_entity_line_dxf(filedxf, data, dia);
			 }
		}
	}while((codedxf != 0) || (strcmp(data->value, "ENDSEC") != 0));
}

/* imports the given dxf-file, returns TRUE if successful */
gboolean import_dxf(gchar *filename, DiagramData *dia){
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
		if(c[i] == '\n'){
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
