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

#include "config.h"

#include <glib/gi18n-lib.h>

#include <stdio.h>
/* Information used here is taken from the FIG Format 3.2 specification
   <URL:http://www.xfig.org/userman/fig-format.html>
   Some items left unspecified in the specifications are taken from the
   XFig source v. 3.2.3c
*/

#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <errno.h>
#include <locale.h>

#include <glib.h>
#include <glib/gstdio.h>

#include "geometry.h"
#include "filter.h"
#include "object.h"
#include "properties.h"
#include "propinternals.h"
#include "group.h"
#include "dia-layer.h"

#include "create.h"
#include "xfig.h"

#define BUFLEN 512

static Color fig_colors[FIG_MAX_USER_COLORS];

/** Eats the rest of the line.
 */
static void
eat_line(FILE *file) {
    char buf[BUFLEN];

    do {
	if (fgets(buf, BUFLEN, file) == NULL) {
	    return;
	}
	if (buf[strlen(buf)-1] == '\n') return;
    } while (!feof(file));
}

/** Skip past FIG comments (lines starting with #) and empty lines.
 * Returns TRUE if there is more in the file to read.
 */
static int
skip_comments(FILE *file) {
    int ch;

    while (!feof(file)) {
	if ((ch = fgetc(file)) == EOF) {
	    return FALSE;
	}

	if (ch == '\n') continue;
	else if (ch == '#') {
	    eat_line(file);
	    continue;
	} else {
	    ungetc(ch, file);
	    return TRUE;
	}
    }
    return FALSE;
}

static Color
fig_color(int color_index, DiaContext *ctx)
{
    if (color_index <= -1)
        return color_black; /* Default color */
    else if (color_index < FIG_MAX_DEFAULT_COLORS)
        return fig_default_colors[color_index];
    else if (color_index < FIG_MAX_USER_COLORS)
	return fig_colors[color_index-FIG_MAX_DEFAULT_COLORS];
    else {
        dia_context_add_message(ctx,
	  _("Color index %d too high; only 512 colors allowed. Using black instead."),
	  color_index);
	return color_black;
    }
}

static Color
fig_area_fill_color(int area_fill, int color_index, DiaContext *ctx)
{
    Color col;
    col = fig_color(color_index, ctx);
    if (area_fill == -1) return col;
    if (area_fill >= 0 && area_fill <= 20) {
	if (color_index == -1 || color_index == 0) {
	    col.red = 0xff*(20-area_fill)/20;
	    col.green = 0xff*(20-area_fill)/20;
	    col.blue = 0xff*(20-area_fill)/20;
	    col.alpha = 1.0;
	} else {
	    col.red = (col.red*area_fill)/20;
	    col.green = (col.green*area_fill)/20;
	    col.blue = (col.blue*area_fill)/20;
	    col.alpha = 1.0;
	}
    } else if (area_fill > 20 && area_fill <= 40) {
	/* White and black area illegal here */
	col.red += (0xff-col.red)*(area_fill-20)/20;
	col.green += (0xff-col.green)*(area_fill-20)/20;
	col.blue += (0xff-col.blue)*(area_fill-20)/20;
	col.alpha = 1.0;
    } else {
	dia_context_add_message(ctx, _("Patterns are not supported by Dia"));
    }

    return col;
}



static PropDescription xfig_simple_prop_descs_line[] = {
    { PROP_STDNAME_LINE_WIDTH, PROP_STDTYPE_LINE_WIDTH },
    { "line_colour", PROP_TYPE_COLOUR },
    PROP_DESC_END};


static DiaLineStyle
fig_line_style_to_dia (int line_style, DiaContext *ctx)
{
  switch (line_style) {
    case 0:
      return DIA_LINE_STYLE_SOLID;
    case 1:
      return DIA_LINE_STYLE_DASHED;
    case 2:
      return DIA_LINE_STYLE_DOTTED;
    case 3:
      return DIA_LINE_STYLE_DASH_DOT;
    case 4:
      return DIA_LINE_STYLE_DASH_DOT_DOT;
    case 5:
      dia_context_add_message (ctx, _("Triple-dotted lines are not supported by Dia; "
                                      "using double-dotted"));
      return DIA_LINE_STYLE_DASH_DOT_DOT;
    default:
      dia_context_add_message (ctx, _("Line style %d should not appear"), line_style);
      return DIA_LINE_STYLE_SOLID;
    }
}


static void
fig_simple_properties (DiaObject  *obj,
                       int         line_style,
                       float       dash_length,
                       int         thickness,
                       int         pen_color,
                       int         fill_color,
                       int         area_fill,
                       DiaContext *ctx)
{
  GPtrArray *props = prop_list_from_descs (xfig_simple_prop_descs_line,
                                            pdtpp_true);
  RealProperty *rprop;
  ColorProperty *cprop;

  g_assert(props->len == 2);

  rprop = g_ptr_array_index (props, 0);
  rprop->real_data = thickness / FIG_ALT_UNIT;

  cprop = g_ptr_array_index (props,1);
  cprop->color_data = fig_color (pen_color, ctx);


  if (line_style != -1) {
    LinestyleProperty *lsprop =
        (LinestyleProperty *) make_new_prop ("line_style",
                                             PROP_TYPE_LINESTYLE,
                                             PROP_FLAG_DONT_SAVE);
    lsprop->dash = dash_length / FIG_ALT_UNIT;
    lsprop->style = fig_line_style_to_dia (line_style, ctx);

    g_ptr_array_add (props, lsprop);
  }

  if (area_fill == -1) {
    BoolProperty *bprop =
        (BoolProperty *) make_new_prop ("show_background",
                                        PROP_TYPE_BOOL,
                                        PROP_FLAG_DONT_SAVE);
    bprop->bool_data = FALSE;

    g_ptr_array_add (props, bprop);
  } else {
    ColorProperty *prop =
        (ColorProperty *) make_new_prop ("fill_colour",
                                         PROP_TYPE_COLOUR,
                                         PROP_FLAG_DONT_SAVE);
    prop->color_data = fig_area_fill_color (area_fill, fill_color, ctx);

    g_ptr_array_add (props, prop);
  }

  dia_object_set_properties (obj, props);
  prop_list_free (props);
}


static int
fig_read_n_points(FILE *file, int n, Point **points, DiaContext *ctx)
{
    int i;
    GArray *points_list = g_array_sized_new(FALSE, FALSE, sizeof(Point), n);

    for (i = 0; i < n; i++) {
	int x,y;
	Point p;
	if (fscanf(file, " %d %d ", &x, &y) != 2) {
	    dia_context_add_message_with_errno (ctx, errno,
						_("Error while reading %dth of %d points"), i, n);
	    g_array_free(points_list, TRUE);
	    return FALSE;
	}
	p.x = x/FIG_UNIT;
	p.y = y/FIG_UNIT;
	g_array_append_val(points_list, p);
    }
    if (fscanf(file, "\n") == EOF)
      dia_context_add_message (ctx, _("Unexpected end of file."));

    *points = (Point *)points_list->data;
    g_array_free(points_list, FALSE);
    return TRUE;
}

static Arrow *
fig_read_arrow(FILE *file, DiaContext *ctx)
{
    int arrow_type, style;
    real thickness, width, height;
    Arrow *arrow;
    char* old_locale;

    old_locale = setlocale(LC_NUMERIC, "C");

    if (fscanf(file, "%d %d %lf %lf %lf\n",
	       &arrow_type, &style, &thickness,
	       &width, &height) != 5) {
	dia_context_add_message(ctx, _("Error while reading arrowhead"));
	setlocale(LC_NUMERIC,old_locale);
	return NULL;
    }
    setlocale(LC_NUMERIC,old_locale);

    arrow = g_new(Arrow, 1);

    switch (arrow_type) {
    case 0:
	arrow->type = ARROW_LINES;
	break;
    case 1:
	arrow->type = (style?ARROW_FILLED_TRIANGLE:ARROW_HOLLOW_TRIANGLE);
	break;
    case 2:
	arrow->type = (style?ARROW_FILLED_CONCAVE:ARROW_BLANKED_CONCAVE);
	break;
    case 3:
	arrow->type = (style?ARROW_FILLED_DIAMOND:ARROW_HOLLOW_DIAMOND);
	break;
    default:
	dia_context_add_message(ctx, _("Unknown arrow type %d\n"), arrow_type);
	g_clear_pointer (&arrow, g_free);
	return NULL;
    }
    arrow->width = width/FIG_UNIT;
    arrow->length = height/FIG_UNIT;

    return arrow;
}

static gchar *
fig_fix_text(gchar *text) {
    int i, j;
    int asciival;
    GError *err = NULL;
    gchar *converted;
    gboolean needs_conversion = FALSE;

    for (i = 0, j = 0; text[i] != 0; i++, j++) {
	if (text[i] == '\\') {
	    sscanf(text+i+1, "%3o", &asciival);
	    text[j] = asciival;
	    i+=3;
	    needs_conversion = TRUE;
	} else {
	    text[j] = text[i];
	}
    }
    /* Strip final newline */
    text[j-1] = 0;
    if (text[j-2] == '\001') {
	text[j-2] = 0;
    }
    if (needs_conversion) {
      /* Crudely assuming that fig uses Latin-1 */
      converted = g_convert (text,
                             strlen (text),
                             "UTF-8",
                             "ISO-8859-1",
                             NULL,
                             NULL,
                             &err);
      if (err != NULL) {
        g_printerr ("Error converting %s: %s\n", text, err->message);
        return text;
      }
      if (!g_utf8_validate (converted, -1, NULL)) {
        g_printerr ("Fails to validate %s\n", converted);
        return text;
      }
      if (text != converted) g_clear_pointer (&text, g_free);
      return converted;
    } else {
      return text;
    }
}


static char *
fig_read_text_line (FILE *file)
{
  char *text_buf;
  guint text_alloc, text_len;

  getc (file);
  text_alloc = 80;
  text_buf = g_new0 (char, text_alloc);
  text_len = 0;

  while (fgets (text_buf + text_len, text_alloc - text_len, file) != NULL) {
    if (strlen (text_buf) < text_alloc-1) {
      break;
    }
    text_len = text_alloc;
    text_alloc *= 2;
    text_buf = g_renew (char, text_buf, text_alloc);
  }

  text_buf = fig_fix_text (text_buf);

  return text_buf;
}


static GList *depths[FIG_MAX_DEPTHS];

/* If there's something in the compound stack, we ignore the depth field,
   as it will be determined by the group anyway */
static GSList *compound_stack = NULL;
/* When collection compounds, this is the *highest* depth an element is
   found at.  Since Dia groups are of one level, we put them all at that
   level.  Best we can do now. */
static int compound_depth;

/** Add an object at a given depth.  This function checks for depth limits
 * and updates the compound depth if needed.
 *
 * @param newobj An object to add.  If we're inside a compound, this
 * doesn't really add the object.
 * @param depth A depth as in the Fig format, max 999
 */
static void
add_at_depth(DiaObject *newobj, int depth, DiaContext *ctx)
{
    if (depth < 0 || depth >= FIG_MAX_DEPTHS) {
	dia_context_add_message(ctx, _("Depth %d out of range, only 0-%d allowed.\n"),
		      depth, FIG_MAX_DEPTHS-1);
	depth = FIG_MAX_DEPTHS - 1;
    }
    if (compound_stack == NULL)
	depths[depth] = g_list_append(depths[depth], newobj);
    else
	if (compound_depth > depth) compound_depth = depth;
}

static DiaObject *
fig_read_ellipse(FILE *file, DiaContext *ctx)
{
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
    DiaObject *newobj = NULL;
    char* old_locale;

    old_locale = setlocale(LC_NUMERIC, "C");
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
	dia_context_add_message_with_errno(ctx, errno, _("Couldn't read ellipse info."));
	setlocale(LC_NUMERIC, old_locale);
	return NULL;
    }
    setlocale(LC_NUMERIC, old_locale);

    /* Curiously, the sub_type doesn't matter, as all info can be
       extracted this way */
    newobj = create_standard_ellipse((center_x-radius_x)/FIG_UNIT,
				     (center_y-radius_y)/FIG_UNIT,
				     (2*radius_x)/FIG_UNIT,
				     (2*radius_y)/FIG_UNIT);
    if (newobj == NULL) return NULL;
    fig_simple_properties(newobj, line_style, style_val, thickness,
			  pen_color, fill_color, area_fill, ctx);

    /* Pen style field (not used) */
    /* Style_val (size of dots and dashes) in 1/80 inch */
    /* Direction (not used) */
    /* Angle -- can't rotate yet */

    /* Depth field */
    add_at_depth(newobj, depth, ctx);

    return newobj;
}

static DiaObject *
fig_read_polyline(FILE *file, DiaContext *ctx)
{
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
    int forward_arrow, backward_arrow;
    Arrow *forward_arrow_info = NULL, *backward_arrow_info = NULL;
    int npoints;
    Point *points = NULL;
    GPtrArray *props = g_ptr_array_new();
    DiaObject *newobj = NULL;
    int flipped = 0;
    char *image_file = NULL;
    char* old_locale;

    old_locale = setlocale(LC_NUMERIC, "C");
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
	dia_context_add_message_with_errno(ctx, errno, _("Couldn't read polyline info.\n"));
	goto exit;
    }

    if (forward_arrow == 1) {
	forward_arrow_info = fig_read_arrow(file, ctx);
    }

    if (backward_arrow == 1) {
	backward_arrow_info = fig_read_arrow(file, ctx);
    }

    if (sub_type == 5) { /* image has image name before npoints */
	/* Despite what the specs say */
	if (fscanf(file, " %d", &flipped) != 1) {
	    dia_context_add_message_with_errno(ctx, errno, _("Couldn't read flipped bit."));
	    goto exit;
	}

	image_file = fig_read_text_line(file);

    }

    if (!fig_read_n_points(file, npoints, &points, ctx)) {
	goto exit;
    }

    switch (sub_type) {
    case 4: {
	RealProperty *rprop =
	    (RealProperty *)make_new_prop("corner_radius",
					  PROP_TYPE_REAL,PROP_FLAG_DONT_SAVE);
	if (radius < 0) {
	    dia_context_add_message(ctx, _("Negative corner radius; negating"));
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
				     points[2].y-points[0].y);
	if (newobj == NULL) goto exit;
	newobj->ops->set_props(newobj, props);
	break;
    case 5: /* imported-picture bounding-box) */
	newobj = create_standard_image(points[0].x, points[0].y,
				       points[2].x-points[0].x,
				       points[2].y-points[0].y,
				       image_file);
	if (newobj == NULL) goto exit;
	break;
    case 1: /* polyline */
	newobj = create_standard_polyline(npoints, points,
					  forward_arrow_info,
					  backward_arrow_info);
	if (newobj == NULL) goto exit;
	break;
    case 3: /* polygon */
	newobj = create_standard_polygon(npoints, points);
	if (newobj == NULL) goto exit;
	break;
    default:
	dia_context_add_message(ctx, _("Unknown polyline subtype: %d\n"), sub_type);
	goto exit;
    }

    fig_simple_properties(newobj, line_style, style_val, thickness,
			  pen_color, fill_color, area_fill, ctx);
    /* Pen style field (not used) */
    /* Style_val (size of dots and dashes) in 1/80 inch*/
    /* Join style */
    /* Cap style */

    /* Depth field */
    add_at_depth(newobj, depth, ctx);
 exit:
    setlocale(LC_NUMERIC, old_locale);
    prop_list_free(props);
    g_clear_pointer (&points, g_free);
    g_clear_pointer (&forward_arrow_info, g_free);
    g_clear_pointer (&backward_arrow_info, g_free);
    g_clear_pointer (&image_file, g_free);
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

/* The XFig 'X-Splines' seem to be a generalization of cubic B-Splines
 * and Catmull-Rom splines.
 * Our Beziers, if Beziers they are, have the basis matrix M_bez
 * [-1  3 -3  1]
 * [ 3 -6  3  0]
 * [-3  3  0  0]
 * [ 1  0  0  0]
 *
 * The basis matrix for cubic B-Splines according to Hearn & Baker (M_b)
 * [-1  3 -3  1]
 * [ 3 -6  3  0]*1/6
 * [-3  0  3  0]
 * [ 1  4  1  0]
 * So the transformation matrix (M_bez^-1)*M_b should be
 * [ 1  4  1  0]
 * [ 0  4  2  0]*1/6
 * [ 0  2  4  0]
 * [ 0  1  4  1]
 *
 * The basis matrix for Catmull-Rom splines (M_c) is:
 * [-s 2-s s-2 s]
 * [2s s-3 3-2s -s]
 * [-s  0  s  0]
 * [ 0  1  0  0] where s = (1-t)/2 and t = 0, so s = 1/2:
 * [ -.5  1.5 -1.5   .5]
 * [   1 -2.5    2  -.5]
 * [ -.5    0   .5    0]
 * [   0    1    4    1]
 * Thus, the transformation matrix for the interpolated splines should be
 * (M_bez^-1)*M_c
 * The conversion matrix should then be:
 * [0    1    4    1]
 * [-1/6 1 4+1/6   1]
 * [0   1/6   5  5/6]
 * [0    0    5    1]
 * or
 * [ 0  6  24  6]
 * [-1  6  25  6]*1/6
 * [ 0  1  30  5]
 * [ 0  0  30  5]
 */
G_GNUC_UNUSED
static real matrix_bspline_to_bezier[4][4] =
    {{1/6.0, 4/6.0, 1/6.0, 0},
     {0,   4/6.0, 2/6.0, 0},
     {0,   2/6.0, 4/6.0, 0},
     {0,   1/6.0, 4/6.0, 1/6.0}};

G_GNUC_UNUSED
static real matrix_catmull_to_bezier[4][4] =
    {{0,      1,   4,     1},
     {-1/6.0, 1, 25/26.0, 1},
     {0,      1,   5,   5/6.0},
     {0,      0,   5,   5/6.0}};

static DiaObject *
fig_read_spline(FILE *file, DiaContext *ctx)
{
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
    int forward_arrow, backward_arrow;
    Arrow *forward_arrow_info = NULL, *backward_arrow_info = NULL;
    int npoints;
    Point *points = NULL;
    GPtrArray *props = g_ptr_array_new();
    DiaObject *newobj = NULL;
    BezPoint *bezpoints;
    int i;
    char* old_locale;

    old_locale = setlocale(LC_NUMERIC, "C");
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
	dia_context_add_message_with_errno(ctx, errno, _("Couldn't read spline info."));
	goto exit;
    }

    if (forward_arrow == 1) {
	forward_arrow_info = fig_read_arrow(file, ctx);
    }

    if (backward_arrow == 1) {
	backward_arrow_info = fig_read_arrow(file, ctx);
    }

    if (!fig_read_n_points(file, npoints, &points, ctx)) {
	goto exit;
    }

    switch (sub_type) {
    case 0: /* Open approximated spline */
    case 1: /* Closed approximated spline */
	dia_context_add_message(ctx, _("Cannot convert approximated spline yet."));
	goto exit;
    case 2: /* Open interpolated spline */
    case 3: /* Closed interpolated spline */
	/* Despite what the Fig description says, interpolated splines
	   now also have the line with spline info from the X-spline */
    case 4: /* Open X-spline */
    case 5: /* Closed X-spline */
	{
	    double f;
	    gboolean interpolated = TRUE;
	    for (i = 0; i < npoints; i++) {
		if (fscanf(file, " %lf ", &f) != 1) {
		    dia_context_add_message_with_errno(ctx, errno,_("Couldn't read spline info."));
		    goto exit;
		}
		if (f != -1.0 && f != 0.0) {
		    dia_context_add_message(ctx, _("Cannot convert approximated spline yet."));
		    interpolated = FALSE;
		}
	    }
	    if (!interpolated)
		goto exit;
	    /* Matrix-based conversion not ready yet. */
#if 0
	    if (sub_type%2 == 0) {
		bezpoints = fig_transform_spline(npoints, points, FALSE, f);
		newobj = create_standard_bezierline(npoints, bezpoints,
						    forward_arrow_info,
						    backward_arrow_info);
	    } else {
		points = g_renew(Point, points, npoints+1);
		points[npoints] = points[0];
		npoints++;
		bezpoints = fig_transform_spline(npoints, points, TRUE, f);
		newobj = create_standard_beziergon(npoints, bezpoints);
	    }
#else
	    if (sub_type%2 == 0) {
		bezpoints = transform_spline(npoints, points, FALSE);
		newobj = create_standard_bezierline(npoints, bezpoints,
						    forward_arrow_info,
						    backward_arrow_info);
	    } else {
		points = g_renew(Point, points, npoints+1);
		points[npoints] = points[0];
		npoints++;
		bezpoints = transform_spline(npoints, points, TRUE);
		newobj = create_standard_beziergon(npoints, bezpoints);
	    }
#endif
            g_clear_pointer(&bezpoints, g_free);
	}
	if (newobj == NULL) goto exit;
	break;
    default:
	dia_context_add_message(ctx, _("Unknown spline subtype: %d\n"), sub_type);
	goto exit;
    }

    fig_simple_properties(newobj, line_style, style_val, thickness,
			  pen_color, fill_color, area_fill, ctx);
    /* Pen style field (not used) */
    /* Style_val (size of dots and dashes) in 1/80 inch*/
    /* Cap style */

    /* Depth field */
    add_at_depth(newobj, depth, ctx);
 exit:
    setlocale(LC_NUMERIC, old_locale);
    prop_list_free(props);
    g_clear_pointer (&forward_arrow_info, g_free);
    g_clear_pointer (&backward_arrow_info, g_free);
    g_clear_pointer (&points, g_free);
    return newobj;
}

static DiaObject *
fig_read_arc(FILE *file, DiaContext *ctx)
{
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
    int forward_arrow, backward_arrow;
    Arrow *forward_arrow_info = NULL, *backward_arrow_info = NULL;
    DiaObject *newobj = NULL;
    real center_x, center_y;
    int x1, y1;
    int x2, y2;
    int x3, y3;
    char* old_locale;
    Point p2, pm;
    real distance;

    old_locale = setlocale(LC_NUMERIC, "C");
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
	dia_context_add_message_with_errno(ctx, errno, _("Couldn't read arc info."));
	goto exit;
    }

    if (forward_arrow == 1) {
	forward_arrow_info = fig_read_arrow(file, ctx);
    }

    if (backward_arrow == 1) {
	backward_arrow_info = fig_read_arrow(file, ctx);
    }

    p2.x = x2/FIG_UNIT; p2.y = y2/FIG_UNIT;
    pm.x = (x1+x3)/(2*FIG_UNIT); pm.y = (y1+y3)/(2*FIG_UNIT);
    distance = distance_point_point (&p2, &pm);

    switch (sub_type) {
    case 0:
    case 1:
    case 2: /* We can't do pie-wedge properly yet */
	newobj = create_standard_arc(x1/FIG_UNIT, y1/FIG_UNIT,
				     x3/FIG_UNIT, y3/FIG_UNIT,
				     direction ? distance : -distance,
				     forward_arrow_info,
				     backward_arrow_info);
	if (newobj == NULL) goto exit;
	if (sub_type == 2) {
		/* set new fill property on arc? */
		dia_context_add_message(ctx, _("Filled arc treated as unfilled"));
	}
	break;
    default:
	dia_context_add_message(ctx, _("Unknown polyline arc: %d\n"), sub_type);
	goto exit;
    }

    fig_simple_properties(newobj, line_style, style_val, thickness,
			  pen_color, fill_color, area_fill, ctx);

    /* Pen style field (not used) */
    /* Style_val (size of dots and dashes) in 1/80 inch*/
    /* Join style */
    /* Cap style */

    /* Depth field */
    add_at_depth(newobj, depth, ctx);

 exit:
    setlocale(LC_NUMERIC, old_locale);
    g_clear_pointer (&forward_arrow_info, g_free);
    g_clear_pointer (&backward_arrow_info, g_free);
    return newobj;
}

static PropDescription xfig_text_descs[] = {
    { "text", PROP_TYPE_TEXT },
    PROP_DESC_END
    /* Can't do the angle */
    /* Height and length are ignored */
    /* Flags */
};

static DiaObject *
fig_read_text(FILE *file, DiaContext *ctx)
{
    GPtrArray *props = NULL;
    TextProperty *tprop;

    DiaObject *newobj = NULL;
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
    char* old_locale;

    old_locale = setlocale(LC_NUMERIC, "C");
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
	dia_context_add_message_with_errno(ctx, errno, _("Couldn't read text info."));
	setlocale(LC_NUMERIC, old_locale);
	return NULL;
    }
    /* Skip one space exactly */
    text_buf = fig_read_text_line(file);

    newobj = create_standard_text(x/FIG_UNIT, y/FIG_UNIT);
    if (newobj == NULL) goto exit;

    props = prop_list_from_descs(xfig_text_descs,pdtpp_true);

    tprop = g_ptr_array_index(props,0);
    tprop->text_data = g_strdup(text_buf);
    /*g_clear_pointer (&text_buf, g_free); */
    tprop->attr.alignment = sub_type;
    tprop->attr.position.x = x/FIG_UNIT;
    tprop->attr.position.y = y/FIG_UNIT;

    if ((font_flags & 4) == 0) {
	switch (font) {
	case 0: tprop->attr.font = dia_font_new_from_legacy_name("Times-Roman"); break;
	case 1: tprop->attr.font = dia_font_new_from_legacy_name("Times-Roman"); break;
	case 2: tprop->attr.font = dia_font_new_from_legacy_name("Times-Bold"); break;
	case 3: tprop->attr.font = dia_font_new_from_legacy_name("Times-Italic"); break;
	case 4: tprop->attr.font = dia_font_new_from_legacy_name("Helvetica"); break;
	case 5: tprop->attr.font = dia_font_new_from_legacy_name("Courier"); break;
	default:
	    dia_context_add_message(ctx, _("Can't find LaTeX font nr. %d, using sans"), font);
	    tprop->attr.font = dia_font_new_from_legacy_name("Helvetica");
	}
    } else {
	if (font == -1) {
	    /* "Default font" - wazzat? */
	    tprop->attr.font = dia_font_new_from_legacy_name("Times-Roman");
	} else if (font < 0 || font >= num_fig_fonts()) {
	    dia_context_add_message(ctx, _("Can't find PostScript font nr. %d, using sans"), font);
	    tprop->attr.font = dia_font_new_from_legacy_name("Helvetica");
	} else {
	    tprop->attr.font = dia_font_new_from_legacy_name(fig_fonts[font]);
	}
    }
    tprop->attr.height = font_size*2.54/72.0;
    tprop->attr.color = fig_color(color, ctx);
    dia_object_set_properties (newobj, props);

    /* Depth field */
    add_at_depth(newobj, depth, ctx);

 exit:
    setlocale (LC_NUMERIC, old_locale);
    g_clear_pointer (&text_buf, g_free);
    g_clear_pointer (&props, prop_list_free);

    return newobj;
}

static gboolean
fig_read_object(FILE *file, DiaContext *ctx)
{
    int objecttype;
    DiaObject *item = NULL;

    if (fscanf(file, "%d ", &objecttype) != 1) {
	if (!feof(file)) {
	    dia_context_add_message_with_errno(ctx, errno, _("Couldn't identify Fig object."));
	}
	return FALSE;
    }

    switch (objecttype) {
    case -6: { /* End of compound */
	if (compound_stack == NULL) {
	    dia_context_add_message(ctx, _("Compound end outside compound\n"));
	    return FALSE;
	}

	/* Make group item with these items */
	if (g_list_length((GList*)compound_stack->data) > 1)
	    item = create_standard_group((GList*)compound_stack->data);
	else /* a single item needs no group */
	    item = (DiaObject *)((GList*)compound_stack->data)->data;
	compound_stack = g_slist_remove(compound_stack, compound_stack->data);
	if (compound_stack == NULL) {
	    depths[compound_depth] = g_list_append(depths[compound_depth],
						    item);
	}
	break;
    }
    case 0: { /* Color pseudo-object. */
	int colornumber;
	int colorvalues;
	Color color;

	if (fscanf(file, " %d #%xd", &colornumber, &colorvalues) != 2) {
	    dia_context_add_message_with_errno(ctx, errno, _("Couldn't read color\n"));
	    return FALSE;
	}

	if (colornumber < 32 || colornumber > FIG_MAX_USER_COLORS) {
	    dia_context_add_message(ctx, _("Color number %d out of range 0..%d.  Discarding color.\n"),
			  colornumber, FIG_MAX_USER_COLORS);
	    return FALSE;
	}

	color.red = ((colorvalues & 0x00ff0000)>>16) / 255.0;
	color.green = ((colorvalues & 0x0000ff00)>>8) / 255.0;
	color.blue = (colorvalues & 0x000000ff) / 255.0;
	color.alpha = 1.0;

	fig_colors[colornumber-32] = color;
	break;
    }
    case 1: { /* Ellipse which is a generalization of circle. */
	item = fig_read_ellipse(file, ctx);
	if (item == NULL) {
	    return FALSE;
	}
	break;
    }
    case 2: /* Polyline which includes polygon and box. */
	item = fig_read_polyline(file, ctx);
	if (item == NULL) {
	    return FALSE;
	}
	break;
    case 3: /* Spline which includes closed/open control/interpolated spline. */
	item = fig_read_spline(file, ctx);
	if (item == NULL) {
	    return FALSE;
	}
	break;
    case 4: /* Text. */
	item = fig_read_text(file, ctx);
	if (item == NULL) {
	    return FALSE;
	}
	break;
    case 5: /* Arc. */
	item = fig_read_arc(file, ctx);
	if (item == NULL) {
	    return FALSE;
	}
	break;
    case 6: {/* Compound object which is composed of one or more objects. */
	int dummy;
	if (fscanf(file, " %d %d %d %d\n", &dummy, &dummy, &dummy, &dummy) != 4) {
	    dia_context_add_message_with_errno(ctx, errno, _("Couldn't read group extent."));
	    return FALSE;
	}
	/* Group extends don't really matter */
	if (compound_stack == NULL)
	    compound_depth = FIG_MAX_DEPTHS - 1;
	compound_stack = g_slist_append(compound_stack, NULL);
	return TRUE;
	break;
    }
    default:
	dia_context_add_message(ctx, _("Unknown object type %d\n"), objecttype);
	return FALSE;
	break;
    }
    if (compound_stack != NULL && item != NULL) { /* We're building a compound */
	GList *compound = (GList *)compound_stack->data;
	compound = g_list_append(compound, item);
	compound_stack->data = compound;
    }
    return TRUE;
}

static int
fig_read_line_choice(FILE *file, char *choice1, char *choice2, DiaContext *ctx)
{
    char buf[BUFLEN];

    if (!fgets(buf, BUFLEN, file)) {
	return -1;
    }

    buf[strlen(buf)-1] = 0; /* Remove trailing newline */
    g_strstrip(buf); /* And any other whitespace */
    if (!g_ascii_strcasecmp(buf, choice1)) return 0;
    if (!g_ascii_strcasecmp(buf, choice2)) return 1;

  dia_context_add_message (ctx,
                           _("“%s” is not one of “%s” or “%s”\n"),
                           buf,
                           choice1,
                           choice2);

  return 0;
}


static int
fig_read_paper_size(FILE *file, DiagramData *dia, DiaContext *ctx) {
    char buf[BUFLEN];
    int paper;

    if (!fgets(buf, BUFLEN, file)) {
	dia_context_add_message_with_errno(ctx, errno, _("Error reading paper size."));
	return FALSE;
    }

    buf[strlen(buf)-1] = 0; /* Remove trailing newline */
    g_strstrip(buf); /* And any other whitespace */
    if ((paper = find_paper(buf)) != -1) {
	get_paper_info(&dia->paper, paper, NULL);
	return TRUE;
    }

  dia_context_add_message (ctx,
                           _("Unknown paper size “%s”, using default\n"),
                           buf);

  return TRUE;
}


int figversion;

static int
fig_read_meta_data(FILE *file, DiagramData *dia, DiaContext *ctx)
{
    if (figversion >= 300) { /* Might exist earlier */
	int portrait;

	if ((portrait = fig_read_line_choice(file, "Portrait", "Landscape", ctx)) == -1) {
	    dia_context_add_message(ctx, _("Error reading paper orientation."));
	    return FALSE;
	}
	dia->paper.is_portrait = portrait;
    }

    if (figversion >= 300) { /* Might exist earlier */
	int justify;

	if ((justify = fig_read_line_choice(file, "Center", "Flush Left", ctx)) == -1) {
	    dia_context_add_message(ctx, _("Error reading justification."));
	    return FALSE;
	}
	/* Don't know what to do with this */
    }

    if (figversion >= 300) { /* Might exist earlier */
	int units;

	if ((units = fig_read_line_choice(file, "Metric", "Inches", ctx)) == -1) {
	    dia_context_add_message(ctx, _("Error reading units."));
	    return FALSE;
	}
	/* Don't know what to do with this */
    }

    if (figversion >= 302) {
	if (!fig_read_paper_size(file, dia, ctx)) return FALSE;
    }

    {
	real mag;
	char* old_locale;

	old_locale = setlocale(LC_NUMERIC, "C");
	if (fscanf(file, "%lf\n", &mag) != 1) {
	    dia_context_add_message_with_errno(ctx, errno, _("Error reading magnification."));
	    setlocale(LC_NUMERIC, old_locale);
	    return FALSE;
	}
        setlocale(LC_NUMERIC, old_locale);

	dia->paper.scaling = mag/100;
    }

    if (figversion >= 302) {
	int multiple;

	if ((multiple = fig_read_line_choice(file, "Single", "Multiple", ctx)) == -1) {
	    dia_context_add_message(ctx, _("Error reading multipage indicator."));
	    return FALSE;
	}

	/* Don't know what to do with this */
    }

    {
	int transparent;

	if (fscanf(file, "%d\n", &transparent) != 1) {
	    dia_context_add_message_with_errno(ctx, errno, _("Error reading transparent color."));
	    return FALSE;
	}

	/* Don't know what to do with this */
    }

    if (!skip_comments(file)) {
	if (!feof(file)) {
	    dia_context_add_message_with_errno(ctx, errno, _("Error reading Fig file."));
	} else {
	    dia_context_add_message(ctx, _("Premature end of Fig file\n"));
	}
	return FALSE;
    }

    {
	int resolution, coord_system;

	if (fscanf(file, "%d %d\n", &resolution, &coord_system) != 2) {
	    dia_context_add_message_with_errno(ctx, errno, _("Error reading resolution."));
	    return FALSE;
	}

	/* Don't know what to do with this */
    }
    return TRUE;
}

/* imports the given fig-file, returns TRUE if successful */
static gboolean
import_fig(const gchar *filename, DiagramData *dia, DiaContext *ctx, void* user_data)
{
    FILE *figfile;
    char buf[BUFLEN];
    int figmajor, figminor;
    int i;

    for (i = 0; i < FIG_MAX_USER_COLORS; i++) {
	fig_colors[i] = color_black;
    }
    for (i = 0; i < FIG_MAX_DEPTHS; i++) {
	depths[i] = NULL;
    }

    figfile = g_fopen(filename,"r");
    if (figfile == NULL) {
	dia_context_add_message_with_errno(ctx, errno, _("Couldn't open: '%s' for reading.\n"),
		                dia_context_get_filename(ctx));
	return FALSE;
    }

    /* First check magic bytes */
    if (fgets(buf, BUFLEN, figfile) == NULL ||
        sscanf(buf, "#FIG %d.%d\n", &figmajor, &figminor) != 2)
    {
	dia_context_add_message_with_errno(ctx, errno, _("Doesn't look like a Fig file"));
	fclose(figfile);
	return FALSE;
    }

    if (figmajor != 3 || figminor != 2) {
	dia_context_add_message(ctx, _("This is a Fig version %d.%d file.\n It may not be importable."),
				figmajor, figminor);
    }

    figversion = figmajor*100+figminor;

    if (!skip_comments(figfile)) {
	if (!feof(figfile)) {
	    dia_context_add_message_with_errno(ctx, errno, _("Error reading Fig file."));
	} else {
	    dia_context_add_message(ctx, _("Premature end of Fig file"));
	}
	fclose(figfile);
	return FALSE;
    }

  if (!fig_read_meta_data (figfile, dia, ctx)) {
    fclose (figfile);
    return FALSE;
  }

  compound_stack = NULL;

  do {
    if (!skip_comments (figfile)) {
      if (!feof (figfile)) {
        dia_context_add_message_with_errno (ctx,
                                            errno,
                                            _("Error reading Fig file."));
      } else {
        break;
      }
    }

    if (!fig_read_object (figfile, ctx)) {
      fclose (figfile);
      break;
    }
  } while (TRUE);

  /* Now we can reorder for the depth fields */
  for (i = 0; i < FIG_MAX_DEPTHS; i++) {
    if (depths[i] != NULL) {
      dia_layer_add_objects_first (dia_diagram_data_get_active_layer (dia),
                                   depths[i]);
    }
  }

  return TRUE;
}

/* interface from filter.h */

static const gchar *extensions[] = {"fig", NULL };
DiaImportFilter xfig_import_filter = {
    N_("Xfig File Format"),
    extensions,
    import_fig
};
