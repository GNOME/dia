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
#include "files.h"
#include "sheet.h"
#include "arrows.h"

#include "uml.h"

#include "pixmaps/generalization.xpm"

typedef struct _Generalization Generalization;
typedef struct _GeneralizationPropertiesDialog GeneralizationPropertiesDialog;

struct _Generalization {
  OrthConn orth;

  Point text_pos;
  Alignment text_align;
  real text_width;
  
  char *name;
  char *stereotype; /* including << and >> */

  GeneralizationPropertiesDialog* properties_dialog;
};

struct _GeneralizationPropertiesDialog {
  GtkWidget *dialog;
  
  GtkEntry *name;
  GtkEntry *stereotype;
};

#define GENERALIZATION_WIDTH 0.1
#define GENERALIZATION_TRIANGLESIZE 0.8
#define GENERALIZATION_FONTHEIGHT 0.8

static Font *genlz_font = NULL;

static real generalization_distance_from(Generalization *genlz, Point *point);
static void generalization_select(Generalization *genlz, Point *clicked_point,
			      Renderer *interactive_renderer);
static void generalization_move_handle(Generalization *genlz, Handle *handle,
				   Point *to, HandleMoveReason reason);
static void generalization_move(Generalization *genlz, Point *to);
static void generalization_draw(Generalization *genlz, Renderer *renderer);
static Object *generalization_create(Point *startpoint,
				 void *user_data,
				 Handle **handle1,
				 Handle **handle2);
static void generalization_destroy(Generalization *genlz);
static Object *generalization_copy(Generalization *genlz);
static GtkWidget *generalization_get_properties(Generalization *genlz);
static void generalization_apply_properties(Generalization *genlz);

static void generalization_save(Generalization *genlz, ObjectNode obj_node);
static Object *generalization_load(ObjectNode obj_node, int version);

static void generalization_update_data(Generalization *genlz);

static ObjectTypeOps generalization_type_ops =
{
  (CreateFunc) generalization_create,
  (LoadFunc)   generalization_load,
  (SaveFunc)   generalization_save
};

ObjectType generalization_type =
{
  "UML - Generalization",   /* name */
  0,                      /* version */
  (char **) generalization_xpm,  /* pixmap */
  
  &generalization_type_ops       /* ops */
};

SheetObject generalization_sheetobj =
{
  "UML - Generalization",             /* type */
  "Generalization, class inheritance.",
                              /* description */
  (char **) generalization_xpm,     /* pixmap */

  NULL                        /* user_data */
};

static ObjectOps generalization_ops = {
  (DestroyFunc)         generalization_destroy,
  (DrawFunc)            generalization_draw,
  (DistanceFunc)        generalization_distance_from,
  (SelectFunc)          generalization_select,
  (CopyFunc)            generalization_copy,
  (MoveFunc)            generalization_move,
  (MoveHandleFunc)      generalization_move_handle,
  (GetPropertiesFunc)   generalization_get_properties,
  (ApplyPropertiesFunc) generalization_apply_properties,
  (IsEmptyFunc)         object_return_false
};

static real
generalization_distance_from(Generalization *genlz, Point *point)
{
  OrthConn *orth = &genlz->orth;
  return orthconn_distance_from(orth, point, GENERALIZATION_WIDTH);
}

static void
generalization_select(Generalization *genlz, Point *clicked_point,
		  Renderer *interactive_renderer)
{
  orthconn_update_data(&genlz->orth);
}

static void
generalization_move_handle(Generalization *genlz, Handle *handle,
		       Point *to, HandleMoveReason reason)
{
  assert(genlz!=NULL);
  assert(handle!=NULL);
  assert(to!=NULL);
  
  orthconn_move_handle(&genlz->orth, handle, to, reason);
  generalization_update_data(genlz);
}

static void
generalization_move(Generalization *genlz, Point *to)
{
  orthconn_move(&genlz->orth, to);
  generalization_update_data(genlz);
}

static void
generalization_draw(Generalization *genlz, Renderer *renderer)
{
  OrthConn *orth = &genlz->orth;
  Point *points;
  int n;
  Point pos;
  
  points = &orth->points[0];
  n = orth->numpoints;
  
  renderer->ops->set_linewidth(renderer, GENERALIZATION_WIDTH);
  renderer->ops->set_linestyle(renderer, LINESTYLE_SOLID);
  renderer->ops->set_linejoin(renderer, LINEJOIN_MITER);
  renderer->ops->set_linecaps(renderer, LINECAPS_BUTT);

  renderer->ops->draw_polyline(renderer, points, n, &color_black);

  arrow_draw(renderer, ARROW_HOLLOW_TRIANGLE,
	     &points[0], &points[1],
	     GENERALIZATION_TRIANGLESIZE, GENERALIZATION_TRIANGLESIZE,
	     GENERALIZATION_WIDTH,
	     &color_black, &color_white);

  renderer->ops->set_font(renderer, genlz_font, GENERALIZATION_FONTHEIGHT);
  pos = genlz->text_pos;
  
  if (genlz->stereotype != NULL) {
    renderer->ops->draw_string(renderer,
			       genlz->stereotype,
			       &pos, genlz->text_align,
			       &color_black);

    pos.y += GENERALIZATION_FONTHEIGHT;
  }
  
  if (genlz->name != NULL) {
    renderer->ops->draw_string(renderer,
			       genlz->name,
			       &pos, genlz->text_align,
			       &color_black);
  }
  
}

static void
generalization_update_data(Generalization *genlz)
{
  OrthConn *orth = &genlz->orth;
  Object *obj = (Object *) genlz;
  int num_segm, i;
  Point *points;
  Rectangle rect;
  
  orthconn_update_data(orth);
  
  orthconn_update_boundingbox(orth);
  /* fix boundinggeneralization for linewidth and triangle: */
  obj->bounding_box.top -= GENERALIZATION_WIDTH/2.0 + GENERALIZATION_TRIANGLESIZE;
  obj->bounding_box.left -= GENERALIZATION_WIDTH/2.0 + GENERALIZATION_TRIANGLESIZE;
  obj->bounding_box.bottom += GENERALIZATION_WIDTH/2.0 + GENERALIZATION_TRIANGLESIZE;
  obj->bounding_box.right += GENERALIZATION_WIDTH/2.0 + GENERALIZATION_TRIANGLESIZE;
  
  obj->position = orth->points[0];

  /* Calc text pos: */
  num_segm = genlz->orth.numpoints - 1;
  points = genlz->orth.points;
  i = num_segm / 2;
  
  if ((num_segm % 2) == 0) { /* If no middle segment, use horizontal */
    if (genlz->orth.orientation[i]==VERTICAL)
      i--;
  }

  switch (genlz->orth.orientation[i]) {
  case HORIZONTAL:
    genlz->text_align = ALIGN_CENTER;
    genlz->text_pos.x = 0.5*(points[i].x+points[i+1].x);
    genlz->text_pos.y = points[i].y - font_descent(genlz_font, GENERALIZATION_FONTHEIGHT);
    break;
  case VERTICAL:
    genlz->text_align = ALIGN_LEFT;
    genlz->text_pos.x = points[i].x + 0.1;
    genlz->text_pos.y =
      0.5*(points[i].y+points[i+1].y) - font_descent(genlz_font, GENERALIZATION_FONTHEIGHT);
    break;
  }

  /* Add the text recangle to the bounding box: */
  rect.left = genlz->text_pos.x;
  if (genlz->text_align == ALIGN_CENTER)
    rect.left -= genlz->text_width/2.0;
  rect.right = rect.left + genlz->text_width;
  rect.top = genlz->text_pos.y - font_ascent(genlz_font, GENERALIZATION_FONTHEIGHT);
  rect.bottom = rect.top + 2*GENERALIZATION_FONTHEIGHT;

  rectangle_union(&obj->bounding_box, &rect);
}

static Object *
generalization_create(Point *startpoint,
	       void *user_data,
  	       Handle **handle1,
	       Handle **handle2)
{
  Generalization *genlz;
  OrthConn *orth;
  Object *obj;

  if (genlz_font == NULL) {
    genlz_font = font_getfont("Courier");
  }
  
  genlz = g_malloc(sizeof(Generalization));
  orth = &genlz->orth;
  obj = (Object *) genlz;
  
  obj->type = &generalization_type;

  obj->ops = &generalization_ops;

  orthconn_init(orth, startpoint);

  generalization_update_data(genlz);

  genlz->name = NULL;
  genlz->stereotype = NULL;

  genlz->text_width = 0;
  
  genlz->properties_dialog = NULL;
  
  *handle1 = &orth->endpoint_handles[0];
  *handle2 = &orth->endpoint_handles[1];

  return (Object *)genlz;
}

static void
generalization_destroy(Generalization *genlz)
{
  orthconn_destroy(&genlz->orth);

  if (genlz->properties_dialog != NULL) {
    gtk_widget_destroy(genlz->properties_dialog->dialog);
    g_free(genlz->properties_dialog);
  }
}

static Object *
generalization_copy(Generalization *genlz)
{
  Generalization *newgenlz;
  OrthConn *orth, *neworth;
  Object *newobj;
  
  orth = &genlz->orth;
  
  newgenlz = g_malloc(sizeof(Generalization));
  neworth = &newgenlz->orth;
  newobj = (Object *) newgenlz;

  orthconn_copy(orth, neworth);

  newgenlz->name = (genlz->name != NULL)? strdup(genlz->name):NULL;
  newgenlz->stereotype = (genlz->stereotype != NULL)? strdup(genlz->stereotype):NULL;
  newgenlz->text_width = genlz->text_width;
  newgenlz->properties_dialog = NULL;
  
  generalization_update_data(newgenlz);
  
  return (Object *)newgenlz;
}


static void
generalization_save(Generalization *genlz, ObjectNode obj_node)
{
  orthconn_save(&genlz->orth, obj_node);

  data_add_string(new_attribute(obj_node, "name"),
		  genlz->name);
  data_add_string(new_attribute(obj_node, "stereotype"),
		  genlz->stereotype);
}

static Object *
generalization_load(ObjectNode obj_node, int version)
{
  Generalization *genlz;
  AttributeNode attr;
  OrthConn *orth;
  Object *obj;

  if (genlz_font == NULL) {
    genlz_font = font_getfont("Courier");
  }

  genlz = g_new(Generalization, 1);

  orth = &genlz->orth;
  obj = (Object *) genlz;

  obj->type = &generalization_type;
  obj->ops = &generalization_ops;

  orthconn_load(orth, obj_node);

  genlz->name = NULL;
  attr = object_find_attribute(obj_node, "name");
  if (attr != NULL)
    genlz->name = data_string(attribute_first_data(attr));
  
  genlz->stereotype = NULL;
  attr = object_find_attribute(obj_node, "stereotype");
  if (attr != NULL)
    genlz->stereotype = data_string(attribute_first_data(attr));

  genlz->text_width = 0.0;

  genlz->properties_dialog = NULL;
  
  if (genlz->name != NULL) {
    genlz->text_width =
      font_string_width(genlz->name, genlz_font, GENERALIZATION_FONTHEIGHT);
  }
  if (genlz->stereotype != NULL) {
    genlz->text_width = MAX(genlz->text_width,
			  font_string_width(genlz->stereotype, genlz_font, GENERALIZATION_FONTHEIGHT));
  }
  
  generalization_update_data(genlz);

  return (Object *)genlz;
}

static void
generalization_apply_properties(Generalization *genlz)
{
  GeneralizationPropertiesDialog *prop_dialog;
  char *str;

  prop_dialog = genlz->properties_dialog;

  /* Read from dialog and put in object: */
  if (genlz->name != NULL)
    g_free(genlz->name);
  str = gtk_entry_get_text(prop_dialog->name);
  if (strlen(str) != 0)
    genlz->name = strdup(str);
  else
    genlz->name = NULL;

  if (genlz->stereotype != NULL)
    g_free(genlz->stereotype);
  
  str = gtk_entry_get_text(prop_dialog->stereotype);
  
  if (strlen(str) != 0) {
    genlz->stereotype = g_malloc(sizeof(char)*strlen(str)+2+1);
    genlz->stereotype[0] = UML_STEREOTYPE_START;
    genlz->stereotype[1] = 0;
    strcat(genlz->stereotype, str);
    genlz->stereotype[strlen(str)+1] = UML_STEREOTYPE_END;
    genlz->stereotype[strlen(str)+2] = 0;
  } else {
    genlz->stereotype = NULL;
  }

  genlz->text_width = 0.0;

  if (genlz->name != NULL) {
    genlz->text_width =
      font_string_width(genlz->name, genlz_font, GENERALIZATION_FONTHEIGHT);
  }
  if (genlz->stereotype != NULL) {
    genlz->text_width = MAX(genlz->text_width,
			  font_string_width(genlz->stereotype, genlz_font, GENERALIZATION_FONTHEIGHT));
  }
  
  generalization_update_data(genlz);
}

static void
fill_in_dialog(Generalization *genlz)
{
  GeneralizationPropertiesDialog *prop_dialog;
  char *str;
  
  prop_dialog = genlz->properties_dialog;

  if (genlz->name != NULL)
    gtk_entry_set_text(prop_dialog->name, genlz->name);
  else 
    gtk_entry_set_text(prop_dialog->name, "");
  
  
  if (genlz->stereotype != NULL) {
    str = strdup(genlz->stereotype);
    strcpy(str, genlz->stereotype+1);
    str[strlen(str)-1] = 0;
    gtk_entry_set_text(prop_dialog->stereotype, str);
    g_free(str);
  } else {
    gtk_entry_set_text(prop_dialog->stereotype, "");
  }
}

static GtkWidget *
generalization_get_properties(Generalization *genlz)
{
  GeneralizationPropertiesDialog *prop_dialog;
  GtkWidget *dialog;
  GtkWidget *entry;
  GtkWidget *hbox;
  GtkWidget *label;

  if (genlz->properties_dialog == NULL) {

    prop_dialog = g_new(GeneralizationPropertiesDialog, 1);
    genlz->properties_dialog = prop_dialog;

    dialog = gtk_vbox_new(FALSE, 0);
    prop_dialog->dialog = dialog;
    
    hbox = gtk_hbox_new(FALSE, 5);

    label = gtk_label_new("Name:");
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    entry = gtk_entry_new();
    prop_dialog->name = GTK_ENTRY(entry);
    gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, TRUE, 0);
    gtk_widget_show (label);
    gtk_widget_show (entry);
    gtk_box_pack_start (GTK_BOX (dialog), hbox, TRUE, TRUE, 0);
    gtk_widget_show(hbox);

    hbox = gtk_hbox_new(FALSE, 5);
    gtk_container_border_width (GTK_CONTAINER (hbox), 5);
    label = gtk_label_new("Stereotype:");
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    entry = gtk_entry_new();
    prop_dialog->stereotype = GTK_ENTRY(entry);
    gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, TRUE, 0);
    gtk_widget_show (label);
    gtk_widget_show (entry);
    gtk_box_pack_start (GTK_BOX (dialog), hbox, TRUE, TRUE, 0);
    gtk_widget_show(hbox);
  }
  
  fill_in_dialog(genlz);
  gtk_widget_show (genlz->properties_dialog->dialog);

  return genlz->properties_dialog->dialog;
}




