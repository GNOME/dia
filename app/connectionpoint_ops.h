/* xxxxxx -- an diagram creation/manipulation program
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
#ifndef CONNECTIONPOINT_OPS_H
#define CONNECTIONPOINT_OPS_H

#include "connectionpoint.h"
#include "diagram.h"
#include "display.h"

extern void connectionpoint_draw(ConnectionPoint *conpoint,
				 DDisplay *ddisp);
extern void connectionpoint_add_update(ConnectionPoint *conpoint,
				       Diagram *dia);
extern void diagram_update_connections_selection(Diagram *dia);
extern void diagram_update_connections_object(Diagram *dia, Object *obj);
extern void ddisplay_connect_selected(DDisplay *ddisp);
extern void diagram_unconnect_selected(Diagram *dia);

#endif /* CONNECTIONPOINT_OPS_H */
