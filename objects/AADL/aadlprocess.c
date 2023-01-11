/* AADL plugin for DIA
*
* Copyright (C) 2005 Laboratoire d'Informatique de Paris 6
* Author: Pierre Duquesne
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

#include "config.h"

#include <glib/gi18n-lib.h>

#include "aadl.h"
#include "pixmaps/aadlprocess.xpm"

/***********************************************
 **               AADL PROCESS             **
 ***********************************************/


void
aadlbox_draw_inclined_box (Aadlbox      *aadlbox,
                           DiaRenderer  *renderer,
                           DiaLineStyle  linestyle)
{
  Element *elem;
  double x, y, w, h;
  Point points[4];

  g_return_if_fail (aadlbox != NULL);
  g_return_if_fail (renderer != NULL);

  elem = &aadlbox->element;

  x = elem->corner.x;
  y = elem->corner.y;
  w = elem->width;
  h = elem->height;

  points[0].x = x + w * AADLBOX_INCLINE_FACTOR;
  points[1].x = x + w;
  points[0].y = points[1].y = y;

  points[3].x = x;
  points[2].x = x + w - w * AADLBOX_INCLINE_FACTOR;
  points[3].y = points[2].y = y + h;

  dia_renderer_set_fillstyle (renderer, DIA_FILL_STYLE_SOLID);
  dia_renderer_set_linewidth (renderer, AADLBOX_BORDERWIDTH);
  dia_renderer_set_linestyle (renderer, linestyle, AADLBOX_DASH_LENGTH);

  dia_renderer_draw_polygon (renderer,
                             points,
                             4,
                             &aadlbox->fill_color,
                             &aadlbox->line_color);
}


static void
aadlprocess_draw_borders (Aadlbox *aadlbox, DiaRenderer *renderer)
{
  aadlbox_draw_inclined_box (aadlbox, renderer, DIA_LINE_STYLE_SOLID);
}


void
aadlbox_inclined_project_point_on_nearest_border(Aadlbox *aadlbox,Point *p,
					       real *angle)
{
  /*
        ++++++B------------+                 +----------++++++++
        +    /000000000000/+		     |0000000000|      +
        +   /000000000000/ +		     |0000000000|      +
        +  /000000000000/  +    ---------->  |0000000000|      +
        + /000000000000/   +		     |0000000000|      +
        +/000000000000/    +		     |0000000000|      +
        A------------ ++++++                 +----------++++++++
  */

  DiaRectangle rectangle;
  real w, h, delta_y, delta_x;

  w = aadlbox->element.width;
  h = aadlbox->element.height;

  rectangle.top = aadlbox->element.corner.y;
  rectangle.left = aadlbox->element.corner.x;
  rectangle.bottom = aadlbox->element.corner.y + h;
  rectangle.right = aadlbox->element.corner.x + w - w * AADLBOX_INCLINE_FACTOR;

  delta_y = h - (p->y - aadlbox->element.corner.y);
  delta_x = delta_y * (w * AADLBOX_INCLINE_FACTOR) / h;

  p->x -= delta_x;

  aadlbox_project_point_on_rectangle(&rectangle, p, angle);

  delta_y = h - (p->y - aadlbox->element.corner.y);
  delta_x = delta_y * (w * AADLBOX_INCLINE_FACTOR) / h;

  p->x += delta_x;
}

static void aadlprocess_draw(Aadlbox *aadlbox, DiaRenderer *renderer)
{
  aadlprocess_draw_borders(aadlbox, renderer);
  aadlbox_draw(aadlbox, renderer);
}


static Aadlbox_specific aadlprocess_specific =
{
  (AadlProjectionFunc) aadlbox_inclined_project_point_on_nearest_border,
  (AadlTextPosFunc)    aadlprocess_text_position,
  (AadlSizeFunc) aadlprocess_minsize
};


ObjectTypeOps aadlprocess_type_ops;

DiaObjectType aadlprocess_type =
{
  "AADL - Process",           /* name */
  0,                       /* version */
  aadlprocess_xpm,          /* pixmap */
  &aadlprocess_type_ops,       /* ops */
  NULL,
  &aadlprocess_specific, /* user data */
  NULL, /* prop_descs: property descriptions */
  NULL, /* prop_offsets: DiaObject struct offsets */
  DIA_OBJECT_CAN_PARENT /* flags */
};


static ObjectOps aadlprocess_ops =
{
  (DestroyFunc)         aadlbox_destroy,
  (DrawFunc)            aadlprocess_draw,              /* redefined */
  (DistanceFunc)        aadlbox_distance_from,
  (SelectFunc)          aadlbox_select,
  (CopyFunc)            aadlbox_copy,
  (MoveFunc)            aadlbox_move,
  (MoveHandleFunc)      aadlbox_move_handle,
  (GetPropertiesFunc)   object_create_props_dialog,
  (ApplyPropertiesDialogFunc) object_apply_props_from_dialog,
  (ObjectMenuFunc)      aadlbox_get_object_menu,
  (DescribePropsFunc)   aadlbox_describe_props,
  (GetPropsFunc)        aadlbox_get_props,
  (SetPropsFunc)        aadlbox_set_props,
  (TextEditFunc) 0,
  (ApplyPropertiesListFunc) object_apply_props,
};



static DiaObject *aadlprocess_create(Point *startpoint, void *user_data, Handle **handle1, Handle **handle2)
{
  DiaObject *obj = aadlbox_create(startpoint, user_data, handle1, handle2);

  obj->type = &aadlprocess_type;
  obj->ops  = &aadlprocess_ops;

  return obj;
}

static DiaObject *
aadlprocess_load(ObjectNode obj_node, int version,DiaContext *ctx)
{
  DiaObject *obj;
  Point startpoint = {0.0,0.0};
  Handle *handle1,*handle2;

  obj = aadlprocess_create(&startpoint,&aadlprocess_specific, &handle1,&handle2);
  aadlbox_load(obj_node, version, ctx, (Aadlbox *) obj);
  return obj;
}


ObjectTypeOps aadlprocess_type_ops =
{
  (CreateFunc) aadlprocess_create,
  (LoadFunc)   aadlprocess_load,/*using_properties*/     /* load */
  (SaveFunc)   aadlbox_save,      /* save */
  (GetDefaultsFunc)   NULL,
  (ApplyDefaultsFunc) NULL
};
