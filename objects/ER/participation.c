/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
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
#include <assert.h>
#include <gtk/gtk.h>
#include <math.h>
#include <string.h>

#include "object.h"
#include "orth_conn.h"
#include "render.h"
#include "attributes.h"
#include "sheet.h"
#include "arrows.h"

#include "pixmaps/participation.xpm"

#define WIDTH 0.4

typedef struct _Participation Participation;
typedef struct _ParticipationPropertiesDialog ParticipationPropertiesDialog;

struct _Participation {
  OrthConn orth;

  gboolean total;

  ParticipationPropertiesDialog* properties_dialog;
};

struct _ParticipationPropertiesDialog {
  GtkWidget *dialog;
  
  GtkToggleButton *total;
};

#define PARTICIPATION_WIDTH 0.1
#define PARTICIPATION_DASHLEN 0.4
#define PARTICIPATION_FONTHEIGHT 0.8

static real participation_distance_from(Participation *dep, Point *point);
static void participation_select(Participation *dep, Point *clicked_point,
			      Renderer *interactive_renderer);
static void participation_move_handle(Participation *dep, Handle *handle,
				   Point *to, HandleMoveReason reason);
static void participation_move(Participation *dep, Point *to);
static void participation_draw(Participation *dep, Renderer *renderer);
static Object *participation_create(Point *startpoint,
				 void *user_data,
				 Handle **handle1,
				 Handle **handle2);
static void participation_destroy(Participation *dep);
static Object *participation_copy(Participation *dep);
static GtkWidget *participation_get_properties(Participation *dep);
static void participation_apply_properties(Participation *dep);
static void participation_save(Participation *dep, ObjectNode obj_node);
static Object *participation_load(ObjectNode obj_node, int version);

static void participation_update_data(Participation *dep);

static ObjectTypeOps participation_type_ops =
{
  (CreateFunc) participation_create,
  (LoadFunc)   participation_load,
  (SaveFunc)   participation_save
};

ObjectType participation_type =
{
  "ER - Participation",   /* name */
  0,                      /* version */
  (char **) participation_xpm,  /* pixmap */
  
  &participation_type_ops       /* ops */
};

SheetObject participation_sheetobj =
{
  "ER - Participation",             /* type */
  "Create a participation",           /* description */
  (char **) participation_xpm,     /* pixmap */

  NULL                        /* user_data */
};

static ObjectOps participation_ops = {
  (DestroyFunc)         participation_destroy,
  (DrawFunc)            participation_draw,
  (DistanceFunc)        participation_distance_from,
  (SelectFunc)          participation_select,
  (CopyFunc)            participation_copy,
  (MoveFunc)            participation_move,
  (MoveHandleFunc)      participation_move_handle,
  (GetPropertiesFunc)   participation_get_properties,
  (ApplyPropertiesFunc) participation_apply_properties,
  (IsEmptyFunc)         object_return_false
};

static real
participation_distance_from(Participation *participation, Point *point)
{
  OrthConn *orth = &participation->orth;
  return orthconn_distance_from(orth, point, PARTICIPATION_WIDTH);
}

static void
participation_select(Participation *participation, Point *clicked_point,
		  Renderer *interactive_renderer)
{
  orthconn_update_data(&participation->orth);
}

static void
participation_move_handle(Participation *participation, Handle *handle,
		       Point *to, HandleMoveReason reason)
{
  assert(participation!=NULL);
  assert(handle!=NULL);
  assert(to!=NULL);
  
  orthconn_move_handle(&participation->orth, handle, to, reason);
  participation_update_data(participation);
}

static void
participation_move(Participation *participation, Point *to)
{
  orthconn_move(&participation->orth, to);
  participation_update_data(participation);
}

static void
participation_draw(Participation *participation, Renderer *renderer)
{
  OrthConn *orth = &participation->orth;
  Point *points;
  Point *tpoints;
  int i, n;
  Point pos;
  
  points = &orth->points[0];
  n = orth->numpoints;
  
  renderer->ops->set_linewidth(renderer, PARTICIPATION_WIDTH);
  renderer->ops->set_linestyle(renderer, LINESTYLE_SOLID);
  renderer->ops->set_linejoin(renderer, LINEJOIN_MITER);
  renderer->ops->set_linecaps(renderer, LINECAPS_BUTT);

  if(participation->total) {
    tpoints = g_new(Point, n * 2);
    for(i = 0; i < n - 1; i++) {
      if(orth->orientation[i] == VERTICAL) {
	tpoints[i].x = points[i].x - WIDTH / 2.0;
	tpoints[i].y = points[i].y;
	tpoints[i + n].x = points[i].x + WIDTH / 2.0;
	tpoints[i + n].y = points[i].y;
      }
      else {
	tpoints[i].x = points[i].x;
	tpoints[i].y = points[i].y - WIDTH / 2.0;
	tpoints[i + n].x = points[i].x;
	tpoints[i + n].y = points[i].y + WIDTH / 2.0;
      }
    }
    tpoints[n - 1].x = points[n - 1].x;
    tpoints[n - 1].y = points[n - 1].y;
    tpoints[2 * n - 1].x = points[2 * n - 1].x;
    tpoints[2 * n - 1].y = points[2 * n - 1].y;
    renderer->ops->draw_polyline(renderer, tpoints, n, &color_black);
    renderer->ops->draw_polyline(renderer, &tpoints[n], n, &color_black);
    g_free(tpoints);
  }
  else 
    renderer->ops->draw_polyline(renderer, points, n, &color_black);
}

static void
participation_update_data(Participation *participation)
{
  OrthConn *orth = &participation->orth;
  Object *obj = (Object *) participation;
  
  orthconn_update_data(orth);
  
  orthconn_update_boundingbox(orth);
  /* fix boundingparticipation for linewidth */
  obj->bounding_box.top -= PARTICIPATION_WIDTH/2.0;
  obj->bounding_box.left -= PARTICIPATION_WIDTH/2.0;
  obj->bounding_box.bottom += PARTICIPATION_WIDTH/2.0;
  obj->bounding_box.right += PARTICIPATION_WIDTH/2.0;
  
  obj->position = orth->points[0];
}

static Object *
participation_create(Point *startpoint,
	       void *user_data,
  	       Handle **handle1,
	       Handle **handle2)
{
  Participation *participation;
  OrthConn *orth;
  Object *obj;
  
  participation = g_malloc(sizeof(Participation));
  orth = &participation->orth;
  obj = (Object *) participation;
  
  obj->type = &participation_type;

  obj->ops = &participation_ops;

  orthconn_init(orth, startpoint);

  participation_update_data(participation);

  participation->total = FALSE;
  
  participation->properties_dialog = NULL;
  
  *handle1 = &orth->endpoint_handles[0];
  *handle2 = &orth->endpoint_handles[1];

  return (Object *)participation;
}

static void
participation_destroy(Participation *participation)
{
  if (participation->properties_dialog != NULL) {
    gtk_widget_destroy(participation->properties_dialog->dialog);
    g_free(participation->properties_dialog);
  }

  orthconn_destroy(&participation->orth);
}

static Object *
participation_copy(Participation *participation)
{
  Participation *newparticipation;
  OrthConn *orth, *neworth;
  Object *newobj;
  
  orth = &participation->orth;
  
  newparticipation = g_malloc(sizeof(Participation));
  neworth = &newparticipation->orth;
  newobj = (Object *) newparticipation;

  orthconn_copy(orth, neworth);

  newparticipation->total = participation->total;
  newparticipation->properties_dialog = NULL;
  
  participation_update_data(newparticipation);
  
  return (Object *)newparticipation;
}


static void
participation_save(Participation *participation, ObjectNode obj_node)
{
  orthconn_save(&participation->orth, obj_node);

  data_add_boolean(new_attribute(obj_node, "total"),
		   participation->total);
}

static Object *
participation_load(ObjectNode obj_node, int version)
{
  AttributeNode attr;
  Participation *participation;
  OrthConn *orth;
  Object *obj;

  participation = g_new(Participation, 1);

  orth = &participation->orth;
  obj = (Object *) participation;

  obj->type = &participation_type;
  obj->ops = &participation_ops;

  orthconn_load(orth, obj_node);

  attr = object_find_attribute(obj_node, "total");
  if (attr != NULL)
    participation->total = data_boolean(attribute_first_data(attr));

  participation->properties_dialog = NULL;
      
  participation_update_data(participation);

  return (Object *)participation;
}


static void
participation_apply_properties(Participation *participation)
{
  ParticipationPropertiesDialog *prop_dialog;
  char *str;

  prop_dialog = participation->properties_dialog;

  /* Read from dialog and put in object: */
  
  participation->total = prop_dialog->total->active;

  participation_update_data(participation);
}

static GtkWidget *
participation_get_properties(Participation *participation)
{
  ParticipationPropertiesDialog *prop_dialog;
  GtkWidget *dialog;
  GtkWidget *checkbox;
  GtkWidget *entry;
  GtkWidget *hbox;
  GtkWidget *label;

  if (participation->properties_dialog == NULL) {
    prop_dialog = g_new(ParticipationPropertiesDialog, 1);
    participation->properties_dialog = prop_dialog;

    dialog = gtk_vbox_new(FALSE, 0);
    prop_dialog->dialog = dialog;

    hbox = gtk_hbox_new(FALSE, 5);
    checkbox = gtk_check_button_new_with_label("Total:");
    prop_dialog->total = GTK_TOGGLE_BUTTON(checkbox);
    gtk_box_pack_start (GTK_BOX (hbox),	checkbox, TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (dialog), hbox, TRUE, TRUE, 0);
  }
  gtk_widget_show_all (dialog);

  gtk_toggle_button_set_state(prop_dialog->total, participation->total);

  return dialog;
}




