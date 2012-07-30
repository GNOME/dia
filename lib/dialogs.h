/* Dia -- a diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * dialogs.c: helper routines for creating simple dialogs
 * Copyright (C) 2002 Lars Clausen
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
#include <gtk/gtk.h>

GtkWidget *
dialog_make(char *title, char *okay_text, char *cancel_text,
	    GtkWidget **okay_button, GtkWidget **cancel_button);
GtkSpinButton *
dialog_add_spinbutton(GtkWidget *dialog, char *title,
		      real min, real max, real decimals);
