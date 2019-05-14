/*
 * File: plug-ins/xslt/xslt.h
 *
 * Made by Matthieu Sozeau <mattam@netcourrier.com>
 *
 * Started on  Thu May 16 23:22:12 2002 Matthieu Sozeau
 * Last update Mon Jun  3 19:36:32 2002 Matthieu Sozeau
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
 *
 */
#include "config.h"
#include <glib.h>
#include <gtk/gtk.h>
#ifndef G_OS_WIN32
#include <dlfcn.h>
#endif
#include "plug-ins.h"
#include "intl.h"

/*
  The transformation is 2-pass. First, it transforms the document using
  a fromsl_s stylesheet, then apply a specific stylesheet to the result.
  e.g. for uml to java: apply the dia-uml.xsl stylesheet, then dia-uml2java.xsl
*/

/* Stylesheets for a specific primary stylesheet */
typedef struct toxsl_s {
  gchar *name;
  gchar *xsl;
  struct toxsl_s *next;
} toxsl_t;

/* Primary stylesheet for a dia object type */
typedef struct fromxsl_s {
  gchar *name;
  gchar *xsl;
  toxsl_t *xsls;
} fromxsl_t;

/* Possible stylesheets */
extern GPtrArray *froms;

/* Selected stylesheets */
extern toxsl_t *xsl_to;
extern fromxsl_t *xsl_from;

void     xslt_dialog_create (void);
void     xslt_ok            (void);
void     xslt_clear         (void);
void     xslt_unload        (PluginInfo *info);
gboolean xslt_can_unload    (PluginInfo *info);
