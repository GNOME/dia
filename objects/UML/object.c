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
#include <stdio.h>

#include "config.h"
#include "intl.h"
#include "object.h"
#include "element.h"
#include "render.h"
#include "attributes.h"
#include "sheet.h"
#include "text.h"

#include "uml.h"

#include "pixmaps/object.xpm"

typedef struct _Objet Objet;
typedef struct _ObjetPropertiesDialog ObjetPropertiesDialog;


struct _Objet {
  Element element;

  ConnectionPoint connections[8];
  
  char *stereotype;
  Text *text;
  char *exstate;  /* used for explicit state */
  Text *attributes;

  Point ex_pos, st_pos;
  int is_active;
  int show_attributes;
  int is_multiple;  
};

struct _ObjetPropertiesDialog {
  GtkWidget *dialog;
  
  GtkEntry *name;
  GtkEntry *stereotype;
  GtkWidget *attribs;
  GtkToggleButton *show_attrib;
  GtkToggleButton *active;
  GtkToggleButton *multiple;
};

#define OBJET_BORDERWIDTH 0.1
#define OBJET_ACTIVEBORDERWIDTH 0.2
#define OBJET_LINEWIDTH 0.05
#define OBJET_MARGIN_X 0.5
#define OBJET_MARGIN_Y 0.5
#define OBJET_MARGIN_M 0.4
#define OBJET_FONTHEIGHT 0.8

static ObjetPropertiesDialog* properties_dialog = NULL;

static real objet_distance_from(Objet *pkg, Point *point);
static void objet_select(Objet *pkg, Point *clicked_point,
				Renderer *interactive_renderer);
static void objet_move_handle(Objet *pkg, Handle *handle,
				     Point *to, HandleMoveReason reason, ModifierKeys modifiers);
static void objet_move(Objet *pkg, Point *to);
static void objet_draw(Objet *pkg, Renderer *renderer);
static Object *objet_create(Point *startpoint,
				   void *user_data,
				   Handle **handle1,
				   Handle **handle2);
static void objet_destroy(Objet *pkg);
static Object *objet_copy(Objet *pkg);
static void objet_save(Objet *pkg, ObjectNode obj_node,
		       const char *filename);
static Object *objet_load(ObjectNode obj_node, int version,
			  const char *filename);
static void objet_update_data(Objet *pkg);
static GtkWidget *objet_get_properties(Objet *dep);
static void objet_apply_properties(Objet *dep);

static ObjectTypeOps objet_type_ops =
{
  (CreateFunc) objet_create,
  (LoadFunc)   objet_load,
  (SaveFunc)   objet_save
};

/* Non-nice typo, needed for backwards compatibility. */
ObjectType objet_type =
{
  "UML - Objet",   /* name */
  0,                      /* version */
  (char **) object_xpm,  /* pixmap */
  
  &objet_type_ops       /* ops */
};

ObjectType umlobject_type =
{
  "UML - Object",   /* name */
  0,                      /* version */
  (char **) object_xpm,  /* pixmap */
  
  &objet_type_ops       /* ops */
};

SheetObject objet_sheetobj =
{
  "UML - Object",             /* type */
  N_("Create an object"),           /* description */
  (char **) object_xpm,     /* pixmap */

  NULL                        /* user_data */
};

static ObjectOps objet_ops = {
  (DestroyFunc)         objet_destroy,
  (DrawFunc)            objet_draw,
  (DistanceFunc)        objet_distance_from,
  (SelectFunc)          objet_select,
  (CopyFunc)            objet_copy,
  (MoveFunc)            objet_move,
  (MoveHandleFunc)      objet_move_handle,
  (GetPropertiesFunc)   objet_get_properties,
  (ApplyPropertiesFunc) objet_apply_properties,
  (IsEmptyFunc)         object_return_false,
  (ObjectMenuFunc)      NULL
};

static real
objet_distance_from(Objet *pkg, Point *point)
{
  Object *obj = &pkg->element.object;
  return distance_rectangle_point(&obj->bounding_box, point);
}

static void
objet_select(Objet *pkg, Point *clicked_point,
	       Renderer *interactive_renderer)
{
  text_set_cursor(pkg->text, clicked_point, interactive_renderer);
  text_grab_focus(pkg->text, (Object *)pkg);
  element_update_handles(&pkg->element);
}

static void
objet_move_handle(Objet *pkg, Handle *handle,
			 Point *to, HandleMoveReason reason, ModifierKeys modifiers)
{
  assert(pkg!=NULL);
  assert(handle!=NULL);
  assert(to!=NULL);

  assert(handle->id < 8);
}

static void
objet_move(Objet *pkg, Point *to)
{
  pkg->element.corner = *to;
  objet_update_data(pkg);
}

static void
objet_draw(Objet *pkg, Renderer *renderer)
{
  Element *elem;
  real bw, x, y, w, h;
  Point p1, p2;
  int i;
  
  assert(pkg != NULL);
  assert(renderer != NULL);

  elem = &pkg->element;

  x = elem->corner.x;
  y = elem->corner.y;
  w = elem->width;
  h = elem->height;
  
  bw = (pkg->is_active) ? OBJET_ACTIVEBORDERWIDTH: OBJET_BORDERWIDTH;

  renderer->ops->set_fillstyle(renderer, FILLSTYLE_SOLID);
  renderer->ops->set_linewidth(renderer, bw);
  renderer->ops->set_linestyle(renderer, LINESTYLE_SOLID);


  p1.x = x; p1.y = y;
  p2.x = x+w; p2.y = y+h;

  if (pkg->is_multiple) {
    p1.x += OBJET_MARGIN_M;
    p2.y -= OBJET_MARGIN_M;
    renderer->ops->fill_rect(renderer, 
			     &p1, &p2,
			     &color_white); 
    renderer->ops->draw_rect(renderer, 
			     &p1, &p2,
			     &color_black);
    p1.x -= OBJET_MARGIN_M;
    p1.y += OBJET_MARGIN_M;
    p2.x -= OBJET_MARGIN_M;
    p2.y += OBJET_MARGIN_M;
    y += OBJET_MARGIN_M;
  }
    
  renderer->ops->fill_rect(renderer, 
			   &p1, &p2,
			   &color_white);
  renderer->ops->draw_rect(renderer, 
			   &p1, &p2,
			   &color_black);

  
  text_draw(pkg->text, renderer);

  if (pkg->stereotype != NULL) {
      renderer->ops->draw_string(renderer,
				 pkg->stereotype,
				 &pkg->st_pos, ALIGN_CENTER,
				 &color_black);
  }

  if (pkg->exstate != NULL) {
      renderer->ops->draw_string(renderer,
				 pkg->exstate,
				 &pkg->ex_pos, ALIGN_CENTER,
				 &color_black);
  }

  /* Is there a better way to underline? */
  p1.x = x + (w - pkg->text->max_width)/2;
  p1.y = pkg->text->position.y + pkg->text->descent;
  p2.x = p1.x + pkg->text->max_width;
  p2.y = p1.y;
  
  renderer->ops->set_linewidth(renderer, OBJET_LINEWIDTH);
    
  for (i=0; i<pkg->text->numlines; i++) { 
    p1.x = x + (w - pkg->text->row_width[i])/2;
    p2.x = p1.x + pkg->text->row_width[i];
    renderer->ops->draw_line(renderer,
			     &p1, &p2,
			     &color_black);
    p1.y = p2.y += pkg->text->height;
  }

  if (pkg->show_attributes) {
      p1.x = x; p2.x = x + w;
      p1.y = p2.y = pkg->attributes->position.y - pkg->attributes->ascent - OBJET_MARGIN_Y;
      
      renderer->ops->set_linewidth(renderer, bw);
      renderer->ops->draw_line(renderer,
			       &p1, &p2,
			       &color_black);

      text_draw(pkg->attributes, renderer);
  }
}

static void
objet_update_data(Objet *pkg)
{
  Element *elem = &pkg->element;
  Object *obj = (Object *) pkg;
  Font *font;
  Point p1, p2;
  real h, w = 0;
  
  font = pkg->text->font;
  h = elem->corner.y + OBJET_MARGIN_Y;

  if (pkg->is_multiple) {
    h += OBJET_MARGIN_M;
  }
    
  if (pkg->stereotype != NULL) {
      w = font_string_width(pkg->stereotype, font, OBJET_FONTHEIGHT);
      h += OBJET_FONTHEIGHT;
      pkg->st_pos.y = h;
      h += OBJET_MARGIN_Y/2.0;
  }

  w = MAX(w, pkg->text->max_width);
  p1.y = h + pkg->text->ascent;  /* position of text */

  h += pkg->text->height*pkg->text->numlines;

  if (pkg->exstate != NULL) {
      w = MAX(w, font_string_width(pkg->exstate, font, OBJET_FONTHEIGHT));
      h += OBJET_FONTHEIGHT;
      pkg->ex_pos.y = h;
  }
  
  h += OBJET_MARGIN_Y;

  if (pkg->show_attributes) {
      h += OBJET_MARGIN_Y + pkg->attributes->ascent;
      p2.x = elem->corner.x + OBJET_MARGIN_X;
      p2.y = h;      
      text_set_position(pkg->attributes, &p2);

      h += pkg->attributes->height*pkg->attributes->numlines; 

      w = MAX(w, pkg->attributes->max_width);
  }

  w += 2*OBJET_MARGIN_X; 

  p1.x = elem->corner.x + w/2.0;
  text_set_position(pkg->text, &p1);
  
  pkg->ex_pos.x = pkg->st_pos.x = p1.x;

  
  if (pkg->is_multiple) {
    w += OBJET_MARGIN_M;
  }
    
  elem->width = w;
  elem->height = h - elem->corner.y;

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
  /* fix boundingobjet for line width and top rectangle: */
  obj->bounding_box.top -= OBJET_BORDERWIDTH/2.0;
  obj->bounding_box.left -= OBJET_BORDERWIDTH/2.0;
  obj->bounding_box.bottom += OBJET_BORDERWIDTH/2.0;
  obj->bounding_box.right += OBJET_BORDERWIDTH/2.0;

  obj->position = elem->corner;

  element_update_handles(elem);
}

static Object *
objet_create(Point *startpoint,
		    void *user_data,
		    Handle **handle1,
		    Handle **handle2)
{
  Objet *pkg;
  Element *elem;
  Object *obj;
  Point p;
  Font *font;
  int i;
  
  pkg = g_malloc(sizeof(Objet));
  elem = &pkg->element;
  obj = (Object *) pkg;
  
  obj->type = &umlobject_type;

  obj->ops = &objet_ops;

  elem->corner = *startpoint;

  font = font_getfont("Helvetica");
  
  pkg->show_attributes = FALSE;
  pkg->is_active = FALSE;
  pkg->is_multiple = FALSE;

  pkg->exstate = NULL;
  pkg->stereotype = NULL;

  /* The text position is recalculated later */
  p.x = 0.0;
  p.y = 0.0;
  pkg->attributes = new_text("", font, 0.8, &p, &color_black, ALIGN_LEFT);
  pkg->text = new_text("", font, 0.8, &p, &color_black, ALIGN_CENTER);
  
  element_init(elem, 8, 8);
  
  for (i=0;i<8;i++) {
    obj->connections[i] = &pkg->connections[i];
    pkg->connections[i].object = obj;
    pkg->connections[i].connected = NULL;
  }
  objet_update_data(pkg);

  for (i=0;i<8;i++) {
    obj->handles[i]->type = HANDLE_NON_MOVABLE;
  }

  *handle1 = NULL;
  *handle2 = NULL;

  return (Object *)pkg;
}

static void
objet_destroy(Objet *pkg)
{
  text_destroy(pkg->text);

  if (pkg->stereotype != NULL)
      g_free(pkg->stereotype);

  if (pkg->exstate != NULL)
      g_free(pkg->exstate);

  text_destroy(pkg->attributes);

  element_destroy(&pkg->element);
}

static Object *
objet_copy(Objet *pkg)
{
  int i;
  Objet *newpkg;
  Element *elem, *newelem;
  Object *newobj;
  
  elem = &pkg->element;
  
  newpkg = g_malloc(sizeof(Objet));
  newelem = &newpkg->element;
  newobj = (Object *) newpkg;

  element_copy(elem, newelem);

  newpkg->text = text_copy(pkg->text);
  
  for (i=0;i<8;i++) {
    newobj->connections[i] = &newpkg->connections[i];
    newpkg->connections[i].object = newobj;
    newpkg->connections[i].connected = NULL;
    newpkg->connections[i].pos = pkg->connections[i].pos;
    newpkg->connections[i].last_pos = pkg->connections[i].last_pos;
  }

  newpkg->stereotype = (pkg->stereotype != NULL) ? 
      strdup(pkg->stereotype): NULL;

  newpkg->exstate = (pkg->exstate != NULL) ? 
      strdup(pkg->exstate): NULL;

  newpkg->attributes = text_copy(pkg->attributes);

  newpkg->is_active = pkg->is_active;
  newpkg->show_attributes = pkg->show_attributes;
  newpkg->is_multiple = pkg->is_multiple;
  
  objet_update_data(newpkg);
  
  return (Object *)newpkg;
}


static void
objet_save(Objet *pkg, ObjectNode obj_node, const char *filename)
{
  element_save(&pkg->element, obj_node);

  data_add_text(new_attribute(obj_node, "text"),
		pkg->text);

  data_add_string(new_attribute(obj_node, "stereotype"),
		  pkg->stereotype);

  data_add_string(new_attribute(obj_node, "exstate"),
		  pkg->exstate);

  data_add_text(new_attribute(obj_node, "attrib"),
		pkg->attributes);
    
  data_add_boolean(new_attribute(obj_node, "is_active"),
		   pkg->is_active);
  
  data_add_boolean(new_attribute(obj_node, "show_attribs"),
		   pkg->show_attributes);

  data_add_boolean(new_attribute(obj_node, "multiple"),
		   pkg->is_multiple);
}

static Object *
objet_load(ObjectNode obj_node, int version, const char *filename)
{
  Objet *pkg;
  AttributeNode attr;
  Element *elem;
  Object *obj;
  int i;
  
  pkg = g_malloc(sizeof(Objet));
  elem = &pkg->element;
  obj = (Object *) pkg;
  
  obj->type = &objet_type;
  obj->ops = &objet_ops;

  element_load(elem, obj_node);
  
  pkg->text = NULL;
  attr = object_find_attribute(obj_node, "text");
  if (attr != NULL)
    pkg->text = data_text(attribute_first_data(attr));

  pkg->stereotype = NULL;
  attr = object_find_attribute(obj_node, "stereotype");
  if (attr != NULL)
      pkg->stereotype = data_string(attribute_first_data(attr));

  pkg->exstate = NULL;
  attr = object_find_attribute(obj_node, "exstate");
  if (attr != NULL)
      pkg->exstate = data_string(attribute_first_data(attr));

  pkg->attributes = NULL;
  attr = object_find_attribute(obj_node, "attrib");
  if (attr != NULL)
    pkg->attributes = data_text(attribute_first_data(attr));

  attr = object_find_attribute(obj_node, "is_active");
  if (attr != NULL)
    pkg->is_active = data_boolean(attribute_first_data(attr));
  else
    pkg->is_active = FALSE;

  attr = object_find_attribute(obj_node, "show_attribs");
  if (attr != NULL)
    pkg->show_attributes = data_boolean(attribute_first_data(attr));
  else
    pkg->show_attributes = FALSE;
  
  attr = object_find_attribute(obj_node, "multiple");
  if (attr != NULL)
    pkg->is_multiple = data_boolean(attribute_first_data(attr));
  else
    pkg->is_multiple = FALSE;

  element_init(elem, 8, 8);

  for (i=0;i<8;i++) {
    obj->connections[i] = &pkg->connections[i];
    pkg->connections[i].object = obj;
    pkg->connections[i].connected = NULL;
  }
  objet_update_data(pkg);

  for (i=0;i<8;i++) {
    obj->handles[i]->type = HANDLE_NON_MOVABLE;
  }

  return (Object *) pkg;
}


static void
objet_apply_properties(Objet *dep)
{
  ObjetPropertiesDialog *prop_dialog;
  char *str;

  prop_dialog = properties_dialog;

  /* Read from dialog and put in object: */
  if (dep->exstate != NULL)
    g_free(dep->exstate);
  str = gtk_entry_get_text(prop_dialog->name);
  if (strlen(str) != 0) {
      dep->exstate = g_malloc(sizeof(char)*strlen(str)+2+1);
      sprintf(dep->exstate, "[%s]", str);
  } else
    dep->exstate = NULL;

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

  dep->is_active = prop_dialog->active->active;
  dep->show_attributes = prop_dialog->show_attrib->active;
  dep->is_multiple = prop_dialog->multiple->active;

  
  text_set_string(dep->attributes,
		  gtk_editable_get_chars( GTK_EDITABLE(prop_dialog->attribs),
						 0, -1));

  objet_update_data(dep);
}

static void
fill_in_dialog(Objet *dep)
{
  ObjetPropertiesDialog *prop_dialog;
  char *str;
  
  prop_dialog = properties_dialog;

  if (dep->exstate != NULL) {
      str = strdup(dep->exstate+1);
      str[strlen(str)-1] = 0;
      gtk_entry_set_text(prop_dialog->name, str);
      g_free(str);
  } else 
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

  gtk_toggle_button_set_active(prop_dialog->show_attrib, dep->show_attributes);
  gtk_toggle_button_set_active(prop_dialog->active, dep->is_active);
  gtk_toggle_button_set_active(prop_dialog->multiple, dep->is_multiple);


  gtk_text_freeze(GTK_TEXT(prop_dialog->attribs));
  gtk_text_set_point(GTK_TEXT(prop_dialog->attribs), 0);
  gtk_text_forward_delete( GTK_TEXT(prop_dialog->attribs), gtk_text_get_length(GTK_TEXT(prop_dialog->attribs)));
  gtk_text_insert( GTK_TEXT(prop_dialog->attribs),
		   NULL, NULL, NULL,
		   text_get_string_copy(dep->attributes),
		   -1);
  gtk_text_thaw(GTK_TEXT(prop_dialog->attribs));
}

static GtkWidget *
objet_get_properties(Objet *dep)
{
  ObjetPropertiesDialog *prop_dialog;
  GtkWidget *dialog;
  GtkWidget *checkbox;
  GtkWidget *entry;
  GtkWidget *hbox;
  GtkWidget *label;

  if (properties_dialog == NULL) {

    prop_dialog = g_new(ObjetPropertiesDialog, 1);
    properties_dialog = prop_dialog;

    dialog = gtk_vbox_new(FALSE, 0);
    prop_dialog->dialog = dialog;
    
    hbox = gtk_hbox_new(FALSE, 5);

    label = gtk_label_new(_("Explicit state:"));
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
    
    label = gtk_label_new(_("Attributes:"));
    gtk_box_pack_start (GTK_BOX (dialog), label, FALSE, TRUE, 0);
    entry = gtk_text_new(NULL, NULL);
    prop_dialog->attribs = entry;
    gtk_text_set_editable(GTK_TEXT(entry), TRUE);
    gtk_box_pack_start (GTK_BOX (dialog), entry, TRUE, TRUE, 0);
    gtk_widget_show (label);
    gtk_widget_show (entry);
        
    hbox = gtk_hbox_new(FALSE, 5);
    checkbox = gtk_check_button_new_with_label(_("Show attributes"));
    gtk_box_pack_start (GTK_BOX (hbox), checkbox, FALSE, TRUE, 0);
    prop_dialog->show_attrib = GTK_TOGGLE_BUTTON( checkbox );
    gtk_widget_show(checkbox);
    checkbox = gtk_check_button_new_with_label(_("Active object"));
    gtk_box_pack_start (GTK_BOX (hbox), checkbox, FALSE, TRUE, 0);
    prop_dialog->active = GTK_TOGGLE_BUTTON( checkbox );
    gtk_widget_show(checkbox);
      
    checkbox = gtk_check_button_new_with_label(_("multiple instance"));
    gtk_box_pack_start (GTK_BOX (hbox), checkbox, FALSE, TRUE, 0);
    prop_dialog->multiple = GTK_TOGGLE_BUTTON( checkbox );
    gtk_widget_show(checkbox);
      
    gtk_widget_show(hbox);
    gtk_box_pack_start (GTK_BOX (dialog), hbox, TRUE, TRUE, 0);

  }
  
  fill_in_dialog(dep);
  gtk_widget_show (properties_dialog->dialog);

  return properties_dialog->dialog;
}

