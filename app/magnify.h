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
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */
#ifndef MAGNIFY_H
#define MAGNIFY_H

#include "tool.h"

typedef struct _MagnifyTool MagnifyTool;

Tool *create_magnify_tool(void);
void free_magnify_tool(Tool *tool);
void set_zoom_out(Tool *tool);
void set_zoom_in(Tool *tool);

#endif /* MAGNIFY_H */
