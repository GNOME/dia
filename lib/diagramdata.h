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
#include "font.h"
#include "diarenderer.h"

G_BEGIN_DECLS

#ifndef __in_diagram_data
#define DIA_DIAGRAM_DATA_PRIVATE(name) __priv_##name
#else
#define DIA_DIAGRAM_DATA_PRIVATE(name) name
#endif


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
  Color bg_color, pagebreak_color, grid_color, guide_color;
  int compress_save;
  gchar *unit, *font_unit;
};

GType dia_diagram_data_get_type (void);

#define DIA_TYPE_DIAGRAM_DATA           (dia_diagram_data_get_type ())
#define DIA_DIAGRAM_DATA(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), DIA_TYPE_DIAGRAM_DATA, DiagramData))
#define DIA_DIAGRAM_DATA_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST ((klass), DIA_TYPE_DIAGRAM_DATA, DiagramDataClass))
#define DIA_IS_DIAGRAM_DATA(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DIA_TYPE_DIAGRAM_DATA))
#define DIA_DIAGRAM_DATA_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), DIA_TYPE_DIAGRAM_DATA, DiagramDataClass))

/*!
 * \brief Base class for diagrams. This gets passed to plug-ins to work on diagrams.
 *
 * Dia's diagram object is the container of #DiaLayer, the managment object of #DiaObject selections
 * and text foci (#Focus) as well a highlithing state resulting from selections.
 *
 * \ingroup DiagramStructure
 */
struct _DiagramData {
  GObject parent_instance; /*!< inheritance in C */

  DiaRectangle extents;      /*!< The extents of the diagram        */

  Color bg_color;         /*!< The diagrams background color */

  PaperInfo paper;        /*!< info about the page info for the diagram */
  gboolean is_compressed; /*!< TRUE if by default it should be save compressed.
			     The user can override this in Save As... */

  /*!< Layers ordered by decreasing z-order */
  GPtrArray *DIA_DIAGRAM_DATA_PRIVATE(layers);
  /*!< The active layer, Defensive programmers check for NULL */
  DiaLayer  *DIA_DIAGRAM_DATA_PRIVATE(active_layer);

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
  void (* object_add)        (DiagramData*, DiaLayer*, DiaObject*);
  void (* object_remove)     (DiagramData*, DiaLayer*, DiaObject*);
  void (* selection_changed) (DiagramData*, int);

} DiagramDataClass;

typedef enum {
  DIA_HIGHLIGHT_NONE,
  DIA_HIGHLIGHT_CONNECTIONPOINT,
  DIA_HIGHLIGHT_CONNECTIONPOINT_MAIN,
  DIA_HIGHLIGHT_TEXT_EDIT
} DiaHighlightType;

void data_raise_layer(DiagramData *data, DiaLayer *layer);
void data_lower_layer(DiagramData *data, DiaLayer *layer);

void data_add_layer(DiagramData *data, DiaLayer *layer);
void data_add_layer_at(DiagramData *data, DiaLayer *layer, int pos);
void data_set_active_layer(DiagramData *data, DiaLayer *layer);
DiaLayer *dia_diagram_data_get_active_layer (DiagramData *self);
void data_remove_layer(DiagramData *data, DiaLayer *layer);
int  data_layer_get_index (const DiagramData *data, const DiaLayer *layer);
int data_layer_count(const DiagramData *data);
DiaLayer *data_layer_get_nth (const DiagramData *data, guint index);

void data_highlight_add(DiagramData *data, DiaObject *obj, DiaHighlightType type);
void data_highlight_remove(DiagramData *data, DiaObject *obj);
DiaHighlightType data_object_get_highlight(DiagramData *data, DiaObject *obj);

void data_select(DiagramData *data, DiaObject *obj);
void data_unselect(DiagramData *data, DiaObject *obj);
void data_remove_all_selected(DiagramData *data);
gboolean data_update_extents(DiagramData *data); /* returns true if changed. */
GList *data_get_sorted_selected(DiagramData *data);
GList *data_get_sorted_selected_remove(DiagramData *data);
void data_emit(DiagramData *data,DiaLayer *layer,DiaObject* obj,const char *signal_name);

void data_foreach_object (DiagramData *data, GFunc func, gpointer user_data);


typedef void (*ObjectRenderer)(DiaObject *obj, DiaRenderer *renderer,
			       int active_layer,
			       gpointer data);
void data_render(DiagramData *data, DiaRenderer *renderer, DiaRectangle *update,
		 ObjectRenderer obj_renderer /* Can be NULL */,
		 gpointer gdata);
void data_render_paginated(DiagramData *data, DiaRenderer *renderer, gpointer user_data);

DiagramData *diagram_data_clone (DiagramData *data);
DiagramData *diagram_data_clone_selected (DiagramData *data);

#define DIA_FOR_LAYER_IN_DIAGRAM(diagram, layer, i, body) \
  G_STMT_START {                                          \
    int __dia_layers_len = data_layer_count (diagram);    \
    for (int i = 0; i < __dia_layers_len; i++) {          \
      DiaLayer *layer = data_layer_get_nth (diagram, i);  \
      body                                                \
    }                                                     \
  } G_STMT_END

G_END_DECLS

#endif /* DIAGRAMDATA_H */
