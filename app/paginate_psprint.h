#ifndef _PAGINATE_PSPRINT_H
#define _PAGINATE_PSPRINT_H

#include <stdio.h>
#include <glib.h>

#include "diagram.h"
#include "diagramdata.h"

void paginate_psprint (Diagram *dia, FILE *file, const gchar *paper_name,
		       gdouble scale);

void diagram_print_ps (Diagram *dia);

#endif
