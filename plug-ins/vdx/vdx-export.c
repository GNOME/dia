#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>

#include <string.h>
#include <math.h>
#include <glib.h>
#include <stdlib.h>
#include <errno.h>
#include <locale.h>

#include "intl.h"
#include "message.h"
#include "geometry.h"
#include "diarenderer.h"
#include "filter.h"
#include "object.h"
#include "properties.h"
#include "dia_image.h"
#include "group.h"

#include "vdx.h"

gboolean
export_vdx(DiagramData *data, const gchar *filename, 
           const gchar *diafilename, void* user_data);

gboolean
export_vdx(DiagramData *data, const gchar *filename, 
           const gchar *diafilename, void* user_data)
{
    message_warning("Visio export not yet supported"); 
    return FALSE;
}

static const gchar *extensions[] = { "vdx", NULL };
DiaExportFilter vdx_export_filter = {
  N_("Visio XML format"),
  extensions,
  export_vdx
};
