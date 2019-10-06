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

#pragma once

#include "dia-guide.h"
#include "tool.h"

typedef struct _GuideTool GuideTool;

Tool *create_guide_tool(void);
void free_guide_tool(Tool *tool);

/** Start dragging a new guide. */
void _guide_tool_start_new (DDisplay *display,
                            GtkOrientation orientation);

/** Start editing (i.e. moving) an existing guide. */
void guide_tool_start_edit (DDisplay *display,
                            DiaGuide *guide);

/** Start using the guide tool.
 *  If guide is not NULL, then start editing that guide.
 *  If guide is NULL, then start adding a new guide. */
void _guide_tool_start (DDisplay *display,
                        GtkOrientation orientation,
                        DiaGuide *guide);

/** Inform the tool of the ruler height. Required to calculate
 *  the position of the guide on the page. */
void guide_tool_set_ruler_height(Tool *tool, int height);

/** Set the guide to edit. */
void guide_tool_set_guide(Tool *tool, DiaGuide *guide);

/** Set the orientation of the tool. */
void guide_tool_set_orientation(Tool *tool, GtkOrientation orientation);
