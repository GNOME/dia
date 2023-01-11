/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998, 1999 Alexander Larsson
 *
 * Custom Objects -- objects defined in XML rather than C.
 * Copyright (C) 1999 James Henstridge.
 *
 * Non-uniform scaling/subshape support by Marcel Toele.
 * Modifications (C) 2007 Kern Automatiseringsdiensten BV.
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

#pragma once

#include <glib.h>

#include "geometry.h"
#include "dia_xml.h"
#include "object.h"
#include "text.h"
#include "properties.h"
#include "dia_svg.h"

G_BEGIN_DECLS

typedef enum {
  GE_LINE,
  GE_POLYLINE,
  GE_POLYGON,
  GE_RECT,
  GE_ELLIPSE,
  GE_PATH,
  GE_SHAPE,
  GE_TEXT,
  GE_IMAGE,
  GE_SUBSHAPE
} GraphicElementType;

typedef union _GraphicElement GraphicElement;
typedef struct _GraphicElementAny GraphicElementAny;
typedef struct _GraphicElementLine GraphicElementLine;
typedef struct _GraphicElementPoly GraphicElementPoly;
typedef struct _GraphicElementRect GraphicElementRect;
typedef struct _GraphicElementEllipse GraphicElementEllipse;
typedef struct _GraphicElementPath GraphicElementPath;
typedef struct _GraphicElementText GraphicElementText;
typedef struct _GraphicElementImage GraphicElementImage;
typedef struct _GraphicElementSubShape GraphicElementSubShape;

#define SHAPE_INFO_COMMON  \
  GraphicElementType type; \
  DiaSvgStyle s

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
  real corner_radius;
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
  DiaRectangle text_bounds;
};

struct _GraphicElementImage {
  SHAPE_INFO_COMMON;
  Point topleft;
  real width, height;
  DiaImage *image;
};

#define OFFSET_METHOD_PROPORTIONAL 0
#define OFFSET_METHOD_FIXED   1
#define SUBSCALE_ACCELERATION 1
#define SUBSCALE_MININUM_SCALE 0.0001
struct _GraphicElementSubShape {
  SHAPE_INFO_COMMON;
  GList *display_list;

  gint h_anchor_method;
  gint v_anchor_method;

  real default_scale;

  /* subshape bounding box, center, ... */

  Point center;
  real half_width;
  real half_height;
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
  GraphicElementImage image;
  GraphicElementSubShape subshape;
};

typedef struct _ExtAttribute {
  char *name;
  PropertyType type;
  gpointer extra_data;
} ExtAttribute;

typedef enum {
  SHAPE_ASPECT_FREE,
  SHAPE_ASPECT_FIXED,
  SHAPE_ASPECT_RANGE
} ShapeAspectType;

#define DEFAULT_WIDTH 2.0
#define DEFAULT_HEIGHT 2.0
#define DEFAULT_BORDER 0.25

typedef struct _ShapeInfo ShapeInfo;
/*!
 * \brief Type information for a DiaObject created from shape file
 *
 * \ingroup ObjectCustom
 */
struct _ShapeInfo {
  /*! The objects type name */
  gchar *name;
  /*! The icon file to use */
  gchar *icon;

  /*! the filename is info required to load the real data on demand */
  gchar *filename;
  gboolean loaded;

  /*! everything below could be put into it's own struct to also spare memory when the shapes are not created */
  /* @{ */
  int nconnections;
  Point *connections;
  int main_cp; /*!< the cp that gets connections from the whole object */
  int object_flags; /*!< set of PropFlags e.g. parenting */
  DiaRectangle shape_bounds;
  gboolean has_text;
  gboolean resize_with_text;
  gint text_align;
  DiaRectangle text_bounds;

  ShapeAspectType aspect_type;
  real aspect_min, aspect_max;

  real default_width; /*!< unit cm as everything else internally in Dia */
  real default_height;


  GList *display_list;

  GList *subshapes;

  DiaObjectType *object_type; /* back link so we can find the correct type */

  /*MC 11/03 added */
  int n_ext_attr;
  int ext_attr_size;

  PropDescription *props;
  PropOffset *prop_offsets;
  /* @} */
};

/* there is no destructor for ShapeInfo at the moment */
ShapeInfo *shape_info_load(const gchar *filename);
ShapeInfo *shape_info_get(ObjectNode obj_node);
ShapeInfo *shape_info_getbyname(const gchar *name);

real shape_info_get_default_width(ShapeInfo *info);
real shape_info_get_default_height(ShapeInfo *info);

void shape_info_realise(ShapeInfo* info);
void shape_info_print(ShapeInfo *info);

void shape_info_register (ShapeInfo *);
gboolean shape_typeinfo_load (ShapeInfo* info);

static inline gpointer G_GNUC_MALLOC
dia_new_with_extra (size_t n_main_bytes, size_t n_extra, size_t n_extra_bytes)
{
  if (G_LIKELY (n_extra_bytes == 0 || n_extra <= G_MAXSIZE / n_extra_bytes)) {
    size_t points_size = n_extra * n_extra_bytes;
    if (G_LIKELY (G_MAXSIZE - points_size >= n_main_bytes)) {
      return g_malloc0 (points_size + n_main_bytes);
    }
  }

  g_error ("%s: overflow allocating %"G_GSIZE_FORMAT"+(%"G_GSIZE_FORMAT"*%"G_GSIZE_FORMAT") bytes",
           G_STRLOC, n_main_bytes, n_extra, n_extra_bytes);

  return NULL;
}

G_END_DECLS
