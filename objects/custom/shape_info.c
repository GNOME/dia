/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998, 1999 Alexander Larsson
 *
 * Custom Objects -- objects defined in XML rather than C.
 * Copyright (C) 1999 James Henstridge.
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
#include <stdlib.h>
#include <tree.h>
#include <parser.h>
#include <float.h>
#include <ctype.h>
#include <string.h>
#include "shape_info.h"
#include "custom_util.h"
#include "intl.h"

static ShapeInfo *load_shape_info(const gchar *filename);

static GHashTable *name_to_info = NULL;

ShapeInfo *
shape_info_load(const gchar *filename)
{
  ShapeInfo *info = load_shape_info(filename);

  if (!info)
    return NULL;
  if (!name_to_info)
    name_to_info = g_hash_table_new(g_str_hash, g_str_equal);
  g_hash_table_insert(name_to_info, info->name, info);
  return info;
}

ShapeInfo *
shape_info_get(ObjectNode obj_node)
{
  ShapeInfo *info = NULL;
  char *str;

  str = xmlGetProp(obj_node, "type");
  if (str && name_to_info) {
    info = g_hash_table_lookup(name_to_info, str);
    free(str);
  }
  return info;
}

static void
parse_style(xmlNodePtr node, GraphicStyle *s)
{
  char *str, *ptr;

  ptr = str = xmlGetProp(node, "style");
  if (!str)
    return;
  while (ptr[0] != '\0') {
    /* skip white space at start */
    while (ptr[0] != '\0' && isspace(ptr[0])) ptr++;
    if (ptr[0] == '\0') break;

    if (!strncmp("stroke-width:", ptr, 13)) {
      ptr += 13;
      s->line_width = g_strtod(ptr, &ptr);
    } else if (!strncmp("stroke:", ptr, 7)) {
      ptr += 7;
      while (ptr[0] != '\0' && isspace(ptr[0])) ptr++;
      if (ptr[0] == '\0') break;

      if (!strncmp(ptr, "none", 4))
	s->stroke = COLOUR_NONE;
      else if (!strncmp(ptr, "foreground", 10) || !strncmp(ptr, "fg", 2) ||
	       !strncmp(ptr, "default", 7))
	s->stroke = COLOUR_FOREGROUND;
      else if (!strncmp(ptr, "background", 10) || !strncmp(ptr, "bg", 2) ||
	       !strncmp(ptr, "inverse", 7))
	s->stroke = COLOUR_BACKGROUND;
      else if (!strncmp(ptr, "text", 4))
	s->stroke = COLOUR_TEXT;
      else if (ptr[0] == '#')
	s->stroke = strtol(ptr+1, NULL, 16) & 0xffffff;
    } else if (!strncmp("fill:", ptr, 5)) {
      ptr += 5;
      while (ptr[0] != '\0' && isspace(ptr[0])) ptr++;
      if (ptr[0] == '\0') break;

      if (!strncmp(ptr, "none", 4))
	s->fill = COLOUR_NONE;
      else if (!strncmp(ptr, "foreground", 10) || !strncmp(ptr, "fg", 2) ||
	       !strncmp(ptr, "inverse", 7))
	s->fill = COLOUR_FOREGROUND;
      else if (!strncmp(ptr, "background", 10) || !strncmp(ptr, "bg", 2) ||
	       !strncmp(ptr, "default", 7))
	s->fill = COLOUR_BACKGROUND;
      else if (!strncmp(ptr, "text", 4))
	s->fill = COLOUR_TEXT;
      else if (ptr[0] == '#')
	s->fill = strtol(ptr+1, NULL, 16) & 0xffffff;
    } else if (!strncmp("stroke-linecap:", ptr, 15)) {
      ptr += 15;
      while (ptr[0] != '\0' && isspace(ptr[0])) ptr++;
      if (ptr[0] == '\0') break;

      if (!strncmp(ptr, "butt", 4))
	s->linecap = LINECAPS_BUTT;
      else if (!strncmp(ptr, "round", 5))
	s->linecap = LINECAPS_ROUND;
      else if (!strncmp(ptr, "square", 6) || !strncmp(ptr, "projecting", 10))
	s->linecap = LINECAPS_PROJECTING;
      else if (!strncmp(ptr, "default", 7))
	s->linecap = LINECAPS_DEFAULT;
    } else if (!strncmp("stroke-linejoin:", ptr, 16)) {
      ptr += 16;
      while (ptr[0] != '\0' && isspace(ptr[0])) ptr++;
      if (ptr[0] == '\0') break;

      if (!strncmp(ptr, "miter", 5))
	s->linejoin = LINEJOIN_MITER;
      else if (!strncmp(ptr, "round", 5))
	s->linejoin = LINEJOIN_ROUND;
      else if (!strncmp(ptr, "bevel", 5))
	s->linejoin = LINEJOIN_BEVEL;
      else if (!strncmp(ptr, "default", 7))
	s->linejoin = LINEJOIN_DEFAULT;
    } else if (!strncmp("stroke-pattern:", ptr, 15)) {
      ptr += 15;
      while (ptr[0] != '\0' && isspace(ptr[0])) ptr++;
      if (ptr[0] == '\0') break;

      if (!strncmp(ptr, "solid", 5))
	s->linestyle = LINESTYLE_SOLID;
      else if (!strncmp(ptr, "dashed", 6))
	s->linestyle = LINESTYLE_DASHED;
      else if (!strncmp(ptr, "dash-dot", 8))
	s->linestyle = LINESTYLE_DASH_DOT;
      else if (!strncmp(ptr, "dash-dot-dot", 12))
	s->linestyle = LINESTYLE_DASH_DOT_DOT;
      else if (!strncmp(ptr, "dotted", 6))
	s->linestyle = LINESTYLE_DOTTED;
      else if (!strncmp(ptr, "default", 7))
	s->linestyle = LINESTYLE_DEFAULT;
    } else if (!strncmp("stroke-dashlength:", ptr, 18)) {
      ptr += 18;
      while (ptr[0] != '\0' && isspace(ptr[0])) ptr++;
      if (ptr[0] == '\0') break;

      if (!strncmp(ptr, "default", 7))
	s->dashlength = 1.0;
      else
	s->dashlength = g_strtod(ptr, &ptr);
    }

    /* skip up to the next attribute */
    while (ptr[0] != '\0' && ptr[0] != ';' && ptr[0] != '\n') ptr++;
    if (ptr[0] != '\0') ptr++;
  }
  free(str);
}

/* routine to chomp off the start of the string */
#define path_chomp(path) while (path[0]!='\0'&&strchr(" \t\n\r,", path[0])) path++

static void
parse_path(ShapeInfo *info, const char *path_str, GraphicStyle *s)
{
  enum {
    PATH_MOVE, PATH_LINE, PATH_HLINE, PATH_VLINE, PATH_CURVE,
    PATH_SMOOTHCURVE, PATH_CLOSE } last_type = PATH_MOVE;
  Point last_open = {0.0, 0.0};
  Point last_point = {0.0, 0.0};
  Point last_control = {0.0, 0.0};
  gboolean last_relative = FALSE;
  real x1, y1, x2, y2, x3, y3;
  static GArray *points = NULL;
  BezPoint bez;
  gchar *path = (gchar *)path_str;

  if (!points)
    points = g_array_new(FALSE, FALSE, sizeof(BezPoint));
  g_array_set_size(points, 0);

  path_chomp(path);
  while (path[0] != '\0') {
#ifdef DEBUG_CUSTOM
    g_print("Path: %s\n", path);
#endif
    /* check for a new command */
    switch (path[0]) {
    case 'M':
      path++;
      path_chomp(path);
      last_type = PATH_MOVE;
      last_relative = FALSE;
      break;
    case 'm':
      path++;
      path_chomp(path);
      last_type = PATH_MOVE;
      last_relative = TRUE;
      break;
    case 'L':
      path++;
      path_chomp(path);
      last_type = PATH_LINE;
      last_relative = FALSE;
      break;
    case 'l':
      path++;
      path_chomp(path);
      last_type = PATH_LINE;
      last_relative = TRUE;
      break;
    case 'H':
      path++;
      path_chomp(path);
      last_type = PATH_HLINE;
      last_relative = FALSE;
      break;
    case 'h':
      path++;
      path_chomp(path);
      last_type = PATH_HLINE;
      last_relative = TRUE;
      break;
    case 'V':
      path++;
      path_chomp(path);
      last_type = PATH_VLINE;
      last_relative = FALSE;
      break;
    case 'v':
      path++;
      path_chomp(path);
      last_type = PATH_VLINE;
      last_relative = TRUE;
      break;
    case 'C':
      path++;
      path_chomp(path);
      last_type = PATH_CURVE;
      last_relative = FALSE;
      break;
    case 'c':
      path++;
      path_chomp(path);
      last_type = PATH_CURVE;
      last_relative = TRUE;
      break;
    case 'S':
      path++;
      path_chomp(path);
      last_type = PATH_SMOOTHCURVE;
      last_relative = FALSE;
      break;
    case 's':
      path++;
      path_chomp(path);
      last_type = PATH_SMOOTHCURVE;
      last_relative = TRUE;
      break;
    case 'Z':
    case 'z':
      path++;
      path_chomp(path);
      last_type = PATH_CLOSE;
      last_relative = FALSE;
      break;
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
    case '.':
    case '+':
    case '-':
      if (last_type == PATH_CLOSE) {
	g_warning("parse_path: argument given for implicite close path");
	/* consume one number so we don't fall into an infinite loop */
	while (path != '\0' && strchr("0123456789.+-", path[0])) path++;
	path_chomp(path);
      }
      break;
    default:
      g_warning("unsupported path codde '%c'", path[0]);
      path++;
      path_chomp(path);
      break;
    }
    /* actually parse the path component */
    switch (last_type) {
    case PATH_MOVE:
      bez.type = BEZ_MOVE_TO;
      bez.p1.x = g_strtod(path, &path);
      path_chomp(path);
      bez.p1.y = g_strtod(path, &path);
      path_chomp(path);
      if (last_relative) {
	bez.p1.x += last_point.x;
	bez.p1.y += last_point.y;
      }
      last_point = bez.p1;
      last_control = bez.p1;
      last_open = bez.p1;

      /* if there is some unclosed commands, add them as a GE_PATH */
      if (points->len > 0 &&
	  g_array_index(points, BezPoint, 0).type != BEZ_MOVE_TO) {
	GraphicElementPath *el = g_malloc(sizeof(GraphicElementPath) +
					  points->len * sizeof(BezPoint));
	el->type = GE_PATH;
	el->s = *s;
	el->npoints = points->len;
	memcpy((char *)el->points, points->data, points->len*sizeof(BezPoint));
	info->display_list = g_list_append(info->display_list, el);
      }
      g_array_set_size(points, 0);
      g_array_append_val(points, bez);
      break;
    case PATH_LINE:
      bez.type = BEZ_LINE_TO;
      bez.p1.x = g_strtod(path, &path);
      path_chomp(path);
      bez.p1.y = g_strtod(path, &path);
      path_chomp(path);
      if (last_relative) {
	bez.p1.x += last_point.x;
	bez.p1.y += last_point.y;
      }
      last_point = bez.p1;
      last_control = bez.p1;

      g_array_append_val(points, bez);
      break;
    case PATH_HLINE:
      bez.type = BEZ_LINE_TO;
      bez.p1.x = g_strtod(path, &path);
      path_chomp(path);
      bez.p1.y = last_point.y;
      if (last_relative)
	bez.p1.x += last_point.x;
      last_point = bez.p1;
      last_control = bez.p1;

      g_array_append_val(points, bez);
      break;
    case PATH_VLINE:
      bez.type = BEZ_LINE_TO;
      bez.p1.x = last_point.x;
      bez.p1.y = g_strtod(path, &path);
      path_chomp(path);
      if (last_relative)
	bez.p1.y += last_point.y;
      last_point = bez.p1;
      last_control = bez.p1;

      g_array_append_val(points, bez);
      break;
    case PATH_CURVE:
      bez.type = BEZ_CURVE_TO;
      bez.p1.x = g_strtod(path, &path);
      path_chomp(path);
      bez.p1.y = g_strtod(path, &path);
      path_chomp(path);
      bez.p2.x = g_strtod(path, &path);
      path_chomp(path);
      bez.p2.y = g_strtod(path, &path);
      path_chomp(path);
      bez.p3.x = g_strtod(path, &path);
      path_chomp(path);
      bez.p3.y = g_strtod(path, &path);
      path_chomp(path);
      if (last_relative) {
	bez.p1.x += last_point.x;
	bez.p1.y += last_point.y;
	bez.p2.x += last_point.x;
	bez.p2.y += last_point.y;
	bez.p3.x += last_point.x;
	bez.p3.y += last_point.y;
      }
      last_point = bez.p3;
      last_control = bez.p2;

      g_array_append_val(points, bez);
      break;
    case PATH_SMOOTHCURVE:
      bez.type = BEZ_CURVE_TO;
      bez.p1.x = 2 * last_point.x - last_control.x;
      bez.p1.y = 2 * last_point.y - last_control.y;
      bez.p2.x = g_strtod(path, &path);
      path_chomp(path);
      bez.p2.y = g_strtod(path, &path);
      path_chomp(path);
      bez.p3.x = g_strtod(path, &path);
      path_chomp(path);
      bez.p3.y = g_strtod(path, &path);
      path_chomp(path);
      if (last_relative) {
	bez.p2.x += last_point.x;
	bez.p2.y += last_point.y;
	bez.p3.x += last_point.x;
	bez.p3.y += last_point.y;
      }
      last_point = bez.p3;
      last_control = bez.p2;

      g_array_append_val(points, bez);
      break;
    case PATH_CLOSE:
      /* close the path with a line */
      if (last_open.x != last_point.x || last_open.y != last_point.y) {
	bez.type = BEZ_LINE_TO;
	bez.p1 = last_open;
	g_array_append_val(points, bez);
      }
      /* if there is some unclosed commands, add them as a GE_SHAPE */
      if (points->len > 0) {
	GraphicElementPath *el = g_malloc(sizeof(GraphicElementPath) +
					  points->len * sizeof(BezPoint));
	el->type = GE_SHAPE;
	el->s = *s;
	el->npoints = points->len;
	memcpy((char *)el->points, points->data, points->len*sizeof(BezPoint));
	info->display_list = g_list_append(info->display_list, el);
      }
      g_array_set_size(points, 0);
      last_point = last_open;
      last_control = last_open;
      break;
    }
    /* get rid of any ignorable characters */
    path_chomp(path);
  }
  /* if there is some unclosed commands, add them as a GE_PATH */
  if (points->len > 0) {
    GraphicElementPath *el = g_malloc(sizeof(GraphicElementPath) +
				      points->len * sizeof(BezPoint));
    el->type = GE_PATH;
    el->s = *s;
    el->npoints = points->len;
    memcpy((char *)el->points, points->data, points->len*sizeof(BezPoint));
    info->display_list = g_list_append(info->display_list, el);
  }
  g_array_set_size(points, 0);
}

static void
parse_svg_node(ShapeInfo *info, xmlNodePtr node, xmlNsPtr svg_ns,
	       GraphicStyle *style)
{
  CHAR *str;

  /* walk SVG node ... */
  for (node = node->childs; node != NULL; node = node->next) {
    GraphicElement *el = NULL;
    GraphicStyle s;

    if (node->type != XML_ELEMENT_NODE || node->ns != svg_ns)
      continue;
    s = *style;
    parse_style(node, &s);
    if (!strcmp(node->name, "line")) {
      GraphicElementLine *line = g_new(GraphicElementLine, 1);

      el = (GraphicElement *)line;
      line->type = GE_LINE;
      str = xmlGetProp(node, "x1");
      if (str) {
	line->p1.x = g_strtod(str, NULL);
	free(str);
      }
      str = xmlGetProp(node, "y1");
      if (str) {
	line->p1.y = g_strtod(str, NULL);
	free(str);
      }
      str = xmlGetProp(node, "x2");
      if (str) {
	line->p2.x = g_strtod(str, NULL);
	free(str);
      }
      str = xmlGetProp(node, "y2");
      if (str) {
	line->p2.y = g_strtod(str, NULL);
	free(str);
      }
    } else if (!strcmp(node->name, "polyline")) {
      GraphicElementPoly *poly;
      GArray *arr = g_array_new(FALSE, FALSE, sizeof(real));
      real val, *rarr;
      char *tmp;
      int i;

      tmp = str = xmlGetProp(node, "points");
      while (tmp[0] != '\0') {
	/* skip junk */
	while (tmp[0] != '\0' && !isdigit(tmp[0]) && tmp[0]!='.'&&tmp[0]!='-')
	  tmp++;
	if (tmp[0] == '\0') break;
	val = g_strtod(tmp, &tmp);
	g_array_append_val(arr, val);
      }
      free(str);
      val = 0;
      if (arr->len % 2 == 1) 
	g_array_append_val(arr, val);
      poly = g_malloc0(sizeof(GraphicElementPoly) + arr->len/2*sizeof(Point));
      el = (GraphicElement *)poly;
      poly->type = GE_POLYLINE;
      poly->npoints = arr->len / 2;
      rarr = (real *)arr->data;
      for (i = 0; i < poly->npoints; i++) {
	poly->points[i].x = rarr[2*i];
	poly->points[i].y = rarr[2*i+1];
      }
      g_array_free(arr, TRUE);
    } else if (!strcmp(node->name, "polygon")) {
      GraphicElementPoly *poly;
      GArray *arr = g_array_new(FALSE, FALSE, sizeof(real));
      real val, *rarr;
      char *tmp;
      int i;

      tmp = str = xmlGetProp(node, "points");
      while (tmp[0] != '\0') {
	/* skip junk */
	while (tmp[0] != '\0' && !isdigit(tmp[0]) && tmp[0]!='.'&&tmp[0]!='-')
	  tmp++;
	if (tmp[0] == '\0') break;
	val = g_strtod(tmp, &tmp);
	g_array_append_val(arr, val);
      }
      free(str);
      val = 0;
      if (arr->len % 2 == 1) 
	g_array_append_val(arr, val);
      poly = g_malloc0(sizeof(GraphicElementPoly) + arr->len/2*sizeof(Point));
      el = (GraphicElement *)poly;
      poly->type = GE_POLYGON;
      poly->npoints = arr->len / 2;
      rarr = (real *)arr->data;
      for (i = 0; i < poly->npoints; i++) {
	poly->points[i].x = rarr[2*i];
	poly->points[i].y = rarr[2*i+1];
      }
      g_array_free(arr, TRUE);
    } else if (!strcmp(node->name, "rect")) {
      GraphicElementRect *rect = g_new(GraphicElementRect, 1);

      el = (GraphicElement *)rect;
      rect->type = GE_RECT;
      str = xmlGetProp(node, "x1");
      if (str) {
	rect->corner1.x = g_strtod(str, NULL);
	free(str);
      }
      str = xmlGetProp(node, "y1");
      if (str) {
	rect->corner1.y = g_strtod(str, NULL);
	free(str);
      }
      str = xmlGetProp(node, "x2");
      if (str) {
	rect->corner2.x = g_strtod(str, NULL);
	free(str);
      }
      str = xmlGetProp(node, "y2");
      if (str) {
	rect->corner2.y = g_strtod(str, NULL);
	free(str);
      }
    } else if (!strcmp(node->name, "circle")) {
      GraphicElementEllipse *ellipse = g_new(GraphicElementEllipse, 1);

      el = (GraphicElement *)ellipse;
      ellipse->type = GE_ELLIPSE;
      str = xmlGetProp(node, "cx");
      if (str) {
	ellipse->center.x = g_strtod(str, NULL);
	free(str);
      }
      str = xmlGetProp(node, "cy");
      if (str) {
	ellipse->center.y = g_strtod(str, NULL);
	free(str);
      }
      str = xmlGetProp(node, "r");
      if (str) {
	ellipse->width = ellipse->height = 2 * g_strtod(str, NULL);
	free(str);
      }
    } else if (!strcmp(node->name, "ellipse")) {
      GraphicElementEllipse *ellipse = g_new(GraphicElementEllipse, 1);

      el = (GraphicElement *)ellipse;
      ellipse->type = GE_ELLIPSE;
      str = xmlGetProp(node, "cx");
      if (str) {
	ellipse->center.x = g_strtod(str, NULL);
	free(str);
      }
      str = xmlGetProp(node, "cy");
      if (str) {
	ellipse->center.y = g_strtod(str, NULL);
	free(str);
      }
      str = xmlGetProp(node, "rx");
      if (str) {
	ellipse->width = 2 * g_strtod(str, NULL);
	free(str);
      }
      str = xmlGetProp(node, "ry");
      if (str) {
	ellipse->height = 2 * g_strtod(str, NULL);
	free(str);
      }
    } else if (!strcmp(node->name, "path")) {
      str = xmlGetProp(node, "d");
      if (str) {
	parse_path(info, str, &s);
	free(str);
      }
    } else if (!strcmp(node->name, "g")) {
      /* add elements from the group element */
      parse_svg_node(info, node, svg_ns, &s);
    }
    if (el) {
      el->any.s = s;
      info->display_list = g_list_append(info->display_list, el);
    }
  }
}

static void
check_point(ShapeInfo *info, Point *pt)
{
  if (pt->x < info->shape_bounds.left)
    info->shape_bounds.left = pt->x;
  if (pt->x > info->shape_bounds.right)
    info->shape_bounds.right = pt->x;
  if (pt->y < info->shape_bounds.top)
    info->shape_bounds.top = pt->y;
  if (pt->y > info->shape_bounds.bottom)
    info->shape_bounds.bottom = pt->y;
}

static void
update_bounds(ShapeInfo *info)
{
  GList *tmp;
  Point pt;
  for (tmp = info->display_list; tmp; tmp = tmp->next) {
    GraphicElement *el = tmp->data;
    int i;

    switch (el->type) {
    case GE_LINE:
      check_point(info, &(el->line.p1));
      check_point(info, &(el->line.p2));
      break;
    case GE_POLYLINE:
      for (i = 0; i < el->polyline.npoints; i++)
	check_point(info, &(el->polyline.points[i]));
      break;
    case GE_POLYGON:
      for (i = 0; i < el->polygon.npoints; i++)
	check_point(info, &(el->polygon.points[i]));
      break;
    case GE_RECT:
      check_point(info, &(el->rect.corner1));
      check_point(info, &(el->rect.corner2));
      break;
    case GE_ELLIPSE:
      pt = el->ellipse.center;
      pt.x -= el->ellipse.width / 2.0;
      pt.y -= el->ellipse.height / 2.0;
      check_point(info, &pt);
      pt.x += el->ellipse.width;
      pt.y += el->ellipse.height;
      check_point(info, &pt);
      break;
    case GE_PATH:
    case GE_SHAPE:
      for (i = 0; i < el->path.npoints; i++)
	switch (el->path.points[i].type) {
	case BEZ_CURVE_TO:
	  check_point(info, &el->path.points[i].p3);
	  check_point(info, &el->path.points[i].p2);
	case BEZ_MOVE_TO:
	case BEZ_LINE_TO:
	  check_point(info, &el->path.points[i].p1);
	}
      break;
    }
  }
}

static ShapeInfo *
load_shape_info(const gchar *filename)
{
  xmlDocPtr doc = xmlParseFile(filename);
  xmlNsPtr shape_ns, svg_ns;
  xmlNodePtr node;
  ShapeInfo *info;
  char *tmp;
  int descr_score = -1;
  
  if (!doc) {
    g_warning("parse error for %s", filename);
    return NULL;
  }
  if (!(shape_ns = xmlSearchNsByHref(doc, doc->root,
		"http://www.daa.com.au/~james/dia-shape-ns"))) {
    xmlFreeDoc(doc);
    g_warning("could not find shape namespace");
    return NULL;
  }
  if (!(svg_ns = xmlSearchNsByHref(doc, doc->root,
		"http://www.w3.org/Graphics/SVG/svg-19990730.dtd"))) {
    xmlFreeDoc(doc);
    g_warning("could not find svg namespace");
    return NULL;
  }
  if (doc->root->ns != shape_ns || strcmp(doc->root->name, "shape")) {
    g_warning("root element was %s -- expecting shape", doc->root->name);
    xmlFreeDoc(doc);
    return NULL;
  }

  info = g_new0(ShapeInfo, 1);
  info->shape_bounds.top = DBL_MAX;
  info->shape_bounds.left = DBL_MAX;
  info->shape_bounds.bottom = -DBL_MAX;
  info->shape_bounds.right = -DBL_MAX;

  for (node = doc->root->childs; node != NULL; node = node->next) {
    if (node->type != XML_ELEMENT_NODE)
      continue;
    if (node->ns == shape_ns && !strcmp(node->name, "name")) {
      tmp = xmlNodeGetContent(node);
      g_free(info->name);
      info->name = g_strdup(tmp);
      free(tmp);
    } else if (node->ns == shape_ns && !strcmp(node->name, "description")) {
      gint score;

      /* compare the xml:lang property on this element to see if we get a
       * better language match.  LibXML seems to throw away attribute
       * namespaces, so we use "lang" instead of "xml:lang" */
      tmp = xmlGetProp(node, "xml:lang");
      if (!tmp) tmp = xmlGetProp(node, "lang");
      score = intl_score_locale(tmp);
      if (tmp) free(tmp);

      if (descr_score < 0 || score < descr_score) {
	descr_score = score;
	tmp = xmlNodeGetContent(node);
	g_free(info->description);
	info->description = g_strdup(tmp);
	free(tmp);
      }
    } else if (node->ns == shape_ns && !strcmp(node->name, "icon")) {
      tmp = xmlNodeGetContent(node);
      g_free(info->icon);
      info->icon = custom_get_relative_filename(filename, tmp);
      free(tmp);
    } else if (node->ns == shape_ns && !strcmp(node->name, "connections")) {
      GArray *arr = g_array_new(FALSE, FALSE, sizeof(Point));
      xmlNodePtr pt_node;

      for (pt_node = node->childs; pt_node != NULL; pt_node = pt_node->next) {
	if (pt_node->ns == shape_ns && !strcmp(pt_node->name, "point")) {
	  Point pt = { 0.0, 0.0 };
	  CHAR *str;

	  str = xmlGetProp(pt_node, "x");
	  if (str) {
	    pt.x = g_strtod(str, NULL);
	    free(str);
	  }
	  str = xmlGetProp(pt_node, "y");
	  if (str) {
	    pt.y = g_strtod(str, NULL);
	    free(str);
	  }
	  g_array_append_val(arr, pt);
	}
      }
      info->nconnections = arr->len;
      info->connections = (Point *)arr->data;
      g_array_free(arr, FALSE);
    } else if (node->ns == shape_ns && !strcmp(node->name, "textbox")) {
      CHAR *str;
      
      str = xmlGetProp(node, "x1");
      if (str) {
	info->text_bounds.left = g_strtod(str, NULL);
	free(str);
      }
      str = xmlGetProp(node, "y1");
      if (str) {
	info->text_bounds.top = g_strtod(str, NULL);
	free(str);
      }
      str = xmlGetProp(node, "x2");
      if (str) {
	info->text_bounds.right = g_strtod(str, NULL);
	free(str);
      }
      str = xmlGetProp(node, "y2");
      if (str) {
	info->text_bounds.bottom = g_strtod(str, NULL);
	free(str);
      }
      info->has_text = TRUE;
    } else if (node->ns == shape_ns && !strcmp(node->name, "aspectratio")) {
      tmp = xmlGetProp(node, "type");
      if (tmp) {
	if (!strcmp(tmp, "free"))
	  info->aspect_type = SHAPE_ASPECT_FREE;
	else if (!strcmp(tmp, "fixed"))
	  info->aspect_type = SHAPE_ASPECT_FIXED;
	else if (!strcmp(tmp, "range")) {
	  char *str;

	  info->aspect_type = SHAPE_ASPECT_RANGE;
	  info->aspect_min = 0.0;
	  info->aspect_max = G_MAXFLOAT;
	  str = xmlGetProp(node, "min");
	  if (str) {
	    info->aspect_min = g_strtod(str, NULL);
	    free(str);
	  }
	  str = xmlGetProp(node, "max");
	  if (str) {
	    info->aspect_max = g_strtod(str, NULL);
	    free(str);
	  }
	  if (info->aspect_max < info->aspect_min) {
	    real asp = info->aspect_max;
	    info->aspect_max = info->aspect_min;
	    info->aspect_min = asp;
	  }
	}
	free(tmp);
      }
    } else if (node->ns == svg_ns && !strcmp(node->name, "svg")) {
      GraphicStyle s = {
	1.0, COLOUR_FOREGROUND, COLOUR_NONE,
	LINECAPS_DEFAULT, LINEJOIN_DEFAULT, LINESTYLE_DEFAULT, 1.0
      };

      parse_style(node, &s);
      parse_svg_node(info, node, svg_ns, &s);
      update_bounds(info);
    }
  }
  return info;
}

void
shape_info_print(ShapeInfo *info)
{
  GList *tmp;
  int i;
  g_print("Name        : %s\n", info->name);
  g_print("Description : %s\n", info->description);
  g_print("Connections :\n");
  for (i = 0; i < info->nconnections; i++)
    g_print("  (%g, %g)\n", info->connections[i].x, info->connections[i].y);
  g_print("Shape bounds: (%g, %g) - (%g, %g)\n",
	  info->shape_bounds.left, info->shape_bounds.top,
	  info->shape_bounds.right, info->shape_bounds.bottom);
  if (info->has_text)
    g_print("Text bounds : (%g, %g) - (%g, %g)\n",
	    info->text_bounds.left, info->text_bounds.top,
	    info->text_bounds.right, info->text_bounds.bottom);
  g_print("Aspect ratio: ");
  switch (info->aspect_type) {
  case SHAPE_ASPECT_FREE: g_print("free\n"); break;
  case SHAPE_ASPECT_FIXED: g_print("fixed\n"); break;
  case SHAPE_ASPECT_RANGE:
    g_print("range %g - %g\n", info->aspect_min, info->aspect_max); break;
  }
  g_print("Display list:\n");
  for (tmp = info->display_list; tmp; tmp = tmp->next) {
    GraphicElement *el = tmp->data;
    int i;

    switch (el->type) {
    case GE_LINE:
      g_print("  line: (%g, %g) (%g, %g)\n", el->line.p1.x, el->line.p1.y,
	      el->line.p2.x, el->line.p2.y);
      break;
    case GE_POLYLINE:
      g_print("  polyline:");
      for (i = 0; i < el->polyline.npoints/2; i++)
	g_print(" (%g, %g)", el->polyline.points[2*i],
		el->polyline.points[2*i+1]);
      g_print("\n");
      break;
    case GE_POLYGON:
      g_print("  polygon:");
      for (i = 0; i < el->polygon.npoints/2; i++)
	g_print(" (%g, %g)", el->polygon.points[2*i],
		el->polygon.points[2*i+1]);
      g_print("\n");
      break;
    case GE_RECT:
      g_print("  rect: (%g, %g) (%g, %g)\n",
	      el->rect.corner1.x, el->rect.corner1.y,
	      el->rect.corner2.x, el->rect.corner2.y);
      break;
    case GE_ELLIPSE:
      g_print("  ellipse: center=(%g, %g) width=%g height=%g\n",
	      el->ellipse.center.x, el->ellipse.center.y,
	      el->ellipse.width, el->ellipse.height);
      break;
    case GE_PATH:
      g_print("  path:");
      for (i = 0; i < el->path.npoints; i++)
	switch (el->path.points[i].type) {
	case BEZ_MOVE_TO:
	  g_print(" M (%g, %g)", el->path.points[i].p1.x,
		  el->path.points[i].p1.y);
	  break;
	case BEZ_LINE_TO:
	  g_print(" L (%g, %g)", el->path.points[i].p1.x,
		  el->path.points[i].p1.y);
	  break;
	case BEZ_CURVE_TO:
	  g_print(" C (%g, %g) (%g, %g) (%g, %g)", el->path.points[i].p1.x,
		  el->path.points[i].p1.y, el->path.points[i].p2.x,
		  el->path.points[i].p2.y, el->path.points[i].p3.x,
		  el->path.points[i].p3.y);
	  break;
	}
      break;
    case GE_SHAPE:
      g_print("  shape:");
      for (i = 0; i < el->path.npoints; i++)
	switch (el->path.points[i].type) {
	case BEZ_MOVE_TO:
	  g_print(" M (%g, %g)", el->path.points[i].p1.x,
		  el->path.points[i].p1.y);
	  break;
	case BEZ_LINE_TO:
	  g_print(" L (%g, %g)", el->path.points[i].p1.x,
		  el->path.points[i].p1.y);
	  break;
	case BEZ_CURVE_TO:
	  g_print(" C (%g, %g) (%g, %g) (%g, %g)", el->path.points[i].p1.x,
		  el->path.points[i].p1.y, el->path.points[i].p2.x,
		  el->path.points[i].p2.y, el->path.points[i].p3.x,
		  el->path.points[i].p3.y);
	  break;
	}
      break;
    default:
      break;
    }
  }
  g_print("\n");
}
