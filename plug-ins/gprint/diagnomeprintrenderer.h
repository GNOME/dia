/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998, 1999 Alexander Larsson
 *
 * render_gnomeprint.[ch] -- gnome-print renderer for dia
 * Copyright (C) 1999 James Henstridge
 *
 * diagnomeprintrenderer.[ch] - resurrection as plug-in and porting to
 *                              DiaRenderer interface
 * Copyright (C) 2004 Hans Breuer
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
 
#ifndef DIA_GNOME_PRINT_RENDERER_H
#define DIA_GNOME_PRINT_RENDERER_H

#include "diarenderer.h"
#include <libgnomeprint/gnome-print.h>

G_BEGIN_DECLS

#define DIA_GNOME_PRINT_TYPE_RENDERER           (dia_gnome_print_renderer_get_type ())
#define DIA_GNOME_PRINT_RENDERER(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), DIA_GNOME_PRINT_TYPE_RENDERER, DiaGnomePrintRenderer))
#define DIA_GNOME_PRINT_RENDERER_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST ((klass), DIA_GNOME_PRINT_TYPE_RENDERER, DiaGnomePrintRendererClass))
#define DIA_IS_GNOME_PRINT_RENDERER(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DIA_GNOME_PRINT_TYPE_RENDERER))
#define DIA_GNOME_PRINT_RENDERER_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), DIA_GNOME_PRINT_TYPE_RENDERER, DiaGnomePrintRendererClass))

GType dia_gnome_print_renderer_get_type (void) G_GNUC_CONST;

typedef struct _DiaGnomePrintRenderer DiaGnomePrintRenderer;
typedef struct _DiaGnomePrintRendererClass DiaGnomePrintRendererClass;

struct _DiaGnomePrintRenderer
{
  DiaRenderer parent_instance;

  GnomePrintConfig  *config;
  GnomePrintContext *ctx;

  DiaFont *font;
  LineStyle saved_line_style;
  real dash_length;
  real dot_length; 
};

struct _DiaGnomePrintRendererClass
{
  DiaRendererClass parent_class;
};

G_END_DECLS

#endif /* DIA_GNOME_PRINT_RENDERER_H */
