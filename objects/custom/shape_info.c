#include <stdlib.h>
#include <tree.h>
#include <parser.h>
#include <float.h>
#include <ctype.h>
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
parse_style(xmlNodePtr node, real *line_width, gboolean *swap_stroke,
	    gboolean *swap_fill)
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
      *line_width = g_strtod(ptr, &ptr);
    } else if (!strncmp("stroke:", ptr, 7)) {
      ptr += 7;
      while (ptr[0] != '\0' && isspace(ptr[0])) ptr++;
      if (ptr[0] == '\0') break;
      *swap_stroke = !strncmp("background", ptr, 10) ||
		     !strncmp("fill", ptr, 4);
    } else if (!strncmp("fill:", ptr, 5)) {
      ptr += 5;
      while (ptr[0] != '\0' && isspace(ptr[0])) ptr++;
      if (ptr[0] == '\0') break;
      *swap_fill = !strncmp("foreground", ptr, 10) ||
		   !strncmp("stroke", ptr, 6);
    }

    /* skip up to the next attribute */
    while (ptr[0] != '\0' && ptr[0] != ';' && ptr[0] != '\n') ptr++;
    if (ptr[0] != '\0') ptr++;
  }
  free(str);
}

static void
parse_svg_node(ShapeInfo *info, xmlNodePtr node, xmlNsPtr svg_ns,
	       real line_width, gboolean swap_stroke, gboolean swap_fill)
{
  CHAR *str;

  /* walk SVG node ... */
  for (node = node->childs; node != NULL; node = node->next) {
    GraphicElement *el = NULL;
    real new_width = line_width;
    gboolean new_fswap = swap_stroke, new_bswap = swap_stroke;

    if (node->type != XML_ELEMENT_NODE || node->ns != svg_ns)
      continue;
    parse_style(node, &new_width, &new_fswap, &new_bswap);
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
      poly->type = GE_POLYLINE;
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
    } else if (!strcmp(node->name, "g")) {
      /* add elements from the group element */
      parse_svg_node(info, node, svg_ns, new_width, new_fswap, new_bswap);
    }
    if (el) {
      el->any.line_width = new_width;
      el->any.swap_stroke = new_fswap;
      el->any.swap_fill   = new_bswap;
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
      info->icon = get_relative_filename(filename, tmp);
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
      parse_svg_node(info, node, svg_ns, 1.0, FALSE, FALSE);
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
    default:
      break;
    }
  }
  g_print("\n");
}
