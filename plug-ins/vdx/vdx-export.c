/* -*- Mode: C; c-basic-offset: 4 -*- */
/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * vdx-export.c: Visio XML export filter for dia
 * Copyright (C) 2006 Ian Redfern
 * based on the xfig filter code
 * Copyright (C) 2001 Lars Clausen
 * based on the dxf filter code
 * Copyright (C) 2000 James Henstridge, Steffen Macke
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

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
