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
#include "propinternals.h"

static real coord_scale = 5.0;
static real width_scale = 10.0;

/* maximum line length */
#define DXF_LINE_LENGTH 256

typedef struct _DxfData
{
    char code[DXF_LINE_LENGTH];
    char value[DXF_LINE_LENGTH];
} DxfData;

gboolean import_dxf(const gchar *filename, DiagramData *dia, void* user_data);
gboolean read_dxf_codes(FILE *filedxf, DxfData *data);
void read_entity_line_dxf(FILE *filedxf, DxfData *data, DiagramData *dia);
void read_entity_circle_dxf(FILE *filedxf, DxfData *data, DiagramData *dia);
void read_entity_ellipse_dxf(FILE *filedxf, DxfData *data, DiagramData *dia);
void read_entity_arc_dxf(FILE *filedxf, DxfData *data, DiagramData *dia);
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

static PropDescription dxf_prop_descs[] = {
    { "start_point", PROP_TYPE_POINT },
    { "end_point", PROP_TYPE_POINT },
    { "line_colour", PROP_TYPE_COLOUR },
    { "line_width", PROP_TYPE_REAL },
    { "line_style", PROP_TYPE_LINESTYLE},
    PROP_DESC_END};

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
    GPtrArray *props;
    PointProperty *ptprop;
    LinestyleProperty *lsprop;
    ColorProperty *cprop;
    RealProperty *rprop;

    real line_width = 0.1;
    LineStyle style = LINESTYLE_SOLID;
    Layer *layer = NULL;
    
    old_locale = setlocale(LC_NUMERIC, "C");
    do {
        if(read_dxf_codes(filedxf, data) == FALSE){
            setlocale(LC_NUMERIC, old_locale);
            return;
        }
        codedxf = atoi(data->code);
        switch(codedxf){
        case 6:	 style = get_dia_linestyle_dxf(data->value);
            break;		
        case  8: layer = layer_find_by_name(data->value, dia);
            break;
        case 10:
            start.x = atof(data->value) / coord_scale;
            break;
        case 11: 
            end.x = atof(data->value) / coord_scale;
            break;
        case 20: 
            start.y = (-1)*atof(data->value) / coord_scale;
            break;
        case 21: 
            end.y = (-1)*atof(data->value) / coord_scale;
            break;
        case 39: 
            line_width = atof(data->value) / width_scale;
            break;
        }	
    } while(codedxf != 0);
    setlocale(LC_NUMERIC, old_locale);

    line_obj = otype->ops->create(&start, otype->default_user_data,
                                  &h1, &h2);
    layer_add_object(layer, line_obj);
		
    props = prop_list_from_descs(dxf_prop_descs,pdtpp_true);
    g_assert(props->len == 5);

    ptprop = g_ptr_array_index(props,0);
    ptprop->point_data = start;

    ptprop = g_ptr_array_index(props,1);
    ptprop->point_data = end;

    cprop = g_ptr_array_index(props,2);
    cprop->color_data = line_colour;

    rprop = g_ptr_array_index(props,3);
    rprop->real_data = line_width;

    lsprop = g_ptr_array_index(props,4);
    lsprop->style = style;
    lsprop->dash = 1.0;

    line_obj->ops->set_props(line_obj, props);

    prop_list_free(props);
}

static PropDescription dxf_ellipse_prop_descs[] = {
    { "elem_corner", PROP_TYPE_POINT },
    { "elem_width", PROP_TYPE_REAL },
    { "elem_height", PROP_TYPE_REAL },
    { "line_colour", PROP_TYPE_COLOUR },
    { "line_width", PROP_TYPE_REAL },
    { "show_background", PROP_TYPE_BOOL},
    PROP_DESC_END};

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

    PointProperty *ptprop;
    RealProperty *rprop;
    BoolProperty *bprop;
    ColorProperty *cprop;
    GPtrArray *props;

    real line_width = 0.1;
    Layer *layer = NULL;
    
    old_locale = setlocale(LC_NUMERIC, "C");
    do {
        if(read_dxf_codes(filedxf, data) == FALSE){
            setlocale(LC_NUMERIC, old_locale);
            return;
        }
        codedxf = atoi(data->code);
        switch(codedxf){
        case  8: 
            layer = layer_find_by_name(data->value, dia);
            break;
        case 10: 
            center.x = atof(data->value) / coord_scale;
            break;
        case 20: 
            center.y = (-1)*atof(data->value) / coord_scale;
            break;
        case 39: 
            line_width = atof(data->value) / width_scale;
            break;
        case 40: 
            radius = atof(data->value) / coord_scale;
            break;
        }
        
    } while(codedxf != 0);
    setlocale(LC_NUMERIC, old_locale);
 
    center.x -= radius;
    center.y -= radius;
    ellipse_obj = otype->ops->create(&center, otype->default_user_data,
                                     &h1, &h2);
    layer_add_object(layer, ellipse_obj);
	
    props = prop_list_from_descs(dxf_ellipse_prop_descs,pdtpp_true);
    g_assert(props->len == 6);

    ptprop = g_ptr_array_index(props,0);
    ptprop->point_data = center;
    rprop = g_ptr_array_index(props,1);
    rprop->real_data = radius * 2.0;
    rprop = g_ptr_array_index(props,2);
    rprop->real_data = radius * 2.0;
    cprop = g_ptr_array_index(props,3);
    cprop->color_data = line_colour;
    rprop = g_ptr_array_index(props,4);
    rprop->real_data = line_width;
    bprop = g_ptr_array_index(props,5);
    bprop->bool_data = FALSE;
    
    ellipse_obj->ops->set_props(ellipse_obj, props);
    prop_list_free(props);
}

static PropDescription dxf_arc_prop_descs[] = {
    { "start_point", PROP_TYPE_POINT },
    { "end_point", PROP_TYPE_POINT },
    { "curve_distance", PROP_TYPE_REAL },
    { "line_colour", PROP_TYPE_COLOUR },
    { "line_width", PROP_TYPE_REAL },
    PROP_DESC_END};

/* reads a circle entity from the dxf file and creates a circle object in dia*/
void read_entity_arc_dxf(FILE *filedxf, DxfData *data, DiagramData *dia){
    int codedxf;
    char *old_locale;
    
    /* arc data */
    Point start,center,end;
    real radius = 1.0, start_angle = 0.0, end_angle=360.0;
    real curve_distance;

    ObjectType *otype = object_get_type("Standard - Arc");  
    Handle *h1, *h2;
  
    Object *arc_obj;
    Color line_colour = { 0.0, 0.0, 0.0 };

    ColorProperty *cprop;
    PointProperty *ptprop;
    RealProperty *rprop;
    GPtrArray *props;

    real line_width = 0.1;
    Layer *layer = NULL;
		
    old_locale = setlocale(LC_NUMERIC, "C");
    do {
        if(read_dxf_codes(filedxf, data) == FALSE){
            setlocale(LC_NUMERIC,old_locale);
            return;
        }
        codedxf = atoi(data->code);
        switch(codedxf){
        case  8: 
            layer = layer_find_by_name(data->value, dia);
            break;
        case 10: 
            center.x = atof(data->value) / coord_scale;
            break;
        case 20: 
            center.y = (-1)*atof(data->value) / coord_scale;
            break;
        case 39: 
            line_width = atof(data->value) / width_scale;
            break;
        case 40: 
            radius = atof(data->value) / coord_scale;
            break;
        case 50:
            start_angle = atof(data->value)*M_PI/180.0;
            break;
        case 51:
            end_angle = atof(data->value)*M_PI/180.0;
            break;
        }
    } while(codedxf != 0);
    setlocale(LC_NUMERIC, old_locale);

    /* printf("c.x=%f c.y=%f s",center.x,center.y); */
    start.x = center.x + cos(start_angle) * radius; 
    start.y = center.y - sin(start_angle) * radius;
    end.x = center.x + cos(end_angle) * radius;
    end.y = center.y - sin(end_angle) * radius;
    /*printf("s.x=%f s.y=%f e.x=%f e.y=%f\n",start.x,start.y,end.x,end.y);*/


    if (end_angle < start_angle) end_angle += 2.0*M_PI;
    curve_distance = radius * (1 - cos ((end_angle - start_angle)/2));

    /*printf("start_angle: %f end_angle: %f radius:%f  curve_distance:%f\n",
      start_angle,end_angle,radius,curve_distance);*/
   
    arc_obj = otype->ops->create(&center, otype->default_user_data,
                                     &h1, &h2);
    layer_add_object(layer, arc_obj);
    
    props = prop_list_from_descs(dxf_arc_prop_descs,pdtpp_true);
    g_assert(props->len == 5);

    ptprop = g_ptr_array_index(props,0);
    ptprop->point_data = start;
    ptprop = g_ptr_array_index(props,1);
    ptprop->point_data = end;
    rprop = g_ptr_array_index(props,2);
    rprop->real_data = curve_distance;
    cprop = g_ptr_array_index(props,3);
    cprop->color_data = line_colour;
    rprop = g_ptr_array_index(props,4);
    rprop->real_data = line_width;
    
    arc_obj->ops->set_props(arc_obj, props);
    prop_list_free(props);
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
    PointProperty *ptprop;
    RealProperty *rprop;
    BoolProperty *bprop;
    ColorProperty *cprop;
    GPtrArray *props;

    real line_width = 0.1;
    Layer *layer = NULL;
    
    old_locale = setlocale(LC_NUMERIC, "C");
    do {
        if(read_dxf_codes(filedxf, data) == FALSE){
            setlocale(LC_NUMERIC, old_locale);
            return;
        }
        codedxf = atoi(data->code);
        switch(codedxf){
        case  8: 
            layer = layer_find_by_name(data->value, dia);
            break;
        case 10: 
            center.x = atof(data->value) / coord_scale;
            break;
        case 11: 
            ratio_width_height = atof(data->value) / coord_scale;
            break;
        case 20: 
            center.y = (-1)*atof(data->value) / coord_scale;
            break;
        case 39: 
            line_width = atof(data->value) / width_scale;
            break;
        case 40: 
            width = atof(data->value) * 2; /* XXX what scale */
            break;
        }
    } while(codedxf != 0);
    setlocale(LC_NUMERIC, old_locale);
 
    center.x -= width;
    center.y -= (width*ratio_width_height);
    ellipse_obj = otype->ops->create(&center, otype->default_user_data,
                                     &h1, &h2);
    layer_add_object(layer, ellipse_obj);
        
    props = prop_list_from_descs(dxf_ellipse_prop_descs,pdtpp_true);
    g_assert(props->len == 6);

    ptprop = g_ptr_array_index(props,0);
    ptprop->point_data = center;
    rprop = g_ptr_array_index(props,1);
    rprop->real_data = width;
    rprop = g_ptr_array_index(props,2);
    rprop->real_data = width * ratio_width_height;
    cprop = g_ptr_array_index(props,3);
    cprop->color_data = line_colour;
    rprop = g_ptr_array_index(props,4);
    rprop->real_data = line_width;
    bprop = g_ptr_array_index(props,5);
    bprop->bool_data = FALSE;
    
    ellipse_obj->ops->set_props(ellipse_obj, props);
    prop_list_free(props);
}

static PropDescription dxf_text_prop_descs[] = {
    { "text", PROP_TYPE_TEXT },
    PROP_DESC_END};

void read_entity_text_dxf(FILE *filedxf, DxfData *data, DiagramData *dia)
{
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

    TextProperty *tprop;
    GPtrArray *props;

    Layer *layer = NULL;
    
    old_locale = setlocale(LC_NUMERIC, "C");
    do {
        if (read_dxf_codes(filedxf, data) == FALSE) {
            setlocale(LC_NUMERIC,old_locale);
            return;
        }
        codedxf = atoi(data->code);
        switch (codedxf) {
        case  1: textvalue = g_strdup(data->value);
            break;
        case  8: layer = layer_find_by_name(data->value, dia);
            break;
        case 10: 
            location.x = atof(data->value) / coord_scale;
            break;
        case 20:
            location.y = (-1)*atof(data->value) / coord_scale;
            break;
        case 40: 
            height = atof(data->value) / coord_scale;
            break;
        case 72: 
            switch(atoi(data->value)){
            case 0: textalignment = ALIGN_LEFT;
                break;
            case 1: textalignment = ALIGN_CENTER;
                break;
            case 2: textalignment = ALIGN_RIGHT;
                break;	
            }
        }
    } while(codedxf != 0);
    setlocale(LC_NUMERIC,old_locale);
  
    text_obj = otype->ops->create(&location, otype->default_user_data,
                                  &h1, &h2);
    layer_add_object(layer, text_obj);
    props = prop_list_from_descs(dxf_text_prop_descs,pdtpp_true);
    g_assert(props->len == 1);

    tprop = g_ptr_array_index(props,0);
    g_free(tprop->text_data);
    tprop->text_data = textvalue;
    tprop->attr.alignment = textalignment;
    tprop->attr.position.x = location.x;
    tprop->attr.position.y = location.y;
    tprop->attr.font = dia_font_new(BASIC_MONOSPACE_FONT,STYLE_NORMAL,height);
    tprop->attr.height = height;
        
    text_obj->ops->set_props(text_obj, props);
    prop_list_free(props);
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
    } while ((codedxf != 0) || (strcmp(data->value, "ENDTAB") != 0));
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
    } while ((codedxf != 0) || (strcmp(data->value, "ENDSEC") != 0));
}

/* reads the entities section of the dxf file */
void read_section_entities_dxf(FILE *filedxf, DxfData *data, DiagramData *dia) {
    int codedxf;
	
    if (read_dxf_codes(filedxf, data) == FALSE){
        return;		
    }
    codedxf = atoi(data->code);
    do {  
        if((codedxf == 0) && (strcmp(data->value, "LINE") == 0)) {
            read_entity_line_dxf(filedxf, data, dia);
        } else if((codedxf == 0) && (strcmp(data->value, "CIRCLE") == 0)) {
            read_entity_circle_dxf(filedxf, data, dia);
        } else if((codedxf == 0) && (strcmp(data->value, "ELLIPSE") == 0)) {
            read_entity_ellipse_dxf(filedxf, data, dia);
        } else if((codedxf == 0) && (strcmp(data->value, "TEXT") == 0)) {
            read_entity_text_dxf(filedxf, data, dia);
        } else if((codedxf == 0) && (strcmp(data->value, "ARC") == 0)) {
               read_entity_arc_dxf(filedxf,data,dia);
        } else {
            /* if (codedxf == 0) {
                g_warning("unknown DXF entity: %s",data->value);
                }*/
            if(read_dxf_codes(filedxf, data) == FALSE) {
                return;
            }
        }
        codedxf = atoi(data->code);		
    } while((codedxf != 0) || (strcmp(data->value, "ENDSEC") != 0));
}

/* imports the given dxf-file, returns TRUE if successful */
gboolean import_dxf(const gchar *filename, DiagramData *dia, void* user_data){
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




