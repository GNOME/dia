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
#ifndef INTERFACE_H
#define INTERFACE_H

#include "display.h"

#ifdef GNOME
#include <gnome.h>
#endif
#include "gtkhwrapbox.h"
#include "intl.h"
#include "interface.h"
#include "menus.h"
#include "disp_callbacks.h"
#include "tool.h"
#include "sheet.h"
#include "app_procs.h"
#include "arrows.h"
#include "color_area.h"
#include "linewidth_area.h"
#include "lineprops_area.h"
#include "attributes.h"

void create_display_shell(DDisplay *ddisp,
			  int width, int height,
			  char *title, int use_mbar, int top_level_window);

void create_toolbox (void);
void toolbox_show(void);
void toolbox_hide(void);

GtkWidget *interface_get_toolbox_shell(void);

void tool_select_callback(GtkWidget *widget, gpointer data);

void create_tree_window(void);

GtkWidget *popup_shell;
void create_sheets(GtkWidget *parent);
GtkWidget *modify_tool_button;

typedef struct _ToolButton ToolButton;

typedef struct _ToolButtonData ToolButtonData;

struct _ToolButtonData
{
  ToolType type;
  gpointer extra_data;
  gpointer user_data; /* Used by create_object_tool */
  GtkWidget *widget;
};

struct _ToolButton
{
  gchar **icon_data;
  char  *tool_desc;
  char  *menuitem_name;
  ToolButtonData callback_data;
};

extern const int num_tools;
extern ToolButton tool_data[];
extern gchar *interface_current_sheet_name;

void tool_select_update (GtkWidget *w, gpointer   data);

void fill_sheet_menu(void);

#endif /* INTERFACE_H */
