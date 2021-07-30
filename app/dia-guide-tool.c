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
#include <config.h>

#include <gdk/gdk.h>

#include "cursor.h"
#include "dia-guide-tool.h"
#include "undo.h"
#include "diainteractiverenderer.h"

#define GUIDE_POSITION_UNDEFINED G_MINDOUBLE

/* The original position of the current guide
 * before we started dragging it. Need to keep track of it for
 * undo moving/deleting guides. */
static real guide_original_pos = 0;

struct _GuideTool {
  Tool tool;

  DiaGuide *guide;
  real position;
  GtkOrientation orientation;
  int ruler_height;
};



static void
guide_button_release(GuideTool *tool, GdkEventButton *event, DDisplay *ddisp);

static void
guide_motion(GuideTool *tool, GdkEventMotion *event, DDisplay *ddisp);



void _guide_tool_start_new (DDisplay *display,
                            GtkOrientation orientation)
{
  _guide_tool_start (display, orientation, NULL);
}

void _guide_tool_start (DDisplay *display,
                        GtkOrientation orientation,
                        DiaGuide *guide)
{
  tool_select(GUIDE_TOOL, guide, GINT_TO_POINTER(orientation), NULL, 0);
  display->dragged_new_guideline_orientation = orientation;
}

Tool *create_guide_tool(void)
{
  GuideTool *tool;

  tool = g_new0(GuideTool, 1);
  tool->tool.type = GUIDE_TOOL;

  tool->tool.button_release_func = (ButtonReleaseFunc) &guide_button_release;
  tool->tool.motion_func = (MotionFunc) &guide_motion;

  tool->guide        = NULL;
  tool->position     = GUIDE_POSITION_UNDEFINED;
  tool->orientation  = GTK_ORIENTATION_HORIZONTAL;  /* Default. */

  return (Tool *)tool;
}


void
free_guide_tool (Tool *tool)
{
  GuideTool *gtool = (GuideTool *) tool;

  g_clear_pointer (&gtool, g_free);
}


static void
guide_button_release(GuideTool *tool, GdkEventButton *event,
                     DDisplay *ddisp)
{
  /* Reset. */
  ddisp->is_dragging_new_guideline = FALSE;

  if (tool->position == GUIDE_POSITION_UNDEFINED)
  {
    /* Dragged out of bounds, so remove the guide. */
    if (tool->guide) {
      tool->guide->position = guide_original_pos; /* So that when we undo, it goes back to original position. */
      dia_diagram_remove_guide (ddisp->diagram, tool->guide, TRUE);
      tool->guide = NULL;
    }
  }
  else
  {
    if (!tool->guide) {
      /* Add a new guide. */
      dia_diagram_add_guide (ddisp->diagram, tool->position, tool->orientation, TRUE);
    }
    else
    {
      /* Moved an existing guide, so update the undo stack. */
      dia_move_guide_change_new (ddisp->diagram, tool->guide, guide_original_pos, tool->position);
      undo_set_transactionpoint (ddisp->diagram->undo);
    }
  }

  diagram_add_update_all(ddisp->diagram);
  diagram_modified(ddisp->diagram);
  diagram_flush(ddisp->diagram);

  tool_reset();
}


static void
guide_motion (GuideTool *tool, GdkEventMotion *event, DDisplay *ddisp)
{
  int tx, ty;
  Point to;
  int disp_width;
  int disp_height;

  disp_width = dia_interactive_renderer_get_width_pixels (DIA_INTERACTIVE_RENDERER (ddisp->renderer));
  disp_height = dia_interactive_renderer_get_height_pixels (DIA_INTERACTIVE_RENDERER (ddisp->renderer));

  /* Event coordinates. */
  tx = event->x;
  ty = event->y;

  /* Minus ruler height. */
  if (tool->orientation == GTK_ORIENTATION_HORIZONTAL)
    ty -= tool->ruler_height;
  else
    tx -= tool->ruler_height;

  /* Diagram coordinates. */
  ddisplay_untransform_coords(ddisp, tx, ty, &to.x, &to.y);

  if (tx < 0 || tx >= disp_width ||
      ty < 0 || ty >= disp_height)
  {
    /* Out of bounds. */
    tool->position = GUIDE_POSITION_UNDEFINED;

    /* Cancel dragging a new guide. */
    ddisp->is_dragging_new_guideline = FALSE;
    ddisp->dragged_new_guideline_position = 0;

    if(tool->guide)
    {
      /* Deleting an existing guide. Hide it for now. */
      tool->guide->position = G_MAXDOUBLE;
    }
  }
  else
  {
    /* In bounds - add or move a guide. */
    if (tool->orientation == GTK_ORIENTATION_HORIZONTAL)
      tool->position = to.y;
    else
      tool->position = to.x;

    if(tool->guide)
    {
      /* Move existing guide. */
      tool->guide->position = tool->position;
    }
    else
    {
      /* Add new guide. */
      ddisp->is_dragging_new_guideline = TRUE;
      ddisp->dragged_new_guideline_position = tool->position;
    }
  }

  diagram_add_update_all(ddisp->diagram);
  diagram_modified(ddisp->diagram);
  ddisplay_flush(ddisp);
}

void guide_tool_set_ruler_height(Tool *tool, int height)
{
  GuideTool *gtool = (GuideTool *)tool;
  gtool->ruler_height = height;
}

void guide_tool_start_edit (DDisplay *display,
                            DiaGuide *guide)
{
  _guide_tool_start (display, guide->orientation, guide);

  if(guide)
  {
    /* Store original position for undo stack. */
    guide_original_pos = guide->position;
  }
}

void guide_tool_set_guide(Tool *tool, DiaGuide *guide)
{
  GuideTool *gtool = (GuideTool *)tool;
  gtool->guide = guide;
}

void guide_tool_set_orientation(Tool *tool, GtkOrientation orientation)
{
  GuideTool *gtool = (GuideTool *)tool;
  gtool->orientation = orientation;
}
