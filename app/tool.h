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
#ifndef TOOL_H
#define TOOL_H

#include <gdk/gdk.h>

typedef struct _Tool Tool;
typedef struct _ToolState ToolState;

typedef enum _ToolType ToolType;

#include "display.h"

typedef void (* ButtonPressFunc)   (Tool *, GdkEventButton *, DDisplay *ddisp);
typedef void (* ButtonHoldFunc)    (Tool *, GdkEventButton *, DDisplay *ddisp);
typedef void (* DoubleClickFunc)   (Tool *, GdkEventButton *, DDisplay *ddisp);
typedef void (* ButtonReleaseFunc) (Tool *, GdkEventButton *, DDisplay *ddisp);
typedef void (* MotionFunc)        (Tool *, GdkEventMotion *, DDisplay *ddisp);

enum _ToolType {
  CREATE_OBJECT_TOOL,
  MAGNIFY_TOOL,
  MODIFY_TOOL,
  SCROLL_TOOL,
  TEXTEDIT_TOOL,
  GUIDE_TOOL,
};

struct _Tool {
  ToolType type;

  /*  Action functions  */
  ButtonPressFunc    button_press_func;
  ButtonHoldFunc     button_hold_func;
  ButtonReleaseFunc  button_release_func;
  MotionFunc         motion_func;
  DoubleClickFunc    double_click_func;
};

struct _ToolState {
  ToolType type;
  gpointer extra_data;
  gpointer user_data;
  GtkWidget *button;
  int invert_persistence;
};

extern Tool *active_tool, *transient_tool;

void tool_get(ToolState *state);
void tool_restore(const ToolState *state);
void tool_free(Tool *tool);
void tool_select(ToolType type, gpointer extra_data, gpointer user_date,
                 GtkWidget *button, int invert_persistence);
void tool_select_former(void);
void tool_reset(void);
void tool_options_dialog_show(ToolType type, gpointer extra_data,
			      gpointer user_data,GtkWidget *button,
                              int invert_persistence);

#endif /* TOOL_H */
