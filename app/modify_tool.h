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
#ifndef MODIFY_TOOL_H
#define MODIFY_TOOL_H

#include "geometry.h"
#include "tool.h"
#include "time.h"

typedef struct _ModifyTool ModifyTool;

enum ModifyToolState {
  STATE_NONE,
  STATE_MOVE_OBJECT,
  STATE_MOVE_HANDLE,
  STATE_BOX_SELECT
};

struct _ModifyTool {
  Tool tool;

  enum ModifyToolState state;
  int break_connections;
  Point move_compensate;
  Object *object;
  Handle *handle;
  Point last_to;
  Point start_at;
  time_t start_time;

  GdkGC *gc;

  int x1, y1, x2, y2;
  Point start_box;
  Point end_box;

  gboolean auto_scrolled; /* TRUE if the diagram auto scrolled last time 
                             modify_motion was called */
  /* Undo info: */
  Point *orig_pos;
};

Tool *create_modify_tool(void);
void free_modify_tool(Tool *tool);

#endif /* MODIFY_TOOL_H */
