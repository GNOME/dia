/* 
 * SADT diagram support for dia 
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

#ifndef __CONNPOINT_LINE_H
#define __CONNPOINT_LINE_H

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <glib.h>

#include "geometry.h"
#include "connectionpoint.h"
#include "object.h"
#include "dia_xml.h"

typedef struct {
  Point start,end;
  Object *parent;
  
  int num_connections;
  GSList *connections;
} ConnPointLine;


extern ConnPointLine *connpointline_create(Object *parent, 
					   int num_connections);
extern void connpointline_destroy(ConnPointLine *cpl);
extern void connpointline_update(ConnPointLine *cpl,Point *start,Point *end);
extern int connpointline_can_add_point(ConnPointLine *cpl,
				       Point *clicked);
extern int connpointline_can_remove_point(ConnPointLine *cpl,
					  Point *clicked);

extern ObjectChange *connpointline_add_points(ConnPointLine *cpl, 
					      Point *clickedpoint, 
					      int count);
extern ObjectChange *connpointline_remove_points(ConnPointLine *cpl, 
						 Point *clickedpoint, 
						 int count);
extern ConnPointLine *connpointline_load(Object *obj,ObjectNode obj_node,
					 const gchar *name, int default_nc,
					 int *realconncount);
extern void connpointline_save(ConnPointLine *cpl,ObjectNode obj_node,
			       const gchar *name);
extern ConnPointLine *connpointline_copy(Object *newobj,ConnPointLine *cpl,
					 int *realconncount);

inline static ObjectChange *
connpointline_add_point(ConnPointLine *cpl, Point *clickedpoint)
{ 
  return connpointline_add_points(cpl,clickedpoint,1); 
}

inline static ObjectChange *
connpointline_remove_point(ConnPointLine *cpl, Point *clickedpoint)
{
  return connpointline_remove_points(cpl,clickedpoint,1);
}


#endif __CONNPOINT_LINE_H




