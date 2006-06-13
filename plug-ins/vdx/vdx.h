/* -*- Mode: C; c-basic-offset: 4 -*- */
/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * vdx.h: Visio XML import and export filter for dia
 * Copyright (C) 2006 Ian Redfern
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
    gboolean ok;             /* Flag for whether to stop processing */
    gboolean stop;           /* Flag for whether to stop processing */
    unsigned int Page;          /* Page number */
};

typedef struct VDXDocument VDXDocument;

/* Various conversion ratios */

static const double vdx_Font_Size_Conversion = 4; /* Empirical */
static const double vdx_Y_Offset = 24.0; /* in cm */
static const double vdx_Y_Flip = -1.0; /* Upside down */
static const double vdx_Point_Scale = 2.54; /* Visio is in inches, Dia in cm */
static const double vdx_Page_Width = 17.0; /* in cm */
static const double vdx_Arrow_Scale = 0.05; /* Empirical */
static const double vdx_Dash_Length = 0.17; /* Empirical */

Color
vdx_parse_color(const char *s, const VDXDocument *theDoc);

void *
vdx_read_object(xmlNodePtr cur, VDXDocument *theDoc, void *p);


#endif
