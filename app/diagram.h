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
#ifndef DIAGRAM_H
#define DIAGRAM_H

#include <gtk/gtk.h>

typedef struct _Diagram Diagram;

#include "geometry.h"
#include "object.h"
#include "handle.h"
#include "connectionpoint.h"
#include "display.h"
#include "diagramdata.h"

struct _Diagram {
  char *filename;
  int unsaved;            /* True if diagram is created but not saved.*/
  int modified;
  
  DiagramData *data;
  
  guint display_count;
  GSList *displays;       /* List of all displays showing this diagram */
};

extern GList *open_diagrams; /* Read only! */

extern Diagram *new_diagram(char *filename);  /* Note: filename is copied */
extern void diagram_destroy(Diagram *dia);
extern void diagram_modified(Diagram *dia);
extern void diagram_set_modified(Diagram *dia, int modified);
extern void diagram_add_ddisplay(Diagram *dia, DDisplay *ddisp);
extern void diagram_remove_ddisplay(Diagram *dia, DDisplay *ddisp);
extern void diagram_add_object(Diagram *dia, Object *obj);
extern void diagram_add_object_list(Diagram *dia, GList *list);
extern void diagram_remove_all_selected(Diagram *diagram, int delete_empty);
extern void diagram_remove_selected(Diagram *diagram, Object *obj);
extern void diagram_add_selected(Diagram *diagram, Object *obj);
extern void diagram_add_selected_list(Diagram *diagram, GList *list);
extern int diagram_is_selected(Diagram *diagram, Object *obj);
extern GList *diagram_get_sorted_selected(Diagram *dia);
/* Removes selected from objects list, NOT selected list: */
extern GList *diagram_get_sorted_selected_remove(Diagram *dia);
extern void diagram_add_update(Diagram *dia, Rectangle *update);
extern void diagram_add_update_all(Diagram *dia);
extern void diagram_add_update_pixels(Diagram *dia, Point *point,
				      int pixel_width, int pixel_height);
extern void diagram_flush(Diagram *dia);
extern Object *diagram_find_clicked_object(Diagram *dia,
					   Point *pos,
					   real maxdist);
extern real diagram_find_closest_handle(Diagram *dia, Handle **handle,
					Object **obj, Point *pos);
extern real diagram_find_closest_connectionpoint(Diagram *dia,
						 ConnectionPoint **cp,
						 Point *pos);
extern void diagram_update_extents(Diagram *dia);

extern void diagram_place_under_selected(Diagram *dia);
extern void diagram_place_over_selected(Diagram *dia);
extern void diagram_group_selected(Diagram *dia);
extern void diagram_ungroup_selected(Diagram *dia);

extern void diagram_set_filename(Diagram *dia, char *filename);

extern void diagram_export_to_eps(Diagram *dia, char *filename);

extern int diagram_modified_exists(void);

#endif /* DIAGRAM_H */


