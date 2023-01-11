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
#include "pixmaps/aadlpackage.xpm"

/* To have a really useful package element, we should make it a class of its
   own, not inherited from aadlbox, with 2 special handles -- that could be
   moved -- to draw the public/private separation */

/***********************************************
 **                 AADL PACKAGE              **
 ***********************************************/


static void
aadlpackage_draw_borders (Aadlbox *aadlbox, DiaRenderer *renderer)
{
  Element *elem;
  real x, y, w, h;
  Point points[9];

  g_return_if_fail (aadlbox != NULL);
  g_return_if_fail (renderer != NULL);

  elem = &aadlbox->element;

  x = elem->corner.x;
  y = elem->corner.y;
  w = elem->width;
  h = elem->height;

  dia_renderer_set_fillstyle (renderer, DIA_FILL_STYLE_SOLID);
  dia_renderer_set_linewidth (renderer, AADLBOX_BORDERWIDTH);
  dia_renderer_set_linestyle (renderer, DIA_LINE_STYLE_SOLID, 0.0);

  points[0].x = x;                 points[0].y = y;
  points[1].x = x + 0.03 * w ;     points[1].y = y;
  points[2].x = x + 0.08 * w ;     points[2].y = y -  AADL_PORT_MAX_OUT;
  points[3].x = x + 0.40 * w ;     points[3].y = y -  AADL_PORT_MAX_OUT;
  points[4].x = x + 0.45 * w ;     points[4].y = y;
  points[5].x = x + w - 0.05 * w;  points[5].y = y;
  points[6].x = x + w;             points[6].y = y + 0.05 * h;
  points[7].x = x + w;             points[7].y = y + h;
  points[8].x = x ;                points[8].y = y + h;

  dia_renderer_draw_polygon (renderer,
                             points,
                             9,
                             &aadlbox->fill_color,
                             &aadlbox->line_color);
}


static void aadlpackage_draw(Aadlbox *aadlbox, DiaRenderer *renderer)
{
  aadlpackage_draw_borders(aadlbox, renderer);
  aadlbox_draw(aadlbox, renderer);
}


static Aadlbox_specific aadlpackage_specific =
{
  (AadlProjectionFunc) aadldata_project_point_on_nearest_border,
  (AadlTextPosFunc)    aadldata_text_position,
  (AadlSizeFunc) aadldata_minsize
};

ObjectTypeOps aadlpackage_type_ops;

DiaObjectType aadlpackage_type =
{
  "AADL - Package",           /* name */
  0,                       /* version */
  aadlpackage_xpm,          /* pixmap */

  &aadlpackage_type_ops,      /* ops */
  NULL,
  &aadlpackage_specific, /* user data */
  NULL, /* prop_descs: property descriptions */
  NULL, /* prop_offsets: DiaObject struct offsets */
  DIA_OBJECT_CAN_PARENT /* flags */
};


static ObjectOps aadlpackage_ops =
{
  (DestroyFunc)         aadlbox_destroy,
  (DrawFunc)            aadlpackage_draw,              /* redefined */
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



static DiaObject *aadlpackage_create(Point *startpoint, void *user_data, Handle **handle1, Handle **handle2)
{
  DiaObject *obj = aadlbox_create(startpoint, user_data, handle1, handle2);

  obj->type = &aadlpackage_type;
  obj->ops  = &aadlpackage_ops;

  return obj;
}

static DiaObject *
aadlpackage_load(ObjectNode obj_node, int version, DiaContext *ctx)
{
  DiaObject *obj;
  Point startpoint = {0.0,0.0};
  Handle *handle1,*handle2;

  obj = aadlpackage_create(&startpoint,&aadlpackage_specific, &handle1,&handle2);
  aadlbox_load(obj_node, version, ctx, (Aadlbox *) obj);

  return obj;
}


ObjectTypeOps aadlpackage_type_ops =
{
  (CreateFunc) aadlpackage_create,
  (LoadFunc)   aadlpackage_load,      /* load */
  (SaveFunc)   aadlbox_save,          /* save */
  (GetDefaultsFunc)   NULL,
  (ApplyDefaultsFunc) NULL
};
