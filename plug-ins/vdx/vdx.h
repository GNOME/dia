/* -*- Mode: C; c-basic-offset: 4 -*- */
/* Dia -- a diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * vdx.h: Visio XML import and export filter for dia
 * Copyright (C) 2006-2007 Ian Redfern
 * based on the xfig filter code
 * Copyright (C) 2001 Lars Clausen
 * based on the dxf filter code
 * Copyright (C) 2000 James Henstridge, Steffen Macke
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

#ifndef VISIO_H
#define VISIO_H

/* This structure holds all the internal state that is referred to from
   elsewhere in the XML */

struct VDXDocument
{
    GArray *Colors;
    GArray *FaceNames;
    GArray *Fonts;
    GArray *Masters;
    GArray *StyleSheets;
    GArray *LayerNames;         /* Dia's multi-page list of layers */
    GArray *PageLayers;         /* Layers on this page */
    gboolean ok;             /* Flag for whether to stop processing */
    gboolean stop;           /* Flag for whether to stop processing */
    unsigned int Page;          /* Page number */
    gboolean debug_comments;        /* Flag for g_debug() output */
    unsigned int *debug_shape_ids;  /* List to colour in */
    unsigned int shape_id;          /* For debugging */
};

typedef struct VDXDocument VDXDocument;

/* Various conversion ratios */

static const double vdx_Font_Size_Conversion = 72/25.4*1.14; /* Empirical */
static const double vdx_Y_Offset = 24.0; /* in cm */
static const double vdx_Y_Flip = -1.0; /* Upside down */
static const double vdx_Point_Scale = 2.54; /* Visio is in inches, Dia in cm */
static const double vdx_Line_Scale = 2.54; /* Visio is in inches, Dia in cm */
static const double vdx_Page_Width = 35.0; /* in cm */
static const double vdx_Arrow_Scale = 0.13; /* Empirical */
static const double vdx_Dash_Length = 0.17; /* Empirical */
static const double EPSILON = 0.0001; /* Sensitivity */
static const double vdx_Arrow_Sizes[] = 
        { 0.75, 1.0, 1.4, 1.6, 1.8, 2.0 }; /* Empirical */
static const double vdx_Arrow_Width_Height_Ratio = 0.7; /* Empirical */
static const ArrowType vdx_Arrows[] = { ARROW_NONE, 
                                  ARROW_LINES, ARROW_FILLED_TRIANGLE, 
                                  ARROW_FILLED_TRIANGLE, ARROW_FILLED_TRIANGLE,
                                  ARROW_FILLED_CONCAVE, ARROW_FILLED_TRIANGLE, 
                                  ARROW_FILLED_TRIANGLE, ARROW_FILLED_TRIANGLE, 
                                  ARROW_SLASH_ARROW, ARROW_FILLED_ELLIPSE,
                                  ARROW_FILLED_DIAMOND, ARROW_FILLED_TRIANGLE, 
                                  ARROW_FILLED_TRIANGLE, ARROW_HOLLOW_TRIANGLE, 
                                  ARROW_HOLLOW_TRIANGLE, ARROW_FILLED_TRIANGLE 
                                };

#define VDX_NAMEU_LEN 30
#define DEG_TO_RAD M_PI/180.0                  /* Degrees to radians */

Color
vdx_parse_color(const char *s, const VDXDocument *theDoc, DiaContext *ctx);

const char *
vdx_string_color(const Color c);

void *
vdx_read_object(xmlNodePtr cur, VDXDocument *theDoc, void *p, DiaContext *ctx);

void
vdx_write_object(FILE *file, unsigned int depth, const void *p);

const char *
vdx_convert_xml_string(const char *s);

#endif
