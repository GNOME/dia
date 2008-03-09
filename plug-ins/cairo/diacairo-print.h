
#include <gtk/gtkversion.h>

#if GTK_CHECK_VERSION(2,10,0)
#include <gtk/gtkprintoperation.h>
#include "diagramdata.h"

GtkPrintOperation *
create_print_operation (DiagramData *data, 
			const char *name);

void
cairo_print_callback (DiagramData *dia,
                      guint flags, /* further additions */
                      void *user_data);
#endif
