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
/*! \file diagramdata.h -- Describing the base class of diagrams */
#ifndef DIAGRAMDATA_H
#define DIAGRAMDATA_H

#include <glib.h>
#include <string.h>

#include "diatypes.h"
#include "color.h"
#include "geometry.h"
#include "paper.h"

G_BEGIN_DECLS

/*!
 * \brief Helper to create new diagram
 */
struct _NewDiagramData {
  gchar *papertype;
  gfloat tmargin, bmargin, lmargin, rmargin;
  gboolean is_portrait;
  gfloat scaling;
  gboolean fitto;
  gint fitwidth, fitheight;
  Color bg_color, pagebreak_color, grid_color;
  int compress_save;
  gchar *unit, *font_unit;
};

GType diagram_data_get_type (void) G_GNUC_CONST;

#define DIA_TYPE_DIAGRAM_DATA           (diagram_data_get_type ())
#define DIA_DIAGRAM_DATA(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), DIA_TYPE_DIAGRAM_DATA, DiagramData))
#define DIA_DIAGRAM_DATA_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST ((klass), DIA_TYPE_DIAGRAM_DATA, DiagramDataClass))
#define DIA_IS_DIAGRAM_DATA(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DIA_TYPE_DIAGRAM_DATA))
#define DIA_DIAGRAM_DATA_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), DIA_TYPE_DIAGRAM_DATA, DiagramDataClass))

/*!
 * \brief Base class for diagrams. This gets passed to plug-ins to work on diagrams.
 *
 * Dia's diagram object is the container of _Layer, the managment object of _DiaObject selections
 * and text foci (_Focus) as well a highlithing state resulting from selections.
 *
 * \ingroup DiagramStructure
 */
struct _DiagramData {
  GObject parent_instance; /*!< inheritance in C */

  Rectangle extents;      /*!< The extents of the diagram        */

  Color bg_color;         /*!< The diagrams background color */

  PaperInfo paper;        /*!< info about the page info for the diagram */
  gboolean is_compressed; /*!< TRUE if by default it should be save compressed.
			     The user can override this in Save As... */

  GPtrArray *layers;     /*!< Layers ordered by decreasing z-order */
  Layer *active_layer;   /*!< The active layer, Defensive programmers check for NULL */

  guint selected_count_private; /*!< kept for binary compatibility and sanity, don't use ! */
  GList *selected;        /*!< List of objects that are selected,
			     all from the active layer! */

  /** List of text fields (foci) that can be edited in the diagram.
   *  Updated by focus.c */
  GList *text_edits;
  /** Units and font units used in this diagram.  Default cm and point */
  gchar *unit, *font_unit;
  /** The focus (from text_edits) that's currently being edited, if any.
   *  Updated by focus.c */
  Focus *active_text_edit;

  GList *highlighted;        /*!< List of objects that are highlighted */

};

/* DiagramData vtable */
typedef struct _DiagramDataClass {
  GObjectClass parent_class;

  /* Signals */
  void (* object_add)        (DiagramData*, Layer*, DiaObject*);
  void (* object_remove)     (DiagramData*, Layer*, DiaObject*);
  void (* selection_changed) (DiagramData*, int);

} DiagramDataClass;

/*! 
 * \brief A diagram consists of layers holding objects 
 *
 * \ingroup DiagramStructure
 * \todo : make this a GObject as well
 */
struct _Layer {
  char *name;             /*!< The name of the layer */
  Rectangle extents;      /*!< The extents of the layer */

  GList *objects;         /*!< List of objects in the layer,
			     sorted by decreasing z-value,
			     objects can ONLY be connected to objects
			     in the same layer! */

  gboolean visible;       /*!< The visibility of the layer */
  gboolean connectable;   /*!< Whether the layer can currently be connected to.
			     The selected layer is by default connectable */

  DiagramData *parent_diagram; /*!< Back-pointer to the diagram.  This
				  must only be set by functions internal
				  to the diagram, and accessed via
				  layer_get_parent_diagram() */
};

typedef enum {
  DIA_HIGHLIGHT_NONE,
  DIA_HIGHLIGHT_CONNECTIONPOINT,
  DIA_HIGHLIGHT_CONNECTIONPOINT_MAIN,
  DIA_HIGHLIGHT_TEXT_EDIT
} DiaHighlightType;

Layer *new_layer (char *name, DiagramData *parent);
void layer_destroy(Layer *layer);

void data_raise_layer(DiagramData *data, Layer *layer);
void data_lower_layer(DiagramData *data, Layer *layer);

void data_add_layer(DiagramData *data, Layer *layer);
void data_add_layer_at(DiagramData *data, Layer *layer, int pos);
void data_set_active_layer(DiagramData *data, Layer *layer);
void data_remove_layer(DiagramData *data, Layer *layer);
int  data_layer_get_index (const DiagramData *data, const Layer *layer);
int data_layer_count(const DiagramData *data);
Layer *data_layer_get_nth (const DiagramData *data, guint index);

void data_highlight_add(DiagramData *data, DiaObject *obj, DiaHighlightType type);
void data_highlight_remove(DiagramData *data, DiaObject *obj);
DiaHighlightType data_object_get_highlight(DiagramData *data, DiaObject *obj);

void data_select(DiagramData *data, DiaObject *obj);
void data_unselect(DiagramData *data, DiaObject *obj);
void data_remove_all_selected(DiagramData *data);
gboolean data_update_extents(DiagramData *data); /* returns true if changed. */
GList *data_get_sorted_selected(DiagramData *data);
GList *data_get_sorted_selected_remove(DiagramData *data);
void data_emit(DiagramData *data,Layer *layer,DiaObject* obj,const char *signal_name);

void data_foreach_object (DiagramData *data, GFunc func, gpointer user_data);


typedef void (*ObjectRenderer)(DiaObject *obj, DiaRenderer *renderer,
			       int active_layer,
			       gpointer data);
void data_render(DiagramData *data, DiaRenderer *renderer, Rectangle *update,
		 ObjectRenderer obj_renderer /* Can be NULL */,
		 gpointer gdata);
void data_render_paginated(DiagramData *data, DiaRenderer *renderer, gpointer user_data);

void layer_render(Layer *layer, DiaRenderer *renderer, Rectangle *update,
		  ObjectRenderer obj_renderer /* Can be NULL */,
		  gpointer data,
		  int active_layer);

DiagramData *diagram_data_clone (DiagramData *data);
DiagramData *diagram_data_clone_selected (DiagramData *data);

int layer_object_get_index(Layer *layer, DiaObject *obj);
DiaObject *layer_object_get_nth(Layer *layer, guint index);
int layer_object_count(Layer *layer);
gchar *layer_get_name(Layer *layer);
void layer_add_object(Layer *layer, DiaObject *obj);
void layer_add_object_at(Layer *layer, DiaObject *obj, int pos);
void layer_add_objects(Layer *layer, GList *obj_list);
void layer_add_objects_first(Layer *layer, GList *obj_list);
void layer_remove_object(Layer *layer, DiaObject *obj);
void layer_remove_objects(Layer *layer, GList *obj_list);
GList *layer_find_objects_intersecting_rectangle(Layer *layer, Rectangle*rect);
GList *layer_find_objects_in_rectangle(Layer *layer, Rectangle *rect);
GList *layer_find_objects_containing_rectangle(Layer *layer, Rectangle *rect);
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

G_END_DECLS

#endif /* DIAGRAMDATA_H */






