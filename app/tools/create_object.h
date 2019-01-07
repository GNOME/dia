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
#ifndef CREATE_OBJECT_H
#define CREATE_OBJECT_H

#include "geometry.h"
#include "tool.h"

typedef struct _CreateObjectTool CreateObjectTool;

struct _CreateObjectTool {
  Tool tool;

  DiaObjectType *objtype;
  void *user_data;
  
  int moving;
  Handle *handle;
  DiaObject *obj;
  Point last_to;
  int invert_persistence;
};


Tool *create_create_object_tool(DiaObjectType *objtype, void *user_date, int invert_persistence);
void free_create_object_tool(Tool *tool);

#endif /* CREATE_OBJECT_H */
