/* Dynamic container for connection points  -*- c -*-
 * Copyright(C) 2000 Cyrille Chepelov
 *
 * Connection point line is a helper struct, to hold a few connection points
 * on a line segment. There can be a variable number of these connection 
 * points. The user should be made able to add or remove some connection 
 * points.
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
 \file connpoint_line.h - services for extra connection points 
 */

#ifndef __CONNPOINT_LINE_H
#define __CONNPOINT_LINE_H

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <glib.h>

#include "geometry.h"
#include "connectionpoint.h"
#include "object.h"
#include "dia_xml.h"

/*!
 * \brief Helper class to have dynamic _ConnectionPoint on a line
 *
 * ConnPointLine can be used to implement dynamic conection points for
 * a _DiaObject. It supports unddo/redo, load/save to standard props
 * and obviously connections to it.
 *
 * \ingroup ObjectParts
 */
typedef struct _ConnPointLine {
  /*! Placement of the line. */
  Point start, end;
  /*! _DiaObject owning this  ConnPointLine */
  DiaObject *parent;
  /*! The current number of connection - read only */
  int num_connections;
  /*! The list of _ConnectionPoint */
  GSList *connections;
} ConnPointLine;


ConnPointLine *connpointline_create(DiaObject *parent, 
				    int num_connections);
void connpointline_destroy(ConnPointLine *cpl);
void connpointline_update(ConnPointLine *cpl);
void connpointline_putonaline(ConnPointLine *cpl,Point *start,Point *end, gint dirs);
int connpointline_can_add_point(ConnPointLine *cpl,
				Point *clicked);
int connpointline_can_remove_point(ConnPointLine *cpl,
				   Point *clicked);

ObjectChange *connpointline_add_points(ConnPointLine *cpl, 
				       Point *clickedpoint, 
				       int count);
ObjectChange *connpointline_remove_points(ConnPointLine *cpl, 
					  Point *clickedpoint, 
					  int count);
ConnPointLine *connpointline_load(DiaObject *obj,ObjectNode obj_node,
				  const gchar *name, int default_nc,
				  int *realconncount,
				  DiaContext *ctx);
void connpointline_save(ConnPointLine *cpl,ObjectNode obj_node,
			const gchar *name, DiaContext *ctx);
ConnPointLine *connpointline_copy(DiaObject *newobj,ConnPointLine *cpl,
				  int *realconncount);

#define connpointline_add_point(cpl, clickedpoint) \
    connpointline_add_points(cpl, clickedpoint, 1)
#define connpointline_remove_point(cpl, clickedpoint) \
    connpointline_remove_points(cpl, clickedpoint, 1)

int connpointline_adjust_count(ConnPointLine *cpl,
			       int newcount, Point *where);

#endif /* __CONNPOINT_LINE_H */
