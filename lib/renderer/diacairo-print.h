
#include <gtk/gtk.h>
#include "diagramdata.h"

GtkPrintOperation *
create_print_operation (DiagramData *data, 
			const char *name);

ObjectChange *
cairo_print_callback (DiagramData *dia,
                      const gchar *filename,
                      guint flags, /* further additions */
                      void *user_data);
