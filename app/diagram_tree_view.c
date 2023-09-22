/* Dia -- a diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * diagram_tree.c : a tree showing open diagrams
 * Copyright (C) 2001 Jose A Ortega Ruiz
 *
 * diagram_tree_view.c : complete rewrite to get rid of deprecated widgets
 * Copyright (C) 2009 Hans Breuer
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

#include <gtk/gtk.h>

#include <lib/object.h>

#include "diagram_tree_model.h"
#include "diagram_tree.h"

#include <lib/group.h> /* IS_GROUP */
#include "display.h"
#include "properties-dialog.h" /* object_list_properties_show */
#include "dia-diagram-properties-dialog.h" /* diagram_properties_show */
#include "persistence.h"
#include "widgets.h"
#include "dia-layer.h"

typedef struct _DiagramTreeView DiagramTreeView;
struct _DiagramTreeView {
  GtkTreeView parent_instance;

  GtkUIManager *ui_manager;
  GtkMenu      *popup;
};

typedef struct _DiagramTreeViewClass DiagramTreeViewClass;
struct _DiagramTreeViewClass {
  GtkTreeViewClass parent_class;
};

#define DIAGRAM_TREE_VIEW_TYPE (_dtv_get_type ())
#define DIAGRAM_TREE_VIEW(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), DIAGRAM_TREE_VIEW_TYPE, DiagramTreeView))

static GType _dtv_get_type (void);
#if 0
//future
G_DEFINE_TYPE_WITH_CODE (DiagramTreeView, _dtv, GTK_TYPE_TREE_VIEW,
			 G_IMPLEMENT_INTERFACE (DIAGRAM_TYPE_SEARCHABLE,
						_dtv_init))
#else
G_DEFINE_TYPE (DiagramTreeView, _dtv, GTK_TYPE_TREE_VIEW);
#endif

static gboolean
_dtv_button_press (GtkWidget      *widget,
		   GdkEventButton *event)
{
  GtkTreeSelection *selection;
  GtkTreeModel     *model;
  GtkTreePath      *path = NULL;
  GtkTreeIter       iter;
  DiaObject        *object;
  Diagram          *diagram;

  if (event->button == 3) {
    selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (widget));
    model = gtk_tree_view_get_model (GTK_TREE_VIEW (widget));

    if (!gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (widget),
					event->x, event->y,
                                        &path, NULL, NULL, NULL)) {
      return TRUE;
    }

    if (gtk_tree_selection_count_selected_rows (gtk_tree_view_get_selection (GTK_TREE_VIEW (widget))) < 2) {
      gtk_tree_selection_unselect_all (selection);
      gtk_tree_selection_select_path (selection, path);

      /* FIXME: for multiple selection we should check all object selected being compatible */
      gtk_tree_model_get_iter (model, &iter, path);
      gtk_tree_model_get (model, &iter, OBJECT_COLUMN, &object, -1);
      gtk_tree_path_free (path);
    }

    gtk_menu_popup (DIAGRAM_TREE_VIEW (widget)->popup, NULL, NULL, NULL, NULL, event->button, event->time);
  } else {
    GTK_WIDGET_CLASS (_dtv_parent_class)->button_press_event (widget, event);

    if (gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (widget),
                                       event->x, event->y,
                                       &path, NULL, NULL, NULL)) {
      model = gtk_tree_view_get_model (GTK_TREE_VIEW (widget));
      gtk_tree_model_get_iter (model, &iter, path);
      gtk_tree_model_get (model, &iter, OBJECT_COLUMN, &object, -1);
      gtk_tree_model_get (model, &iter, DIAGRAM_COLUMN, &diagram, -1);

      if (object && diagram && ddisplay_active_diagram () == diagram) {
        if (event->button == 1 && event->type == GDK_2BUTTON_PRESS) {
          /* double-click 'locates' */
          ddisplay_present_object (ddisplay_active(), object);
        }
      }
      g_clear_object (&diagram);
      gtk_tree_path_free (path);
    }
  }

  return TRUE;
}


static gboolean
_dtv_query_tooltip (GtkWidget  *widget,
                    int         x,
                    int         y,
                    gboolean    keyboard_mode,
                    GtkTooltip *tooltip)
{
  int                    bin_x, bin_y;
  GtkTreePath           *path = NULL;
  GtkTreeViewColumn     *column;
  GtkTreeIter            iter;
  GtkTreeModel          *model;
  GdkRectangle           cell_area;
  GString               *markup;
  gboolean               had_info = FALSE;

  gtk_tree_view_convert_widget_to_bin_window_coords (
	GTK_TREE_VIEW (widget), x, y, &bin_x, &bin_y);

  if (gtk_tree_view_get_path_at_pos
	(GTK_TREE_VIEW (widget), bin_x, bin_y, &path, &column, NULL, NULL)) {

    model = gtk_tree_view_get_model (GTK_TREE_VIEW (widget));
    if (gtk_tree_model_get_iter (model, &iter, path)) {
      /* show some useful  information */
      Diagram   *diagram;
      DiaLayer  *layer;
      DiaObject *object;


      gtk_tree_model_get (model, &iter, DIAGRAM_COLUMN, &diagram, -1);
      gtk_tree_model_get (model, &iter, LAYER_COLUMN, &layer, -1);
      gtk_tree_model_get (model, &iter, OBJECT_COLUMN, &object, -1);

      markup = g_string_new (NULL);

      if (diagram) {
        char *em = g_markup_printf_escaped ("<b>%s</b>: %s\n",
                                            _("Diagram"),
                                            diagram->filename);

        g_string_append (markup, em);

        g_clear_pointer (&em, g_free);
      }

      if (layer) {
        const char *name = dia_layer_get_name (layer);
        char *em = g_markup_printf_escaped ("<b>%s</b>: %s\n",
                                            _("Layer"),
                                            name);

        g_string_append (markup, em);

        g_clear_pointer (&em, g_free);
      } else if (diagram) {
        int layers = data_layer_count (DIA_DIAGRAM_DATA (diagram));

        g_string_append_printf (markup,
                                g_dngettext (GETTEXT_PACKAGE,
                                             "%d Layer",
                                             "%d Layers",
                                             layers),
                                layers);
      }

      if (object) {
        char *em = g_markup_printf_escaped ("<b>%s</b>: %s\n",
                                            _("Type"),
                                            object->type->name);

        g_string_append (markup, em);

        g_clear_pointer (&em, g_free);

        g_string_append_printf (markup,
                                "<b>%s</b>: %g,%g\n",
                                Q_("object|Position"),
                                object->position.x,
                                object->position.y);
        g_string_append_printf (markup,
                                "%d %s",
                                g_list_length (object->children),
                                _("Children"));
        /* and some dia_object_get_meta ? */
      } else if (layer) {
        int objects = dia_layer_object_count (layer);

        g_string_append_printf (markup,
                                g_dngettext (GETTEXT_PACKAGE,
                                             "%d Object",
                                             "%d Objects",
                                             objects),
                                objects);
      }

      if (markup->len > 0) {
        gtk_tree_view_get_cell_area (GTK_TREE_VIEW (widget), path, column, &cell_area);

        gtk_tree_view_convert_bin_window_to_widget_coords (GTK_TREE_VIEW (widget),
                                                           cell_area.x,
                                                           cell_area.y,
                                                           &cell_area.x,
                                                           &cell_area.y);

        gtk_tooltip_set_tip_area (tooltip, &cell_area);
        gtk_tooltip_set_markup (tooltip, markup->str);
        had_info = TRUE;
      }

      /* drop references */
      g_clear_object (&diagram);
      g_string_free (markup, TRUE);
    }
    gtk_tree_path_free (path);
    if (had_info) {
      return TRUE;
    }
  }

  return GTK_WIDGET_CLASS (_dtv_parent_class)->query_tooltip (widget,
                                                              x,
                                                              y,
                                                              keyboard_mode,
                                                              tooltip);
}


static void
_dtv_row_activated (GtkTreeView       *view,
		    GtkTreePath       *path,
		    GtkTreeViewColumn *column)
{
  GtkTreeModel *model;
  GtkTreeIter   iter;
  DiaObject    *object;

  model = gtk_tree_view_get_model (view);

  if (gtk_tree_model_get_iter (model, &iter, path)) {
    gtk_tree_model_get (model, &iter, OBJECT_COLUMN, &object, -1);

    /* FIXME: g_signal_emit (view, signals[REVISION_ACTIVATED], 0, object); */

  }

  if (GTK_TREE_VIEW_CLASS (_dtv_parent_class)->row_activated)
    GTK_TREE_VIEW_CLASS (_dtv_parent_class)->row_activated (view, path, column);
}

static void
_dtv_finalize (GObject *object)
{
  G_OBJECT_CLASS (_dtv_parent_class)->finalize (object);
}
static void
_dtv_class_init (DiagramTreeViewClass *klass)
{
  GObjectClass     *object_class    = G_OBJECT_CLASS (klass);
  GtkWidgetClass   *widget_class    = GTK_WIDGET_CLASS (klass);
  GtkTreeViewClass *tree_view_class = GTK_TREE_VIEW_CLASS (klass);

  object_class->finalize            = _dtv_finalize;

  widget_class->button_press_event  = _dtv_button_press;
  widget_class->query_tooltip       = _dtv_query_tooltip;

  tree_view_class->row_activated    = _dtv_row_activated;
}

/* given the model and the path, construct the icon */
static void
_dtv_cell_pixbuf_func (GtkCellLayout   *layout,
		       GtkCellRenderer *cell,
		       GtkTreeModel    *tree_model,
		       GtkTreeIter     *iter,
		       gpointer         data)
{
  DiaObject *object;
  GdkPixbuf *pixbuf = NULL;

  gtk_tree_model_get (tree_model, iter, OBJECT_COLUMN, &object, -1);
  if (object) {
    pixbuf = dia_object_type_get_icon (object->type);
    if (pixbuf == NULL && IS_GROUP(object)) {
      pixbuf = pixbuf_from_resource ("/org/gnome/Dia/icons/dia-group.png");
    }
  } else {
#if 0 /* these icons are not that useful */
    Layer *layer;
    gtk_tree_model_get (tree_model, iter, LAYER_COLUMN, &layer, -1);
    if (layer)
      pixbuf = pixbuf_from_resource ("/org/gnome/Dia/icons/dia-layers.png");
    else /* must be diagram */
      pixbuf = pixbuf_from_resource ("/org/gnome/Dia/icons/dia-diagram.png");
#endif
  }

  g_object_set (cell, "pixbuf", pixbuf, NULL);
  g_clear_object (&pixbuf);
}


static void
_dtv_select_items (GtkAction *action,
                  DiagramTreeView *dtv)
{
  GtkTreeSelection *selection;
  GtkTreeModel     *model;
  GList            *rows, *r;
  gboolean          once = TRUE;

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (dtv));
  rows = gtk_tree_selection_get_selected_rows (selection, &model);
  r = rows;
  while (r) {
    GtkTreeIter     iter;

    if (gtk_tree_model_get_iter (model, &iter, r->data)) {
      Diagram   *diagram;
      DiaLayer  *layer;
      DiaObject *object;

      gtk_tree_model_get (model, &iter, DIAGRAM_COLUMN, &diagram, -1);
      gtk_tree_model_get (model, &iter, LAYER_COLUMN, &layer, -1);
      gtk_tree_model_get (model, &iter, OBJECT_COLUMN, &object, -1);

      if (once) { /* destroy previous selection in first iteration */
	diagram_remove_all_selected(diagram, TRUE);
	once = FALSE;
      }

      if (layer) /* fixme: layer dialog update missing */
        data_set_active_layer (DIA_DIAGRAM_DATA(diagram), layer);
      if (object)
        diagram_select (diagram, object);
      if (diagram) {
        diagram_add_update_all (diagram);
        diagram_flush (diagram);
        g_clear_object (&diagram);
      }
    }
    r = g_list_next (r);
  }
  g_list_foreach (rows, (GFunc) gtk_tree_path_free, NULL);
  g_list_free (rows);
}
static void
_dtv_locate_item (GtkAction *action,
                  DiagramTreeView *dtv)
{
  GtkTreeSelection *selection;
  GtkTreeModel     *model;
  GList            *rows, *r;

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (dtv));
  rows = gtk_tree_selection_get_selected_rows (selection, &model);
  r = rows;
  while (r) {
    GtkTreeIter     iter;

    if (gtk_tree_model_get_iter (model, &iter, r->data)) {
      Diagram   *diagram;
      DiaObject *object;

      gtk_tree_model_get (model, &iter, DIAGRAM_COLUMN, &diagram, -1);
      gtk_tree_model_get (model, &iter, OBJECT_COLUMN, &object, -1);

      if (object && diagram) {
        /* It's kind of stupid to have this running in two loops:
         * - one may expect to have one presentation for all selected objects
         * - if there really would be multiple displays and objects we would
         *   iterate through them with a very questionable order ;)
         */
        GSList *displays;

        for (displays = diagram->displays;
             displays != NULL; displays = g_slist_next (displays)) {
          DDisplay *ddisp = (DDisplay *)displays->data;

          ddisplay_present_object (ddisp, object);
        }
      }
      /* drop all references got from the model */
      g_clear_object (&diagram);
    }
    r = g_list_next (r);
  }
  g_list_foreach (rows, (GFunc) gtk_tree_path_free, NULL);
  g_list_free (rows);
}
static void
_dtv_showprops_item (GtkAction *action,
                     DiagramTreeView *dtv)
{
  GList *selected = NULL;
  GtkTreeSelection *selection;
  GtkTreeModel     *model;
  GList            *rows, *r;
  Diagram          *diagram = NULL;

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (dtv));
  rows = gtk_tree_selection_get_selected_rows (selection, &model);
  r = rows;
  while (r) {
    GtkTreeIter     iter;

    if (gtk_tree_model_get_iter (model, &iter, r->data)) {
      DiaObject *object;
      Diagram   *current_diagram;

      gtk_tree_model_get (model, &iter, DIAGRAM_COLUMN, &current_diagram, -1);
      gtk_tree_model_get (model, &iter, OBJECT_COLUMN, &object, -1);

      if (object) {
        selected = g_list_append (selected, object);
      }

      if (!diagram) {
        /* keep the reference */
        diagram = current_diagram;
      } else if (diagram == current_diagram) {
        /* still the same */
        g_clear_object (&current_diagram);
      } else {
        g_clear_object (&current_diagram);
        break; /* can only handle one diagram's objects */
      }
    }
    r = g_list_next (r);
  }

  if (diagram && selected) {
    object_list_properties_show (diagram, selected);
  } else if (diagram) {
    diagram_properties_show (diagram);
  }

  g_clear_object (&diagram);

  g_list_foreach (rows, (GFunc) gtk_tree_path_free, NULL);
  g_list_free (rows);
}
/* create the popup menu */
static void
_dtv_create_popup_menu (DiagramTreeView *dtv)
{
  static GtkActionEntry menu_items [] = {
    { "Select",  NULL, N_("Select"), NULL, NULL, G_CALLBACK (_dtv_select_items) },
    { "Locate",  NULL, N_("Locate"), NULL, NULL, G_CALLBACK (_dtv_locate_item) },
    { "Properties", NULL, N_("Properties"), NULL, NULL, G_CALLBACK(_dtv_showprops_item) }
  };
  static const gchar *ui_description =
    "<ui>"
    "  <popup name='PopupMenu'>"
    "    <menuitem action='Select'/>"
    "    <menuitem action='Locate'/>"
    "    <menuitem action='Properties'/>"
    "  </popup>"
    "</ui>";
  GtkActionGroup *action_group;

  g_return_if_fail (dtv->popup == NULL && dtv->ui_manager == NULL);

  action_group = gtk_action_group_new ("PopupActions");
  gtk_action_group_set_translation_domain (action_group, NULL); /* FIXME? */
  gtk_action_group_add_actions (action_group, menu_items,
				G_N_ELEMENTS (menu_items), dtv);

  dtv->ui_manager = gtk_ui_manager_new ();
  gtk_ui_manager_insert_action_group (dtv->ui_manager, action_group, 0);
  if (gtk_ui_manager_add_ui_from_string (dtv->ui_manager, ui_description, -1, NULL))
    dtv->popup = GTK_MENU(gtk_ui_manager_get_widget (dtv->ui_manager, "/ui/PopupMenu"));
}

static void
_dtv_init (DiagramTreeView *dtv)
{
  GtkTreeViewColumn *column;
  GtkCellRenderer   *cell;
  int                font_size;
  GtkStyleContext   *style;
  PangoFontDescription *font_style;

  /* connect the model with the view */
  gtk_tree_view_set_model (GTK_TREE_VIEW (dtv), diagram_tree_model_new ());

  gtk_tree_view_set_fixed_height_mode (GTK_TREE_VIEW (dtv), TRUE);
  /* the tree requires reading across rows (semantic hint) */
#if 0 /* stripe style */
  gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (dtv), TRUE);
#endif

  style = gtk_widget_get_style_context (GTK_WIDGET (dtv));
  gtk_style_context_get(style, GTK_STATE_FLAG_NORMAL,
                        GTK_STYLE_PROPERTY_FONT, &font_style, NULL);
  font_size = pango_font_description_get_size (font_style);
  font_size = PANGO_PIXELS (font_size);

  /* first colum: name of diagram/layer/object - here is the tree */
  cell = gtk_cell_renderer_text_new ();
  gtk_cell_renderer_text_set_fixed_height_from_font (GTK_CELL_RENDERER_TEXT (cell), 1);
  g_object_set (cell, "ellipsize", PANGO_ELLIPSIZE_END, NULL);

  column = gtk_tree_view_column_new ();
  gtk_tree_view_column_set_title (column, _("Name"));
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
  gtk_tree_view_column_set_min_width (column, font_size * 10);
  gtk_tree_view_column_set_expand (column, TRUE);
  gtk_tree_view_column_set_resizable (column, TRUE);

  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (column), cell, TRUE);
  gtk_tree_view_column_add_attribute (column, cell, "text", NAME_COLUMN);
  gtk_tree_view_insert_column (GTK_TREE_VIEW (dtv), column, -1);

  /* this is enough for simple name search (just typing) */
  gtk_tree_view_set_search_column (GTK_TREE_VIEW (dtv), NAME_COLUMN);
  /* must have a sortable model ... */
  gtk_tree_view_column_set_sort_column_id (column, NAME_COLUMN);

  /* type column - show the type icon */
  column = gtk_tree_view_column_new ();
  gtk_tree_view_column_set_title (column, _("Type"));
  /* must have fixed size, too */
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
  /* without it gets zero size - not very useful! */
  gtk_tree_view_column_set_min_width (column, font_size * 6);
  gtk_tree_view_column_set_resizable (column, TRUE);
  cell = gtk_cell_renderer_pixbuf_new ();
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (column), cell, TRUE);
  gtk_cell_layout_set_cell_data_func (GTK_CELL_LAYOUT (column), cell,
				      _dtv_cell_pixbuf_func, dtv, NULL);
  gtk_tree_view_insert_column (GTK_TREE_VIEW (dtv), column, -1);
  gtk_tree_view_column_set_sort_column_id (column, OBJECT_COLUMN);

  /*  TODO: other fancy stuff */

  {
    GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (dtv));

    gtk_tree_selection_set_mode (selection, GTK_SELECTION_MULTIPLE);
    gtk_tree_view_set_rubber_banding (GTK_TREE_VIEW (dtv), TRUE);
  }
  gtk_widget_set_has_tooltip (GTK_WIDGET (dtv), TRUE);


  _dtv_create_popup_menu (dtv);
}

static GtkWidget *
diagram_tree_view_new (void)
{
  return g_object_new (DIAGRAM_TREE_VIEW_TYPE, NULL);
}

/* should eventually go to it's own file, just for testing
 * The DiagramTreeView should remain integrateable with the integrated UI.
 */
void
diagram_tree_show (void)
{
  static GtkWidget *window = NULL;

  if (!window) {
    GtkWidget *sw;
    GtkWidget *dtv;

    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title (GTK_WINDOW (window), _("Diagram Tree"));

    sw = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (sw), GTK_SHADOW_ETCHED_IN);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_container_add (GTK_CONTAINER (window), sw);
    gtk_window_set_default_size (GTK_WINDOW (window), 300, 600);

    dtv = diagram_tree_view_new ();
    gtk_container_add (GTK_CONTAINER (sw), dtv);
    /* expand all rows after the treeview widget has been realized */
    g_signal_connect (dtv, "realize",
		      G_CALLBACK (gtk_tree_view_expand_all), NULL);
    g_signal_connect (G_OBJECT (window), "destroy",
		      G_CALLBACK (gtk_widget_destroyed),
		      &window);

    gtk_window_set_role (GTK_WINDOW (window), "diagram_tree");

    if (!gtk_widget_get_visible (window))
      gtk_widget_show_all (window);

    /* FIXME: remove flicker by removing gtk_widget_show from persistence_register_window() */
    persistence_register_window (GTK_WINDOW (window));
  }
  gtk_window_present (GTK_WINDOW(window));
}
