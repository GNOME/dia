/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * List of "dynamic" (animated) objects, and their refresh rates
 * Copyright (C) 2002 Cyrille Chépélov
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

#ifndef DYNAMIC_REFRESH_H
#define DYNAMIC_REFRESH_H

/* Starts the dynamic refresh stuff */
void dynobj_refresh_init(void);

/* Terminates the dynamic refresh stuff */
void dynobj_refresh_finish(void);

/* Launches the dynamic refresh idle handler at the next possibility */
void dynobj_refresh_kick(void);


#endif /* DYNAMIC_REFRESH_H */
