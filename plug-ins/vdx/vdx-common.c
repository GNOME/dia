/* -*- Mode: C; c-basic-offset: 4 -*- */
/* Dia -- a diagram creation/manipulation program
 *
 * vdx-common.c: Visio XML import filter for dia
 * Copyright (C) 2006-2007 Ian Redfern
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

#include <stdio.h>
#include <glib.h>
#include "color.h"

#include "visio-types.h"

char * vdx_Units[] =
{
  "BOOL",
  "CM",
  "COLOR",
  "DA",
  "DATE",
  "DEG",
  "DL",
  "DP",
  "DT",
  "FT",
  "F_I",
  "GUID",
  "IN",
  "IN_F",
  "M",
  "MM",
  "NUM",
  "NURBS",
  "PNT",
  "POLYLINE",
  "PT",
  "STR",
  NULL
};
char * vdx_Types[] =
{
  "any",
  "Act",
  "Align",
  "ArcTo",
  "Char",
  "ColorEntry",
  "Colors",
  "Connect",
  "Connection",
  "Connects",
  "Control",
  "CustomProp",
  "CustomProps",
  "DocProps",
  "DocumentProperties",
  "DocumentSettings",
  "DocumentSheet",
  "Ellipse",
  "EllipticalArcTo",
  "Event",
  "EventItem",
  "EventList",
  "FaceName",
  "FaceNames",
  "Field",
  "Fill",
  "FontEntry",
  "Fonts",
  "Foreign",
  "ForeignData",
  "Geom",
  "Group",
  "HeaderFooter",
  "HeaderFooterFont",
  "Help",
  "Hyperlink",
  "Icon",
  "Image",
  "InfiniteLine",
  "Layer",
  "LayerMem",
  "Layout",
  "Line",
  "LineTo",
  "Master",
  "Misc",
  "MoveTo",
  "NURBSTo",
  "Page",
  "PageLayout",
  "PageProps",
  "PageSheet",
  "Para",
  "PolylineTo",
  "PreviewPicture",
  "PrintProps",
  "PrintSetup",
  "Prop",
  "Protection",
  "RulerGrid",
  "Scratch",
  "Shape",
  "Shapes",
  "SplineKnot",
  "SplineStart",
  "StyleProp",
  "StyleSheet",
  "Tab",
  "Tabs",
  "Text",
  "TextBlock",
  "TextXForm",
  "User",
  "VisioDocument",
  "Window",
  "Windows",
  "XForm",
  "XForm1D",
  "cp",
  "fld",
  "pp",
  "tp",
  "text",
  NULL
};
