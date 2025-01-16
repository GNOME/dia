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

#include "config.h"

#include <glib/gi18n-lib.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <glib.h>
#include <glib/gstdio.h>

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
#include "dia-layer.h"

static double coord_scale = 1.0, measure_scale = 1.0;
static double text_scale = 1.0;

#define WIDTH_SCALE (coord_scale * measure_scale)
#define DEFAULT_LINE_WIDTH 0.001

Point extent_min, extent_max;
Point limit_min, limit_max;

/* maximum line length */
#define DXF_LINE_LENGTH 257

typedef struct _DxfLayerData {
  char layerName[256];
  int acad_colour;
} DxfLayerData;

typedef struct _DxfData {
  int  code;
  char codeline[DXF_LINE_LENGTH];
  char value[DXF_LINE_LENGTH];
} DxfData;

static gboolean import_dxf(const gchar *filename, DiagramData *dia, DiaContext *ctx, void* user_data);
static gboolean read_dxf_codes(FILE *filedxf, DxfData *data);
static DiaObject *read_entity_line_dxf(FILE *filedxf, DxfData *data, DiagramData *dia);
static DiaObject *read_entity_circle_dxf(FILE *filedxf, DxfData *data, DiagramData *dia);
static DiaObject *read_entity_ellipse_dxf(FILE *filedxf, DxfData *data, DiagramData *dia);
static DiaObject *read_entity_arc_dxf(FILE *filedxf, DxfData *data, DiagramData *dia);
static DiaObject *read_entity_solid_dxf(FILE *filedxf, DxfData *data, DiagramData *dia);
static DiaObject *read_entity_polyline_dxf(FILE *filedxf, DxfData *data, DiagramData *dia);
static DiaObject *read_entity_text_dxf(FILE *filedxf, DxfData *data, DiagramData *dia);
static void read_entity_measurement_dxf(FILE *filedxf, DxfData *data, DiagramData *dia);
static void read_entity_scale_dxf(FILE *filedxf, DxfData *data, DiagramData *dia);
static void read_entity_textsize_dxf(FILE *filedxf, DxfData *data, DiagramData *dia);
static void read_table_layer_dxf(FILE *filedxf, DxfData *data, DiagramData *dia);
static void read_section_header_dxf(FILE *filedxf, DxfData *data, DiagramData *dia);
static void read_section_classes_dxf(FILE *filedxf, DxfData *data, DiagramData *dia);
static void read_section_tables_dxf(FILE *filedxf, DxfData *data, DiagramData *dia);
static void read_section_entities_dxf(FILE *filedxf, DxfData *data, DiagramData *dia);
static void read_section_blocks_dxf(FILE *filedxf, DxfData *data, DiagramData *dia);
static DiaLayer *layer_find_by_name(char *layername, DiagramData *dia);
static DiaLineStyle get_dia_linestyle_dxf(char *dxflinestyle);

GHashTable *_color_by_layer_ht = NULL;

static void
_dxf_color_set_by_layer (const DiaLayer *layer, int color_index)
{
  if (!_color_by_layer_ht) {
    /* lazy creation */
    _color_by_layer_ht = g_hash_table_new (g_direct_hash, g_direct_equal);
  }

  g_hash_table_insert (_color_by_layer_ht, (void *)layer, GINT_TO_POINTER (color_index));
}


static int
_dxf_color_get_by_layer (const DiaLayer *layer)
{
  int color_index;

  if (!_color_by_layer_ht) {
    return 0;
  }

  color_index = GPOINTER_TO_INT (g_hash_table_lookup (_color_by_layer_ht, layer));
  if (color_index > 0) {
    return color_index;
  }

  return 0;
}


static void
_color_init_from_rgb (Color *color, RGB_t rgb)
{
  color->red   = rgb.r / 255.0;
  color->green = rgb.g / 255.0;
  color->blue  = rgb.b / 255.0;
  color->alpha = 1.0;
}


/* returns the layer with the given name */
/* TODO: merge this with other layer code? */
static DiaLayer *
layer_find_by_name (char *layername, DiagramData *dia)
{
  DiaLayer *matching_layer;

  matching_layer = NULL;

  DIA_FOR_LAYER_IN_DIAGRAM (dia, layer, i, {
    if (strcmp (dia_layer_get_name (layer), layername) == 0) {
      matching_layer = layer;
      break;
    }
  });

  if (matching_layer == NULL) {
    matching_layer = dia_layer_new (layername, dia);
    data_add_layer (dia, matching_layer);
    // dia now owns the layer
    g_object_unref (matching_layer);
  }

  return matching_layer;
}


/* returns the matching dia linestyle for a given dxf linestyle */
/* if no matching style is found, LINESTYLE solid is returned as a default */
static DiaLineStyle
get_dia_linestyle_dxf (char *dxflinestyle)
{
  if (g_strcmp0 (dxflinestyle, "DASHED") == 0) {
    return DIA_LINE_STYLE_DASHED;
  }
  if (g_strcmp0 (dxflinestyle, "DASHDOT") == 0) {
    return DIA_LINE_STYLE_DASH_DOT;
  }
  if (g_strcmp0 (dxflinestyle, "DOT") == 0) {
    return DIA_LINE_STYLE_DOTTED;
  }
  if (g_strcmp0 (dxflinestyle, "DIVIDE") == 0) {
    return DIA_LINE_STYLE_DASH_DOT_DOT;
  }

  return DIA_LINE_STYLE_SOLID;
}


/* reads a line entity from the dxf file and creates a line object in dia*/
static DiaObject *
read_entity_line_dxf (FILE *filedxf, DxfData *data, DiagramData *dia)
{
    /* line data */
    Point start, end;

    DiaObjectType *otype = object_get_type ("Standard - Line");
    Handle *h1, *h2;

    DiaObject *line_obj;
    Color line_colour;
    RGB_t color = { 0, };
    GPtrArray *props;

    double line_width = DEFAULT_LINE_WIDTH;
    DiaLineStyle style = DIA_LINE_STYLE_SOLID;
    DiaLayer *layer = dia_diagram_data_get_active_layer (dia);

    end.x=0;
    end.y=0;

    props = g_ptr_array_new ();

    do {
      if (read_dxf_codes (filedxf, data) == FALSE){
        return NULL;
      }
      switch (data->code){
        case 6:
          style = get_dia_linestyle_dxf (data->value);
          break;
        case 8:
          layer = layer_find_by_name (data->value, dia);
          color = pal_get_rgb (_dxf_color_get_by_layer (layer));
          break;
        case 10:
          start.x = g_ascii_strtod (data->value, NULL) * coord_scale * measure_scale;
          break;
        case 11:
          end.x = g_ascii_strtod (data->value, NULL) * coord_scale * measure_scale;
          break;
        case 20:
          start.y = (-1)*g_ascii_strtod (data->value, NULL) * coord_scale * measure_scale;
          break;
        case 21:
          end.y = (-1)*g_ascii_strtod (data->value, NULL) * coord_scale * measure_scale;
          break;
        case 39:
          line_width = g_ascii_strtod (data->value, NULL) * WIDTH_SCALE;
          /*printf( "line width %f\n", line_width ); */
          break;
        case 62 :
          color = pal_get_rgb (atoi (data->value));
          break;
        default:
          g_warning ("Unhandled %i", data->code);
          break;
      }
    } while (data->code != 0);

    _color_init_from_rgb (&line_colour, color);
    line_obj = otype->ops->create(&start, otype->default_user_data,
                                  &h1, &h2);

    prop_list_add_point (props, "start_point", &start);
    prop_list_add_point (props, "end_point", &end);
    prop_list_add_line_colour (props, &line_colour);
    prop_list_add_line_width (props, line_width);
    prop_list_add_line_style (props, style, 1.0);

    line_obj->ops->set_props(line_obj, props);

    prop_list_free (props);

    if (layer) {
      dia_layer_add_object (layer, line_obj);
    } else {
      return line_obj;
    }

    /* don't add it it twice */
    return NULL;
}


/* reads a solid entity from the dxf file and creates a polygon object in dia*/
static DiaObject *
read_entity_solid_dxf (FILE *filedxf, DxfData *data, DiagramData *dia)
{
  /* polygon data */
  Point p[4];

  DiaObjectType *otype = object_get_type ("Standard - Polygon");
  Handle *h1, *h2;

  DiaObject *polygon_obj;
  MultipointCreateData *pcd;

  Color fill_colour;

  GPtrArray *props;

  double line_width = 0.001;
  DiaLineStyle style = DIA_LINE_STYLE_SOLID;
  DiaLayer *layer = dia_diagram_data_get_active_layer (dia);
  RGB_t color  = { 127, 127, 127 };

  memset (p, 0, sizeof (p));

  do {
    if (read_dxf_codes (filedxf, data) == FALSE) {
      return NULL;
    }
    switch (data->code){
      case 6:
        style = get_dia_linestyle_dxf (data->value);
        break;
      case 8:
        layer = layer_find_by_name (data->value, dia);
        color = pal_get_rgb (_dxf_color_get_by_layer (layer));
        /*printf( "layer: %s ", data->value );*/
        break;
      case 10:
        p[0].x = g_ascii_strtod (data->value, NULL) * coord_scale * measure_scale;
        /*printf( "P0.x: %f ", p[0].x );*/
        break;
      case 11:
        p[1].x = g_ascii_strtod (data->value, NULL) * coord_scale * measure_scale;
        /*printf( "P1.x: %f ", p[1].x );*/
        break;
      case 12: /* bot only swapped, but special for only 3 points given */
        p[2].x = p[3].x = g_ascii_strtod (data->value, NULL) * coord_scale * measure_scale;
        /*printf( "P2.x: %f ", p[2].x );*/
        break;
      case 13: /* SOLID order swapped compared to Dia's */
        p[2].x = g_ascii_strtod (data->value, NULL) * coord_scale * measure_scale;
        /*printf( "P3.x: %f ", p[3].x );*/
        break;
      case 20:
        p[0].y = (-1)*g_ascii_strtod (data->value, NULL) * coord_scale * measure_scale;
        /*printf( "P0.y: %f ", p[0].y );*/
        break;
      case 21:
        p[1].y = (-1)*g_ascii_strtod (data->value, NULL) * coord_scale * measure_scale;
        /*printf( "P1.y: %f ", p[1].y );*/
        break;
      case 22:
        p[2].y = p[3].y = (-1)*g_ascii_strtod (data->value, NULL) * coord_scale * measure_scale;
        /*printf( "P2.y: %f ", p[2].y );*/
        break;
      case 23:
        p[2].y = (-1)*g_ascii_strtod (data->value, NULL) * coord_scale * measure_scale;
        /*printf( "P3.y: %f\n", p[3].y );*/
        break;
      case 39:
        line_width = g_ascii_strtod (data->value, NULL) * WIDTH_SCALE;
        /*printf( "width %f\n", line_width );*/
        break;
      case 62:
        color = pal_get_rgb (atoi (data->value));
        break;
      default:
        g_warning ("Unhandled %i", data->code);
        break;
    }
  } while (data->code != 0);

  pcd = g_new0 (MultipointCreateData, 1);

  if (p[2].x != p[3].x || p[2].y != p[3].y) {
    pcd->num_points = 4;
  } else {
    pcd->num_points = 3;
  }

  pcd->points = g_new0 (Point, pcd->num_points);

  memcpy (pcd->points, p, sizeof (Point) * pcd->num_points);

  polygon_obj = otype->ops->create (NULL, pcd, &h1, &h2);

  _color_init_from_rgb (&fill_colour, color);

  props = g_ptr_array_new ();

  prop_list_add_line_colour (props, &fill_colour);
  prop_list_add_line_width (props, line_width);
  prop_list_add_line_style (props, style, 1.0);
  prop_list_add_fill_colour (props, &fill_colour);
  prop_list_add_show_background (props, TRUE);

  dia_object_set_properties (polygon_obj, props);

  prop_list_free (props);

  if (layer) {
    dia_layer_add_object (layer, polygon_obj);
  } else {
    return polygon_obj;
  }

  return NULL;
}


static int
is_equal (double a, double b)
{
  double epsilon = 0.00001;

  if (a == b) {
    return 1 ;
  }

  if (( a + epsilon ) > b && ( a - epsilon ) < b) {
    return 1;
  }

  return 0;
}


/* reads a polyline entity from the dxf file and creates a polyline object in dia*/
static DiaObject *
read_entity_polyline_dxf (FILE *filedxf, DxfData *data, DiagramData *dia)
{
  int i;

  /* polygon data */
  Point *p = NULL, start, end, center;

  DiaObjectType *otype = object_get_type ("Standard - PolyLine");
  Handle *h1, *h2;

  DiaObject *polyline_obj;
  MultipointCreateData *pcd;

  Color line_colour;

  GPtrArray *props;

  double line_width = DEFAULT_LINE_WIDTH;
  double radius, start_angle = 0;
  DiaLineStyle style = DIA_LINE_STYLE_SOLID;
  DiaLayer *layer = dia_diagram_data_get_active_layer (dia);
  RGB_t color = { 0, };
  unsigned char closed = 0;
  int points = 0;
  double bulge = 0.0;
  int bulge_end = -1;
  gboolean bulge_x_avail = FALSE, bulge_y_avail = FALSE;

  do {
    if (read_dxf_codes (filedxf, data) == FALSE){
      return NULL;
    }

    switch (data->code){
      case 0:
        if (!strcmp (data->value, "VERTEX")) {
          points++;

          p = g_renew (Point, p, points);

          /*printf( "Vertex %d\n", points );*/

        }
        break;
      case 6:
        style = get_dia_linestyle_dxf (data->value);
        break;
      case 8:
        layer = layer_find_by_name (data->value, dia);
        color = pal_get_rgb (_dxf_color_get_by_layer (layer));
        /*printf( "layer: %s ", data->value );*/
        break;
      case 10:
        if (points != 0) {
          p[points-1].x = g_ascii_strtod (data->value, NULL) * coord_scale * measure_scale;
          /*printf( "P[%d].x: %f ", points-1, p[points-1].x );*/
          bulge_x_avail = (bulge_end == points);
        }
        break;
      case 20:
        if (points != 0) {
          p[points-1].y = (-1)*g_ascii_strtod (data->value, NULL) * coord_scale * measure_scale;
          /*printf( "P[%d].y: %f\n", points-1, p[points-1].y );*/
          bulge_y_avail = (bulge_end == points);
        }
        break;
      case 39:
        line_width = g_ascii_strtod (data->value, NULL) * WIDTH_SCALE;
        /*printf( "width %f\n", line_width );*/
        break;
      case 40: /* default starting width */
      case 41: /* default ending width */
        line_width = g_ascii_strtod (data->value, NULL) * WIDTH_SCALE;
        break;
      case 42:
        bulge = g_ascii_strtod (data->value, NULL);
        /* The bulge is meant to be _between_ two VERTEX, here: p[points-1] and p[points,]
          * but we have not yet read the end point; so just remember the point to 'bulge to' */
        bulge_end = points+1;
        bulge_x_avail = bulge_y_avail = FALSE;
        break;
      case 62:
        color = pal_get_rgb (atoi (data->value));
        break;
      case 70:
        closed = 1 & atoi (data->value);
        /*printf( "closed %d %s", closed, data->value );*/
        break;
      default:
        g_warning ("Unhandled %i", data->code);
        break;
    }

    if (points == bulge_end && bulge_x_avail && bulge_y_avail) {
      /* turn the last segment into a bulge */

      p = g_renew (Point, p, points + 10);

      if (points < 2) {
        continue;
      }

      start = p[points-2];
      end = p[points-1];

      radius = sqrt (pow (end.x - start.x, 2 ) + pow (end.y - start.y, 2))/2;

      center.x = start.x + ( end.x - start.x )/2;
      center.y = start.y + ( end.y - start.y )/2;

      if (is_equal (start.x, end.x)) {
        if (is_equal (start.y, end.y)) {
          continue; /* better than complaining? */
          g_warning ("Bad vertex bulge");
        } else if (start.y > center.y) {
          /*start_angle = 90.0;*/
          start_angle = M_PI/2;
        } else {
          /*start_angle = 270.0;*/
          start_angle = M_PI * 1.5;
        }
      } else if (is_equal (start.y, end.y)) {
        if (is_equal (start.x, end.x)) {
          continue;
          g_warning ("Bad vertex bulge");
        } else if (start.x > center.x) {
          start_angle = 0.0;
        } else {
          start_angle = M_PI;
        }
      } else {
        start_angle = atan (center.y - start.y /center.x - start.x);
      }

      /*printf( "start x %f end x %f center x %f\n", start.x, end.x, center.x );
        printf( "start y %f end y %f center y %f\n", start.y, end.y, center.y );
        printf( "bulge %s %f startx_angle %f\n", data->value, radius, start_angle );*/

      for (i = (points - 1); i < (points + 9); i++) {
        p[i].x = center.x + cos (start_angle) * radius;
        p[i].y = center.y + sin (start_angle) * radius;
        start_angle += (-M_PI/10.0 * bulge);
        /*printf( "i %d x %f y %f\n", i, p[i].x, p[i].y );*/
      }
      points += 10;

      p[points-1] = end;
    }
  } while (strcmp (data->value, "SEQEND"));

  if (points == 0) {
    g_printerr ("No vertexes defined\n");
    return NULL;
  }

  pcd = g_new0 (MultipointCreateData, 1);

  if (closed) {
    otype = object_get_type ("Standard - Polygon");
  }

  pcd->num_points = points;
  pcd->points = g_new0 (Point, pcd->num_points);

  memcpy (pcd->points, p, sizeof (Point) * pcd->num_points);

  g_clear_pointer (&p, g_free);

  polyline_obj = otype->ops->create (NULL, pcd, &h1, &h2);

  _color_init_from_rgb (&line_colour, color);

  props = g_ptr_array_new ();

  prop_list_add_line_colour (props, &line_colour);
  prop_list_add_line_width (props, line_width);
  prop_list_add_line_style (props, style, 1.0);

  dia_object_set_properties (polyline_obj, props);

  prop_list_free (props);

  if (layer) {
    dia_layer_add_object (layer, polyline_obj);
  } else {
    return polyline_obj;
  }

  return NULL; /* don't add it twice */
}


/* reads a circle entity from the dxf file and creates a circle object in dia*/
static DiaObject *
read_entity_circle_dxf (FILE *filedxf, DxfData *data, DiagramData *dia)
{
  /* circle data */
  Point center = {0, 0};
  double radius = 1.0;

  DiaObjectType *otype = object_get_type ("Standard - Ellipse");
  Handle *h1, *h2;

  DiaObject *ellipse_obj;
  RGB_t color  = { 0, };
  Color line_colour;

  GPtrArray *props;

  double line_width = DEFAULT_LINE_WIDTH;
  DiaLayer *layer = dia_diagram_data_get_active_layer (dia);

  do {
    if (read_dxf_codes (filedxf, data) == FALSE) {
      return NULL;
    }

    switch (data->code) {
      case 8:
        layer = layer_find_by_name (data->value, dia);
        color = pal_get_rgb (_dxf_color_get_by_layer (layer));
        break;
      case 10:
        center.x = g_ascii_strtod (data->value, NULL) * coord_scale * measure_scale;
        break;
      case 20:
        center.y = (-1) * g_ascii_strtod (data->value, NULL) * coord_scale * measure_scale;
        break;
      case 39:
        line_width = g_ascii_strtod (data->value, NULL) * WIDTH_SCALE;
        break;
      case 40:
        radius = g_ascii_strtod (data->value, NULL) * coord_scale * measure_scale;
        break;
      case 62 :
        color = pal_get_rgb (atoi (data->value));
        break;
      default:
        g_warning ("Unhandled %i", data->code);
    }

  } while (data->code != 0);

  center.x -= radius;
  center.y -= radius;
  ellipse_obj = otype->ops->create (&center, otype->default_user_data,
                                    &h1, &h2);

  _color_init_from_rgb (&line_colour, color);

  props = g_ptr_array_new ();

  prop_list_add_point (props, "elem_corner", &center);
  prop_list_add_real (props, "elem_width", radius * 2.0);
  prop_list_add_real (props, "elem_height", radius * 2.0);
  prop_list_add_line_colour (props, &line_colour);
  prop_list_add_line_width (props, line_width);
  prop_list_add_show_background (props, FALSE);

  dia_object_set_properties (ellipse_obj, props);
  prop_list_free (props);

  if (layer) {
    dia_layer_add_object (layer, ellipse_obj);
  } else {
    return ellipse_obj;
  }

  return NULL; /* don't add it twice */
}


/* reads a circle entity from the dxf file and creates a circle object in dia*/
static DiaObject *
read_entity_arc_dxf (FILE *filedxf, DxfData *data, DiagramData *dia)
{
  /* arc data */
  Point start, end;
  Point center = {0, 0};
  double radius = 1.0, start_angle = 0.0, end_angle=360.0;
  double curve_distance;

  DiaObjectType *otype = object_get_type ("Standard - Arc");
  Handle *h1, *h2;

  DiaObject *arc_obj;
  RGB_t color  = { 0, };
  Color line_colour;
  GPtrArray *props;

  double line_width = DEFAULT_LINE_WIDTH;
  DiaLayer *layer = dia_diagram_data_get_active_layer (dia);

  do {
    if (read_dxf_codes (filedxf, data) == FALSE){
      return NULL;
    }

    switch (data->code){
      case 8:
        layer = layer_find_by_name (data->value, dia);
        color = pal_get_rgb (_dxf_color_get_by_layer (layer));
        break;
      case 10:
        center.x = g_ascii_strtod (data->value, NULL) * coord_scale * measure_scale;
        break;
      case 20:
        center.y = (-1) * g_ascii_strtod (data->value, NULL) * coord_scale * measure_scale;
        break;
      case 39:
        line_width = g_ascii_strtod (data->value, NULL) * WIDTH_SCALE;
        break;
      case 40:
        radius = g_ascii_strtod (data->value, NULL) * coord_scale * measure_scale;
        break;
      case 50:
        start_angle = g_ascii_strtod (data->value, NULL) * M_PI / 180.0;
        break;
      case 51:
        end_angle = g_ascii_strtod (data->value, NULL) * M_PI / 180.0;
        break;
      case 62 :
        color = pal_get_rgb (atoi (data->value));
        break;
      default:
        g_warning ("Unhandled %i", data->code);
    }
  } while (data->code != 0);

  /* printf("c.x=%f c.y=%f s",center.x,center.y); */
  start.x = center.x + cos(start_angle) * radius;
  start.y = center.y - sin(start_angle) * radius;
  end.x = center.x + cos(end_angle) * radius;
  end.y = center.y - sin(end_angle) * radius;
  /*printf("s.x=%f s.y=%f e.x=%f e.y=%f\n",start.x,start.y,end.x,end.y);*/

  if (end_angle < start_angle) {
    end_angle += 2.0 * M_PI;
  }
  curve_distance = radius * (1 - cos ((end_angle - start_angle)/2));

  /*printf("start_angle: %f end_angle: %f radius:%f  curve_distance:%f\n",
    start_angle,end_angle,radius,curve_distance);*/

  arc_obj = otype->ops->create (&center, otype->default_user_data, &h1, &h2);

  _color_init_from_rgb (&line_colour, color);

  props = g_ptr_array_new ();
  prop_list_add_point (props, "start_point", &start);
  prop_list_add_point (props, "end_point", &end);
  prop_list_add_real (props, "curve_distance", curve_distance);
  prop_list_add_line_colour (props, &line_colour);
  prop_list_add_line_width (props, line_width);

  dia_object_set_properties (arc_obj, props);
  prop_list_free (props);

  if (layer) {
    dia_layer_add_object (layer, arc_obj);
  } else {
    return arc_obj;
  }

  return NULL; /* don't add it twice */
}


/* reads an ellipse entity from the dxf file and creates an ellipse object in dia*/
static DiaObject *
read_entity_ellipse_dxf (FILE *filedxf, DxfData *data, DiagramData *dia)
{
  /* ellipse data */
  Point center = {0, 0};
  double width = 1.0;
  double ratio_width_height = 1.0;

  DiaObjectType *otype = object_get_type ("Standard - Ellipse");
  Handle *h1, *h2;

  DiaObject *ellipse_obj;
  RGB_t color = { 0, };
  Color line_colour;
  GPtrArray *props;

  double line_width = DEFAULT_LINE_WIDTH;
  DiaLayer *layer = dia_diagram_data_get_active_layer (dia);

  do {
    if (read_dxf_codes (filedxf, data) == FALSE) {
      return NULL;
    }
    switch (data->code) {
      case  8:
        layer = layer_find_by_name(data->value, dia);
        color = pal_get_rgb (_dxf_color_get_by_layer (layer));
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
      case 62:
        color = pal_get_rgb (atoi(data->value));
        break;
      default:
        g_warning ("Unhandled %i", data->code);
        break;
    }
  } while (data->code != 0);

  center.x -= width;
  center.y -= (width*ratio_width_height);
  ellipse_obj = otype->ops->create (&center, otype->default_user_data,
                                    &h1, &h2);

  _color_init_from_rgb (&line_colour, color);

  props = g_ptr_array_new ();

  prop_list_add_point (props, "elem_corner", &center);
  prop_list_add_real (props, "elem_width", width);
  prop_list_add_real (props, "elem_height", width * ratio_width_height);
  prop_list_add_line_colour (props, &line_colour);
  prop_list_add_line_width (props, line_width);
  prop_list_add_show_background (props, FALSE);

  dia_object_set_properties (ellipse_obj, props);
  prop_list_free (props);

  if (layer) {
    dia_layer_add_object (layer, ellipse_obj);
  } else {
    return ellipse_obj;
  }

  return NULL; /* don't add it twice */
}


static PropDescription dxf_text_prop_descs[] = {
  { "text", PROP_TYPE_TEXT },
  PROP_DESC_END
};


static DiaObject *
read_entity_text_dxf (FILE *filedxf, DxfData *data, DiagramData *dia)
{
  RGB_t color = { 0, };

  /* text data */
  Point location = {0, 0};
  double height = text_scale * coord_scale * measure_scale;
  double y_offset = 0;
  DiaAlignment textalignment = DIA_ALIGN_LEFT;
  char *textvalue = NULL, *textp;

  DiaObjectType *otype = object_get_type("Standard - Text");
  Handle *h1, *h2;

  DiaObject *text_obj;
  Color text_colour;

  TextProperty *tprop;
  GPtrArray *props;

  DiaLayer *layer = dia_diagram_data_get_active_layer (dia);

  do {
    if (read_dxf_codes (filedxf, data) == FALSE) {
      return NULL;
    }

    switch (data->code) {
      case  1:
        textvalue = g_strdup(data->value);
        textp = textvalue;
        /* FIXME - poor tab to space converter */
        do {
          if (textp[0] == '^' && textp[1] == 'I') {
            textp[0] = ' ';
            textp[1] = ' ';
            textp++;
          }
        } while( *(++textp) != '\0' );

        /*printf( "Found text: %s\n", textvalue );*/
        break;
      case  8:
        layer = layer_find_by_name (data->value, dia);
        color = pal_get_rgb (_dxf_color_get_by_layer (layer));
        break;
      case 10:
        location.x = g_ascii_strtod (data->value, NULL) * coord_scale * measure_scale;
        /*printf( "Found text location x: %f\n", location.x );*/
        break;
      case 11:
        location.x = g_ascii_strtod (data->value, NULL) * coord_scale * measure_scale;
        /*printf( "Found text location x: %f\n", location.x );*/
        break;
      case 20:
        location.y = (-1) * g_ascii_strtod (data->value, NULL) * coord_scale * measure_scale;
        /*printf( "Found text location y: %f\n", location.y );*/
        break;
      case 21:
        location.y = (-1) * g_ascii_strtod (data->value, NULL) * coord_scale * measure_scale;
        /*location.y = (-1)*g_ascii_strtod(data->value, NULL) / text_scale;*/
        /*printf( "Found text location y: %f\n", location.y );*/
        break;
      case 40:
        height = g_ascii_strtod (data->value, NULL) * text_scale * coord_scale * measure_scale;
        /*printf( "text height %f\n", height );*/
        break;
      case 62:
        color = pal_get_rgb (atoi (data->value));
        break;
      case 72:
        switch (atoi (data->value)) {
          case 0:
            textalignment = DIA_ALIGN_LEFT;
            break;
          case 1:
            textalignment = DIA_ALIGN_CENTRE;
            break;
          case 2:
            textalignment = DIA_ALIGN_RIGHT;
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
          default:
            g_return_val_if_reached (NULL);
            break;
        }
        break;
      case 73:
        switch (atoi (data->value)) {
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
          default:
            g_return_val_if_reached (NULL);
            break;
        }
        break;
      default:
        g_warning ("Unhandled %i", data->code);
        break;
    }
  } while (data->code != 0);

  location.y += y_offset * height;
  _color_init_from_rgb (&text_colour, color);
  text_obj = otype->ops->create (&location, otype->default_user_data,
                                 &h1, &h2);
  props = prop_list_from_descs (dxf_text_prop_descs,pdtpp_true);
  g_return_val_if_fail (props->len == 1, NULL);

  tprop = g_ptr_array_index (props, 0);
  g_clear_pointer (&tprop->text_data, g_free);
  tprop->text_data = textvalue;
  tprop->attr.alignment = textalignment;
  tprop->attr.position.x = location.x;
  tprop->attr.position.y = location.y;

  attributes_get_default_font (&tprop->attr.font, &tprop->attr.height);
  tprop->attr.color = text_colour;
  tprop->attr.height = height;

  dia_object_set_properties (text_obj, props);
  prop_list_free (props);

  if (layer) {
    dia_layer_add_object (layer, text_obj);
  } else {
    return text_obj;
  }

  return NULL; /* don't add it twice */
}


/* reads the layer table from the dxf file and creates the layers */
static void
read_table_layer_dxf (FILE *filedxf, DxfData *data, DiagramData *dia)
{
  DiaLayer *layer = NULL;
  int color_index;

  do {
    if (read_dxf_codes (filedxf, data) == FALSE) {
      return;
    }

    switch (data->code) {
      case 2 : /* layer name */
        layer = layer_find_by_name( data->value, dia );
        break;
      case 62 : /* Color number, if negative layer is off */
        color_index = atoi(data->value);
        if (layer && color_index < 0)
          dia_layer_set_visible (layer, FALSE);
        else
          _dxf_color_set_by_layer (layer, color_index);
        break;
      default:
        g_warning ("Unhandled %i", data->code);
        break;
    }
  } while ((data->code != 0) || (strcmp (data->value, "ENDTAB") != 0));
}


/* reads a scale entity from the dxf file */
static void
read_entity_scale_dxf (FILE *filedxf, DxfData *data, DiagramData *dia)
{
  if (read_dxf_codes (filedxf, data) == FALSE) {
    return;
  }

  switch (data->code) {
    case 40:
      coord_scale = g_ascii_strtod (data->value, NULL);
      if (coord_scale > 0.0) {
        coord_scale = 1.0 / coord_scale;
      }
      g_message ("Scale: %f", coord_scale );
      break;
    default:
      break;
  }
}


static void
read_entitiy_lengthunit_dxf (FILE *filedxf, DxfData *data, DiagramData *dia)
{
  double e; /* undocumented ... */

  if (read_dxf_codes (filedxf, data) == FALSE) {
    return;
  }

  switch (data->code) {
    case 70:
      /* 1 = Scientific; 2 = Decimal; 3 = Engineering;
            * 4 = Architectural; 5 = Fractional; 6 = Windows desktop
      */
      e = atoi (data->value);
      g_message ("LengthUnit: %f:%f", e, coord_scale);
      break;
    default:
      break;
  }
}


/* reads a scale entity from the dxf file */
static void
read_entity_measurement_dxf (FILE *filedxf, DxfData *data, DiagramData *dia)
{
  if (read_dxf_codes (filedxf, data) == FALSE) {
    return;
  }

  switch(data->code) {
    case 70:
      /* value 0 = English, 1 = Metric */
      if (atoi (data->value) == 0 ) {
        measure_scale = 2.54;
      } else {
        measure_scale = 1.0;
      }
      /*printf( "Measure Scale: %f\n", measure_scale );*/
      break;
    default:
      break;
  }
}


/* reads a textsize entity from the dxf file */
static void
read_entity_textsize_dxf (FILE *filedxf, DxfData *data, DiagramData *dia)
{
  if (read_dxf_codes (filedxf, data) == FALSE) {
    return;
  }

  switch(data->code) {
    case 40:
      text_scale = g_ascii_strtod (data->value, NULL);
      if (text_scale > 0.0) {
        text_scale = 1.0 / text_scale;
      }
      /*printf( "Text Size: %f\n", text_scale );*/
      break;
    default:
      break;
  }
}


/* reads the headers section of the dxf file */
static void
read_section_header_dxf (FILE *filedxf, DxfData *data, DiagramData *dia)
{
  if (read_dxf_codes (filedxf, data) == FALSE) {
    return;
  }

  do {
    if ((data->code == 9) && (strcmp (data->value, "$DIMSCALE") == 0)) {
      read_entity_scale_dxf (filedxf, data, dia);
    } else if ((data->code == 9) && (strcmp (data->value, "DIMLUNIT") == 0)) {
      /* nothing documented */
      read_entitiy_lengthunit_dxf (filedxf, data, dia);
    } else if ((data->code == 9) && (strcmp (data->value, "$TEXTSIZE") == 0)) {
      read_entity_textsize_dxf (filedxf, data, dia);
    } else if ((data->code == 9) && (strcmp (data->value, "$MEASUREMENT") == 0)) {
      read_entity_measurement_dxf (filedxf, data, dia);
    } else {
      if (read_dxf_codes (filedxf, data) == FALSE){
        return;
      }
    }
  } while ((data->code != 0) || (strcmp(data->value, "ENDSEC") != 0));
}


/* reads the classes section of the dxf file */
static void
read_section_classes_dxf (FILE *filedxf, DxfData *data, DiagramData *dia)
{
  if (read_dxf_codes (filedxf, data) == FALSE){
    return;
  }

  do {
    if ((data->code == 9) && (strcmp (data->value, "$LTSCALE") == 0)) {
      read_entity_scale_dxf (filedxf, data, dia);
    } else if ((data->code == 9) && (strcmp (data->value, "$TEXTSIZE") == 0)) {
      read_entity_textsize_dxf (filedxf, data, dia);
    } else {
      if (read_dxf_codes (filedxf, data) == FALSE){
        return;
      }
    }
  } while ((data->code != 0) || (strcmp (data->value, "ENDSEC") != 0));
}


/* reads the tables section of the dxf file */
static void
read_section_tables_dxf (FILE *filedxf, DxfData *data, DiagramData *dia)
{
  if (read_dxf_codes (filedxf, data) == FALSE) {
    return;
  }

  do {
    if ((data->code == 0) && (strcmp (data->value, "LAYER") == 0)) {
      read_table_layer_dxf (filedxf, data, dia);
    } else {
      if (read_dxf_codes (filedxf, data) == FALSE) {
          return;
      }
    }
  } while ((data->code != 0) || (strcmp (data->value, "ENDSEC") != 0));
}


/* reads the entities section of the dxf file */
static void
read_section_entities_dxf (FILE *filedxf, DxfData *data, DiagramData *dia)
{
  if (read_dxf_codes (filedxf, data) == FALSE) {
    return;
  }

  do {
    if ((data->code == 0) && (strcmp (data->value, "LINE") == 0)) {
      read_entity_line_dxf (filedxf, data, dia);
    } else if ((data->code == 0) && (strcmp (data->value, "VERTEX") == 0)) {
      read_entity_line_dxf (filedxf, data, dia);
    } else if ((data->code == 0) && (strcmp (data->value, "SOLID") == 0)) {
      read_entity_solid_dxf (filedxf, data, dia);
    } else if ((data->code == 0) && (strcmp (data->value, "POLYLINE") == 0)) {
      read_entity_polyline_dxf (filedxf, data, dia);
    } else if ((data->code == 0) && (strcmp (data->value, "CIRCLE") == 0)) {
      read_entity_circle_dxf (filedxf, data, dia);
    } else if ((data->code == 0) && (strcmp (data->value, "ELLIPSE") == 0)) {
      read_entity_ellipse_dxf (filedxf, data, dia);
    } else if ((data->code == 0) && (strcmp (data->value, "TEXT") == 0)) {
      read_entity_text_dxf (filedxf, data, dia);
    } else if ((data->code == 0) && (strcmp (data->value, "ARC") == 0)) {
      read_entity_arc_dxf (filedxf,data,dia);
    } else {
      if (read_dxf_codes (filedxf, data) == FALSE) {
          return;
      }
    }
  } while((data->code != 0) || (strcmp (data->value, "ENDSEC") != 0));
}


/* reads the blocks section of the dxf file */
static void
read_section_blocks_dxf (FILE *filedxf, DxfData *data, DiagramData *dia)
{
  int group_items = 0, group = 0;
  GList *group_list = NULL;
  DiaObject *obj = NULL;
  DiaLayer *group_layer = NULL;

  if (read_dxf_codes (filedxf, data) == FALSE){
    return;
  }

  do {
    if ((data->code == 0) && (strcmp (data->value, "LINE") == 0)) {
      obj = read_entity_line_dxf (filedxf, data, dia);
    } else if ((data->code == 0) && (strcmp (data->value, "SOLID") == 0)) {
      obj = read_entity_solid_dxf (filedxf, data, dia);
    } else if ((data->code == 0) && (strcmp (data->value, "VERTEX") == 0)) {
      read_entity_line_dxf (filedxf, data, dia);
    } else if ((data->code == 0) && (strcmp (data->value, "POLYLINE") == 0)) {
      obj = read_entity_polyline_dxf (filedxf, data, dia);
    } else if ((data->code == 0) && (strcmp (data->value, "CIRCLE") == 0)) {
      obj = read_entity_circle_dxf (filedxf, data, dia);
    } else if ((data->code == 0) && (strcmp (data->value, "ELLIPSE") == 0)) {
      obj = read_entity_ellipse_dxf (filedxf, data, dia);
    } else if ((data->code == 0) && (strcmp (data->value, "TEXT") == 0)) {
      obj = read_entity_text_dxf (filedxf, data, dia);
    } else if ((data->code == 0) && (strcmp (data->value, "ARC") == 0)) {
      obj = read_entity_arc_dxf (filedxf,data,dia);
    } else if ((data->code == 0) && (strcmp (data->value, "BLOCK") == 0)) {
      /* printf("Begin group\n" ); */

      group = TRUE;
      group_items = 0;
      group_list = NULL;
      group_layer = NULL;

      do {
        if (read_dxf_codes(filedxf, data) == FALSE) {
          return;
        }

        if (data->code == 8) {
          group_layer = layer_find_by_name (data->value, dia);
          data_set_active_layer (dia, group_layer);
        }
      } while(data->code != 0);
    } else if ((data->code == 0) && (strcmp (data->value, "ENDBLK") == 0)) {
      /* printf( "End group %d\n", group_items ); */

      if (group && group_items > 0 && group_list != NULL) {
        obj = group_create (group_list);
        if (NULL == group_layer) {
          dia_layer_add_object (dia_diagram_data_get_active_layer (dia),
                                obj);
        } else {
          dia_layer_add_object (group_layer, obj);
        }
      }

      group = FALSE;
      group_items = 0;
      group_list = NULL;
      obj = NULL;

      if (read_dxf_codes (filedxf, data) == FALSE) {
        return;
      }
    } else {
      if (read_dxf_codes (filedxf, data) == FALSE) {
        return;
      }
    }

    if (group && obj != NULL) {
      group_items++;

      group_list = g_list_prepend (group_list, obj);

      obj = NULL;
    }
  } while ((data->code != 0) || (strcmp (data->value, "ENDSEC") != 0));
}


/* imports the given dxf-file, returns TRUE if successful */
static gboolean
import_dxf (const char  *filename,
            DiagramData *dia,
            DiaContext  *ctx,
            void        *user_data)
{
  FILE *filedxf;
  DxfData *data;

  filedxf = g_fopen (filename,"r");
  if (filedxf == NULL) {
    dia_context_add_message (ctx,
                             _("Couldn't open: '%s' for reading.\n"),
                             dia_context_get_filename (ctx));
    return FALSE;
  }

  data = g_new0 (DxfData, 1);

  do {
    if (read_dxf_codes (filedxf, data) == FALSE) {
      g_clear_pointer (&data, g_free);
      dia_context_add_message (ctx,
                               _("read_dxf_codes failed on '%s'"),
                               dia_context_get_filename (ctx));
      return FALSE;
    } else {
      if (0 == data->code && strstr (data->codeline, "AutoCAD Binary DXF")) {
        g_clear_pointer (&data, g_free);
        dia_context_add_message (ctx,
                                 _("Binary DXF from '%s' not supported"),
                                 dia_context_get_filename (ctx));
        return FALSE;
      }

      if (0 == data->code) {
        if (strcmp (data->value, "SECTION") == 0) {
          /* don't think we need to do anything */
        } else if (strcmp (data->value, "ENDSEC") == 0) {
          /* don't think we need to do anything */
        } else if (strcmp (data->value, "EOF") == 0) {
          /* handled below */
        } else {
          g_printerr ("DXF 0:%s not handled\n", data->value);
        }
      } else if (data->code == 2) {
        if (strcmp (data->value, "ENTITIES") == 0) {
          /* printf( "reading section entities\n" );*/
          read_section_entities_dxf(filedxf, data, dia);
        } else if (strcmp (data->value, "BLOCKS") == 0) {
          /*printf( "reading section BLOCKS\n" );*/
          read_section_blocks_dxf (filedxf, data, dia);
        } else if (strcmp (data->value, "CLASSES") == 0) {
          /*printf( "reading section CLASSES\n" );*/
          read_section_classes_dxf (filedxf, data, dia);
        } else if (strcmp (data->value, "HEADER") == 0) {
          /*printf( "reading section HEADER\n" );*/
          read_section_header_dxf (filedxf, data, dia);
        } else if (strcmp (data->value, "TABLES") == 0) {
          /*printf( "reading section tables\n" );*/
          read_section_tables_dxf (filedxf, data, dia);
        } else if (strcmp (data->value, "OBJECTS") == 0) {
          /*printf( "reading section objects\n" );*/
          read_section_entities_dxf (filedxf, data, dia);
        }
      } else if (data->code == 999) {
        /* Don't complain on comments, but silently ignore */
      } else {
        g_warning ("Unknown dxf code %d", data->code);
      }
    }
  } while ((data->code != 0) || (strcmp (data->value, "EOF") != 0));

  g_clear_pointer (&data, g_free);
  g_clear_pointer (&_color_by_layer_ht, g_hash_table_unref);

  return TRUE;
}


/* reads a code/value pair from the DXF file */
static gboolean
read_dxf_codes (FILE *filedxf, DxfData *data)
{
  char *c;

  if (fgets (data->codeline, DXF_LINE_LENGTH, filedxf) == NULL) {
    return FALSE;
  }
  data->code = atoi (data->codeline);

  if (fgets (data->value, DXF_LINE_LENGTH, filedxf) == NULL) {
    return FALSE;
  }
  c = data->value;

  for (int i = 0; i < DXF_LINE_LENGTH; i++) {
    if ((c[i] == '\n') || (c[i] == '\r')) {
      c[i] = 0;
      break;
    }
  }

  return TRUE;
}


/* interface from filter.h */
static const char *extensions[] = {"dxf", NULL };
DiaImportFilter dxf_import_filter = {
  N_("Drawing Interchange File"),
  extensions,
  import_dxf
};
