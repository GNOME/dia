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
#ifndef GRID_H
#define GRID_H

#include <gtk/gtk.h>

typedef struct _Grid Grid;

#include "geometry.h"
struct _Grid {
  guint visible;
  guint snap;
  real width_x;
  real width_y;
  
  GdkGC *gc;

  GtkWidget *dialog;         /* The dialog for changing the grid  */
  GtkWidget *entry_x;
  gint handler_x;
  GtkWidget *entry_y;
  gint handler_y;
};

#include "display.h"

extern void grid_draw(DDisplay *ddisp, Rectangle *update);
extern void snap_to_grid(Grid *grid, coord *x, coord *y);
extern void grid_show_dialog(Grid *grid, DDisplay *ddisp);
extern void grid_destroy_dialog(Grid *grid);

#endif GRID_H
