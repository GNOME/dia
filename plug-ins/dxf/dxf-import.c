/* -*- Mode: C; c-basic-offset: 4 -*- */
/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * dxf-import.c: dxf import filter for dia
 * Copyright (C) 2000 Steffen Macke
 * Copyright (C) 2002 Angus Ainslie 
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <glib.h>
#include <glib/gstdio.h>

#include "intl.h"
#include "message.h"
#include "geometry.h"
#include "diarenderer.h"
#include "filter.h"
#include "object.h"
#include "properties.h"
#include "propinternals.h"
#include "autocad_pal.h"

#include "group.h"
#include "create.h"
#include "attributes.h"

static real coord_scale = 1.0, measure_scale = 1.0;
static real text_scale = 1.0;

#define WIDTH_SCALE (coord_scale * measure_scale)
#define DEFAULT_LINE_WIDTH 0.001

Point extent_min, extent_max;
Point limit_min, limit_max;

/* maximum line length */
#define DXF_LINE_LENGTH 256

typedef struct layer_data_tag 
{
   char layerName[256];
   int acad_colour;
} layer_data_type;

typedef struct _DxfData
{
    char code[DXF_LINE_LENGTH];
    char value[DXF_LINE_LENGTH];
} DxfData;

gboolean import_dxf(const gchar *filename, DiagramData *dia, void* user_data);
gboolean read_dxf_codes(FILE *filedxf, DxfData *data);
DiaObject *read_entity_line_dxf(FILE *filedxf, DxfData *data, DiagramData *dia);
DiaObject *read_entity_circle_dxf(FILE *filedxf, DxfData *data, DiagramData *dia);
DiaObject *read_entity_ellipse_dxf(FILE *filedxf, DxfData *data, DiagramData *dia);
DiaObject *read_entity_arc_dxf(FILE *filedxf, DxfData *data, DiagramData *dia);
DiaObject *read_entity_solid_dxf(FILE *filedxf, DxfData *data, DiagramData *dia);
DiaObject *read_entity_polyline_dxf(FILE *filedxf, DxfData *data, DiagramData *dia);
DiaObject *read_entity_text_dxf(FILE *filedxf, DxfData *data, DiagramData *dia);
void read_entity_measurement_dxf(FILE *filedxf, DxfData *data,
                                 DiagramData *dia);
void read_entity_scale_dxf(FILE *filedxf, DxfData *data, DiagramData *dia);
void read_entity_textsize_dxf(FILE *filedxf, DxfData *data, DiagramData *dia);
void read_entity_mesurement_dxf(FILE *filedxf, DxfData *data, DiagramData *dia);
void read_table_layer_dxf(FILE *filedxf, DxfData *data, DiagramData *dia);
void read_section_header_dxf(FILE *filedxf, DxfData *data, DiagramData *dia);
void read_section_classes_dxf(FILE *filedxf, DxfData *data, DiagramData *dia);
void read_section_tables_dxf(FILE *filedxf, DxfData *data, DiagramData *dia);
void read_section_entities_dxf(FILE *filedxf, DxfData *data, DiagramData *dia);
void read_section_blocks_dxf(FILE *filedxf, DxfData *data, DiagramData *dia);
Layer *layer_find_by_name(char *layername, DiagramData *dia);
LineStyle get_dia_linestyle_dxf(char *dxflinestyle);

/* returns the layer with the given name */
/* TODO: merge this with other layer code? */
Layer *
layer_find_by_name(char *layername, DiagramData *dia) 
{
    Layer *matching_layer, *layer;
    int i;
	
    matching_layer = NULL;
   
    for (i=0; i<dia->layers->len; i++) {
        layer = (Layer *)g_ptr_array_index(dia->layers, i);
        if(strcmp(layer->name, layername) == 0) {
            matching_layer = layer;
            break;
        }
    }

   if( matching_layer == NULL )
     {
	matching_layer = new_layer(g_strdup( layername ), dia);
	data_add_layer(dia, matching_layer);
     }

    return matching_layer;
}

/* returns the matching dia linestyle for a given dxf linestyle */
/* if no matching style is found, LINESTYLE solid is returned as a default */
LineStyle 
get_dia_linestyle_dxf(char *dxflinestyle) 
{
    if (strcmp(dxflinestyle, "DASHED") == 0)
        return LINESTYLE_DASHED;
    if (strcmp(dxflinestyle, "DASHDOT") == 0)
        return LINESTYLE_DASH_DOT;
    if (strcmp(dxflinestyle, "DOT") == 0)
        return LINESTYLE_DOTTED;
    if (strcmp(dxflinestyle, "DIVIDE") == 0)
        return LINESTYLE_DASH_DOT_DOT;

    return LINESTYLE_SOLID;
}

static PropDescription dxf_prop_descs[] = {
    { "start_point", PROP_TYPE_POINT },
    { "end_point", PROP_TYPE_POINT },
    { "line_colour", PROP_TYPE_COLOUR },
    { PROP_STDNAME_LINE_WIDTH, PROP_STDTYPE_LINE_WIDTH },
    { "line_style", PROP_TYPE_LINESTYLE},
    PROP_DESC_END};


/* reads a line entity from the dxf file and creates a line object in dia*/
DiaObject *
read_entity_line_dxf(FILE *filedxf, DxfData *data, DiagramData *dia)
{
    int codedxf;

    /* line data */
    Point start, end;
    
    DiaObjectType *otype = object_get_type("Standard - Line");  
    Handle *h1, *h2;
    
    DiaObject *line_obj;
    Color line_colour = { 0.0, 0.0, 0.0 };
    RGB_t color;
    GPtrArray *props;
    PointProperty *ptprop;
    LinestyleProperty *lsprop;
    ColorProperty *cprop;
    RealProperty *rprop;

    real line_width = DEFAULT_LINE_WIDTH;
    LineStyle style = LINESTYLE_SOLID;
    Layer *layer = dia->active_layer;
    
    end.x=0;
    end.y=0;

    do {
        if(read_dxf_codes(filedxf, data) == FALSE){
            return( NULL );
        }
        codedxf = atoi(data->code);
        switch(codedxf){
        case 6:	 style = get_dia_linestyle_dxf(data->value);
            break;		
        case  8: layer = layer_find_by_name(data->value, dia);
            break;
        case 10:
            start.x = g_ascii_strtod(data->value, NULL) * coord_scale * measure_scale;
            break;
        case 11: 
            end.x = g_ascii_strtod(data->value, NULL) * coord_scale * measure_scale;
            break;
        case 20: 
            start.y = (-1)*g_ascii_strtod(data->value, NULL) * coord_scale * measure_scale;
            break;
        case 21: 
            end.y = (-1)*g_ascii_strtod(data->value, NULL) * coord_scale * measure_scale;
            break;
        case 39: 
            line_width = g_ascii_strtod(data->value, NULL) * WIDTH_SCALE;
	   /*printf( "line width %f\n", line_width ); */
            break;
	case 62 :
            color = pal_get_rgb (atoi(data->value));
	    line_colour.red = color.r / 255.0;
	    line_colour.green = color.g / 255.0;
	    line_colour.blue = color.b / 255.0;
            break;
        }	
    } while(codedxf != 0);

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
   
   return( line_obj );
}

static PropDescription dxf_solid_prop_descs[] = {
     { "line_colour", PROP_TYPE_COLOUR },
     { PROP_STDNAME_LINE_WIDTH, PROP_STDTYPE_LINE_WIDTH },
     { "line_style", PROP_TYPE_LINESTYLE },
     { "fill_colour", PROP_TYPE_COLOUR },
     { "show_background", PROP_TYPE_BOOL },
   PROP_DESC_END};

/* reads a solid entity from the dxf file and creates a polygon object in dia*/
DiaObject *
read_entity_solid_dxf(FILE *filedxf, DxfData *data, DiagramData *dia)
{
   int codedxf;
   
   /* polygon data */
   Point p[4];
   
   DiaObjectType *otype = object_get_type("Standard - Polygon");  
   Handle *h1, *h2;
   
   DiaObject *polygon_obj;
   MultipointCreateData *pcd;

   Color fill_colour = { 0.5, 0.5, 0.5 };

   GPtrArray *props;
   LinestyleProperty *lsprop;
   ColorProperty *cprop, *fprop;
   RealProperty *rprop;
   BoolProperty *bprop;
   
    real line_width = 0.001;
    LineStyle style = LINESTYLE_SOLID;
    Layer *layer = dia->active_layer;
   RGB_t color;
   
/*   printf( "Solid " ); */
   
    do {
        if(read_dxf_codes(filedxf, data) == FALSE){
            return( NULL );
        }
        codedxf = atoi(data->code);
        switch(codedxf){
        case 6:	 
	   style = get_dia_linestyle_dxf(data->value);
	   break;		
        case  8: 
	   layer = layer_find_by_name(data->value, dia);
	   /*printf( "layer: %s ", data->value );*/
	   break;
        case 10:
	   p[0].x = g_ascii_strtod(data->value, NULL) * coord_scale * measure_scale;
	   /*printf( "P0.x: %f ", p[0].x );*/
	   break;
        case 11: 
            p[1].x = g_ascii_strtod(data->value, NULL) * coord_scale * measure_scale;
	   /*printf( "P1.x: %f ", p[1].x );*/
            break;
        case 12: 
            p[2].x = g_ascii_strtod(data->value, NULL) * coord_scale * measure_scale;
	   /*printf( "P2.x: %f ", p[2].x );*/
            break;
        case 13: 
            p[3].x = g_ascii_strtod(data->value, NULL) * coord_scale * measure_scale;
	   /*printf( "P3.x: %f ", p[3].x );*/
            break;
        case 20: 
            p[0].y = (-1)*g_ascii_strtod(data->value, NULL) * coord_scale * measure_scale;
	   /*printf( "P0.y: %f ", p[0].y );*/
            break;
        case 21: 
            p[1].y = (-1)*g_ascii_strtod(data->value, NULL) * coord_scale * measure_scale;
	   /*printf( "P1.y: %f ", p[1].y );*/
            break;
        case 22: 
            p[2].y = (-1)*g_ascii_strtod(data->value, NULL) * coord_scale * measure_scale;
	   /*printf( "P2.y: %f ", p[2].y );*/
            break;
        case 23: 
            p[3].y = (-1)*g_ascii_strtod(data->value, NULL) * coord_scale * measure_scale;
	   /*printf( "P3.y: %f\n", p[3].y );*/
            break;
        case 39: 
            line_width = g_ascii_strtod(data->value, NULL) * WIDTH_SCALE;
	   /*printf( "width %f\n", line_width );*/
            break;
        case 62: 
            color = pal_get_rgb (atoi(data->value));
	    fill_colour.red = color.r / 255.0;
	    fill_colour.green = color.g / 255.0;
	    fill_colour.blue = color.b / 255.0;
            break;
        }	
    } while(codedxf != 0);

   pcd = g_new( MultipointCreateData, 1);
   
   if( p[2].x != p[3].x && p[2].y != p[3].y )
     pcd->num_points = 4;
   else
     pcd->num_points = 3;
   
   pcd->points = g_new( Point, pcd->num_points );
   
   memcpy( pcd->points, p, sizeof( Point ) * pcd->num_points );

   polygon_obj = otype->ops->create( NULL, pcd, &h1, &h2 );

   layer_add_object( layer, polygon_obj );

   props = prop_list_from_descs( dxf_solid_prop_descs, pdtpp_true );
   g_assert(props->len == 5);

   cprop = g_ptr_array_index( props,0 );
   cprop->color_data = fill_colour;

   rprop = g_ptr_array_index( props,1 );
   rprop->real_data = line_width;

   lsprop = g_ptr_array_index( props,2 );
   lsprop->style = style;
   lsprop->dash = 1.0;

   fprop = g_ptr_array_index( props, 3 );
   fprop->color_data = fill_colour;

   bprop = g_ptr_array_index( props, 4 );
   bprop->bool_data = TRUE;

   polygon_obj->ops->set_props( polygon_obj, props );

   prop_list_free(props);
   
   return( polygon_obj );
}

static PropDescription dxf_polyline_prop_descs[] = {
     { "line_colour", PROP_TYPE_COLOUR },
     { PROP_STDNAME_LINE_WIDTH, PROP_STDTYPE_LINE_WIDTH },
     { "line_style", PROP_TYPE_LINESTYLE },
   PROP_DESC_END};

static int is_equal( double a, double b )
{
   double epsilon = 0.001;
   
   if( a == b )
     return( 1 );
   
   if(( a + epsilon ) > b && ( a - epsilon ) < b )
     return( 1 );
   
   return( 0 );
}

/* reads a polyline entity from the dxf file and creates a polyline object in dia*/
DiaObject *
read_entity_polyline_dxf(FILE *filedxf, DxfData *data, DiagramData *dia)
{
    int codedxf, i;
   
        /* polygon data */
    Point *p = NULL, start, end, center;
    
    DiaObjectType *otype = object_get_type("Standard - PolyLine");
    Handle *h1, *h2;
    
    DiaObject *polyline_obj;
    MultipointCreateData *pcd;

    Color line_colour = { 0.0, 0.0, 0.0 };

    GPtrArray *props;
    LinestyleProperty *lsprop;
    ColorProperty *cprop;
    RealProperty *rprop;
   
    real line_width = DEFAULT_LINE_WIDTH;
    real radius, start_angle = 0;
    LineStyle style = LINESTYLE_SOLID;
    Layer *layer = dia->active_layer;
    RGB_t color;
    unsigned char closed = 0;
    int points = 0;
   
    do {
        if(read_dxf_codes(filedxf, data) == FALSE){
            return( NULL );
        }
        codedxf = atoi(data->code);
        switch(codedxf){
            case 0:
                if( !strcmp( data->value, "VERTEX" ))
                {
                    points++;
		
                    p = g_realloc( p, sizeof( Point ) * points );
		
                        /*printf( "Vertex %d\n", points );*/
		  
                }
		break;	   
            case 6:	 
                style = get_dia_linestyle_dxf(data->value);
                break;		
            case  8: 
                layer = layer_find_by_name(data->value, dia);
                    /*printf( "layer: %s ", data->value );*/
                break;
            case 10:
                if( points != 0 )
                {
                    p[points-1].x = g_ascii_strtod(data->value, NULL) * coord_scale * measure_scale;
                        /*printf( "P[%d].x: %f ", points-1, p[points-1].x );*/
                }
                break;
            case 20: 
                if( points != 0 )
                {
                    p[points-1].y = (-1)*g_ascii_strtod(data->value, NULL) * coord_scale * measure_scale;
                        /*printf( "P[%d].y: %f\n", points-1, p[points-1].y );*/
                }
                break;
            case 39: 
                line_width = g_ascii_strtod(data->value, NULL) * WIDTH_SCALE;
                    /*printf( "width %f\n", line_width );*/
                break;
	    case 40: /* default starting width */
	    case 41: /* default ending width */
	        line_width = g_ascii_strtod(data->value, NULL) * WIDTH_SCALE;
		break;
            case 42:
                    /* FIXME - the bulge code doesn't work */
                p = g_realloc( p, sizeof( Point ) * ( points + 10 ));

                start = p[points-2];
                end = p[points-1];
	   
                radius = sqrt( pow( end.x - start.x, 2 ) + pow( end.y - start.y, 2 ))/2;

                center.x = start.x + ( end.x - start.x )/2;
                center.y = start.y + ( end.y - start.y )/2;
	   
                if( is_equal( start.x, end.x ))
                {
                    if( is_equal( start.y, end.y ))
                    {
                        g_warning(_("Bad vertex bulge\n") );
                    }
                    else if( start.y > center.y )
                    {
                            /*start_angle = 90.0;*/
                        start_angle = M_PI/2;
                    }
                    else
                    {
                            /*start_angle = 270.0;*/
                        start_angle = M_PI * 1.5;
                    }
                }
                else if( is_equal( start.y, end.y ))
                {
                    if( is_equal( start.x, end.x ))
                    {
                        g_warning( _("Bad vertex bulge\n") );
                    }
                    else if( start.x > center.x )
                    {
                        start_angle = 0.0;
                    }
                    else
                    {
                        start_angle = M_PI;
                    }
                }
                else
                {
                    start_angle = atan( center.y - start.y /center.x - start.x );
                }
	   
                    /*printf( "start x %f end x %f center x %f\n", start.x, end.x, center.x );
                      printf( "start y %f end y %f center y %f\n", start.y, end.y, center.y );
                      printf( "bulge %s %f startx_angle %f\n", data->value, radius, start_angle );*/
	   
                for( i=(points-1); i<(points+9); i++ );
                {
                    p[i].x = center.x + cos( start_angle ) * radius;
                    p[i].y = center.y + sin( start_angle ) * radius;
                    start_angle += M_PI/10.0;
                        /*printf( "i %d x %f y %f\n", i, p[i].x, p[i].y );*/
                }
                points += 10;
	   
                p[points-1] = end;
                break;
            case 62: 
                color = pal_get_rgb (atoi(data->value));
                line_colour.red = color.r / 255.0;
                line_colour.green = color.g / 255.0;
                line_colour.blue = color.b / 255.0;
                break;
            case 70:
                closed = 1 & atoi( data->value );
                    /*printf( "closed %d %s", closed, data->value );*/
                break;
        }	
    } while( strcmp( data->value, "SEQEND" ));
   
    if( points == 0 )
    {
        printf( "No vertexes defined\n" );
        return( NULL );
    }
   
    pcd = g_new( MultipointCreateData, 1);
   
    if( closed )
    {
	otype = object_get_type("Standard - Polygon");
    }
   
    pcd->num_points = points;
    pcd->points = g_new( Point, pcd->num_points );
   
    memcpy( pcd->points, p, sizeof( Point ) * pcd->num_points );
   
    g_free( p );

    polyline_obj = otype->ops->create( NULL, pcd, &h1, &h2 );

    layer_add_object( layer, polyline_obj );

    props = prop_list_from_descs( dxf_polyline_prop_descs, pdtpp_true );
    g_assert( props->len == 3 );

    cprop = g_ptr_array_index( props,0 );
    cprop->color_data = line_colour;

    rprop = g_ptr_array_index( props,1 );
    rprop->real_data = line_width;

    lsprop = g_ptr_array_index( props,2 );
    lsprop->style = style;
    lsprop->dash = 1.0;

    polyline_obj->ops->set_props( polyline_obj, props );

    prop_list_free(props);
   
    return( polyline_obj );
}

static PropDescription dxf_ellipse_prop_descs[] = {
    { "elem_corner", PROP_TYPE_POINT },
    { "elem_width", PROP_TYPE_REAL },
    { "elem_height", PROP_TYPE_REAL },
    { "line_colour", PROP_TYPE_COLOUR },
    { PROP_STDNAME_LINE_WIDTH, PROP_STDTYPE_LINE_WIDTH },
    { "show_background", PROP_TYPE_BOOL},
    PROP_DESC_END};

/* reads a circle entity from the dxf file and creates a circle object in dia*/
DiaObject *read_entity_circle_dxf(FILE *filedxf, DxfData *data, DiagramData *dia)
{
    int codedxf;
    
    /* circle data */
    Point center;
    real radius = 1.0;
    
    DiaObjectType *otype = object_get_type("Standard - Ellipse");  
    Handle *h1, *h2;
    
    DiaObject *ellipse_obj;
    Color line_colour = { 0.0, 0.0, 0.0 };

    PointProperty *ptprop;
    RealProperty *rprop;
    BoolProperty *bprop;
    ColorProperty *cprop;
    GPtrArray *props;

    real line_width = DEFAULT_LINE_WIDTH;
    Layer *layer = dia->active_layer;
    
    do {
        if(read_dxf_codes(filedxf, data) == FALSE){
            return( NULL );
        }
        codedxf = atoi(data->code);
        switch(codedxf){
        case  8: 
            layer = layer_find_by_name(data->value, dia);
            break;
        case 10: 
            center.x = g_ascii_strtod(data->value, NULL) * coord_scale * measure_scale;
            break;
        case 20: 
            center.y = (-1)*g_ascii_strtod(data->value, NULL) * coord_scale * measure_scale;
            break;
        case 39: 
            line_width = g_ascii_strtod(data->value, NULL) * WIDTH_SCALE;
            break;
        case 40: 
            radius = g_ascii_strtod(data->value, NULL) * coord_scale * measure_scale;
            break;
        }
        
    } while(codedxf != 0);
 
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
   
   return( ellipse_obj );
}

static PropDescription dxf_arc_prop_descs[] = {
    { "start_point", PROP_TYPE_POINT },
    { "end_point", PROP_TYPE_POINT },
    { "curve_distance", PROP_TYPE_REAL },
    { "line_colour", PROP_TYPE_COLOUR },
    { PROP_STDNAME_LINE_WIDTH, PROP_STDTYPE_LINE_WIDTH },
    PROP_DESC_END};

/* reads a circle entity from the dxf file and creates a circle object in dia*/
DiaObject *read_entity_arc_dxf(FILE *filedxf, DxfData *data, DiagramData *dia)
{
    int codedxf;
    
    /* arc data */
    Point start,center,end;
    real radius = 1.0, start_angle = 0.0, end_angle=360.0;
    real curve_distance;

    DiaObjectType *otype = object_get_type("Standard - Arc");  
    Handle *h1, *h2;
  
    DiaObject *arc_obj;
    Color line_colour = { 0.0, 0.0, 0.0 };

    ColorProperty *cprop;
    PointProperty *ptprop;
    RealProperty *rprop;
    GPtrArray *props;

    real line_width = DEFAULT_LINE_WIDTH;
    Layer *layer = dia->active_layer;
		
    do {
        if(read_dxf_codes(filedxf, data) == FALSE){
            return( NULL );
        }
        codedxf = atoi(data->code);
        switch(codedxf){
        case  8: 
            layer = layer_find_by_name(data->value, dia);
            break;
        case 10: 
            center.x = g_ascii_strtod(data->value, NULL) * coord_scale * measure_scale;
            break;
        case 20: 
            center.y = (-1)*g_ascii_strtod(data->value, NULL) * coord_scale * measure_scale;
            break;
        case 39: 
            line_width = g_ascii_strtod(data->value, NULL) * WIDTH_SCALE;
            break;
        case 40: 
            radius = g_ascii_strtod(data->value, NULL) * coord_scale * measure_scale;
            break;
        case 50:
            start_angle = g_ascii_strtod(data->value, NULL)*M_PI/180.0;
            break;
        case 51:
            end_angle = g_ascii_strtod(data->value, NULL)*M_PI/180.0;
            break;
        }
    } while(codedxf != 0);

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
   
   return( arc_obj );
}

/* reads an ellipse entity from the dxf file and creates an ellipse object in dia*/
DiaObject *
read_entity_ellipse_dxf(FILE *filedxf, DxfData *data, DiagramData *dia)
{
    int codedxf;
    
    /* ellipse data */
    Point center;
    real width = 1.0;
    real ratio_width_height = 1.0;
    
    DiaObjectType *otype = object_get_type("Standard - Ellipse");
    Handle *h1, *h2;
    
    DiaObject *ellipse_obj; 
    Color line_colour = { 0.0, 0.0, 0.0 };
    PointProperty *ptprop;
    RealProperty *rprop;
    BoolProperty *bprop;
    ColorProperty *cprop;
    GPtrArray *props;

    real line_width = DEFAULT_LINE_WIDTH;
    Layer *layer = dia->active_layer;
    
    do {
        if(read_dxf_codes(filedxf, data) == FALSE){
            return( NULL );
        }
        codedxf = atoi(data->code);
        switch(codedxf){
        case  8: 
            layer = layer_find_by_name(data->value, dia);
            break;
        case 10: 
            center.x = g_ascii_strtod(data->value, NULL) * coord_scale * measure_scale;
            break;
        case 11: 
            ratio_width_height = g_ascii_strtod(data->value, NULL) * coord_scale * measure_scale;
            break;
        case 20: 
            center.y = (-1)*g_ascii_strtod(data->value, NULL) * coord_scale * measure_scale;
            break;
        case 39: 
            line_width = g_ascii_strtod(data->value, NULL) * WIDTH_SCALE;
            break;
        case 40: 
            width = g_ascii_strtod(data->value, NULL) * WIDTH_SCALE; /* XXX what scale */
            break;
        }
    } while(codedxf != 0);
 
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
   
   return( ellipse_obj );
}

static PropDescription dxf_text_prop_descs[] = {
    { "text", PROP_TYPE_TEXT },
    PROP_DESC_END};

DiaObject *
read_entity_text_dxf(FILE *filedxf, DxfData *data, DiagramData *dia)
{
    int codedxf;
    RGB_t color;

    /* text data */
    Point location;
    real height = text_scale * coord_scale * measure_scale;
    real y_offset = 0;
    Alignment textalignment = ALIGN_LEFT;
    char *textvalue = NULL, *textp;
    
    DiaObjectType *otype = object_get_type("Standard - Text");
    Handle *h1, *h2;
    
    DiaObject *text_obj;
    Color text_colour = { 0.0, 0.0, 0.0 };

    TextProperty *tprop;
    GPtrArray *props;

    Layer *layer = dia->active_layer;
    
    do {
        if (read_dxf_codes(filedxf, data) == FALSE) {
            return( NULL );
        }
        codedxf = atoi(data->code);
        switch (codedxf) {
        case  1: 
	   textvalue = g_strdup(data->value);
	   textp = textvalue;
	   /* FIXME - poor tab to space converter */
	   do 
	     {
		if( textp[0] == '^' && textp[1] == 'I' )
		  {
		     textp[0] = ' ';
		     textp[1] = ' ';
		     textp++;
		  }
		
	     }
	   while( *(++textp) != '\0' );
		
	   /*printf( "Found text: %s\n", textvalue );*/
            break;
        case  8: layer = layer_find_by_name(data->value, dia);
            break;
        case 10: 
            location.x = g_ascii_strtod(data->value, NULL) * coord_scale * measure_scale;
	   /*printf( "Found text location x: %f\n", location.x );*/
            break;
        case 11:
            location.x = g_ascii_strtod(data->value, NULL) * coord_scale * measure_scale;
	   /*printf( "Found text location x: %f\n", location.x );*/
            break;
        case 20:
            location.y = (-1)*g_ascii_strtod(data->value, NULL) * coord_scale * measure_scale;
	   /*printf( "Found text location y: %f\n", location.y );*/
            break;
        case 21:
            location.y = (-1)*g_ascii_strtod(data->value, NULL) * coord_scale * measure_scale;
	   /*location.y = (-1)*g_ascii_strtod(data->value, NULL) / text_scale;*/
	   printf( "Found text location y: %f\n", location.y );
            break;
        case 40: 
            height = g_ascii_strtod(data->value, NULL) * text_scale * coord_scale * measure_scale;
	   /*printf( "text height %f\n", height );*/
            break;
	 case 62: 
	   color = pal_get_rgb (atoi(data->value));
	   text_colour.red = color.r / 255.0;
	   text_colour.green = color.g / 255.0;
	   text_colour.blue = color.b / 255.0;
            break;
        case 72: 
	   switch(atoi(data->value))
	     {
	      case 0:
		textalignment = ALIGN_LEFT;
                break;
	      case 1: 
		textalignment = ALIGN_CENTER;
                break;
	      case 2: 
		textalignment = ALIGN_RIGHT;
                break;	
	      case 3:
		/* FIXME - it's not clear what these are */
                break;
	      case 4: 
		/* FIXME - it's not clear what these are */
                break;	
	      case 5: 
		/* FIXME - it's not clear what these are */
                break;	
            }
	   break;
        case 73: 
	   switch(atoi(data->value))
	     {
	      case 0:
	      case 1:
		/* FIXME - not really the same vertical alignment */
		/* 0 = baseline */
		/* 1 = bottom */
		y_offset = 0;
                break;
	      case 2: 
		/* 2 = middle */
		y_offset = 0.5;
                break;	
	      case 3:
		/* 3 = top */
		y_offset = 1;
                break;	
            }
	   break;
        }
    } while(codedxf != 0);
  
    location.y += y_offset * height;
   
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

    attributes_get_default_font(&tprop->attr.font, &tprop->attr.height);
    tprop->attr.color = text_colour;
    tprop->attr.height = height;
        
    text_obj->ops->set_props(text_obj, props);
    prop_list_free(props);
   
   return( text_obj );
}

/* reads the layer table from the dxf file and creates the layers */
void 
read_table_layer_dxf(FILE *filedxf, DxfData *data, DiagramData *dia)
{
    int codedxf;
	
    do {
        if(read_dxf_codes(filedxf, data) == FALSE){
            return;
        }
        else {
            codedxf = atoi(data->code);
            if(codedxf == 2){
	       layer_find_by_name( data->value, dia );
            }
        }
    } while ((codedxf != 0) || (strcmp(data->value, "ENDTAB") != 0));
}

/* reads a scale entity from the dxf file */
void read_entity_scale_dxf(FILE *filedxf, DxfData *data, DiagramData *dia)
{
   int codedxf;

   if(read_dxf_codes(filedxf, data) == FALSE)
      return;

   codedxf = atoi(data->code);
       
   switch(codedxf)
     {
      case 40: 
	coord_scale = g_ascii_strtod(data->value, NULL);
	g_message(_("Scale: %f\n"), coord_scale );
	break;
      
      default:
	break;
     }	
   
}

/* reads a scale entity from the dxf file */
void read_entity_measurement_dxf(FILE *filedxf, DxfData *data,
                                 DiagramData *dia)
{
    int codedxf;

   if(read_dxf_codes(filedxf, data) == FALSE)
      return;

   codedxf = atoi(data->code);
       
   switch(codedxf)
     {
      case 70:
	/* value 0 = English, 1 = Metric */
	if( atoi( data->value ) == 0 )
	  measure_scale = 2.54;
	else
	  measure_scale = 1.0;
	/*printf( "Measure Scale: %f\n", measure_scale );*/
	break;
      
      default:
	break;
     }	
   
}

/* reads a textsize entity from the dxf file */
void 
read_entity_textsize_dxf(FILE *filedxf, DxfData *data, DiagramData *dia)
{
   int codedxf;

   if(read_dxf_codes(filedxf, data) == FALSE)
     return;
     
   codedxf = atoi(data->code);

   switch(codedxf)
     {
      case 40:
	text_scale = g_ascii_strtod(data->value, NULL);
	/*printf( "Text Size: %f\n", text_scale );*/
	break;		
      default:
	break;
     }	

}

/* reads the headers section of the dxf file */
void 
read_section_header_dxf(FILE *filedxf, DxfData *data, DiagramData *dia) 
{
    int codedxf;
	
    if(read_dxf_codes(filedxf, data) == FALSE){
        return;
    }
    do {
       codedxf = atoi(data->code);
       if((codedxf == 9) && (strcmp(data->value, "$DIMSCALE") == 0)) {
	  read_entity_scale_dxf(filedxf, data, dia);
        } else if((codedxf == 9) && (strcmp(data->value, "$TEXTSIZE") == 0)) {
	  read_entity_textsize_dxf(filedxf, data, dia);
        } else if((codedxf == 9) && (strcmp(data->value, "$MEASUREMENT") == 0)) {
	  read_entity_measurement_dxf(filedxf, data, dia);
        } else {
	   if(read_dxf_codes(filedxf, data) == FALSE){
	      return;
	   }
	   
	}
    } while ((codedxf != 0) || (strcmp(data->value, "ENDSEC") != 0));
}

/* reads the classes section of the dxf file */
void read_section_classes_dxf(FILE *filedxf, DxfData *data, DiagramData *dia) {
    int codedxf;
	
    if(read_dxf_codes(filedxf, data) == FALSE){
        return;
    }
    do {
       codedxf = atoi(data->code);
       if((codedxf == 9) && (strcmp(data->value, "$LTSCALE") == 0)) {
	  read_entity_scale_dxf(filedxf, data, dia);
        } else if((codedxf == 9) && (strcmp(data->value, "$TEXTSIZE") == 0)) {
	  read_entity_textsize_dxf(filedxf, data, dia);
        } else {
	   if(read_dxf_codes(filedxf, data) == FALSE){
	      return;
	   }
	   
	}
    } while ((codedxf != 0) || (strcmp(data->value, "ENDSEC") != 0));
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
        } else if((codedxf == 0) && (strcmp(data->value, "VERTEX") == 0)) {
            read_entity_line_dxf(filedxf, data, dia);
        } else if((codedxf == 0) && (strcmp(data->value, "SOLID") == 0)) {
            read_entity_solid_dxf(filedxf, data, dia);
        } else if((codedxf == 0) && (strcmp(data->value, "POLYLINE") == 0)) {
            read_entity_polyline_dxf(filedxf, data, dia);
        } else if((codedxf == 0) && (strcmp(data->value, "CIRCLE") == 0)) {
            read_entity_circle_dxf(filedxf, data, dia);
        } else if((codedxf == 0) && (strcmp(data->value, "ELLIPSE") == 0)) {
            read_entity_ellipse_dxf(filedxf, data, dia);
        } else if((codedxf == 0) && (strcmp(data->value, "TEXT") == 0)) {
            read_entity_text_dxf(filedxf, data, dia);
        } else if((codedxf == 0) && (strcmp(data->value, "ARC") == 0)) {
               read_entity_arc_dxf(filedxf,data,dia);
        } else {
            if(read_dxf_codes(filedxf, data) == FALSE) {
                return;
            }
        }
        codedxf = atoi(data->code);		
    } while((codedxf != 0) || (strcmp(data->value, "ENDSEC") != 0));
}

/* reads the blocks section of the dxf file */
void 
read_section_blocks_dxf(FILE *filedxf, DxfData *data, DiagramData *dia) 
{
    int codedxf, group_items = 0, group = 0;
    GList *group_list = NULL;
    DiaObject *obj = NULL;
    Layer *group_layer = NULL;
   
    if (read_dxf_codes(filedxf, data) == FALSE){
        return;		
    }
    codedxf = atoi(data->code);
    do {  
        if((codedxf == 0) && (strcmp(data->value, "LINE") == 0)) {
            read_entity_line_dxf(filedxf, data, dia);
        } else if((codedxf == 0) && (strcmp(data->value, "SOLID") == 0)) {
            obj = read_entity_solid_dxf(filedxf, data, dia);
        } else if((codedxf == 0) && (strcmp(data->value, "VERTEX") == 0)) {
            read_entity_line_dxf(filedxf, data, dia);
        } else if((codedxf == 0) && (strcmp(data->value, "POLYLINE") == 0)) {
            obj = read_entity_polyline_dxf(filedxf, data, dia);
        } else if((codedxf == 0) && (strcmp(data->value, "CIRCLE") == 0)) {
            read_entity_circle_dxf(filedxf, data, dia);
        } else if((codedxf == 0) && (strcmp(data->value, "ELLIPSE") == 0)) {
            read_entity_ellipse_dxf(filedxf, data, dia);
        } else if((codedxf == 0) && (strcmp(data->value, "TEXT") == 0)) {
            obj = read_entity_text_dxf(filedxf, data, dia);
        } else if((codedxf == 0) && (strcmp(data->value, "ARC") == 0)) {
            read_entity_arc_dxf(filedxf,data,dia);
        } else if((codedxf == 0) && (strcmp(data->value, "BLOCK") == 0)) {
                /* printf("Begin group\n" ); */
	 
            group = TRUE;
            group_items = 0;
            group_list = NULL;
            group_layer = NULL;
	 
            do {
                if(read_dxf_codes(filedxf, data) == FALSE)
                    return;

                codedxf = atoi(data->code);

                if( codedxf == 8 )
                    group_layer = layer_find_by_name( data->value, dia ); 

            } while( codedxf != 0 );
	
        } else if((codedxf == 0) && (strcmp(data->value, "ENDBLK") == 0)) {
                /* printf( "End group %d\n", group_items ); */

            if( group && group_items > 0 && group_list != NULL )
            {
                obj = group_create( group_list );
                if( NULL == group_layer )
                    layer_add_object( dia->active_layer, obj );
                else
                    layer_add_object( group_layer, obj );
            }
	 
            group = FALSE;
            group_items = 0;
            group_list = NULL;
            obj = NULL;
				  
            if(read_dxf_codes(filedxf, data) == FALSE)
                return;
	 
        } else {
            if(read_dxf_codes(filedxf, data) == FALSE) {
                return;
            }
        }
      
        if( group && obj != NULL )
        {
            group_items++;

            group_list = g_list_prepend( group_list, obj );
	   
            obj = NULL;
        }
      
        codedxf = atoi(data->code);		
    } while((codedxf != 0) || (strcmp(data->value, "ENDSEC") != 0));
}

/* imports the given dxf-file, returns TRUE if successful */
gboolean
import_dxf(const gchar *filename, DiagramData *dia, void* user_data)
{
    FILE *filedxf;
    DxfData *data;
    int codedxf;
    
    filedxf = g_fopen(filename,"r");
    if(filedxf == NULL){
        message_error(_("Couldn't open: '%s' for reading.\n"), 
		      dia_message_filename(filename));
        return FALSE;
    }
    
    data = g_new(DxfData, 1);
    
    do {
        if(read_dxf_codes(filedxf, data) == FALSE) {
            g_free(data);
	    message_error(_("read_dxf_codes failed on '%s'\n"),
			  dia_message_filename(filename) );
            return FALSE;
        }
        else {
            codedxf = atoi(data->code);
            if (0 == codedxf && strstr(data->code, "AutoCAD Binary DXF")) {
                g_free(data);
	        message_error(_("Binary DXF from '%s' not supported\n"),
			      dia_message_filename(filename) );
                return FALSE;
            }
	    if (0 == codedxf) {
                if(strcmp(data->value, "SECTION") == 0) {
		  /* don't think we need to do anything */
                } else if(strcmp(data->value, "ENDSEC") == 0) {
		  /* don't think we need to do anything */
                } else if(strcmp(data->value, "EOF") == 0) {
		  /* handled below */
		} else {
		  g_print ("DXF 0:%s not handled\n", data->value);
		}
            } else if(codedxf == 2) {
                if(strcmp(data->value, "ENTITIES") == 0) {
		   /*printf( "reading section entities\n" );*/
                    read_section_entities_dxf(filedxf, data, dia);
                }
                else if(strcmp(data->value, "BLOCKS") == 0) {
		   /*printf( "reading section BLOCKS\n" );*/
                    read_section_blocks_dxf(filedxf, data, dia);
                }
                else if(strcmp(data->value, "CLASSES") == 0) {
		   /*printf( "reading section CLASSES\n" );*/
                    read_section_classes_dxf(filedxf, data, dia);
                }
                else if(strcmp(data->value, "HEADER") == 0) {
		   /*printf( "reading section HEADER\n" );*/
                    read_section_header_dxf(filedxf, data, dia);
                }
                else if(strcmp(data->value, "TABLES") == 0) {
		  /*printf( "reading section tables\n" );*/
                    read_section_tables_dxf(filedxf, data, dia);
                }
	        else if(strcmp(data->value, "OBJECTS") == 0) {
		  /*printf( "reading section objects\n" );*/
                    read_section_entities_dxf(filedxf, data, dia);
		}
	    }
	   else
	     g_warning(_("Unknown dxf code %d\n"), codedxf );
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




