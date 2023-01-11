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
#include "pixmaps/aadlsubprogram.xpm"

/***********************************************
 **               AADL SUBPROGRAM             **
 ***********************************************/

static void
aadlsubprogram_draw_borders (Aadlbox *aadlbox, DiaRenderer *renderer)
{
  Element *elem;
  real x, y, w, h;
  Point center;

  g_return_if_fail (aadlbox != NULL);
  g_return_if_fail (renderer != NULL);

  elem = &aadlbox->element;

  x = elem->corner.x;
  y = elem->corner.y;
  w = elem->width;
  h = elem->height;

  center.x = x + 0.5*w;
  center.y = y + 0.5*h;

  dia_renderer_set_fillstyle (renderer, DIA_FILL_STYLE_SOLID);
  dia_renderer_set_linewidth (renderer, AADLBOX_BORDERWIDTH);
  dia_renderer_set_linestyle (renderer, DIA_LINE_STYLE_SOLID, 0.0);

  dia_renderer_draw_ellipse (renderer,
                             &center,
                             w,
                             h,
                             &aadlbox->fill_color,
                             &aadlbox->line_color);
}

#define heavyside(n) (n>0?1:0)
#define sign(n) (n>=0?1:-1)
#define point_to_angle(p) (atan((p)->y/(p)->x) + M_PI*heavyside(-(p)->x)*sign((p)->y))

void
aadlsubprogram_project_point_on_nearest_border(Aadlbox *aadlbox,Point *p,
					       real *angle)
{
  Point center;
  real w, h, r, t, norm_factor;

  w = aadlbox->element.width;
  h = aadlbox->element.height;
  center.x = aadlbox->element.corner.x + 0.5 * w;
  center.y = aadlbox->element.corner.y + 0.5 * h;

  point_sub(p, &center);

  /* normalize ellipse to a circle */
  r = w;
  norm_factor = r/h;
  p->y *= norm_factor;

  t = point_to_angle(p);

  p->x = 0.5*r*cos(t);
  p->y = 0.5*r*sin(t);

  /* unnormalize */
  p->y /= norm_factor;

  point_add(p, &center);
  *angle = t;
}

static Aadlbox_specific aadlsubprogram_specific =
{
  (AadlProjectionFunc) aadlsubprogram_project_point_on_nearest_border,
  (AadlTextPosFunc)    aadlsubprogram_text_position,
  (AadlSizeFunc) aadlsubprogram_minsize
};


static void
aadlsubprogram_draw (Aadlbox *aadlbox, DiaRenderer *renderer)
{
  aadlsubprogram_draw_borders (aadlbox, renderer);
  aadlbox_draw (aadlbox, renderer);
}

ObjectTypeOps aadlsubprogram_type_ops;

DiaObjectType aadlsubprogram_type =
{
  "AADL - Subprogram",           /* name */
  0,                          /* version */
  aadlsubprogram_xpm,          /* pixmap */
  &aadlsubprogram_type_ops,       /* ops */
  NULL,
  &aadlsubprogram_specific, /* user data */
  NULL, /* prop_descs: property descriptions */
  NULL, /* prop_offsets: DiaObject struct offsets */
  DIA_OBJECT_CAN_PARENT /* flags */
};


static ObjectOps aadlsubprogram_ops =
{
  (DestroyFunc)         aadlbox_destroy,
  (DrawFunc)            aadlsubprogram_draw,              /* redefined */
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



static DiaObject *aadlsubprogram_create(Point *startpoint, void *user_data, Handle **handle1, Handle **handle2)
{
  DiaObject *obj = aadlbox_create(startpoint, user_data, handle1, handle2);

  obj->type = &aadlsubprogram_type;
  obj->ops  = &aadlsubprogram_ops;

  return obj;
}

static DiaObject *
aadlsubprogram_load(ObjectNode obj_node, int version, DiaContext *ctx)
{
  DiaObject *obj;
  Point startpoint = {0.0,0.0};
  Handle *handle1,*handle2;

  obj = aadlsubprogram_create(&startpoint,&aadlsubprogram_specific, &handle1,&handle2);
  aadlbox_load(obj_node, version, ctx, (Aadlbox *) obj);
  return obj;
}


ObjectTypeOps aadlsubprogram_type_ops =
{
  (CreateFunc) aadlsubprogram_create,
  (LoadFunc)   aadlsubprogram_load,/*using_properties*/     /* load */
  (SaveFunc)   aadlbox_save,      /* save */
  (GetDefaultsFunc)   NULL,
  (ApplyDefaultsFunc) NULL
};
