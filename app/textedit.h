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

#ifndef TEXTEDIT_H
#define TEXTEDIT_H
#include <sys/types.h>
#include <glib.h>

#include "diatypes.h"
#include "display.h"

Focus *textedit_move_focus(DDisplay *ddisp, Focus *focus, gboolean forwards);

gboolean textedit_mode(DDisplay *ddisp);
void textedit_activate_focus(DDisplay *ddisp, Focus *focus, Point *clicked);
gboolean textedit_activate_object(DDisplay *ddisp, DiaObject *obj, Point *clicked);
gboolean textedit_activate_first(DDisplay *ddisp);
void textedit_deactivate_focus(void);
void textedit_remove_focus(DiaObject *obj, Diagram *diagram);
void textedit_remove_focus_all(Diagram *diagram);
#endif
