#ifndef _PAGINATE_GNOMEPRINT_H
#define _PAGINATE_GNOMEPRINT_H

#include <libgnomeprint/gnome-print.h>

#include "diagram.h"
#include "diagramdata.h"
#include "render_gnomeprint.h"

void paginate_gnomeprint (Diagram *dia,
			  GnomePrintContext *ctx,
			  const gchar *paper_name);

#endif
