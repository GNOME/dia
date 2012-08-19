#include <config.h>

#include "diacellrendererenum.h"
#include "properties.h"

enum
{
  COLUMN_ENUM_NAME,
  COLUMN_ENUM_VALUE,
  NUM_ENUM_COLUMNS
};

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

  return cell_renderer;
}
