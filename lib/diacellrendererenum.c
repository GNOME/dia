#include "config.h"

#include <glib/gi18n-lib.h>

#include "diacellrendererenum.h"
#include "properties.h"

enum
{
  COLUMN_ENUM_NAME,
  COLUMN_ENUM_VALUE,
  NUM_ENUM_COLUMNS
};

/*!
 * \brief Signal handler GtkCellRendererCombo::changed:
 *
 * From GTK+:
 *
 * @renderer: the object on which the signal is emitted
 * @path_string: a string of the path identifying the edited cell
 *               (relative to the tree view model)
 * @iter: the new iter selected in the combo box
 *            (relative to the combo box model)
 *
 * This signal is emitted each time after the user selected an item in
 * the combo box, either by using the mouse or the arrow keys.  Contrary
 * to GtkComboBox, GtkCellRendererCombo::changed is not emitted for
 * changes made to a selected item in the entry.  The argument 'iter'
 * corresponds to the newly selected item in the combo box and it is relative
 * to the GtkTreeModel set via the model property on GtkCellRendererCombo.
 *
 * Note that as soon as you change the model displayed in the tree view,
 * the tree view will immediately cease the editing operating.  This
 * means that you most probably want to refrain from changing the model
 * until the combo cell renderer emits the edited or editing_canceled signal.
 */
static void
_enum_changed (GtkCellRenderer *renderer,
	       const char      *path_string,
	       GtkTreeIter     *iter,
	       GtkTreeView     *tree_view)
{
  GtkTreeModel *store = gtk_tree_view_get_model (tree_view);
  int val, column;
  GtkTreeModel *model;
  GtkTreeIter store_iter;

  g_object_get (G_OBJECT (renderer), "model", &model, NULL);
  /* read the enum value from the combobox model */
  gtk_tree_model_get (model, iter, COLUMN_ENUM_VALUE, &val, -1);
  /* put it into the store model */
  column = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (renderer), COLUMN_KEY));
  if (gtk_tree_model_get_iter_from_string (store, &store_iter, path_string))
      gtk_tree_store_set (GTK_TREE_STORE (store), &store_iter, column, val, -1);

  g_print ("changed: %d - %s\n", val, path_string);
}
/*!
 * \brief Signal handler GtkCellRendererText::edited
 '
 * From GTK+:
 * This signal is emitted after @renderer has been edited.
 *
 * It is the responsibility of the application to update the model
 * and store @new_text at the position indicated by @path.
 */
static void
_enum_edited (GtkCellRenderer *renderer,
	      const char      *path_string,
	      const char      *new_string,
	      GtkTreeView     *tree_view)
{
  GtkTreeModel *store = gtk_tree_view_get_model (tree_view);
  /* Despite the comment for _enum_changed we have changed the model already
   * there. Changing it here would involve some caching of the value there
   * or translating new_string back to the enumv we need for the store.
   * But we have to mark the store to be modified otherwise only the UI state
   * would change without transferring back in _arrayprop_set_from_widget()
   */
  g_object_set_data (G_OBJECT (store), "modified", GINT_TO_POINTER (1));

  // FIXME: What should the number here be?
  g_print ("edited: %d - %s\n", 42, new_string);
}

GtkCellRenderer *
dia_cell_renderer_enum_new (const PropEnumData *enum_data, GtkTreeView *tree_view)
{
  /* The combo-renderer should be customized for better rendering,
   * e.g. using a shorter name like visible_char[]={ '+', '-', '#', ' ' } instead of
   * the full of _uml_visibilities[]; likewise for line style there or arrows there
   * should be a pixbuf preview instead of strings ...
   */
  GtkCellRenderer *cell_renderer = gtk_cell_renderer_combo_new ();
  /* create the model from enum_data */
  GtkListStore *model;
  GtkTreeIter iter;
  int i;

  model = gtk_list_store_new (NUM_ENUM_COLUMNS, G_TYPE_STRING, G_TYPE_INT);
  for (i = 0; enum_data[i].name != NULL; ++i) {

    gtk_list_store_append (model, &iter);

    gtk_list_store_set (model, &iter,
                        COLUMN_ENUM_NAME, enum_data[i].name,
                        COLUMN_ENUM_VALUE, enum_data[i].enumv,
                        -1);
  }

  g_object_set (cell_renderer,
                "model", model,
                "text-column", COLUMN_ENUM_NAME,
                "has-entry", FALSE,
                "editable", TRUE,
                NULL);

  g_signal_connect (G_OBJECT (cell_renderer), "changed",
		    G_CALLBACK (_enum_changed), tree_view);
  g_signal_connect (G_OBJECT (cell_renderer), "edited",
		    G_CALLBACK (_enum_edited), tree_view);

  return cell_renderer;
}
