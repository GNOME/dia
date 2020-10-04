/* Dia -- a diagram creation/manipulation program
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

#pragma once

#include "diatypes.h"
#include "dia-object-change.h"

struct _Focus {
  DiaObject *obj;
  Text *text;
  int has_focus;

  /* return TRUE if modified object.
     Set change if object is changed. */
  int (*key_event) (Focus            *focus,
                    guint             keystate,
                    guint             keysym,
                    const char       *str,
                    int               strlen,
                    DiaObjectChange **change);
};

void request_focus(Focus *focus);
Focus *get_active_focus(DiagramData *dia);
void give_focus(Focus *focus);
Focus *focus_get_first_on_object(DiaObject *obj);
Focus *focus_next_on_diagram(DiagramData *dia);
Focus *focus_previous_on_diagram(DiagramData *dia);
void remove_focus_on_diagram(DiagramData *dia);
gboolean remove_focus_object(DiaObject *obj);
void reset_foci_on_diagram(DiagramData *dia);
DiaObject* focus_get_object(Focus *focus);
