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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <gnome.h>
#include <bonobo.h>
#ifdef USE_OAF
#  include <liboaf/liboaf.h>
#else
#  include <libgnorba/gnorba.h>
#endif


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

static BonoboGenericFactory *factory = NULL;
static int running_objects = 0;


static void
dia_view_activate(BonoboView *view, gboolean activate,
		  EmbeddedView *view_data)
{
  DDisplay *ddisp = view_data->display;

  bonobo_view_activate_notify(view, activate);
  if (activate) {
    toolbox_show();

    gtk_widget_show(ddisp->origin);
    gtk_widget_show(ddisp->hrule);
    gtk_widget_show(ddisp->vrule);
    gtk_widget_show(ddisp->hsb);
    gtk_widget_show(ddisp->vsb);
    gtk_widget_show(ddisp->zoom_status->parent);
  } else {
    toolbox_hide();

    gtk_widget_hide(ddisp->origin);
    gtk_widget_hide(ddisp->hrule);
    gtk_widget_hide(ddisp->vrule);
    gtk_widget_hide(ddisp->hsb);
    gtk_widget_hide(ddisp->vsb);
    gtk_widget_hide(ddisp->zoom_status->parent);
  }
}

static void
dia_view_display_destroy (GtkWidget *ddisp_shell, EmbeddedView *view_data)
{
  view_data->display = NULL;
  bonobo_object_unref(BONOBO_OBJECT(view_data->view));
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
  bonobo_object_unref(BONOBO_OBJECT(view));
}

static void
dia_embeddable_system_exception(BonoboEmbeddable *embeddable, CORBA_Object corba_object,
				CORBA_Environment *ev, gpointer data)
{
  bonobo_object_unref(BONOBO_OBJECT(embeddable));
}

static BonoboView *
view_factory (BonoboEmbeddable *embeddable,
	      const Bonobo_ViewFrame view_frame,
	      EmbeddedDia *embedded_dia)
{
  EmbeddedView *view_data;
  BonoboView  *view;
  GtkWidget *menuitem;
  GString *path;

  
  /*
   * Create the private view data.
   */
  view_data = g_new0 (EmbeddedView, 1);
  view_data->embedded_dia = embedded_dia;
  
  view_data->display = new_display(embedded_dia->diagram);

  gtk_signal_connect (GTK_OBJECT (view_data->display->shell), "destroy",
                      GTK_SIGNAL_FUNC (dia_view_display_destroy),
                      view_data);
  
  gtk_widget_hide(view_data->display->origin);
  gtk_widget_hide(view_data->display->hrule);
  gtk_widget_hide(view_data->display->vrule);
  gtk_widget_hide(view_data->display->hsb);
  gtk_widget_hide(view_data->display->vsb);
  gtk_widget_hide(view_data->display->zoom_status->parent);

  view = bonobo_view_new (view_data->display->shell);
  view_data->view = view;
  gtk_object_set_data (GTK_OBJECT (view), "view_data", view_data);
  
  
  gtk_signal_connect(GTK_OBJECT (view), "activate",
		     GTK_SIGNAL_FUNC (dia_view_activate), view_data);
  
  gtk_signal_connect(GTK_OBJECT (view), "system_exception",
		     GTK_SIGNAL_FUNC (dia_view_system_exception), view_data);

  gtk_signal_connect (GTK_OBJECT (view), "destroy",
		      GTK_SIGNAL_FUNC (dia_view_destroy), view_data);


  /* Gross stuff: */
  path = g_string_new("<Display>/");
  g_string_append(path, _("File/New diagram"));
  menuitem = menus_get_item_from_path(path->str);
  gtk_widget_hide(menuitem);
  g_string_truncate(path, 10);
  g_string_append(path, _("File/Open..."));
  menuitem = menus_get_item_from_path(path->str);
  gtk_widget_hide(menuitem);
  g_string_truncate(path, 10);
  g_string_append(path, _("File/Save As..."));
  menuitem = menus_get_item_from_path(path->str);
  gtk_widget_hide(menuitem);
  g_string_truncate(path, 10);
  g_string_append(path, _("File/Close"));
  menuitem = menus_get_item_from_path(path->str);
  gtk_widget_hide(menuitem);
  g_string_truncate(path, 10);
  g_string_append(path, _("File/Exit"));
  menuitem = menus_get_item_from_path(path->str);
  gtk_widget_hide(menuitem);
  g_string_truncate(path, 10);
  g_string_append(path, _("View/New View"));
  menuitem = menus_get_item_from_path(path->str);
  gtk_widget_hide(menuitem);
  g_string_free(path, TRUE);
  
  return view;
}

static BonoboObject *
embeddable_factory (BonoboGenericFactory *this,
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

static BonoboGenericFactory *
init_dia_factory (void)
{
#ifdef USE_OAF
  return bonobo_generic_factory_new ("OAFIID:dia-diagram-factory:31cb6c0f-e1f5-4c54-8824-8f49fd7e50e7",
				     embeddable_factory, NULL);
#else
  return bonobo_generic_factory_new ("embeddable-factory:dia-diagram",
				     embeddable_factory, NULL);
#endif
}

static void
init_server_factory (int argc, char **argv)
{
  CORBA_Environment ev;
  CORBA_ORB orb;
  
#ifdef USE_OAF  
  gnome_init_with_popt_table("bonobo-dia-diagram", VERSION,
			     argc, argv, oaf_popt_options, 0, NULL);
  orb = oaf_init(argc, argv);
#else
  CORBA_exception_init (&ev);

  gnome_CORBA_init_with_popt_table ("bonobo-dia-diagram", VERSION,
				    &argc, argv, NULL, 0, NULL, GNORBA_INIT_SERVER_FUNC, &ev);
  
  CORBA_exception_free (&ev);
  
  orb = gnome_CORBA_ORB ();
#endif

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
  
  init_server_factory (argc, argv);
  factory = init_dia_factory ();

  app_init(0, NULL);
  
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

