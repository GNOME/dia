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
#include "pixmaps/aadlthread.xpm"

/***********************************************
 **               AADL THREAD             **
 ***********************************************/
/* same as process */

static void
aadlthread_draw_borders (Aadlbox *aadlbox, DiaRenderer *renderer)
{
  aadlbox_draw_inclined_box (aadlbox, renderer, DIA_LINE_STYLE_DASHED);
}


static void
aadlthread_draw (Aadlbox *aadlbox, DiaRenderer *renderer)
{
  aadlthread_draw_borders (aadlbox, renderer);
  aadlbox_draw (aadlbox, renderer);
}

static Aadlbox_specific aadlthread_specific =
{
  (AadlProjectionFunc) aadlbox_inclined_project_point_on_nearest_border,
  (AadlTextPosFunc)    aadlprocess_text_position,
  (AadlSizeFunc) aadlprocess_minsize
};


ObjectTypeOps aadlthread_type_ops;

DiaObjectType aadlthread_type =
{
  "AADL - Thread",          /* name */
  0,                     /* version */
  aadlthread_xpm,         /* pixmap */

  &aadlthread_type_ops,      /* ops */
  NULL,
  &aadlthread_specific, /* user data */
  NULL, /* prop_descs: property descriptions */
  NULL, /* prop_offsets: DiaObject struct offsets */
  DIA_OBJECT_CAN_PARENT /* flags */
};


static ObjectOps aadlthread_ops =
{
  (DestroyFunc)         aadlbox_destroy,
  (DrawFunc)            aadlthread_draw,              /* redefined */
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



static DiaObject *aadlthread_create(Point *startpoint, void *user_data, Handle **handle1, Handle **handle2)
{
  DiaObject *obj = aadlbox_create(startpoint, user_data, handle1, handle2);

  obj->type = &aadlthread_type;
  obj->ops  = &aadlthread_ops;

  return obj;
}

static DiaObject *
aadlthread_load(ObjectNode obj_node, int version, DiaContext *ctx)
{
  DiaObject *obj;
  Point startpoint = {0.0,0.0};
  Handle *handle1,*handle2;

  obj = aadlthread_create(&startpoint,&aadlthread_specific, &handle1,&handle2);
  aadlbox_load(obj_node, version, ctx, (Aadlbox *) obj);
  return obj;
}


ObjectTypeOps aadlthread_type_ops =
{
  (CreateFunc) aadlthread_create,
  (LoadFunc)   aadlthread_load,/*using_properties*/     /* load */
  (SaveFunc)   aadlbox_save,      /* save */
  (GetDefaultsFunc)   NULL,
  (ApplyDefaultsFunc) NULL
};
