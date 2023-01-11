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
#include "pixmaps/aadlthreadgroup.xpm"

/***********************************************
 **                 AADL THREADGROUP               **
 ***********************************************/


#define AADL_THREADGROUP_CORNER_SIZE 2
#define AADL_THREADGROUP_CORNER_SIZE_FACTOR 0.25


static void
aadlthreadgroup_draw_borders (Aadlbox *aadlbox, DiaRenderer *renderer)
{
  aadlbox_draw_rounded_box (aadlbox, renderer, DIA_LINE_STYLE_DASHED);
}


static Aadlbox_specific aadlthreadgroup_specific = {
  (AadlProjectionFunc) aadldata_project_point_on_nearest_border,
  (AadlTextPosFunc)    aadldata_text_position,
  (AadlSizeFunc) aadldata_minsize
};


static void aadlthreadgroup_draw(Aadlbox *aadlbox, DiaRenderer *renderer)
{
  aadlthreadgroup_draw_borders(aadlbox, renderer);
  aadlbox_draw(aadlbox, renderer);
}

ObjectTypeOps aadlthreadgroup_type_ops;

DiaObjectType aadlthreadgroup_type =
{
  "AADL - Thread Group",          /* name */
  0,                           /* version */
  aadlthreadgroup_xpm,          /* pixmap */
  &aadlthreadgroup_type_ops,       /* ops */
  NULL,
  &aadlthreadgroup_specific, /* user data */
  NULL, /* prop_descs: property descriptions */
  NULL, /* prop_offsets: DiaObject struct offsets */
  DIA_OBJECT_CAN_PARENT /* flags */
};


static ObjectOps aadlthreadgroup_ops =
{
  (DestroyFunc)         aadlbox_destroy,
  (DrawFunc)            aadlthreadgroup_draw,              /* redefined */
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



static DiaObject *aadlthreadgroup_create(Point *startpoint, void *user_data, Handle **handle1, Handle **handle2)
{
  DiaObject *obj = aadlbox_create(startpoint, user_data, handle1, handle2);

  obj->type = &aadlthreadgroup_type;
  obj->ops  = &aadlthreadgroup_ops;

  return obj;
}

static DiaObject *
aadlthreadgroup_load(ObjectNode obj_node, int version, DiaContext *ctx)
{
  DiaObject *obj;
  Point startpoint = {0.0,0.0};
  Handle *handle1,*handle2;

  obj = aadlthreadgroup_create(&startpoint, &aadlthreadgroup_specific,
			       &handle1,&handle2);
  aadlbox_load(obj_node, version, ctx, (Aadlbox *) obj);
  return obj;
}


ObjectTypeOps aadlthreadgroup_type_ops =
{
  (CreateFunc) aadlthreadgroup_create,
  (LoadFunc)   aadlthreadgroup_load,/*using_properties*/     /* load */
  (SaveFunc)   aadlbox_save,      /* save */
  (GetDefaultsFunc)   NULL,
  (ApplyDefaultsFunc) NULL
};
