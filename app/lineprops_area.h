/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * lineprops_area.h -- Copyright (C) 1999 James Henstridge.
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
#ifndef LINEPROPS_AREA_H
#define LINEPROPS_AREA_H

#include "arrows.h"

#include <gtk/gtk.h>

typedef void (*DiaChangeLineCallback) (LineStyle lstyle, real dash_length,
                                       gpointer user_data);

GtkWidget *dia_line_chooser_new  (DiaChangeLineCallback callback,
				  gpointer user_data);

#endif
