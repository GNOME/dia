/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 2006 Lars Clausen
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

/**
 * SECTION:create
 * @title: Create
 * @short_description: Construct objects
 *
 * This file contains functions for importers in particular to create
 * standard objects.
 */

#include "config.h"

#include <glib/gi18n-lib.h>

#include <glib.h>
#include <stdlib.h>

#include "message.h"
#include "geometry.h"
#include "filter.h"
#include "object.h"
#include "properties.h"
#include "propinternals.h"
#include "group.h"
#include "create.h"


DiaObject *
create_standard_text (double xpos, double ypos)
{
  DiaObjectType *otype = object_get_type ("Standard - Text");
  DiaObject *new_obj;
  Handle *h1, *h2;
  Point point;

  if (otype == NULL) {
    message_error (_("Can't find standard object"));
    return NULL;
  }

  point.x = xpos;
  point.y = ypos;

  new_obj = otype->ops->create (&point, otype->default_user_data, &h1, &h2);

  return new_obj;
}


static PropDescription create_element_prop_descs[] = {
    { "elem_corner", PROP_TYPE_POINT },
    { "elem_width", PROP_TYPE_REAL },
    { "elem_height", PROP_TYPE_REAL },
    PROP_DESC_END};

static GPtrArray *make_element_props(real xpos, real ypos,
                                     real width, real height)
{
    GPtrArray *props;
    PointProperty *pprop;
    RealProperty *rprop;

    props = prop_list_from_descs(create_element_prop_descs,pdtpp_true);
    g_assert(props->len == 3);

    pprop = g_ptr_array_index(props,0);
    pprop->point_data.x = xpos;
    pprop->point_data.y = ypos;
    rprop = g_ptr_array_index(props,1);
    rprop->real_data = width;
    rprop = g_ptr_array_index(props,2);
    rprop->real_data = height;

    return props;
}

DiaObject *
create_standard_ellipse(real xpos, real ypos, real width, real height) {
    DiaObjectType *otype = object_get_type("Standard - Ellipse");
    DiaObject *new_obj;
    Handle *h1, *h2;

    GPtrArray *props;
    Point point;

    if (otype == NULL){
	message_error(_("Can't find standard object"));
	return NULL;
    }

    point.x = xpos;
    point.y = ypos;

    new_obj = otype->ops->create (&point,
                                  otype->default_user_data,
                                  &h1,
                                  &h2);

    props = make_element_props (xpos, ypos, width, height);
    dia_object_set_properties (new_obj, props);
    prop_list_free (props);

    return new_obj;
}


DiaObject *
create_standard_box(real xpos, real ypos, real width, real height) {
    DiaObjectType *otype = object_get_type("Standard - Box");
    DiaObject *new_obj;
    Handle *h1, *h2;
    Point point;
    GPtrArray *props;

    if (otype == NULL){
	message_error(_("Can't find standard object"));
	return NULL;
    }

    point.x = xpos;
    point.y = ypos;

    new_obj = otype->ops->create (&point,
                                  otype->default_user_data,
                                  &h1,
                                  &h2);

    props = make_element_props (xpos, ypos, width, height);
    dia_object_set_properties (new_obj, props);
    prop_list_free (props);

    return new_obj;
}

static PropDescription create_line_prop_descs[] = {
    PROP_STD_START_ARROW,
    PROP_STD_END_ARROW,
    PROP_DESC_END};

DiaObject *
create_standard_zigzagline(int num_points, const Point *points,
			   const Arrow *end_arrow, const Arrow *start_arrow)
{
    DiaObjectType *otype = object_get_type("Standard - ZigZagLine");
    DiaObject *new_obj;
    Handle *h1, *h2;
    MultipointCreateData pcd;
    GPtrArray *props;

    if (otype == NULL){
	message_error(_("Can't find standard object"));
	return NULL;
    }

    pcd.num_points = num_points;
    pcd.points = (Point *)points;

    new_obj = otype->ops->create(NULL, &pcd, &h1, &h2);

    props = prop_list_from_descs (create_line_prop_descs, pdtpp_true);
    g_assert (props->len == 2);

    if (start_arrow != NULL) {
      ((ArrowProperty *) g_ptr_array_index (props, 0))->arrow_data = *start_arrow;
    }
    if (end_arrow != NULL) {
      ((ArrowProperty *) g_ptr_array_index (props, 1))->arrow_data = *end_arrow;
    }

    dia_object_set_properties (new_obj, props);
    prop_list_free (props);

    return new_obj;
}

DiaObject *
create_standard_polyline(int num_points,
			 Point *points,
			 Arrow *end_arrow,
			 Arrow *start_arrow)
{
    DiaObjectType *otype = object_get_type("Standard - PolyLine");
    DiaObject *new_obj;
    Handle *h1, *h2;
    MultipointCreateData pcd;
    GPtrArray *props;

    if (otype == NULL){
	message_error(_("Can't find standard object"));
	return NULL;
    }

    pcd.num_points = num_points;
    pcd.points = points;

    new_obj = otype->ops->create(NULL, &pcd, &h1, &h2);

    props = prop_list_from_descs (create_line_prop_descs, pdtpp_true);
    g_assert (props->len == 2);

    if (start_arrow != NULL) {
      ((ArrowProperty *) g_ptr_array_index (props, 0))->arrow_data = *start_arrow;
    }
    if (end_arrow != NULL) {
      ((ArrowProperty *) g_ptr_array_index (props, 1))->arrow_data = *end_arrow;
    }

    dia_object_set_properties (new_obj, props);
    prop_list_free (props);

    return new_obj;
}

DiaObject *
create_standard_polygon(int num_points,
			Point *points) {
    DiaObjectType *otype = object_get_type("Standard - Polygon");
    DiaObject *new_obj;
    Handle *h1, *h2;
    MultipointCreateData pcd;

    if (otype == NULL){
	message_error(_("Can't find standard object"));
	return NULL;
    }

    pcd.num_points = num_points;
    pcd.points = points;

    new_obj = otype->ops->create(NULL, &pcd, &h1, &h2);

    return new_obj;
}

DiaObject *
create_standard_bezierline(int num_points,
			   BezPoint *points,
			   Arrow *end_arrow,
			   Arrow *start_arrow) {
    DiaObjectType *otype = object_get_type("Standard - BezierLine");
    DiaObject *new_obj;
    Handle *h1, *h2;
    BezierCreateData bcd;
    GPtrArray *props;

    if (otype == NULL){
	message_error(_("Can't find standard object"));
	return NULL;
    }

    bcd.num_points = num_points;
    bcd.points = points;

    new_obj = otype->ops->create(NULL, &bcd, &h1, &h2);

    props = prop_list_from_descs (create_line_prop_descs, pdtpp_true);
    g_assert (props->len == 2);

    if (start_arrow != NULL) {
      ((ArrowProperty *) g_ptr_array_index (props, 0))->arrow_data = *start_arrow;
    }
    if (end_arrow != NULL) {
      ((ArrowProperty *) g_ptr_array_index (props, 1))->arrow_data = *end_arrow;
    }

    dia_object_set_properties (new_obj, props);
    prop_list_free (props);

    return new_obj;
}

DiaObject *
create_standard_beziergon(int num_points,
			  BezPoint *points) {
    DiaObjectType *otype = object_get_type("Standard - Beziergon");
    DiaObject *new_obj;
    Handle *h1, *h2;
    BezierCreateData bcd;

    if (otype == NULL){
	message_error(_("Can't find standard object"));
	return NULL;
    }

    bcd.num_points = num_points;
    bcd.points = points;

    new_obj = otype->ops->create(NULL, &bcd, &h1, &h2);

    return new_obj;
}

DiaObject *
create_standard_path(int num_points, BezPoint *points)
{
    DiaObjectType *otype = object_get_type("Standard - Path");
    DiaObject *new_obj;
    Handle *h1, *h2;
    BezierCreateData bcd;

    if (otype == NULL){
	message_error(_("Can't find standard object"));
	return NULL;
    }

    bcd.num_points = num_points;
    bcd.points = points;

    new_obj = otype->ops->create(NULL, &bcd, &h1, &h2);

    return new_obj;
}

static PropDescription create_arc_prop_descs[] = {
    { "curve_distance", PROP_TYPE_REAL },
    PROP_STD_START_ARROW,
    PROP_STD_END_ARROW,
    PROP_DESC_END};

DiaObject *
create_standard_arc(real x1, real y1, real x2, real y2,
		    real distance,
		    Arrow *end_arrow,
		    Arrow *start_arrow) {
    DiaObjectType *otype = object_get_type("Standard - Arc");
    DiaObject *new_obj;
    Handle *h1, *h2;
    Point p1, p2;
    GPtrArray *props;

    if (otype == NULL){
	message_error(_("Can't find standard object"));
	return NULL;
    }

    p1.x = x1;
    p1.y = y1;
    p2.x = x2;
    p2.y = y2;


    new_obj = otype->ops->create (&p1,
                                  otype->default_user_data,
                                  &h1,
                                  &h2);
    dia_object_move_handle (new_obj, h2, &p2, NULL, HANDLE_MOVE_USER_FINAL, 0);
    props = prop_list_from_descs (create_arc_prop_descs, pdtpp_true);
    g_assert (props->len == 3);

    ((RealProperty *) g_ptr_array_index (props, 0))->real_data = distance;
    if (start_arrow != NULL) {
      ((ArrowProperty *) g_ptr_array_index (props, 1))->arrow_data = *start_arrow;
    }
    if (end_arrow != NULL) {
      ((ArrowProperty *) g_ptr_array_index (props, 2))->arrow_data = *end_arrow;
    }

    dia_object_set_properties (new_obj, props);
    prop_list_free (props);

    return new_obj;
}

static PropDescription create_file_prop_descs[] = {
    { "image_file", PROP_TYPE_FILE },
    PROP_DESC_END};

DiaObject *
create_standard_image(real xpos, real ypos, real width, real height,
		      char *file) {
    DiaObjectType *otype = object_get_type("Standard - Image");
    DiaObject *new_obj;
    Handle *h1, *h2;
    Point point;
    GPtrArray *props;
    StringProperty *sprop;

    if (otype == NULL){
	message_error(_("Can't find standard object"));
	return NULL;
    }

    point.x = xpos;
    point.y = ypos;

    new_obj = otype->ops->create (&point,
                                  otype->default_user_data,
                                  &h1,
                                  &h2);

    props = make_element_props (xpos, ypos, width, height);
    dia_object_set_properties (new_obj, props);
    prop_list_free (props);


    props = prop_list_from_descs (create_file_prop_descs, pdtpp_true);
    g_assert (props->len == 1);
    sprop = g_ptr_array_index (props, 0);
    g_clear_pointer (&sprop->string_data, g_free);
    sprop->string_data = g_strdup (file);
    dia_object_set_properties (new_obj, props);
    prop_list_free (props);

    return new_obj;
}

DiaObject *
create_standard_group(GList *items) {
    DiaObject *new_obj;

    new_obj = group_create((GList*)items);

    return new_obj;
}
