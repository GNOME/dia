#include <stdlib.h>
#include <tree.h>
#include <parser.h>
#include <float.h>
#include <ctype.h>
#include "shape_info.h"

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
parse_svg_node(ShapeInfo *info, xmlNodePtr node, xmlNsPtr svg_ns)
{
  CHAR *str;

  /* walk SVG node ... */
  for (node = node->childs; node != NULL; node = node->next) {
    GraphicElement *el = NULL;
    if (node->type != XML_ELEMENT_NODE || node->ns != svg_ns)
      continue;
    if (!strcmp(node->name, "line")) {
      el = g_malloc0(sizeof(GraphicElementType) + 2*sizeof(Point));
      el->type = GE_LINE;
      str = xmlGetProp(node, "x1");
      if (str) {
	el->d.line.p1.x = g_strtod(str, NULL);
	free(str);
      }
      str = xmlGetProp(node, "y1");
      if (str) {
	el->d.line.p1.y = g_strtod(str, NULL);
	free(str);
      }
      str = xmlGetProp(node, "x2");
      if (str) {
	el->d.line.p2.x = g_strtod(str, NULL);
	free(str);
      }
      str = xmlGetProp(node, "y2");
      if (str) {
	el->d.line.p2.y = g_strtod(str, NULL);
	free(str);
      }
    } else if (!strcmp(node->name, "polyline")) {
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
      el = g_malloc0(sizeof(GraphicElementType) + sizeof(int) +
		     arr->len/2 * sizeof(Point));
      el->type = GE_POLYLINE;
      el->d.polyline.npoints = arr->len / 2;
      rarr = (real *)arr->data;
      for (i = 0; i < el->d.polyline.npoints; i++) {
	el->d.polyline.points[i].x = rarr[2*i];
	el->d.polyline.points[i].y = rarr[2*i+1];
      }
      g_array_free(arr, TRUE);
    } else if (!strcmp(node->name, "polygon")) {
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
      el = g_malloc0(sizeof(GraphicElementType) + sizeof(int) +
		     arr->len/2 * sizeof(Point));
      el->type = GE_POLYGON;
      el->d.polygon.npoints = arr->len / 2;
      rarr = (real *)arr->data;
      for (i = 0; i < el->d.polygon.npoints; i++) {
	el->d.polygon.points[i].x = rarr[2*i];
	el->d.polygon.points[i].y = rarr[2*i+1];
      }
      g_array_free(arr, TRUE);
    } else if (!strcmp(node->name, "rect")) {
      el = g_malloc0(sizeof(GraphicElementType) + 2*sizeof(Point));
      el->type = GE_RECT;
      str = xmlGetProp(node, "x1");
      if (str) {
	el->d.rect.corner1.x = g_strtod(str, NULL);
	free(str);
      }
      str = xmlGetProp(node, "y1");
      if (str) {
	el->d.rect.corner1.y = g_strtod(str, NULL);
	free(str);
      }
      str = xmlGetProp(node, "x2");
      if (str) {
	el->d.rect.corner2.x = g_strtod(str, NULL);
	free(str);
      }
      str = xmlGetProp(node, "y2");
      if (str) {
	el->d.rect.corner2.y = g_strtod(str, NULL);
	free(str);
      }
    } else if (!strcmp(node->name, "circle")) {
      el = g_malloc0(sizeof(GraphicElementType) +
		     sizeof(Point) + 2 * sizeof(real));
      el->type = GE_ELLIPSE;
      str = xmlGetProp(node, "cx");
      if (str) {
	el->d.ellipse.center.x = g_strtod(str, NULL);
	free(str);
      }
      str = xmlGetProp(node, "cy");
      if (str) {
	el->d.ellipse.center.y = g_strtod(str, NULL);
	free(str);
      }
      str = xmlGetProp(node, "r");
      if (str) {
	el->d.ellipse.width = el->d.ellipse.height = 2 * g_strtod(str, NULL);
	free(str);
      }
    } else if (!strcmp(node->name, "ellipse")) {
      el = g_malloc0(sizeof(GraphicElementType) +
		     sizeof(Point) + 2 * sizeof(real));
      el->type = GE_ELLIPSE;
      str = xmlGetProp(node, "cx");
      if (str) {
	el->d.ellipse.center.x = g_strtod(str, NULL);
	free(str);
      }
      str = xmlGetProp(node, "cy");
      if (str) {
	el->d.ellipse.center.y = g_strtod(str, NULL);
	free(str);
      }
      str = xmlGetProp(node, "rx");
      if (str) {
	el->d.ellipse.width = 2 * g_strtod(str, NULL);
	free(str);
      }
      str = xmlGetProp(node, "ry");
      if (str) {
	el->d.ellipse.height = 2 * g_strtod(str, NULL);
	free(str);
      }
    } else if (!strcmp(node->name, "path")) {
    } else if (!strcmp(node->name, "g")) {
      /* add elements from the group element */
      parse_svg_node(info, node, svg_ns);
    }
    if (el)
      info->display_list = g_list_append(info->display_list, el);
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
      check_point(info, &(el->d.line.p1));
      check_point(info, &(el->d.line.p2));
      break;
    case GE_POLYLINE:
      for (i = 0; i < el->d.polyline.npoints; i++)
	check_point(info, &(el->d.polyline.points[i]));
      break;
    case GE_POLYGON:
      for (i = 0; i < el->d.polygon.npoints; i++)
	check_point(info, &(el->d.polygon.points[i]));
      break;
    case GE_RECT:
      check_point(info, &(el->d.rect.corner1));
      check_point(info, &(el->d.rect.corner2));
      break;
    case GE_ELLIPSE:
      pt = el->d.ellipse.center;
      pt.x -= el->d.ellipse.width / 2.0;
      pt.y -= el->d.ellipse.height / 2.0;
      check_point(info, &pt);
      pt.x += el->d.ellipse.width;
      pt.y += el->d.ellipse.height;
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
  
  if (!doc) {
    g_warning("parse error for %s", filename);
    return NULL;
  }
  if (!(shape_ns = xmlSearchNsByHref(doc, doc->root,
		"http://i.need.to.think.of.a.namespace.name"))) {
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
      tmp = xmlNodeGetContent(node);
      g_free(info->description);
      info->description = g_strdup(tmp);
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
    } else if (node->ns == svg_ns && !strcmp(node->name, "svg")) {
      parse_svg_node(info, node, svg_ns);
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
  g_print("Display list:\n");
  for (tmp = info->display_list; tmp; tmp = tmp->next) {
    GraphicElement *el = tmp->data;

    switch (el->type) {
    case GE_LINE:
      g_print("  line: (%g, %g) (%g, %g)\n", el->d.line.p1.x, el->d.line.p1.y,
	      el->d.line.p2.x, el->d.line.p2.y);
      break;
    case GE_RECT:
      g_print("  rect: (%g, %g) (%g, %g)\n",
	      el->d.rect.corner1.x, el->d.rect.corner1.y,
	      el->d.rect.corner2.x, el->d.rect.corner2.y);
      break;
    default:
      break;
    }
  }
  g_print("\n");
}
