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
#ifndef DISP_CALLBACKS_H
#define DISP_CALLBACKS_H

#include "display.h"

#define CANVAS_EVENT_MASK   \
         GDK_EXPOSURE_MASK | GDK_POINTER_MOTION_MASK | \
	 GDK_POINTER_MOTION_HINT_MASK | GDK_BUTTON_PRESS_MASK | \
	 GDK_BUTTON_RELEASE_MASK | GDK_STRUCTURE_MASK | \
	 GDK_ENTER_NOTIFY_MASK | GDK_KEY_PRESS_MASK |  \
	 GDK_KEY_RELEASE_MASK

extern gint ddisplay_canvas_events (GtkWidget *, GdkEvent *);
extern gint ddisplay_hsb_update (GtkAdjustment *adjustment,
				 DDisplay *ddisp);
extern gint ddisplay_vsb_update (GtkAdjustment *adjustment,
				 DDisplay *ddisp);
extern gint ddisplay_delete (GtkWidget *widget, GdkEvent  *event, gpointer data);
extern void ddisplay_destroy (GtkWidget *widget, gpointer data);

#endif /* DISP_CALLBACKS_H */
