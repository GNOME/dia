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
#include "element.h"
#include "render.h"
#include "attributes.h"
#include "sheet.h"
#include "text.h"

#include "uml.h"

#include "pixmaps/largepackage.xpm"

typedef struct _LargePackage LargePackage;
typedef struct _LargePackageState LargePackageState;
typedef struct _PackagePropertiesDialog PackagePropertiesDialog;

struct _LargePackageState {
  ObjectState obj_state;
  
  char *name;
  char *stereotype; /* Can be NULL, including << and >> */
};

struct _LargePackage {
  Element element;

  ConnectionPoint connections[8];

  char *name;
  char *stereotype; /* Can be NULL, including << and >> */

  Font *font;
  
  real topwidth;
  real topheight;

  PackagePropertiesDialog* properties_dialog;
};

struct _PackagePropertiesDialog {
  GtkWidget *dialog;
  
  GtkEntry *name;
  GtkEntry *stereotype;
};

#define LARGEPACKAGE_BORDERWIDTH 0.1
#define LARGEPACKAGE_FONTHEIGHT 0.8

static real largepackage_distance_from(LargePackage *pkg, Point *point);
static void largepackage_select(LargePackage *pkg, Point *clicked_point,
				Renderer *interactive_renderer);
static void largepackage_move_handle(LargePackage *pkg, Handle *handle,
				     Point *to, HandleMoveReason reason, ModifierKeys modifiers);
static void largepackage_move(LargePackage *pkg, Point *to);
static void largepackage_draw(LargePackage *pkg, Renderer *renderer);
static Object *largepackage_create(Point *startpoint,
				   void *user_data,
				   Handle **handle1,
				   Handle **handle2);
static void largepackage_destroy(LargePackage *pkg);
static Object *largepackage_copy(LargePackage *pkg);

static void largepackage_save(LargePackage *pkg, ObjectNode obj_node,
			      const char *filename);
static Object *largepackage_load(ObjectNode obj_node, int version,
				 const char *filename);

static void largepackage_update_data(LargePackage *pkg);
static GtkWidget *largepackage_get_properties(LargePackage *pkg);
static ObjectChange *largepackage_apply_properties(LargePackage *pkg);

static LargePackageState *constraint_get_state(LargePackage *pkg);
static void implements_set_state(LargePackage *pkg,
				 LargePackageState *state);

static ObjectTypeOps largepackage_type_ops =
{
  (CreateFunc) largepackage_create,
  (LoadFunc)   largepackage_load,
  (SaveFunc)   largepackage_save
};

ObjectType largepackage_type =
{
  "UML - LargePackage",   /* name */
  0,                      /* version */
  (char **) largepackage_xpm,  /* pixmap */
  
  &largepackage_type_ops       /* ops */
};

SheetObject largepackage_sheetobj =
{
  "UML - LargePackage",             /* type */
  N_("Create a (large) package"),           /* description */
  (char **) largepackage_xpm,     /* pixmap */

  NULL                        /* user_data */
};

static ObjectOps largepackage_ops = {
  (DestroyFunc)         largepackage_destroy,
  (DrawFunc)            largepackage_draw,
  (DistanceFunc)        largepackage_distance_from,
  (SelectFunc)          largepackage_select,
  (CopyFunc)            largepackage_copy,
  (MoveFunc)            largepackage_move,
  (MoveHandleFunc)      largepackage_move_handle,
  (GetPropertiesFunc)   largepackage_get_properties,
  (ApplyPropertiesFunc) largepackage_apply_properties,
  (ObjectMenuFunc)      NULL
};

static real
largepackage_distance_from(LargePackage *pkg, Point *point)
{
  Object *obj = &pkg->element.object;
  return distance_rectangle_point(&obj->bounding_box, point);
}

static void
largepackage_select(LargePackage *pkg, Point *clicked_point,
		    Renderer *interactive_renderer)
{
  element_update_handles(&pkg->element);
}

static void
largepackage_move_handle(LargePackage *pkg, Handle *handle,
			 Point *to, HandleMoveReason reason, ModifierKeys modifiers)
{
  assert(pkg!=NULL);
  assert(handle!=NULL);
  assert(to!=NULL);

  assert(handle->id < 8);
  
  element_move_handle(&pkg->element, handle->id, to, reason);
  largepackage_update_data(pkg);
}

static void
largepackage_move(LargePackage *pkg, Point *to)
{
  pkg->element.corner = *to;

  largepackage_update_data(pkg);
}

static void
largepackage_draw(LargePackage *pkg, Renderer *renderer)
{
  Element *elem;
  real x, y, w, h;
  Point p1, p2;
  
  assert(pkg != NULL);
  assert(renderer != NULL);

  elem = &pkg->element;

  x = elem->corner.x;
  y = elem->corner.y;
  w = elem->width;
  h = elem->height;
  
  renderer->ops->set_fillstyle(renderer, FILLSTYLE_SOLID);
  renderer->ops->set_linewidth(renderer, LARGEPACKAGE_BORDERWIDTH);
  renderer->ops->set_linestyle(renderer, LINESTYLE_SOLID);


  p1.x = x; p1.y = y;
  p2.x = x+w; p2.y = y+h;

  renderer->ops->fill_rect(renderer, 
			   &p1, &p2,
			   &color_white);
  renderer->ops->draw_rect(renderer, 
			   &p1, &p2,
			   &color_black);

  p1.x= x; p1.y = y - pkg->topheight;
  p2.x = x + pkg->topwidth; p2.y = y;

  renderer->ops->fill_rect(renderer, 
			   &p1, &p2,
			   &color_white);
  
  renderer->ops->draw_rect(renderer, 
			   &p1, &p2,
			   &color_black);


  renderer->ops->set_font(renderer, pkg->font, LARGEPACKAGE_FONTHEIGHT);

  p1.x = x + 0.1;
  p1.y = y - LARGEPACKAGE_FONTHEIGHT - font_descent(pkg->font, LARGEPACKAGE_FONTHEIGHT) - 0.1;

  if (pkg->stereotype) {
    renderer->ops->draw_string(renderer, pkg->stereotype, &p1,
			       ALIGN_LEFT, &color_black);
  }
  p1.y += LARGEPACKAGE_FONTHEIGHT;

  renderer->ops->draw_string(renderer, pkg->name, &p1,
			     ALIGN_LEFT, &color_black);


}

static void
largepackage_update_data(LargePackage *pkg)
{
  Element *elem = &pkg->element;
  Object *obj = (Object *) pkg;

  if (elem->width < (pkg->topwidth + 0.2))
    elem->width = pkg->topwidth + 0.2;
  if (elem->height < 1.0)
    elem->height = 1.0;
  
  /* Update connections: */
  pkg->connections[0].pos = elem->corner;
  pkg->connections[1].pos.x = elem->corner.x + elem->width / 2.0;
  pkg->connections[1].pos.y = elem->corner.y;
  pkg->connections[2].pos.x = elem->corner.x + elem->width;
  pkg->connections[2].pos.y = elem->corner.y;
  pkg->connections[3].pos.x = elem->corner.x;
  pkg->connections[3].pos.y = elem->corner.y + elem->height / 2.0;
  pkg->connections[4].pos.x = elem->corner.x + elem->width;
  pkg->connections[4].pos.y = elem->corner.y + elem->height / 2.0;
  pkg->connections[5].pos.x = elem->corner.x;
  pkg->connections[5].pos.y = elem->corner.y + elem->height;
  pkg->connections[6].pos.x = elem->corner.x + elem->width / 2.0;
  pkg->connections[6].pos.y = elem->corner.y + elem->height;
  pkg->connections[7].pos.x = elem->corner.x + elem->width;
  pkg->connections[7].pos.y = elem->corner.y + elem->height;
  
  element_update_boundingbox(elem);
  /* fix boundinglargepackage for line width and top rectangle: */
  obj->bounding_box.top -= LARGEPACKAGE_BORDERWIDTH/2.0 + pkg->topheight;
  obj->bounding_box.left -= LARGEPACKAGE_BORDERWIDTH/2.0;
  obj->bounding_box.bottom += LARGEPACKAGE_BORDERWIDTH/2.0;
  obj->bounding_box.right += LARGEPACKAGE_BORDERWIDTH/2.0;

  obj->position = elem->corner;

  element_update_handles(elem);
}

static Object *
largepackage_create(Point *startpoint,
		    void *user_data,
		    Handle **handle1,
		    Handle **handle2)
{
  LargePackage *pkg;
  Element *elem;
  Object *obj;
  int i;
  
  pkg = g_malloc(sizeof(LargePackage));
  elem = &pkg->element;
  obj = (Object *) pkg;
  
  obj->type = &largepackage_type;

  obj->ops = &largepackage_ops;

  elem->corner = *startpoint;

  element_init(elem, 8, 8);

  elem->width = 4.0;
  elem->height = 4.0;
  
  pkg->font = font_getfont("Courier");

  pkg->stereotype = NULL;
  pkg->name = strdup("");

  pkg->topwidth = 2.0;
  pkg->topheight = LARGEPACKAGE_FONTHEIGHT*2 + 0.1*2;

  pkg->properties_dialog = NULL;
  
  for (i=0;i<8;i++) {
    obj->connections[i] = &pkg->connections[i];
    pkg->connections[i].object = obj;
    pkg->connections[i].connected = NULL;
  }
  largepackage_update_data(pkg);

  *handle1 = NULL;
  *handle2 = obj->handles[0];
  return (Object *)pkg;
}

static void
largepackage_destroy(LargePackage *pkg)
{

  if (pkg->stereotype != NULL)
    g_free(pkg->stereotype);
  
  g_free(pkg->name);
  
  if (pkg->properties_dialog != NULL) {
    gtk_widget_destroy(pkg->properties_dialog->dialog);
    g_free(pkg->properties_dialog);
  }
  
  element_destroy(&pkg->element);
}

static Object *
largepackage_copy(LargePackage *pkg)
{
  int i;
  LargePackage *newpkg;
  Element *elem, *newelem;
  Object *newobj;
  
  elem = &pkg->element;
  
  newpkg = g_malloc(sizeof(LargePackage));
  newelem = &newpkg->element;
  newobj = (Object *) newpkg;

  element_copy(elem, newelem);

  if (pkg->stereotype == NULL)
    newpkg->stereotype = NULL;
  else
    newpkg->stereotype = strdup(pkg->stereotype);

  newpkg->name = strdup(pkg->name);

  newpkg->font = pkg->font;
  newpkg->topwidth = pkg->topwidth;
  newpkg->topheight = pkg->topheight;
  newpkg->properties_dialog = NULL;
  
  for (i=0;i<8;i++) {
    newobj->connections[i] = &newpkg->connections[i];
    newpkg->connections[i].object = newobj;
    newpkg->connections[i].connected = NULL;
    newpkg->connections[i].pos = pkg->connections[i].pos;
    newpkg->connections[i].last_pos = pkg->connections[i].last_pos;
  }

  largepackage_update_data(newpkg);
  
  return (Object *)newpkg;
}

static void
largepackage_state_free(ObjectState *ostate)
{
  LargePackageState *state = (LargePackageState *)ostate;
  int i;
  g_free(state->name);
  g_free(state->stereotype);
}

static LargePackageState *
largepackage_get_state(LargePackage *pkg)
{
  int i;
  LargePackageState *state = g_new(LargePackageState, 1);

  state->obj_state.free = largepackage_state_free;

  state->name = g_strdup(pkg->name);
  state->stereotype = g_strdup(pkg->stereotype);

  return state;
}

static void
largepackage_set_state(LargePackage *pkg, LargePackageState *state)
{
  int i;
  
  g_free(pkg->name);
  pkg->name = state->name;
  g_free(pkg->stereotype);
  pkg->stereotype = state->stereotype;
  
  g_free(state);
  
  largepackage_update_data(pkg);
}

static void
largepackage_save(LargePackage *pkg, ObjectNode obj_node,
		  const char *filename)
{
  element_save(&pkg->element, obj_node);

  data_add_string(new_attribute(obj_node, "name"),
		  pkg->name);
  data_add_string(new_attribute(obj_node, "stereotype"),
		  pkg->stereotype);
}

static Object *
largepackage_load(ObjectNode obj_node, int version, const char *filename)
{
  LargePackage *pkg;
  AttributeNode attr;
  Element *elem;
  Object *obj;
  int i;
  
  pkg = g_malloc(sizeof(LargePackage));
  elem = &pkg->element;
  obj = (Object *) pkg;
  
  obj->type = &largepackage_type;
  obj->ops = &largepackage_ops;

  element_load(elem, obj_node);

  pkg->name = NULL;
  attr = object_find_attribute(obj_node, "name");
  if (attr != NULL)
    pkg->name = data_string(attribute_first_data(attr));
  
  pkg->stereotype = NULL;
  attr = object_find_attribute(obj_node, "stereotype");
  if (attr != NULL)
    pkg->stereotype = data_string(attribute_first_data(attr));

  pkg->font = font_getfont("Courier");

  pkg->topwidth = 2.0;
  pkg->topwidth = MAX(pkg->topwidth,
		      font_string_width(pkg->name, pkg->font,
					LARGEPACKAGE_FONTHEIGHT)+2*0.1);
  if (pkg->stereotype != NULL)
    pkg->topwidth = MAX(pkg->topwidth,
			font_string_width(pkg->stereotype, pkg->font,
					  LARGEPACKAGE_FONTHEIGHT)+2*0.1);
  
  pkg->topheight = LARGEPACKAGE_FONTHEIGHT*2 + 0.1*2;

  pkg->properties_dialog = NULL;

  element_init(elem, 8, 8);

  for (i=0;i<8;i++) {
    obj->connections[i] = &pkg->connections[i];
    pkg->connections[i].object = obj;
    pkg->connections[i].connected = NULL;
  }
  largepackage_update_data(pkg);

  return (Object *) pkg;
}

static ObjectChange *
largepackage_apply_properties(LargePackage *pkg)
{
  PackagePropertiesDialog *prop_dialog;
  ObjectState *old_state;
  char *str;

  prop_dialog = pkg->properties_dialog;

  old_state = (ObjectState *)largepackage_get_state(pkg);

  /* Read from dialog and put in object: */
  g_free(pkg->name);
  str = gtk_entry_get_text(prop_dialog->name);
  pkg->name = strdup(str);

  if (pkg->stereotype != NULL)
    g_free(pkg->stereotype);
  
  str = gtk_entry_get_text(prop_dialog->stereotype);
  
  if (strlen(str) != 0) {
    pkg->stereotype = g_malloc(sizeof(char)*strlen(str)+2+1);
    pkg->stereotype[0] = UML_STEREOTYPE_START;
    pkg->stereotype[1] = 0;
    strcat(pkg->stereotype, str);
    pkg->stereotype[strlen(str)+1] = UML_STEREOTYPE_END;
    pkg->stereotype[strlen(str)+2] = 0;
  } else {
    pkg->stereotype = NULL;
  }

  pkg->topwidth = 2.0;
  pkg->topwidth = MAX(pkg->topwidth,
		      font_string_width(pkg->name, pkg->font,
					LARGEPACKAGE_FONTHEIGHT)+2*0.1);
  if (pkg->stereotype != NULL)
    pkg->topwidth = MAX(pkg->topwidth,
			font_string_width(pkg->stereotype, pkg->font,
					  LARGEPACKAGE_FONTHEIGHT)+2*0.1);
  
  pkg->topheight = LARGEPACKAGE_FONTHEIGHT*2 + 0.1*2;

  largepackage_update_data(pkg);
  return new_object_state_change((Object *)pkg, old_state, 
				 (GetStateFunc)largepackage_get_state,
				 (SetStateFunc)largepackage_set_state);
}

static void
fill_in_dialog(LargePackage *pkg)
{
  PackagePropertiesDialog *prop_dialog;
  char *str;
  
  prop_dialog = pkg->properties_dialog;

  gtk_entry_set_text(prop_dialog->name, pkg->name);
  
  if (pkg->stereotype != NULL) {
    str = strdup(pkg->stereotype);
    strcpy(str, pkg->stereotype+1);
    str[strlen(str)-1] = 0;
    gtk_entry_set_text(prop_dialog->stereotype, str);
    g_free(str);
  } else {
    gtk_entry_set_text(prop_dialog->stereotype, "");
  }
}

static GtkWidget *
largepackage_get_properties(LargePackage *pkg)
{
  PackagePropertiesDialog *prop_dialog;
  GtkWidget *dialog;
  GtkWidget *entry;
  GtkWidget *hbox;
  GtkWidget *label;

  if (pkg->properties_dialog == NULL) {
    prop_dialog = g_new(PackagePropertiesDialog, 1);
    pkg->properties_dialog = prop_dialog;

    dialog = gtk_vbox_new(FALSE, 0);
    prop_dialog->dialog = dialog;
    
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

    hbox = gtk_hbox_new(FALSE, 5);
    gtk_container_set_border_width (GTK_CONTAINER (hbox), 5);
    label = gtk_label_new(_("Stereotype:"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    entry = gtk_entry_new();
    prop_dialog->stereotype = GTK_ENTRY(entry);
    gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, TRUE, 0);
    gtk_widget_show (label);
    gtk_widget_show (entry);
    gtk_box_pack_start (GTK_BOX (dialog), hbox, TRUE, TRUE, 0);
    gtk_widget_show(hbox);
  }
  fill_in_dialog(pkg);
  gtk_widget_show (pkg->properties_dialog->dialog);
  return pkg->properties_dialog->dialog;
}




