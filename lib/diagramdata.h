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
#ifndef DIAGRAMDATA_H
#define DIAGRAMDATA_H

#include <glib.h>

#include "diatypes.h"
/* #include "object.h" later after declaring types */
#include "color.h"
#include "geometry.h"
#include "diavar.h"
#include "paper.h"

struct _NewDiagramData {
  gchar *papertype;
  gfloat tmargin, bmargin, lmargin, rmargin;
  gboolean is_portrait;
  gfloat scaling;
  gboolean fitto;
  gint fitwidth, fitheight;
  Color bg_color, pagebreak_color, grid_color;
  int compress_save;
};

GType diagram_data_get_type (void) G_GNUC_CONST;

#define DIA_TYPE_DIAGRAM_DATA           (diagram_data_get_type ())
#define DIA_DIAGRAM_DATA(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), DIA_TYPE_DIAGRAM_DATA, DiagramData))
#define DIA_DIAGRAM_DATA_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST ((klass), DIA_TYPE_DIAGRAM_DATA, DiagramDataClass))
#define DIA_IS_DIAGRAM_DATA(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DIA_TYPE_DIAGRAM_DATA))
#define DIA_DIAGRAM_DATA_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), DIA_TYPE_DIAGRAM_DATA, DiagramDataClass))



struct _DiagramData {
  GObject parent_instance;

  Rectangle extents;      /* The extents of the diagram        */

  Color bg_color;
  Color pagebreak_color;

  PaperInfo paper;       /* info about the page info for the diagram */
  gboolean is_compressed; /* TRUE if by default it should be save compressed.
			     The user can override this in Save As... */

  struct  {
    /* grid line intervals */
    real width_x, width_y, width_w;
    /* the interval between visible grid lines */
    guint visible_x, visible_y;
    /* the interval between major lines (non-stippled).
     * if 0, no major lines are drawn (all lines are stippled).
     * if 1, all lines are solid.
     */
    guint major_lines;
    /* True if the grid is dynamically calculated.
     * When true, width_x and width_y are ignored.
     */
    gboolean dynamic;
    /* The color of the grid lines.
     */
    Color colour;
    /** True if this grid is a hex grid. */
    gboolean hex;
    /** Size of each edge on a hex grid. */
    real hex_size;
  } grid;

  struct {
    /* sorted arrays of the guides for the diagram */
    real *hguides, *vguides;
    guint nhguides, nvguides;
  } guides;

  GPtrArray *layers;     /* Layers ordered by decreasing z-order */
  Layer *active_layer;

  guint selected_count_private; /* kept for binary compatibility and sanity, don't use ! */
  GList *selected;        /* List of objects that are selected,
			     all from the active layer! */

  /** List of text fields that can be edited in the diagram.
   *  Updated by text_register_focusable. */
  GList *text_edits;
};

typedef struct _DiagramDataClass {
  GObjectClass parent_class;
} DiagramDataClass;

struct _Layer {
  char *name;
  Rectangle extents;      /* The extents of the layer        */

  GList *objects;         /* List of objects in the layer,
			     sorted by decreasing z-valued,
			     objects can ONLY be connected to objects
			     in the same layer! */

  gboolean visible;
  gboolean connectable;   /* Whether the layer can currently be connected to.
			     The selected layer is by default connectable */

  DiagramData *parent_diagram; /* Back-pointer to the diagram.  This
				  must only be set by functions internal
				  to the diagram, and accessed via
				  layer_get_parent_diagram() */
};

#include "object.h"
#include "dynamic_obj.h"

DIAVAR int render_bounding_boxes;

DiagramData *new_diagram_data(NewDiagramData *prefs);
void diagram_data_destroy(DiagramData *data);

Layer *new_layer (char *name, DiagramData *parent);
void layer_destroy(Layer *layer);

void data_raise_layer(DiagramData *data, Layer *layer);
void data_lower_layer(DiagramData *data, Layer *layer);

void data_add_layer(DiagramData *data, Layer *layer);
void data_add_layer_at(DiagramData *data, Layer *layer, int pos);
void data_set_active_layer(DiagramData *data, Layer *layer);
void data_delete_layer(DiagramData *data, Layer *layer);
void data_select(DiagramData *data, DiaObject *obj);
void data_unselect(DiagramData *data, DiaObject *obj);
void data_remove_all_selected(DiagramData *data);
gboolean data_update_extents(DiagramData *data); /* returns true if changed. */
GList *data_get_sorted_selected(DiagramData *data);
GList *data_get_sorted_selected_remove(DiagramData *data);

typedef void (*ObjectRenderer)(DiaObject *obj, DiaRenderer *renderer,
			       int active_layer,
			       gpointer data);
void data_render(DiagramData *data, DiaRenderer *renderer, Rectangle *update,
		 ObjectRenderer obj_renderer /* Can be NULL */,
		 gpointer gdata);  
void layer_render(Layer *layer, DiaRenderer *renderer, Rectangle *update,
		  ObjectRenderer obj_renderer /* Can be NULL */,
		  gpointer data,
		  int active_layer);

int layer_object_index(Layer *layer, DiaObject *obj);
void layer_add_object(Layer *layer, DiaObject *obj);
void layer_add_object_at(Layer *layer, DiaObject *obj, int pos);
void layer_add_objects(Layer *layer, GList *obj_list);
void layer_add_objects_first(Layer *layer, GList *obj_list);
void layer_remove_object(Layer *layer, DiaObject *obj);
void layer_remove_objects(Layer *layer, GList *obj_list);
GList *layer_find_objects_intersecting_rectangle(Layer *layer, Rectangle*rect);
GList *layer_find_objects_in_rectangle(Layer *layer, Rectangle *rect);
DiaObject *layer_find_closest_object(Layer *layer, Point *pos, real maxdist);
DiaObject *layer_find_closest_object_except(Layer *layer, Point *pos,
					 real maxdist, GList *avoid);
real layer_find_closest_connectionpoint(Layer *layer,
					ConnectionPoint **closest,
					Point *pos,
					DiaObject *notthis);
int layer_update_extents(Layer *layer); /* returns true if changed. */
void layer_replace_object_with_list(Layer *layer, DiaObject *obj,
				    GList *list);
void layer_set_object_list(Layer *layer, GList *list);
DiagramData *layer_get_parent_diagram(Layer *layer);
/* Make sure all objects that are in the layer and not in the new
   list eventually gets destroyed. */

#endif /* DIAGRAMDATA_H */






