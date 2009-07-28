
#include <gtk/gtkversion.h>

#include <gtk/gtkprintoperation.h>
#include "diagramdata.h"

GtkPrintOperation *
create_print_operation (DiagramData *data, 
			const char *name);

void
cairo_print_callback (DiagramData *dia,
                      const gchar *filename,
                      guint flags, /* further additions */
                      void *user_data);
