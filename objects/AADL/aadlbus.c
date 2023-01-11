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
#include "pixmaps/aadlbus.xpm"

/***********************************************
 **               AADL BUS             **
 ***********************************************/

static void
aadlbus_draw_borders (Aadlbox *aadlbox, DiaRenderer *renderer)
{
  Element *elem;
  real x, y, w, h;
  Point points[10];

  g_return_if_fail (aadlbox != NULL);
  g_return_if_fail (renderer != NULL);

  elem = &aadlbox->element;

  x = elem->corner.x;
  y = elem->corner.y;
  w = elem->width;
  h = elem->height;

  points[0].x = x;
  points[0].y = y + h*0.5;

  points[1].x = x + w*AADL_BUS_ARROW_SIZE_FACTOR;
  points[1].y = y;

  points[2].x = x + w*AADL_BUS_ARROW_SIZE_FACTOR;
  points[2].y = y + h*AADL_BUS_HEIGHT_FACTOR;

  points[3].x = x + w - w*AADL_BUS_ARROW_SIZE_FACTOR;
  points[3].y = y + h*AADL_BUS_HEIGHT_FACTOR;

  points[4].x = x + w - w*AADL_BUS_ARROW_SIZE_FACTOR;
  points[4].y = y;

  points[5].x = x + w;
  points[5].y = y + h*0.5;

  points[6].x = x + w - w*AADL_BUS_ARROW_SIZE_FACTOR;
  points[6].y = y + h ;

  points[7].x = x + w - w*AADL_BUS_ARROW_SIZE_FACTOR;
  points[7].y = y + h*(1-AADL_BUS_HEIGHT_FACTOR);

  points[8].x = x + w*AADL_BUS_ARROW_SIZE_FACTOR;
  points[8].y = y + h*(1-AADL_BUS_HEIGHT_FACTOR);

  points[9].x = x + w*AADL_BUS_ARROW_SIZE_FACTOR;
  points[9].y = y + h;

  dia_renderer_set_fillstyle (renderer, DIA_FILL_STYLE_SOLID);
  dia_renderer_set_linewidth (renderer, AADLBOX_BORDERWIDTH);
  dia_renderer_set_linestyle (renderer, DIA_LINE_STYLE_SOLID, 0.0);

  dia_renderer_draw_polygon (renderer,
                             points,
                             10,
                             &aadlbox->fill_color,
                             &aadlbox->line_color);
}

static void
aadlbus_draw (Aadlbox *aadlbox, DiaRenderer *renderer)
{
  aadlbus_draw_borders (aadlbox, renderer);
  aadlbox_draw (aadlbox, renderer);
}

static void
aadlbus_project_point_on_nearest_border(Aadlbox *aadlbox,Point *p,
					       real *angle)
{
  Element *element = &aadlbox->element;

  real w = element->width;
  real h = element->height;

  /* top left corner */
  double x1 = element->corner.x;
  double y1 = element->corner.y;

  /* bottom right corner */
  double x2 = element->corner.x + w;
  double y2 = element->corner.y + h;


  if ( p->x >= x1 + w*AADL_BUS_ARROW_SIZE_FACTOR
       && p->x <= x2 - w*AADL_BUS_ARROW_SIZE_FACTOR)
  {
    DiaRectangle rectangle;

    rectangle.left = x1 + w*AADL_BUS_ARROW_SIZE_FACTOR;
    rectangle.top  = y1 + h*AADL_BUS_HEIGHT_FACTOR;
    rectangle.right  = x2 - w*AADL_BUS_ARROW_SIZE_FACTOR;
    rectangle.bottom = y2 - h*AADL_BUS_HEIGHT_FACTOR;

    aadlbox_project_point_on_rectangle(&rectangle, p, angle);
  }
  else
  {
    Point a, b, c, m;
    real k1, k2;

    /* left arrow */
    if (p->x < x1 + w*AADL_BUS_ARROW_SIZE_FACTOR) {

      *angle = M_PI;

      /*        m      b
                 �    /|
                     / |
                    /  |
                   a---c
      */

      a.x = x1;
      a.y = y1 + h*0.5;

      b.x = x1 + w*AADL_BUS_ARROW_SIZE_FACTOR;
      b.y = (p->y<y1+0.5*h)?y1:y2;                /* up or down */

      c.x = b.x;
      c.y = a.y;
    }

    /* right arrow */
    else {

      *angle = 0;

      a.x = x2;
      a.y = y1 + h*0.5;

      b.x = x2 - w*AADL_BUS_ARROW_SIZE_FACTOR;
      b.y = (p->y<y1+0.5*h)?y1:y2;               /* up or down */

      c.x = b.x;
      c.y = a.y;
    }

    point_copy(&m, p);

    /* intersection between (AB) and (MC) */

    k1 = (b.y - a.y) / (b.x - a.x);
    k2 = (m.y - c.y) / (m.x - c.x);

    p->x = (m.y - a.y + k1*a.x - k2*m.x) / (k1 - k2);
    p->y = a.y + k1 * (p->x - a.x);
  }
}




static Aadlbox_specific aadlbus_specific =
{
  (AadlProjectionFunc) aadlbus_project_point_on_nearest_border,
  (AadlTextPosFunc)    aadlbus_text_position,
  (AadlSizeFunc) aadlbus_minsize
};

extern ObjectTypeOps aadlbus_type_ops;

DiaObjectType aadlbus_type =
{
  "AADL - Bus",           /* name */
  0,                   /* version */
  aadlbus_xpm,          /* pixmap */
  &aadlbus_type_ops,       /* ops */
  NULL,
  &aadlbus_specific, /* user data */
  NULL, /* prop_descs: property descriptions */
  NULL, /* prop_offsets: DiaObject struct offsets */
  DIA_OBJECT_CAN_PARENT /* flags */
};

static ObjectOps aadlbus_ops =
{
  (DestroyFunc)         aadlbox_destroy,
  (DrawFunc)            aadlbus_draw,              /* redefined */
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



static DiaObject *aadlbus_create(Point *startpoint, void *user_data, Handle **handle1, Handle **handle2)
{
  DiaObject *obj = aadlbox_create(startpoint, user_data, handle1, handle2);

  obj->type = &aadlbus_type;
  obj->ops  = &aadlbus_ops;

  return obj;
}

static DiaObject *aadlbus_load(ObjectNode obj_node, int version,DiaContext *ctx)
{
  DiaObject *obj;
  Point startpoint = {0.0,0.0};
  Handle *handle1,*handle2;

  obj = aadlbus_create(&startpoint,&aadlbus_specific, &handle1,&handle2);
  aadlbox_load(obj_node, version, ctx, (Aadlbox *) obj);
  return obj;
}


ObjectTypeOps aadlbus_type_ops =
{
  (CreateFunc) aadlbus_create,
  (LoadFunc)   aadlbus_load,/*using_properties*/     /* load */
  (SaveFunc)   aadlbox_save,      /* save */
  (GetDefaultsFunc)   NULL,
  (ApplyDefaultsFunc) NULL
};
