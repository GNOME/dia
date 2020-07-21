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

#pragma once

#include <glib.h>
#include <diatypes.h>


G_BEGIN_DECLS


struct _PaperInfo {
  char     *name;
  double    tmargin;
  double    bmargin;
  double    lmargin;
  double    rmargin;
  gboolean  is_portrait;
  double    scaling;
  gboolean  fitto;
  int       fitwidth;
  int       fitheight;
  double    width;
  double    height;
};


int         find_paper          (const char     *name);
int         get_default_paper   (void);
void        get_paper_info      (PaperInfo      *paper,
                                 int             i,
                                 NewDiagramData *data);
GList      *get_paper_name_list (void);
const char *dia_paper_get_name  (int             i);
double      get_paper_psheight  (int             i);
double      get_paper_pswidth   (int             i);
double      get_paper_lmargin   (int             i);
double      get_paper_rmargin   (int             i);
double      get_paper_bmargin   (int             i);
double      get_paper_tmargin   (int             i);

G_END_DECLS
