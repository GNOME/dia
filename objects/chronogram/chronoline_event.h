/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * Chronogram objects support
 * Copyright (C) 2000 Cyrille Chepelov 
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

#ifndef __CHRONOLINE_EVENT_H
#define __CHRONOLINE_EVENT_H

#include <glib.h>
#include "geometry.h"

typedef enum {
  CLE_OFF, /* data is now in the "off" state */
  CLE_ON,  /* data is now in the "on" state */
  CLE_UNKNOWN, /* data is now in the "unknown" state */
  CLE_START /* should not be found outside this module. */
} CLEventType;

typedef struct {
  CLEventType type;
  real time;
  real x;
} CLEvent;

typedef GSList CLEventList; /* of CLEvent */

void destroy_cle(gpointer data, gpointer user_data);
extern void destroy_clevent_list(CLEventList *clel);
extern void reparse_clevent(const gchar *events, CLEventList **lst,
			    int *chksum, real rise, real fall, real time_end);

#endif /* __CHRONOLINE_EVENT_H */
