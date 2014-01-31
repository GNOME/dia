/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
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
#ifndef ARROWS_H
#define ARROWS_H

#include "diatypes.h"
#include "geometry.h"
#include "color.h"
#include "dia_xml.h"
#include "diacontext.h"

/* NOTE: Add new arrow types at the end, or the enums
   will change order leading to file incompatibilities. */
/*!
 * \defgroup ObjectArrows Arrows
 * \ingroup ObjectParts
 * \brief A set of standard arrows to be used at line ends
 */

/*!
 * \brief Enumeration of arrow kinds
 * \ingroup ObjectArrows
 * Comments in curly braces mention ISO 10303-AP201 names 
 */
typedef enum {
  ARROW_NONE = 0,          /*!< No arrow */
  ARROW_LINES,             /*!< {open arrow} */
  ARROW_HOLLOW_TRIANGLE,   /*!< {blanked arrow} */
  ARROW_FILLED_TRIANGLE,   /*!< {filled arrow} */
  ARROW_HOLLOW_DIAMOND,    /*!< outline of a diamond */
  ARROW_FILLED_DIAMOND,    /*!< filled diamond, no outline */
  ARROW_HALF_HEAD,
  ARROW_SLASHED_CROSS,     /*!< Vertical + diagonal line */
  ARROW_FILLED_ELLIPSE,
  ARROW_HOLLOW_ELLIPSE,
  ARROW_DOUBLE_HOLLOW_TRIANGLE,
  ARROW_DOUBLE_FILLED_TRIANGLE,
  ARROW_UNFILLED_TRIANGLE,       /*!< {unfilled arrow} */
  ARROW_FILLED_DOT,              /*!< {filled dot} Ellipse + vertical line */ 
  ARROW_DIMENSION_ORIGIN,        /*!< {dimension origin} Ellipse + vert line */ 
  ARROW_BLANKED_DOT,             /*!< {blanked dot} Empty ellipse + vert line */
  ARROW_FILLED_BOX,              /* {filled box} Box + vertical line */
  ARROW_BLANKED_BOX,             /* {blanked box} Box + vertical line */
  ARROW_SLASH_ARROW,             /* {slash arrow} Vertical + diagonal line*/
  ARROW_INTEGRAL_SYMBOL,         /* {integral symbol} Vertical + integral */
  ARROW_CROW_FOOT,
  ARROW_CROSS,                   /* Vertical line */
  ARROW_FILLED_CONCAVE,
  ARROW_BLANKED_CONCAVE,
  ARROW_ROUNDED,
  ARROW_HALF_DIAMOND,		/* ---< */
  ARROW_OPEN_ROUNDED,		/* ---c */
  ARROW_FILLED_DOT_N_TRIANGLE,	/* ---|>o */
  ARROW_ONE_OR_MANY,            /* ER-model: 1 or many*/
  ARROW_NONE_OR_MANY,           /* ER-model: 0 or many*/
  ARROW_ONE_OR_NONE,            /* ER-model: 1 or 0 */
  ARROW_ONE_EXACTLY,            /* ER-model: exactly one*/
  ARROW_BACKSLASH,                  /* -\----  */
  ARROW_THREE_DOTS,

  MAX_ARROW_TYPE /* No arrow heads may be defined beyond here. */
} ArrowType;

/** The number of centimeters long and wide an arrow starts with by default.
 * This can be changed without breaking old diagrams, as the arrow width
 * is stored in there.
 * Note:  Currently, many places have this number hardcoded.
 * find . -name \*.[ch] | xargs grep \\.8
 */
#define DEFAULT_ARROW_SIZE 0.5

/** The minimum width or length of an arrowhead.  This to avoid borderline
 *  cases that break trig functions, as seen in bug #144394
 */
#define MIN_ARROW_DIMENSION 0.001

/*!
 * \brief Definition of line ends
 * \ingroup ObjectArrows
 */
struct _Arrow {
  ArrowType type; /*!< arrow kind */
  real length;    /*!< arrow length */
  real width;     /*!< arrow width */
};


void arrow_draw(DiaRenderer *renderer, ArrowType type,
		Point *to, Point *from,
		real length, real width, real linewidth,
		Color *fg_color, Color *bg_color);

/** following the signature pattern of lib/boundingbox.h 
 * the arrow bounding box is returned in rect
 * \ingroup ObjectArrows
 */
void arrow_bbox (const Arrow *arrow, real line_width, const Point *to, const Point *from, 
                 Rectangle *rect);
/*! Calculate the new line end point in case of an Arrow 
 * \ingroup ObjectArrows
 */
void
calculate_arrow_point(const Arrow *arrow, const Point *to, const Point *from,
		      Point *move_arrow, Point *move_line,
		      real linewidth);

void save_arrow(ObjectNode obj_node, Arrow *arrow, gchar *type_attribute,
		gchar *length_attribute, gchar *width_attribute, DiaContext *ctx);
void load_arrow(ObjectNode obj_node, Arrow *arrow, gchar *type_attribute, 
		gchar *length_attribute, gchar *width_attribute, DiaContext *ctx);

/*! Returns the ArrowType for a given name of an arrow, or 0 if not found.
 * \ingroup ObjectArrows
 */
ArrowType arrow_type_from_name(const gchar *name);
/*! Returns the index in arrow_types of the given arrow type.
 * \ingroup ObjectArrows
 */
gint arrow_index_from_type(ArrowType type);
/*! Convert an arrow index back to the ArrowType
 * \ingroup ObjectArrows
 */
ArrowType arrow_type_from_index(gint index);
const gchar *arrow_get_name_from_type(ArrowType type);
GList *get_arrow_names(void);

#endif /* ARROWS_H */
