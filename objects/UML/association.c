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

#include "config.h"
#include "intl.h"
#include "object.h"
#include "orth_conn.h"
#include "render.h"
#include "attributes.h"
#include "sheet.h"
#include "arrows.h"

#include "uml.h"

#include "pixmaps/association.xpm"
#include "pixmaps/aggregation.xpm"

typedef struct _Association Association;
typedef struct _AssociationPropertiesDialog AssociationPropertiesDialog;

typedef enum {
  ASSOC_NODIR,
  ASSOC_RIGHT,
  ASSOC_LEFT
} AssociationDirection;

typedef enum {
  AGGREGATE_NONE,
  AGGREGATE_NORMAL,
  AGGREGATE_COMPOSITION
} AggregateType;

typedef struct _AssociationEnd {
  char *role; /* Can be NULL */
  char *multiplicity; /* Can be NULL */
  Point text_pos;
  real text_width;
  Alignment text_align;
  
  int arrow;
  AggregateType aggregate; /* Note: Can only be != NONE on ONE side! */
} AssociationEnd;

struct _Association {
  OrthConn orth;

  Point text_pos;
  Alignment text_align;
  real text_width;
  
  char *name;
  AssociationDirection direction;

  AssociationEnd end[2];
  
  AssociationPropertiesDialog* properties_dialog;
};

struct _AssociationPropertiesDialog {
  GtkWidget *dialog;
  
  GtkEntry *name;
  GtkMenu *dir_menu;
  GtkOptionMenu *dir_omenu;
  
  struct {
    GtkEntry *role;
    GtkEntry *multiplicity;
    GtkToggleButton *draw_arrow;
    GtkToggleButton *aggregate;
    GtkToggleButton *composition;
  } end[2];
};

#define ASSOCIATION_WIDTH 0.1
#define ASSOCIATION_TRIANGLESIZE 0.8
#define ASSOCIATION_DIAMONDLEN 1.4
#define ASSOCIATION_FONTHEIGHT 0.8

static Font *assoc_font = NULL;

static real association_distance_from(Association *assoc, Point *point);
static void association_select(Association *assoc, Point *clicked_point,
			       Renderer *interactive_renderer);
static void association_move_handle(Association *assoc, Handle *handle,
				    Point *to, HandleMoveReason reason, ModifierKeys modifiers);
static void association_move(Association *assoc, Point *to);
static void association_draw(Association *assoc, Renderer *renderer);
static Object *association_create(Point *startpoint,
				  void *user_data,
				  Handle **handle1,
				  Handle **handle2);
static void association_destroy(Association *assoc);
static Object *association_copy(Association *assoc);
static GtkWidget *association_get_properties(Association *assoc);
static void association_apply_properties(Association *assoc);
static void association_save(Association *assoc, ObjectNode obj_node,
			     const char *filename);
static Object *association_load(ObjectNode obj_node, int version,
				const char *filename);

static void association_update_data(Association *assoc);

static ObjectTypeOps association_type_ops =
{
  (CreateFunc) association_create,
  (LoadFunc)   association_load,
  (SaveFunc)   association_save
};

ObjectType association_type =
{
  "UML - Association",   /* name */
  0,                      /* version */
  (char **) association_xpm,  /* pixmap */
  
  &association_type_ops       /* ops */
};

SheetObject association_sheetobj =
{
  "UML - Association",             /* type */
  N_("Association, two classes are associated."),
                              /* description */
  (char **) association_xpm,     /* pixmap */

  GINT_TO_POINTER(0)                        /* user_data */
};

SheetObject aggregation_sheetobj =
{
  "UML - Association",             /* type */
  N_("Aggregation, one class is part of another."),
                              /* description */
  (char **) aggregation_xpm,     /* pixmap */

  GINT_TO_POINTER(1)                        /* user_data */
};

static ObjectOps association_ops = {
  (DestroyFunc)         association_destroy,
  (DrawFunc)            association_draw,
  (DistanceFunc)        association_distance_from,
  (SelectFunc)          association_select,
  (CopyFunc)            association_copy,
  (MoveFunc)            association_move,
  (MoveHandleFunc)      association_move_handle,
  (GetPropertiesFunc)   association_get_properties,
  (ApplyPropertiesFunc) association_apply_properties,
  (IsEmptyFunc)         object_return_false,
  (ObjectMenuFunc)      NULL
};

static real
association_distance_from(Association *assoc, Point *point)
{
  OrthConn *orth = &assoc->orth;
  return orthconn_distance_from(orth, point, ASSOCIATION_WIDTH);
}

static void
association_select(Association *assoc, Point *clicked_point,
		  Renderer *interactive_renderer)
{
  orthconn_update_data(&assoc->orth);
}

static void
association_move_handle(Association *assoc, Handle *handle,
		       Point *to, HandleMoveReason reason, ModifierKeys modifiers)
{
  assert(assoc!=NULL);
  assert(handle!=NULL);
  assert(to!=NULL);
  
  orthconn_move_handle(&assoc->orth, handle, to, reason);
  association_update_data(assoc);
}

static void
association_move(Association *assoc, Point *to)
{
  orthconn_move(&assoc->orth, to);
  association_update_data(assoc);
}

static void
association_draw(Association *assoc, Renderer *renderer)
{
  OrthConn *orth = &assoc->orth;
  Point *points;
  Point poly[3];
  int n,i;
  Point pos;
  
  points = &orth->points[0];
  n = orth->numpoints;
  
  renderer->ops->set_linewidth(renderer, ASSOCIATION_WIDTH);
  renderer->ops->set_linestyle(renderer, LINESTYLE_SOLID);
  renderer->ops->set_linejoin(renderer, LINEJOIN_MITER);
  renderer->ops->set_linecaps(renderer, LINECAPS_BUTT);

  renderer->ops->draw_polyline(renderer, points, n, &color_black);

  if (assoc->end[0].arrow) {
    arrow_draw(renderer, ARROW_LINES,
	       &points[0], &points[1],
	       ASSOCIATION_TRIANGLESIZE, ASSOCIATION_TRIANGLESIZE,
	       ASSOCIATION_WIDTH,
	       &color_black, &color_white);
  }

  switch (assoc->end[0].aggregate) {
  case AGGREGATE_NORMAL:
    arrow_draw(renderer, ARROW_HOLLOW_DIAMOND,
	       &points[0], &points[1],
	       ASSOCIATION_DIAMONDLEN, ASSOCIATION_TRIANGLESIZE*0.6,
	       ASSOCIATION_WIDTH,
	       &color_black, &color_white);
    break;
  case AGGREGATE_COMPOSITION:
    arrow_draw(renderer, ARROW_FILLED_DIAMOND,
	       &points[0], &points[1],
	       ASSOCIATION_DIAMONDLEN, ASSOCIATION_TRIANGLESIZE*0.6,
	       ASSOCIATION_WIDTH,
	       &color_black, &color_white);
    break;
  case AGGREGATE_NONE:
    /* Nothing */
    break;
  }

  if (assoc->end[1].arrow) {
    arrow_draw(renderer, ARROW_LINES,
	       &points[n-1], &points[n-2],
	       ASSOCIATION_TRIANGLESIZE, ASSOCIATION_TRIANGLESIZE,
	       ASSOCIATION_WIDTH,
	       &color_black, &color_white);
  }

  switch (assoc->end[1].aggregate) {
  case AGGREGATE_NORMAL:
    arrow_draw(renderer, ARROW_HOLLOW_DIAMOND,
	       &points[n-1], &points[n-2],
	       ASSOCIATION_DIAMONDLEN, ASSOCIATION_TRIANGLESIZE*0.6,
	       ASSOCIATION_WIDTH,
	       &color_black, &color_white);
    break;
  case AGGREGATE_COMPOSITION:
    arrow_draw(renderer, ARROW_FILLED_DIAMOND,
	       &points[n-1], &points[n-2],
	       ASSOCIATION_DIAMONDLEN, ASSOCIATION_TRIANGLESIZE*0.6,
	       ASSOCIATION_WIDTH,
	       &color_black, &color_white);
    break;
  case AGGREGATE_NONE:
    /* Nothing */
    break;
  }

  /* Name: */
  renderer->ops->set_font(renderer, assoc_font, ASSOCIATION_FONTHEIGHT);
 
  if (assoc->name != NULL) {
    pos = assoc->text_pos;
    renderer->ops->draw_string(renderer, assoc->name,
			       &pos, assoc->text_align,
			       &color_black);
  }

  /* Direction: */
  renderer->ops->set_fillstyle(renderer, FILLSTYLE_SOLID);

  switch (assoc->direction) {
  case ASSOC_NODIR:
    break;
  case ASSOC_RIGHT:
    poly[0].x = assoc->text_pos.x + assoc->text_width + 0.1;
    if (assoc->text_align == ALIGN_CENTER)
      poly[0].x -= assoc->text_width/2.0;
    poly[0].y = assoc->text_pos.y;
    poly[1].x = poly[0].x;
    poly[1].y = poly[0].y - ASSOCIATION_FONTHEIGHT*0.5;
    poly[2].x = poly[0].x + ASSOCIATION_FONTHEIGHT*0.5;
    poly[2].y = poly[0].y - ASSOCIATION_FONTHEIGHT*0.5*0.5;
    renderer->ops->fill_polygon(renderer, poly, 3, &color_black);
    break;
  case ASSOC_LEFT:
    poly[0].x = assoc->text_pos.x - 0.2;
    if (assoc->text_align == ALIGN_CENTER)
      poly[0].x -= assoc->text_width/2.0;
    poly[0].y = assoc->text_pos.y;
    poly[1].x = poly[0].x;
    poly[1].y = poly[0].y - ASSOCIATION_FONTHEIGHT*0.5;
    poly[2].x = poly[0].x - ASSOCIATION_FONTHEIGHT*0.5;
    poly[2].y = poly[0].y - ASSOCIATION_FONTHEIGHT*0.5*0.5;
    renderer->ops->fill_polygon(renderer, poly, 3, &color_black);
    break;
  }


  for (i=0;i<2;i++) {
    AssociationEnd *end = &assoc->end[i];
    pos = end->text_pos;

    if (end->role != NULL) {
      renderer->ops->draw_string(renderer, end->role,
				 &pos, end->text_align,
				 &color_black);
      pos.y += ASSOCIATION_FONTHEIGHT;
    }
    if (end->multiplicity != NULL) {
      renderer->ops->draw_string(renderer, end->multiplicity,
				 &pos, end->text_align,
				 &color_black);
    }
  }
}

static void
association_update_data(Association *assoc)
{
  OrthConn *orth = &assoc->orth;
  Object *obj = (Object *) assoc;
  int num_segm, i, n;
  Point *points;
  Rectangle rect;
  AssociationEnd *end;
  
  orthconn_update_data(orth);
  
  orthconn_update_boundingbox(orth);
  /* fix boundingassociation for linewidth and triangle: */
  obj->bounding_box.top -= ASSOCIATION_WIDTH/2.0 + ASSOCIATION_DIAMONDLEN;
  obj->bounding_box.left -= ASSOCIATION_WIDTH/2.0 + ASSOCIATION_DIAMONDLEN;
  obj->bounding_box.bottom += ASSOCIATION_WIDTH/2.0 + ASSOCIATION_DIAMONDLEN;
  obj->bounding_box.right += ASSOCIATION_WIDTH/2.0 + ASSOCIATION_DIAMONDLEN;
  
  /* Calc text pos: */
  num_segm = assoc->orth.numpoints - 1;
  points = assoc->orth.points;
  i = num_segm / 2;
  
  if ((num_segm % 2) == 0) { /* If no middle segment, use horizontal */
    if (assoc->orth.orientation[i]==VERTICAL)
      i--;
  }

  switch (assoc->orth.orientation[i]) {
  case HORIZONTAL:
    assoc->text_align = ALIGN_CENTER;
    assoc->text_pos.x = 0.5*(points[i].x+points[i+1].x);
    assoc->text_pos.y = points[i].y - font_descent(assoc_font, ASSOCIATION_FONTHEIGHT);
    break;
  case VERTICAL:
    assoc->text_align = ALIGN_LEFT;
    assoc->text_pos.x = points[i].x + 0.1;
    assoc->text_pos.y =
      0.5*(points[i].y+points[i+1].y) - font_descent(assoc_font, ASSOCIATION_FONTHEIGHT);
    break;
  }

  /* Add the text recangle to the bounding box: */
  rect.left = assoc->text_pos.x;
  if (assoc->text_align == ALIGN_CENTER)
    rect.left -= assoc->text_width/2.0;
  rect.right = rect.left + assoc->text_width;
  rect.top = assoc->text_pos.y - font_ascent(assoc_font, ASSOCIATION_FONTHEIGHT);
  rect.bottom = rect.top + ASSOCIATION_FONTHEIGHT;

  rectangle_union(&obj->bounding_box, &rect);


  /* Update the text-points of the ends: */
  /* END 1: */
  end = &assoc->end[0];
  end->text_pos = points[0];
  switch (assoc->orth.orientation[0]) {
  case HORIZONTAL:
    end->text_pos.y -= font_descent(assoc_font, ASSOCIATION_FONTHEIGHT);
    if (points[0].x < points[1].x)
      end->text_align = ALIGN_LEFT;
    else
      end->text_align = ALIGN_RIGHT;
    break;
  case VERTICAL:
    end->text_pos.y += font_ascent(assoc_font, ASSOCIATION_FONTHEIGHT);
    end->text_align = ALIGN_LEFT;
    break;
  }
  /* Add the text recangle to the bounding box: */
  rect.left = end->text_pos.x;
  rect.right = rect.left + end->text_width;
  rect.top = end->text_pos.y - font_ascent(assoc_font, ASSOCIATION_FONTHEIGHT);
  rect.bottom = rect.top + 2*ASSOCIATION_FONTHEIGHT;
  
  rectangle_union(&obj->bounding_box, &rect);

  /* END 2: */
  end = &assoc->end[1];
  n = assoc->orth.numpoints - 1;
  end->text_pos = points[n];
  switch (assoc->orth.orientation[n-1]) {
  case HORIZONTAL:
    end->text_pos.y -= font_descent(assoc_font, ASSOCIATION_FONTHEIGHT);
    if (points[n].x < points[n-1].x)
      end->text_align = ALIGN_LEFT;
    else
      end->text_align = ALIGN_RIGHT;
    break;
  case VERTICAL:
    end->text_pos.y += font_ascent(assoc_font, ASSOCIATION_FONTHEIGHT);
    if (points[n].y > points[n-1].y) {
      if (end->role!=NULL)
	end->text_pos.y -= ASSOCIATION_FONTHEIGHT;
      if (end->multiplicity!=NULL)
	end->text_pos.y -= ASSOCIATION_FONTHEIGHT;
    }
    end->text_align = ALIGN_LEFT;
    break;
  }
  /* Add the text recangle to the bounding box: */
  rect.left = end->text_pos.x;
  rect.right = rect.left + end->text_width;
  rect.top = end->text_pos.y - font_ascent(assoc_font, ASSOCIATION_FONTHEIGHT);
  rect.bottom = rect.top + 2*ASSOCIATION_FONTHEIGHT;
  
  rectangle_union(&obj->bounding_box, &rect);
}

static Object *
association_create(Point *startpoint,
	       void *user_data,
  	       Handle **handle1,
	       Handle **handle2)
{
  Association *assoc;
  OrthConn *orth;
  Object *obj;
  int i;
  int user_d;

  if (assoc_font == NULL) {
    assoc_font = font_getfont("Courier");
  }
  
  assoc = g_malloc(sizeof(Association));
  orth = &assoc->orth;
  obj = (Object *) assoc;
  
  obj->type = &association_type;

  obj->ops = &association_ops;

  orthconn_init(orth, startpoint);
  
  assoc->name = NULL;
  assoc->direction = ASSOC_NODIR;
  for (i=0;i<2;i++) {
    assoc->end[i].role = NULL;
    assoc->end[i].multiplicity = NULL;
    assoc->end[i].arrow = FALSE;
    assoc->end[i].aggregate = AGGREGATE_NONE;
    assoc->end[i].text_width = 0.0;
  }
  
  assoc->text_width = 0.0;
  assoc->properties_dialog = NULL;

  user_d = GPOINTER_TO_INT(user_data);
  switch (user_d) {
  case 0:
    /* Ok already. */
    break;
  case 1:
    assoc->end[1].aggregate = AGGREGATE_NORMAL;
    break;
  }

  association_update_data(assoc);
  
  *handle1 = orth->handles[0];
  *handle2 = orth->handles[orth->numpoints-2];

  return (Object *)assoc;
}

static void
association_destroy(Association *assoc)
{
  int i;
  
  orthconn_destroy(&assoc->orth);

  if (assoc->name != NULL)
    g_free(assoc->name);

  for (i=0;i<2;i++) {
    if (assoc->end[i].role != NULL)
      g_free(assoc->end[i].role);
    
    if (assoc->end[i].multiplicity != NULL)
      g_free(assoc->end[i].multiplicity);
  }

  if (assoc->properties_dialog != NULL) {
    gtk_widget_destroy(assoc->properties_dialog->dialog);
    g_free(assoc->properties_dialog);
  }
}

static Object *
association_copy(Association *assoc)
{
  Association *newassoc;
  OrthConn *orth, *neworth;
  Object *newobj;
  int i;
  
  orth = &assoc->orth;
  
  newassoc = g_malloc(sizeof(Association));
  neworth = &newassoc->orth;
  newobj = (Object *) newassoc;

  orthconn_copy(orth, neworth);

  newassoc->name = (assoc->name != NULL)? strdup(assoc->name):NULL;
  newassoc->direction = assoc->direction;
  for (i=0;i<2;i++) {
    newassoc->end[i] = assoc->end[i];
    newassoc->end[i].role =
      (assoc->end[i].role != NULL)?strdup(assoc->end[i].role):NULL;
    newassoc->end[i].multiplicity =
      (assoc->end[i].multiplicity != NULL)?strdup(assoc->end[i].multiplicity):NULL;
  }

  newassoc->text_width = assoc->text_width;
  newassoc->properties_dialog = NULL;
  
  association_update_data(newassoc);
  
  return (Object *)newassoc;
}


static void
association_save(Association *assoc, ObjectNode obj_node,
		 const char *filename)
{
  int i;
  AttributeNode attr;
  DataNode composite;
  
  orthconn_save(&assoc->orth, obj_node);

  data_add_string(new_attribute(obj_node, "name"),
		  assoc->name);
  data_add_enum(new_attribute(obj_node, "direction"),
		assoc->direction);

  attr = new_attribute(obj_node, "ends");
  for (i=0;i<2;i++) {
    composite = data_add_composite(attr, NULL);

    data_add_string(composite_add_attribute(composite, "role"),
		    assoc->end[i].role);
    data_add_string(composite_add_attribute(composite, "multiplicity"),
		    assoc->end[i].multiplicity);
    data_add_boolean(composite_add_attribute(composite, "arrow"),
		  assoc->end[i].arrow);
    data_add_enum(composite_add_attribute(composite, "aggregate"),
		  assoc->end[i].aggregate);
  }
}

static Object *
association_load(ObjectNode obj_node, int version, const char *filename)
{
  Association *assoc;
  AttributeNode attr;
  DataNode composite;
  OrthConn *orth;
  Object *obj;
  int i;
  
  if (assoc_font == NULL) {
    assoc_font = font_getfont("Courier");
  }

  assoc = g_new(Association, 1);

  orth = &assoc->orth;
  obj = (Object *) assoc;

  obj->type = &association_type;
  obj->ops = &association_ops;

  orthconn_load(orth, obj_node);

  assoc->name = NULL;
  attr = object_find_attribute(obj_node, "name");
  if (attr != NULL)
    assoc->name = data_string(attribute_first_data(attr));
  
  assoc->text_width = 0.0;
  if (assoc->name != NULL) {
    assoc->text_width =
      font_string_width(assoc->name, assoc_font, ASSOCIATION_FONTHEIGHT);
  }

  assoc->direction = ASSOC_NODIR;
  attr = object_find_attribute(obj_node, "direction");
  if (attr != NULL)
    assoc->direction = data_enum(attribute_first_data(attr));

  attr = object_find_attribute(obj_node, "ends");
  composite = attribute_first_data(attr);
  for (i=0;i<2;i++) {

    assoc->end[i].role = NULL;
    attr = composite_find_attribute(composite, "role");
    if (attr != NULL)
      assoc->end[i].role = data_string(attribute_first_data(attr));
    
    assoc->end[i].multiplicity = NULL;
    attr = composite_find_attribute(composite, "multiplicity");
    if (attr != NULL)
      assoc->end[i].multiplicity = data_string(attribute_first_data(attr));
    
    assoc->end[i].arrow = FALSE;
    attr = composite_find_attribute(composite, "arrow");
    if (attr != NULL)
      assoc->end[i].arrow = data_boolean(attribute_first_data(attr));

    assoc->end[i].aggregate = AGGREGATE_NONE;
    attr = composite_find_attribute(composite, "aggregate");
    if (attr != NULL)
      assoc->end[i].aggregate = data_enum(attribute_first_data(attr));

    assoc->end[i].text_width = 0.0;
    if (assoc->end[i].role != NULL) {
      assoc->end[i].text_width = 
	font_string_width(assoc->end[i].role, assoc_font,
			  ASSOCIATION_FONTHEIGHT);
    }
    if (assoc->end[i].multiplicity != NULL) {
      assoc->end[i].text_width =
	MAX(assoc->end[i].text_width,
	    font_string_width(assoc->end[i].multiplicity,
			      assoc_font, ASSOCIATION_FONTHEIGHT) );
    }
    composite = data_next(composite);
  }

  assoc->properties_dialog = NULL;
  
  association_update_data(assoc);

  return (Object *)assoc;
}

static void
association_apply_properties(Association *assoc)
{
  AssociationPropertiesDialog *prop_dialog;
  char *str;
  GtkWidget *menuitem;
  int i;
  
  prop_dialog = assoc->properties_dialog;

  /* Read from dialog and put in object: */
  if (assoc->name != NULL)
    g_free(assoc->name);
  str = gtk_entry_get_text(prop_dialog->name);
  if (strlen(str) != 0)
    assoc->name = strdup(str);
  else
    assoc->name = NULL;

  assoc->text_width = 0.0;

  if (assoc->name != NULL) {
    assoc->text_width =
      font_string_width(assoc->name, assoc_font, ASSOCIATION_FONTHEIGHT);
  }

  menuitem = gtk_menu_get_active(prop_dialog->dir_menu);
  assoc->direction =
    GPOINTER_TO_INT(gtk_object_get_user_data(GTK_OBJECT(menuitem)));

  for (i=0;i<2;i++) {
    AssociationEnd *end = &assoc->end[i];
    /* Role: */
    if (end->role != NULL)
      g_free(end->role);
    str = gtk_entry_get_text(prop_dialog->end[i].role);
    if (strlen(str) != 0)
      end->role = strdup(str);
    else
      end->role = NULL;

    /* Multiplicity: */
    if (end->multiplicity != NULL)
      g_free(end->multiplicity);
    str = gtk_entry_get_text(prop_dialog->end[i].multiplicity);
    if (strlen(str) != 0)
      end->multiplicity = strdup(str);
    else
      end->multiplicity = NULL;

    end->text_width = 0.0;

    if (end->role != NULL) {
      end->text_width = 
	font_string_width(end->role, assoc_font, ASSOCIATION_FONTHEIGHT);
    }
    if (end->multiplicity != NULL) {
      end->text_width =
	MAX(end->text_width,
	    font_string_width(end->multiplicity, assoc_font, ASSOCIATION_FONTHEIGHT) );
    }

    end->arrow = prop_dialog->end[i].draw_arrow->active;
    
    end->aggregate = AGGREGATE_NONE;
    if (prop_dialog->end[i].aggregate->active)
      end->aggregate = AGGREGATE_NORMAL;
    if (prop_dialog->end[i].composition->active)
      end->aggregate = AGGREGATE_COMPOSITION;

  }

  association_update_data(assoc);
}

static void
fill_in_dialog(Association *assoc)
{
  AssociationPropertiesDialog *prop_dialog;
  int i;
 
  prop_dialog = assoc->properties_dialog;

  if (assoc->name != NULL)
    gtk_entry_set_text(prop_dialog->name, assoc->name);
  else 
    gtk_entry_set_text(prop_dialog->name, "");
  
  gtk_option_menu_set_history(prop_dialog->dir_omenu, assoc->direction);

  for (i=0;i<2;i++) {
    if (assoc->end[i].role != NULL)
      gtk_entry_set_text(prop_dialog->end[i].role, assoc->end[i].role);
    else 
      gtk_entry_set_text(prop_dialog->end[i].role, "");
    
    if (assoc->end[i].multiplicity != NULL)
      gtk_entry_set_text(prop_dialog->end[i].multiplicity,
			 assoc->end[i].multiplicity);
    else 
      gtk_entry_set_text(prop_dialog->end[i].multiplicity, "");

    gtk_toggle_button_set_active(prop_dialog->end[i].draw_arrow,
				assoc->end[i].arrow);
    gtk_toggle_button_set_active(prop_dialog->end[i].aggregate,
				assoc->end[i].aggregate == AGGREGATE_NORMAL);
    gtk_toggle_button_set_active(prop_dialog->end[i].composition,
				assoc->end[i].aggregate == AGGREGATE_COMPOSITION);
  }
}

static void
mutex_aggregate_callback(GtkWidget *widget,
			 AssociationPropertiesDialog *prop_dialog)
{
  int i;
  GtkToggleButton *button;

  button = GTK_TOGGLE_BUTTON(widget);

  if (!button->active)
    return;

  for (i=0;i<2;i++) {
    if (prop_dialog->end[i].aggregate != button) {
      gtk_toggle_button_set_active(prop_dialog->end[i].aggregate, 0);
    }
    if (prop_dialog->end[i].composition != button) {
      gtk_toggle_button_set_active(prop_dialog->end[i].composition, 0);
    }
  }
}

static GtkWidget *
association_get_properties(Association *assoc)
{
  AssociationPropertiesDialog *prop_dialog;
  GtkWidget *dialog;
  GtkWidget *frame;
  GtkWidget *entry;
  GtkWidget *hbox;
  GtkWidget *split_hbox;
  GtkWidget *vbox;
  GtkWidget *label;
  GtkWidget *omenu;
  GtkWidget *menu;
  GtkWidget *submenu;
  GtkWidget *menuitem;
  GtkWidget *checkbox;
  GSList *group;
  int i;
  
  if (assoc->properties_dialog == NULL) {

    prop_dialog = g_new(AssociationPropertiesDialog, 1);
    assoc->properties_dialog = prop_dialog;

    dialog = gtk_vbox_new(FALSE, 0);
    prop_dialog->dialog = dialog;
    
    /* Name entry: */
    hbox = gtk_hbox_new(FALSE, 5);
    label = gtk_label_new(_("Name:"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    entry = gtk_entry_new();
    prop_dialog->name = GTK_ENTRY(entry);
    gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, TRUE, 0);
    gtk_widget_show (label);
    gtk_widget_show (entry);
    gtk_box_pack_start (GTK_BOX (dialog), hbox, TRUE, TRUE, 0);
    gtk_widget_show(hbox);

    /* Direction entry: */
    hbox = gtk_hbox_new(FALSE, 5);
    label = gtk_label_new(_("Direction:"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    
    omenu = gtk_option_menu_new ();
    menu = gtk_menu_new ();
    prop_dialog->dir_menu = GTK_MENU(menu);
    prop_dialog->dir_omenu = GTK_OPTION_MENU(omenu);
    submenu = NULL;
    group = NULL;
    
    menuitem = gtk_radio_menu_item_new_with_label (group, _("None"));
    gtk_object_set_user_data(GTK_OBJECT(menuitem),
			     GINT_TO_POINTER(ASSOC_NODIR));
    group = gtk_radio_menu_item_group (GTK_RADIO_MENU_ITEM (menuitem));
    gtk_menu_append (GTK_MENU (menu), menuitem);
    gtk_widget_show (menuitem);
    
    menuitem = gtk_radio_menu_item_new_with_label (group, _("Right"));
    gtk_object_set_user_data(GTK_OBJECT(menuitem),
			     GINT_TO_POINTER(ASSOC_RIGHT));
    group = gtk_radio_menu_item_group (GTK_RADIO_MENU_ITEM (menuitem));
    gtk_menu_append (GTK_MENU (menu), menuitem);
    gtk_widget_show (menuitem);
    
    menuitem = gtk_radio_menu_item_new_with_label (group, _("Left"));
    gtk_object_set_user_data(GTK_OBJECT(menuitem),
			     GINT_TO_POINTER(ASSOC_LEFT));
    group = gtk_radio_menu_item_group (GTK_RADIO_MENU_ITEM (menuitem));
    gtk_menu_append (GTK_MENU (menu), menuitem);
    gtk_widget_show (menuitem);
    
    gtk_option_menu_set_menu (GTK_OPTION_MENU (omenu), menu);
    gtk_box_pack_start (GTK_BOX (hbox), omenu, FALSE, TRUE, 0);
    
    gtk_widget_show (label);
    gtk_widget_show (omenu);
    gtk_box_pack_start (GTK_BOX (dialog), hbox, TRUE, TRUE, 0);
    gtk_widget_show(hbox);
    
    split_hbox = gtk_hbox_new(TRUE, 5);
    gtk_box_pack_start (GTK_BOX (dialog), split_hbox, TRUE, TRUE, 0);
    gtk_widget_show(split_hbox);

    group = NULL; /* For the radio-buttons */
    
    for (i=0;i<2;i++) {
      char *str;
      if (i==0)
	str = _("Side A");
      else
	str = _("Side B");
      frame = gtk_frame_new(str);
      
      vbox = gtk_vbox_new(FALSE, 5);
      /* End 'i' into vbox: */
      if (i==0)
	label = gtk_label_new(_("Side A"));
      else
	label = gtk_label_new(_("Side B"));
      
      gtk_box_pack_start (GTK_BOX (vbox), label, TRUE, TRUE, 0);
      
      /* Role entry: */
      hbox = gtk_hbox_new(FALSE, 5);
      label = gtk_label_new(_("Role:"));
      gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
      entry = gtk_entry_new();
      prop_dialog->end[i].role = GTK_ENTRY(entry);
      gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, TRUE, 0);
      gtk_widget_show (label);
      gtk_widget_show (entry);
      gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);
      gtk_widget_show(hbox);

      /* Multiplicity entry: */
      hbox = gtk_hbox_new(FALSE, 5);
      label = gtk_label_new(_("Multiplicity:"));
      gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
      entry = gtk_entry_new();
      prop_dialog->end[i].multiplicity = GTK_ENTRY(entry);
      gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, TRUE, 0);
      gtk_widget_show (label);
      gtk_widget_show (entry);
      gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);
      gtk_widget_show(hbox);

      /* Show arrow: */
      checkbox = gtk_check_button_new_with_label(_("Show arrow"));
      prop_dialog->end[i].draw_arrow = GTK_TOGGLE_BUTTON( checkbox );
      gtk_widget_show(checkbox);
      gtk_box_pack_start (GTK_BOX (vbox), checkbox, TRUE, TRUE, 0);

      /* Aggregate */
      checkbox = gtk_check_button_new_with_label(_("Aggregate"));
      prop_dialog->end[i].aggregate = GTK_TOGGLE_BUTTON( checkbox );
      gtk_signal_connect(GTK_OBJECT(checkbox), "toggled",
			 (GtkSignalFunc) mutex_aggregate_callback, prop_dialog);
      gtk_widget_show(checkbox);
      gtk_box_pack_start (GTK_BOX (vbox), checkbox, TRUE, TRUE, 0);

      /* Composition */
      checkbox = gtk_check_button_new_with_label(_("Composition"));
      prop_dialog->end[i].composition = GTK_TOGGLE_BUTTON( checkbox );
      gtk_signal_connect(GTK_OBJECT(checkbox), "toggled",
			 (GtkSignalFunc) mutex_aggregate_callback, prop_dialog);
      gtk_widget_show(checkbox);
      gtk_box_pack_start (GTK_BOX (vbox), checkbox, TRUE, TRUE, 0);
	
      gtk_container_set_border_width(GTK_CONTAINER(vbox), 5);
      gtk_container_add(GTK_CONTAINER(frame), vbox);
      gtk_box_pack_start (GTK_BOX (split_hbox), frame, TRUE, TRUE, 0);
      gtk_widget_show(vbox);
      gtk_widget_show(frame);
    }

  }
  fill_in_dialog(assoc);
  gtk_widget_show (assoc->properties_dialog->dialog);

  return assoc->properties_dialog->dialog;
}




