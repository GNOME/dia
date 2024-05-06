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

#include "config.h"

#include <glib/gi18n-lib.h>

#include <xpm-pixbuf.h>

#include "sheet-editor-button.h"
#include "widgets.h"
#include "message.h"


typedef struct _DiaSheetEditorButtonPrivate DiaSheetEditorButtonPrivate;
struct _DiaSheetEditorButtonPrivate {
  gboolean        is_newline;
  SheetMod       *sheet;
  SheetObjectMod *object;

  GtkWidget      *image;
};


G_DEFINE_TYPE_WITH_PRIVATE (DiaSheetEditorButton, dia_sheet_editor_button, GTK_TYPE_RADIO_BUTTON)


enum {
  PROP_0,
  PROP_SHEET,
  PROP_IS_NEWLINE,
  PROP_OBJECT,
  LAST_PROP
};
static GParamSpec *pspecs[LAST_PROP] = { NULL, };


static GdkPixbuf *
get_fallback_pixbuf (void)
{
  static GdkPixbuf *instance;

  if (instance == NULL) {
    instance = pixbuf_from_resource ("/org/gnome/Dia/icons/dia-object-unknown.png");
    g_object_add_weak_pointer (G_OBJECT (instance), (gpointer *) &instance);
  }

  return instance;
}


static GdkPixbuf *
get_object_pixbuf (SheetObject *so)
{
  GdkPixbuf *pixbuf = NULL;

  g_return_val_if_fail (so, NULL);

  if (so->pixmap != NULL) {
    if (g_str_has_prefix ((char *) so->pixmap, "res:")) {
      pixbuf = pixbuf_from_resource (((char *) so->pixmap) + 4);
    } else {
      pixbuf = xpm_pixbuf_load (so->pixmap);
    }
  } else {
    if (so->pixmap_file != NULL) {
      GError *error = NULL;

      pixbuf = gdk_pixbuf_new_from_file (so->pixmap_file, &error);

      if (error) {
        message_warning ("%s", error->message);
        g_clear_error (&error);
      }
    }
  }

  if (pixbuf != NULL) {
    int width = gdk_pixbuf_get_width (pixbuf);
    int height = gdk_pixbuf_get_height (pixbuf);

    if (width > 22) {
      GdkPixbuf *cropped;

      g_warning ("Shape icon '%s' size wrong, cropped.", so->pixmap_file);

      cropped = gdk_pixbuf_new_subpixbuf (pixbuf,
                                          (width - 22) / 2,
                                          height > 22 ? (height - 22) / 2 : 0,
                                          22,
                                          height > 22 ? 22 : height);

      g_clear_object (&pixbuf);
      pixbuf = cropped;
    }
  } else {
    message_warning (_("No icon for “%s”"), so->object_type);
    pixbuf = get_fallback_pixbuf ();
  }

  return pixbuf;
}


static void
dia_sheet_editor_button_set_property (GObject      *object,
                                      guint         property_id,
                                      const GValue *value,
                                      GParamSpec   *pspec)
{
  DiaSheetEditorButton *self = DIA_SHEET_EDITOR_BUTTON (object);
  DiaSheetEditorButtonPrivate *priv = dia_sheet_editor_button_get_instance_private (self);

  switch (property_id) {
    case PROP_SHEET:
      priv->sheet = g_value_get_pointer (value);
      break;
    case PROP_IS_NEWLINE:
      priv->is_newline = g_value_get_boolean (value);
      break;
    case PROP_OBJECT:
      priv->object = g_value_get_pointer (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}


static void
dia_sheet_editor_button_get_property (GObject    *object,
                                      guint       property_id,
                                      GValue     *value,
                                      GParamSpec *pspec)
{
  DiaSheetEditorButton *self = DIA_SHEET_EDITOR_BUTTON (object);
  DiaSheetEditorButtonPrivate *priv = dia_sheet_editor_button_get_instance_private (self);

  switch (property_id) {
    case PROP_SHEET:
      g_value_set_pointer (value, priv->sheet);
      break;
    case PROP_IS_NEWLINE:
      g_value_set_boolean (value, priv->is_newline);
      break;
    case PROP_OBJECT:
      g_value_set_pointer (value, priv->object);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}


static void
dia_sheet_editor_button_constructed (GObject *object)
{
  DiaSheetEditorButton *self = DIA_SHEET_EDITOR_BUTTON (object);
  DiaSheetEditorButtonPrivate *priv = dia_sheet_editor_button_get_instance_private (self);

  g_return_if_fail (priv->sheet != NULL);

  if (priv->is_newline) {
    gtk_image_set_from_icon_name (GTK_IMAGE (priv->image),
                                  "dia-newline",
                                  GTK_ICON_SIZE_BUTTON);
    gtk_widget_set_tooltip_text (GTK_WIDGET (self), _("Line Break"));
    gtk_button_set_relief (GTK_BUTTON (self), GTK_RELIEF_NONE);
  } else {
    g_return_if_fail (priv->object != NULL);

    gtk_image_set_from_pixbuf (GTK_IMAGE (priv->image),
                               get_object_pixbuf (&priv->object->sheet_object));
  }
}


static void
dia_sheet_editor_button_class_init (DiaSheetEditorButtonClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = dia_sheet_editor_button_set_property;
  object_class->get_property = dia_sheet_editor_button_get_property;
  object_class->constructed = dia_sheet_editor_button_constructed;

  /**
   * DiaSheetEditorButton:sheet:
   *
   * Since: 0.98
   */
  pspecs[PROP_SHEET] =
    g_param_spec_pointer ("sheet",
                          "Sheet",
                          "The sheet this is in",
                          G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

  /**
   * DiaSheetEditorButton:is-newline:
   *
   * Since: 0.98
   */
  pspecs[PROP_IS_NEWLINE] =
    g_param_spec_boolean ("is-newline",
                          "Is Newline",
                          "This button is a newline",
                          FALSE,
                          G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

  /**
   * DiaSheetEditorButton:object:
   *
   * Since: 0.98
   */
  pspecs[PROP_OBJECT] =
    g_param_spec_pointer ("object",
                          "Object",
                          "The object this represents",
                          G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

  g_object_class_install_properties (object_class, LAST_PROP, pspecs);
}


static void
dia_sheet_editor_button_init (DiaSheetEditorButton *self)
{
  DiaSheetEditorButtonPrivate *priv = dia_sheet_editor_button_get_instance_private (self);

  gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (self), FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (self), 0);

  priv->image = gtk_image_new ();
  gtk_widget_show (priv->image);
  gtk_container_add (GTK_CONTAINER (self), priv->image);
}


GtkWidget *
dia_sheet_editor_button_new_object (GSList         *group,
                                    SheetMod       *sheet,
                                    SheetObjectMod *object)
{
  GtkWidget *btn = g_object_new (DIA_TYPE_SHEET_EDITOR_BUTTON,
                                 "sheet", sheet,
                                 "is-newline", FALSE,
                                 "object", object,
                                  NULL);

  if (group) {
    gtk_radio_button_set_group (GTK_RADIO_BUTTON (btn), group);
  }

  return btn;
}


GtkWidget *
dia_sheet_editor_button_new_newline (GSList   *group,
                                     SheetMod *sheet)
{
  GtkWidget *btn = g_object_new (DIA_TYPE_SHEET_EDITOR_BUTTON,
                                 "sheet", sheet,
                                 "is-newline", TRUE,
                                 NULL);

  if (group) {
    gtk_radio_button_set_group (GTK_RADIO_BUTTON (btn), group);
  }

  return btn;
}


SheetMod *
dia_sheet_editor_button_get_sheet (DiaSheetEditorButton *self)
{
  DiaSheetEditorButtonPrivate *priv;

  g_return_val_if_fail (DIA_IS_SHEET_EDITOR_BUTTON (self), NULL);

  priv = dia_sheet_editor_button_get_instance_private (self);

  return priv->sheet;
}


SheetObjectMod *
dia_sheet_editor_button_get_object (DiaSheetEditorButton *self)
{
  DiaSheetEditorButtonPrivate *priv;

  g_return_val_if_fail (DIA_IS_SHEET_EDITOR_BUTTON (self), NULL);

  priv = dia_sheet_editor_button_get_instance_private (self);

  return priv->object;
}
