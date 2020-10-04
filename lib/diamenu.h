/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1999 Alexander Larsson
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
#ifndef DIAMENU_H
#define DIAMENU_H

#include "diatypes.h"
#include "dia-object-change.h"

/* Flags for DiaMenuItem->active */
#define DIAMENU_ACTIVE (1<<0)
#define DIAMENU_TOGGLE (1<<1)
#define DIAMENU_TOGGLE_ON (1<<2)

/* Note: The returned change is already applied. */
typedef DiaObjectChange *(*DiaMenuCallback) (DiaObject *obj,
                                             Point     *pos,
                                             gpointer   data);

struct _DiaMenuItem {
  char *text;
  DiaMenuCallback callback;
  gpointer callback_data;
  int active; /* Actually flags now, but keeps name for compatibility */
  /* Private for app:  */
  void *app_data; /* init to NULL */
};

typedef void (*DiaMenuAppDataFree) (DiaMenu *menu);

struct _DiaMenu {
  char *title;
  int num_items;
  DiaMenuItem *items;
  /* Private for app:  */
  void *app_data; /* init to NULL */
  DiaMenuAppDataFree app_data_free;
};

#endif /*DIAMENU_H*/
