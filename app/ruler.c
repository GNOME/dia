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

#include <config.h>

#undef GTK_DISABLE_DEPRECATED /* GtkRuler */
/* GtkRuler is deprecated by gtk-2-24, gone with gtk-3-0 because it is 
 * deemed to be too specialized for maintenance in Gtk. Maybe Dia is 
 * too specialized to be ported?
 */
#include <gtk/gtk.h>

#include "ruler.h"

GtkWidget *
dia_ruler_new (GtkOrientation orientation, GtkWidget *shell)
{
  GtkWidget *rule;

  if (orientation == GTK_ORIENTATION_HORIZONTAL)
    rule = gtk_hruler_new ();
  else if (orientation == GTK_ORIENTATION_VERTICAL)
    rule = gtk_vruler_new ();

  g_signal_connect_swapped (G_OBJECT (shell), "motion_notify_event",
                            G_CALLBACK(GTK_WIDGET_GET_CLASS (rule)->motion_notify_event),
                            G_OBJECT (rule));
  return rule;
}

void 
dia_ruler_set_range (GtkWidget *ruler,
                     gdouble    lower,
                     gdouble    upper,
                     gdouble    position,
                     gdouble    max_size)
{
  gtk_ruler_set_range (GTK_RULER (ruler), lower, upper, position, max_size);
}
