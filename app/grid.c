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
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <glib.h>

#include "config.h"
#include "intl.h"
#include "grid.h"
#include "preferences.h"

void
grid_draw(DDisplay *ddisp, Rectangle *update)
{
  Grid *grid = &ddisp->grid;
  Renderer *renderer = ddisp->renderer;
  int width = ddisp->renderer->pixel_width;
  int height = ddisp->renderer->pixel_height;
  /* distance between visible grid lines */
  real width_x = ddisp->diagram->data->grid.width_x *
    ddisp->diagram->data->grid.visible_x;
  real width_y = ddisp->diagram->data->grid.width_y *
    ddisp->diagram->data->grid.visible_y;

  if ( (ddisplay_transform_length(ddisp, width_x) <= 1.0) ||
       (ddisplay_transform_length(ddisp, width_y) <= 1.0) ) {
    return;
  }
  
  if (grid->visible) {
    real pos;
    int x,y;

    (renderer->ops->set_linewidth)(renderer, 0.0);
    if (prefs.grid.solid) {
      (renderer->ops->set_linestyle)(renderer, LINESTYLE_SOLID);
    } else {
      (renderer->ops->set_dashlength)(renderer,
      		    ddisplay_untransform_length(ddisp, 31));
      (renderer->ops->set_linestyle)(renderer, LINESTYLE_DOTTED);
    }
    
    /* Vertical lines: */
    pos = ceil( update->left / width_x ) * width_x;
    while (pos <= update->right) {
      ddisplay_transform_coords(ddisp, pos,0,&x,&y);
      (renderer->interactive_ops->draw_pixel_line)(renderer,
						   x, 0,
						   x, height,
						   &prefs.grid.colour);
      pos += width_x;
    }

    /* Horizontal lines: */
    pos = ceil( update->top / width_y ) * width_y;
    while (pos <= update->bottom) {
      ddisplay_transform_coords(ddisp, 0,pos,&x,&y);
      (renderer->interactive_ops->draw_pixel_line)(renderer,
						   0, y,
						   width, y,
						   &prefs.grid.colour);
      pos += width_y;
    }
  }

  if (prefs.pagebreak.visible) {
    Diagram *dia = ddisp->diagram;
    real origx = 0, origy = 0, pos;
    real pwidth = dia->data->paper.width;
    real pheight = dia->data->paper.height;
    int x,y;

    (renderer->ops->set_linewidth)(renderer, 0.0);
    (renderer->ops->set_dashlength)(renderer,
				    ddisplay_untransform_length(ddisp, 31));
    if (prefs.pagebreak.solid)
      (renderer->ops->set_linestyle)(renderer, LINESTYLE_SOLID);
    else
      (renderer->ops->set_linestyle)(renderer, LINESTYLE_DOTTED);

    if (dia->data->paper.fitto) {
      origx = dia->data->extents.left;
      origy = dia->data->extents.top;
    }

    /* vertical lines ... */
    pos = origx + ceil((update->left - origx) / pwidth) * pwidth;
    while (pos <= update->right) {
      ddisplay_transform_coords(ddisp, pos,0,&x,&y);
      (renderer->interactive_ops->draw_pixel_line)(renderer,
						   x, 0,
						   x, height,
						   &prefs.pagebreak.colour);
      pos += pwidth;
    }
    /* Horizontal lines: */
    pos = origy + ceil((update->top - origy) / pheight) * pheight;
    while (pos <= update->bottom) {
      ddisplay_transform_coords(ddisp, 0,pos,&x,&y);
      (renderer->interactive_ops->draw_pixel_line)(renderer,
						   0, y,
						   width, y,
						   &prefs.pagebreak.colour);
      pos += pheight;
    }
  }
}

void
snap_to_grid(DDisplay *ddisp, coord *x, coord *y)
{
  if (ddisp->grid.snap) {
    real width_x = ddisp->diagram->data->grid.width_x;
    real width_y = ddisp->diagram->data->grid.width_y;

    *x = ROUND((*x) / width_x) * width_x;
    *y = ROUND((*y) / width_y) * width_y;
  }
}

static void
grid_x_update(GtkWidget *entry, DDisplay *ddisp)
{
  real size;
  char buffer[32];

  size = atof(gtk_entry_get_text(GTK_ENTRY(entry)));

  if (size > 0.01) {
    ddisp->diagram->data->grid.width_x = size;
    
    g_snprintf(buffer, 32, "%.4f", (double)ddisp->diagram->data->grid.width_x);
    gtk_entry_set_text(GTK_ENTRY(entry), buffer);
    diagram_add_update_all(ddisp->diagram);
    diagram_flush(ddisp->diagram);
  }
}

static gint
grid_x_update_event(GtkWidget *entry, GdkEventFocus *ev, DDisplay *ddisp)
{
  grid_x_update(entry, ddisp);
  return FALSE;
}

static void
grid_y_update(GtkWidget *entry, DDisplay *ddisp)
{
  real size;
  char buffer[32];

  size = atof(gtk_entry_get_text(GTK_ENTRY(entry)));

  if (size > 0.01) {
    ddisp->diagram->data->grid.width_y = size;
    
    g_snprintf(buffer, 32, "%.4f", (double)ddisp->diagram->data->grid.width_y);
    gtk_entry_set_text(GTK_ENTRY(entry), buffer);
    diagram_add_update_all(ddisp->diagram);
    diagram_flush(ddisp->diagram);
  }
}

static gint
grid_y_update_event(GtkWidget *entry, GdkEventFocus *ev, DDisplay *ddisp)
{
  grid_y_update(entry, ddisp);
  return FALSE;
}

void grid_show_dialog(Grid *grid, DDisplay *ddisp)
{
  char buffer[32];
  
  if (grid->dialog == NULL) {
    GtkWidget *dialog;
    GtkWidget *vbox;
    GtkWidget *hbox;
    GtkWidget *label, *entry;
    
    grid->dialog = dialog = gtk_window_new(GTK_WINDOW_DIALOG);
    gtk_window_set_title (GTK_WINDOW (dialog), _("Grid options"));
    gtk_container_set_border_width (GTK_CONTAINER (dialog), 5);
    gtk_signal_connect (GTK_OBJECT (dialog), "delete_event",
			GTK_SIGNAL_FUNC(gtk_widget_hide), NULL);

    vbox = gtk_vbox_new(FALSE, 5);
    gtk_container_add(GTK_CONTAINER(dialog), vbox);
    
    hbox = gtk_hbox_new(FALSE, 5);
    label = gtk_label_new(_("Grid x size:"));
    grid->entry_x = entry = gtk_entry_new();
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);
    gtk_widget_show(label);
    gtk_widget_show(entry);
    gtk_widget_show(hbox);

    grid->handler_x =
      gtk_signal_connect (GTK_OBJECT (entry), "focus_out_event",
			  GTK_SIGNAL_FUNC (grid_x_update_event), ddisp);
    gtk_signal_connect (GTK_OBJECT (entry), "activate",
			GTK_SIGNAL_FUNC (grid_x_update), ddisp);

    hbox = gtk_hbox_new(FALSE, 5);
    label = gtk_label_new(_("Grid y size:"));
    grid->entry_y = entry = gtk_entry_new();
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);
    gtk_widget_show(label);
    gtk_widget_show(entry);
    gtk_widget_show(hbox);

    grid->handler_y =
      gtk_signal_connect (GTK_OBJECT (entry), "focus_out_event",
			  GTK_SIGNAL_FUNC (grid_y_update_event), ddisp);
    gtk_signal_connect (GTK_OBJECT (entry), "activate",
			GTK_SIGNAL_FUNC (grid_y_update), ddisp);

    gtk_widget_show(vbox);
  }

  g_snprintf(buffer, 32, "%.4f", (double)ddisp->diagram->data->grid.width_x);
  gtk_entry_set_text(GTK_ENTRY(grid->entry_x), buffer);
  g_snprintf(buffer, 32, "%.4f", (double)ddisp->diagram->data->grid.width_y);
  gtk_entry_set_text(GTK_ENTRY(grid->entry_y), buffer);
  
  gtk_widget_show(grid->dialog);
}

void grid_destroy_dialog(Grid *grid)
{
  if (grid->dialog) {
    /* Have to remove these, or will get segfault if they are active: */
    gtk_signal_disconnect(GTK_OBJECT(grid->entry_x), grid->handler_x);
    gtk_signal_disconnect(GTK_OBJECT(grid->entry_y), grid->handler_y);
    
    gtk_widget_destroy(grid->dialog);
  }
}
