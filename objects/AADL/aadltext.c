/* AADL plugin for DIA
*
* Copyright (C) 2005 Laboratoire d'Informatique de Paris 6
* Author: Pierre Duquesne
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

#include "config.h"

#include <glib/gi18n-lib.h>

#include "aadl.h"

/* functions useful for text positionning */

/* DATA */
void aadldata_text_position(Aadlbox *aadlbox, Point *p)
{
  Element *elem = &aadlbox->element;

  text_calc_boundingbox(aadlbox->name, NULL);
  p->x = elem->corner.x + AADLBOX_TEXT_MARGIN;
  p->y = elem->corner.y + AADLBOX_TEXT_MARGIN + aadlbox->name->ascent;
}

void aadldata_minsize(Aadlbox *aadlbox, Point *size)
{
  text_calc_boundingbox(aadlbox->name, NULL);
  size->x = aadlbox->name->max_width + 2*AADLBOX_TEXT_MARGIN;
  size->y = aadlbox->name->height * aadlbox->name->numlines
                                   + 2*AADLBOX_TEXT_MARGIN;
}


/* PROCESS */
void aadlprocess_text_position(Aadlbox *aadlbox, Point *p)
{
  Element *elem = &aadlbox->element;

  text_calc_boundingbox(aadlbox->name, NULL);
  p->x = elem->corner.x + elem->width * AADLBOX_INCLINE_FACTOR
                        + AADLBOX_TEXT_MARGIN;
  p->y = elem->corner.y + AADLBOX_TEXT_MARGIN + aadlbox->name->ascent;
}

void aadlprocess_minsize(Aadlbox *aadlbox, Point *size)
{
  /*
         [-------------L--------]
                           [-d--]
              /-----------------/
             / |----w-----|    /
            /  |          |   /
           /   | T E X T  |  /
          /    |----------| /
         /-----------------/


     L == 2d + w   and   d == L*Factor   <===>  L == w / (1 - 2*Factor)

  */
  real w, L;

  text_calc_boundingbox(aadlbox->name, NULL);
  w = aadlbox->name->max_width + 2*AADLBOX_TEXT_MARGIN;
  L = w / (1 - 2 * AADLBOX_INCLINE_FACTOR);

  size->x = L;
  size->y = aadlbox->name->height * aadlbox->name->numlines
              + 2*AADLBOX_TEXT_MARGIN;
}

/* BUS */
void aadlbus_text_position(Aadlbox *aadlbox, Point *p)
{
  Element *elem = &aadlbox->element;

  text_calc_boundingbox(aadlbox->name, NULL);
  p->x = elem->corner.x + elem->width * AADL_BUS_ARROW_SIZE_FACTOR
                        + AADLBOX_TEXT_MARGIN;
  p->y = elem->corner.y + elem->height * AADL_BUS_HEIGHT_FACTOR
                        + AADLBOX_TEXT_MARGIN + aadlbox->name->ascent;
}

void aadlbus_minsize(Aadlbox *aadlbox, Point *size)
{
  real w, L, h, H;

  text_calc_boundingbox(aadlbox->name, NULL);
  w = aadlbox->name->max_width + 2*AADLBOX_TEXT_MARGIN;
  L = w / (1 - 2 * AADL_BUS_ARROW_SIZE_FACTOR);

  h = aadlbox->name->height * aadlbox->name->numlines
                    + 2*AADLBOX_TEXT_MARGIN;
  H = h / (1 - 2 * AADL_BUS_HEIGHT_FACTOR);

  size->x = L;
  size->y = H;
}


/*  SYSTEM */
#define SYSTEM_FACTOR (AADL_ROUNDEDBOX_CORNER_SIZE_FACTOR / 5)

void aadlsystem_text_position(Aadlbox *aadlbox, Point *p)
{
  Element *elem = &aadlbox->element;

  text_calc_boundingbox(aadlbox->name, NULL);
  p->x = elem->corner.x + elem->width * SYSTEM_FACTOR
                        + AADLBOX_TEXT_MARGIN;
  p->y = elem->corner.y + elem->height * SYSTEM_FACTOR
                        + AADLBOX_TEXT_MARGIN + aadlbox->name->ascent;
}

void aadlsystem_minsize(Aadlbox *aadlbox, Point *size)
{
  real w, L, h, H;

  text_calc_boundingbox(aadlbox->name, NULL);
  w = aadlbox->name->max_width + 2*AADLBOX_TEXT_MARGIN;
  L = w / (1 - 2 * SYSTEM_FACTOR);

  h = aadlbox->name->height * aadlbox->name->numlines
                    + 2*AADLBOX_TEXT_MARGIN;
  H = h / (1 - 2 * SYSTEM_FACTOR);

  size->x = L;
  size->y = H;
}

/* MEMORY */
#define MEMORY_FACTOR (AADL_MEMORY_FACTOR)

void aadlmemory_text_position(Aadlbox *aadlbox, Point *p)
{
  Element *elem = &aadlbox->element;

  text_calc_boundingbox(aadlbox->name, NULL);
  p->x = elem->corner.x + AADLBOX_TEXT_MARGIN;
  p->y = elem->corner.y + 2 * elem->height * MEMORY_FACTOR
                        + AADLBOX_TEXT_MARGIN + aadlbox->name->ascent;
}

void aadlmemory_minsize(Aadlbox *aadlbox, Point *size)
{
  real w, h, H;

  text_calc_boundingbox(aadlbox->name, NULL);
  w = aadlbox->name->max_width + 2*AADLBOX_TEXT_MARGIN;

  h = aadlbox->name->height * aadlbox->name->numlines
                    + 2*AADLBOX_TEXT_MARGIN;
  H = h / (1 - 3 * MEMORY_FACTOR);

  size->x = w;
  size->y = H;
}

/* SUBPROGRAM */
void aadlsubprogram_text_position(Aadlbox *aadlbox, Point *p)
{
  Element *elem = &aadlbox->element;

  text_calc_boundingbox(aadlbox->name, NULL);
  p->x = elem->corner.x+ (2-sqrt(2))/4 * elem->width + AADLBOX_TEXT_MARGIN;
  p->y = elem->corner.y+ (2-sqrt(2))/4 * elem->height + AADLBOX_TEXT_MARGIN
         + aadlbox->name->ascent;
}

void aadlsubprogram_minsize(Aadlbox *aadlbox, Point *size)
{
  real w, h;

  text_calc_boundingbox(aadlbox->name, NULL);

  w = aadlbox->name->max_width + 2*AADLBOX_TEXT_MARGIN;
  h = aadlbox->name->height * aadlbox->name->numlines
                  + 2*AADLBOX_TEXT_MARGIN;

  size->x = w * sqrt(2);
  size->y = h * sqrt(2);
}

