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
#ifndef _SHAPE_INFO_H_
#define _SHAPE_INFO_H_

#include <glib.h>

#include "geometry.h"
#include "dia_xml.h"
#include "object.h"
#include "text.h"
#include "intl.h"
#include "dia_svg.h"

typedef enum {
  GE_LINE,
  GE_POLYLINE,
  GE_POLYGON,
  GE_RECT,
  GE_ELLIPSE,
  GE_PATH,
  GE_SHAPE,
  GE_TEXT
} GraphicElementType;

typedef union _GraphicElement GraphicElement;
typedef struct _GraphicElementAny GraphicElementAny;
typedef struct _GraphicElementLine GraphicElementLine;
typedef struct _GraphicElementPoly GraphicElementPoly;
typedef struct _GraphicElementRect GraphicElementRect;
typedef struct _GraphicElementEllipse GraphicElementEllipse;
typedef struct _GraphicElementPath GraphicElementPath;
typedef struct _GraphicElementText GraphicElementText;

#define SHAPE_INFO_COMMON  \
  GraphicElementType type; \
  DiaSvgGraphicStyle s

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

struct _GraphicElementPath {
  SHAPE_INFO_COMMON;
  int npoints;
  BezPoint points[1];
};

struct _GraphicElementText {
  SHAPE_INFO_COMMON;
  Point anchor;
  char *string;
  Text *object;
  Rectangle text_bounds;
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
  GraphicElementPath path;
  GraphicElementPath shape;
  GraphicElementText text;
};

typedef enum {
  SHAPE_ASPECT_FREE,
  SHAPE_ASPECT_FIXED,
  SHAPE_ASPECT_RANGE
} ShapeAspectType;

typedef struct _ShapeInfo ShapeInfo;
struct _ShapeInfo {
  gchar *name;
#if 0
  gchar *description;
#endif
  gchar *icon;

  int nconnections;
  Point *connections;
  Rectangle shape_bounds;
  gboolean has_text;
  gboolean resize_with_text;
  gint text_align;
  Rectangle text_bounds;

  ShapeAspectType aspect_type;
  real aspect_min, aspect_max;

  GList *display_list;

  ObjectType *object_type; /* back link so we can find the correct type */
};

/* there is no destructor for ShapeInfo at the moment */
ShapeInfo *shape_info_load(const gchar *filename);
ShapeInfo *shape_info_get(ObjectNode obj_node);
ShapeInfo *shape_info_getbyname(const gchar *name);

void shape_info_realise(ShapeInfo* info);
void shape_info_print(ShapeInfo *info);
void parse_path(ShapeInfo *info, const char *path_str, DiaSvgGraphicStyle *s);

#endif

