#ifndef DIA_CELLRENDERERENUM_H
#define DIA_CELLRENDERERENUM_H

#include <gtk/gtk.h>
#include "diatypes.h"

/* Found no built-in way to get to the column in the callback ... */
#define COLUMN_KEY "column-key"

GtkCellRenderer *dia_cell_renderer_enum_new (const PropEnumData *enum_data,
					     GtkTreeView        *tree_view);

#endif
