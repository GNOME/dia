#include <config.h>

#include "diacellrendererenum.h"
#include "properties.h"

enum
{
  COLUMN_ENUM_NAME,
  COLUMN_ENUM_VALUE,
  NUM_ENUM_COLUMNS
};

static void
_enum_changed (GtkCellRenderer *renderer, const char *string, GtkTreeIter *iter, GtkTreeModel *model)
{
  int val;
  GValue value = { 0, };

  gtk_tree_model_get (model, iter, COLUMN_ENUM_VALUE, &val, -1);
  g_value_init (&value, G_TYPE_INT);
  g_value_set_int (&value, val);
  g_object_set_property (G_OBJECT (renderer), "text", &value);
  g_value_unset (&value);
  g_print ("changed: %d - %s\n", val, string);
}
static void
_enum_edited (GtkCellRenderer *renderer, const char *path, const char *new_string, GtkTreeModel *model)
{
  GtkTreeIter iter;
  int val;

  if (gtk_tree_model_get_iter_from_string (model, &iter, path)) {
    gtk_tree_model_get (model, &iter, COLUMN_ENUM_VALUE, &val, -1);
    g_print ("edited: %d - %s\n", val, new_string);
  }
}

GtkCellRenderer *
dia_cell_renderer_enum_new (const PropEnumData *enum_data)
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

  g_signal_connect (G_OBJECT (cell_renderer), "changed", G_CALLBACK (_enum_changed), model); 
  g_signal_connect (G_OBJECT (cell_renderer), "edited", G_CALLBACK (_enum_edited), model); 

  return cell_renderer;
}
