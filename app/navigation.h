/* Dia -- a diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *  
 * $Id$
 *
 * navigation.h : a navigation popup window to browse large diagrams.
 * Copyright (C) 2003 Luc Pionchon
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
 *  
 */

#ifndef NAVIGATION_H
#define NAVIGATION_H

#include <gtk/gtk.h>
#include "display.h"

/**
 * Returns a button which triggers a popup navigation window.
 *
 * The popup window is created when the button is "pressed",
 * and destroyed when the button is "released". In the meantime, the
 * popup window grabs the pointer/focus. Moving the mouse adjust the
 * scrollbars of the given #DDisplay accordingly.
 *
 * @ddisp: the #DDisplay to navigate through.
 *
 * Returns: a new #GtkButton.
 **/
GtkWidget * navigation_popup_new (DDisplay *ddisp);

#endif
