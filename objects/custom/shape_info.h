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
#include "render.h"
#include "dia_xml.h"
#include "object.h"

typedef enum {
  GE_LINE,
  GE_POLYLINE,
  GE_POLYGON,
  GE_RECT,
  GE_ELLIPSE,
  GE_PATH,
  GE_SHAPE
} GraphicElementType;

typedef union _GraphicElement GraphicElement;
typedef struct _GraphicStyle GraphicStyle;
typedef struct _GraphicElementAny GraphicElementAny;
typedef struct _GraphicElementLine GraphicElementLine;
typedef struct _GraphicElementPoly GraphicElementPoly;
typedef struct _GraphicElementRect GraphicElementRect;
typedef struct _GraphicElementEllipse GraphicElementEllipse;
typedef struct _GraphicElementPath GraphicElementPath;

/* special colours */
#define COLOUR_NONE -1
#define COLOUR_FOREGROUND -2
#define COLOUR_BACKGROUND -3
#define COLOUR_TEXT -4

/* these should be changed if they ever cause a conflict */
#define LINECAPS_DEFAULT 20
#define LINEJOIN_DEFAULT 20
#define LINESTYLE_DEFAULT 20

struct _GraphicStyle {
  real line_width;
  gint32 stroke;
  gint32 fill;

  LineCaps linecap;
  LineJoin linejoin;
  LineStyle linestyle;
  real dashlength;
};

#define SHAPE_INFO_COMMON  \
  GraphicElementType type; \
  GraphicStyle s

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
  gchar *icon;

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

