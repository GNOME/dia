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
 
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

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

#include "intl.h"
#include "message.h"
#include "geometry.h"
#include "render.h"
#include "filter.h"
#include "object.h"
#include "properties.h"
#include "propinternals.h"
#include "../app/group.h"

#include "../objects/standard/create.h"
#include "xfig.h"

#define BUFLEN 512

static Color fig_colors[FIG_MAX_USER_COLORS];

gboolean import_fig(const gchar *filename, DiagramData *dia, void* user_data);

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
create_standard_text(real xpos, real ypos,
		     DiagramData *dia) {
    ObjectType *otype = object_get_type("Standard - Text");
    Object *new_obj;
    Handle *h1, *h2;
    Point point;

    if (otype == NULL){
	message_error(_("Can't find standard object"));
	return NULL;
    }

    point.x = xpos;
    point.y = ypos;

    new_obj = otype->ops->create(&point, otype->default_user_data,
				 &h1, &h2);
    
    return new_obj;
}

static PropDescription xfig_element_prop_descs[] = {
    { "elem_corner", PROP_TYPE_POINT },
    { "elem_width", PROP_TYPE_REAL },
    { "elem_height", PROP_TYPE_REAL },
    PROP_DESC_END};

static GPtrArray *make_element_props(real xpos, real ypos,
                                     real width, real height) 
{
    GPtrArray *props;
    PointProperty *pprop;
    RealProperty *rprop;

    props = prop_list_from_descs(xfig_element_prop_descs,pdtpp_true);
    g_assert(props->len == 3);
    
    pprop = g_ptr_array_index(props,0);
    pprop->point_data.x = xpos;
    pprop->point_data.y = ypos;
    rprop = g_ptr_array_index(props,1);
    rprop->real_data = width;
    rprop = g_ptr_array_index(props,2);
    rprop->real_data = height;
    
    return props;
}

static Object *
create_standard_ellipse(real xpos, real ypos, real width, real height,
			DiagramData *dia) {
    ObjectType *otype = object_get_type("Standard - Ellipse");
    Object *new_obj;
    Handle *h1, *h2;
    
    GPtrArray *props;
    Point point;

    if (otype == NULL){
	message_error(_("Can't find standard object"));
	return NULL;
    }

    point.x = xpos;
    point.y = ypos;

    new_obj = otype->ops->create(&point, otype->default_user_data,
				 &h1, &h2);
    /*   layer_add_object(dia->active_layer, new_obj); */
  
    props = make_element_props(xpos,ypos,width,height);
    new_obj->ops->set_props(new_obj, props);
    prop_list_free(props);

    return new_obj;
}


static Object *
create_standard_box(real xpos, real ypos, real width, real height,
		    DiagramData *dia) {
  ObjectType *otype = object_get_type("Standard - Box");
  Object *new_obj;
  Handle *h1, *h2;
  Point point;
  GPtrArray *props;

  if (otype == NULL){
      message_error(_("Can't find standard object"));
      return NULL;
  }

  point.x = xpos;
  point.y = ypos;

  new_obj = otype->ops->create(&point, otype->default_user_data,
			       &h1, &h2);
  /*  layer_add_object(dia->active_layer, new_obj); */
  
  props = make_element_props(xpos,ypos,width,height);
  new_obj->ops->set_props(new_obj, props);
  prop_list_free(props);

  return new_obj;
}

static Object *
create_standard_polyline(int num_points, 
			 Point *points,
			 DiagramData *dia) {
    ObjectType *otype = object_get_type("Standard - PolyLine");
    Object *new_obj;
    Handle *h1, *h2;
    PolylineCreateData *pcd;

    if (otype == NULL){
	message_error(_("Can't find standard object"));
	return NULL;
    }

    pcd = g_new(PolylineCreateData, 1);
    pcd->num_points = num_points;
    pcd->points = points;

    new_obj = otype->ops->create(NULL, pcd,
				 &h1, &h2);

    g_free(pcd);
    
    return new_obj;
}

static Object *
create_standard_polygon(int num_points, 
			Point *points,
			DiagramData *dia) {
    ObjectType *otype = object_get_type("Standard - Polygon");
    Object *new_obj;
    Handle *h1, *h2;
    PolylineCreateData *pcd;

    if (otype == NULL){
	message_error(_("Can't find standard object"));
	return NULL;
    }

    pcd = g_new(PolygonCreateData, 1);
    pcd->num_points = num_points;
    pcd->points = points;

    new_obj = otype->ops->create(NULL, pcd, &h1, &h2);

    g_free(pcd);
    
    return new_obj;
}

static Object *
create_standard_bezierline(int num_points, 
			   BezPoint *points,
			   DiagramData *dia) {
    ObjectType *otype = object_get_type("Standard - BezierLine");
    Object *new_obj;
    Handle *h1, *h2;
    BezierlineCreateData *bcd;

    if (otype == NULL){
	message_error(_("Can't find standard object"));
	return NULL;
    }

    bcd = g_new(BezierlineCreateData, 1);
    bcd->num_points = num_points;
    bcd->points = points;

    new_obj = otype->ops->create(NULL, bcd,
				 &h1, &h2);

    g_free(bcd);
    
    return new_obj;
}

static Object *
create_standard_beziergon(int num_points, 
			  BezPoint *points,
			  DiagramData *dia) {
    ObjectType *otype = object_get_type("Standard - Beziergon");
    Object *new_obj;
    Handle *h1, *h2;
    BeziergonCreateData *bcd;

    if (otype == NULL){
	message_error(_("Can't find standard object"));
	return NULL;
    }

    bcd = g_new(BeziergonCreateData, 1);
    bcd->num_points = num_points;
    bcd->points = points;

    new_obj = otype->ops->create(NULL, bcd,
				 &h1, &h2);

    g_free(bcd);
    
    return new_obj;
}


static PropDescription xfig_arc_prop_descs[] = {
    { "curve_distance", PROP_TYPE_REAL },
    PROP_DESC_END};

static Object *
create_standard_arc(real x1, real y1, real x2, real y2,
		    real radius, DiagramData *dia) {
    ObjectType *otype = object_get_type("Standard - Arc");
    Object *new_obj;
    Handle *h1, *h2;
    Point point;
    GPtrArray *props;

    if (otype == NULL){
	message_error(_("Can't find standard object"));
	return NULL;
    }

    point.x = x1;
    point.y = y1;

    new_obj = otype->ops->create(&point, otype->default_user_data,
				 &h1, &h2);
    /*    layer_add_object(dia->active_layer, new_obj); */
    
    props = prop_list_from_descs(xfig_arc_prop_descs,pdtpp_true);
    g_assert(props->len == 1);
    
    ((RealProperty *)g_ptr_array_index(props,0))->real_data = radius;
    new_obj->ops->set_props(new_obj, props);
    prop_list_free(props);

    return new_obj;
}

static PropDescription xfig_file_prop_descs[] = {
    { "image_file", PROP_TYPE_FILE },
    PROP_DESC_END};

static Object *
create_standard_image(real xpos, real ypos, real width, real height,
		      char *file, DiagramData *dia) {
    ObjectType *otype = object_get_type("Standard - Image");
    Object *new_obj;
    Handle *h1, *h2;
    Point point;
    GPtrArray *props;
    StringProperty *sprop;

    if (otype == NULL){
	message_error(_("Can't find standard object"));
	return NULL;
    }

    point.x = xpos;
    point.y = ypos;

    new_obj = otype->ops->create(&point, otype->default_user_data,
				 &h1, &h2);
    /*    layer_add_object(dia->active_layer, new_obj); */
    
    props = make_element_props(xpos,ypos,width,height);
    new_obj->ops->set_props(new_obj, props);
    prop_list_free(props);


    props = prop_list_from_descs(xfig_file_prop_descs,pdtpp_true);
    g_assert(props->len == 1);    
    sprop = g_ptr_array_index(props,0);
    g_free(sprop->string_data);
    sprop->string_data = g_strdup(file);
    new_obj->ops->set_props(new_obj, props);
    prop_list_free(props);

    return new_obj;
}

static Object *
create_standard_group(GList *items, DiagramData *dia) {
    Object *new_obj;

    new_obj = group_create((GList*)items);

    /*    layer_add_object(dia->active_layer, new_obj); */

    return new_obj;
}

static Color
fig_color(int color_index) {
    if (color_index == -1) return color_black; /* Default color */
    if (color_index < 32) return fig_default_colors[color_index];
    else return fig_colors[color_index-32];
}

static Color
fig_area_fill_color(int area_fill, int color_index) {
    Color col;
    col = fig_color(color_index);
    if (area_fill == -1) return col;
    if (area_fill >= 0 && area_fill <= 20) {
	if (color_index == -1 || color_index == 0) {
	    col.red = 0xff*(20-area_fill)/20;
	    col.green = 0xff*(20-area_fill)/20;
	    col.blue = 0xff*(20-area_fill)/20;
	} else {
	    col.red = (col.red*area_fill)/20;
	    col.green = (col.green*area_fill)/20;
	    col.blue = (col.blue*area_fill)/20;
	}
    } else if (area_fill > 20 && area_fill <= 40) {
	/* White and black area illegal here */
	col.red += (0xff-col.red)*(area_fill-20)/20;
	col.green += (0xff-col.green)*(area_fill-20)/20;
	col.blue += (0xff-col.blue)*(area_fill-20)/20;
    } else {
	message_warning(_("Patterns are not supported by Dia"));
    }
    
    return col;
}



static PropDescription xfig_simple_prop_descs_line[] = {
    { "line_width", PROP_TYPE_REAL },
    { "line_colour", PROP_TYPE_COLOUR },
    PROP_DESC_END};

static LineStyle 
fig_line_style_to_dia(int line_style) 
{
    switch (line_style) {
    case 0:
        return LINESTYLE_SOLID;
    case 1:
        return LINESTYLE_DASHED;
    case 2:
	return LINESTYLE_DOTTED;
    case 3:
        return LINESTYLE_DASH_DOT;
    case 4:
        return LINESTYLE_DASH_DOT_DOT;
    case 5:
        message_warning(_("Triple-dotted lines are not supported by Dia, "
			  "using double-dotted"));
        return LINESTYLE_DASH_DOT_DOT;
    default:
        message_error(_("Line style %d should not appear\n"), line_style);
        return LINESTYLE_SOLID;
    }
}

static void
fig_simple_properties(Object *obj,
		      int line_style,
		      int thickness,
		      int pen_color,
		      int fill_color,
		      int area_fill) {
    GPtrArray *props = prop_list_from_descs(xfig_simple_prop_descs_line,
                                            pdtpp_true);
    RealProperty *rprop;
    ColorProperty *cprop;

    g_assert(props->len == 2);
    
    rprop = g_ptr_array_index(props,0);
    rprop->real_data = thickness/FIG_ALT_UNIT;
    
    cprop = g_ptr_array_index(props,1);
    cprop->color_data = fig_color(pen_color);


    if (line_style != -1) {
        LinestyleProperty *lsprop = 
            (LinestyleProperty *)make_new_prop("line_style", 
                                               PROP_TYPE_LINESTYLE,
                                               PROP_FLAG_DONT_SAVE);
        lsprop->dash = 1.0;
        lsprop->style = fig_line_style_to_dia(line_style);

        g_ptr_array_add(props,lsprop);
    }

    if (area_fill == -1) {
        BoolProperty *bprop = 
            (BoolProperty *)make_new_prop("show_background",
                                          PROP_TYPE_BOOL,PROP_FLAG_DONT_SAVE);
        bprop->bool_data = FALSE;

        g_ptr_array_add(props,bprop);
    } else {
        ColorProperty *cprop = 
            (ColorProperty *)make_new_prop("fill_colour",
                                           PROP_TYPE_COLOUR,
                                           PROP_FLAG_DONT_SAVE);
        cprop->color_data = fig_area_fill_color(area_fill, fill_color);

        g_ptr_array_add(props,cprop);
    }

    obj->ops->set_props(obj, props);
    prop_list_free(props);
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

static GList *depths[1000];

/* If there's something in the compound stack, we ignore the depth field,
   as it will be determined by the group anyway */
static GSList *compound_stack = NULL;
/* When collection compounds, this is the *highest* depth an element is
   found at.  Since Dia groups are of one level, we put them all at that
   level.  Best we can do now. */
static int compound_depth;

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
    Object *newobj = NULL;

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
	       &end_x, &end_y) < 19) {
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
    if (newobj == NULL) return NULL;
    fig_simple_properties(newobj, line_style, thickness,
			  pen_color, fill_color, area_fill);

    /* Pen style field (not used) */
    /* Style_val (size of dots and dashes) in 1/80 inch */
    /* Direction (not used) */
    /* Angle -- can't rotate yet */

    /* Depth field */
    if (compound_stack == NULL)
	depths[depth] = g_list_prepend(depths[depth], newobj);
    else
	if (compound_depth > depth) compound_depth = depth;

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
    GPtrArray *props = g_ptr_array_new();
    Object *newobj = NULL;
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
    case 4: {
	RealProperty *rprop = 
	    (RealProperty *)make_new_prop("corner_radius",
					  PROP_TYPE_REAL,PROP_FLAG_DONT_SAVE);
	if (radius < 0) {
	    message_warning(_("Negative corner radius, negating"));
	    rprop->real_data = -radius/FIG_ALT_UNIT;
	} else {
	    rprop->real_data = radius/FIG_ALT_UNIT;
	}
	g_ptr_array_add(props,rprop);
    }
	/* Notice fallthrough */
    case 2: /* box */
	if (points[0].x > points[2].x) {
	    real tmp = points[0].x;
	    points[0].x = points[2].x;
	    points[2].x = tmp;
	}
	if (points[0].y > points[2].y) {
	    real tmp = points[0].y;
	    points[0].y = points[2].y;
	    points[2].y = tmp;
	}
	newobj = create_standard_box(points[0].x, points[0].y,
				     points[2].x-points[0].x,
				     points[2].y-points[0].y, dia);
	if (newobj == NULL) goto exit;
	newobj->ops->set_props(newobj, props);
	break;
    case 5: /* imported-picture bounding-box) */
	newobj = create_standard_image(points[0].x, points[0].y,
				       points[2].x-points[0].x,
				       points[2].y-points[0].y,
				       image_file, dia);
	if (newobj == NULL) goto exit;
	break;
    case 1: /* polyline */
	newobj = create_standard_polyline(npoints, points, dia);
	if (newobj == NULL) goto exit;
	break;
    case 3: /* polygon */
	newobj = create_standard_polygon(npoints, points, dia);
	if (newobj == NULL) goto exit;
	break;
    default: 
	message_error(_("Unknown polyline subtype: %d\n"), sub_type);
	goto exit;
    }

    fig_simple_properties(newobj, line_style, thickness,
			  pen_color, fill_color, area_fill);
    /* Pen style field (not used) */
    /* Style_val (size of dots and dashes) in 1/80 inch*/
    /* Join style */
    /* Cap style */
     
    /* Depth field */
    if (compound_stack == NULL)
	depths[depth] = g_list_prepend(depths[depth], newobj);
    else
	if (compound_depth > depth) compound_depth = depth;
 exit:
    prop_list_free(props);
    if (image_file != NULL)
	g_free(image_file);
    return newobj;
}

#define TENSION 0.25

static BezPoint *transform_spline(int npoints, Point *points, gboolean closed) {
    BezPoint *bezpoints = g_new(BezPoint, npoints);
    int i;
    Point vector;

    for (i = 0; i < npoints; i++) {
	bezpoints[i].p3 = points[i];
	bezpoints[i].type = BEZ_CURVE_TO;
    }
    bezpoints[0].type = BEZ_MOVE_TO;
    bezpoints[0].p1 = points[0];
    for (i = 1; i < npoints-1; i++) {
	bezpoints[i].p2 = points[i];
	bezpoints[i+1].p1 = points[i];
	vector = points[i-1];
	point_sub(&vector, &points[i+1]);
	point_scale(&vector, -TENSION);
	point_sub(&bezpoints[i].p2, &vector);
	point_add(&bezpoints[i+1].p1, &vector);
    }
    if (closed) {
	bezpoints[npoints-1].p2 = points[i];
	bezpoints[1].p1 = points[i];
	vector = points[npoints-2];
	point_sub(&vector, &points[1]);
	point_scale(&vector, -TENSION);
	point_sub(&bezpoints[npoints-1].p2, &vector);
	point_add(&bezpoints[1].p1, &vector);
    } else {
	bezpoints[1].p1 = points[0];
	bezpoints[npoints-1].p2 = bezpoints[npoints-1].p3;
    }
    return bezpoints;
}

static Object *
fig_read_spline(FILE *file, DiagramData *dia) {
    int sub_type;
    int line_style;
    int thickness;
    int pen_color;
    int fill_color;
    int depth;
    int pen_style;
    int area_fill;
    real style_val;
    int cap_style;
    int forward_arrow;
    int backward_arrow;
    int npoints;
    Point *points;
    GPtrArray *props = g_ptr_array_new();
    Object *newobj = NULL;
    BezPoint *bezpoints;
    int i;

    if (fscanf(file, "%d %d %d %d %d %d %d %d %lf %d %d %d %d\n",
	       &sub_type,
	       &line_style,
	       &thickness,
	       &pen_color,
	       &fill_color,
	       &depth,
	       &pen_style,
	       &area_fill,
	       &style_val,
	       &cap_style,
	       &forward_arrow,
	       &backward_arrow,
	       &npoints) != 13) {
	message_error(_("Couldn't read spline info: %s\n"), strerror(errno));
	return NULL;
    }

    if (forward_arrow == 1) {
	fig_read_arrow(file);
    }

    if (backward_arrow == 1) {
	fig_read_arrow(file);
    }

    if (!fig_read_n_points(file, npoints, &points)) {
	return NULL;
    }
     
    switch (sub_type) {
    case 0: /* Open approximated spline */
    case 1: /* Closed approximated spline */
	message_warning(_("Cannot convert approximated spline yet."));
	goto exit;
    case 2: /* Open interpolated spline */
    case 3: /* Closed interpolated spline */
	/* Despite what the Fig description says, interpolated splines
	   now also have the line with spline info from the X-spline */
    case 4: /* Open X-spline */
    case 5: /* Closed X-spline */
	{
	    gboolean interpolated = TRUE;
	    for (i = 0; i < npoints; i++) {
		double f;
		if (fscanf(file, " %lf ", &f) != 1) {
		    message_error(_("Couldn't read spline info: %s\n"),
				  strerror(errno));
		    goto exit;
		}
		if (f != -1.0 && f != 0.0) {
		    message_warning(_("Cannot convert approximated spline yet."));
		    interpolated = FALSE;
		}
	    }
	    if (!interpolated)
		goto exit;
	}
	/* Notice fallthrough */
	if (sub_type%2 == 0) {
	    bezpoints = transform_spline(npoints, points, FALSE);
	    newobj = create_standard_bezierline(npoints, bezpoints, dia);
	} else {
	    points = g_renew(Point, points, npoints+1);
	    points[npoints] = points[0];
	    npoints++;
	    bezpoints = transform_spline(npoints, points, TRUE);
	    newobj = create_standard_beziergon(npoints, bezpoints, dia);
	}
	if (newobj == NULL) goto exit;
	break;
    default: 
	message_error(_("Unknown spline subtype: %d\n"), sub_type);
	goto exit;
    }

    fig_simple_properties(newobj, line_style, thickness,
			  pen_color, fill_color, area_fill);
    /* Pen style field (not used) */
    /* Style_val (size of dots and dashes) in 1/80 inch*/
    /* Cap style */
     
    /* Depth field */
    if (compound_stack == NULL)
	depths[depth] = g_list_prepend(depths[depth], newobj);
    else
	if (compound_depth > depth) compound_depth = depth;
 exit:
    prop_list_free(props);
    g_free(points);
    return newobj;
}

static Object *
fig_read_arc(FILE *file, DiagramData *dia) {
    int sub_type;
    int line_style;
    int thickness;
    int pen_color;
    int fill_color;
    int depth;
    int pen_style;
    int area_fill;
    real style_val;
    int cap_style;
    int direction;
    int forward_arrow;
    int backward_arrow;
    Object *newobj = NULL;
    real center_x, center_y;
    int x1, y1;
    int x2, y2;
    int x3, y3;
    real radius;

    if (fscanf(file, "%d %d %d %d %d %d %d %d %lf %d %d %d %d %lf %lf %d %d %d %d %d %d\n",
	       &sub_type,
	       &line_style,
	       &thickness,
	       &pen_color,
	       &fill_color,
	       &depth,
	       &pen_style,
	       &area_fill,
	       &style_val,
	       &cap_style,
	       &direction,
	       &forward_arrow,
	       &backward_arrow,
	       &center_x, &center_y,
	       &x1, &y1,
	       &x2, &y2,
	       &x3, &y3) != 21) {
	message_error(_("Couldn't read arc info: %s\n"), strerror(errno));
	return NULL;
    }

    if (forward_arrow == 1) {
	fig_read_arrow(file);
    }

    if (backward_arrow == 1) {
	fig_read_arrow(file);
    }

    radius = sqrt((x1-center_x)*(x1-center_x)+(y1-center_y)*(y1-center_y))/FIG_UNIT;

    switch (sub_type) {
    case 0: /* We can't do pie-wedge properly yet */
    case 1: 
	newobj = create_standard_arc(x1/FIG_UNIT, y1/FIG_UNIT,
				     x3/FIG_UNIT, y3/FIG_UNIT,
				     radius, dia);
	if (newobj == NULL) return NULL;
	break;
    default: 
	message_error(_("Unknown polyline subtype: %d\n"), sub_type);
	return NULL;
    }

    fig_simple_properties(newobj, line_style, thickness,
			  pen_color, fill_color, area_fill);

    /* Pen style field (not used) */
    /* Style_val (size of dots and dashes) in 1/80 inch*/
    /* Join style */
    /* Cap style */
     
    /* Depth field */
    if (compound_stack == NULL)
	depths[depth] = g_list_prepend(depths[depth], newobj);
    else
	if (compound_depth > depth) compound_depth = depth;

    return newobj;
}

static PropDescription xfig_text_descs[] = {
   { "text", PROP_TYPE_TEXT },
    PROP_DESC_END
    /* Can't do the angle */
    /* Height and length are ignored */
    /* Flags */
};

static Object *
fig_read_text(FILE *file, DiagramData *dia) {
    GPtrArray *props = NULL;
    TextProperty *tprop;

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
    char *text_buf = NULL;

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

    newobj = create_standard_text(x, y, dia);
    if (newobj == NULL) goto exit;

    props = prop_list_from_descs(xfig_text_descs,pdtpp_true);
    g_assert(props->len == 4);

    tprop = g_ptr_array_index(props,0);
    tprop->text_data = g_strdup(text_buf);
    /*g_free(text_buf); */
    tprop->attr.alignment = sub_type;
    tprop->attr.position.x = x/FIG_UNIT;
    tprop->attr.position.y = y/FIG_UNIT;
    tprop->attr.font = dia_font_new_from_legacy_name(fig_fonts[font]);
    tprop->attr.height = font_size*3.54/72.0;
    tprop->attr.color = fig_color(color);
    newobj->ops->set_props(newobj, props);
    
    /* Depth field */
    if (compound_stack == NULL)
	depths[depth] = g_list_prepend(depths[depth], newobj);
    else
	if (compound_depth > depth) compound_depth = depth;

 exit:
    if (text_buf != NULL) free(text_buf);
    if (props != NULL) prop_list_free(props);
    return newobj;
}

static gboolean
fig_read_object(FILE *file, DiagramData *dia) {
  int objecttype;
  Object *item = NULL;

  if (fscanf(file, "%d ", &objecttype) != 1) {
      if (!feof(file))
	  message_error(_("Couldn't identify FIG object: %s\n"), strerror(errno));
      return FALSE;
  }

  switch (objecttype) {
  case -6: { /* End of compound */
      if (compound_stack == NULL) {
	  message_error(_("Compound end outside compound\n"));
	  return FALSE;
      }

      /* Make group item with these items */
      item = create_standard_group((GList*)compound_stack->data, dia);
      compound_stack = g_slist_remove(compound_stack, compound_stack->data);
      if (compound_stack == NULL) {
	  depths[compound_depth] = g_list_prepend(depths[compound_depth],
						  item);
      }
      break;
  }
  case 0: { /* Color pseudo-object. */
    int colornumber;
    int colorvalues;
    Color color;

    if (fscanf(file, " %d #%xd", &colornumber, &colorvalues) != 2) {
	message_error(_("Couldn't read color: %s\n"), strerror(errno));
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
      if (item == NULL) {
	  return FALSE;
      }
      break;
  }
  case 2: /* Polyline which includes polygon and box. */
      item = fig_read_polyline(file, dia);
      if (item == NULL) {
	  return FALSE;
      }
      break;
  case 3: /* Spline which includes closed/open control/interpolated spline. */
      item = fig_read_spline(file, dia);
      if (item == NULL) {
	  return FALSE;
      }
      break;
  case 4: /* Text. */
      item = fig_read_text(file, dia);
      if (item == NULL) {
	  return FALSE;
      }
      break;
  case 5: /* Arc. */
      item = fig_read_arc(file, dia);
      if (item == NULL) {
	  return FALSE;
      }
      break;
  case 6: {/* Compound object which is composed of one or more objects. */
      int dummy;
      if (fscanf(file, " %d %d %d %d\n", &dummy, &dummy, &dummy, &dummy) != 4) {
	  message_error(_("Couldn't read group extend: %s\n"), strerror(errno));
	  return FALSE;
      }
      /* Group extends don't really matter */
      if (compound_stack == NULL)
	  compound_depth = 999;
      compound_stack = g_slist_prepend(compound_stack, NULL);
      return TRUE;
      break;
  }
  default:
      message_error(_("Unknown object type %d\n"), objecttype);
      return FALSE;
      break;
  }
  if (compound_stack != NULL && item != NULL) { /* We're building a compound */
      GList *compound = (GList *)compound_stack->data;
      compound = g_list_prepend(compound, item);
      compound_stack->data = compound;
  }
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

  message_warning(_("Unknown paper size `%s', using default\n"), buf);
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
import_fig(const gchar *filename, DiagramData *dia, void* user_data) {
  FILE *figfile;
  int figmajor, figminor;	
  int i;

  for (i = 0; i < FIG_MAX_USER_COLORS; i++) {
    fig_colors[i] = color_black;
  }
  for (i = 0; i < 1000; i++) {
      depths[i] = NULL;
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
    return FALSE;
  }
  
  compound_stack = NULL;

  do {
      if (! fig_read_object(figfile, dia)) {
	  fclose(figfile);
	  break;
      }
  } while (TRUE);

  /* Now we can reorder for the depth fields */
  for (i = 999; i >= 0; i--) {
      if (depths[i] != NULL)
	  layer_add_objects_first(dia->active_layer, depths[i]);
  }
  return TRUE;
}

/* interface from filter.h */

static const gchar *extensions[] = {"fig", NULL };
DiaImportFilter xfig_import_filter = {
	N_("XFig File Format"),
	extensions,
	import_fig
};
