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
#include <assert.h>
#include <gtk/gtk.h>
#include <math.h>

#include "object.h"
#include "connectionpoint.h"
#include "render.h"
#include "font.h"
#include "text.h"
#include "attributes.h"
#include "widgets.h"

#include "pixmaps/text.xpm"

#define HANDLE_TEXT HANDLE_CUSTOM1

typedef struct _Textobj Textobj;
typedef struct _TextobjPropertiesDialog TextobjPropertiesDialog;
typedef struct _TextobjDefaultsDialog TextobjDefaultsDialog;

struct _Textobj {
  Object object;
  
  Handle text_handle;

  Text *text;

  TextobjPropertiesDialog *properties_dialog;
};

typedef struct _TextobjProperties {
  real height;
  Font *font;
  Alignment alignment;
  Color color;
} TextobjProperties;

struct _TextobjPropertiesDialog {
  GtkWidget *vbox;
  
  DiaAlignmentSelector *alignment;
  DiaFontSelector *font;
  GtkSpinButton *font_size;
  DiaColorSelector *color;
};

struct _TextobjDefaultsDialog {
  GtkWidget *vbox;

  DiaAlignmentSelector *alignment;
  DiaFontSelector *font;
  GtkSpinButton *font_size;
};

static TextobjDefaultsDialog *textobj_defaults_dialog;
static TextobjProperties default_properties =
{ 1.0, NULL, ALIGN_CENTER }; /* Can't initialize the font here */

static real textobj_distance_from(Textobj *textobj, Point *point);
static void textobj_select(Textobj *textobj, Point *clicked_point,
			   Renderer *interactive_renderer);
static void textobj_move_handle(Textobj *textobj, Handle *handle,
				Point *to, HandleMoveReason reason);
static void textobj_move(Textobj *textobj, Point *to);
static void textobj_draw(Textobj *textobj, Renderer *renderer);
static void textobj_update_data(Textobj *textobj);
static Object *textobj_create(Point *startpoint,
			      void *user_data,
			      Handle **handle1,
			      Handle **handle2);
static void textobj_destroy(Textobj *textobj);
static Object *textobj_copy(Textobj *textobj);
static GtkWidget *textobj_get_properties(Textobj *textobj);
static void textobj_apply_properties(Textobj *textobj);

static void textobj_save(Textobj *textobj, ObjectNode obj_node);
static Object *textobj_load(ObjectNode obj_node, int version);
static int textobj_is_empty(Textobj *textobj);
static GtkWidget *textobj_get_defaults();
static void textobj_apply_defaults();

static ObjectTypeOps textobj_type_ops =
{
  (CreateFunc) textobj_create,
  (LoadFunc)   textobj_load,
  (SaveFunc)   textobj_save,
  (GetDefaultsFunc)   textobj_get_defaults,
  (ApplyDefaultsFunc) textobj_apply_defaults
};

ObjectType textobj_type =
{
  "Standard - Text",   /* name */
  0,                   /* version */
  (char **) text_xpm,  /* pixmap */

  &textobj_type_ops    /* ops */
};

ObjectType *_textobj_type = (ObjectType *) &textobj_type;

static ObjectOps textobj_ops = {
  (DestroyFunc)         textobj_destroy,
  (DrawFunc)            textobj_draw,
  (DistanceFunc)        textobj_distance_from,
  (SelectFunc)          textobj_select,
  (CopyFunc)            textobj_copy,
  (MoveFunc)            textobj_move,
  (MoveHandleFunc)      textobj_move_handle,
  (GetPropertiesFunc)   textobj_get_properties,
  (ApplyPropertiesFunc) textobj_apply_properties,
  (IsEmptyFunc)         textobj_is_empty
};

static void
textobj_apply_properties(Textobj *textobj)
{
  TextobjPropertiesDialog *prop_dialog;
  Alignment align;
  Font *font;
  real font_size;
  Color col;
  
  prop_dialog = textobj->properties_dialog;

  align = dia_alignment_selector_get_alignment(prop_dialog->alignment);
  text_set_alignment(textobj->text, align);

  font = dia_font_selector_get_font(prop_dialog->font);
  text_set_font(textobj->text, font);

  font_size = gtk_spin_button_get_value_as_float(prop_dialog->font_size);;
  text_set_height(textobj->text, font_size);

  dia_color_selector_get_color(prop_dialog->color, &col);
  text_set_color(textobj->text, &col);
  
  textobj_update_data(textobj);
}

static GtkWidget *
textobj_get_properties(Textobj *textobj)
{
  TextobjPropertiesDialog *prop_dialog;
  GtkWidget *alignment;
  GtkWidget *font;
  GtkWidget *font_size;
  GtkWidget *color;
  GtkWidget *hbox;
  GtkWidget *vbox;
  GtkWidget *label;
  GtkAdjustment *adj;

  if (textobj->properties_dialog == NULL) {
    prop_dialog = g_new(TextobjPropertiesDialog, 1);
    textobj->properties_dialog = prop_dialog;
    
    vbox = gtk_vbox_new(FALSE, 5);
    prop_dialog->vbox = vbox;

    hbox = gtk_hbox_new(FALSE, 5);
    label = gtk_label_new("Alignment:");
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    gtk_widget_show (label);
    alignment = dia_alignment_selector_new();
    prop_dialog->alignment = DIAALIGNMENTSELECTOR(alignment);
    gtk_box_pack_start (GTK_BOX (hbox), alignment, TRUE, TRUE, 0);
    gtk_widget_show (alignment);
    gtk_widget_show(hbox);
    gtk_box_pack_start (GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

    hbox = gtk_hbox_new(FALSE, 5);
    label = gtk_label_new("Font:");
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    gtk_widget_show (label);
    font = dia_font_selector_new();
    prop_dialog->font = DIAFONTSELECTOR(font);
    gtk_box_pack_start (GTK_BOX (hbox), font, TRUE, TRUE, 0);
    gtk_widget_show (font);
    gtk_widget_show(hbox);
    gtk_box_pack_start (GTK_BOX(vbox), hbox, TRUE, TRUE, 0);
    
    hbox = gtk_hbox_new(FALSE, 5);
    label = gtk_label_new("Fontsize:");
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    gtk_widget_show (label);
    adj = (GtkAdjustment *) gtk_adjustment_new(0.1, 0.1, 10.0, 0.1, 0.0, 0.0);
    font_size = gtk_spin_button_new(adj, 1.0, 2);
    gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(font_size), TRUE);
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(font_size), TRUE);
    prop_dialog->font_size = GTK_SPIN_BUTTON(font_size);
    gtk_box_pack_start(GTK_BOX (hbox), font_size, TRUE, TRUE, 0);
    gtk_widget_show (font_size);
    gtk_widget_show(hbox);
    gtk_box_pack_start (GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

    hbox = gtk_hbox_new(FALSE, 5);
    label = gtk_label_new("Color:");
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    gtk_widget_show (label);
    color = dia_color_selector_new();
    prop_dialog->color = DIACOLORSELECTOR(color);
    gtk_box_pack_start (GTK_BOX (hbox), color, TRUE, TRUE, 0);
    gtk_widget_show (color);
    gtk_widget_show(hbox);
    gtk_box_pack_start (GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

    gtk_widget_show (vbox);
  }
  
  prop_dialog = textobj->properties_dialog;
    
  dia_alignment_selector_set_alignment(prop_dialog->alignment, textobj->text->alignment);
  dia_font_selector_set_font(prop_dialog->font, textobj->text->font);
  gtk_spin_button_set_value(prop_dialog->font_size, textobj->text->height);
  dia_color_selector_set_color(prop_dialog->color, &textobj->text->color);
  
  return textobj->properties_dialog->vbox;
}

static void
textobj_apply_defaults()
{
  default_properties.alignment = dia_alignment_selector_get_alignment(textobj_defaults_dialog->alignment);

  default_properties.font = dia_font_selector_get_font(textobj_defaults_dialog->font);

  default_properties.height = gtk_spin_button_get_value_as_float(textobj_defaults_dialog->font_size);
}

void
init_defaults()
{
  if (default_properties.font == NULL)
    default_properties.font = font_getfont("Courier");
}

static GtkWidget *
textobj_get_defaults()
{
  GtkWidget *alignment;
  GtkWidget *font;
  GtkWidget *font_size;
  GtkWidget *hbox;
  GtkWidget *vbox;
  GtkWidget *label;
  GtkAdjustment *adj;

  if (textobj_defaults_dialog == NULL) {
    textobj_defaults_dialog = g_new(TextobjDefaultsDialog, 1);
    
    vbox = gtk_vbox_new(FALSE, 5);
    textobj_defaults_dialog->vbox = vbox;

    hbox = gtk_hbox_new(FALSE, 5);
    label = gtk_label_new("Alignment:");
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    gtk_widget_show (label);
    alignment = dia_alignment_selector_new();
    textobj_defaults_dialog->alignment = DIAALIGNMENTSELECTOR(alignment);
    gtk_box_pack_start (GTK_BOX (hbox), alignment, TRUE, TRUE, 0);
    gtk_widget_show (alignment);
    gtk_widget_show(hbox);
    gtk_box_pack_start (GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

    hbox = gtk_hbox_new(FALSE, 5);
    label = gtk_label_new("Font:");
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    gtk_widget_show (label);
    font = dia_font_selector_new();
    textobj_defaults_dialog->font = DIAFONTSELECTOR(font);
    gtk_box_pack_start (GTK_BOX (hbox), font, TRUE, TRUE, 0);
    gtk_widget_show (font);
    gtk_widget_show(hbox);
    gtk_box_pack_start (GTK_BOX(vbox), hbox, TRUE, TRUE, 0);
    
    hbox = gtk_hbox_new(FALSE, 5);
    label = gtk_label_new("Fontsize:");
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    gtk_widget_show (label);
    adj = (GtkAdjustment *) gtk_adjustment_new(0.1, 0.1, 10.0, 0.1, 0.0, 0.0);
    font_size = gtk_spin_button_new(adj, 1.0, 2);
    gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(font_size), TRUE);
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(font_size), TRUE);
    textobj_defaults_dialog->font_size = GTK_SPIN_BUTTON(font_size);
    gtk_box_pack_start(GTK_BOX (hbox), font_size, TRUE, TRUE, 0);
    gtk_widget_show (font_size);
    gtk_widget_show(hbox);
    gtk_box_pack_start (GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

    gtk_widget_show (vbox);
  }

  init_defaults();
  
  dia_alignment_selector_set_alignment(textobj_defaults_dialog->alignment, default_properties.alignment);
  dia_font_selector_set_font(textobj_defaults_dialog->font, default_properties.font);
  gtk_spin_button_set_value(textobj_defaults_dialog->font_size, default_properties.height);
  
  return textobj_defaults_dialog->vbox;
}

static real
textobj_distance_from(Textobj *textobj, Point *point)
{
  return text_distance_from(textobj->text, point); 
}

static void
textobj_select(Textobj *textobj, Point *clicked_point,
	       Renderer *interactive_renderer)
{
  text_set_cursor(textobj->text, clicked_point, interactive_renderer);
  text_grab_focus(textobj->text, (Object *)textobj);
}

static void
textobj_move_handle(Textobj *textobj, Handle *handle,
		    Point *to, HandleMoveReason reason)
{
  assert(textobj!=NULL);
  assert(handle!=NULL);
  assert(to!=NULL);

  if (handle->id == HANDLE_TEXT) {
    text_set_position(textobj->text, to);
  }
  
  textobj_update_data(textobj);
}

static void
textobj_move(Textobj *textobj, Point *to)
{
  text_set_position(textobj->text, to);
  
  textobj_update_data(textobj);
}

static void
textobj_draw(Textobj *textobj, Renderer *renderer)
{
  assert(textobj != NULL);
  assert(renderer != NULL);

  text_draw(textobj->text, renderer);
}

static void
textobj_update_data(Textobj *textobj)
{
  Object *obj = &textobj->object;
  
  obj->position = textobj->text->position;
  
  text_calc_boundingbox(textobj->text, &obj->bounding_box);
  
  textobj->text_handle.pos = textobj->text->position;
}

static int
textobj_is_empty(Textobj *textobj)
{
  return text_is_empty(textobj->text);
}

static Object *
textobj_create(Point *startpoint,
	       void *user_data,
	       Handle **handle1,
	       Handle **handle2)
{
  Textobj *textobj;
  Object *obj;
  Color col;
  
  textobj = g_malloc(sizeof(Textobj));
  obj = &textobj->object;
  
  obj->type = &textobj_type;

  obj->ops = &textobj_ops;

  init_defaults();

  col = attributes_get_foreground();
  textobj->text = new_text("", default_properties.font, default_properties.height,
			   startpoint, &col, default_properties.alignment );
  
  textobj->properties_dialog = NULL;
  
  object_init(obj, 1, 0);

  obj->handles[0] = &textobj->text_handle;
  textobj->text_handle.id = HANDLE_TEXT;
  textobj->text_handle.type = HANDLE_MAJOR_CONTROL;
  textobj->text_handle.connect_type = HANDLE_CONNECTABLE;
  textobj->text_handle.connected_to = NULL;

  textobj_update_data(textobj);

  *handle1 = NULL;
  *handle2 = obj->handles[0];
  return (Object *)textobj;
}

static void
textobj_destroy(Textobj *textobj)
{
  if (textobj->properties_dialog != NULL) {
    gtk_widget_destroy(textobj->properties_dialog->vbox);
    g_free(textobj->properties_dialog);
  }
  text_destroy(textobj->text);
  object_destroy(&textobj->object);
}

static Object *
textobj_copy(Textobj *textobj)
{
  Textobj *newtext;
  Object *obj, *newobj;
  
  obj = &textobj->object;
  
  newtext = g_malloc(sizeof(Textobj));
  newobj = &newtext->object;

  object_copy(obj, newobj);

  newtext->text = text_copy(textobj->text);

  newobj->handles[0] = &newtext->text_handle;
  
  newtext->text_handle = textobj->text_handle;
  newtext->text_handle.connected_to = NULL;

  newtext->properties_dialog = NULL;

  return (Object *)newtext;
}

static void
textobj_save(Textobj *textobj, ObjectNode obj_node)
{
  object_save(&textobj->object, obj_node);

  data_add_text(new_attribute(obj_node, "text"),
		textobj->text);
}

static Object *
textobj_load(ObjectNode obj_node, int version)
{
  Textobj *textobj;
  Object *obj;
  AttributeNode attr;
  Point startpoint = {0.0, 0.0};

  textobj = g_malloc(sizeof(Textobj));
  obj = &textobj->object;
  
  obj->type = &textobj_type;
  obj->ops = &textobj_ops;

  object_load(obj, obj_node);

  attr = object_find_attribute(obj_node, "text");
  if (attr != NULL)
    textobj->text = data_text( attribute_first_data(attr) );
  else
    textobj->text = new_text("", font_getfont("Courier"), 1.0,
			     &startpoint, &color_black, ALIGN_CENTER);

  textobj->properties_dialog = NULL;
  
  object_init(obj, 1, 0);

  obj->handles[0] = &textobj->text_handle;
  textobj->text_handle.id = HANDLE_TEXT;
  textobj->text_handle.type = HANDLE_MINOR_CONTROL;
  textobj->text_handle.connect_type = HANDLE_CONNECTABLE;
  textobj->text_handle.connected_to = NULL;

  textobj_update_data(textobj);

  return (Object *)textobj;
}
