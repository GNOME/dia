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
#include <stdio.h>
#include <string.h>

#include <gtk/gtk.h>

#include "object.h"
#include "files.h"
#include "message.h"

#include "dummy_dep.h"

void
object_init(Object *obj,
	    int num_handles,
	    int num_connections)
{
  obj->num_handles = num_handles;
  if (num_handles>0)
    obj->handles = g_malloc(sizeof(Handle *)*num_handles);
  else
    obj->handles = NULL;

  obj->num_connections = num_connections;
  if (num_connections>0)
    obj->connections = g_malloc(sizeof(ConnectionPoint *) * num_connections);
  else
    obj->connections = NULL;
}

void
object_destroy(Object *obj)
{
  object_unconnect_all(obj);

  if (obj->handles)
    g_free(obj->handles);

  if (obj->connections)
    g_free(obj->connections);

}

/* After this copying you have to fix up:
   handles
   connections
*/
void
object_copy(Object *from, Object *to)
{
  to->type = from->type;
  to->position = from->position;
  to->bounding_box = from->bounding_box;
  
  to->num_handles = from->num_handles;
  if (to->num_handles>0)
    to->handles = g_malloc(sizeof(Handle *)*to->num_handles);
  else
    to->handles = NULL;

  to->num_connections = from->num_connections;
  if (to->num_connections>0)
    to->connections = g_malloc(sizeof(ConnectionPoint *) * to->num_connections);
  else
    to->connections = NULL;

  to->ops = from->ops;
}

void
object_add_handle(Object *obj, Handle *handle)
{
  obj->num_handles++;

  obj->handles =
    g_realloc(obj->handles, obj->num_handles*sizeof(Handle *));
  
  obj->handles[obj->num_handles-1] = handle;
}

void
object_remove_handle(Object *obj, Handle *handle)
{
  int i, handle_nr;

  handle_nr = -1;
  for (i=0;i<obj->num_handles;i++) {
    if (obj->handles[i] == handle)
      handle_nr = i;
  }

  if (handle_nr < 0) {
    message_error("Internal error, object_remove_handle: Handle doesn't exist");
    return;
  }

  for (i=handle_nr;i<(obj->num_handles-1);i++) {
    obj->handles[i] = obj->handles[i+1];
  }
  obj->handles[obj->num_handles-1] = NULL;
    
  obj->num_handles--;

  obj->handles =
    g_realloc(obj->handles, obj->num_handles*sizeof(Handle *));
}


void
object_connect(Object *obj, Handle *handle,
	       ConnectionPoint *connectionpoint)
{
  if (!handle->connectable) {
    message_error("Error? trying to connect a non conectable handle.\n"
		  "Check this out...\n");
    return;
  }
  handle->connected_to = connectionpoint;
  connectionpoint->connected =
    g_list_prepend(connectionpoint->connected, obj);
  
  printf("*** Made connection\n");
}

void
object_unconnect(Object *connected_obj, Handle *handle)
{
  ConnectionPoint *connectionpoint;

  connectionpoint = handle->connected_to;

  if (connectionpoint!=NULL) {
    connectionpoint->connected =
      g_list_remove(connectionpoint->connected, connected_obj);
    handle->connected_to = NULL;
    printf("*** Broke connection!\n");
  }
}

void
object_remove_connections_to(ConnectionPoint *conpoint)
{
  GList *list;
  Object *connected_obj;
  int i;
  
  list = conpoint->connected;
  while (list != NULL) {
    connected_obj = (Object *)list->data;

    for (i=0;i<connected_obj->num_handles;i++) {
      if (connected_obj->handles[i]->connected_to == conpoint) {
	connected_obj->handles[i]->connected_to = NULL;
	printf("*** Broke connection!\n");
      }
    }
    list = g_list_next(list);
  }
  g_list_free(conpoint->connected);
  conpoint->connected = NULL;
}

void
object_unconnect_all(Object *obj)
{
  int i;
  
  for (i=0;i<obj->num_handles;i++) {
    object_unconnect(obj, obj->handles[i]);
  }
  for (i=0;i<obj->num_connections;i++) {
    object_remove_connections_to(obj->connections[i]);
  }
}

void object_save(Object *obj, int fd)
{
  write_point(fd, &obj->position);
  write_rectangle(fd, &obj->bounding_box);
}

void object_load(Object *obj, int fd)
{
  read_point(fd, &obj->position);
  read_rectangle(fd, &obj->bounding_box);
}

/****** Object register: **********/

static guint hash(gpointer key)
{
  char *string = (char *)key;
  int sum;

  sum = 0;
  while (*string) {
    sum += (*string);
    string++;
  }

  return sum;
}

static gint compare(gpointer a, gpointer b)
{
  return strcmp((char *)a, (char *)b)==0;
}

static GHashTable *object_type_table = NULL;

void
object_registry_init(void)
{
  object_type_table = g_hash_table_new( (GHashFunc) hash, (GCompareFunc) compare );
}

void
object_register_type(ObjectType *type)
{
  if (g_hash_table_lookup(object_type_table, type->name) != NULL) {
    message_warning("Several object-types was named %s.\n"
		    "Only first one will be used.\n"
		    "Some things might not work as expected.\n",
		    type->name);
    return;
  }
  g_hash_table_insert(object_type_table, type->name, type);
}

ObjectType *
object_get_type(char *name)
{
  return (ObjectType *) g_hash_table_lookup(object_type_table, name);
}


int
object_return_false(Object *obj)
{
  return FALSE;
}

void
object_return_void(Object *obj)
{
  return;
}

void
object_show_properties_none(Object *obj,
			    ObjectChangedFunc *changed_callback,
			    void *changed_callback_data)
{
  GtkWidget *dialog;
  GtkWidget *button;
  GtkWidget *label;

  dialog = gtk_dialog_new();
  
  gtk_window_set_title (GTK_WINDOW (dialog), "Object properties");
  gtk_container_border_width (GTK_CONTAINER (dialog), 5);

  label = gtk_label_new("This object has no properties.\n");
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), label,
		      TRUE, TRUE, 0);
  gtk_widget_show (label);
    
  button = gtk_button_new_with_label ("Apply");
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->action_area), 
		      button, TRUE, TRUE, 0);
  gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
			     GTK_SIGNAL_FUNC(gtk_widget_destroy),
			     GTK_OBJECT(dialog));
  gtk_widget_grab_default (button);
  gtk_widget_show (button);
  gtk_widget_show (dialog);
}

void
object_show_properties_none_yet(Object *obj,
				ObjectChangedFunc *changed_callback,
				void *changed_callback_data)
{
  GtkWidget *dialog;
  GtkWidget *button;
  GtkWidget *label;

  dialog = gtk_dialog_new();
  
  gtk_window_set_title (GTK_WINDOW (dialog), "Object properties");
  gtk_container_border_width (GTK_CONTAINER (dialog), 5);

  label = gtk_label_new("This object has no properties yet.\n"
			"This is not finished code you know.");
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), label,
		      TRUE, TRUE, 0);
  gtk_widget_show (label);
    
  button = gtk_button_new_with_label ("Apply");
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->action_area), 
		      button, TRUE, TRUE, 0);
  gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
			     GTK_SIGNAL_FUNC(gtk_widget_destroy),
			     GTK_OBJECT(dialog));
  gtk_widget_grab_default (button);
  gtk_widget_show (button);
  gtk_widget_show (dialog);
}

