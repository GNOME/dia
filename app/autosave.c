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

/* Autosave */

/* Automatically save a copy with the .autosave extension after idle time.
 * Don't autosave unmodified diagrams, and remove the autosave file when
 * the diagram is saved successfully.  Also remove autosave file when the
 * diagram is closed, even if it was modified.
 * If (auto)saving crashes you, this will really fuck you over!
 *
 */

#include <config.h>

#include "autosave.h"
#include "diagram.h"
#include "load_save.h"

static void
autosave_save_diagram(gpointer data)
{
  Diagram *dia = (Diagram *)data;

  g_idle_remove_by_data(data);

  diagram_autosave(dia);
}


/**
 * autosave_check_autosave:
 * @data: unused???
 *
 * Makes autosave copies of the diagrams in the appropriate directory
 * This function will be called after a diagram is modified, and will
 * only save documents that are modified and not already autosaved,
 * and only at the next idle period.
 *
 * Since: dawn-of-time
 */
gboolean
autosave_check_autosave (gpointer data)
{
  GList *diagrams = dia_open_diagrams();
  Diagram *diagram;

  while (diagrams != NULL) {
    diagram = (Diagram *)diagrams->data;
    if (diagram_is_modified(diagram) &&
	!diagram->autosaved) {
      /* Diagram has been modified.  At next idleness, save it */
      g_idle_add ((GSourceFunc)autosave_save_diagram, diagram);
    }
    diagrams = g_list_next(diagrams);
  }
  return TRUE;
}
