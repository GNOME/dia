/* Dia -- an diagram creation/manipulation program -*- c -*-
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
#include "undo.h"
#include "filter.h"
#include "menus.h"

struct _Diagram {
  char *filename;
  int unsaved;            /* True if diagram is created but not saved.*/
  int mollified;
  gboolean autosaved;     /* True if the diagram is autosaved since last mod */
  char *autosavefilename;     /* Holds the name of the current autosave file
			       * for this diagram, or NULL.  */
  
  DiagramData *data;
  
  guint display_count;
  GSList *displays;       /* List of all displays showing this diagram */

  UndoStack *undo;
};

struct _object_extent
{
  Object *object;
  Rectangle *extent;
};

typedef struct _object_extent object_extent;

GList *dia_open_diagrams(void); /* Read only! */

Diagram *diagram_load(const char *filename, DiaImportFilter *ifilter);
int diagram_load_into (Diagram *dest, const char *filename, DiaImportFilter *ifilter);
Diagram *new_diagram(const char *filename); /*Note: filename is copied*/
/** Perform updates related to getting a new current diagram */
void diagram_set_current(Diagram *diagram);
void diagram_destroy(Diagram *dia);
gboolean diagram_is_modified(Diagram *dia);
void diagram_modified(Diagram *dia);
void diagram_set_modified(Diagram *dia, int modified);
void diagram_add_ddisplay(Diagram *dia, DDisplay *ddisp);
void diagram_remove_ddisplay(Diagram *dia, DDisplay *ddisp);
void diagram_add_object(Diagram *dia, Object *obj);
void diagram_add_object_list(Diagram *dia, GList *list);
void diagram_selected_break_external(Diagram *dia);
void diagram_remove_all_selected(Diagram *diagram, int delete_empty);
void diagram_unselect_object(Diagram *diagram, Object *obj);
void diagram_unselect_objects(Diagram *dia, GList *obj_list);
void diagram_select(Diagram *diagram, Object *obj);
void diagram_select_list(Diagram *diagram, GList *list);
int diagram_is_selected(Diagram *diagram, Object *obj);
GList *diagram_get_sorted_selected(Diagram *dia);
/* Removes selected from objects list, NOT selected list: */
GList *diagram_get_sorted_selected_remove(Diagram *dia);
void diagram_add_update(Diagram *dia, Rectangle *update);
void diagram_add_update_with_border(Diagram *dia, Rectangle *update,
				    int pixel_border);
void diagram_add_update_all(Diagram *dia);
void diagram_add_update_all_all_and_flush();
void diagram_add_update_pixels(Diagram *dia, Point *point,
			       int pixel_width, int pixel_height);
void diagram_flush(Diagram *dia);
Object *diagram_find_clicked_object(Diagram *dia,
				    Point *pos,
				    real maxdist);
Object *diagram_find_clicked_object_except(Diagram *dia,
					   Point *pos,
					   real maxdist,
					   GList *avoid);
real diagram_find_closest_handle(Diagram *dia, Handle **handle,
				 Object **obj, Point *pos);
real diagram_find_closest_connectionpoint(Diagram *dia,
					  ConnectionPoint **cp,
					  Point *pos,
					  Object *notthis);
void diagram_update_extents(Diagram *dia);
gint diagram_parent_sort_cb(object_extent ** a, object_extent **b);

void diagram_update_menu_sensitivity (Diagram *dia, UpdatableMenuItems *items);
void diagram_update_menubar_sensitivity(Diagram *dia, UpdatableMenuItems *items);
void diagram_update_popupmenu_sensitivity(Diagram *dia);

void diagram_place_under_selected(Diagram *dia);
void diagram_place_over_selected(Diagram *dia);
void diagram_place_down_selected(Diagram *dia);
void diagram_place_up_selected(Diagram *dia);
void diagram_group_selected(Diagram *dia);
void diagram_ungroup_selected(Diagram *dia);
void diagram_parent_selected(Diagram *dia);
void diagram_unparent_selected(Diagram *dia);
void diagram_unparent_children_selected(Diagram *dia);

void diagram_set_filename(Diagram *dia, char *filename);
gchar *diagram_get_name(Diagram *dia);

int diagram_modified_exists(void);

void diagram_redraw_all(void);

void diagram_object_modified(Diagram *dia, Object *object);

#endif /* DIAGRAM_H */


