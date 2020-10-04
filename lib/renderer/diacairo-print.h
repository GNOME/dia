#pragma once

#include <gtk/gtk.h>
#include "diagramdata.h"
#include "dia-object-change.h"


G_BEGIN_DECLS

GtkPrintOperation *create_print_operation (DiagramData *data,
                                           const char  *name);
DiaObjectChange   *cairo_print_callback   (DiagramData *dia,
                                           const char  *filename,
                                           /* further additions */
                                           guint        flags,
                                           void        *user_data);

G_END_DECLS
