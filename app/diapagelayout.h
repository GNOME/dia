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

#define DIA_PAGE_LAYOUT(obj) GTK_CHECK_CAST(obj, dia_page_layout_get_type(), DiaPageLayout)
#define DIA_PAGE_LAYOUT_CLASS(klass) GTK_CHECK_CLASS_CAST(klass, dia_page_layout_get_type(), DiaPageLayoutClass)
#define DIA_IS_PAGE_LAYOUT(obj) GTK_CHECK_TYPE(obj, dia_page_layout_get_type())

typedef struct _DiaPageLayout DiaPageLayout;
typedef struct _DiaPageLayoutClass DiaPageLayoutClass;

typedef enum {
  DIA_PAGE_ORIENT_PORTRAIT,
  DIA_PAGE_ORIENT_LANDSCAPE
} DiaPageOrientation;

struct _DiaPageLayout {
  GtkTable parent;

  /*<private>*/
  GtkWidget *paper_size, *paper_label;
  GtkWidget *orient_portrait, *orient_landscape;
  GtkWidget *tmargin, *bmargin, *lmargin, *rmargin;
  GtkWidget *scaling;
  GtkWidget *fittopage;

  GtkWidget *darea;

  GdkGC *gc;
  GdkColor white, black, blue;
  gint papernum; /* index into page_metrics array */

  /* position of paper preview */
  gint x, y, width, height;

  gboolean block_changed;
};

struct _DiaPageLayoutClass {
  GtkTableClass parent_class;

  void (*changed)(DiaPageLayout *pl);
  void (*fittopage)(DiaPageLayout *pl);
};

GtkType      dia_page_layout_get_type    (void);
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
gfloat       dia_page_layout_get_scaling (DiaPageLayout *self);
void         dia_page_layout_set_scaling (DiaPageLayout *self,
					  gfloat scaling);

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
