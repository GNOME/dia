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

#include "config.h"

#include <glib/gi18n-lib.h>

#include <string.h>

#include "diagram.h"
#include "object.h"
#include "group.h"
#include "object_ops.h"
#include "focus.h"
#include "message.h"
#include "menus.h"
#include "preferences.h"
#include "properties-dialog.h"
#include "cut_n_paste.h"
#include "layer-editor/layer_dialog.h"
#include "app_procs.h"
#include "dia_dirs.h"
#include "load_save.h"
#include "recent_files.h"
#include "dia-application.h"
#include "autosave.h"
#include "dynamic_refresh.h"
#include "textedit.h"
#include "lib/diamarshal.h"
#include "parent.h"
#include "diacontext.h"
#include "dia-layer.h"

typedef struct _DiagramPrivate DiagramPrivate;
struct _DiagramPrivate {
  GFile *file;
};

G_DEFINE_TYPE_WITH_PRIVATE (Diagram, dia_diagram, DIA_TYPE_DIAGRAM_DATA)

enum {
  PROP_0,
  PROP_FILE,
  LAST_PROP
};

static GParamSpec *pspecs[LAST_PROP] = { NULL, };


static GList *open_diagrams = NULL;

struct _ObjectExtent
{
  DiaObject *object;
  DiaRectangle extent;
};

typedef struct _ObjectExtent ObjectExtent;

static int diagram_parent_sort_cb(gconstpointer a, gconstpointer b);


enum {
  REMOVED,
  LAST_SIGNAL
};

static guint diagram_signals[LAST_SIGNAL] = { 0, };

static void
dia_diagram_set_property (GObject      *object,
                          guint         property_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
  Diagram *self = DIA_DIAGRAM (object);

  switch (property_id) {
    case PROP_FILE:
      dia_diagram_set_file (self, g_value_get_object (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}


static void
dia_diagram_get_property (GObject    *object,
                          guint       property_id,
                          GValue     *value,
                          GParamSpec *pspec)
{
  Diagram *self = DIA_DIAGRAM (object);

  switch (property_id) {
    case PROP_FILE:
      g_value_set_object (value, dia_diagram_get_file (self));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}


static void
dia_diagram_dispose (GObject *object)
{
  Diagram *dia = DIA_DIAGRAM (object);

  g_return_if_fail (dia->displays == NULL);

  if (g_list_index (open_diagrams, dia) >= 0) {
    open_diagrams = g_list_remove (open_diagrams, dia);
  }

  if (dia->undo)
    undo_destroy (dia->undo);
  dia->undo = NULL;

  diagram_cleanup_autosave (dia);

  G_OBJECT_CLASS (dia_diagram_parent_class)->dispose (object);
}


static void
dia_diagram_finalize (GObject *object)
{
  Diagram *dia = DIA_DIAGRAM (object);
  DiagramPrivate *priv = dia_diagram_get_instance_private (dia);

  g_return_if_fail (dia->displays == NULL);

  g_clear_object (&priv->file);
  g_clear_pointer (&dia->filename, g_free);

  G_OBJECT_CLASS (dia_diagram_parent_class)->finalize (object);
}


static void
_diagram_removed (Diagram* dia)
{
}


static void
dia_diagram_class_init (DiagramClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);


  object_class->set_property = dia_diagram_set_property;
  object_class->get_property = dia_diagram_get_property;
  object_class->dispose = dia_diagram_dispose;
  object_class->finalize = dia_diagram_finalize;

  /**
   * DiaDiagram:file:
   *
   * Since: 0.98
   */
  pspecs[PROP_FILE] =
    g_param_spec_object ("file",
                         "File",
                         "The file backing this diagram",
                         G_TYPE_FILE,
                         G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, LAST_PROP, pspecs);

  diagram_signals[REMOVED] =
    g_signal_new ("removed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (DiagramClass, removed),
                  NULL, NULL,
                  dia_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  klass->removed = _diagram_removed;
}


static void
_object_add (Diagram   *dia,
             DiaLayer  *layer,
             DiaObject *obj,
             gpointer   user_data)
{
  if (obj) {
    object_add_updates (obj, dia);
  }
}


static void
_object_remove (Diagram   *dia,
                DiaLayer  *layer,
                DiaObject *obj,
                gpointer   user_data)
{
  if (obj) {
    object_add_updates (obj, dia);
  }
}


static void
dia_diagram_init (Diagram *self)
{
  self->data = &self->parent_instance; /* compatibility */
                                       /* Sure, but it's also silly ZB */

  self->pagebreak_color = prefs.new_diagram.pagebreak_color;
  self->guide_color = prefs.new_diagram.guide_color;

  get_paper_info (&self->data->paper, -1, &prefs.new_diagram);

  self->grid.width_x = prefs.grid.x;
  self->grid.width_y = prefs.grid.y;
  self->grid.hex_size = prefs.grid.hex_size;
  self->grid.colour = prefs.new_diagram.grid_color;
  self->grid.hex = prefs.grid.hex;
  self->grid.visible_x = prefs.grid.vis_x;
  self->grid.visible_y = prefs.grid.vis_y;
  self->grid.dynamic = prefs.grid.dynamic;
  self->grid.major_lines = prefs.grid.major_lines;

  self->guides = NULL;

  self->filename = NULL;

  self->unsaved = TRUE;
  self->mollified = FALSE;
  self->autosavefilename = NULL;

  if (self->undo) {
    undo_destroy (self->undo);
  }
  self->undo = new_undo_stack (self);

  if (!g_list_find (open_diagrams, self)) {
    open_diagrams = g_list_prepend (open_diagrams, self);
  }

  g_signal_connect (G_OBJECT (self), "object_add", G_CALLBACK (_object_add), self);
  g_signal_connect (G_OBJECT (self), "object_remove", G_CALLBACK (_object_remove), self);
}


GList *
dia_open_diagrams (void)
{
  return open_diagrams;
}


int
diagram_load_into (Diagram         *diagram,
                   const char      *filename,
                   DiaImportFilter *ifilter)
{
  /* ToDo: move context further up in the callstack and to sth useful with it's content */
  DiaContext *ctx = dia_context_new (_("Load Into"));

  if (!ifilter) {
    ifilter = filter_guess_import_filter (filename);
  }

  /* slightly hacked to avoid 'Not a Dia File' for .shape */
  if (!ifilter && g_str_has_suffix (filename, ".shape")) {
    ifilter = filter_import_get_by_name ("dia-svg");
  }

  if (!ifilter) {
    /* default to native format */
    ifilter = &dia_import_filter;
  }

  dia_context_set_filename (ctx, filename);
  if (ifilter->import_func (filename, diagram->data, ctx, ifilter->user_data)) {
    GFile *file = NULL;

    if (ifilter != &dia_import_filter) {
      /* When loading non-Dia files, change filename to reflect that saving
       * will produce a Dia file. See bug #440093 */
      if (strcmp (diagram->filename, filename) == 0) {
        /* not a real load into but initial load */
        char *old_filename = g_strdup (diagram->filename);
        char *suffix_offset = g_utf8_strrchr (old_filename, -1, (gunichar) '.');
        char *new_filename;

        if (suffix_offset != NULL) {
          new_filename = g_strndup (old_filename, suffix_offset - old_filename);
          g_clear_pointer (&old_filename, g_free);
        } else {
          new_filename = old_filename;
        }
        old_filename = g_strconcat (new_filename, ".dia", NULL);
        g_clear_pointer (&new_filename, g_free);

        file = g_file_new_for_path (old_filename);
        dia_diagram_set_file (diagram, file);

        g_clear_pointer (&old_filename, g_free);

        diagram->unsaved = TRUE;

        diagram_modified (diagram);
      }
    } else {
      /* Valid existing Dia file opened - file already saved, set filename to diagram */
      diagram->unsaved = FALSE;

      file = g_file_new_for_path (filename);
      dia_diagram_set_file (diagram, file);
    }

    diagram_set_modified (diagram, TRUE);
    dia_context_release (ctx);

    g_clear_object (&file);

    return TRUE;
  } else {
    dia_context_release(ctx);
    return FALSE;
  }
}

Diagram *
diagram_load(const char *filename, DiaImportFilter *ifilter)
{
  Diagram *diagram = NULL;
  GList *diagrams;
  gboolean was_default = FALSE;

  for (diagrams = open_diagrams; diagrams != NULL; diagrams = g_list_next(diagrams)) {
    Diagram *old_diagram = (Diagram*)diagrams->data;
    if (old_diagram->is_default) {
      diagram = old_diagram;
      was_default = TRUE;
      break;
    }
  }

  /* TODO: Make diagram not be initialized twice */
  if (diagram == NULL) {
    GFile *file = g_file_new_for_path (filename);

    diagram = dia_diagram_new (file);

    g_clear_object (&file);
  }

  if (diagram == NULL) return NULL;

  if (!diagram_load_into (diagram, filename, ifilter)) {
    if (!was_default) /* don't kill the default diagram on import failure */
      diagram_destroy(diagram);
    diagram = NULL;
  } else {
    /* not modifying 'diagram->unsaved' the state depends on the import filter used */
    diagram_set_modified (diagram, FALSE);
    if (app_is_interactive ()) {
      recent_file_history_add (filename);
      if (was_default) {
        dia_application_diagram_remove (dia_application_get_default (),
                                        diagram);
        dia_application_diagram_add (dia_application_get_default (),
                                     diagram);
      }
    }
  }

  if (diagram != NULL && was_default && app_is_interactive()) {
    diagram->is_default = FALSE;

    if (g_slist_length(diagram->displays) == 1) {
      display_set_active (diagram->displays->data);
    }
  }

  return diagram;
}

/**
 * dia_diagram_new:
 * @file: the #GFile backing this diagram
 *
 * Create a new diagram with the given file.
 */
Diagram *
dia_diagram_new (GFile *file)
{
  Diagram *dia = g_object_new (DIA_TYPE_DIAGRAM,
                               "file", file,
                               NULL);

  return dia;
}

// TODO: This seems bad
void
diagram_destroy(Diagram *dia)
{
  g_signal_emit (dia, diagram_signals[REMOVED], 0);
  g_clear_object (&dia);
}


/**
 * diagram_is_modified:
 * @dia: the #Diagram
 *
 * Returns: %TRUE if we consider the diagram modified.
 *
 * Since: dawn-of-time
 */
gboolean
diagram_is_modified (Diagram *dia)
{
  return dia->mollified || !undo_is_saved (dia->undo);
}


/**
 * diagram_modified:
 * @dia: the #Diagram
 *
 * We might just have change the diagrams modified status.
 *
 * This doesn't set the status, but merely updates the display.
 *
 * Since: dawn-of-time
 */
void
diagram_modified (Diagram *dia)
{
  GSList *displays;
  char *dia_name = diagram_get_name (dia);
  char *extra = g_path_get_dirname (dia->filename);
  char *title = g_strdup_printf ("%s%s (%s)",
                                 diagram_is_modified (dia) ? "*" : "",
                                 dia_name,
                                 extra ? extra : " ");

  g_clear_pointer (&dia_name, g_free);
  g_clear_pointer (&extra, g_free);

  displays = dia->displays;
  while (displays!=NULL) {
    DDisplay *ddisp = (DDisplay *) displays->data;

    ddisplay_set_title (ddisp, title);

    displays = g_slist_next (displays);
  }

  if (diagram_is_modified (dia)) {
    dia->autosaved = FALSE;
    dia->is_default = FALSE;
  }

  /* diagram_set_modified(dia, TRUE); */
  g_clear_pointer (&title, g_free);
}


/**
 * diagram_set_modified:
 * @dia: the #Diagram
 * @modified: the new state
 *
 * Set this diagram explicitly modified.  This should not be called
 * by things that change the undo stack, as those modifications are
 * noticed through changes in the undo stack.
 *
 * Since: dawn-of-time
 */
void
diagram_set_modified (Diagram *dia, gboolean modified)
{
  if (dia->mollified != modified) {
    dia->mollified = modified;
  }
  diagram_modified (dia);
}

/* ************ Functions that check for menu sensitivity ********* */

/* Suggested optimization: The loops take a lot of the time.  Collapse them
 * into one, have them set flags for the things that have been found true.
 * Harder to maintain.
 */
static gboolean
diagram_selected_any_groups(Diagram *dia) {
  GList *selected;

  for (selected = dia->data->selected;
       selected != NULL; selected = selected->next) {
    DiaObject *obj = (DiaObject*)selected->data;
    if (IS_GROUP(obj)) return TRUE;
  }
  return FALSE;
}

static gboolean
diagram_selected_any_parents(Diagram *dia) {
  GList *selected;

  for (selected = dia->data->selected;
       selected != NULL; selected = selected->next) {
    DiaObject *obj = (DiaObject*)selected->data;
    if (object_flags_set(obj, DIA_OBJECT_CAN_PARENT) && obj->children != NULL)
      return TRUE;
  }
  return FALSE;
}

static gboolean
diagram_selected_any_children(Diagram *dia) {
  GList *selected;

  for (selected = dia->data->selected;
       selected != NULL; selected = selected->next) {
    DiaObject *obj = (DiaObject*)selected->data;
    if (obj->parent != NULL) return TRUE;
  }
  return FALSE;
}


/**
 * diagram_selected_can_parent:
 * @dia: the #Diagram
 *
 * This is slightly more complex -- must see if any non-parented objects
 * are within a parenting object.
 *
 * Since: dawn-of-time
 */
static gboolean
diagram_selected_can_parent(Diagram *dia) {
  GList *selected;
  GList *parents = NULL;

  for (selected = dia->data->selected;
       selected != NULL; selected = selected->next) {
    DiaObject *obj = (DiaObject*)selected->data;
    if (object_flags_set(obj, DIA_OBJECT_CAN_PARENT)) {
      parents = g_list_prepend(parents, obj);
    }
  }
  for (selected = dia->data->selected;
       selected != NULL; selected = selected->next) {
    DiaObject *obj = (DiaObject*)selected->data;
    if (obj->parent == NULL) {
      GList *parent_tmp;
      for (parent_tmp = parents; parent_tmp != NULL; parent_tmp = parent_tmp->next) {
	if (object_within_parent(obj, (DiaObject*)parent_tmp->data)) {
	  g_list_free(parents);
	  return TRUE;
	}
      }
    }
  }
  g_list_free(parents);
  return FALSE;
}

/** Returns TRUE if an object is fully enclosed by a another object, which
 * can be a parent */
gboolean
object_within_parent(DiaObject *obj, DiaObject *p)
{
  DiaRectangle obj_bb = obj->bounding_box;
  if (!object_flags_set(p, DIA_OBJECT_CAN_PARENT))
    return FALSE;
  if (p == obj)
    return FALSE;
  if (obj_bb.left > p->bounding_box.left &&
      obj_bb.right < p->bounding_box.right &&
      obj_bb.top > p->bounding_box.top &&
      obj_bb.bottom < p->bounding_box.bottom)
    return TRUE;
  return FALSE;
}

/*
  This is the real implementation of the sensitivity update.
  TODO: move it to the DDisplay as it belongs to it IMHO
 */
void
diagram_update_menu_sensitivity (Diagram *dia)
{
  int selected_count = dia ? g_list_length (dia->data->selected) : 0;
  DDisplay *ddisp = ddisplay_active();
  gboolean focus_active = dia ? (get_active_focus(dia->data) != NULL) : FALSE;
  gboolean textedit_active = ddisp ? textedit_mode(ddisp) : FALSE;
  GtkAction *action;

  /* Edit menu */
  if ((action = menus_get_action ("EditUndo")) != NULL)
    gtk_action_set_sensitive (action, dia ? undo_available(dia->undo, TRUE) : FALSE);
  if ((action = menus_get_action ("EditRedo")) != NULL)
    gtk_action_set_sensitive (action, dia ? undo_available(dia->undo, FALSE) : FALSE);
  if ((action = menus_get_action ("EditCopy")) != NULL)
    gtk_action_set_sensitive (action, textedit_active || selected_count > 0);
  if ((action = menus_get_action ("EditCut")) != NULL)
    gtk_action_set_sensitive (action, textedit_mode(ddisp) || selected_count > 0);
  if ((action = menus_get_action ("EditPaste")) != NULL)
    gtk_action_set_sensitive (action, textedit_active || cnp_exist_stored_objects());
  if ((action = menus_get_action ("EditDelete")) != NULL)
    gtk_action_set_sensitive (action, !textedit_active && selected_count > 0);
  if ((action = menus_get_action ("EditDuplicate")) != NULL)
    gtk_action_set_sensitive (action, !textedit_active && selected_count > 0);

  if ((action = menus_get_action ("EditCopytext")) != NULL)
    gtk_action_set_sensitive (action, focus_active);
  if ((action = menus_get_action ("EditCuttext")) != NULL)
    gtk_action_set_sensitive (action, focus_active);
  if ((action = menus_get_action ("EditPastetext")) != NULL)
    gtk_action_set_sensitive (action, focus_active);

  /* Objects menu */
  if ((action = menus_get_action ("ObjectsSendtoback")) != NULL)
    gtk_action_set_sensitive (action, !textedit_active && selected_count > 0);
  if ((action = menus_get_action ("ObjectsBringtofront")) != NULL)
    gtk_action_set_sensitive (action, !textedit_active && selected_count > 0);
  if ((action = menus_get_action ("ObjectsSendbackwards")) != NULL)
    gtk_action_set_sensitive (action, !textedit_active && selected_count > 0);
  if ((action = menus_get_action ("ObjectsBringforwards")) != NULL)
    gtk_action_set_sensitive (action, !textedit_active && selected_count > 0);

  if ((action = menus_get_action ("ObjectsLayerAbove")) != NULL)
    gtk_action_set_sensitive (action, !textedit_active && selected_count > 0);
  if ((action = menus_get_action ("ObjectsLayerBelow")) != NULL)
    gtk_action_set_sensitive (action, !textedit_active && selected_count > 0);

  if ((action = menus_get_action ("ObjectsGroup")) != NULL)
    gtk_action_set_sensitive (action, !textedit_active && selected_count > 1);
  if ((action = menus_get_action ("ObjectsUngroup")) != NULL)
    gtk_action_set_sensitive (action, !textedit_active && dia && diagram_selected_any_groups (dia));
  if ((action = menus_get_action ("ObjectsParent")) != NULL)
    gtk_action_set_sensitive (action, !textedit_active && dia && diagram_selected_can_parent (dia));
  if ((action = menus_get_action ("ObjectsUnparent")) != NULL)
    gtk_action_set_sensitive (action, !textedit_active && dia && diagram_selected_any_children (dia));
  if ((action = menus_get_action ("ObjectsUnparentchildren")) != NULL)
    gtk_action_set_sensitive (action, !textedit_active && dia && diagram_selected_any_parents (dia));

  if ((action = menus_get_action ("ObjectsProperties")) != NULL)
    gtk_action_set_sensitive (action, selected_count > 0);

  /* Objects->Align menu */
  if ((action = menus_get_action ("ObjectsAlignLeft")) != NULL)
    gtk_action_set_sensitive (action, !textedit_active && selected_count > 1);
  if ((action = menus_get_action ("ObjectsAlignCenter")) != NULL)
    gtk_action_set_sensitive (action, !textedit_active && selected_count > 1);
  if ((action = menus_get_action ("ObjectsAlignRight")) != NULL)
    gtk_action_set_sensitive (action, !textedit_active && selected_count > 1);
  if ((action = menus_get_action ("ObjectsAlignSpreadouthorizontally")) != NULL)
    gtk_action_set_sensitive (action, !textedit_active && selected_count > 1);
  if ((action = menus_get_action ("ObjectsAlignAdjacent")) != NULL)
    gtk_action_set_sensitive (action, !textedit_active && selected_count > 1);
  if ((action = menus_get_action ("ObjectsAlignTop")) != NULL)
    gtk_action_set_sensitive (action, !textedit_active && selected_count > 1);
  if ((action = menus_get_action ("ObjectsAlignMiddle")) != NULL)
    gtk_action_set_sensitive (action, !textedit_active && selected_count > 1);
  if ((action = menus_get_action ("ObjectsAlignBottom")) != NULL)
    gtk_action_set_sensitive (action, !textedit_active && selected_count > 1);
  if ((action = menus_get_action ("ObjectsAlignSpreadoutvertically")) != NULL)
    gtk_action_set_sensitive (action, !textedit_active && selected_count > 1);
  if ((action = menus_get_action ("ObjectsAlignStacked")) != NULL)
    gtk_action_set_sensitive (action, !textedit_active && selected_count > 1);
  if ((action = menus_get_action ("ObjectsAlignConnected")) != NULL)
    gtk_action_set_sensitive (action, !textedit_active && selected_count > 1);

  /* Select menu */
  if ((action = menus_get_action ("SelectAll")) != NULL)
    gtk_action_set_sensitive (action, !textedit_active);
  if ((action = menus_get_action ("SelectNone")) != NULL)
    gtk_action_set_sensitive (action, !textedit_active);
  if ((action = menus_get_action ("SelectInvert")) != NULL)
    gtk_action_set_sensitive (action, !textedit_active);
  if ((action = menus_get_action ("SelectTransitive")) != NULL)
    gtk_action_set_sensitive (action, !textedit_active);
  if ((action = menus_get_action ("SelectConnected")) != NULL)
    gtk_action_set_sensitive (action, !textedit_active);
  if ((action = menus_get_action ("SelectSametype")) != NULL)
    gtk_action_set_sensitive (action, !textedit_active);

  if ((action = menus_get_action ("SelectReplace")) != NULL)
    gtk_action_set_sensitive (action, !textedit_active);
  if ((action = menus_get_action ("SelectUnion")) != NULL)
    gtk_action_set_sensitive (action, !textedit_active);
  if ((action = menus_get_action ("SelectIntersection")) != NULL)
    gtk_action_set_sensitive (action, !textedit_active);
  if ((action = menus_get_action ("SelectRemove")) != NULL)
    gtk_action_set_sensitive (action, !textedit_active);
  if ((action = menus_get_action ("SelectInverse")) != NULL)
    gtk_action_set_sensitive (action, !textedit_active);

  /* Tools menu - toolbox actions */
  gtk_action_group_set_sensitive (menus_get_tool_actions (),  !textedit_active);

  /* View menu - should not need disabling yet */
}


void
diagram_add_ddisplay (Diagram *dia, DDisplay *ddisp)
{
  if (dia->displays == NULL) {
    // If the diagram wasn't already open in the UI
    dia_application_diagram_add (dia_application_get_default (),
                                ddisp->diagram);
  }

  dia->displays = g_slist_prepend (dia->displays, ddisp);
}

void
diagram_remove_ddisplay (Diagram *dia, DDisplay *ddisp)
{
  dia->displays = g_slist_remove(dia->displays, ddisp);

  if (g_slist_length (dia->displays) == 0) {
    // Remove the diagram from the UI
    dia_application_diagram_remove (dia_application_get_default (),
                                    ddisp->diagram);

    diagram_destroy (dia);
  }
}


void
diagram_add_object (Diagram *dia, DiaObject *obj)
{
  dia_layer_add_object (dia_diagram_data_get_active_layer (DIA_DIAGRAM_DATA (dia)),
                        obj);

  diagram_modified (dia);
}


void
diagram_add_object_list (Diagram *dia, GList *list)
{
  dia_layer_add_objects (dia_diagram_data_get_active_layer (DIA_DIAGRAM_DATA (dia)),
                         list);

  diagram_modified (dia);
}


void
diagram_selected_break_external (Diagram *dia)
{
  GList *list;
  GList *connected_list;
  DiaObject *obj;
  DiaObject *other_obj;
  int i, j;

  list = dia->data->selected;
  while (list != NULL) {
    obj = (DiaObject *) list->data;

    /* Break connections between this object and objects not selected: */
    for (i = 0; i < obj->num_handles; i++) {
      ConnectionPoint *con_point;
      con_point = obj->handles[i]->connected_to;

      if (con_point == NULL)
        continue; /* Not connected */

      other_obj = con_point->object;
      if (g_list_find (dia->data->selected, other_obj) == NULL) {
        /* other_obj is not selected, break connection */
        DiaChange *change = dia_unconnect_change_new (dia, obj, obj->handles[i]);
        dia_change_apply (change, DIA_DIAGRAM_DATA (dia));
        object_add_updates (obj, dia);
      }
    }

    /* Break connections from non selected objects to this object: */
    for (i = 0; i < obj->num_connections; i++) {
      connected_list = obj->connections[i]->connected;

      while (connected_list != NULL) {
        other_obj = (DiaObject *) connected_list->data;

        if (g_list_find (dia->data->selected, other_obj) == NULL) {
          /* other_obj is not in list, break all connections
             to obj from other_obj */

          for (j = 0; j < other_obj->num_handles; j++) {
            ConnectionPoint *con_point;
            con_point = other_obj->handles[j]->connected_to;

            if (con_point && (con_point->object == obj)) {
              DiaChange *change;
              connected_list = g_list_previous (connected_list);
              change = dia_unconnect_change_new (dia, other_obj, other_obj->handles[j]);
              dia_change_apply (change, DIA_DIAGRAM_DATA (dia));
              if (connected_list == NULL)
                connected_list = obj->connections[i]->connected;
            }
          }
        }
        connected_list = g_list_next (connected_list);
      }
    }

    list = g_list_next (list);
  }
}

void
diagram_remove_all_selected (Diagram *diagram, int delete_empty)
{
  object_add_updates_list (diagram->data->selected, diagram);
  textedit_remove_focus_all (diagram);
  data_remove_all_selected (diagram->data);
}

void
diagram_unselect_object (Diagram *diagram, DiaObject *obj)
{
  object_add_updates (obj, diagram);
  textedit_remove_focus (obj, diagram);
  data_unselect (DIA_DIAGRAM_DATA (diagram), obj);
}

void
diagram_unselect_objects(Diagram *dia, GList *obj_list)
{
  GList *list;
  DiaObject *obj;

  /* otherwise we would signal objects step by step */
  g_signal_handlers_block_by_func (dia, DIA_DIAGRAM_DATA_GET_CLASS (dia)->selection_changed, NULL);
  list = obj_list;
  while (list != NULL) {
    obj = (DiaObject *) list->data;

    if (g_list_find(dia->data->selected, obj) != NULL){
      diagram_unselect_object(dia, obj);
    }

    list = g_list_next(list);
  }
  g_signal_handlers_unblock_by_func (dia, DIA_DIAGRAM_DATA_GET_CLASS (dia)->selection_changed, NULL);
  g_signal_emit_by_name (dia, "selection_changed", g_list_length (dia->data->selected));
}


/**
 * diagram_select:
 * @diagram: the #Diagram that the object belongs to (sorta redundant now)
 * @obj: the object that should be made selected.
 *
 * Make a single object selected.
 * Note that an object inside a closed group cannot be made selected, nor
 * can an object in a non-active layer.
 *
 * Since: dawn-of-time
 */
void
diagram_select (Diagram *diagram, DiaObject *obj)
{
  if (dia_object_is_selectable(obj)) {
    data_select(diagram->data, obj);
    obj->ops->selectf(obj, NULL, NULL);
    object_add_updates(obj, diagram);
  }
}

void
diagram_select_list (Diagram *dia, GList *list)
{
  g_return_if_fail (dia && list);
  /* otherwise we would signal objects step by step */
  g_signal_handlers_block_by_func (dia, DIA_DIAGRAM_DATA_GET_CLASS (dia)->selection_changed, NULL);
  while (list != NULL) {
    DiaObject *obj = (DiaObject *)list->data;

    diagram_select (dia, obj);

    list = g_list_next (list);
  }
  if (get_active_focus ((DiagramData*) dia) == NULL) {
    textedit_activate_first (ddisplay_active ());
  }
  g_signal_handlers_unblock_by_func (dia, DIA_DIAGRAM_DATA_GET_CLASS (dia)->selection_changed, NULL);
  g_signal_emit_by_name (dia, "selection_changed", g_list_length (dia->data->selected));
}


int
diagram_is_selected (Diagram *diagram, DiaObject *obj)
{
  return g_list_find (diagram->data->selected, obj) != NULL;
}


void
diagram_redraw_all (void)
{
  GList *list;
  Diagram *dia;

  list = open_diagrams;

  while (list != NULL) {
    dia = (Diagram *) list->data;

    diagram_add_update_all (dia);
    diagram_flush (dia);

    list = g_list_next (list);
  }

  return;
}


void
diagram_add_update_all (Diagram *dia)
{
  GSList *l;
  DDisplay *ddisp;

  l = dia->displays;
  while (l!=NULL) {
    ddisp = (DDisplay *) l->data;

    ddisplay_add_update_all (ddisp);

    l = g_slist_next (l);
  }
}


void
diagram_add_update (Diagram *dia, const DiaRectangle *update)
{
  GSList *l;
  DDisplay *ddisp;

  l = dia->displays;
  while (l!=NULL) {
    ddisp = (DDisplay *) l->data;

    ddisplay_add_update(ddisp, update);

    l = g_slist_next(l);
  }
}

/**
 * diagram_add_update_with_border:
 * @dia: the #Diagram
 * @update: the area to update
 * @pixel_border: border around @update:
 *
 * Add an update of the given rectangle, but with an additional
 * border around it.  The pixels are added after the rectangle has
 * been converted to pixel coords.
 * Currently used for leaving room for highlighting.
 * */
void
diagram_add_update_with_border (Diagram            *dia,
                                const DiaRectangle *update,
                                int                 pixel_border)
{
  GSList *l;
  DDisplay *ddisp;

  l = dia->displays;
  while (l!=NULL) {
    ddisp = (DDisplay *) l->data;

    ddisplay_add_update_with_border(ddisp, update, pixel_border);

    l = g_slist_next(l);
  }
}

void
diagram_add_update_pixels(Diagram *dia, Point *point,
			  int pixel_width, int pixel_height)
{
  GSList *l;
  DDisplay *ddisp;

  l = dia->displays;
  while (l!=NULL) {
    ddisp = (DDisplay *) l->data;

    ddisplay_add_update_pixels(ddisp, point, pixel_width, pixel_height);

    l = g_slist_next(l);
  }
}

void
diagram_flush(Diagram *dia)
{
  GSList *l;
  DDisplay *ddisp;
  l = dia->displays;
  while (l!=NULL) {
    ddisp = (DDisplay *) l->data;

    ddisplay_flush(ddisp);

    l = g_slist_next(l);
  }
  dynobj_refresh_kick();
}

DiaObject *
diagram_find_clicked_object (Diagram *dia,
                             Point   *pos,
                             real     maxdist)
{
  return dia_layer_find_closest_object_except (dia_diagram_data_get_active_layer (DIA_DIAGRAM_DATA (dia)),
                                               pos, maxdist, NULL);
}

DiaObject *
diagram_find_clicked_object_except (Diagram *dia,
                                    Point   *pos,
                                    real     maxdist,
                                    GList   *avoid)
{
  return dia_layer_find_closest_object_except (dia_diagram_data_get_active_layer (DIA_DIAGRAM_DATA (dia)),
                                               pos,
                                               maxdist,
                                               avoid);
}

/*
 * Always returns the last handle in an object that has
 * the closest distance
 */
real
diagram_find_closest_handle(Diagram *dia, Handle **closest,
			    DiaObject **object, Point *pos)
{
  GList *l;
  DiaObject *obj;
  Handle *handle;
  real mindist, dist;
  int i;

  mindist = 1000000.0; /* Really big value... */

  *closest = NULL;

  l = dia->data->selected;
  while (l!=NULL) {
    obj = (DiaObject *) l->data;

    for (i=0;i<obj->num_handles;i++) {
      handle = obj->handles[i];
      /* Note: Uses manhattan metric for speed... */
      dist = distance_point_point_manhattan(pos, &handle->pos);
      if (dist<=mindist) {
	mindist = dist;
	*closest = handle;
	*object = obj;
      }
    }

    l = g_list_next(l);
  }

  return mindist;
}

real
diagram_find_closest_connectionpoint (Diagram          *dia,
                                      ConnectionPoint **closest,
                                      Point            *pos,
                                      DiaObject        *notthis)
{
  real dist = 100000000.0;

  DIA_FOR_LAYER_IN_DIAGRAM (DIA_DIAGRAM_DATA (dia), layer, i, {
    ConnectionPoint *this_cp;
    real this_dist;
    if (dia_layer_is_connectable (layer)) {
      this_dist = dia_layer_find_closest_connectionpoint (layer,
                                                          &this_cp,
                                                          pos,
                                                          notthis);
      if (this_dist < dist) {
        dist = this_dist;
        *closest = this_cp;
      }
    }
  });

  return dist;
}

void
diagram_update_extents(Diagram *dia)
{
  gfloat cur_scale = dia->data->paper.scaling;
  /* anropar update_scrollbars() */

  if (data_update_extents(dia->data)) {
    /* Update scrollbars because extents were changed: */
    GSList *l;
    DDisplay *ddisp;

    l = dia->displays;
    while (l!=NULL) {
      ddisp = (DDisplay *) l->data;

      ddisplay_update_scrollbars(ddisp);

      l = g_slist_next(l);
    }
    if (cur_scale != dia->data->paper.scaling) {
      diagram_add_update_all(dia);
      diagram_flush(dia);
    }
  }
}

/* Remove connections from obj to objects outside created group. */
static void
strip_connections(DiaObject *obj, GList *not_strip_list, Diagram *dia)
{
  int i;
  Handle *handle;
  DiaChange *change;

  for (i=0;i<obj->num_handles;i++) {
    handle = obj->handles[i];
    if ((handle->connected_to != NULL) &&
        (g_list_find(not_strip_list, handle->connected_to->object)==NULL)) {
      change = dia_unconnect_change_new (dia, obj, handle);
      dia_change_apply (change, DIA_DIAGRAM_DATA (dia));
    }
  }
}


/* GCompareFunc */
static int
diagram_parent_sort_cb (gconstpointer _a, gconstpointer _b)
{
  ObjectExtent **a = (ObjectExtent **)_a;
  ObjectExtent **b = (ObjectExtent **)_b;

  if ((*a)->extent.left < (*b)->extent.left)
    return 1;
  else if ((*a)->extent.left > (*b)->extent.left)
    return -1;
  else
    if ((*a)->extent.top < (*b)->extent.top)
      return 1;
    else if ((*a)->extent.top > (*b)->extent.top)
      return -1;
    else
      return 0;
}


/* needs faster algorithm -- if we find that parenting is slow.
 * If it works, don't optimize it until it's a hotspot. */
void diagram_parent_selected(Diagram *dia)
{
  GList *list = dia->data->selected;
  int length = g_list_length(list);
  int idx, idx2;
  ObjectExtent *oe;
  gboolean any_parented = FALSE;
  GPtrArray *extents = g_ptr_array_sized_new(length);
  while (list)
  {
    oe = g_new(ObjectExtent, 1);
    oe->object = list->data;
    parent_handle_extents(list->data, &oe->extent);
    g_ptr_array_add(extents, oe);
    list = g_list_next(list);
  }
  /* sort all the objects by their left position */
  g_ptr_array_sort(extents, diagram_parent_sort_cb);

  for (idx = 0; idx < length; idx++)
  {
    ObjectExtent *oe1 = g_ptr_array_index(extents, idx);
    if (oe1->object->parent)
      continue;

    for (idx2 = idx + 1; idx2 < length; idx2++)
    {
      ObjectExtent *oe2 = g_ptr_array_index(extents, idx2);
      if (!object_flags_set(oe2->object, DIA_OBJECT_CAN_PARENT))
        continue;

      if (oe1->extent.right <= oe2->extent.right
        && oe1->extent.bottom <= oe2->extent.bottom)
      {
        DiaChange *change;
        change = dia_parenting_change_new (dia, oe2->object, oe1->object, TRUE);
        dia_change_apply (change, DIA_DIAGRAM_DATA (dia));
        any_parented = TRUE;
        /*
              oe1->object->parent = oe2->object;
        oe2->object->children = g_list_append(oe2->object->children, oe1->object);
        */
        break;
      }
    }
  }
  g_ptr_array_free(extents, TRUE);
  if (any_parented) {
    diagram_modified(dia);
    diagram_flush(dia);
    undo_set_transactionpoint(dia->undo);
  }
}

/** Remove all selected objects from their parents (if any). */
void diagram_unparent_selected(Diagram *dia)
{
  GList *list;
  DiaObject *obj, *parent;
  DiaChange *change;
  gboolean any_unparented = FALSE;

  for (list = dia->data->selected; list != NULL; list = g_list_next(list))
  {
    obj = (DiaObject *) list->data;
    parent = obj->parent;

    if (!parent)
      continue;

    change = dia_parenting_change_new (dia, parent, obj, FALSE);
    dia_change_apply (change, DIA_DIAGRAM_DATA (dia));
    any_unparented = TRUE;
    /*
    parent->children = g_list_remove(parent->children, obj);
    obj->parent = NULL;
    */
  }
  if (any_unparented) {
    diagram_modified(dia);
    diagram_flush(dia);
    undo_set_transactionpoint(dia->undo);
  }
}

/** Remove all children from the selected parents. */
void diagram_unparent_children_selected(Diagram *dia)
{
  GList *list;
  DiaObject *obj, *child;
  gboolean any_unparented = FALSE;
  for (list = dia->data->selected; list != NULL; list = g_list_next(list))
  {
    obj = (DiaObject *) list->data;
    if (!object_flags_set(obj, DIA_OBJECT_CAN_PARENT) || !obj->children)
      continue;

    any_unparented = TRUE;
    /* Yes, this creates a whole bunch of Changes.  They're lightweight
     * structures, though, and it's easier to assure correctness this
     * way.  If needed, we can make a parent undo with a list of children.
     */
    while (obj->children != NULL) {
      DiaChange *change;
      child = (DiaObject *) obj->children->data;
      change = dia_parenting_change_new (dia, obj, child, FALSE);
      /* This will remove one item from the list, so the while terminates. */
      dia_change_apply (change, DIA_DIAGRAM_DATA (dia));
    }
    if (obj->children != NULL)
      printf("Obj still has %d children\n",
	     g_list_length(obj->children));
  }
  if (any_unparented) {
    diagram_modified(dia);
    diagram_flush(dia);
    undo_set_transactionpoint(dia->undo);
  }
}

void
diagram_group_selected (Diagram *dia)
{
  GList *list;
  GList *group_list;
  DiaObject *group;
  DiaObject *obj;
  GList *orig_list;
  DiaChange *change;

  if (g_list_length (dia->data->selected) < 1) {
    message_error (_("Trying to group with no selected objects."));
    return;
  }

#if 0
  /* the following is wrong as it screws up the selected list, see bug #153525
     * I just don't get what was originally intended so please speak up if you know  --hb
     */
  dia->data->selected = parent_list_affected(dia->data->selected);
#endif

  orig_list = g_list_copy (dia_layer_get_object_list (dia_diagram_data_get_active_layer (DIA_DIAGRAM_DATA (dia))));

  /* We have to rebuild the selection list so that it is the same
     order as in the Diagram list. */
  group_list = diagram_get_sorted_selected_remove (dia);

  list = group_list;
  while (list != NULL) {
    obj = DIA_OBJECT (list->data);

    /* Remove connections from obj to objects outside created group. */
    /* strip_connections sets up its own undo info. */
    /* The connections aren't reattached by ungroup. */
    strip_connections (obj, dia->data->selected, dia);

    list = g_list_next (list);
  }

  /* Remove list of selected objects */
  textedit_remove_focus_all (dia);
  data_remove_all_selected (dia->data);

  group = group_create (group_list);
  change = dia_group_objects_change_new (dia, group_list, group, orig_list);
  dia_change_apply (change, DIA_DIAGRAM_DATA (dia));

  /* Select the created group */
  diagram_select (dia, group);

  diagram_modified (dia);
  diagram_flush (dia);

  undo_set_transactionpoint (dia->undo);
}

void diagram_ungroup_selected(Diagram *dia)
{
  DiaObject *group;
  GList *group_list;
  GList *selected, *selection_copy;
  int group_index;
  int any_groups = 0;

  if (g_list_length(dia->data->selected) < 1) {
    message_error("Trying to ungroup with no selected objects.");
    return;
  }

  selection_copy = g_list_copy(dia->data->selected);
  selected = selection_copy;
  while (selected != NULL) {
    group = (DiaObject *)selected->data;

    if (IS_GROUP(group)) {
      DiaChange *change;

      /* Fix selection */
      diagram_unselect_object(dia, group);

      group_list = group_objects(group);

      group_index = dia_layer_object_get_index (dia_diagram_data_get_active_layer (DIA_DIAGRAM_DATA (dia)), group);

      change = dia_ungroup_objects_change_new (dia, group_list, group, group_index);
      dia_change_apply (change, DIA_DIAGRAM_DATA (dia));

      diagram_select_list(dia, group_list);
      any_groups = 1;
    }
    selected = g_list_next(selected);
  }
  g_list_free(selection_copy);

  if (any_groups) {
    diagram_modified(dia);
    diagram_flush(dia);
    undo_set_transactionpoint(dia->undo);
  }
}

GList *
diagram_get_sorted_selected(Diagram *dia)
{
  return data_get_sorted_selected(dia->data);
}


/**
 * diagram_get_sorted_selected_remove:
 * @dia: the #Diagram
 *
 * Remove the currently selected objects from the diagram's object list.
 *
 * Returns: a newly created list of the selected objects, in order.
 *
 * Since: dawn-of-time
 */
GList *
diagram_get_sorted_selected_remove (Diagram *dia)
{
  diagram_modified(dia);

  return data_get_sorted_selected_remove(dia->data);
}

void
diagram_place_under_selected (Diagram *dia)
{
  GList *sorted_list;
  GList *orig_list;

  if (g_list_length (dia->data->selected) == 0)
    return;

  orig_list = g_list_copy (dia_layer_get_object_list (dia_diagram_data_get_active_layer (DIA_DIAGRAM_DATA (dia))));

  sorted_list = diagram_get_sorted_selected_remove (dia);
  object_add_updates_list (sorted_list, dia);
  dia_layer_add_objects_first (dia_diagram_data_get_active_layer (DIA_DIAGRAM_DATA (dia)), sorted_list);

  dia_reorder_objects_change_new (dia, g_list_copy (sorted_list), orig_list);

  diagram_modified (dia);
  diagram_flush (dia);
  undo_set_transactionpoint (dia->undo);
}

void
diagram_place_over_selected(Diagram *dia)
{
  GList *sorted_list;
  GList *orig_list;

  if (g_list_length (dia->data->selected) == 0)
    return;

  orig_list = g_list_copy (dia_layer_get_object_list (dia_diagram_data_get_active_layer (DIA_DIAGRAM_DATA (dia))));

  sorted_list = diagram_get_sorted_selected_remove (dia);
  object_add_updates_list (sorted_list, dia);
  dia_layer_add_objects (dia_diagram_data_get_active_layer (DIA_DIAGRAM_DATA (dia)), sorted_list);

  dia_reorder_objects_change_new (dia, g_list_copy (sorted_list), orig_list);

  diagram_modified(dia);
  diagram_flush(dia);
  undo_set_transactionpoint(dia->undo);
}

void
diagram_place_up_selected(Diagram *dia)
{
  GList *sorted_list;
  GList *orig_list;
  GList *tmp, *stmp;
  GList *new_list = NULL;

  if (g_list_length (dia->data->selected) == 0)
    return;

  orig_list = g_list_copy (dia_layer_get_object_list (dia_diagram_data_get_active_layer (DIA_DIAGRAM_DATA (dia))));

  sorted_list = diagram_get_sorted_selected(dia);
  object_add_updates_list(orig_list, dia);

  new_list = g_list_copy(orig_list);
  stmp = g_list_last(sorted_list);

  for (tmp = g_list_last(new_list);
       tmp != NULL;
       tmp = g_list_previous(tmp)) {
    if (stmp == NULL) break;
    if (tmp->prev == NULL) break;
    if (tmp->data == stmp->data) {
      stmp = g_list_previous(stmp);
    } else if (tmp->prev->data == stmp->data) {
      void *swap = tmp->data;
      tmp->data = tmp->prev->data;
      tmp->prev->data = swap;
      stmp = g_list_previous(stmp);
    }
  }

  dia_layer_set_object_list (dia_diagram_data_get_active_layer (DIA_DIAGRAM_DATA (dia)), new_list);

  dia_reorder_objects_change_new (dia, g_list_copy (sorted_list), orig_list);

  diagram_modified(dia);
  diagram_flush(dia);
  undo_set_transactionpoint(dia->undo);
}

void
diagram_place_down_selected (Diagram *dia)
{
  GList *sorted_list;
  GList *orig_list;
  GList *tmp, *stmp;
  GList *new_list = NULL;

  if (g_list_length (dia->data->selected) == 0)
    return;

  orig_list = g_list_copy (dia_layer_get_object_list (dia_diagram_data_get_active_layer (DIA_DIAGRAM_DATA (dia))));

  sorted_list = diagram_get_sorted_selected (dia);
  object_add_updates_list (orig_list, dia);

  /* Sanity check */
  g_assert(g_list_length (dia->data->selected) == g_list_length(sorted_list));

  new_list = g_list_copy(orig_list);
  stmp = sorted_list;

  for (tmp = new_list; tmp != NULL; tmp = g_list_next(tmp)) {
    if (stmp == NULL) break;
    if (tmp->next == NULL) break;
    if (tmp->data == stmp->data) {
      /* This just takes care of any starting matches */
      stmp = g_list_next(stmp);
    } else if (tmp->next->data == stmp->data) {
      /* This flips the non-selected element forwards, ala bubblesort */
      void *swap = tmp->data;
      tmp->data = tmp->next->data;
      tmp->next->data = swap;
      stmp = g_list_next(stmp);
    }
  }

  dia_layer_set_object_list (dia_diagram_data_get_active_layer (DIA_DIAGRAM_DATA (dia)), new_list);

  dia_reorder_objects_change_new (dia, g_list_copy (sorted_list), orig_list);

  diagram_modified(dia);
  diagram_flush(dia);
  undo_set_transactionpoint(dia->undo);
}


/**
 * diagram_get_name:
 * @dia: the #Diagram
 *
 * Get the 'sensible' (human-readable) name for the
 * diagram.
 *
 * This name may or may not be the same as the filename.
 *
 * Returns: A newly allocated string
 *
 * Since: dawn-of-time
 */
char *
diagram_get_name (Diagram *dia)
{
  return g_path_get_basename (dia->filename);
}


int
diagram_modified_exists (void)
{
  GList *list;
  Diagram *dia;

  list = open_diagrams;

  while (list != NULL) {
    dia = (Diagram *) list->data;

    if (diagram_is_modified(dia))
      return TRUE;

    list = g_list_next(list);
  }
  return FALSE;
}

void
diagram_object_modified(Diagram *dia, DiaObject *object)
{
  /* signal about the change */
  dia_application_diagram_change (dia_application_get_default (),
                                  dia,
                                  DIAGRAM_CHANGE_OBJECT,
                                  object);
}

void
dia_diagram_set_file (Diagram *self,
                      GFile   *file)
{
  DiagramPrivate *priv;
  GSList *l;
  DDisplay *ddisp;
  char *title;

  g_return_if_fail (DIA_IS_DIAGRAM (self));
  g_return_if_fail (file && G_IS_FILE (file));

  priv = dia_diagram_get_instance_private (self);

  if (priv->file == file) {
    return;
  }

  g_clear_object (&priv->file);
  priv->file = g_object_ref (file);

  g_clear_pointer (&self->filename, g_free);
  self->filename = g_file_get_path (file);

  title = diagram_get_name (self);

  l = self->displays;
  while (l != NULL) {
    ddisp = (DDisplay *) l->data;

    ddisplay_set_title (ddisp, title);

    l = g_slist_next (l);
  }

  g_clear_pointer (&title, g_free);

  /* signal about the change */
  dia_application_diagram_change (dia_application_get_default (),
                                  self,
                                  DIAGRAM_CHANGE_NAME,
                                  NULL);

  g_object_notify_by_pspec (G_OBJECT (self), pspecs[PROP_FILE]);
}


GFile *
dia_diagram_get_file (Diagram *self)
{
  DiagramPrivate *priv;

  priv = dia_diagram_get_instance_private (self);

  return priv->file;
}


/**
 * dia_diagram_add_guide:
 * @dia: the #Diagram
 * @position: when (relative to origin) to place the #DiaGuide
 * @orientation: which axis the guide is for
 * @push_undo: Update the undo stack if %TRUE
 *
 * Add a guide to the diagram at the given position and orientation.
 *
 * Returns: the new #DiaGuide
 *
 * Since: 0.98
 */
DiaGuide *
dia_diagram_add_guide (Diagram        *dia,
                       real            position,
                       GtkOrientation  orientation,
                       gboolean        push_undo)
{
  DiaGuide *guide = dia_guide_new (orientation, position);

  dia->guides = g_list_append (dia->guides, guide);

  if (push_undo) {
    dia_add_guide_change_new (dia, guide, TRUE);   /* Update undo stack. */
    undo_set_transactionpoint (dia->undo);
  }

  diagram_add_update_all (dia);
  diagram_modified (dia);
  diagram_flush (dia);

  return guide;
}


/**
 * dia_diagram_pick_guide:
 * @dia: #the #Diagram
 * @x: horizontal position
 * @y: vertical position
 * @epsilon_x: margin of error for @x
 * @epsilon_y: margin of error for @y
 *
 * Rick a guide within (@epsilon_x, @epsilon_y) distance of (@x, @y).
 *
 * Returns: %NULL if no such guide exists.
 *
 * Since: 0.98
 */
DiaGuide *
dia_diagram_pick_guide (Diagram *dia,
                        gdouble  x,
                        gdouble  y,
                        gdouble  epsilon_x,
                        gdouble  epsilon_y)
{
  GList *list;
  DiaGuide *ret = NULL;
  gdouble mindist = G_MAXDOUBLE;

  g_return_val_if_fail (epsilon_x > 0 && epsilon_y > 0, NULL);

  for (list = dia->guides;
       list;
       list = g_list_next (list)) {
    DiaGuide *guide = list->data;
    real position = guide->position;
    gdouble dist;

    switch (guide->orientation) {
      case GTK_ORIENTATION_HORIZONTAL:
        dist = ABS (position - y);
        if (dist < MIN (epsilon_y, mindist)) {
          mindist = dist;
          ret = guide;
        }
        break;

      case GTK_ORIENTATION_VERTICAL:
        dist = ABS (position - x);
        if (dist < MIN (epsilon_x, mindist / epsilon_y * epsilon_x)) {
          mindist = dist * epsilon_y / epsilon_x;
          ret = guide;
        }
        break;

      default:
        continue;
    }
  }

  return ret;
}


/**
 * dia_diagram_pick_guide_h:
 * @dia: #the #Diagram
 * @x: horizontal position
 * @y: vertical position
 * @epsilon_x: margin of error for @x
 * @epsilon_y: margin of error for @y
 *
 * Rick a %GTK_ORIENTATION_HORIZONTAL guide
 * within (@epsilon_x, @epsilon_y) distance of (@x, @y).
 *
 * Returns: %NULL if no such guide exists.
 *
 * Since: 0.98
 */
DiaGuide *
dia_diagram_pick_guide_h (Diagram *dia,
                          gdouble  x,
                          gdouble  y,
                          gdouble  epsilon_x,
                          gdouble  epsilon_y)
{
  GList *list;
  DiaGuide *ret = NULL;
  gdouble mindist = G_MAXDOUBLE;

  g_return_val_if_fail (epsilon_x > 0 && epsilon_y > 0, NULL);

  for (list = dia->guides;
       list;
       list = g_list_next (list)) {
    DiaGuide *guide = list->data;
    real position = guide->position;
    gdouble dist;

    switch (guide->orientation) {
      case GTK_ORIENTATION_HORIZONTAL:
        dist = ABS (position - y);
        if (dist < MIN (epsilon_y, mindist)) {
          mindist = dist;
          ret = guide;
        }
        break;

      case GTK_ORIENTATION_VERTICAL:
      default:
        continue;
      }
  }

  return ret;
}


/**
 * dia_diagram_pick_guide_v:
 * @dia: #the #Diagram
 * @x: horizontal position
 * @y: vertical position
 * @epsilon_x: margin of error for @x
 * @epsilon_y: margin of error for @y
 *
 * Rick a %GTK_ORIENTATION_VERTICAL guide
 * within (@epsilon_x, @epsilon_y) distance of (@x, @y).
 *
 * Returns: %NULL if no such guide exists.
 *
 * Since: 0.98
 */
DiaGuide *
dia_diagram_pick_guide_v (Diagram *dia,
                          gdouble  x,
                          gdouble  y,
                          gdouble  epsilon_x,
                          gdouble  epsilon_y)
{
  GList *list;
  DiaGuide *ret = NULL;
  gdouble mindist = G_MAXDOUBLE;

  g_return_val_if_fail (epsilon_x > 0 && epsilon_y > 0, NULL);

  for (list = dia->guides;
       list;
       list = g_list_next (list)) {
    DiaGuide *guide = list->data;
    real position = guide->position;
    gdouble dist;

    switch (guide->orientation) {
      case GTK_ORIENTATION_VERTICAL:
        dist = ABS (position - x);
        if (dist < MIN (epsilon_x, mindist / epsilon_y * epsilon_x)) {
          mindist = dist * epsilon_y / epsilon_x;
          ret = guide;
        }
        break;

      case GTK_ORIENTATION_HORIZONTAL:
      default:
        continue;
    }
  }

  return ret;
}


/**
 * dia_diagram_remove_guide:
 * @dia: the #Diagram
 * @guide: the #DiaGuide
 * @push_undo: Update the undo stack if %TRUE
 *
 * Remove the given guide from the diagram.
 *
 * Since: 0.98
 */
void
dia_diagram_remove_guide (Diagram *dia, DiaGuide *guide, gboolean push_undo)
{
  if (push_undo) {
    dia_delete_guide_change_new (dia, guide, TRUE);   /* Update undo stack. */
  }

  dia->guides = g_list_remove (dia->guides, guide);
}


/**
 * dia_diagram_remove_all_guides:
 * @dia: the #Diagram
 *
 * Remove all guides from the diagram.
 *
 * Updates undo stack.
 */
void
dia_diagram_remove_all_guides (Diagram *dia)
{
  GList *list;

  for (list = g_list_copy (dia->guides); list; list = g_list_next (list)) {
    dia_diagram_remove_guide (dia, list->data, TRUE);
  }

  undo_set_transactionpoint (dia->undo);
}
