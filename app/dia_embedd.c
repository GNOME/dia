/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 2000 Alexander Larsson
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

#include <gnome.h>
#include <bonobo.h>
#include <libgnorba/gnorba.h>

#include "display.h"
#include "menus.h"
#include "disp_callbacks.h"
#include "app_procs.h"

typedef struct _EmbeddedDia EmbeddedDia;
typedef struct _EmbeddedView EmbeddedView;

struct _EmbeddedDia {
  BonoboEmbeddable     *embeddable;

  Diagram *diagram;
};

struct _EmbeddedView {
  BonoboView	    *view;
  EmbeddedDia *embedded_dia;
  DDisplay *display;
};

static BonoboEmbeddableFactory *factory = NULL;
static int running_objects = 0;


static void
dia_view_activate(BonoboView *view, gboolean activate,
		  EmbeddedView *view_data)
{
  bonobo_view_activate_notify(view, activate);
  if (activate) {
    toolbox_show();
  } else {
    toolbox_hide();
  }
}

static void
dia_view_display_destroy (GtkWidget *ddisp_shell, EmbeddedView *view_data)
{
  view_data->display = NULL;
  bonobo_object_destroy(BONOBO_OBJECT(view_data->view));
}

static void
dia_view_destroy (BonoboView *view, EmbeddedView *view_data)
{
  if (view_data->display)
    ddisplay_destroy(view_data->display->shell, view_data->display);
  g_free(view_data);
}

static void
dia_embeddable_destroy(BonoboEmbeddable *embeddable,
		       EmbeddedDia *embedded_dia)
{
  Diagram *dia = embedded_dia->diagram;
  DDisplay *ddisp;

  /* Destroy all views: */
  while (dia->displays) {
    ddisp = (DDisplay *)dia->displays->data;
    gtk_widget_destroy(ddisp->shell);
  }

  diagram_destroy(embedded_dia->diagram);
  g_free(embedded_dia); 

  running_objects--;
  if (running_objects > 0)
    return;
  
  /* When last object has gone unref the factory & quit. */
  bonobo_object_unref(BONOBO_OBJECT(factory));
  gtk_main_quit();
}

static void
dia_view_system_exception(BonoboView *view, CORBA_Object corba_object,
			  CORBA_Environment *ev, gpointer data)
{
  bonobo_object_destroy(BONOBO_OBJECT(view));
}

static void
dia_embeddable_system_exception(BonoboEmbeddable *embeddable, CORBA_Object corba_object,
				CORBA_Environment *ev, gpointer data)
{
  bonobo_object_destroy(BONOBO_OBJECT(embeddable));
}

static BonoboView *
view_factory (BonoboEmbeddable *embeddable,
	      const Bonobo_ViewFrame view_frame,
	      EmbeddedDia *embedded_dia)
{
  EmbeddedView *view_data;
  BonoboView  *view;
  GtkWidget *menuitem;
#ifdef GNOME
  GtkWidget *parent;
  DDisplay *ddisp;
  int pos;
#endif

  
  /*
   * Create the private view data.
   */
  view_data = g_new0 (EmbeddedView, 1);
  view_data->embedded_dia = embedded_dia;
  
  view_data->display = new_display(embedded_dia->diagram);

  gtk_signal_connect (GTK_OBJECT (view_data->display->shell), "destroy",
                      GTK_SIGNAL_FUNC (dia_view_display_destroy),
                      view_data);
  
  view = bonobo_view_new (view_data->display->shell);
  view_data->view = view;
  gtk_object_set_data (GTK_OBJECT (view), "view_data", view_data);
  
  
  gtk_signal_connect(GTK_OBJECT (view), "activate",
		     GTK_SIGNAL_FUNC (dia_view_activate), view_data);
  
  gtk_signal_connect(GTK_OBJECT (view), "system_exception",
		     GTK_SIGNAL_FUNC (dia_view_system_exception), view_data);

  gtk_signal_connect (GTK_OBJECT (view), "destroy",
		      GTK_SIGNAL_FUNC (dia_view_destroy), view_data);


#ifdef GNOME
  /* Gross stuff: */
  ddisp = view_data->display;
  parent = gnome_app_find_menu_pos(ddisp->popup, "File/New diagram", &pos);
  menuitem = (GtkWidget *) g_list_nth (GTK_MENU_SHELL (parent)->children, pos-1)->data;
  gtk_widget_hide(menuitem);
  parent = gnome_app_find_menu_pos(ddisp->popup, "File/Open...", &pos);
  menuitem = (GtkWidget *) g_list_nth (GTK_MENU_SHELL (parent)->children, pos-1)->data;
  gtk_widget_hide(menuitem);
  parent = gnome_app_find_menu_pos(ddisp->popup, "File/Save As...", &pos);
  menuitem = (GtkWidget *) g_list_nth (GTK_MENU_SHELL (parent)->children, pos-1)->data;
  gtk_widget_hide(menuitem);
  parent = gnome_app_find_menu_pos(ddisp->popup, "File/Close", &pos);
  menuitem = (GtkWidget *) g_list_nth (GTK_MENU_SHELL (parent)->children, pos-1)->data;
  gtk_widget_hide(menuitem);
  parent = gnome_app_find_menu_pos(ddisp->popup, "File/Exit", &pos);
  menuitem = (GtkWidget *) g_list_nth (GTK_MENU_SHELL (parent)->children, pos-1)->data;
  gtk_widget_hide(menuitem);
  parent = gnome_app_find_menu_pos(ddisp->popup, "View/New View", &pos);
  menuitem = (GtkWidget *) g_list_nth (GTK_MENU_SHELL (parent)->children, pos-1)->data;
  gtk_widget_hide(menuitem);
#else
  menuitem = menus_get_item_from_path("<Display>/File/New");
  gtk_widget_hide(menuitem);
  menuitem = menus_get_item_from_path("<Display>/File/Open");
  gtk_widget_hide(menuitem);
  menuitem = menus_get_item_from_path("<Display>/File/Save As...");
  gtk_widget_hide(menuitem);
  menuitem = menus_get_item_from_path("<Display>/File/sep2");
  gtk_widget_hide(menuitem);
  menuitem = menus_get_item_from_path("<Display>/File/Close");
  gtk_widget_hide(menuitem);
  menuitem = menus_get_item_from_path("<Display>/File/Quit");
  gtk_widget_hide(menuitem);
  menuitem = menus_get_item_from_path("<Display>/View/New View");
  gtk_widget_hide(menuitem);
#endif
  
  return view;
}

static BonoboObject *
embeddable_factory (BonoboEmbeddableFactory *this,
		    void *data)
{
  BonoboEmbeddable *embeddable;
  EmbeddedDia *embedded_dia;

  embedded_dia = g_new0 (EmbeddedDia, 1);
  if (embedded_dia == NULL)
	  return NULL;

  embedded_dia->diagram = new_diagram("foobar_embedd");
  
  embeddable = bonobo_embeddable_new (BONOBO_VIEW_FACTORY (view_factory),
				      embedded_dia);

  if (embeddable == NULL) {
    diagram_destroy (embedded_dia->diagram);
    return NULL;
  }
	
  running_objects++;
  embedded_dia->embeddable = embeddable;

  gtk_signal_connect(GTK_OBJECT(embeddable), "system_exception",
		     GTK_SIGNAL_FUNC (dia_embeddable_system_exception),
		     embedded_dia);
  
  gtk_signal_connect(GTK_OBJECT(embeddable), "destroy",
		     GTK_SIGNAL_FUNC (dia_embeddable_destroy),
		     embedded_dia);

  return BONOBO_OBJECT (embeddable);
}

static BonoboEmbeddableFactory *
init_dia_factory (void)
{
  return bonobo_embeddable_factory_new ("embeddable-factory:dia-diagram",
					embeddable_factory, NULL);
}

static void
init_server_factory (int argc, char **argv)
{
  CORBA_Environment ev;
  CORBA_ORB orb;
  
  CORBA_exception_init (&ev);
  
  gnome_CORBA_init_with_popt_table ("bonobo-dia-diagram", VERSION,
				    &argc, argv, NULL, 0, NULL, GNORBA_INIT_SERVER_FUNC, &ev);
  
  CORBA_exception_free (&ev);
  
  orb = gnome_CORBA_ORB ();
  if (bonobo_init (orb, NULL, NULL) == FALSE)
    g_error (_("Could not initialize Bonobo!"));
}

int
app_is_embedded(void)
{
  return 1;
}

int
main (int argc, char **argv)
{
  GtkWidget *menuitem;
  
  app_init(argc, argv);

  init_server_factory (argc, argv);
  factory = init_dia_factory ();
  
#ifdef GNOME
  menuitem = menus_get_item_from_path("<Toolbox>/File/New diagram");
  gtk_widget_hide(menuitem);
  menuitem = menus_get_item_from_path("<Toolbox>/File/Open...");
  gtk_widget_hide(menuitem);
  menuitem = menus_get_item_from_path("<Toolbox>/File/Exit");
  gtk_widget_hide(menuitem);
#else
  menuitem = menus_get_item_from_path("<Toolbox>/File/New");
  gtk_widget_hide(menuitem);
  menuitem = menus_get_item_from_path("<Toolbox>/File/Open");
  gtk_widget_hide(menuitem);
  menuitem = menus_get_item_from_path("<Toolbox>/File/sep1");
  gtk_widget_hide(menuitem);
  menuitem = menus_get_item_from_path("<Toolbox>/File/Quit");
  gtk_widget_hide(menuitem);
#endif
  
  /*
   * Start processing.
   */
  bonobo_main ();
  
  return 0;
}

