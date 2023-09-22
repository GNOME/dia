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

#include "config.h"

#include <glib/gi18n-lib.h>

#include <string.h>

#include "widgets.h"
#include "units.h"
#include "message.h"
#include "dia_dirs.h"
#include "diaoptionmenu.h"

#include <stdlib.h>
#include <glib.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <pango/pango.h>
#include <stdio.h>
#include <time.h>
#include <gdk/gdkkeysyms.h>


/* ************************ Misc. util functions ************************ */
struct image_pair { GtkWidget *on; GtkWidget *off; };

static void
dia_toggle_button_swap_images(GtkToggleButton *widget,
			      gpointer data)
{
  struct image_pair *images = (struct image_pair *)data;
  if (gtk_toggle_button_get_active(widget)) {
    gtk_container_remove(GTK_CONTAINER(widget),
			 gtk_bin_get_child(GTK_BIN(widget)));
    gtk_container_add(GTK_CONTAINER(widget),
		      images->on);

  } else {
    gtk_container_remove(GTK_CONTAINER(widget),
			 gtk_bin_get_child(GTK_BIN(widget)));
    gtk_container_add(GTK_CONTAINER(widget),
		      images->off);
  }
}


static void
dia_toggle_button_destroy (GtkWidget *widget, gpointer data)
{
  struct image_pair *images = (struct image_pair *) data;

  g_clear_object (&images->on);
  g_clear_object (&images->off);
  g_clear_pointer (&images, g_free);

}


/** Create a toggle button given two image widgets for on and off */
static GtkWidget *
dia_toggle_button_new(GtkWidget *on_widget, GtkWidget *off_widget)
{
  GtkWidget *button = gtk_toggle_button_new();
//  GtkRcStyle *rcstyle;
  struct image_pair *images;

  images = g_new(struct image_pair, 1);
  /* Since these may not be added at any point, make sure to
   * sink them. */
  images->on = on_widget;
  g_object_ref_sink(images->on);
  gtk_widget_show(images->on);

  images->off = off_widget;
  g_object_ref_sink(images->off);
  gtk_widget_show(images->off);

  /* Make border as small as possible */
  gtk_misc_set_padding (GTK_MISC (images->on), 0, 0);
  gtk_misc_set_padding (GTK_MISC (images->off), 0, 0);
  gtk_widget_set_can_focus (GTK_WIDGET (button), FALSE);
  gtk_widget_set_can_default (GTK_WIDGET (button), FALSE);

/* disabled Gtk3 -- Hub
  rcstyle = gtk_rc_style_new ();
  rcstyle->xthickness = rcstyle->ythickness = 0;
  gtk_widget_modify_style (button, rcstyle);
  g_clear_object (&rcstyle);
*/

  gtk_button_set_relief(GTK_BUTTON(button), GTK_RELIEF_NONE);
  /*  gtk_button_set_focus_on_click(GTK_BUTTON(button), FALSE);*/
  gtk_container_set_border_width(GTK_CONTAINER(button), 0);

  gtk_container_add(GTK_CONTAINER(button), images->off);

  g_signal_connect(G_OBJECT(button), "toggled",
		   G_CALLBACK(dia_toggle_button_swap_images), images);
  g_signal_connect(G_OBJECT(button), "destroy",
		   G_CALLBACK(dia_toggle_button_destroy), images);

  return button;
}


/*
 * Create a toggle button with two icons (created with gdk-pixbuf-csource,
 * for instance).  The icons represent on and off.
 */


/* GTK3: This is built-in (new_from_resource, add_resource_path....) */
/* Adapted from Gtk */
GdkPixbuf *
pixbuf_from_resource (const char *path)
{
  GdkPixbufLoader *loader = NULL;
  GdkPixbuf *pixbuf = NULL;
  GBytes *bytes;

  g_return_val_if_fail (path != NULL, NULL);

  bytes = g_resources_lookup_data (path, G_RESOURCE_LOOKUP_FLAGS_NONE, NULL);

  if (!bytes) {
    g_critical ("Missing resource %s", path);
    goto out;
  }

  loader = gdk_pixbuf_loader_new ();

  if (!gdk_pixbuf_loader_write_bytes (loader, bytes, NULL))
    goto out;

  if (!gdk_pixbuf_loader_close (loader, NULL))
    goto out;

  pixbuf = g_object_ref (gdk_pixbuf_loader_get_pixbuf (loader));

 out:
  gdk_pixbuf_loader_close (loader, NULL);
  g_clear_object (&loader);
  g_bytes_unref (bytes);

  return pixbuf;
}


GtkWidget *
dia_toggle_button_new_with_icon_names (const char *on,
                                       const char *off)
{
  GtkWidget *on_img, *off_img;

  on_img = gtk_image_new_from_icon_name (on, GTK_ICON_SIZE_BUTTON);
  off_img = gtk_image_new_from_icon_name (off, GTK_ICON_SIZE_BUTTON);

  return dia_toggle_button_new (on_img, off_img);
}
