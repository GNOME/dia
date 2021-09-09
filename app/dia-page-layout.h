/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998, 1999 Alexander Larsson
 *
 * diapagelayout.[ch] -- a page settings widget for dia.
 * Copyright (C) 1999 James Henstridge
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

#pragma once

#include <gtk/gtk.h>

#include "dia-app-enums.h"

G_BEGIN_DECLS


#define DIA_TYPE_PAGE_LAYOUT dia_page_layout_get_type ()
G_DECLARE_FINAL_TYPE (DiaPageLayout, dia_page_layout, DIA, PAGE_LAYOUT, GtkGrid)


/**
 * DiaPageOrientation:
 * @DIA_PAGE_ORIENT_PORTRAIT: The page is portrait
 * @DIA_PAGE_ORIENT_LANDSCAPE: The page is landscape
 *
 * Since: dawn-of-time
 */
typedef enum /*< enum,prefix=DIA >*/ {
  DIA_PAGE_ORIENT_PORTRAIT, /*< nick=portrait >*/
  DIA_PAGE_ORIENT_LANDSCAPE /*< nick=landscape >*/
} DiaPageOrientation;


GtkWidget          *dia_page_layout_new                (void);
const char         *dia_page_layout_get_paper          (DiaPageLayout      *pl);
void                dia_page_layout_set_paper          (DiaPageLayout      *pl,
                                                        const char         *paper);
void                dia_page_layout_get_margins        (DiaPageLayout      *pl,
                                                        double             *tmargin,
                                                        double             *bmargin,
                                                        double             *lmargin,
                                                        double             *rmargin);
void                dia_page_layout_set_margins        (DiaPageLayout      *pl,
                                                        double              tmargin,
                                                        double              bmargin,
                                                        double              lmargin,
                                                        double              rmargin);
DiaPageOrientation  dia_page_layout_get_orientation    (DiaPageLayout      *pl);
void                dia_page_layout_set_orientation    (DiaPageLayout      *pl,
                                                        DiaPageOrientation  orient);
gboolean            dia_page_layout_get_fitto          (DiaPageLayout      *self);
void                dia_page_layout_set_fitto          (DiaPageLayout      *self,
                                                        gboolean            fitto);
double              dia_page_layout_get_scaling        (DiaPageLayout      *self);
void                dia_page_layout_set_scaling        (DiaPageLayout      *self,
                                                        double              scaling);
void                dia_page_layout_get_fit_dims       (DiaPageLayout      *self,
                                                        int                *w,
                                                        int                *h);
void                dia_page_layout_set_fit_dims       (DiaPageLayout      *self,
                                                        int                 w,
                                                        int                 h);
void                dia_page_layout_set_changed        (DiaPageLayout      *self,
                                                        gboolean            changed);
void                dia_page_layout_get_effective_area (DiaPageLayout      *self,
                                                        double             *width,
                                                        double             *height);

/* get paper sizes and default margins ... */
void dia_page_layout_get_paper_size      (const char   *paper,
                                          double       *width,
                                          double       *height);
void dia_page_layout_get_default_margins (const char   *paper,
                                          double       *tmargin,
                                          double       *bmargin,
                                          double       *lmargin,
                                          double       *rmargin);
