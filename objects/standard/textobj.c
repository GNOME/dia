/* xxxxxx -- an diagram creation/manipulation program
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

#include "object.h"
#include "connectionpoint.h"
#include "render.h"
#include "font.h"
#include "text.h"
#include "attributes.h"
#include "files.h"

#include "pixmaps/text.xpm"

#define HANDLE_TEXT HANDLE_CUSTOM1

typedef struct _Textobj Textobj;
typedef struct _TextobjPropertiesDialog TextobjPropertiesDialog;

struct _Textobj {
  Object object;
  
  Handle text_handle;

  Text *text;

  TextobjPropertiesDialog *properties_dialog;
};

struct _TextobjPropertiesDialog {
  GtkWidget *dialog;
  
  GtkMenu *menu;
  
  ObjectChangedFunc *changed_callback;
  void *changed_callback_data;
};

static real textobj_distance_from(Textobj *textobj, Point *point);
static void textobj_select(Textobj *textobj, Point *clicked_point,
			   Renderer *interactive_renderer);
static void textobj_move_handle(Textobj *textobj, Handle *handle,
				Point *to, HandleMoveReason reason);
static void textobj_move(Textobj *textobj, Point *to);
static void textobj_draw(Textobj *textobj, Renderer *renderer);
static void textobj_update_data(Textobj *textobj);
static Object *textobj_create(Point *startpoint,
			      void *user_data,
			      Handle **handle1,
			      Handle **handle2);
static void textobj_destroy(Textobj *textobj);
static Object *textobj_copy(Textobj *textobj);
static void textobj_show_properties(Textobj *textobj, ObjectChangedFunc *changed_callback,
				    void *changed_callback_data);

static void textobj_save(Textobj *textobj, int fd);
static Object *textobj_load(int fd, int version);
static int textobj_is_empty(Textobj *textobj);

static ObjectTypeOps textobj_type_ops =
{
  (CreateFunc) textobj_create,
  (LoadFunc)   textobj_load,
  (SaveFunc)   textobj_save
};

ObjectType textobj_type =
{
  "Standard - Text",   /* name */
  0,                   /* version */
  (char **) text_xpm,  /* pixmap */

  &textobj_type_ops    /* ops */
};

ObjectType *_textobj_type = (ObjectType *) &textobj_type;

static ObjectOps textobj_ops = {
  (DestroyFunc)        textobj_destroy,
  (DrawFunc)           textobj_draw,
  (DistanceFunc)       textobj_distance_from,
  (SelectFunc)         textobj_select,
  (CopyFunc)           textobj_copy,
  (MoveFunc)           textobj_move,
  (MoveHandleFunc)     textobj_move_handle,
  (ShowPropertiesFunc) textobj_show_properties,
  (IsEmptyFunc)        textobj_is_empty
};


static void
apply_callback(GtkWidget *widget, gpointer data)
{
  Textobj *textobj;
  TextobjPropertiesDialog *prop_dialog;
  Alignment *align;
  GtkWidget *menuitem;
  
  textobj = (Textobj *)data;
  prop_dialog = textobj->properties_dialog;

  (prop_dialog->changed_callback)((Object *)textobj, prop_dialog->changed_callback_data, BEFORE_CHANGE);

  menuitem = gtk_menu_get_active(prop_dialog->menu);
  align = gtk_object_get_user_data(GTK_OBJECT(menuitem));
  text_set_alignment(textobj->text, *align);
  textobj_update_data(textobj);

  (prop_dialog->changed_callback)((Object *)textobj, prop_dialog->changed_callback_data, AFTER_CHANGE);
}

static void
textobj_show_properties(Textobj *textobj, ObjectChangedFunc *changed_callback,
			void *changed_callback_data)
{
  TextobjPropertiesDialog *prop_dialog;
  GtkWidget *dialog;
  GtkWidget *button;
  GtkWidget *omenu;
  GtkWidget *menu;
  GtkWidget *submenu;
  GtkWidget *menuitem;
  GtkWidget *hbox;
  GtkWidget *label;
  GSList *group;
  static Alignment center=ALIGN_CENTER, left=ALIGN_LEFT, right=ALIGN_RIGHT;

  if (textobj->properties_dialog == NULL) {

    prop_dialog = g_new(TextobjPropertiesDialog, 1);
    textobj->properties_dialog = prop_dialog;

    prop_dialog->changed_callback = changed_callback;
    prop_dialog->changed_callback_data = changed_callback_data;
    
    dialog = gtk_dialog_new();
    prop_dialog->dialog = dialog;
    
    gtk_signal_connect (GTK_OBJECT (dialog), "delete_event",
			GTK_SIGNAL_FUNC(gtk_widget_hide), NULL);
    
    gtk_window_set_title (GTK_WINDOW (dialog), "Text properties");
    gtk_container_border_width (GTK_CONTAINER (dialog), 5);

    omenu = gtk_option_menu_new ();
    menu = gtk_menu_new ();
    prop_dialog->menu = GTK_MENU(menu);
    submenu = NULL;
    group = NULL;
    
    menuitem = gtk_radio_menu_item_new_with_label (group, "Center");
    gtk_object_set_user_data(GTK_OBJECT(menuitem), &center);
    gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
			GTK_SIGNAL_FUNC(apply_callback),
			textobj);
    group = gtk_radio_menu_item_group (GTK_RADIO_MENU_ITEM (menuitem));
    gtk_menu_append (GTK_MENU (menu), menuitem);
    gtk_widget_show (menuitem);
    
    menuitem = gtk_radio_menu_item_new_with_label (group, "Left");
    gtk_object_set_user_data(GTK_OBJECT(menuitem), &left);
    gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
			GTK_SIGNAL_FUNC(apply_callback),
			textobj);
    group = gtk_radio_menu_item_group (GTK_RADIO_MENU_ITEM (menuitem));
    gtk_menu_append (GTK_MENU (menu), menuitem);
    gtk_widget_show (menuitem);
    
    menuitem = gtk_radio_menu_item_new_with_label (group, "Right");
    gtk_object_set_user_data(GTK_OBJECT(menuitem), &right);
    gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
			GTK_SIGNAL_FUNC(apply_callback),
			textobj);
    group = gtk_radio_menu_item_group (GTK_RADIO_MENU_ITEM (menuitem));
    gtk_menu_append (GTK_MENU (menu), menuitem);
    gtk_widget_show (menuitem);
    
    gtk_option_menu_set_menu (GTK_OPTION_MENU (omenu), menu);

    hbox = gtk_hbox_new(FALSE, 5);

    label = gtk_label_new("Alignment:");
    gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (hbox), omenu, TRUE, TRUE, 0);
    
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), 
			hbox, TRUE, TRUE, 0);

    gtk_widget_show (label);
    gtk_widget_show (omenu);
    gtk_widget_show(hbox);
    
    button = gtk_button_new_with_label ("Apply");
    GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->action_area), 
			button, TRUE, TRUE, 0);
    gtk_signal_connect (GTK_OBJECT (button), "clicked",
			GTK_SIGNAL_FUNC(apply_callback),
			textobj);
    gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
			GTK_SIGNAL_FUNC(gtk_widget_hide),
			GTK_OBJECT(dialog));
    gtk_widget_grab_default (button);
    gtk_widget_show (button);
    gtk_widget_show (dialog);
  } else {
    textobj->properties_dialog->changed_callback = changed_callback;
    textobj->properties_dialog->changed_callback_data = changed_callback_data;

    /* TODO: Set the active menu here! */
    gtk_widget_show (textobj->properties_dialog->dialog);
  }
}


static real
textobj_distance_from(Textobj *textobj, Point *point)
{
  return text_distance_from(textobj->text, point); 
}

static void
textobj_select(Textobj *textobj, Point *clicked_point,
	       Renderer *interactive_renderer)
{
  text_set_cursor(textobj->text, clicked_point, interactive_renderer);
  text_grab_focus(textobj->text, (Object *)textobj);
}

static void
textobj_move_handle(Textobj *textobj, Handle *handle,
		    Point *to, HandleMoveReason reason)
{
  assert(textobj!=NULL);
  assert(handle!=NULL);
  assert(to!=NULL);

  if (handle->id == HANDLE_TEXT) {
    text_set_position(textobj->text, to);
  }
  
  textobj_update_data(textobj);
}

static void
textobj_move(Textobj *textobj, Point *to)
{
  text_set_position(textobj->text, to);
  
  textobj_update_data(textobj);
}

static void
textobj_draw(Textobj *textobj, Renderer *renderer)
{
  assert(textobj != NULL);
  assert(renderer != NULL);

  text_draw(textobj->text, renderer);
}

static void
textobj_update_data(Textobj *textobj)
{
  Object *obj = &textobj->object;
  
  obj->position = textobj->text->position;
  
  text_calc_boundingbox(textobj->text, &obj->bounding_box);
  
  textobj->text_handle.pos = textobj->text->position;
}

static int
textobj_is_empty(Textobj *textobj)
{
  return text_is_empty(textobj->text);
}

static Object *
textobj_create(Point *startpoint,
	       void *user_data,
	       Handle **handle1,
	       Handle **handle2)
{
  Textobj *textobj;
  Object *obj;
  Color col;
  
  textobj = g_malloc(sizeof(Textobj));
  obj = &textobj->object;
  
  obj->type = &textobj_type;

  obj->ops = &textobj_ops;


  col = attributes_get_foreground();
  textobj->text = new_text("", font_getfont("Courier"), 1.0,
			   startpoint, &col, ALIGN_CENTER );
  
  textobj->properties_dialog = NULL;
  
  object_init(obj, 1, 0);

  obj->handles[0] = &textobj->text_handle;
  textobj->text_handle.id = HANDLE_TEXT;
  textobj->text_handle.type = HANDLE_MAJOR_CONTROL;
  textobj->text_handle.connectable = TRUE;
  textobj->text_handle.connected_to = NULL;

  textobj_update_data(textobj);

  *handle1 = NULL;
  *handle2 = obj->handles[0];
  return (Object *)textobj;
}

static void
textobj_destroy(Textobj *textobj)
{
  if (textobj->properties_dialog != NULL) {
    gtk_widget_destroy(textobj->properties_dialog->dialog);
    g_free(textobj->properties_dialog);
  }
  text_destroy(textobj->text);
  object_destroy(&textobj->object);
}

static Object *
textobj_copy(Textobj *textobj)
{
  Textobj *newtext;
  Object *obj, *newobj;
  
  obj = &textobj->object;
  
  newtext = g_malloc(sizeof(Textobj));
  newobj = &newtext->object;

  object_copy(obj, newobj);

  newtext->text = text_copy(textobj->text);

  newobj->handles[0] = &newtext->text_handle;
  
  newtext->text_handle = textobj->text_handle;
  newtext->text_handle.connected_to = NULL;

  newtext->properties_dialog = NULL;

  return (Object *)newtext;
}

static void
textobj_save(Textobj *textobj, int fd)
{
  object_save(&textobj->object, fd);

  write_text(fd, textobj->text);
}

static Object *
textobj_load(int fd, int version)
{
  Textobj *textobj;
  Object *obj;

  textobj = g_malloc(sizeof(Textobj));
  obj = &textobj->object;
  
  obj->type = &textobj_type;
  obj->ops = &textobj_ops;

  object_load(obj, fd);

  textobj->text = read_text(fd);
  
  textobj->properties_dialog = NULL;
  
  object_init(obj, 1, 0);

  obj->handles[0] = &textobj->text_handle;
  textobj->text_handle.id = HANDLE_TEXT;
  textobj->text_handle.type = HANDLE_MINOR_CONTROL;
  textobj->text_handle.connectable = TRUE;
  textobj->text_handle.connected_to = NULL;

  textobj_update_data(textobj);

  return (Object *)textobj;
}
