/* Dia -- an diagram creation/manipulation program -*- c -*-
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

/** @file diatypes.h -- All externally visible structures should be defined here */

/* THIS HEADER MUST NOT INCLUDE ANY OTHER HEADER! */

#pragma once

#include <glib-object.h>

#ifndef __GTK_DOC_IGNORE__

/* from geometry.h - but used more generic */
typedef double real;

/* In diagramdata.h: */
typedef struct _DiagramData DiagramData;
typedef struct _NewDiagramData NewDiagramData;


typedef struct _DiaLayer DiaLayer;


/* In arrows.h: */
typedef struct _Arrow Arrow;

/* In bezier_conn.h: */
typedef struct _BezierConn BezierConn;

/* In beziershape.h: */
typedef struct _BezierShape BezierShape;

/* In boundingbox.h: */
typedef struct _PolyBBExtras PolyBBExtras;
typedef struct _LineBBExtras LineBBExtras;
typedef struct _ElementBBExtras ElementBBExtras;

/* In color.h: */
typedef struct _Color Color;

/* In connection.h: */
typedef struct _Connection Connection;

/* In connectionpoint.h: */
typedef struct _ConnectionPoint ConnectionPoint;

/* In create.h: */
typedef struct _MultipointCreateData MultipointCreateData;
typedef struct _BezierCreateData BezierCreateData;

/* In dia_image.h: */
typedef struct _DiaImage DiaImage;

/* In diamenu.h: */
typedef struct _DiaMenuItem DiaMenuItem;
typedef struct _DiaMenu DiaMenu;

/* In diacontext.h */
typedef struct _DiaContext DiaContext;

/* In diarenderer.h: */
typedef struct _BezierApprox BezierApprox;

/* In diacellrendererproperty.h: */
typedef struct _DiaCellRendererProperty DiaCellRendererProperty;

/* In diasvgrenderer.h: */
typedef struct _DiaSvgRenderer DiaSvgRenderer;
typedef struct _DiaSvgRendererClass DiaSvgRendererClass;

/* In diatransform.h: */
typedef struct _DiaTransform DiaTransform;

/* In element.h: */
typedef struct _Element Element;

/* In filter.h: */
typedef struct _DiaExportFilter DiaExportFilter;
typedef struct _DiaImportFilter DiaImportFilter;
typedef struct _DiaCallbackFilter DiaCallbackFilter;

/* In focus.h: */
typedef struct _Focus Focus;

/* In geometry.h: */
typedef struct _Point Point;
typedef struct _DiaRectangle DiaRectangle;
typedef struct _BezPoint BezPoint;
typedef struct _DiaMatrix DiaMatrix;

/* In group.h: */
typedef struct _Group Group;

/* In handle.h: */
typedef struct _Handle Handle;

/* In neworth_conn.h: */
typedef struct _NewOrthConn NewOrthConn;

/* In objchange.h: */
typedef struct _ObjectState ObjectState;

/* In object.h: */
typedef struct _DiaObject DiaObject;
typedef struct _ObjectOps ObjectOps;
typedef struct _DiaObjectType DiaObjectType;
typedef struct _ObjectTypeOps ObjectTypeOps;

/* In orth_conn.h: */
typedef struct _OrthConn OrthConn;

/* In paper.h: */
typedef struct _PaperInfo PaperInfo;

/* In pattern.h: */
typedef struct _DiaPattern DiaPattern;

/* In plug-ins.h: */
typedef struct _PluginInfo PluginInfo;

/* In poly_conn.h: */
typedef struct _PolyConn PolyConn;

/* In polyshape.h: */
typedef struct _PolyShape PolyShape;

/* In properties.h: */
typedef struct _PropDescription PropDescription;
typedef struct _Property Property;
typedef struct _PropEventData PropEventData;
typedef struct _PropDialog PropDialog;
typedef struct _PropEventHandlerChain PropEventHandlerChain;
typedef struct _PropWidgetAssoc PropWidgetAssoc;
typedef struct _PropertyOps PropertyOps;
typedef struct _PropNumData PropNumData;
typedef struct _PropEnumData PropEnumData;
typedef struct _PropDescCommonArrayExtra PropDescCommonArrayExtra;
typedef struct _PropDescDArrayExtra PropDescDArrayExtra;
typedef struct _PropDescSArrayExtra PropDescSArrayExtra;
typedef struct _PropOffset PropOffset;

/* In sheet.h: */
typedef struct _Sheet Sheet;
typedef struct _SheetObject SheetObject;

/* In text.h: */
typedef struct _Text Text;

/* In textline.h: */
typedef struct _TextLine TextLine;

/* In textattr.h: */
typedef struct _TextAttributes TextAttributes;

/** A standard definition of a function that takes a DiaObject */
typedef void (*DiaObjectFunc) (const DiaObject *obj);

typedef enum {
  PATH_UNION = 1,
  PATH_DIFFERENCE,
  PATH_INTERSECTION,
  PATH_EXCLUSION
} PathCombineMode;

#endif
