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

#include "uml.h"

#include "pixmaps/dependency.xpm"

typedef struct _Dependency Dependency;
typedef struct _DependencyPropertiesDialog DependencyPropertiesDialog;


struct _Dependency {
  OrthConn orth;

  Point text_pos;
  Alignment text_align;
  real text_width;
  
  int draw_arrow;
  char *name;
  char *stereotype; /* including << and >> */

  DependencyPropertiesDialog* properties_dialog;
};

struct _DependencyPropertiesDialog {
  GtkWidget *dialog;
  
  GtkEntry *name;
  GtkEntry *stereotype;
  GtkToggleButton *draw_arrow;
};

#define DEPENDENCY_WIDTH 0.1
#define DEPENDENCY_ARROWLEN 0.8
#define DEPENDENCY_ARROWWIDTH 0.5
#define DEPENDENCY_DASHLEN 0.4
#define DEPENDENCY_FONTHEIGHT 0.8

static Font *dep_font = NULL;

static real dependency_distance_from(Dependency *dep, Point *point);
static void dependency_select(Dependency *dep, Point *clicked_point,
			      Renderer *interactive_renderer);
static void dependency_move_handle(Dependency *dep, Handle *handle,
				   Point *to, HandleMoveReason reason);
static void dependency_move(Dependency *dep, Point *to);
static void dependency_draw(Dependency *dep, Renderer *renderer);
static Object *dependency_create(Point *startpoint,
				 void *user_data,
				 Handle **handle1,
				 Handle **handle2);
static void dependency_destroy(Dependency *dep);
static Object *dependency_copy(Dependency *dep);
static GtkWidget *dependency_get_properties(Dependency *dep);
static void dependency_apply_properties(Dependency *dep);
static void dependency_save(Dependency *dep, ObjectNode obj_node);
static Object *dependency_load(ObjectNode obj_node, int version);

static void dependency_update_data(Dependency *dep);

static ObjectTypeOps dependency_type_ops =
{
  (CreateFunc) dependency_create,
  (LoadFunc)   dependency_load,
  (SaveFunc)   dependency_save
};

ObjectType dependency_type =
{
  "UML - Dependency",   /* name */
  0,                      /* version */
  (char **) dependency_xpm,  /* pixmap */
  
  &dependency_type_ops       /* ops */
};

SheetObject dependency_sheetobj =
{
  "UML - Dependency",             /* type */
  "Create a dependency",           /* description */
  (char **) dependency_xpm,     /* pixmap */

  NULL                        /* user_data */
};

static ObjectOps dependency_ops = {
  (DestroyFunc)         dependency_destroy,
  (DrawFunc)            dependency_draw,
  (DistanceFunc)        dependency_distance_from,
  (SelectFunc)          dependency_select,
  (CopyFunc)            dependency_copy,
  (MoveFunc)            dependency_move,
  (MoveHandleFunc)      dependency_move_handle,
  (GetPropertiesFunc)   dependency_get_properties,
  (ApplyPropertiesFunc) dependency_apply_properties,
  (IsEmptyFunc)         object_return_false
};

static real
dependency_distance_from(Dependency *dep, Point *point)
{
  OrthConn *orth = &dep->orth;
  return orthconn_distance_from(orth, point, DEPENDENCY_WIDTH);
}

static void
dependency_select(Dependency *dep, Point *clicked_point,
		  Renderer *interactive_renderer)
{
  orthconn_update_data(&dep->orth);
}

static void
dependency_move_handle(Dependency *dep, Handle *handle,
		       Point *to, HandleMoveReason reason)
{
  assert(dep!=NULL);
  assert(handle!=NULL);
  assert(to!=NULL);
  
  orthconn_move_handle(&dep->orth, handle, to, reason);
  dependency_update_data(dep);
}

static void
dependency_move(Dependency *dep, Point *to)
{
  orthconn_move(&dep->orth, to);
  dependency_update_data(dep);
}

static void
dependency_draw(Dependency *dep, Renderer *renderer)
{
  OrthConn *orth = &dep->orth;
  Point *points;
  int n;
  Point pos;
  
  points = &orth->points[0];
  n = orth->numpoints;
  
  renderer->ops->set_linewidth(renderer, DEPENDENCY_WIDTH);
  renderer->ops->set_linestyle(renderer, LINESTYLE_DASHED);
  renderer->ops->set_dashlength(renderer, DEPENDENCY_DASHLEN);
  renderer->ops->set_linejoin(renderer, LINEJOIN_MITER);
  renderer->ops->set_linecaps(renderer, LINECAPS_BUTT);

  renderer->ops->draw_polyline(renderer, points, n, &color_black);

  if (dep->draw_arrow)
    arrow_draw(renderer, ARROW_LINES,
	       &points[n-1], &points[n-2],
	       DEPENDENCY_ARROWLEN, DEPENDENCY_ARROWWIDTH, DEPENDENCY_WIDTH,
	       &color_black, &color_white);

  renderer->ops->set_font(renderer, dep_font, DEPENDENCY_FONTHEIGHT);
  pos = dep->text_pos;
  
  if (dep->stereotype != NULL) {
    renderer->ops->draw_string(renderer,
			       dep->stereotype,
			       &pos, dep->text_align,
			       &color_black);

    pos.y += DEPENDENCY_FONTHEIGHT;
  }
  
  if (dep->name != NULL) {
    renderer->ops->draw_string(renderer,
			       dep->name,
			       &pos, dep->text_align,
			       &color_black);
  }
  
}

static void
dependency_update_data(Dependency *dep)
{
  OrthConn *orth = &dep->orth;
  Object *obj = (Object *) dep;
  int num_segm, i;
  Point *points;
  Rectangle rect;
  
  orthconn_update_data(orth);
  
  orthconn_update_boundingbox(orth);
  /* fix boundingdependency for linewidth and arrow: */
  obj->bounding_box.top -= DEPENDENCY_WIDTH/2.0 + DEPENDENCY_ARROWLEN;
  obj->bounding_box.left -= DEPENDENCY_WIDTH/2.0 + DEPENDENCY_ARROWLEN;
  obj->bounding_box.bottom += DEPENDENCY_WIDTH/2.0 + DEPENDENCY_ARROWLEN;
  obj->bounding_box.right += DEPENDENCY_WIDTH/2.0 + DEPENDENCY_ARROWLEN;
  
  obj->position = orth->points[0];

  /* Calc text pos: */
  num_segm = dep->orth.numpoints - 1;
  points = dep->orth.points;
  i = num_segm / 2;
  
  if ((num_segm % 2) == 0) { /* If no middle segment, use horizontal */
    if (dep->orth.orientation[i]==VERTICAL)
      i--;
  }

  switch (dep->orth.orientation[i]) {
  case HORIZONTAL:
    dep->text_align = ALIGN_CENTER;
    dep->text_pos.x = 0.5*(points[i].x+points[i+1].x);
    dep->text_pos.y = points[i].y - font_descent(dep_font, DEPENDENCY_FONTHEIGHT);
    break;
  case VERTICAL:
    dep->text_align = ALIGN_LEFT;
    dep->text_pos.x = points[i].x + 0.1;
    dep->text_pos.y =
      0.5*(points[i].y+points[i+1].y) - font_descent(dep_font, DEPENDENCY_FONTHEIGHT);
    break;
  }

  /* Add the text recangle to the bounding box: */
  rect.left = dep->text_pos.x;
  if (dep->text_align == ALIGN_CENTER)
    rect.left -= dep->text_width/2.0;
  rect.right = rect.left + dep->text_width;
  rect.top = dep->text_pos.y - font_ascent(dep_font, DEPENDENCY_FONTHEIGHT);
  rect.bottom = rect.top + 2*DEPENDENCY_FONTHEIGHT;

  rectangle_union(&obj->bounding_box, &rect);
}

static Object *
dependency_create(Point *startpoint,
	       void *user_data,
  	       Handle **handle1,
	       Handle **handle2)
{
  Dependency *dep;
  OrthConn *orth;
  Object *obj;

  if (dep_font == NULL) {
    dep_font = font_getfont("Courier");
  }
  
  dep = g_malloc(sizeof(Dependency));
  orth = &dep->orth;
  obj = (Object *) dep;
  
  obj->type = &dependency_type;

  obj->ops = &dependency_ops;

  orthconn_init(orth, startpoint);

  dependency_update_data(dep);

  dep->draw_arrow = TRUE;
  dep->name = NULL;
  dep->stereotype = NULL;

  dep->text_width = 0;
  
  dep->properties_dialog = NULL;
  
  *handle1 = &orth->endpoint_handles[0];
  *handle2 = &orth->endpoint_handles[1];

  return (Object *)dep;
}

static void
dependency_destroy(Dependency *dep)
{
  if (dep->name != NULL)
    g_free(dep->name);

  if (dep->stereotype != NULL)
    g_free(dep->stereotype);
  
  if (dep->properties_dialog != NULL) {
    gtk_widget_destroy(dep->properties_dialog->dialog);
    g_free(dep->properties_dialog);
  }

  orthconn_destroy(&dep->orth);
}

static Object *
dependency_copy(Dependency *dep)
{
  Dependency *newdep;
  OrthConn *orth, *neworth;
  Object *newobj;
  
  orth = &dep->orth;
  
  newdep = g_malloc(sizeof(Dependency));
  neworth = &newdep->orth;
  newobj = (Object *) newdep;

  orthconn_copy(orth, neworth);

  newdep->name = (dep->name != NULL)? strdup(dep->name):NULL;
  newdep->stereotype = (dep->stereotype != NULL)? strdup(dep->stereotype):NULL;
  newdep->draw_arrow = dep->draw_arrow;
  newdep->text_width = dep->text_width;
  newdep->properties_dialog = NULL;
  
  dependency_update_data(newdep);
  
  return (Object *)newdep;
}


static void
dependency_save(Dependency *dep, ObjectNode obj_node)
{
  orthconn_save(&dep->orth, obj_node);

  data_add_boolean(new_attribute(obj_node, "draw_arrow"),
		   dep->draw_arrow);
  data_add_string(new_attribute(obj_node, "name"),
		  dep->name);
  data_add_string(new_attribute(obj_node, "stereotype"),
		  dep->stereotype);
}

static Object *
dependency_load(ObjectNode obj_node, int version)
{
  AttributeNode attr;
  Dependency *dep;
  OrthConn *orth;
  Object *obj;

  if (dep_font == NULL) {
    dep_font = font_getfont("Courier");
  }

  dep = g_new(Dependency, 1);

  orth = &dep->orth;
  obj = (Object *) dep;

  obj->type = &dependency_type;
  obj->ops = &dependency_ops;

  orthconn_load(orth, obj_node);

  attr = object_find_attribute(obj_node, "draw_arrow");
  if (attr != NULL)
    dep->draw_arrow = data_boolean(attribute_first_data(attr));

  dep->name = NULL;
  attr = object_find_attribute(obj_node, "name");
  if (attr != NULL)
    dep->name = data_string(attribute_first_data(attr));
  
  dep->stereotype = NULL;
  attr = object_find_attribute(obj_node, "stereotype");
  if (attr != NULL)
    dep->stereotype = data_string(attribute_first_data(attr));

  dep->text_width = 0.0;

  if (dep->name != NULL) {
    dep->text_width =
      font_string_width(dep->name, dep_font, DEPENDENCY_FONTHEIGHT);
  }
  if (dep->stereotype != NULL) {
    dep->text_width = MAX(dep->text_width,
			  font_string_width(dep->stereotype, dep_font, DEPENDENCY_FONTHEIGHT));
  }

  dep->properties_dialog = NULL;
      

  dependency_update_data(dep);

  return (Object *)dep;
}


static void
dependency_apply_properties(Dependency *dep)
{
  DependencyPropertiesDialog *prop_dialog;
  char *str;

  prop_dialog = dep->properties_dialog;

  /* Read from dialog and put in object: */
  if (dep->name != NULL)
    g_free(dep->name);
  str = gtk_entry_get_text(prop_dialog->name);
  if (strlen(str) != 0)
    dep->name = strdup(str);
  else
    dep->name = NULL;

  if (dep->stereotype != NULL)
    g_free(dep->stereotype);
  
  str = gtk_entry_get_text(prop_dialog->stereotype);
  
  if (strlen(str) != 0) {
    dep->stereotype = g_malloc(sizeof(char)*strlen(str)+2+1);
    dep->stereotype[0] = UML_STEREOTYPE_START;
    dep->stereotype[1] = 0;
    strcat(dep->stereotype, str);
    dep->stereotype[strlen(str)+1] = UML_STEREOTYPE_END;
    dep->stereotype[strlen(str)+2] = 0;
  } else {
    dep->stereotype = NULL;
  }

  dep->draw_arrow = prop_dialog->draw_arrow->active;

  dep->text_width = 0.0;

  if (dep->name != NULL) {
    dep->text_width =
      font_string_width(dep->name, dep_font, DEPENDENCY_FONTHEIGHT);
  }
  if (dep->stereotype != NULL) {
    dep->text_width = MAX(dep->text_width,
			  font_string_width(dep->stereotype, dep_font, DEPENDENCY_FONTHEIGHT));
  }
  
  dependency_update_data(dep);
}

static void
fill_in_dialog(Dependency *dep)
{
  DependencyPropertiesDialog *prop_dialog;
  char *str;
  
  prop_dialog = dep->properties_dialog;

  if (dep->name != NULL)
    gtk_entry_set_text(prop_dialog->name, dep->name);
  else 
    gtk_entry_set_text(prop_dialog->name, "");
  
  
  if (dep->stereotype != NULL) {
    str = strdup(dep->stereotype);
    strcpy(str, dep->stereotype+1);
    str[strlen(str)-1] = 0;
    gtk_entry_set_text(prop_dialog->stereotype, str);
    g_free(str);
  } else {
    gtk_entry_set_text(prop_dialog->stereotype, "");
  }

  gtk_toggle_button_set_active(prop_dialog->draw_arrow, dep->draw_arrow);
}

static GtkWidget *
dependency_get_properties(Dependency *dep)
{
  DependencyPropertiesDialog *prop_dialog;
  GtkWidget *dialog;
  GtkWidget *checkbox;
  GtkWidget *entry;
  GtkWidget *hbox;
  GtkWidget *label;

  if (dep->properties_dialog == NULL) {

    prop_dialog = g_new(DependencyPropertiesDialog, 1);
    dep->properties_dialog = prop_dialog;

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
    gtk_container_set_border_width (GTK_CONTAINER (hbox), 5);
    label = gtk_label_new("Stereotype:");
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    entry = gtk_entry_new();
    prop_dialog->stereotype = GTK_ENTRY(entry);
    gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, TRUE, 0);
    gtk_widget_show (label);
    gtk_widget_show (entry);
    gtk_box_pack_start (GTK_BOX (dialog), hbox, TRUE, TRUE, 0);
    gtk_widget_show(hbox);
    

    hbox = gtk_hbox_new(FALSE, 5);
    checkbox = gtk_check_button_new_with_label("Show arrow:");
    prop_dialog->draw_arrow = GTK_TOGGLE_BUTTON( checkbox );
    gtk_widget_show(checkbox);
    gtk_widget_show(hbox);
    gtk_box_pack_start (GTK_BOX (hbox),	checkbox, TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (dialog), hbox, TRUE, TRUE, 0);

  }
  
  fill_in_dialog(dep);
  gtk_widget_show (dep->properties_dialog->dialog);

  return dep->properties_dialog->dialog;
}




