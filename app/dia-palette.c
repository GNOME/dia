/* Dia -- an diagram creation/manipulation program
 * Copyright © 2019 Zander Brown <zbrown@gnome.org>
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

#include "dia-palette.h"

struct _DiaPalette
{
  GtkGrid parent;

  int active;
  GtkWidget *dialog;
  GtkWidget *button;
};


G_DEFINE_TYPE (DiaPalette, dia_palette, GTK_TYPE_GRID)

static void
dia_palette_dialog_ok (GtkWidget *widget, DiaPalette *self)
{
  gtk_dialog_response (GTK_DIALOG (self->dialog), GTK_RESPONSE_OK);
}

static void
dia_palette_class_init (DiaPaletteClass *class)
{
}

static void
dia_palette_init (DiaPalette *self)
{
}


GtkWidget *
dia_palette_new ()
{
  return g_object_new (DIA_TYPE_PALETTE, NULL);
}
