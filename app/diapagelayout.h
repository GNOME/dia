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

#ifndef DIAPAGELAYOUT_H
#define DIAPAGELAYOUT_H

#include <gtk/gtk.h>


typedef struct _DiaPageLayout DiaPageLayout;
GType      dia_page_layout_get_type    (void);
#define DIA_PAGE_LAYOUT(obj) G_TYPE_CHECK_INSTANCE_CAST (obj, dia_page_layout_get_type(), DiaPageLayout)

typedef enum {
  DIA_PAGE_ORIENT_PORTRAIT,
  DIA_PAGE_ORIENT_LANDSCAPE
} DiaPageOrientation;

GtkWidget   *dia_page_layout_new         (void);

const gchar *dia_page_layout_get_paper   (DiaPageLayout *pl);
void         dia_page_layout_set_paper   (DiaPageLayout *pl,
					  const gchar *paper);
void         dia_page_layout_get_margins (DiaPageLayout *pl,
					  gfloat *tmargin, gfloat *bmargin,
					  gfloat *lmargin, gfloat *rmargin);
void         dia_page_layout_set_margins (DiaPageLayout *pl,
					  gfloat tmargin, gfloat bmargin,
					  gfloat lmargin, gfloat rmargin);
DiaPageOrientation dia_page_layout_get_orientation (DiaPageLayout *pl);
void         dia_page_layout_set_orientation (DiaPageLayout *pl,
					      DiaPageOrientation orient);
gboolean     dia_page_layout_get_fitto   (DiaPageLayout *self);
void         dia_page_layout_set_fitto   (DiaPageLayout *self,
					  gboolean fitto);
gfloat       dia_page_layout_get_scaling (DiaPageLayout *self);
void         dia_page_layout_set_scaling (DiaPageLayout *self,
					  gfloat scaling);
void         dia_page_layout_get_fit_dims(DiaPageLayout *self,
					  gint *w, gint *h);
void         dia_page_layout_set_fit_dims(DiaPageLayout *self,
					  gint w, gint h);
void         dia_page_layout_set_changed (DiaPageLayout *self,
					  gboolean changed);

void         dia_page_layout_get_effective_area (DiaPageLayout *self,
						 gfloat *width,
						 gfloat *height);

/* get paper sizes and default margins ... */
void dia_page_layout_get_paper_size      (const gchar *paper,
					  gfloat *width, gfloat *height);
void dia_page_layout_get_default_margins (const gchar *paper,
					  gfloat *tmargin, gfloat *bmargin,
					  gfloat *lmargin, gfloat *rmargin);

#endif
