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

#pragma once

#include <glib-object.h>

G_BEGIN_DECLS

/*
 * The transformation is 2-pass. First, it transforms the document using
 * a stylesheet, then apply a specific target stylesheet to the result.
 * e.g. for uml to java: apply the dia-uml.xsl stylesheet, then dia-uml2java.xsl
 */

#define DIA_XSLT_TYPE_STYLESHEET_TARGET (dia_xslt_stylesheet_target_get_type ())

typedef struct _DiaXsltStylesheetTarget DiaXsltStylesheetTarget;

GType                    dia_xslt_stylesheet_target_get_type (void);
DiaXsltStylesheetTarget *dia_xslt_stylesheet_target_ref      (DiaXsltStylesheetTarget   *self);
void                     dia_xslt_stylesheet_target_unref    (DiaXsltStylesheetTarget   *self);
const char              *dia_xslt_stylesheet_target_get_name (DiaXsltStylesheetTarget   *self);


#define DIA_XSLT_TYPE_STYLESHEET (dia_xslt_stylesheet_get_type ())

typedef struct _DiaXsltStylesheet DiaXsltStylesheet;

GType                    dia_xslt_stylesheet_get_type        (void);
DiaXsltStylesheet       *dia_xslt_stylesheet_ref             (DiaXsltStylesheet         *self);
void                     dia_xslt_stylesheet_unref           (DiaXsltStylesheet         *self);
const char              *dia_xslt_stylesheet_get_name        (DiaXsltStylesheet         *self);
void                     dia_xslt_stylesheet_get_targets     (DiaXsltStylesheet         *self,
                                                              DiaXsltStylesheetTarget ***targets,
                                                              size_t                    *n_targets);


#define DIA_XSLT_TYPE_PLUGIN (dia_xslt_plugin_get_type ())

G_DECLARE_FINAL_TYPE (DiaXsltPlugin, dia_xslt_plugin, DIA_XSLT, PLUGIN, GObject)


G_END_DECLS
