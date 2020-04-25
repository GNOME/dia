#pragma once

#include <gtk/gtk.h>
#include "diagram.h"

G_BEGIN_DECLS

/**
 * ConfirmationKind:
 * @CONFIRM_PAGES: Confirm number of pages
 * @CONFIRM_MEMORY: Confirm memory usage
 * @CONFIRM_PRINT: This is for printing
 *
 * Since: dawn-of-time
 */
typedef enum {
  CONFIRM_PAGES = (1<<0),
  CONFIRM_MEMORY = (1<<1),
  CONFIRM_PRINT = (1<<2)
} ConfirmationKind;

gboolean confirm_export_size (Diagram *dia, GtkWindow *parent, guint flags);

G_END_DECLS
