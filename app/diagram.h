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

#include <glib.h>

typedef struct _Diagram Diagram;

#include "geometry.h"
#include "diagramdata.h"
#include "undo.h"
#include "diagrid.h"
#include "dia-guide.h"

G_BEGIN_DECLS

GType dia_diagram_get_type (void) G_GNUC_CONST;

#define DIA_TYPE_DIAGRAM           (dia_diagram_get_type ())
#define DIA_DIAGRAM(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), DIA_TYPE_DIAGRAM, Diagram))
#define DIA_DIAGRAM_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST ((klass), DIA_TYPE_DIAGRAM, DiagramClass))
#define DIA_IS_DIAGRAM(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DIA_TYPE_DIAGRAM))
#define DIA_DIAGRAM_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), DIA_TYPE_DIAGRAM, DiagramClass))

struct _Diagram {
  DiagramData parent_instance;

  char *filename;
  int unsaved;            /* True if diagram is created but not saved.*/
  gboolean is_default;       /* True if the diagram was created as the default.*/
  int mollified;
  gboolean autosaved;     /* True if the diagram is autosaved since last mod */
  char *autosavefilename;     /* Holds the name of the current autosave file
                               * for this diagram, or NULL.  */

  Color pagebreak_color; /*!< just to show page breaks */
  DiaGrid     grid;      /*!< the display grid */

  GList *guides;         /*!< list of guides */
  Color guide_color;     /*!< color for guides */

  DiagramData *data;     /*! just for compatibility, now that the Diagram _is_ and not _has_ DiagramData */

  GSList *displays;       /* List of all displays showing this diagram */

  UndoStack *undo;
};

typedef struct _DiagramClass {
  /*< private >*/
  DiagramDataClass parent_class;

  /*< public >*/

  /* signals */
  void (* removed)           (Diagram*);

} DiagramClass;

GList *dia_open_diagrams(void); /* Read only! */

Diagram *diagram_load(const char *filename, DiaImportFilter *ifilter);
int diagram_load_into (Diagram *dest, const char *filename, DiaImportFilter *ifilter);
void diagram_destroy(Diagram *dia);
gboolean diagram_is_modified(Diagram *dia);
void diagram_modified(Diagram *dia);
void diagram_set_modified(Diagram *dia, int modified);
void diagram_add_object(Diagram *dia, DiaObject *obj);
void diagram_add_object_list(Diagram *dia, GList *list);
void diagram_selected_break_external(Diagram *dia);
void diagram_remove_all_selected(Diagram *diagram, int delete_empty);
void diagram_unselect_object(Diagram *dia, DiaObject *obj);
void diagram_unselect_objects(Diagram *dia, GList *obj_list);
void diagram_select(Diagram *diagram, DiaObject *obj);
void diagram_select_list(Diagram *diagram, GList *list);
int diagram_is_selected(Diagram *diagram, DiaObject *obj);
GList *diagram_get_sorted_selected(Diagram *dia);
/* Removes selected from objects list, NOT selected list: */
GList *diagram_get_sorted_selected_remove(Diagram *dia);
void diagram_add_update (Diagram *dia, const DiaRectangle *update);
void diagram_add_update_with_border (Diagram *dia, const DiaRectangle *update,
				    int pixel_border);
void diagram_add_update_all(Diagram *dia);
void diagram_add_update_pixels(Diagram *dia, Point *point,
			       int pixel_width, int pixel_height);
void diagram_flush(Diagram *dia);
DiaObject *diagram_find_clicked_object(Diagram *dia,
				    Point *pos,
				    real maxdist);
DiaObject *diagram_find_clicked_object_except(Diagram *dia,
					   Point *pos,
					   real maxdist,
					   GList *avoid);
real diagram_find_closest_handle(Diagram *dia, Handle **handle,
				 DiaObject **obj, Point *pos);
real diagram_find_closest_connectionpoint(Diagram *dia,
					  ConnectionPoint **cp,
					  Point *pos,
					  DiaObject *notthis);
void diagram_update_extents(Diagram *dia);

void diagram_update_menu_sensitivity (Diagram *dia);

void diagram_place_under_selected(Diagram *dia);
void diagram_place_over_selected(Diagram *dia);
void diagram_place_down_selected(Diagram *dia);
void diagram_place_up_selected(Diagram *dia);
void diagram_group_selected(Diagram *dia);
void diagram_ungroup_selected(Diagram *dia);
void diagram_parent_selected(Diagram *dia);
void diagram_unparent_selected(Diagram *dia);
void diagram_unparent_children_selected(Diagram *dia);

gboolean object_within_parent(DiaObject *obj, DiaObject *parent);
gchar *diagram_get_name(Diagram *dia);

int diagram_modified_exists(void);

void diagram_redraw_all(void);

void diagram_object_modified(Diagram *dia, DiaObject *object);

Diagram  *dia_diagram_new               (GFile          *file);
void      dia_diagram_set_file          (Diagram        *self,
                                         GFile          *file);
GFile    *dia_diagram_get_file          (Diagram        *self);
DiaGuide *dia_diagram_add_guide         (Diagram        *dia,
                                         real            position,
                                         GtkOrientation  orientation,
                                         gboolean        push_undo);
DiaGuide *dia_diagram_pick_guide        (Diagram        *dia,
                                         gdouble         x,
                                         gdouble         y,
                                         gdouble         epsilon_x,
                                         gdouble         epsilon_y);
DiaGuide *dia_diagram_pick_guide_h      (Diagram        *dia,
                                         gdouble         x,
                                         gdouble         y,
                                         gdouble         epsilon_x,
                                         gdouble         epsilon_y);
DiaGuide *dia_diagram_pick_guide_v      (Diagram        *dia,
                                         gdouble         x,
                                         gdouble         y,
                                         gdouble         epsilon_x,
                                         gdouble         epsilon_y);
void      dia_diagram_remove_guide      (Diagram        *dia,
                                         DiaGuide       *guide,
                                         gboolean        push_undo);
void      dia_diagram_remove_all_guides (Diagram        *dia);


G_END_DECLS

#endif /* DIAGRAM_H */

