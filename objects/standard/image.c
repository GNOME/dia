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
#include <string.h>
#include <gtk/gtk.h>
#include <math.h>
#include <unistd.h>

#include "config.h"
#include "intl.h"
#include "message.h"
#include "object.h"
#include "element.h"
#include "connectionpoint.h"
#include "render.h"
#include "attributes.h"
#include "widgets.h"
#include "dia_image.h"
#include "message.h"

#include "pixmaps/image.xpm"

#define DEFAULT_WIDTH 2.0
#define DEFAULT_HEIGHT 2.0

typedef struct _Image Image;
typedef struct _ImagePropertiesDialog ImagePropertiesDialog;
typedef struct _ImageDefaultsDialog ImageDefaultsDialog;
typedef struct _ImageState ImageState;

struct _ImageState {
  ObjectState obj_state;
  
  real border_width;
  Color border_color;
  LineStyle line_style;

  DiaImage image;
  gchar *file;
  gboolean draw_border;
  gboolean keep_aspect;
};

struct _Image {
  Element element;

  ConnectionPoint connections[8];

  real border_width;
  Color border_color;
  LineStyle line_style;
  
  DiaImage image;
  gchar *file;
  gboolean draw_border;
  gboolean keep_aspect;
};

typedef struct _ImageProperties {
  Color *fg_color;
  real border_width;
  LineStyle line_style;

  gchar *file;

  gboolean draw_border;
  gboolean keep_aspect;
} ImageProperties;

struct _ImagePropertiesDialog {
  GtkWidget *vbox;

  GtkSpinButton *border_width;
  DiaColorSelector *fg_color;
  DiaLineStyleSelector *line_style;

  DiaFileSelector *file;

  GtkToggleButton *draw_border;
  GtkToggleButton *keep_aspect;

  Image *image;
};

struct _ImageDefaultsDialog {
  GtkWidget *vbox;

  DiaLineStyleSelector *line_style;

  DiaFileSelector *file;

  GtkToggleButton *draw_border;
  GtkToggleButton *keep_aspect;
};

static ImagePropertiesDialog *image_properties_dialog;
static ImageDefaultsDialog *image_defaults_dialog;
static ImageProperties default_properties;

static real image_distance_from(Image *image, Point *point);
static void image_select(Image *image, Point *clicked_point,
		       Renderer *interactive_renderer);
static void image_move_handle(Image *image, Handle *handle,
			    Point *to, HandleMoveReason reason, ModifierKeys modifiers);
static void image_move(Image *image, Point *to);
static void image_draw(Image *image, Renderer *renderer);
static void image_update_data(Image *image);
static Object *image_create(Point *startpoint,
			  void *user_data,
			  Handle **handle1,
			  Handle **handle2);
static void image_destroy(Image *image);
static Object *image_copy(Image *image);
static GtkWidget *image_get_properties(Image *image);
static void image_apply_properties(Image *image);

static ImageState *image_get_state(Image *image);
static void image_set_state(Image *image, ImageState *state);

static void image_save(Image *image, ObjectNode obj_node, const char *filename);
static Object *image_load(ObjectNode obj_node, int version, const char *filename);
static GtkWidget *image_get_defaults();
static void image_apply_defaults();

static ObjectTypeOps image_type_ops =
{
  (CreateFunc) image_create,
  (LoadFunc)   image_load,
  (SaveFunc)   image_save,
  (GetDefaultsFunc)   image_get_defaults,
  (ApplyDefaultsFunc) image_apply_defaults
};

ObjectType image_type =
{
  "Standard - Image",  /* name */
  0,                 /* version */
  (char **) image_xpm, /* pixmap */

  &image_type_ops      /* ops */
};

ObjectType *_image_type = (ObjectType *) &image_type;

static ObjectOps image_ops = {
  (DestroyFunc)         image_destroy,
  (DrawFunc)            image_draw,
  (DistanceFunc)        image_distance_from,
  (SelectFunc)          image_select,
  (CopyFunc)            image_copy,
  (MoveFunc)            image_move,
  (MoveHandleFunc)      image_move_handle,
  (GetPropertiesFunc)   image_get_properties,
  (ApplyPropertiesFunc) image_apply_properties,
  (IsEmptyFunc)         object_return_false,
  (ObjectMenuFunc)      NULL,
  (GetStateFunc)        image_get_state,
  (SetStateFunc)        image_set_state
};

static void
image_apply_properties(Image *image)
{
  gchar *new_file;

  if (image != image_properties_dialog->image) {
    message_warning("Image dialog problem:  %p != %p\n", 
		    image, image_properties_dialog->image);
    image = image_properties_dialog->image;
  }

  image->border_width = gtk_spin_button_get_value_as_float(image_properties_dialog->border_width);
  dia_color_selector_get_color(image_properties_dialog->fg_color, &image->border_color);
  dia_line_style_selector_get_linestyle(image_properties_dialog->line_style, &image->line_style, NULL);

  image->draw_border = gtk_toggle_button_get_active(image_properties_dialog->draw_border);
  image->keep_aspect = gtk_toggle_button_get_active(image_properties_dialog->keep_aspect);
  
  new_file = dia_file_selector_get_file(image_properties_dialog->file);
  if (image->file) g_free(image->file);
  if (image->image) {
    dia_image_release(image->image);
  }
  image->image = dia_image_load(new_file);
  if ((image->image != NULL) && (image->keep_aspect)) {
    /* Must... keep... aspect... ratio... */
    float ratio = (float)dia_image_height(image->image)/
      (float)dia_image_width(image->image);
    image->element.height = image->element.width * ratio;
  }
  image->file = g_strdup(new_file);

  image_update_data(image);
}

static GtkWidget *
image_get_properties(Image *image)
{
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *color;
  GtkWidget *checkbox;
  GtkWidget *linestyle;
  GtkWidget *file;
  GtkWidget *border_width;
  GtkAdjustment *adj;

  if (image_properties_dialog == NULL) {
  
    image_properties_dialog = g_new(ImagePropertiesDialog, 1);
  
    vbox = gtk_vbox_new(FALSE, 5);
    image_properties_dialog->vbox = vbox;

    hbox = gtk_hbox_new(FALSE, 5);
    label = gtk_label_new(_("Image file:"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    gtk_widget_show (label);
    file = dia_file_selector_new();
    image_properties_dialog->file = DIAFILESELECTOR(file);
    gtk_box_pack_start (GTK_BOX (hbox), file, TRUE, TRUE, 0);
    gtk_widget_show (file);
    gtk_widget_show(hbox);
    gtk_box_pack_start (GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

    hbox = gtk_hbox_new(FALSE, 5);
    checkbox = gtk_check_button_new_with_label(_("Keep aspect ratio"));
    image_properties_dialog->keep_aspect = GTK_TOGGLE_BUTTON( checkbox );
    gtk_widget_show(checkbox);
    gtk_widget_show(hbox);
    gtk_box_pack_start (GTK_BOX (hbox), checkbox, TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);

    hbox = gtk_hbox_new(FALSE, 5);
    checkbox = gtk_check_button_new_with_label(_("Show border"));
    image_properties_dialog->draw_border = GTK_TOGGLE_BUTTON( checkbox );
    gtk_widget_show(checkbox);
    gtk_widget_show(hbox);
    gtk_box_pack_start (GTK_BOX (hbox), checkbox, TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);

    hbox = gtk_hbox_new(FALSE, 5);
    label = gtk_label_new(_("Border width:"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    gtk_widget_show (label);
    adj = (GtkAdjustment *) gtk_adjustment_new(0.1, 0.00, 10.0, 0.01, 0.0, 0.0);
    border_width = gtk_spin_button_new(adj, 1.0, 2);
    gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(border_width), TRUE);
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(border_width), TRUE);
    image_properties_dialog->border_width = GTK_SPIN_BUTTON(border_width);
    gtk_box_pack_start(GTK_BOX (hbox), border_width, TRUE, TRUE, 0);
    gtk_widget_show (border_width);
    gtk_widget_show(hbox);
    gtk_box_pack_start (GTK_BOX(vbox), hbox, TRUE, TRUE, 0);


    hbox = gtk_hbox_new(FALSE, 5);
    label = gtk_label_new(_("Foreground color:"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    gtk_widget_show (label);
    color = dia_color_selector_new();
    image_properties_dialog->fg_color = DIACOLORSELECTOR(color);
    gtk_box_pack_start (GTK_BOX (hbox), color, TRUE, TRUE, 0);
    gtk_widget_show (color);
    gtk_widget_show(hbox);
    gtk_box_pack_start (GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

    hbox = gtk_hbox_new(FALSE, 5);
    label = gtk_label_new(_("Line style:"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    gtk_widget_show (label);
    linestyle = dia_line_style_selector_new();
    image_properties_dialog->line_style = DIALINESTYLESELECTOR(linestyle);
    gtk_box_pack_start (GTK_BOX (hbox), linestyle, TRUE, TRUE, 0);
    gtk_widget_show (linestyle);
    gtk_widget_show(hbox);
    gtk_box_pack_start (GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

    gtk_widget_show (vbox);
  }

  image_properties_dialog->image = image;
    
  gtk_spin_button_set_value(image_properties_dialog->border_width, 
			    image->border_width);
  dia_color_selector_set_color(image_properties_dialog->fg_color, 
			       &image->border_color);
  dia_file_selector_set_file(image_properties_dialog->file, image->file);
  dia_line_style_selector_set_linestyle(image_properties_dialog->line_style,
					image->line_style, 1.0);
  gtk_toggle_button_set_active(image_properties_dialog->draw_border, image->draw_border);
  gtk_toggle_button_set_active(image_properties_dialog->keep_aspect, image->keep_aspect);

  return image_properties_dialog->vbox;
}

static void
image_init_defaults() {
  static int defaults_initialized = 0;

  if (defaults_initialized) return;

  /* Just for testers */
  default_properties.file = "";
  default_properties.keep_aspect = 1;
  defaults_initialized = 1;
}

static void
image_apply_defaults()
{
  dia_line_style_selector_get_linestyle(image_defaults_dialog->line_style, &default_properties.line_style, NULL);
  default_properties.file = dia_file_selector_get_file(image_defaults_dialog->file);
  default_properties.draw_border = gtk_toggle_button_get_active(image_defaults_dialog->draw_border);
  default_properties.keep_aspect = gtk_toggle_button_get_active(image_defaults_dialog->keep_aspect);
}

static GtkWidget *
image_get_defaults()
{
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *file;
  GtkWidget *linestyle;
  GtkWidget *checkbox;

  if (image_defaults_dialog == NULL) {
    image_init_defaults();

    image_defaults_dialog = g_new(ImageDefaultsDialog, 1);

    vbox = gtk_vbox_new(FALSE, 5);
    image_defaults_dialog->vbox = vbox;

    hbox = gtk_hbox_new(FALSE, 5);
    label = gtk_label_new(_("Image file:"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    gtk_widget_show (label);
    file = dia_file_selector_new();
    image_defaults_dialog->file = DIAFILESELECTOR(file);
    gtk_box_pack_start (GTK_BOX (hbox), file, TRUE, TRUE, 0);
    gtk_widget_show (file);
    gtk_widget_show(hbox);
    gtk_box_pack_start (GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

    hbox = gtk_hbox_new(FALSE, 5);
    checkbox = gtk_check_button_new_with_label(_("Keep aspect ratio:"));
    image_defaults_dialog->keep_aspect = GTK_TOGGLE_BUTTON( checkbox );
    gtk_widget_show(checkbox);
    gtk_widget_show(hbox);
    gtk_box_pack_start (GTK_BOX (hbox), checkbox, TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);

    hbox = gtk_hbox_new(FALSE, 5);
    checkbox = gtk_check_button_new_with_label(_("Show border:"));
    image_defaults_dialog->draw_border = GTK_TOGGLE_BUTTON( checkbox );
    gtk_widget_show(checkbox);
    gtk_widget_show(hbox);
    gtk_box_pack_start (GTK_BOX (hbox), checkbox, TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);

    hbox = gtk_hbox_new(FALSE, 5);
    label = gtk_label_new(_("Line style:"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    gtk_widget_show (label);
    linestyle = dia_line_style_selector_new();
    image_defaults_dialog->line_style = DIALINESTYLESELECTOR(linestyle);
    gtk_box_pack_start (GTK_BOX (hbox), linestyle, TRUE, TRUE, 0);
    gtk_widget_show (linestyle);
    gtk_widget_show(hbox);
    gtk_box_pack_start (GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

    gtk_widget_show (vbox);
  }

  dia_line_style_selector_set_linestyle(image_defaults_dialog->line_style,
					default_properties.line_style, 1.0);
  dia_file_selector_set_file(image_defaults_dialog->file, default_properties.file);
  gtk_toggle_button_set_active(image_defaults_dialog->draw_border, 
			       default_properties.draw_border);
  gtk_toggle_button_set_active(image_defaults_dialog->keep_aspect, 
			       default_properties.keep_aspect);

  return image_defaults_dialog->vbox;
}

static real
image_distance_from(Image *image, Point *point)
{
  Element *elem = (Element *)image;
  Rectangle rect;

  rect.left = elem->corner.x - image->border_width;
  rect.right = elem->corner.x + elem->width + image->border_width;
  rect.top = elem->corner.y - image->border_width;
  rect.bottom = elem->corner.y + elem->height + image->border_width;
  return distance_rectangle_point(&rect, point);
}

static void
image_select(Image *image, Point *clicked_point,
	     Renderer *interactive_renderer)
{
  element_update_handles(&image->element);
}

static void
image_move_handle(Image *image, Handle *handle,
		  Point *to, HandleMoveReason reason, ModifierKeys modifiers)
{
  assert(image!=NULL);
  assert(handle!=NULL);
  assert(to!=NULL);

  if (image->keep_aspect) {
    float width, height;
    float new_width, new_height;
    width = image->element.width;
    height = image->element.height;

    switch (handle->id) {
    case HANDLE_RESIZE_NW:
      new_width = -(to->x-image->element.corner.x)+width;
      new_height = -(to->y-image->element.corner.y)+height;
      if (new_width/new_height > width/height) {
	new_height = new_width*height/width;
      } else {
	new_width = new_height*width/height;
      }
      to->x = image->element.corner.x+(image->element.width-new_width);
      to->y = image->element.corner.y+(image->element.height-new_height);
      element_move_handle((Element *)image, HANDLE_RESIZE_NW, to, reason);
      break;
    case HANDLE_RESIZE_N:
      new_width = (-(to->y-image->element.corner.y)+height)*width/height;
      to->x = image->element.corner.x+new_width;
      element_move_handle((Element *)image, HANDLE_RESIZE_NE, to, reason);
      break;
    case HANDLE_RESIZE_NE:
      new_width = to->x-image->element.corner.x;
      new_height = -(to->y-image->element.corner.y)+height;
      if (new_width/new_height > width/height) {
	new_height = new_width*height/width;
      } else {
	new_width = new_height*width/height;
      }
      to->x = image->element.corner.x+new_width;
      to->y = image->element.corner.y+(image->element.height-new_height);
      element_move_handle((Element *)image, HANDLE_RESIZE_NE, to, reason);
      break;
    case HANDLE_RESIZE_E:
      new_height = (to->x-image->element.corner.x)*height/width;
      to->y = image->element.corner.y+new_height;
      element_move_handle((Element *)image, HANDLE_RESIZE_SE, to, reason);
      break;
    case HANDLE_RESIZE_SE:
      new_width = to->x-image->element.corner.x;
      new_height = to->y-image->element.corner.y;
      if (new_width/new_height > width/height) {
	new_height = new_width*height/width;
      } else {
	new_width = new_height*width/height;
      }
      to->x = image->element.corner.x+new_width;
      to->y = image->element.corner.y+new_height;
      element_move_handle((Element *)image, HANDLE_RESIZE_SE, to, reason);
      break;
    case HANDLE_RESIZE_S:
      new_width = (to->y-image->element.corner.y)*width/height;
      to->x = image->element.corner.x+new_width;
      element_move_handle((Element *)image, HANDLE_RESIZE_SE, to, reason);
      break;
    case HANDLE_RESIZE_SW:
      new_width = -(to->x-image->element.corner.x)+width;
      new_height = to->y-image->element.corner.y;
      if (new_width/new_height > width/height) {
	new_height = new_width*height/width;
      } else {
	new_width = new_height*width/height;
      }
      to->x = image->element.corner.x+(image->element.width-new_width);
      to->y = image->element.corner.y+new_height;
      element_move_handle((Element *)image, HANDLE_RESIZE_SW, to, reason);
      break;
    case HANDLE_RESIZE_W:
      new_height = (-(to->x-image->element.corner.x)+width)*height/width;
      to->y = image->element.corner.y+new_height;
      element_move_handle((Element *)image, HANDLE_RESIZE_SW, to, reason);
      break;
    default:
      message_warning("Unforeseen handle in image_move_handle: %d\n",
		      handle->id);
      
    }
  } else {
    element_move_handle((Element *)image, handle->id, to, reason);
  }
  image_update_data(image);
}

static void
image_move(Image *image, Point *to)
{
  image->element.corner = *to;
  
  image_update_data(image);
}

static void
image_draw(Image *image, Renderer *renderer)
{
  Point ul_corner, lr_corner;
  Element *elem;
  
  assert(image != NULL);
  assert(renderer != NULL);

  elem = &image->element;
  
  lr_corner.x = elem->corner.x + elem->width + image->border_width/2;
  lr_corner.y = elem->corner.y + elem->height + image->border_width/2;
  
  ul_corner.x = elem->corner.x - image->border_width/2;
  ul_corner.y = elem->corner.y - image->border_width/2;

  if (image->draw_border) {
    renderer->ops->set_linewidth(renderer, image->border_width);
    renderer->ops->set_linestyle(renderer, image->line_style);
    renderer->ops->set_linejoin(renderer, LINEJOIN_MITER);
    
    renderer->ops->draw_rect(renderer, 
			     &ul_corner,
			     &lr_corner, 
			     &image->border_color);
  }
  /* Draw the image */
  if (image->image) {
    renderer->ops->draw_image(renderer, &elem->corner, elem->width,
			      elem->height, image->image);
  } else {
    DiaImage broken = dia_image_get_broken();
    renderer->ops->draw_image(renderer, &elem->corner, elem->width,
			      elem->height, broken);
  }
}

static void
image_state_free(ObjectState *st)
{
  ImageState *state = (ImageState *)st;
  if (state->image)
    dia_image_release(state->image);
  if (state->file)
    g_free(state->file);
}

static ImageState *
image_get_state(Image *image)
{
  ImageState *state = g_new(ImageState, 1);

  state->obj_state.free = image_state_free;
  
  state->border_width = image->border_width;
  state->border_color = image->border_color;
  state->line_style = image->line_style;

  state->image = image->image;
  if (state->image)
    dia_image_add_ref(state->image);
  state->file = NULL;
  if (image->file)
    state->file = g_strdup(image->file);

  state->draw_border = image->draw_border;
  state->keep_aspect = image->keep_aspect;

  return state;
}

static void
image_set_state(Image *image, ImageState *state)
{
  image->border_width = state->border_width;
  image->border_color = state->border_color;
  image->line_style = state->line_style;

  if (image->image)
    dia_image_release(image->image);
  
  state->image = image->image;

  if (image->file)
    g_free(image->file);
  image->file = state->file;

  image->draw_border = state->draw_border;
  image->keep_aspect = state->keep_aspect;

  g_free(state);
  
  image_update_data(image);
}

static void
image_update_data(Image *image)
{
  Element *elem = (Element *)image;
  Object *obj = (Object *)image;

  /* Update connections: */
  image->connections[0].pos = elem->corner;
  image->connections[1].pos.x = elem->corner.x + elem->width / 2.0;
  image->connections[1].pos.y = elem->corner.y;
  image->connections[2].pos.x = elem->corner.x + elem->width;
  image->connections[2].pos.y = elem->corner.y;
  image->connections[3].pos.x = elem->corner.x;
  image->connections[3].pos.y = elem->corner.y + elem->height / 2.0;
  image->connections[4].pos.x = elem->corner.x + elem->width;
  image->connections[4].pos.y = elem->corner.y + elem->height / 2.0;
  image->connections[5].pos.x = elem->corner.x;
  image->connections[5].pos.y = elem->corner.y + elem->height;
  image->connections[6].pos.x = elem->corner.x + elem->width / 2.0;
  image->connections[6].pos.y = elem->corner.y + elem->height;
  image->connections[7].pos.x = elem->corner.x + elem->width;
  image->connections[7].pos.y = elem->corner.y + elem->height;

  element_update_boundingbox(elem);
  /* fix boundingbox for line_width: */
  obj->bounding_box.top -= image->border_width;
  obj->bounding_box.left -= image->border_width;
  obj->bounding_box.bottom += image->border_width;
  obj->bounding_box.right += image->border_width;
  
  obj->position = elem->corner;
  
  element_update_handles(elem);
}

static Object *
image_create(Point *startpoint,
	     void *user_data,
	     Handle **handle1,
	     Handle **handle2)
{
  Image *image;
  Element *elem;
  Object *obj;
  int i;

  image = g_malloc(sizeof(Image));
  elem = (Element *) image;
  obj = (Object *) image;
  
  obj->type = &image_type;

  obj->ops = &image_ops;

  elem->corner = *startpoint;
  elem->width = DEFAULT_WIDTH;
  elem->height = DEFAULT_WIDTH;

  image_init_defaults();
    
  image->border_width =  attributes_get_default_linewidth();
  image->border_color = attributes_get_foreground();
  image->line_style = default_properties.line_style;
  
  element_init(elem, 8, 8);

  for (i=0;i<8;i++) {
    obj->connections[i] = &image->connections[i];
    image->connections[i].object = obj;
    image->connections[i].connected = NULL;
  }

  if (strcmp(default_properties.file, "")) {
    image->file = g_strdup(default_properties.file);
    image->image = dia_image_load(image->file);

    if (image->image) {
      elem->width = (elem->width*(float)dia_image_width(image->image))/
	(float)dia_image_height(image->image);
    }
  } else {
    image->file = g_strdup("");
    image->image = NULL;
  }

  image->draw_border = default_properties.draw_border;
  image->keep_aspect = default_properties.keep_aspect;

  image_update_data(image);
  
  *handle1 = NULL;
  *handle2 = obj->handles[7];  
  return (Object *)image;
}

static void 
image_destroy(Image *image) {
  if (image->file != NULL)
    g_free(image->file);

  if (image->image != NULL)
    dia_image_release(image->image);

  element_destroy(&image->element);
}

static Object *
image_copy(Image *image)
{
  int i;
  Image *newimage;
  Element *elem, *newelem;
  Object *newobj;
  
  elem = (Element *)image;
  
  newimage = g_malloc(sizeof(Image));
  newelem = (Element *)newimage;
  newobj = (Object *) newimage;

  element_copy(elem, newelem);

  newimage->border_width = image->border_width;
  newimage->border_color = image->border_color;
  newimage->line_style = image->line_style;
  
  for (i=0;i<8;i++) {
    newobj->connections[i] = &newimage->connections[i];
    newimage->connections[i].object = newobj;
    newimage->connections[i].connected = NULL;
    newimage->connections[i].pos = image->connections[i].pos;
    newimage->connections[i].last_pos = image->connections[i].last_pos;
  }

  newimage->file = g_strdup(image->file);
  if (strcmp(image->file, "")) {
    newimage->image = dia_image_load(image->file);
  }

  newimage->draw_border = image->draw_border;
  newimage->keep_aspect = image->keep_aspect;

  return (Object *)newimage;
}

/* Gets the directory path of a filename.
   Uses current working directory if filename is a relative pathname.
   Examples:
     /some/dir/file.gif => /some/dir/
     dir/file.gif => /cwd/dir/
   
*/
static char *
get_directory(const char *filename) 
{
  char *cwd;
  char *directory;
  char *end;
  int len;
  
  if (filename==NULL)
    return NULL;

  if (g_path_is_absolute(filename)) {
    end = strrchr(filename, G_DIR_SEPARATOR);
    len = end - filename + 1;
    directory = g_malloc((len+1)*sizeof(char));
    memcpy(directory, filename, len);
    directory[len] = 0; /* zero terminate string */
  } else {
    cwd = g_get_current_dir();
    len = strlen(cwd)+strlen(filename)+1;
    directory = g_malloc(len*sizeof(char));
    strncpy(directory, cwd, len);
    strncat(directory, "/", len);
    strncat(directory, filename, len);
    end = strrchr(directory, G_DIR_SEPARATOR);
    if (end!=NULL)
      *(end+1) = 0;
  }

  return directory;
}

static void
image_save(Image *image, ObjectNode obj_node, const char *filename)
{
  char *diafile_dir;
  
  element_save(&image->element, obj_node);

  if (image->border_width != 0.1)
    data_add_real(new_attribute(obj_node, "border_width"),
		  image->border_width);
  
  if (!color_equals(&image->border_color, &color_black))
    data_add_color(new_attribute(obj_node, "border_color"),
		   &image->border_color);
  
  if (image->line_style != LINESTYLE_SOLID)
    data_add_enum(new_attribute(obj_node, "line_style"),
		  image->line_style);
  
  data_add_boolean(new_attribute(obj_node, "draw_border"), image->draw_border);
  data_add_boolean(new_attribute(obj_node, "keep_aspect"), image->keep_aspect);

  if (image->file != NULL) {
    if (g_path_is_absolute(image->file)) { /* Absolute pathname */
      diafile_dir = get_directory(filename);

      if (strncmp(diafile_dir, image->file, strlen(diafile_dir))==0) {
	/* The image pathname has the dia file pathname in the begining */
	/* Save the relative path: */
	data_add_string(new_attribute(obj_node, "file"), image->file + strlen(diafile_dir));
      } else {
	/* Save the absolute path: */
	data_add_string(new_attribute(obj_node, "file"), image->file);
      }
      
      g_free(diafile_dir);
      
    } else {
      /* Relative path. Must be an erronous filename...
	 Just save the filename. */
      data_add_string(new_attribute(obj_node, "file"), image->file);
    }
    
  }
}

static Object *
image_load(ObjectNode obj_node, int version, const char *filename)
{
  Image *image;
  Element *elem;
  Object *obj;
  int i;
  AttributeNode attr;
  char *diafile_dir;
  
  image = g_malloc(sizeof(Image));
  elem = (Element *)image;
  obj = (Object *)image;
  
  obj->type = &image_type;
  obj->ops = &image_ops;

  element_load(elem, obj_node);
  
  image->border_width = 0.1;
  attr = object_find_attribute(obj_node, "border_width");
  if (attr != NULL)
    image->border_width =  data_real( attribute_first_data(attr) );

  image->border_color = color_black;
  attr = object_find_attribute(obj_node, "border_color");
  if (attr != NULL)
    data_color(attribute_first_data(attr), &image->border_color);
  
  image->line_style = LINESTYLE_SOLID;
  attr = object_find_attribute(obj_node, "line_style");
  if (attr != NULL)
    image->line_style =  data_enum( attribute_first_data(attr) );

  image->draw_border = TRUE;
  attr = object_find_attribute(obj_node, "draw_border");
  if (attr != NULL)
    image->draw_border =  data_boolean( attribute_first_data(attr) );

  image->keep_aspect = TRUE;
  attr = object_find_attribute(obj_node, "keep_aspect");
  if (attr != NULL)
    image->keep_aspect =  data_boolean( attribute_first_data(attr) );

  attr = object_find_attribute(obj_node, "file");
  if (attr != NULL) {
    image->file =  data_string( attribute_first_data(attr) );
  } else {
    image->file = g_strdup("");
  }

  element_init(elem, 8, 8);

  for (i=0;i<8;i++) {
    obj->connections[i] = &image->connections[i];
    image->connections[i].object = obj;
    image->connections[i].connected = NULL;
  }

  image->image = NULL;
  
  if (strcmp(image->file, "")!=0) {
    diafile_dir = get_directory(filename);

    if (g_path_is_absolute(image->file)) { /* Absolute pathname */
      image->image = dia_image_load(image->file);
      if (image->image == NULL) {
	/* Not found as abs path, try in same dir as diagram. */
	char *temp_string;
	const char *image_file_name;

	image_file_name = strrchr(image->file, G_DIR_SEPARATOR) + 1;

	temp_string = g_malloc(strlen(diafile_dir) +
			       strlen(image_file_name) +1);

	strcpy(temp_string, diafile_dir);
	strcat(temp_string, image_file_name);

	image->image = dia_image_load(temp_string);

	if (image->image != NULL) {
	  /* Found file in same dir as diagram. */
	  message_warning(_("The image file '%s' was not found in that directory.\n"
			  "Using the file '%s' instead\n"), image->file, temp_string);
	  g_free(image->file);
	  image->file = temp_string;
	} else {
	  g_free(temp_string);
	  
	  image->image = dia_image_load((char *)image_file_name);
	  if (image->image != NULL) {
	    char *tmp;
	    /* Found file in current dir. */
	    message_warning(_("The image file '%s' was not found in that directory.\n"
			    "Using the file '%s' instead\n"), image->file, image_file_name);
	    tmp = image->file;
	    image->file = strdup(image_file_name);
	    g_free(tmp);
	  } else {
	    message_warning(_("The image file '%s' was not found.\n"),
			    image_file_name);
	  }
	}
      }
    } else { /* Relative pathname: */
      char *temp_string;

      temp_string = g_malloc(strlen(diafile_dir) +
			     strlen(image->file) +1);

      strcpy(temp_string, diafile_dir);
      strcat(temp_string, image->file);

      image->image = dia_image_load(temp_string);

      if (image->image != NULL) {
	/* Found file in same dir as diagram. */
	g_free(image->file);
	image->file = temp_string;
      } else {
	g_free(temp_string);
	  
	image->image = dia_image_load(image->file);
	if (image->image == NULL) {
	  /* Didn't find file in current dir. */
	  message_warning(_("The image file '%s' was not found.\n"),
			  image->file);
	}
      }
    }
    g_free(diafile_dir);
  }

  image_update_data(image);

  return (Object *)image;

}
