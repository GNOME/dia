/* GTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * GtkFontSelection widget for Gtk+, by Damon Chaplin, May 1998.
 * Based on the GnomeFontSelector widget, by Elliot Lee, but major changes.
 * The GnomeFontSelector was derived from app/text_tool.c in the GIMP.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/*
 * Modified by the GTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.gtk.org/pub/gtk/. 
 */

#ifndef __DIA_GTK_FONTSEL_H__
#define __DIA_GTK_FONTSEL_H__


#include <gdk/gdk.h>
#include <gtk/gtkdialog.h>
#include <gtk/gtkvbox.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define DIA_GTK_TYPE_FONT_SELECTION              (dia_gtk_font_selection_get_type ())
#define DIA_GTK_FONT_SELECTION(obj)              (GTK_CHECK_CAST ((obj), DIA_GTK_TYPE_FONT_SELECTION, DiaGtkFontSelection))
#define DIA_GTK_FONT_SELECTION_CLASS(klass)      (GTK_CHECK_CLASS_CAST ((klass), DIA_GTK_TYPE_FONT_SELECTION, DiaGtkFontSelectionClass))
#define DIA_GTK_IS_FONT_SELECTION(obj)           (GTK_CHECK_TYPE ((obj), DIA_GTK_TYPE_FONT_SELECTION))
#define DIA_GTK_IS_FONT_SELECTION_CLASS(klass)   (GTK_CHECK_CLASS_TYPE ((klass), DIA_GTK_TYPE_FONT_SELECTION))
#define DIA_GTK_FONT_SELECTION_GET_CLASS(obj)    (GTK_CHECK_GET_CLASS ((obj), DIA_GTK_TYPE_FONT_SELECTION, DiaGtkFontSelectionClass))


#define DIA_GTK_TYPE_FONT_SELECTION_DIALOG              (dia_gtk_font_selection_dialog_get_type ())
#define DIA_GTK_FONT_SELECTION_DIALOG(obj)              (GTK_CHECK_CAST ((obj), DIA_GTK_TYPE_FONT_SELECTION_DIALOG, DiaGtkFontSelectionDialog))
#define DIA_GTK_FONT_SELECTION_DIALOG_CLASS(klass)      (GTK_CHECK_CLASS_CAST ((klass), DIA_GTK_TYPE_FONT_SELECTION_DIALOG, DiaGtkFontSelectionDialogClass))
#define DIA_GTK_IS_FONT_SELECTION_DIALOG(obj)           (GTK_CHECK_TYPE ((obj), DIA_GTK_TYPE_FONT_SELECTION_DIALOG))
#define DIA_GTK_IS_FONT_SELECTION_DIALOG_CLASS(klass)   (GTK_CHECK_CLASS_TYPE ((klass), DIA_GTK_TYPE_FONT_SELECTION_DIALOG))
#define DIA_GTK_FONT_SELECTION_DIALOG_GET_CLASS(obj)    (GTK_CHECK_GET_CLASS ((obj), DIA_GTK_TYPE_FONT_SELECTION_DIALOG, DiaGtkFontSelectionDialogClass))


typedef struct _DiaGtkFontSelection	     DiaGtkFontSelection;
typedef struct _DiaGtkFontSelectionClass	     DiaGtkFontSelectionClass;

typedef struct _DiaGtkFontSelectionDialog	     DiaGtkFontSelectionDialog;
typedef struct _DiaGtkFontSelectionDialogClass  DiaGtkFontSelectionDialogClass;

struct _DiaGtkFontSelection
{
  GtkVBox parent_instance;
  
  GtkWidget *font_entry;
  GtkWidget *family_list;
  GtkWidget *font_style_entry;
  GtkWidget *face_list;
  GtkWidget *size_entry;
  GtkWidget *size_list;
  GtkWidget *pixels_button;
  GtkWidget *points_button;
  GtkWidget *filter_button;
  GtkWidget *preview_entry;

  const PangoContext *context;
  
  PangoFontFamily *family;	/* Current family */
  PangoFontFace *face;		/* Current face */
  
  gint size;
  };

struct _DiaGtkFontSelectionClass
{
  GtkVBoxClass parent_class;

  /* Padding for future expansion */
  void (*_gtk_reserved1) (void);
  void (*_gtk_reserved2) (void);
  void (*_gtk_reserved3) (void);
  void (*_gtk_reserved4) (void);
};


struct _DiaGtkFontSelectionDialog
{
  GtkDialog parent_instance;
  
  GtkWidget *fontsel;
  
  GtkWidget *main_vbox;
  GtkWidget *action_area;
  GtkWidget *ok_button;
  /* The 'Apply' button is not shown by default but you can show/hide it. */
  GtkWidget *apply_button;
  GtkWidget *cancel_button;
  
  /* If the user changes the width of the dialog, we turn auto-shrink off. */
  gint dialog_width;
  gboolean auto_resize;
};

struct _DiaGtkFontSelectionDialogClass
{
  GtkDialogClass parent_class;

  /* Padding for future expansion */
  void (*_gtk_reserved1) (void);
  void (*_gtk_reserved2) (void);
  void (*_gtk_reserved3) (void);
  void (*_gtk_reserved4) (void);
};



/*****************************************************************************
 * DiaGtkFontSelection functions.
 *   see the comments in the DiaGtkFontSelectionDialog functions.
 *****************************************************************************/

GtkType	   dia_gtk_font_selection_get_type		(void) G_GNUC_CONST;
GtkWidget* dia_gtk_font_selection_new		(void);
gchar*	   dia_gtk_font_selection_get_font_name	(DiaGtkFontSelection *fontsel);


gboolean              dia_gtk_font_selection_set_font_name    (DiaGtkFontSelection *fontsel,
							   const gchar      *fontname);
G_CONST_RETURN gchar* dia_gtk_font_selection_get_preview_text (DiaGtkFontSelection *fontsel);
void                  dia_gtk_font_selection_set_preview_text (DiaGtkFontSelection *fontsel,
							   const gchar      *text);
/* This sets the Pango context used for listing fonts */
void	 dia_gtk_font_selection_set_context (DiaGtkFontSelection *fs,
					     const PangoContext    *context);

/*****************************************************************************
 * DiaGtkFontSelectionDialog functions.
 *   most of these functions simply call the corresponding function in the
 *   DiaGtkFontSelection.
 *****************************************************************************/

GtkType	   dia_gtk_font_selection_dialog_get_type	(void) G_GNUC_CONST;
GtkWidget* dia_gtk_font_selection_dialog_new	(const gchar	  *title);

/* This returns the X Logical Font Description fontname, or NULL if no font
   is selected. Note that there is a slight possibility that the font might not
   have been loaded OK. You should call dia_gtk_font_selection_dialog_get_font()
   to see if it has been loaded OK.
   You should g_free() the returned font name after you're done with it. */
gchar*	 dia_gtk_font_selection_dialog_get_font_name    (DiaGtkFontSelectionDialog *fsd);

/* This sets the currently displayed font. It should be a valid X Logical
   Font Description font name (anything else will be ignored), e.g.
   "-adobe-courier-bold-o-normal--25-*-*-*-*-*-*-*" 
   It returns TRUE on success. */
gboolean dia_gtk_font_selection_dialog_set_font_name    (DiaGtkFontSelectionDialog *fsd,
						     const gchar	*fontname);

/* This returns the text in the preview entry. You should copy the returned
   text if you need it. */
G_CONST_RETURN gchar* dia_gtk_font_selection_dialog_get_preview_text (DiaGtkFontSelectionDialog *fsd);

/* This sets the text in the preview entry. It will be copied by the entry,
   so there's no need to g_strdup() it first. */
void	 dia_gtk_font_selection_dialog_set_preview_text (DiaGtkFontSelectionDialog *fsd,
						     const gchar	    *text);

/* This sets the Pango context used for listing fonts */
void	 dia_gtk_font_selection_dialog_set_context (DiaGtkFontSelectionDialog *fsd,
						    const PangoContext    *context);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __DIA_GTK_FONTSEL_H__ */
