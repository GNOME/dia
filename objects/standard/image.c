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

#include <gdk_imlib.h>

#include "message.h"
#include "object.h"
#include "element.h"
#include "connectionpoint.h"
#include "render.h"
#include "attributes.h"
#include "widgets.h"

#include "pixmaps/image.xpm"

#define DEFAULT_WIDTH 2.0
#define DEFAULT_HEIGHT 2.0
#define DEFAULT_BORDER 0.25

typedef struct _Image Image;
typedef struct _ImagePropertiesDialog ImagePropertiesDialog;
typedef struct _ImageDefaultsDialog ImageDefaultsDialog;

struct _Image {
  Element element;

  ConnectionPoint connections[8];

  real border_width;
  Color border_color;
  Color inner_color;
  LineStyle line_style;
  
  GdkImlibImage *image;
  gchar *file;
  gboolean draw_border;
  gboolean keep_aspect;

  ImagePropertiesDialog *properties_dialog;
};

typedef struct _ImageProperties {
  Color *fg_color;
  Color *bg_color;
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
  DiaColorSelector *bg_color;
  DiaLineStyleSelector *line_style;

  DiaFileSelector *file;

  GtkToggleButton *draw_border;
  GtkToggleButton *keep_aspect;
};

struct _ImageDefaultsDialog {
  GtkWidget *vbox;

  DiaLineStyleSelector *line_style;

  DiaFileSelector *file;

  GtkToggleButton *draw_border;
  GtkToggleButton *keep_aspect;
};

static ImageDefaultsDialog *image_defaults_dialog;
static ImageProperties default_properties;

static real image_distance_from(Image *image, Point *point);
static void image_select(Image *image, Point *clicked_point,
		       Renderer *interactive_renderer);
static void image_move_handle(Image *image, Handle *handle,
			    Point *to, HandleMoveReason reason);
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

static void image_save(Image *image, ObjectNode obj_node );
static Object *image_load(ObjectNode obj_node, int version);
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
  (IsEmptyFunc)         object_return_false
};

static void
image_apply_properties(Image *image)
{
  ImagePropertiesDialog *prop_dialog;
  gchar *new_file;

  prop_dialog = image->properties_dialog;

  image->border_width = gtk_spin_button_get_value_as_float(prop_dialog->border_width);
  dia_color_selector_get_color(prop_dialog->fg_color, &image->border_color);
  dia_color_selector_get_color(prop_dialog->bg_color, &image->inner_color);
  image->line_style = dia_line_style_selector_get_linestyle(prop_dialog->line_style);

  image->draw_border = gtk_toggle_button_get_active(prop_dialog->draw_border);
  image->keep_aspect = gtk_toggle_button_get_active(prop_dialog->keep_aspect);
  
  new_file = dia_file_selector_get_file(prop_dialog->file);
  if (image->file) g_free(image->file);
  if (image->image) {
    gdk_imlib_destroy_image(image->image);
  }
  image->image = gdk_imlib_load_image(new_file);
  if ((image->image != NULL) && (image->keep_aspect)) {
    /* Must... keep... aspect... ratio... */
    float ratio = (float)image->image->rgb_height/
      (float)image->image->rgb_width;
    image->element.height = image->element.width * ratio;
  }
  image->file = g_strdup(new_file);

  image_update_data(image);
}

static GtkWidget *
image_get_properties(Image *image)
{
  ImagePropertiesDialog *prop_dialog;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *color;
  GtkWidget *checkbox;
  GtkWidget *linestyle;
  GtkWidget *file;
  GtkWidget *border_width;
  GtkAdjustment *adj;

  if (image->properties_dialog == NULL) {
  
    prop_dialog = g_new(ImagePropertiesDialog, 1);
    image->properties_dialog = prop_dialog;
    vbox = gtk_vbox_new(FALSE, 5);
    prop_dialog->vbox = vbox;

    hbox = gtk_hbox_new(FALSE, 5);
    label = gtk_label_new("Border width:");
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    gtk_widget_show (label);
    adj = (GtkAdjustment *) gtk_adjustment_new(0.1, 0.00, 10.0, 0.01, 0.0, 0.0);
    border_width = gtk_spin_button_new(adj, 1.0, 2);
    gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(border_width), TRUE);
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(border_width), TRUE);
    prop_dialog->border_width = GTK_SPIN_BUTTON(border_width);
    gtk_box_pack_start(GTK_BOX (hbox), border_width, TRUE, TRUE, 0);
    gtk_widget_show (border_width);
    gtk_widget_show(hbox);
    gtk_box_pack_start (GTK_BOX(vbox), hbox, TRUE, TRUE, 0);


    hbox = gtk_hbox_new(FALSE, 5);
    label = gtk_label_new("Foreground color:");
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    gtk_widget_show (label);
    color = dia_color_selector_new();
    prop_dialog->fg_color = DIACOLORSELECTOR(color);
    gtk_box_pack_start (GTK_BOX (hbox), color, TRUE, TRUE, 0);
    gtk_widget_show (color);
    gtk_widget_show(hbox);
    gtk_box_pack_start (GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

    hbox = gtk_hbox_new(FALSE, 5);
    label = gtk_label_new("Background color:");
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    gtk_widget_show (label);
    color = dia_color_selector_new();
    prop_dialog->bg_color = DIACOLORSELECTOR(color);
    gtk_box_pack_start (GTK_BOX (hbox), color, TRUE, TRUE, 0);
    gtk_widget_show (color);
    gtk_widget_show(hbox);
    gtk_box_pack_start (GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

    hbox = gtk_hbox_new(FALSE, 5);
    label = gtk_label_new("Line style:");
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    gtk_widget_show (label);
    linestyle = dia_line_style_selector_new();
    prop_dialog->line_style = DIALINESTYLESELECTOR(linestyle);
    gtk_box_pack_start (GTK_BOX (hbox), linestyle, TRUE, TRUE, 0);
    gtk_widget_show (linestyle);
    gtk_widget_show(hbox);
    gtk_box_pack_start (GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

    hbox = gtk_hbox_new(FALSE, 5);
    label = gtk_label_new("Image file:");
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    gtk_widget_show (label);
    file = dia_file_selector_new();
    prop_dialog->file = DIAFILESELECTOR(file);
    gtk_box_pack_start (GTK_BOX (hbox), file, TRUE, TRUE, 0);
    gtk_widget_show (file);
    gtk_widget_show(hbox);
    gtk_box_pack_start (GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

    hbox = gtk_hbox_new(FALSE, 5);
    checkbox = gtk_check_button_new_with_label("Show border");
    prop_dialog->draw_border = GTK_TOGGLE_BUTTON( checkbox );
    gtk_widget_show(checkbox);
    gtk_widget_show(hbox);
    gtk_box_pack_start (GTK_BOX (hbox), checkbox, TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);

    hbox = gtk_hbox_new(FALSE, 5);
    checkbox = gtk_check_button_new_with_label("Keep aspect ratio");
    prop_dialog->keep_aspect = GTK_TOGGLE_BUTTON( checkbox );
    gtk_widget_show(checkbox);
    gtk_widget_show(hbox);
    gtk_box_pack_start (GTK_BOX (hbox), checkbox, TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);

    gtk_widget_show (vbox);
  }

  prop_dialog = image->properties_dialog;
    
  gtk_spin_button_set_value(prop_dialog->border_width, 
			    image->border_width);
  dia_color_selector_set_color(prop_dialog->fg_color, 
			       &image->border_color);
  dia_color_selector_set_color(prop_dialog->bg_color, 
			       &image->inner_color);
  dia_file_selector_set_file(prop_dialog->file, image->file);
  dia_line_style_selector_set_linestyle(prop_dialog->line_style,
					image->line_style);
  gtk_toggle_button_set_active(prop_dialog->draw_border, image->draw_border);
  gtk_toggle_button_set_active(prop_dialog->keep_aspect, image->keep_aspect);

  return prop_dialog->vbox;
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
  default_properties.line_style = dia_line_style_selector_get_linestyle(image_defaults_dialog->line_style);
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
    label = gtk_label_new("Line style:");
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    gtk_widget_show (label);
    linestyle = dia_line_style_selector_new();
    image_defaults_dialog->line_style = DIALINESTYLESELECTOR(linestyle);
    gtk_box_pack_start (GTK_BOX (hbox), linestyle, TRUE, TRUE, 0);
    gtk_widget_show (linestyle);
    gtk_widget_show(hbox);
    gtk_box_pack_start (GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

    hbox = gtk_hbox_new(FALSE, 5);
    label = gtk_label_new("Image file:");
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    gtk_widget_show (label);
    file = dia_file_selector_new();
    image_defaults_dialog->file = DIAFILESELECTOR(file);
    gtk_box_pack_start (GTK_BOX (hbox), file, TRUE, TRUE, 0);
    gtk_widget_show (file);
    gtk_widget_show(hbox);
    gtk_box_pack_start (GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

    hbox = gtk_hbox_new(FALSE, 5);
    checkbox = gtk_check_button_new_with_label("Show border:");
    image_defaults_dialog->draw_border = GTK_TOGGLE_BUTTON( checkbox );
    gtk_widget_show(checkbox);
    gtk_widget_show(hbox);
    gtk_box_pack_start (GTK_BOX (hbox), checkbox, TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);

    hbox = gtk_hbox_new(FALSE, 5);
    checkbox = gtk_check_button_new_with_label("Keep aspect ratio:");
    image_defaults_dialog->keep_aspect = GTK_TOGGLE_BUTTON( checkbox );
    gtk_widget_show(checkbox);
    gtk_widget_show(hbox);
    gtk_box_pack_start (GTK_BOX (hbox), checkbox, TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);

    gtk_widget_show (vbox);
  }

  dia_line_style_selector_set_linestyle(image_defaults_dialog->line_style,
					default_properties.line_style);
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
		  Point *to, HandleMoveReason reason)
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
    renderer->ops->set_linewidth(renderer, image->border_width);
    renderer->ops->set_linestyle(renderer, LINESTYLE_SOLID);
    renderer->ops->draw_line(renderer, &ul_corner, &lr_corner, 
			     &image->border_color);
  }
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
  image->inner_color = attributes_get_background();
  image->line_style = default_properties.line_style;
  
  element_init(elem, 8, 8);

  for (i=0;i<8;i++) {
    obj->connections[i] = &image->connections[i];
    image->connections[i].object = obj;
    image->connections[i].connected = NULL;
  }

  if (strcmp(default_properties.file, "")) {
    image->file = g_strdup(default_properties.file);
    image->image = gdk_imlib_load_image(image->file);

    if (image->image) {
      elem->width = (elem->width*(float)image->image->rgb_width)/
	(float)image->image->rgb_height;
    }
  } else {
    image->file = g_strdup("");
    image->image = NULL;
  }

  image->draw_border = default_properties.draw_border;
  image->keep_aspect = default_properties.keep_aspect;

  image_update_data(image);

  image->properties_dialog = NULL;
  
  *handle1 = NULL;
  *handle2 = obj->handles[0];  
  return (Object *)image;
}

static void 
image_destroy(Image *image) {
  if (image->properties_dialog != NULL) {
    gtk_widget_destroy(image->properties_dialog->vbox);
    g_free(image->properties_dialog);
  }
  
  if (image->file != NULL)
    g_free(image->file);

  if (image->image != NULL)
    gdk_imlib_destroy_image(image->image);

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
  newimage->inner_color = image->inner_color;
  newimage->line_style = image->line_style;
  
  for (i=0;i<8;i++) {
    newobj->connections[i] = &newimage->connections[i];
    newimage->connections[i].object = newobj;
    newimage->connections[i].connected = NULL;
    newimage->connections[i].pos = image->connections[i].pos;
    newimage->connections[i].last_pos = image->connections[i].last_pos;
  }

  newimage->properties_dialog = NULL;

  newimage->file = g_strdup(image->file);
  if (strcmp(image->file, "")) {
    newimage->image = gdk_imlib_load_image(image->file);
  }

  newimage->draw_border = image->draw_border;
  newimage->keep_aspect = image->keep_aspect;

  return (Object *)newimage;
}

static void
image_save(Image *image, ObjectNode obj_node)
{
  element_save(&image->element, obj_node);

  data_add_real(new_attribute(obj_node, "border_width"),
		image->border_width);
  data_add_color(new_attribute(obj_node, "border_color"),
		 &image->border_color);
  data_add_color(new_attribute(obj_node, "inner_color"),
		 &image->inner_color);
  data_add_enum(new_attribute(obj_node, "line_style"),
		image->line_style);

  data_add_boolean(new_attribute(obj_node, "draw_border"), image->draw_border);
  data_add_boolean(new_attribute(obj_node, "keep_aspect"), image->keep_aspect);

  /* Question here:  Should the file be saved along with the diagram, or
     just a filename? */
  data_add_string(new_attribute(obj_node, "file"), image->file);
}

static Object *
image_load(ObjectNode obj_node, int version)
{
  Image *image;
  Element *elem;
  Object *obj;
  int i;
  AttributeNode attr;

  image = g_malloc(sizeof(Image));
  elem = (Element *)image;
  obj = (Object *)image;
  
  obj->type = &image_type;
  obj->ops = &image_ops;

  element_load(elem, obj_node);
  
  image->properties_dialog = NULL;

  image->border_width = 0.1;
  attr = object_find_attribute(obj_node, "border_width");
  if (attr != NULL)
    image->border_width =  data_real( attribute_first_data(attr) );

  image->border_color = color_black;
  attr = object_find_attribute(obj_node, "border_color");
  if (attr != NULL)
    data_color(attribute_first_data(attr), &image->border_color);
  
  image->inner_color = color_white;
  attr = object_find_attribute(obj_node, "inner_color");
  if (attr != NULL)
    data_color(attribute_first_data(attr), &image->inner_color);
  
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

  if (strcmp(image->file, "")) {
    image->image = gdk_imlib_load_image(image->file);
  }

  image_update_data(image);

  return (Object *)image;

}
