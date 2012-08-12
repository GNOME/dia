/* prepare deprecation of GtkRuler */
#ifndef DIA_RULER_H
#define DIA_RULER_H

#include "display.h"

GtkWidget *dia_ruler_new (GtkOrientation  orientation,
			  GtkWidget      *shell,
			  DDisplay       *ddisp);

void dia_ruler_set_range (GtkWidget      *ruler,
                          gdouble         lower,
                          gdouble         upper,
                          gdouble         position,
                          gdouble         max_size);
#endif
