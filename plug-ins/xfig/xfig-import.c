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
#include "../app/group.h"

#include "xfig.h"

#define BUFLEN 512

static Color fig_colors[FIG_MAX_USER_COLORS];

gboolean import_fig(const gchar *filename, DiagramData *dia, void* user_data);

static char **warnings;

#define WARNING_NO_POLYGONS 0
#define WARNING_NO_PATTERNS 1
#define WARNING_NO_TRIPLE_DOTS 2
#define WARNING_NEGATIVE_CORNER_RADIUS 3
#define WARNING_NO_SPLINES 4
#define MAX_WARNING 5

static void
fig_warn(int warning) {
    if (warnings == NULL) {
	warnings = g_malloc(sizeof(char*)*MAX_WARNING);
	warnings[0] = _("Polygon import is not implemented yes");
	warnings[1] = _("Patterns are not supported by Dia");
	warnings[2] = _("Triple-dotted lines are not supported by Dia, using double-dotted");
	warnings[3] = _("Negative corner radius, negating");
	warnings[4] = _("Spline import is not implemented yet");
    }
    if (warning >= MAX_WARNING) return;
    if (warnings[warning] != NULL) {
	message_warning(warnings[warning]);
	warnings[warning] = NULL;
    }
}

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
    /*   layer_add_object(dia->active_layer, new_obj); */

    props[0].name = "text";
    props[0].type = PROP_TYPE_STRING;
    PROP_VALUE_STRING(props[0]) = text;
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

    point.x = xpos;
    point.y = ypos;

    new_obj = otype->ops->create(&point, otype->default_user_data,
				 &h1, &h2);
    /*   layer_add_object(dia->active_layer, new_obj); */
  
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
  /*  layer_add_object(dia->active_layer, new_obj); */
  
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
create_standard_arc(real x1, real y1, real x2, real y2,
		    real radius, DiagramData *dia) {
    ObjectType *otype = object_get_type("Standard - Arc");
    Object *new_obj;
    Handle *h1, *h2;
    Point point;
    Property props[4];

    point.x = x1;
    point.y = y1;

    new_obj = otype->ops->create(&point, otype->default_user_data,
				 &h1, &h2);
    /*    layer_add_object(dia->active_layer, new_obj); */

    
    /*
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
    */
    props[0].name = "curve_distance";
    props[0].type = PROP_TYPE_REAL;
    PROP_VALUE_REAL(props[0]) = radius;

    new_obj->ops->set_props(new_obj, props, 1);

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
    /*    layer_add_object(dia->active_layer, new_obj); */
    
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
	fig_warn(WARNING_NO_PATTERNS);
    }
    
    return col;
}

static void
fig_simple_properties(Object *obj,
		      int line_style,
		      int thickness,
		      int pen_color,
		      int fill_color,
		      int area_fill) {
    Property props[5];
    int num_props = 0;

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
	    fig_warn(WARNING_NO_TRIPLE_DOTS);
	    PROP_VALUE_ENUM(props[num_props]) = LINESTYLE_DASH_DOT_DOT;
	    break;
	default:
	    message_error(_("Line style %d should not appear\n"), line_style);
	    PROP_VALUE_ENUM(props[num_props]) = LINESTYLE_SOLID;
	}
	num_props++;
    }
    props[num_props].name = "line_width";
    props[num_props].type = PROP_TYPE_REAL;
    PROP_VALUE_REAL(props[num_props]) = thickness/FIG_ALT_UNIT;
    num_props++;
    props[num_props].name = "line_colour";
    props[num_props].type = PROP_TYPE_COLOUR;
    PROP_VALUE_COLOUR(props[num_props]) = fig_color(pen_color);
    num_props++;
    if (area_fill == -1) {
	props[num_props].name = "show_background";
	props[num_props].type = PROP_TYPE_BOOL;
	PROP_VALUE_BOOL(props[num_props]) = FALSE;
	num_props++;
    } else {
	props[num_props].name = "fill_colour";
	props[num_props].type = PROP_TYPE_COLOUR;
	PROP_VALUE_COLOUR(props[num_props]) = fig_area_fill_color(area_fill, fill_color);
	num_props++;
    }

    obj->ops->set_props(obj, props, num_props);
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
     Property props[5];
     int num_props;
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
     
     num_props = 0;
     switch (sub_type) {
     case 4:
	 if (radius < 0) {
	     fig_warn(WARNING_NEGATIVE_CORNER_RADIUS);
  	     props[0].name = "corner_radius";
	     props[0].type = PROP_TYPE_REAL;
	     PROP_VALUE_REAL(props[0]) = -radius/FIG_ALT_UNIT;
	     num_props++;
	 } else {
	     props[0].name = "corner_radius";
	     props[0].type = PROP_TYPE_REAL;
	     PROP_VALUE_REAL(props[0]) = radius/FIG_ALT_UNIT;
	     num_props++;
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
	 newobj->ops->set_props(newobj, props, num_props);
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
     if (image_file != NULL)
	 g_free(image_file);

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

static Object *
fig_read_text(FILE *file, DiagramData *dia) {
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
    g_free(text_buf);

    props[num_props].name = "text_colour";
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
    props[num_props].name = "text_font";
    props[num_props].type = PROP_TYPE_FONT;
    PROP_VALUE_FONT(props[num_props]) = font_getfont(fig_fonts[font]);
    num_props++;

    newobj->ops->set_props(newobj, props, num_props);

    /* Depth field */
    if (compound_stack == NULL)
	depths[depth] = g_list_prepend(depths[depth], newobj);
    else
	if (compound_depth > depth) compound_depth = depth;

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
      fig_warn(WARNING_NO_SPLINES);
      return FALSE;
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
