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
#ifndef ATTRIBUTES_H
#define ATTRIBUTES_H

#include "geometry.h"
#include "color.h"

extern Color attributes_get_foreground(void);
extern Color attributes_get_background(void);
extern void attributes_set_foreground(Color *color);
extern void attributes_set_background(Color *color);
extern void attributes_swap_fgbg(void);
extern void attributes_default_fgbg(void);

extern real attributes_get_default_linewidth(void);
extern void attributes_set_default_linewidth(real width);

#endif /* ATTRIBUTES_H */
