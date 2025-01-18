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

/**
 * SECTION:diagramdata
 *
 * This file defines the #DiagramData object, which holds (mostly) saveable
 * data global to a diagram.
 */

#include "config.h"

#include <glib/gi18n-lib.h>

#define __in_diagram_data
#include "diagramdata.h"
#undef __in_diagram_data
#include "diarenderer.h"
#include "diainteractiverenderer.h"
#include "paper.h"
#include "persistence.h"
#include "dia-layer.h"

#include "dynamic_obj.h"
#include "diamarshal.h"

G_DEFINE_TYPE (DiagramData, dia_diagram_data, G_TYPE_OBJECT)


static const DiaRectangle invalid_extents = { -1.0,-1.0,-1.0,-1.0 };


typedef struct {
  DiaObject *obj;
  DiaHighlightType type;
} ObjectHighlight;


enum {
  PROP_0,
  PROP_ACTIVE_LAYER,
  LAST_PROP
};
static GParamSpec *pspecs[LAST_PROP] = { NULL, };


enum {
  OBJECT_ADD,
  OBJECT_REMOVE,
  SELECTION_CHANGED,
  LAYERS_CHANGED,
  LAST_SIGNAL
};
static guint signals[LAST_SIGNAL] = { 0, };


/* signal default handlers */
static void
_diagram_data_object_add (DiagramData *dia,
                          DiaLayer    *layer,
                          DiaObject   *obj)
{
}

static void
_diagram_data_object_remove (DiagramData *dia,
                             DiaLayer    *layer,
                             DiaObject   *obj)
{
}

static void
_diagram_data_selection_changed (DiagramData* dia, int n)
{
}


static void
dia_diagram_data_init (DiagramData *data)
{
  Color* color = persistence_register_color ("new_diagram_bgcolour", &color_white);
  gboolean compress = persistence_register_boolean ("compress_save", TRUE);
  DiaLayer *first_layer;

  data->extents.left = 0.0;
  data->extents.right = 10.0;
  data->extents.top = 0.0;
  data->extents.bottom = 10.0;

  data->bg_color = *color;

  get_paper_info (&data->paper, -1, NULL);

  data->selected_count_private = 0;
  data->selected = NULL;

  data->highlighted = NULL;

  data->is_compressed = compress; /* Overridden by doc */

  data->text_edits = NULL;
  data->active_text_edit = NULL;

  first_layer = dia_layer_new (_("Background"), data);

  data->layers = g_ptr_array_new_with_free_func (g_object_unref);
  data_add_layer (data, first_layer);
  data_set_active_layer (data, first_layer);

  g_clear_object (&first_layer);
}


static void
active_layer_died (gpointer  data,
                   GObject  *dead_object)
{
  DiagramData *self = DIA_DIAGRAM_DATA (data);

  self->active_layer = NULL;

  g_object_notify_by_pspec (G_OBJECT (self), pspecs[PROP_ACTIVE_LAYER]);
}


static void
diagram_data_finalize (GObject *object)
{
  DiagramData *data = DIA_DIAGRAM_DATA (object);

  g_clear_pointer (&data->paper.name, g_free);

  if (data->active_layer) {
    g_object_weak_unref (G_OBJECT (data->active_layer),
                         active_layer_died,
                         data);
  }
  g_ptr_array_free (data->layers, TRUE);

  g_list_free (data->selected);
  data->selected = NULL; /* for safety */
  data->selected_count_private = 0;

  G_OBJECT_CLASS (dia_diagram_data_parent_class)->finalize (object);
}


static void
dia_diagram_data_set_property (GObject      *object,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  DiagramData *self = DIA_DIAGRAM_DATA (object);

  switch (property_id) {
    case PROP_ACTIVE_LAYER:
      data_set_active_layer (self, g_value_get_object (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}


static void
dia_diagram_data_get_property (GObject    *object,
                               guint       property_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  DiagramData *self = DIA_DIAGRAM_DATA (object);

  switch (property_id) {
    case PROP_ACTIVE_LAYER:
      g_value_set_object (value, dia_diagram_data_get_active_layer (self));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}


/*!
 * \brief Create a copy of the whole diagram data
 *
 * This is kind of a deep copy, everything not immutable gets duplicated.
 * Still this is supposed to be relatively fast, because the most expensive
 * objects have immutuable properties, namely the pixbufs in DiaImage are
 * only referenced.
 *
 * \memberof _DiagramData
 */
DiagramData *
diagram_data_clone (DiagramData *data)
{
  DiagramData *clone;

  clone = g_object_new (DIA_TYPE_DIAGRAM_DATA, NULL);

  clone->extents = data->extents;
  clone->bg_color = data->bg_color;
  clone->paper = data->paper; /* so ugly */
  clone->paper.name = g_strdup (data->paper.name);
  clone->is_compressed = data->is_compressed;

  data_remove_layer (clone, data_layer_get_nth (clone, 0));

  DIA_FOR_LAYER_IN_DIAGRAM (data, src_layer, i, {
    DiaLayer *dest_layer = dia_layer_new_from_layer (src_layer);

    data_add_layer (clone, dest_layer);

    g_clear_object (&dest_layer);
  });

  data_set_active_layer (clone, dia_diagram_data_get_active_layer (data));

  return clone;
}

/*!
 * \brief Create a new diagram data object only containing the selected objects
 * \memberof _DiagramData
 */
DiagramData *
diagram_data_clone_selected (DiagramData *data)
{
  DiagramData *clone;
  DiaLayer *src_layer;
  DiaLayer *dest_layer;
  GList *sorted;

  clone = g_object_new (DIA_TYPE_DIAGRAM_DATA, NULL);

  clone->extents = data->extents;
  clone->bg_color = data->bg_color;
  clone->paper = data->paper; /* so ugly */
  clone->paper.name = g_strdup (data->paper.name);
  clone->is_compressed = data->is_compressed;

  src_layer = dia_diagram_data_get_active_layer (data);
  /* the selection - if any - can only be on the active layer */
  dest_layer = dia_diagram_data_get_active_layer (clone);

  g_object_set (dest_layer,
                "name", dia_layer_get_name (src_layer),
                "connectable", dia_layer_is_connectable (src_layer),
                "visible", dia_layer_is_visible (src_layer),
                NULL);

  sorted = data_get_sorted_selected (data);
  dia_layer_set_object_list (dest_layer, object_copy_list (sorted));
  g_list_free (sorted);

  data_update_extents (clone);

  return clone;
}


static void
dia_diagram_data_class_init (DiagramDataClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = diagram_data_finalize;
  object_class->set_property = dia_diagram_data_set_property;
  object_class->get_property = dia_diagram_data_get_property;

  klass->object_add = _diagram_data_object_add;
  klass->object_remove = _diagram_data_object_remove;
  klass->selection_changed = _diagram_data_selection_changed;

  /**
   * DiagramData:active-layer:
   *
   * Since: 0.98
   */
  pspecs[PROP_ACTIVE_LAYER] =
    g_param_spec_object ("active-layer",
                         "Active Layer",
                         "The active layer",
                         DIA_TYPE_LAYER,
                         G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, LAST_PROP, pspecs);

  signals[OBJECT_ADD] =
    g_signal_new ("object_add",
              G_TYPE_FROM_CLASS (klass),
              G_SIGNAL_RUN_FIRST,
              G_STRUCT_OFFSET (DiagramDataClass, object_add),
              NULL, NULL,
              dia_marshal_VOID__POINTER_POINTER,
              G_TYPE_NONE,
              2,
              G_TYPE_POINTER,
              G_TYPE_POINTER);

  signals[OBJECT_REMOVE] =
    g_signal_new ("object_remove",
              G_TYPE_FROM_CLASS (klass),
              G_SIGNAL_RUN_FIRST,
              G_STRUCT_OFFSET (DiagramDataClass, object_remove),
              NULL, NULL,
              dia_marshal_VOID__POINTER_POINTER,
              G_TYPE_NONE,
              2,
              G_TYPE_POINTER,
              G_TYPE_POINTER);

  signals[SELECTION_CHANGED] =
    g_signal_new ("selection_changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (DiagramDataClass, selection_changed),
                  NULL, NULL,
                  dia_marshal_VOID__INT,
                  G_TYPE_NONE, 1,
                  G_TYPE_INT);

  /**
   * DiagramData::layers-changed:
   * @self: the #DiagramData
   * @position: where the change happened
   * @removed: number of layers removed
   * @added: number of layers added
   *
   * Since: 0.98
   */
  signals[LAYERS_CHANGED] =
    g_signal_new ("layers-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  0, NULL, NULL,
                  dia_marshal_VOID__UINT_UINT_UINT,
                  G_TYPE_NONE, 3,
                  G_TYPE_UINT, G_TYPE_UINT, G_TYPE_UINT);
}


/**
 * data_raise_layer:
 * @data: The diagram that the layer belongs to.
 * @layer: The layer to raise.
 *
 * Raise a layer up one in a diagram.
 *
 * Since: dawn-of-time
 */
void
data_raise_layer (DiagramData *data, DiaLayer *layer)
{
  int layer_nr = -1;
  DiaLayer *tmp;

  layer_nr = data_layer_get_index (data, layer);

  g_return_if_fail (layer_nr >= 0);

  if (layer_nr > 0) {
    tmp = g_ptr_array_index (data->layers, layer_nr - 1);
    g_ptr_array_index (data->layers, layer_nr - 1) =
                                g_ptr_array_index (data->layers, layer_nr);
    g_ptr_array_index (data->layers, layer_nr) = tmp;

    g_signal_emit (data,
                   signals[LAYERS_CHANGED],
                   0,
                   layer_nr - 1,
                   2,
                   2);
  }
}


/**
 * data_lower_layer:
 * @data: The diagram that the layer belongs to.
 * @layer: The layer to lower.
 *
 * Lower a layer by one in a diagram.
 *
 * Since: dawn-of-time
 */
void
data_lower_layer (DiagramData *data, DiaLayer *layer)
{
  int layer_nr = -1;
  DiaLayer *tmp;

  layer_nr = data_layer_get_index (data, layer);

  g_return_if_fail (layer_nr >= 0);

  if (layer_nr < data_layer_count (data) - 1) {
    tmp = g_ptr_array_index (data->layers, layer_nr);
    g_ptr_array_index (data->layers, layer_nr) =
                                g_ptr_array_index (data->layers, layer_nr + 1);
    g_ptr_array_index (data->layers, layer_nr + 1) = tmp;

    g_signal_emit (data,
                   signals[LAYERS_CHANGED],
                   0,
                   layer_nr,
                   2,
                   2);
  }
}


/**
 * data_add_layer:
 * @data: The diagram to add the layer to.
 * @layer: The layer to add.
 *
 * Add a layer object to a diagram.
 */
void
data_add_layer (DiagramData *data, DiaLayer *layer)
{
  data_add_layer_at (data, layer, data_layer_count (data));
}


/**
 * data_add_layer_at:
 * @data: the diagram
 * @layer: layer to add
 * @pos: the position in the layer stack
 *
 * Add a layer at a defined postion in the stack
 *
 * If the given pos is out of range the layer is added at the top of the stack.
 */
void
data_add_layer_at (DiagramData *data, DiaLayer *layer, int pos)
{
  int len;
  int i;

  g_ptr_array_add (data->layers, g_object_ref (layer));
  len = data_layer_count (data);

  if ((pos >= 0) && (pos < len)) {
    for (i = len - 1; i > pos; i--) {
      g_ptr_array_index (data->layers, i) = g_ptr_array_index (data->layers, i - 1);
    }
    g_ptr_array_index (data->layers, pos) = layer;
  }

  g_signal_emit (data, signals[LAYERS_CHANGED], 0, pos, 0, 1);

  dia_layer_set_parent_diagram (layer, data);
  data_emit (data, layer, NULL, "object_add");
  dia_layer_update_extents (layer);
  data_update_extents (data);
}


/*!
 * \brief Get the index of a layer contained in the diagram
 *
 * @param data the diagram
 * @param layer the layer
 * @return the zero based index or -1 if not found
 *
 * \memberof _DiagramData
 */
int
data_layer_get_index (const DiagramData *data, const DiaLayer *layer)
{
  DIA_FOR_LAYER_IN_DIAGRAM (data, current, i, {
    if (layer == current) {
      return i;
    }
  });

  return -1;
}


/**
 * data_layer_get_nth:
 * @data: the #DiagramData
 * @index: the layer position
 *
 * Get the layer at position @index
 *
 * Returns: the #DiaLayer or %NULL if not found
 *
 * Since: dawn-of-time
 */
DiaLayer *
data_layer_get_nth (const DiagramData *data, guint index)
{
  g_return_val_if_fail (DIA_IS_DIAGRAM_DATA (data), NULL);
  g_return_val_if_fail (data->layers, NULL);

  if (index < data_layer_count (data)) {
    return g_ptr_array_index (data->layers, index);
  }

  return NULL;
}


/**
 * data_layer_count:
 * @data: the #DiagramData
 *
 * Get the number of layers in @data
 *
 * Returns: the count
 *
 * Since: dawn-of-time
 */
int
data_layer_count (const DiagramData *data)
{
  g_return_val_if_fail (DIA_IS_DIAGRAM_DATA (data), -1);
  g_return_val_if_fail (data->layers, -1);

  return data->layers->len;
}


/**
 * data_set_active_layer:
 * @data: The diagram in which to set the active layer.
 * @layer: The layer that should be active.
 *
 * Set which layer is the active layer in a diagram.
 */
void
data_set_active_layer (DiagramData *data, DiaLayer *layer)
{
  g_return_if_fail (DIA_IS_DIAGRAM_DATA (data));

  if (data->active_layer == layer) {
    return;
  }

  if (data->active_layer) {
    g_object_weak_unref (G_OBJECT (data->active_layer),
                         active_layer_died,
                         data);
  }
  data->active_layer = layer;
  g_object_weak_ref (G_OBJECT (data->active_layer), active_layer_died, data);

  g_object_notify_by_pspec (G_OBJECT (data), pspecs[PROP_ACTIVE_LAYER]);
}


/**
 * dia_diagram_data_get_active_layer:
 * @self: the #DiagramData
 *
 * Get the active layer, see data_set_active_layer()
 *
 * Returns: the active #DiaLayer, or %NULL
 *
 * Since: 0.98
 */
DiaLayer *
dia_diagram_data_get_active_layer (DiagramData *self)
{
  g_return_val_if_fail (DIA_IS_DIAGRAM_DATA (self), NULL);

  return self->active_layer;
}


/**
 * data_remove_layer:
 * @data: The diagram to remove the layer from.
 * @layer: The layer to remove.
 *
 * Remove a layer from a diagram.
 */
void
data_remove_layer (DiagramData *data, DiaLayer *layer)
{
  DiaLayer *active;
  int location;

  if (data_layer_count (data) <= 1) {
    return;
  }

  active = dia_diagram_data_get_active_layer (data);

  if (active == layer) {
    data_remove_all_selected (data);
  }

  data_emit (data, layer, NULL, "object_remove");

  g_object_ref (layer);
  location = data_layer_get_index (data, layer);
  g_ptr_array_remove_index (data->layers, location);

  g_signal_emit (data, signals[LAYERS_CHANGED], 0, location, 1, 0);

  if (active == layer || active == NULL) {
    DiaLayer *next_layer = data_layer_get_nth (data, location);
    if (next_layer) {
      data_set_active_layer (data, next_layer);
    } else {
      data_set_active_layer (data, data_layer_get_nth (data, location - 1));
    }
  }

  dia_layer_set_parent_diagram (layer, NULL);
  g_object_unref (layer);
}


static ObjectHighlight *
find_object_highlight(GList *list, DiaObject *obj)
{
  ObjectHighlight *oh=NULL;
  while(list) {
    oh = (ObjectHighlight*)list->data;
    if (oh && oh->obj == obj) {
      return oh;
    }
    list = g_list_next(list);
  }
  return NULL;
}


void
data_highlight_add (DiagramData *data, DiaObject *obj, DiaHighlightType type)
{
  ObjectHighlight *oh;

  if (find_object_highlight (data->highlighted, obj)) {
    return; /* should this be an error?`*/
  }

  oh = g_new0 (ObjectHighlight, 1);
  oh->obj = obj;
  oh->type = type;
  data->highlighted = g_list_prepend (data->highlighted, oh);
}


void
data_highlight_remove (DiagramData *data, DiaObject *obj)
{
  ObjectHighlight *oh;

  if (!(oh = find_object_highlight (data->highlighted, obj))) {
    return; /* should this be an error?`*/
  }

  data->highlighted = g_list_remove (data->highlighted, oh);

  g_clear_pointer (&oh, g_free);
}


DiaHighlightType
data_object_get_highlight(DiagramData *data, DiaObject *obj)
{
  ObjectHighlight *oh;
  DiaHighlightType type = DIA_HIGHLIGHT_NONE;
  if ((oh = find_object_highlight (data->highlighted, obj)) != NULL) {
    type = oh->type;
  }
  return type;
}

/*!
 * \brief Select an object in a diagram
 *
 * Note that this does not unselect other objects currently selected in the diagram.
 * @param data The diagram to select in.
 * @param obj The object to select
 *
 * \memberof _DiagramData
 */
void
data_select(DiagramData *data, DiaObject *obj)
{
  if (g_list_find (data->selected, obj))
    return; /* should this be an error?`*/
  data->selected = g_list_prepend(data->selected, obj);
  data->selected_count_private++;
  g_signal_emit (data, signals[SELECTION_CHANGED], 0, data->selected_count_private);
}

/*!
 * \brief Deselect an object in a diagram
 *
 * Note that other objects may still be selected after this function is done.
 * @param data The diagram to deselect in.
 * @param obj The object to deselect.
 *
 * \memberof _DiagramData
 */
void
data_unselect(DiagramData *data, DiaObject *obj)
{
  if (!g_list_find (data->selected, obj))
    return; /* should this be an error?`*/
  data->selected = g_list_remove(data->selected, obj);
  data->selected_count_private--;
  g_signal_emit (data, signals[SELECTION_CHANGED], 0, data->selected_count_private);
}

/*!
 * \brief Clears the list of selected objects.
 * Does *not* remove these objects from the object list, despite its name.
 * @param data The diagram to unselect all objects in.
 * \memberof _DiagramData
 */
void
data_remove_all_selected(DiagramData *data)
{
  g_list_free(data->selected); /* Remove previous selection */
  data->selected = NULL;
  data->selected_count_private = 0;
  g_signal_emit (data, signals[SELECTION_CHANGED], 0, data->selected_count_private);
}


/**
 * data_has_visible_layers:
 * @data: The #DiagramData to check.
 *
 * Return %TRUE if the diagram has visible layers.
 *
 * Returns: %TRUE if at least one layer in the diagram is visible.
 *
 * Since: dawn-of-time
 */
static gboolean
data_has_visible_layers (DiagramData *data)
{
  DIA_FOR_LAYER_IN_DIAGRAM (data, layer, i, {
    if (dia_layer_is_visible (layer)) {
      return TRUE;
    }
  });

  return FALSE;
}


/**
 * data_get_layers_extents_union:
 * @data: The #DiagramData to get the extents for.
 *
 * Set the diagram extents field to the union of the extents of the layers.
 *
 * Since: dawn-of-time
 */
static void
data_get_layers_extents_union (DiagramData *data)
{
  gboolean first = TRUE;
  DiaRectangle new_extents;

  DIA_FOR_LAYER_IN_DIAGRAM (data, layer, i, {
    if (!dia_layer_is_visible (layer)) {
      continue;
    }

    dia_layer_update_extents (layer);

    if (first) {
      dia_layer_get_extents (layer, &new_extents);
      first = rectangle_equals (&new_extents, &invalid_extents);
    } else {
      DiaRectangle extents;

      dia_layer_get_extents (layer, &extents);

      if (!rectangle_equals (&extents, &invalid_extents)) {
        rectangle_union (&new_extents, &extents);
      }
    }
  });

  data->extents = new_extents;
}


/*!
 * \brief Change diagram scaling so that the extents are exactly visible.
 * @param data The diagram to adjust.
 * \protected \memberof _DiagramData
 */
static void
data_adapt_scaling_to_extents(DiagramData *data)
{
  gdouble pwidth = data->paper.width * data->paper.scaling;
  gdouble pheight = data->paper.height * data->paper.scaling;

  gdouble xscale = data->paper.fitwidth * pwidth /
    (data->extents.right - data->extents.left);
  gdouble yscale = data->paper.fitheight * pheight /
    (data->extents.bottom - data->extents.top);

  data->paper.scaling = (float)MIN(xscale, yscale);
  data->paper.width  = (float)(pwidth  / data->paper.scaling);
  data->paper.height = (float)(pheight / data->paper.scaling);
}


/*!
 * \brief Adjust the extents field of a diagram.
 * @param data The diagram to adjust.
 * @return TRUE if the extents changed.
 * \protected \memberof _DiagramData
 */
static gboolean
data_compute_extents (DiagramData *data)
{
  DiaRectangle old_extents = data->extents;

  if (!data_has_visible_layers (data)) {
    if (data_layer_count (data) > 0) {
      DiaLayer *layer = data_layer_get_nth (data, 0);

      dia_layer_update_extents (layer);

      dia_layer_get_extents (layer, &data->extents);
    } else {
      data->extents = invalid_extents;
    }
  } else {
    data_get_layers_extents_union (data);
  }

  if (rectangle_equals (&data->extents, &invalid_extents)) {
    data->extents.left = 0.0;
    data->extents.right = 10.0;
    data->extents.top = 0.0;
    data->extents.bottom = 10.0;
  }

  return (!rectangle_equals (&data->extents, &old_extents));
}


/*!
 * \brief Update the extents of a diagram and adjust scaling if needed.
 * @param data Diagram to update.
 * @return TRUE if the diagram extents changed.
 * \memberof _DiagramData
 */
gboolean
data_update_extents(DiagramData *data)
{

  gboolean changed;

  changed = data_compute_extents(data);
  if (changed && data->paper.fitto) data_adapt_scaling_to_extents(data);

  return changed;
}

/*!
 * \brief Get a list of selected objects in layer ordering.
 * @param data The diagram to get objects from.
 * @return A list of all currently selected objects.  These all reside in
 *  the currently active layer.  This list should be freed after use.
 * \memberof _DiagramData
 */
GList *
data_get_sorted_selected (DiagramData *data)
{
  GList *list;
  GList *sorted_list;
  GList *found;
  DiaObject *obj;

  g_assert (g_list_length (data->selected) == data->selected_count_private);
  if (data->selected_count_private == 0)
    return NULL;

  sorted_list = NULL;
  list = g_list_last (dia_layer_get_object_list (dia_diagram_data_get_active_layer (data)));
  while (list != NULL) {
    found = g_list_find (data->selected, list->data);
    if (found) {
      obj = (DiaObject *) found->data;
      sorted_list = g_list_prepend (sorted_list, obj);
    }
    list = g_list_previous (list);
  }

  return sorted_list;
}

/*!
 * \brief Remove the currently selected objects from the list of objects.
 *
 * The selected objects are returned in a newly created GList.
 * @param data The diagram to get and remove the selected objects from.
 * @return A list of all selected objects in the current diagram.  This list
 *  should be freed after use.  Unlike data_get_sorted_selected, the
 *  objects in the list are not in the diagram anymore.
 * \memberof _DiagramData
 */
GList *
data_get_sorted_selected_remove (DiagramData *data)
{
  GList *list;
  GList *sorted_list;
  GList *found;
  DiaObject *obj;

  g_assert (g_list_length (data->selected) == data->selected_count_private);
  if (data->selected_count_private == 0)
    return NULL;

  sorted_list = NULL;
  list = g_list_last (dia_layer_get_object_list (dia_diagram_data_get_active_layer (data)));
  while (list != NULL) {
    found = g_list_find (data->selected, list->data);
    if (found) {
      obj = (DiaObject *) found->data;
      sorted_list = g_list_prepend (sorted_list, obj);

      list = g_list_previous (list);

      dia_layer_remove_object (dia_diagram_data_get_active_layer (data), obj);
    } else {
      list = g_list_previous (list);
    }
  }

  return sorted_list;
}


/*!
 * \brief Emits a GObject signal on DiagramData
 * @param data The DiagramData that emits the signal.
 * @param layer The Layer that the fired signal carries.
 * @param obj The DiaObject that the fired signal carries.
 * @param signal_name The name of the signal.
 * \memberof _DiagramData
 */
void
data_emit (DiagramData *data,
           DiaLayer    *layer,
           DiaObject   *obj,
	  const char *signal_name)
{
  /* check what signal it is */
  if (strcmp("object_add",signal_name) == 0)
    g_signal_emit(data, signals[OBJECT_ADD], 0, layer, obj);

  if (strcmp("object_remove",signal_name) == 0)
    g_signal_emit(data, signals[OBJECT_REMOVE], 0, layer, obj);

}


/*!
 * \brief Render a diagram
 * @param data The diagram to render.
 * @param renderer The renderer to render on.
 * @param update The area that needs updating or NULL
 * @param obj_renderer If non-NULL, an alternative renderer of objects.
 * @param gdata User data passed on to inner calls.
 * \memberof _DiagramData
 */
void
data_render (DiagramData    *data,
             DiaRenderer    *renderer,
             DiaRectangle   *update,
             ObjectRenderer  obj_renderer,
             gpointer        gdata)
{
  DiaLayer *active;
  guint active_layer;

  if (!DIA_IS_INTERACTIVE_RENDERER (renderer)) {
    dia_renderer_begin_render (renderer, update);
  }

  active = dia_diagram_data_get_active_layer (data);

  DIA_FOR_LAYER_IN_DIAGRAM (data, layer, i, {
    active_layer = (layer == active);
    if (dia_layer_is_visible (layer)) {
      if (obj_renderer) {
        dia_layer_render (layer, renderer, update, obj_renderer, gdata, active_layer);
      } else {
        dia_renderer_draw_layer (renderer, layer, active_layer, update);
      }
    }
  });

  if (!DIA_IS_INTERACTIVE_RENDERER (renderer)) {
    dia_renderer_end_render (renderer);
  }
}

/*!
 * \brief Calls data_render() for paginated formats
 *
 * Call data_render() for every used page in the diagram
 *
 * \memberof _DiagramData
 */
void
data_render_paginated (DiagramData *data, DiaRenderer *renderer, gpointer user_data)
{
  DiaRectangle *extents;
  gdouble width, height;
  gdouble x, y, initx, inity;
  gint xpos, ypos;

  /* the usable area of the page */
  width = data->paper.width;
  height = data->paper.height;

  /* get extents, and make them multiples of width / height */
  extents = &data->extents;
  initx = extents->left;
  inity = extents->top;
  /* make page boundaries align with origin */
  if (!data->paper.fitto) {
    initx = floor(initx / width)  * width;
    inity = floor(inity / height) * height;
  }

  /* iterate through all the pages in the diagram */
  for (y = inity, ypos = 0; y < extents->bottom; y += height, ypos++) {
    /* ensure we are not producing pages for epsilon */
    if ((extents->bottom - y) < 1e-6)
      break;

    for (x = initx, xpos = 0; x < extents->right; x += width, xpos++) {
      DiaRectangle page_bounds;

      if ((extents->right - x) < 1e-6)
        break;

      page_bounds.left = x;
      page_bounds.right = x + width;
      page_bounds.top = y;
      page_bounds.bottom = y + height;

      data_render (data, renderer, &page_bounds, NULL, user_data);
    }
  }
}


/*!
 * \brief Visit all objects within the diagram
 * @param data the diagram
 * @param func per object callback function
 * @param user_data data passed to the callback function
 * \memberof _DiagramData
 */
void
data_foreach_object (DiagramData *data, GFunc func, gpointer user_data)
{
  DIA_FOR_LAYER_IN_DIAGRAM (data, layer, i, {
    g_list_foreach (dia_layer_get_object_list (layer), func, user_data);
  });
}
