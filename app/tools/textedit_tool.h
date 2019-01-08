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
#ifndef TEXTEDIT_TOOL_H
#define TEXTEDIT_TOOL_H

#include "tool.h"
#include "diatypes.h"

enum TexteditToolState {
  STATE_TEXT_SELECT,
  STATE_TEXT_EDIT
};

#define DIA_TYPE_TEXT_EDIT_TOOL (dia_text_edit_tool_get_type ())
G_DECLARE_FINAL_TYPE (DiaTextEditTool, dia_text_edit_tool, DIA, TEXT_EDIT_TOOL, DiaTool)

struct _DiaTextEditTool {
  DiaTool tool;
  
  enum TexteditToolState state;
  DiaObject *object;
  Point start_at;
};

#endif /* TEXTEDIT_TOOL_H */
