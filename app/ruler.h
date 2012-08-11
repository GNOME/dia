/* prepare deprecation of GtkRuler */
#include <gtk/gtk.h>

GtkWidget *dia_ruler_new (GtkOrientation  orientation,
			  GtkWidget      *shell);

void dia_ruler_set_range (GtkWidget      *ruler,
                          gdouble         lower,
                          gdouble         upper,
                          gdouble         position,
                          gdouble         max_size);
