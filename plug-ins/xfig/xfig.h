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

#include "dia-colour.h"

G_BEGIN_DECLS

extern DiaColour fig_default_colors[];
extern char *fig_fonts[];

int num_fig_fonts (void);

#define FIG_MAX_DEFAULT_COLORS 32
#define FIG_MAX_USER_COLORS 512
#define FIG_MAX_DEPTHS 1000
/* 1200 PPI */
#define FIG_UNIT 472.440944881889763779527559055118
/* 1/80 inch */
#define FIG_ALT_UNIT 31.496062992125984251968503937007

G_END_DECLS
