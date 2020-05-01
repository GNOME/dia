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
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Copyright © 2020 Zander Brown <zbrown@gnome.org>
 */

#pragma once

#include <gtk/gtk.h>

G_BEGIN_DECLS


G_DEFINE_AUTOPTR_CLEANUP_FUNC (GtkBuilder, g_object_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC (GtkSpinButton, g_object_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC (GtkComboBox, g_object_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC (GtkHBox, g_object_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC (GtkTreeView, g_object_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC (GtkCellRenderer, g_object_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC (GtkCellRendererText, g_object_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC (GtkBin, g_object_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC (GtkContainer, g_object_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC (GtkVBox, g_object_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC (GtkDialog, g_object_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC (GtkEventBox, g_object_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC (GtkTable, g_object_unref)

G_END_DECLS
