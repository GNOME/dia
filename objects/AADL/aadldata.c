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
#include "pixmaps/aadldata.xpm"

/***********************************************
 **                 AADL DATA                 **
 ***********************************************/


static void
aadldata_draw_borders (Aadlbox *aadlbox, DiaRenderer *renderer)
{
  Element *elem;
  double x, y, w, h;
  Point points[2];

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

  points[0].x = x;     points[0].y = y;
  points[1].x = x + w; points[1].y = y + h;

  dia_renderer_draw_rect (renderer,
                          points,
                          points + 1,
                          &aadlbox->fill_color,
                          &aadlbox->line_color);
}

static void
aadldata_draw (Aadlbox *aadlbox, DiaRenderer *renderer)
{
  aadldata_draw_borders (aadlbox, renderer);
  aadlbox_draw (aadlbox, renderer);
}

void
aadlbox_project_point_on_rectangle(DiaRectangle *rectangle,Point *p,real *angle)
{

  /* top left corner */
  double x1 = rectangle->left;
  double y1 = rectangle->top;

  /* bottom right corner */
  double x2 = rectangle->right;
  double y2 = rectangle->bottom;

  /* _ outside box: */

  /* left side*/
  if (p->x <= x1 && p->y<=y1) { p->x = x1; p->y = y1; *angle = 1.25 * M_PI;}
  else if (p->x <= x1 && p->y >= y2) {p->x = x1; p->y = y2; *angle=0.75*M_PI;}
  else if (p->x <= x1 && p->y >= y1 && p->y <= y2) {p->x = x1;*angle = M_PI;}

  /* right side */
  else if (p->x >= x2 && p->y<=y1) {p->x = x2; p->y = y1; *angle = 1.75*M_PI;}
  else if (p->x >= x2 && p->y >= y2) {p->x = x2; p->y = y2; *angle=0.25*M_PI;}
  else if (p->x >= x2 && p->y >= y1 && p->y <= y2) { p->x = x2; *angle = 0;}

  /* upper */
  else if (p->y <= y1) {p->y = y1;  *angle = 1.5 * M_PI;}

  /* lower */
  else if (p->y >= y2) {p->y = y2; *angle = 0.5 * M_PI;}

  /* _ inside box: */
  else {
    /* distances to border */
    real d1 = p->x - x1;
    real d2 = x2 - p->x;
    real d3 = p->y - y1;
    real d4 = y2 - p->y;

    real mini = min(min(d1,d2), min(d3,d4));

    if (d1 == mini)      { p->x = x1; *angle = M_PI     ;}
    else if (d2 == mini) { p->x = x2; *angle = 0        ;}
    else if (d3 == mini) { p->y = y1; *angle = 1.5*M_PI ;}
    else if (d4 == mini) { p->y = y2; *angle = 0.5*M_PI ;}
  }
}

void
aadldata_project_point_on_nearest_border(Aadlbox *aadlbox,Point *p,real *angle)
{
  DiaRectangle rectangle;

  rectangle.left = aadlbox->element.corner.x;
  rectangle.top  = aadlbox->element.corner.y;
  rectangle.right  = aadlbox->element.corner.x + aadlbox->element.width;
  rectangle.bottom = aadlbox->element.corner.y + aadlbox->element.height;

  aadlbox_project_point_on_rectangle(&rectangle, p, angle);
}



static Aadlbox_specific aadldata_specific =
{
  (AadlProjectionFunc) aadldata_project_point_on_nearest_border,
  (AadlTextPosFunc)    aadldata_text_position,
  (AadlSizeFunc) aadldata_minsize
};



ObjectTypeOps aadldata_type_ops;

DiaObjectType aadldata_type =
{
  "AADL - Data",           /* name */
  0,                    /* version */
  aadldata_xpm,          /* pixmap */
  &aadldata_type_ops,       /* ops */
  NULL,
  &aadldata_specific, /* user data */
  NULL, /* prop_descs: property descriptions */
  NULL, /* prop_offsets: DiaObject struct offsets */
  DIA_OBJECT_CAN_PARENT /* flags */
};


static ObjectOps aadldata_ops =
{
  (DestroyFunc)         aadlbox_destroy,
  (DrawFunc)            aadldata_draw,              /* redefined */
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



static DiaObject *aadldata_create(Point *startpoint, void *user_data, Handle **handle1, Handle **handle2)
{
  DiaObject *obj = aadlbox_create(startpoint, user_data, handle1, handle2);

  obj->type = &aadldata_type;
  obj->ops  = &aadldata_ops;

  return obj;
}

static DiaObject *aadldata_load(ObjectNode obj_node, int version, DiaContext *ctx)
{
  DiaObject *obj;
  Point startpoint = {0.0,0.0};
  Handle *handle1,*handle2;

  obj = aadldata_create(&startpoint,&aadldata_specific, &handle1,&handle2);
  aadlbox_load(obj_node, version, ctx, (Aadlbox *) obj);

  return obj;
}


ObjectTypeOps aadldata_type_ops =
{
  (CreateFunc) aadldata_create,
  (LoadFunc)   aadldata_load,      /* load */
  (SaveFunc)   aadlbox_save,       /* save */
  (GetDefaultsFunc)   NULL,
  (ApplyDefaultsFunc) NULL
};
