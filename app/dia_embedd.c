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
#include <liboaf/liboaf.h>

#include "display.h"
#include "menus.h"
#include "disp_callbacks.h"
#include "app_procs.h"
#include "interface.h"

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


#if 0
  menuitem = menus_get_item_from_path("<Display>/File/New diagram");
  gtk_widget_hide(menuitem);
  menuitem = menus_get_item_from_path("<Display>/File/Open...");
  gtk_widget_hide(menuitem);
  menuitem = menus_get_item_from_path("<Display>/File/Save As...");
  gtk_widget_hide(menuitem);
  menuitem = menus_get_item_from_path("<Display>/File/Close");
  gtk_widget_hide(menuitem);
  menuitem = menus_get_item_from_path("<Display>/File/Exit");
  gtk_widget_hide(menuitem);
  menuitem = menus_get_item_from_path("<Display>/View/New View");
  gtk_widget_hide(menuitem);
#endif
  
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
  return bonobo_generic_factory_new ("OAFIID:GNOME_Dia_DiagramFactory",
				     embeddable_factory, NULL);
}

static void
init_server_factory (int argc, char **argv)
{
  CORBA_ORB orb;
  
  gnome_init_with_popt_table ("dia-embedd", VERSION,
			      argc, argv, oaf_popt_options, 0, NULL);

  orb = oaf_init (argc, argv);

  if (bonobo_init (orb, NULL, NULL) == FALSE)
    g_error (_("Could not initialize Bonobo!"));
}

int
app_is_embedded(void)
{
  return 1;
}

static void
last_unref_cb (BonoboObject *bonobo_object,
	       gpointer      dummy)
{
  bonobo_object_unref (BONOBO_OBJECT (factory));
  gtk_main_quit ();
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
  

  gtk_signal_connect (GTK_OBJECT (bonobo_context_running_get ()),
		      "last_unref",
		      GTK_SIGNAL_FUNC (last_unref_cb),
		      NULL);
  /*
   * Start processing.
   */
  bonobo_main ();
  
  return 0;
}

