#ifndef _PAGINATE_GDIPRINT_H
#define _PAGINATE_GDIPRINT_H

#include "diagramdata.h"

extern "C" void diagram_print_gdi(DiagramData *data, const gchar* filename, DiaContext *ctx);

#endif
