/* -*- Mode: C; c-basic-offset: 4 -*- */
/* Dia -- an diagram creation/manipulation program
 *
 * vdx-common.c: Visio XML import filter for dia
 * Copyright (C) 2006 Ian Redfern
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

#include <visio-types.h>

char * vdx_Units[] =
{
  "BOOL",
  "DA",
  "DATE",
  "DEG",
  "DL",
  "DT",
  "IN",
  "IN_F",
  "MM",
  "NUM",
  "NURBS",
  "PT",
  "STR",
  NULL
};
char * vdx_Types[] =
{
  "any",
  "Act",
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
  "Geom",
  "Group",
  "Help",
  "Hyperlink",
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
  "NameUniv",
  "Page",
  "PageLayout",
  "PageProps",
  "PageSheet",
  "Para",
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
