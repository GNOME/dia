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

gint dia_display_focus_in_event (GtkWidget *widget, GdkEventFocus *event,
			      gpointer data);
gint dia_display_focus_out_event (GtkWidget *widget, GdkEventFocus *event,
			       gpointer data);
void dia_display_realize (GtkWidget *widget, gpointer data);
void dia_display_unrealize (GtkWidget *widget, gpointer data);

gint dia_display_canvas_events (GtkWidget *, GdkEvent *, gpointer data);
void dia_display_popup_menu(DiaDisplay *ddisp, GdkEventButton *event);
gint dia_display_hsb_update (GtkAdjustment *adjustment, DiaDisplay *ddisp);
gint dia_display_vsb_update (GtkAdjustment *adjustment, DiaDisplay *ddisp);
gint dia_display_delete (GtkWidget *widget, GdkEvent  *event, gpointer data);

DiaObject *dia_display_drop_object(DiaDisplay *ddisp, gint x, gint y, DiaObjectType *otype,
			  gpointer user_data);
void dia_display_im_context_commit(GtkIMContext *context, const gchar  *str,
                                DiaDisplay     *ddisp);
void dia_display_im_context_preedit_changed(GtkIMContext *context,
                                         DiaDisplay *ddisp);

#endif /* DISP_CALLBACKS_H */
