/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
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

#ifndef PAPER_H
#define PAPER_H

#include <glib.h>
#include <diatypes.h>

struct _PaperInfo {
  gchar *name;      /* name of the paper */
  gfloat tmargin, bmargin, lmargin, rmargin; /* margin widths in centimeters */
  gboolean is_portrait;   /* page is in portrait orientation? */
  gfloat scaling;         /* scaling factor for image on page */
  gboolean fitto;         /* if we want to use the fitto mode for scaling */
  gint fitwidth, fitheight; /* how many pages in each direction */

  gfloat width, height;   /* usable width/height -- calculated from paper type,
			   * margin widths and paper orientation; the real paper
			   * size is width*scaling, height*scaling */
};

int find_paper(const gchar* name);
int get_default_paper(void);
void get_paper_info(PaperInfo *paper, int i, NewDiagramData *data);

GList *get_paper_name_list(void);
const gchar *get_paper_name(int i);
gdouble get_paper_psheight(int i);
gdouble get_paper_pswidth(int i);
gdouble get_paper_lmargin(int i);
gdouble get_paper_rmargin(int i);
gdouble get_paper_bmargin(int i);
gdouble get_paper_tmargin(int i);

#endif
