#ifndef _SHAPE_INFO_H_
#define _SHAPE_INFO_H_

#include <glib.h>
#include "geometry.h"
#include "dia_xml.h"

typedef enum {
  GE_LINE,
  GE_POLYLINE,
  GE_POLYGON,
  GE_RECT,
  GE_ELLIPSE,
  /* GE_PATH */
} GraphicElementType;
typedef struct _GraphicElement GraphicElement;
struct _GraphicElement {
  GraphicElementType type;
  union {
    struct {
      Point p1, p2;
    } line;
    struct {
      int npoints;
      Point points[1];
    } polyline;
    struct {
      int npoints;
      Point points[1];
    } polygon;
    struct {
      Point corner1, corner2;
    } rect;
    struct {
      Point center;
      real width, height;
    } ellipse;
  } d;
};

typedef struct _ShapeInfo ShapeInfo;
struct _ShapeInfo {
  gchar *name;
  gchar *description;
  int nconnections;
  Point *connections;
  Rectangle shape_bounds;
  gboolean has_text;
  Rectangle text_bounds;
  GList *display_list;
};

ShapeInfo *shape_info_load(const gchar *filename);
ShapeInfo *shape_info_get(ObjectNode obj_node);
void shape_info_print(ShapeInfo *info);


#endif

