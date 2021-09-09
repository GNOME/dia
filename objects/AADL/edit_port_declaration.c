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

#include <gtk/gtk.h>
#include <string.h>
#include "aadl.h"
#include "edit_port_declaration.h"


int aadlbox_point_near_port(Aadlbox *aadlbox, Point *p);



/***********************************************
 **           U N D O  /  R E D O             **
 ***********************************************/

struct _DiaAADLEditPortDeclarationObjectChange {
  DiaObjectChange obj_change;

  int applied;

  int port_num;

  char *oldvalue;
  char *newvalue;
};


DIA_DEFINE_OBJECT_CHANGE (DiaAADLEditPortDeclarationObjectChange,
                          dia_aadl_edit_port_declaration_object_change)


static void
dia_aadl_edit_port_declaration_object_change_apply (DiaObjectChange *self,
                                                    DiaObject       *obj)
{
  DiaAADLEditPortDeclarationObjectChange *change = DIA_AADL_EDIT_PORT_DECLARATION_OBJECT_CHANGE (self);
  Aadlbox *aadlbox = (Aadlbox *) obj;
  int port_num = change->port_num;

  change->applied = 1;
  aadlbox->ports[port_num]->declaration = change->newvalue;

}


static void
dia_aadl_edit_port_declaration_object_change_revert (DiaObjectChange *self,
                                                     DiaObject       *obj)
{
  DiaAADLEditPortDeclarationObjectChange *change = DIA_AADL_EDIT_PORT_DECLARATION_OBJECT_CHANGE (self);
  Aadlbox *aadlbox = (Aadlbox *) obj;
  int port_num = change->port_num;

  change->applied = 0;
  aadlbox->ports[port_num]->declaration = change->oldvalue;
}


static void
dia_aadl_edit_port_declaration_object_change_free (DiaObjectChange *self)
{
  DiaAADLEditPortDeclarationObjectChange *change = DIA_AADL_EDIT_PORT_DECLARATION_OBJECT_CHANGE (self);

  if (change->applied) {
    g_clear_pointer (&change->oldvalue, g_free);
  } else {
    g_clear_pointer (&change->newvalue, g_free);
  }
}



/***********************************************
 **          G T K    W I N D O W             **
 ***********************************************/

static GtkWidget *entry;
static char *text = NULL;


static void
save_text (void)
{
  g_clear_pointer (&text, g_free);
  text = g_strdup (gtk_entry_get_text (GTK_ENTRY (entry)));
}


static gboolean delete_event ( GtkWidget *widget,
			       GdkEvent  *event,
			       gpointer   data )
{
  save_text();
  return FALSE;
}


static void enter_callback( GtkWidget *widget,
                            GtkWidget *window )
{
  save_text();
  gtk_widget_destroy(window);
}

static gboolean focus_out_event( GtkWidget      *widget,
				 GdkEventButton *event,
				 GtkWidget *window )
{
  save_text();
  gtk_widget_destroy(window);
  return FALSE;
}


/* I have to write this little GTK code, because there's no way to know
   which point was clicked if I use the properties functions (get_props, ...)*/

DiaObjectChange *
edit_port_declaration_callback (DiaObject *obj,
                                Point     *clicked,
                                gpointer   data)
{
  GtkWidget *window;
  GtkWidget *vbox;
  GtkWidget *button;
  DiaAADLEditPortDeclarationObjectChange *change;
  Aadlport *port;
  Aadlbox *aadlbox = (Aadlbox *) obj;
  int port_num;

  gtk_init (0, NULL);

  port_num = aadlbox_point_near_port(aadlbox, clicked);
  port = aadlbox->ports[port_num];


  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_position (GTK_WINDOW (window), GTK_WIN_POS_CENTER);
  gtk_widget_set_size_request(window,400,50);
  gtk_window_set_title(GTK_WINDOW(window) , "Port Declaration");
  gtk_container_set_border_width(GTK_CONTAINER(window),5);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_container_add (GTK_CONTAINER (window), vbox);
  gtk_widget_show (vbox);

  entry = gtk_entry_new ();
  gtk_entry_set_max_length (GTK_ENTRY (entry), 1024);
  gtk_entry_set_text (GTK_ENTRY (entry), port->declaration);
  gtk_box_pack_start (GTK_BOX (vbox), entry, TRUE, TRUE, 0);
  gtk_widget_show(entry);

  button = gtk_button_new_with_mnemonic (_("_OK"));
  gtk_box_pack_start (GTK_BOX (vbox), button, TRUE, TRUE, 0);
  gtk_widget_set_can_default (GTK_WIDGET (button), TRUE);
  gtk_widget_grab_default (button);
  gtk_widget_show (button);

  g_signal_connect (G_OBJECT (window), "destroy",
		    G_CALLBACK (gtk_main_quit), NULL);

  g_signal_connect_swapped (G_OBJECT (window), "delete_event",
			    G_CALLBACK(delete_event),
			    G_OBJECT (window) );

  g_signal_connect (G_OBJECT (entry), "activate",
		    G_CALLBACK (enter_callback),
		    (gpointer) window);

  g_signal_connect (G_OBJECT (button), "clicked",
		    G_CALLBACK (enter_callback),
		    (gpointer) window);


  /* avoid bug when quitting DIA with this window opened */
  g_signal_connect (G_OBJECT (window), "focus_out_event",
		    G_CALLBACK (focus_out_event),
		    (gpointer) window);


  gtk_widget_show (window);

  gtk_main();

  /* Text has been edited - widgets destroyed  */

  change = dia_object_change_new (DIA_AADL_TYPE_EDIT_PORT_DECLARATION_OBJECT_CHANGE);

  change->port_num = port_num;

  change->newvalue = text;
  change->oldvalue = aadlbox->ports[port_num]->declaration;

  dia_object_change_apply (DIA_OBJECT_CHANGE (change), obj);

  return DIA_OBJECT_CHANGE (change);
}
