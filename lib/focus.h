/* Dia -- a diagram creation/manipulation program
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
#ifndef FOCUS_H
#define FOCUS_H

#include "object.h"

typedef struct _Focus Focus;

struct _Focus {
  Object *obj;
  int has_focus;
  void *user_data; /* To be used by the object using this focus (eg. Text) */

  /* return TRUE if modified object. */
  int (*key_event)(Focus *focus, guint keysym, char *str, int strlen);
};

extern void request_focus(Focus *focus);
extern Focus *active_focus(void);
extern void remove_focus(void);

#endif /* FOCUS_H */




