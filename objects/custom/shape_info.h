#ifndef _SHAPE_INFO_H_
#define _SHAPE_INFO_H_

#include <glib.h>
#include "geometry.h"
#include "dia_xml.h"
#include "object.h"

typedef enum {
  GE_LINE,
  GE_POLYLINE,
  GE_POLYGON,
  GE_RECT,
  GE_ELLIPSE,
  /* GE_PATH */
} GraphicElementType;

typedef union _GraphicElement GraphicElement;
typedef struct _GraphicElementAny GraphicElementAny;
typedef struct _GraphicElementLine GraphicElementLine;
typedef struct _GraphicElementPoly GraphicElementPoly;
typedef struct _GraphicElementRect GraphicElementRect;
typedef struct _GraphicElementEllipse GraphicElementEllipse;

#define SHAPE_INFO_COMMON  \
  GraphicElementType type; \
  real line_width;         \
  gboolean swap_stroke;    \
  gboolean swap_fill

struct _GraphicElementAny {
  SHAPE_INFO_COMMON;
};

struct _GraphicElementLine {
  SHAPE_INFO_COMMON;
  Point p1, p2;
};

struct _GraphicElementPoly {
  SHAPE_INFO_COMMON;
  int npoints;
  Point points[1];
};

struct _GraphicElementRect {
  SHAPE_INFO_COMMON;
  Point corner1, corner2;
};

struct _GraphicElementEllipse {
  SHAPE_INFO_COMMON;
  Point center;
  real width, height;
};
#undef SHAPE_INFO_COMMON

union _GraphicElement {
  GraphicElementType type;
  GraphicElementAny any;
  GraphicElementLine line;
  GraphicElementPoly polyline;
  GraphicElementPoly polygon;
  GraphicElementRect rect;
  GraphicElementEllipse ellipse;
};

typedef enum {
  SHAPE_ASPECT_FREE,
  SHAPE_ASPECT_FIXED,
  SHAPE_ASPECT_RANGE
} ShapeAspectType;

typedef struct _ShapeInfo ShapeInfo;
struct _ShapeInfo {
  gchar *name;
  gchar *description;
  int nconnections;
  Point *connections;
  Rectangle shape_bounds;
  gboolean has_text;
  Rectangle text_bounds;

  ShapeAspectType aspect_type;
  real aspect_min, aspect_max;

  GList *display_list;

  ObjectType *object_type; /* back link so we can find the correct type */
};

ShapeInfo *shape_info_load(const gchar *filename);
ShapeInfo *shape_info_get(ObjectNode obj_node);
void shape_info_print(ShapeInfo *info);


#endif

