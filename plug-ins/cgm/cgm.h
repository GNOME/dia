/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * cgm.h -- Named constants for Dia CGM plug-in
 * Copyright (C) 2014 Hans Breuer <hans@breuer.org>
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
 
/*!
 * Including parts of CGM v3.0 specification via:
 * http://en.wikipedia.org/wiki/Computer_Graphics_Metafile#cite_note-2
 */

typedef enum {
  CGM_DELIM   = 0, /*!< Metafile Delimiter Elements */
  CGM_DESC    = 1, /*!< Metafile Descriptor Elements */
  CGM_PICDESC = 2, /*!< Metafile Picture Descriptor Elements */
  CGM_ELEMENT = 4, /*!< Metafile Graphical Primitives ... */
  CGM_ATTRIB  = 5  /*!< ... with Associated Attributes */
} CgmElementClass;

/* CGM_DELIM: Metafile Delimiter Elements */
typedef enum {
  CG_NOOP = 0,
  CGM_BEGIN_METAFILE = 1, /*!< has 1 parameter: P1: (string fixed) metafile name */
  CGM_END_METAFILE = 2,
  CGM_BEGIN_PICTURE = 3, /*!< has 1 parameter: P1: (string fixed) picture name */
  CGM_BEGIN_PICTURE_BODY = 4,
  CGM_END_PICTURE = 5,
  CGM_BEGIN_SEGMENT = 6, /*!< has 1 parameter: P1: (name) segment identifier */
  CGM_END_SEGMENT = 7,
  CGM_BEGIN_FIGURE = 8, /*!< CGM v3?  */
  CGM_END_FIGURE = 9 /*!< CGM v3?  */
} CgmDelimElementId;
/* CGM_DESC: Metafile Descriptor Elements */
typedef enum {
  CGM_METAFILE_VERSION = 1, /*!< valid values are 1, 2, 3, 4 */
  CGM_METAFILE_DESCRIPTION = 2,
  CGM_VDC_TYPE = 3, /*<! 0,1 */
  CGM_INTEGER_PRECISION = 4, /*!< 8,16,24,32 */
  CGM_REAL_PRECISION = 5, /*!< {0,1}, {9,12,16,32}, {23,52,16,32} - 32/64 bit, floating/fixed point */
  CGM_COLOUR_PRECISION = 7, /*!< 8,16,24,32 */
  CGM_METAFILE_ELEMENT_LIST = 11,
  CGM_FONT_LIST = 13
} CgmDescElementId;
/* CGM_PICDESC: Metafile Picture Descriptor Elements */
typedef enum {
  CGM_COLOR_SELECTION_MODE = 2,
  CGM_LINE_WIDTH_SPECIFICATION_MODE = 3,
  CGM_EDGE_WIDTH_SPECIFICATION_MODE = 5,
  CGM_VDC_EXTENT = 6,
  CGM_BACKGROUND_COLOR = 7 
} CgmPicdescElementId;
/* CGM_ELEMENT: Metafile Graphical Element */
typedef enum {
  CGM_POLYLINE = 1,
  CGM_DISJOINT_POLYLINE = 2,
  CGM_POLYMARKER = 3,
  CGM_TEXT = 4,
  CGM_RESTRICTED_TEXT = 5,
  CGM_APPEND_TEXT = 6,
  CGM_POLYGON = 7,
  CGM_POLYGON_SET = 8,
  CGM_CELL_ARRAY = 9, /*!< aka. bitmap */
  CGM_RECTANGLE = 11,
  CGM_CIRCLE = 12,
  CGM_CIRCULAR_ARC_CENTER = 15,
  CGM_CIRCULAR_ARC_CENTER_CLOSE = 16,
  CGM_ELLIPSE = 17,
  CGM_ELLIPTICAL_ARC = 18,
  CGM_ELLIPTICAL_ARC_CLOSE = 19,
  CGM_NURBS = 25, /*!< CGM 3.0: NON-UNIFORM RATIONAL B-SPLINE */
  CGM_POLYBEZIER = 26 /*!< CGM 3.0 */
} CgmElementId;

/* CGM_ATTRIB: Metafile Graphical ... Associated Attributes */
enum CgmAttribElementId {
  CGM_LINE_TYPE = 2, /*!< v1: (1=solid or 2=dashed) v3?: adds (3=dot 4=dash-dot 5=dash-dot-dot) */
  CGM_LINE_WIDTH = 3,
  CGM_LINE_COLOR = 4,
  CGM_TEXT_FONT_INDEX = 10,
  CGM_TEXT_COLOR = 14,
  CGM_CHARACTER_HEIGHT = 15,
  CGM_CHARACTER_ORIENTATION = 16,
  CGM_TEXT_ALIGNMENT = 18,
  /* CGM_ATTRIB: Filled-Area ... Attributes */
  CGM_INTERIOR_STYLE = 22, /*!< (1= solid or 4 = empty) */
  CGM_FILL_COLOR = 23,
  CGM_EDGE_TYPE = 27, /*!< (1=solid or 2=dashed) */
  CGM_EDGE_WIDTH = 28,
  CGM_EDGE_COLOR = 29,
  CGM_EDGE_VISIBILITY = 30, /*!< (1 = on) */
  CGM_LINE_CAP = 37,
  CGM_LINE_JOIN = 38,
  CGM_EDGE_CAP = 44,
  CGM_EDGE_JOIN = 45,
};
