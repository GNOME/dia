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

#include <config.h>

#include <stdio.h>
#include <string.h>

#include "paper.h"
#include "diagramdata.h" /* for NewDiagramData */


/**
 * PaperInfo:
 * @name: name of the paper
 * @tmargin: margin widths in %DIA_UNIT_CENTIMETER
 * @bmargin: "
 * @lmargin: "
 * @rmargin: "
 * @is_portrait: page is in portrait orientation?
 * @scaling: scaling factor for image on page
 * @fitto: if we want to use the fitto mode for scaling
 * @fitwidth: how many pages in each direction
 * @fitheight: "
 * @width: usable width/height -- calculated from paper type,
 *         margin widths and paper orientation; the real paper
 *         size is width*scaling, height*scaling
 * @height: "
 *
 * Since: dawn-of-time
 */


/* Paper definitions stolen from gnome-libs.
 * All measurements are in centimetres. */

static const struct _dia_paper_metrics {
  char *paper;
  double pswidth, psheight;
  double lmargin, tmargin, rmargin, bmargin;
} paper_metrics[] = {
  { "A0", 84.1, 118.9, 2.8222, 2.8222, 2.8222, 2.8222 },
  { "A1", 59.4, 84.1, 2.8222, 2.8222, 2.8222, 2.8222 },
  { "A2", 42.0, 59.4, 2.8222, 2.8222, 2.8222, 2.8222 },
  { "A3", 29.7, 42.0, 2.8222, 2.8222, 2.8222, 2.8222 },
  { "A4", 21.0, 29.7, 2.8222, 2.8222, 2.8222, 2.8222 },
  { "A5", 14.8, 21.0, 2.8222, 2.8222, 2.8222, 2.8222 },
  { "A6", 10.5, 14.8, 2.8222, 2.8222, 2.8222, 2.8222 },
  { "B0-Japan", 103.0, 145.6, 2.1167, 2.1167, 2.1167, 2.1167 },
  { "B1-Japan", 72.8, 103.0, 2.1167, 2.1167, 2.1167, 2.1167 },
  { "B2-Japan", 51.5, 72.8, 2.1167, 2.1167, 2.1167, 2.1167 },
  { "B3-Japan", 36.4, 51.5, 2.1167, 2.1167, 2.1167, 2.1167 },
  { "B4-Japan", 25.7, 36.4, 2.1167, 2.1167, 2.1167, 2.1167 },
  { "B5-Japan", 18.2, 25.7, 2.8222, 2.8222, 2.8222, 2.8222 },
  { "B6-Japan", 12.8, 18.2, 2.8222, 2.8222, 2.8222, 2.8222 },
  { "B0", 100.0, 141.4, 2.1167, 2.1167, 2.1167, 2.1167 },
  { "B1", 70.7, 100.0, 2.1167, 2.1167, 2.1167, 2.1167 },
  { "B2", 50.0, 70.7, 2.1167, 2.1167, 2.1167, 2.1167 },
  { "B3", 35.3, 50.0, 2.1167, 2.1167, 2.1167, 2.1167 },
  { "B4", 25.0, 35.3, 2.1167, 2.1167, 2.1167, 2.1167 },
  { "B5", 17.6, 25.0, 2.8222, 2.8222, 2.8222, 2.8222 },
  { "B6", 12.5, 17.6, 2.8222, 2.8222, 2.8222, 2.8222 },
  { "Letter", 21.59, 27.94, 2.54, 2.54, 2.54, 2.54 },
  { "Legal", 21.59, 35.56, 2.54, 2.54, 2.54, 2.54 },
  { "Ledger", 27.9, 43.2, 2.54, 2.54, 2.54, 2.54 },
  { "Half-Letter", 21.59, 14.0, 2.54, 2.54, 2.54, 2.54 },
  { "Executive", 18.45, 26.74, 2.54, 2.54, 2.54, 2.54 },
  { "Tabloid", 28.01, 43.2858, 2.54, 2.54, 2.54, 2.54 },
  { "Monarch", 9.8778, 19.12, 0.3528, 0.3528, 0.3528, 0.3528 },
  { "SuperB", 29.74, 43.2858, 2.8222, 2.8222, 2.8222, 2.8222 },
  { "Envelope-Commercial", 10.5128, 24.2, 0.1764, 0.1764, 0.1764, 0.1764 },
  { "Envelope-Monarch", 9.8778, 19.12, 0.1764, 0.1764, 0.1764, 0.1764 },
  { "Envelope-DL", 11.0, 22.0, 0.1764, 0.1764, 0.1764, 0.1764 },
  { "Envelope-C5", 16.2278, 22.9306, 0.1764, 0.1764, 0.1764, 0.1764 },
  { "EuroPostcard", 10.5128, 14.8167, 0.1764, 0.1764, 0.1764, 0.1764 },
  { NULL, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 }
};


int
find_paper (const char *name)
{
  int i;

  if (name == NULL) return -1;
  for (i = 0; paper_metrics[i].paper != NULL; i++) {
    if (!g_ascii_strncasecmp(paper_metrics[i].paper, name,
		             strlen(paper_metrics[i].paper)))
      break;
  }
  if (paper_metrics[i].paper == NULL)
    i = -1;

  return i;
}

int
get_default_paper(void)
{
  FILE *papersize;
  gchar paper[100];
  const gchar *env;
  gint i;

  if((env = g_getenv("PAPERCONF")) != NULL) {
    g_strlcpy(paper, env, sizeof(paper));
  }
  else if((papersize = fopen("/etc/papersize", "r")) != NULL) {
    while(fgets(paper, sizeof(paper), papersize))
      if(g_ascii_isalnum(paper[0])) {
	break;
      }
    fclose(papersize);
  }
  else
    strcpy(paper, "a4");

  i = find_paper(paper);
  if (i == -1)
    i = find_paper("a4");

  return i;
}

void
get_paper_info(PaperInfo *paper, int i, NewDiagramData *prefs)
{
  if (i == -1 && prefs != NULL)
    i = find_paper(prefs->papertype);
  if (i == -1)
    i = get_default_paper();

  paper->name = g_strdup(paper_metrics[i].paper);
  paper->tmargin = paper_metrics[i].tmargin;
  paper->bmargin = paper_metrics[i].bmargin;
  paper->lmargin = paper_metrics[i].lmargin;
  paper->rmargin = paper_metrics[i].rmargin;
  if (prefs != NULL)
    paper->is_portrait = prefs->is_portrait;
  else
    paper->is_portrait = TRUE;
  paper->scaling = 1.0;
  paper->fitto = FALSE;
  paper->fitwidth = 1;
  paper->fitheight = 1;
  paper->width = paper_metrics[i].pswidth -
    paper_metrics[i].lmargin -
    paper_metrics[i].rmargin;
  paper->height = paper_metrics[i].psheight -
    paper_metrics[i].tmargin -
    paper_metrics[i].bmargin;
  if (!paper->is_portrait) {
    double tmp = paper->width;
    paper->width = paper->height;
    paper->height = tmp;
  }
}

GList *
get_paper_name_list(void)
{
  int i;
  static GList *name_list = NULL;

  if (name_list == NULL) {
    for (i = 0; paper_metrics[i].paper != NULL; i++) {
      name_list = g_list_append(name_list, paper_metrics[i].paper);
    }
  }

  return name_list;
}


const char *
dia_paper_get_name (int i)
{
  return paper_metrics[i].paper;
}


gdouble
get_paper_psheight(int i)
{
  return paper_metrics[i].psheight;
}

gdouble
get_paper_pswidth(int i)
{
  return paper_metrics[i].pswidth;
}

gdouble
get_paper_lmargin(int i)
{
  return paper_metrics[i].lmargin;
}

gdouble
get_paper_rmargin(int i)
{
  return paper_metrics[i].rmargin;
}

gdouble
get_paper_tmargin(int i)
{
  return paper_metrics[i].tmargin;
}

gdouble
get_paper_bmargin(int i)
{
  return paper_metrics[i].bmargin;
}
