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

G_BEGIN_DECLS

typedef struct _ToolState ToolState;

#include "display.h"

#define DIA_TYPE_TOOL (dia_tool_get_type ())
G_DECLARE_DERIVABLE_TYPE (DiaTool, dia_tool, DIA, TOOL, GObject)

typedef enum _ToolType ToolType;

enum _ToolType {
  CREATE_OBJECT_TOOL,
  MAGNIFY_TOOL,
  MODIFY_TOOL,
  SCROLL_TOOL,
  TEXTEDIT_TOOL
};

struct _DiaToolClass {
  GObjectClass parent_class;

  void (* activate)       (DiaTool *);
  void (* deactivate)     (DiaTool *);

  void (* button_press)   (DiaTool *, GdkEventButton *, DDisplayBox *ddisp);
  void (* button_hold)    (DiaTool *, GdkEventButton *, DDisplayBox *ddisp);
  void (* double_click)   (DiaTool *, GdkEventButton *, DDisplayBox *ddisp);
  void (* button_release) (DiaTool *, GdkEventButton *, DDisplayBox *ddisp);
  void (* motion)         (DiaTool *, GdkEventMotion *, DDisplayBox *ddisp);
};

struct _ToolState {
  GType type;
  gpointer extra_data;
  gpointer user_data;
  GtkWidget *button;
  int invert_persistence;
};

extern DiaTool *active_tool, *transient_tool;

void tool_get(ToolState *state);
void tool_restore(const ToolState *state);
void tool_select              (ToolType   type,
                               gpointer   extra_data,
                               gpointer   user_date,
                               GtkWidget *button,
                               int        invert_persistence);
void tool_select_former(void);
void tool_reset(void);
void tool_options_dialog_show (GType      type,
                               gpointer   extra_data, 
                               gpointer   user_data,
                               GtkWidget *button,
                               gboolean   invert_persistence);

void dia_tool_activate       (DiaTool *self);
void dia_tool_deactivate     (DiaTool *self);
void dia_tool_button_press   (DiaTool *self, GdkEventButton *, DDisplay *ddisp);
void dia_tool_button_hold    (DiaTool *self, GdkEventButton *, DDisplay *ddisp);
void dia_tool_double_click   (DiaTool *self, GdkEventButton *, DDisplay *ddisp);
void dia_tool_button_release (DiaTool *self, GdkEventButton *, DDisplay *ddisp);
void dia_tool_motion         (DiaTool *self, GdkEventMotion *, DDisplay *ddisp);

G_END_DECLS

#endif /* TOOL_H */
