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
#include <config.h>

#include <assert.h>
#include <string.h>
#include <math.h>
#ifdef HAVE_UNIST_H
#include <unistd.h>
#endif

#include "intl.h"
#include "message.h"
#include "object.h"
#include "element.h"
#include "connectionpoint.h"
#include "diarenderer.h"
#include "attributes.h"
#include "widgets.h"
#include "dia_image.h"
#include "message.h"
#include "properties.h"

#include "pixmaps/image.xpm"

#define DEFAULT_WIDTH 2.0
#define DEFAULT_HEIGHT 2.0

typedef struct _Image Image;

struct _Image {
  Element element;

  ConnectionPoint connections[8];

  real border_width;
  Color border_color;
  LineStyle line_style;
  real dashlength;
  
  DiaImage image;
  gchar *file;
  gboolean draw_border;
  gboolean keep_aspect;
};

static struct _ImageProperties {
  gchar *file;
  gboolean draw_border;
  gboolean keep_aspect;
} default_properties = { "", FALSE, TRUE };

static real image_distance_from(Image *image, Point *point);
static void image_select(Image *image, Point *clicked_point,
		       DiaRenderer *interactive_renderer);
static ObjectChange* image_move_handle(Image *image, Handle *handle,
				       Point *to, ConnectionPoint *cp,
				       HandleMoveReason reason, ModifierKeys modifiers);
static ObjectChange* image_move(Image *image, Point *to);
static void image_draw(Image *image, DiaRenderer *renderer);
static void image_update_data(Image *image);
static Object *image_create(Point *startpoint,
			  void *user_data,
			  Handle **handle1,
			  Handle **handle2);
static void image_destroy(Image *image);
static Object *image_copy(Image *image);

static PropDescription *image_describe_props(Image *image);
static void image_get_props(Image *image, GPtrArray *props);
static void image_set_props(Image *image, GPtrArray *props);

static void image_save(Image *image, ObjectNode obj_node, const char *filename);
static Object *image_load(ObjectNode obj_node, int version, const char *filename);

static ObjectTypeOps image_type_ops =
{
  (CreateFunc) image_create,
  (LoadFunc)   image_load,
  (SaveFunc)   image_save,
  (GetDefaultsFunc)   NULL,
  (ApplyDefaultsFunc) NULL
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
  (GetPropertiesFunc)   object_create_props_dialog,
  (ApplyPropertiesFunc) object_apply_props_from_dialog,
  (ObjectMenuFunc)      NULL,
  (DescribePropsFunc)   image_describe_props,
  (GetPropsFunc)        image_get_props,
  (SetPropsFunc)        image_set_props,
};

static PropDescription image_props[] = {
  ELEMENT_COMMON_PROPERTIES,
  { "image_file", PROP_TYPE_FILE, PROP_FLAG_VISIBLE,
    N_("Image file"), NULL, NULL},
  { "show_border", PROP_TYPE_BOOL, PROP_FLAG_VISIBLE,
    N_("Draw border"), NULL, NULL},
  { "keep_aspect", PROP_TYPE_BOOL, PROP_FLAG_VISIBLE,
    N_("Keep aspect ratio"), NULL, NULL},
  PROP_STD_LINE_WIDTH,
  PROP_STD_LINE_COLOUR,
  PROP_STD_LINE_STYLE,
  PROP_DESC_END
};

static PropDescription *
image_describe_props(Image *image)
{
  if (image_props[0].quark == 0)
    prop_desc_list_calculate_quarks(image_props);
  return image_props;
}

static PropOffset image_offsets[] = {
  ELEMENT_COMMON_PROPERTIES_OFFSETS,
  { "image_file", PROP_TYPE_FILE, offsetof(Image, file) },
  { "show_border", PROP_TYPE_BOOL, offsetof(Image, draw_border) },
  { "keep_aspect", PROP_TYPE_BOOL, offsetof(Image, keep_aspect) },
  { "line_width", PROP_TYPE_REAL, offsetof(Image, border_width) },
  { "line_colour", PROP_TYPE_COLOUR, offsetof(Image, border_color) },
  { "line_style", PROP_TYPE_LINESTYLE,
    offsetof(Image, line_style), offsetof(Image, dashlength) },
  { NULL, 0, 0 }
};

static void
image_get_props(Image *image, GPtrArray *props)
{
  object_get_props_from_offsets(&image->element.object, image_offsets, props);
}

static void
image_set_props(Image *image, GPtrArray *props)
{
  char *old_file = image->file ? g_strdup(image->file) : NULL;

  object_set_props_from_offsets(&image->element.object, image_offsets, props);

  /* handle changing the image. */
  if (strcmp(image->file, old_file) != 0) {
    Element *elem = &image->element;
    DiaImage img = dia_image_load(image->file);

    if (img)
      image->image = img;
    else
      image->image = dia_image_get_broken();
    elem->height = (elem->width*(float)dia_image_height(image->image))/
      (float)dia_image_width(image->image);
  }
  g_free(old_file);

  image_update_data(image);
}

static real
image_distance_from(Image *image, Point *point)
{
  Element *elem = &image->element;
  Rectangle rect;

  rect.left = elem->corner.x - image->border_width;
  rect.right = elem->corner.x + elem->width + image->border_width;
  rect.top = elem->corner.y - image->border_width;
  rect.bottom = elem->corner.y + elem->height + image->border_width;
  return distance_rectangle_point(&rect, point);
}

static void
image_select(Image *image, Point *clicked_point,
	     DiaRenderer *interactive_renderer)
{
  element_update_handles(&image->element);
}

static ObjectChange*
image_move_handle(Image *image, Handle *handle,
		  Point *to, ConnectionPoint *cp,
		  HandleMoveReason reason, ModifierKeys modifiers)
{
  Element *elem = &image->element;
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
      if (new_height == 0 || new_width/new_height > width/height) {
	new_height = new_width*height/width;
      } else {
	new_width = new_height*width/height;
      }
      to->x = image->element.corner.x+(image->element.width-new_width);
      to->y = image->element.corner.y+(image->element.height-new_height);
      element_move_handle(elem, HANDLE_RESIZE_NW, to, cp, reason, modifiers);
      break;
    case HANDLE_RESIZE_N:
      new_width = (-(to->y-image->element.corner.y)+height)*width/height;
      to->x = image->element.corner.x+new_width;
      element_move_handle(elem, HANDLE_RESIZE_NE, to, cp, reason, modifiers);
      break;
    case HANDLE_RESIZE_NE:
      new_width = to->x-image->element.corner.x;
      new_height = -(to->y-image->element.corner.y)+height;
      if (new_height == 0 || new_width/new_height > width/height) {
	new_height = new_width*height/width;
      } else {
	new_width = new_height*width/height;
      }
      to->x = image->element.corner.x+new_width;
      to->y = image->element.corner.y+(image->element.height-new_height);
      element_move_handle(elem, HANDLE_RESIZE_NE, to, cp, reason, modifiers);
      break;
    case HANDLE_RESIZE_E:
      new_height = (to->x-image->element.corner.x)*height/width;
      to->y = image->element.corner.y+new_height;
      element_move_handle(elem, HANDLE_RESIZE_SE, to, cp, reason, modifiers);
      break;
    case HANDLE_RESIZE_SE:
      new_width = to->x-image->element.corner.x;
      new_height = to->y-image->element.corner.y;
      if (new_height == 0 || new_width/new_height > width/height) {
	new_height = new_width*height/width;
      } else {
	new_width = new_height*width/height;
      }
      to->x = image->element.corner.x+new_width;
      to->y = image->element.corner.y+new_height;
      element_move_handle(elem, HANDLE_RESIZE_SE, to, cp, reason, modifiers);
      break;
    case HANDLE_RESIZE_S:
      new_width = (to->y-image->element.corner.y)*width/height;
      to->x = image->element.corner.x+new_width;
      element_move_handle(elem, HANDLE_RESIZE_SE, to, cp, reason, modifiers);
      break;
    case HANDLE_RESIZE_SW:
      new_width = -(to->x-image->element.corner.x)+width;
      new_height = to->y-image->element.corner.y;
      if (new_height == 0 || new_width/new_height > width/height) {
	new_height = new_width*height/width;
      } else {
	new_width = new_height*width/height;
      }
      to->x = image->element.corner.x+(image->element.width-new_width);
      to->y = image->element.corner.y+new_height;
      element_move_handle(elem, HANDLE_RESIZE_SW, to, cp, reason, modifiers);
      break;
    case HANDLE_RESIZE_W:
      new_height = (-(to->x-image->element.corner.x)+width)*height/width;
      to->y = image->element.corner.y+new_height;
      element_move_handle(elem, HANDLE_RESIZE_SW, to, cp, reason, modifiers);
      break;
    default:
      message_warning("Unforeseen handle in image_move_handle: %d\n",
		      handle->id);
      
    }
  } else {
    element_move_handle(elem, handle->id, to, cp, reason, modifiers);
  }
  image_update_data(image);

  return NULL;
}

static ObjectChange*
image_move(Image *image, Point *to)
{
  image->element.corner = *to;
  
  image_update_data(image);

  return NULL;
}

static void
image_draw(Image *image, DiaRenderer *renderer)
{
  DiaRendererClass *renderer_ops = DIA_RENDERER_GET_CLASS (renderer);
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
    renderer_ops->set_linewidth(renderer, image->border_width);
    renderer_ops->set_linestyle(renderer, image->line_style);
    renderer_ops->set_dashlength(renderer, image->dashlength);
    renderer_ops->set_linejoin(renderer, LINEJOIN_MITER);
    
    renderer_ops->draw_rect(renderer, 
			     &ul_corner,
			     &lr_corner, 
			     &image->border_color);
  }
  /* Draw the image */
  if (image->image) {
    renderer_ops->draw_image(renderer, &elem->corner, elem->width,
			      elem->height, image->image);
  } else {
    DiaImage broken = dia_image_get_broken();
    renderer_ops->draw_image(renderer, &elem->corner, elem->width,
			      elem->height, broken);
  }
}

static void
image_update_data(Image *image)
{
  Element *elem = &image->element;
  ElementBBExtras *extra = &elem->extra_spacing;
  Object *obj = &elem->object;

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
  
  extra->border_trans = image->border_width / 2.0;
  element_update_boundingbox(elem);
  
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

  image = g_malloc0(sizeof(Image));
  elem = &image->element;
  obj = &elem->object;
  
  obj->type = &image_type;
  obj->ops = &image_ops;

  elem->corner = *startpoint;
  elem->width = DEFAULT_WIDTH;
  elem->height = DEFAULT_HEIGHT;
    
  image->border_width =  attributes_get_default_linewidth();
  image->border_color = attributes_get_foreground();
  attributes_get_default_line_style(&image->line_style,
				    &image->dashlength);
  
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
  return &image->element.object;
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
  
  elem = &image->element;
  
  newimage = g_malloc0(sizeof(Image));
  newelem = &newimage->element;
  newobj = &newelem->object;

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
  if (image->image)
    dia_image_add_ref(image->image);
  newimage->image = image->image;

  newimage->draw_border = image->draw_border;
  newimage->keep_aspect = image->keep_aspect;

  return &newimage->element.object;
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
  char *dirname;
  
  if (filename==NULL)
    return NULL;

  dirname = g_dirname(filename);
  if (g_path_is_absolute(dirname)) {
      directory = g_strconcat(dirname, G_DIR_SEPARATOR_S, NULL);
  } else {
      cwd = g_get_current_dir();
      directory = g_strconcat(cwd, G_DIR_SEPARATOR_S, dirname, NULL);
      g_free(cwd);
  }
  g_free(dirname);

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

  if (image->line_style != LINESTYLE_SOLID &&
      image->dashlength != DEFAULT_LINESTYLE_DASHLEN)
    data_add_real(new_attribute(obj_node, "dashlength"),
		  image->dashlength);
  
  data_add_boolean(new_attribute(obj_node, "draw_border"), image->draw_border);
  data_add_boolean(new_attribute(obj_node, "keep_aspect"), image->keep_aspect);

  if (image->file != NULL) {
    if (g_path_is_absolute(image->file)) { /* Absolute pathname */
      diafile_dir = get_directory(filename);

      if (strncmp(diafile_dir, image->file, strlen(diafile_dir))==0) {
	/* The image pathname has the dia file pathname in the begining */
	/* Save the relative path: */
	data_add_filename(new_attribute(obj_node, "file"), image->file + strlen(diafile_dir));
      } else {
	/* Save the absolute path: */
	data_add_filename(new_attribute(obj_node, "file"), image->file);
      }
      
      g_free(diafile_dir);
      
    } else {
      /* Relative path. Must be an erronous filename...
	 Just save the filename. */
      data_add_filename(new_attribute(obj_node, "file"), image->file);
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
  
  image = g_malloc0(sizeof(Image));
  elem = &image->element;
  obj = &elem->object;
  
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

  image->dashlength = DEFAULT_LINESTYLE_DASHLEN;
  attr = object_find_attribute(obj_node, "dashlength");
  if (attr != NULL)
    image->dashlength = data_real(attribute_first_data(attr));

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
    image->file =  data_filename( attribute_first_data(attr) );
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
	    image->file = g_strdup(image_file_name);
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

  return &image->element.object;
}
