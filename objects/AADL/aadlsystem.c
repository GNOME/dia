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
#include "pixmaps/aadlsystem.xpm"

/***********************************************
 **                 AADL SYSTEM               **
 ***********************************************/

void
aadlbox_draw_rounded_box (Aadlbox      *aadlbox,
                          DiaRenderer  *renderer,
                          DiaLineStyle  linestyle)
{
  Element *elem;
  double x, y, w, h;
  BezPoint bez[9];

  g_return_if_fail (aadlbox != NULL);
  g_return_if_fail (renderer != NULL);

  elem = &aadlbox->element;

  x = elem->corner.x;
  y = elem->corner.y;
  w = elem->width;
  h = elem->height;

  /*      x  0-------------1  x      */
  /*	   /		     \ 	     */
  /*	  7    	       	      2	     */
  /*	  |  		      |	     */
  /*	  6    	 	      3	     */
  /*	   \ 	       	     / 	     */
  /*   	  x 5---------------4 x	     */
  /*	       	       	             */

  bez[0].type = BEZ_MOVE_TO;
  bez[1].type = BEZ_LINE_TO;
  bez[2].type = BEZ_CURVE_TO;
  bez[3].type = BEZ_LINE_TO;
  bez[4].type = BEZ_CURVE_TO;
  bez[5].type = BEZ_LINE_TO;
  bez[6].type = BEZ_CURVE_TO;
  bez[7].type = BEZ_LINE_TO;
  bez[8].type = BEZ_CURVE_TO;


  bez[0].p1.y = bez[1].p1.y = bez[2].p1.y =  bez[2].p2.y = y;
  bez[0].p1.x = x + AADL_ROUNDEDBOX_CORNER_SIZE_FACTOR * w;
  bez[1].p1.x = x + w - AADL_ROUNDEDBOX_CORNER_SIZE_FACTOR * w;
  bez[2].p1.x = bez[2].p3.x = bez[2].p2.x = x + w;
  bez[2].p3.y = y + AADL_ROUNDEDBOX_CORNER_SIZE_FACTOR * h;

  bez[3].p1.x = bez[4].p1.x = bez[4].p2.x = x + w;
  bez[3].p1.y = y + h - h * AADL_ROUNDEDBOX_CORNER_SIZE_FACTOR;
  bez[4].p1.y = bez[4].p2.y =  bez[4].p3.y = y + h;
  bez[4].p3.x = x + w - w * AADL_ROUNDEDBOX_CORNER_SIZE_FACTOR;

  bez[5].p1.y = bez[6].p1.y = bez[6].p2.y = y + h;
  bez[5].p1.x = x + w * AADL_ROUNDEDBOX_CORNER_SIZE_FACTOR;
  bez[6].p1.x = bez[6].p2.x = bez[6].p3.x = x;
  bez[6].p3.y = y + h - h * AADL_ROUNDEDBOX_CORNER_SIZE_FACTOR;

  bez[7].p1.x = bez[8].p1.x = bez[8].p2.x = x;
  bez[7].p1.y = y + h * AADL_ROUNDEDBOX_CORNER_SIZE_FACTOR;
  bez[8].p1.y = bez[8].p2.y = bez[8].p3.y = y;
  bez[8].p3.x = x + w * AADL_ROUNDEDBOX_CORNER_SIZE_FACTOR;

  dia_renderer_set_fillstyle (renderer, DIA_FILL_STYLE_SOLID);
  dia_renderer_set_linewidth (renderer, AADLBOX_BORDERWIDTH);
  dia_renderer_set_linestyle (renderer, linestyle, AADLBOX_DASH_LENGTH);

  dia_renderer_draw_beziergon (renderer,
                               bez,
                               9,
                               &aadlbox->fill_color,
                               &aadlbox->line_color);
}


static void
aadlsystem_draw_borders (Aadlbox *aadlbox, DiaRenderer *renderer)
{
  aadlbox_draw_rounded_box (aadlbox, renderer, DIA_LINE_STYLE_SOLID);
}


static Aadlbox_specific aadlsystem_specific =
{
  (AadlProjectionFunc) aadldata_project_point_on_nearest_border,
  (AadlTextPosFunc)    aadlsystem_text_position,
  (AadlSizeFunc) aadlsystem_minsize
};

static void aadlsystem_draw(Aadlbox *aadlbox, DiaRenderer *renderer)
{
  aadlsystem_draw_borders(aadlbox, renderer);
  aadlbox_draw(aadlbox, renderer);
}

ObjectTypeOps aadlsystem_type_ops;

DiaObjectType aadlsystem_type =
{
  "AADL - System",           /* name */
  0,                      /* version */
  aadlsystem_xpm,          /* pixmap */

  &aadlsystem_type_ops,       /* ops */
  NULL,
  &aadlsystem_specific, /* user data */
  NULL, /* prop_descs: property descriptions */
  NULL, /* prop_offsets: DiaObject struct offsets */
  DIA_OBJECT_CAN_PARENT /* flags */
};


static ObjectOps aadlsystem_ops =
{
  (DestroyFunc)         aadlbox_destroy,
  (DrawFunc)            aadlsystem_draw,              /* redefined */
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



static DiaObject *aadlsystem_create(Point *startpoint, void *user_data, Handle **handle1, Handle **handle2)
{
  DiaObject *obj = aadlbox_create(startpoint, user_data, handle1, handle2);

  obj->type = &aadlsystem_type;
  obj->ops  = &aadlsystem_ops;

  return obj;
}

static DiaObject *
aadlsystem_load(ObjectNode obj_node, int version, DiaContext *ctx)
{
  DiaObject *obj;
  Point startpoint = {0.0,0.0};
  Handle *handle1,*handle2;

  obj = aadlsystem_create(&startpoint,&aadlsystem_specific, &handle1,&handle2);
  aadlbox_load(obj_node, version, ctx, (Aadlbox *) obj);
  return obj;
}


ObjectTypeOps aadlsystem_type_ops =
{
  (CreateFunc) aadlsystem_create,
  (LoadFunc)   aadlsystem_load,/*using_properties*/     /* load */
  (SaveFunc)   aadlbox_save,      /* save */
  (GetDefaultsFunc)   NULL,
  (ApplyDefaultsFunc) NULL
};
