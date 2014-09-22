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
#include <sys/stat.h>
#include <time.h>
#ifdef HAVE_UNIST_H
#include <unistd.h>
#endif
#include <glib/gstdio.h>

#include "intl.h"
#include "message.h"
#include "object.h"
#include "element.h"
#include "connectionpoint.h"
#include "diarenderer.h"
#include "attributes.h"
#include "dia_image.h"
#include "message.h"
#include "properties.h"

#include "tool-icons.h"

#define DEFAULT_WIDTH 2.0
#define DEFAULT_HEIGHT 2.0

#define NUM_CONNECTIONS 9

typedef struct _Image Image;

/*!
 * \brief Standard - Image : rectangular image
 * \extends _Element
 * \ingroup StandardObjects
 */
struct _Image {
  Element element;

  ConnectionPoint connections[NUM_CONNECTIONS];

  real border_width;
  Color border_color;
  LineStyle line_style;
  real dashlength;
  
  DiaImage *image;
  gchar *file;
  
  gboolean inline_data;
  /* may contain the images pixbuf pointer - if so: reference counted! */
  GdkPixbuf *pixbuf;

  gboolean draw_border;
  gboolean keep_aspect;

  time_t mtime;
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
static DiaObject *image_create(Point *startpoint,
			  void *user_data,
			  Handle **handle1,
			  Handle **handle2);
static void image_destroy(Image *image);
static DiaObject *image_copy(Image *image);

static void image_get_props(Image *image, GPtrArray *props);
static void image_set_props(Image *image, GPtrArray *props);

static void image_save(Image *image, ObjectNode obj_node, DiaContext *ctx);
static DiaObject *image_load(ObjectNode obj_node, int version, DiaContext *ctx);

static ObjectTypeOps image_type_ops =
{
  (CreateFunc) image_create,
  (LoadFunc)   image_load,
  (SaveFunc)   image_save,
  (GetDefaultsFunc)   NULL,
  (ApplyDefaultsFunc) NULL
};

static PropDescription image_props[] = {
  ELEMENT_COMMON_PROPERTIES,
  { "image_file", PROP_TYPE_FILE, PROP_FLAG_VISIBLE,
    N_("Image file"), NULL, NULL},
  { "inline_data", PROP_TYPE_BOOL, PROP_FLAG_VISIBLE|PROP_FLAG_OPTIONAL,
    N_("Inline data"), N_("Store image data in diagram"), NULL },
  { "pixbuf", PROP_TYPE_PIXBUF, PROP_FLAG_DONT_MERGE|PROP_FLAG_OPTIONAL,
    N_("Pixbuf"), N_("The Pixbuf reference"), NULL },
  { "show_border", PROP_TYPE_BOOL, PROP_FLAG_VISIBLE,
    N_("Draw border"), NULL, NULL},
  { "keep_aspect", PROP_TYPE_BOOL, PROP_FLAG_VISIBLE,
    N_("Keep aspect ratio"), NULL, NULL},
  PROP_STD_LINE_WIDTH,
  PROP_STD_LINE_COLOUR,
  PROP_STD_LINE_STYLE,
  PROP_DESC_END
};

DiaObjectType image_type =
{
  "Standard - Image",  /* name */
  0,                 /* version */
  (const char **) image_icon, /* pixmap */

  &image_type_ops,      /* ops */
  NULL,
  0,
  image_props
};

DiaObjectType *_image_type = (DiaObjectType *) &image_type;

static ObjectOps image_ops = {
  (DestroyFunc)         image_destroy,
  (DrawFunc)            image_draw,
  (DistanceFunc)        image_distance_from,
  (SelectFunc)          image_select,
  (CopyFunc)            image_copy,
  (MoveFunc)            image_move,
  (MoveHandleFunc)      image_move_handle,
  (GetPropertiesFunc)   object_create_props_dialog,
  (ApplyPropertiesDialogFunc) object_apply_props_from_dialog,
  (ObjectMenuFunc)      NULL,
  (DescribePropsFunc)   object_describe_props,
  (GetPropsFunc)        image_get_props,
  (SetPropsFunc)        image_set_props,
  (TextEditFunc) 0,
  (ApplyPropertiesListFunc) object_apply_props,
};

static PropOffset image_offsets[] = {
  ELEMENT_COMMON_PROPERTIES_OFFSETS,
  { "image_file", PROP_TYPE_FILE, offsetof(Image, file) },
  { "inline_data", PROP_TYPE_BOOL, offsetof(Image, inline_data) },
  { "pixbuf", PROP_TYPE_PIXBUF, offsetof(Image, pixbuf) },
  { "show_border", PROP_TYPE_BOOL, offsetof(Image, draw_border) },
  { "keep_aspect", PROP_TYPE_BOOL, offsetof(Image, keep_aspect) },
  { PROP_STDNAME_LINE_WIDTH, PROP_STDTYPE_LINE_WIDTH, offsetof(Image, border_width) },
  { "line_colour", PROP_TYPE_COLOUR, offsetof(Image, border_color) },
  { "line_style", PROP_TYPE_LINESTYLE,
    offsetof(Image, line_style), offsetof(Image, dashlength) },
  { NULL, 0, 0 }
};

/*!
 * \brief Get properties of the _Image
 * \memberof _Image
 * Overwites DiaObject::get_props() to initialize pixbuf property
 */
static void
image_get_props(Image *image, GPtrArray *props)
{
  if (image->inline_data) {
    if (image->pixbuf != dia_image_pixbuf (image->image))
      image->pixbuf = g_object_ref ((GdkPixbuf *) dia_image_pixbuf (image->image));
  }
  object_get_props_from_offsets(&image->element.object, image_offsets, props);
}

/*!
 * \brief Set properties of the _Image
 * \memberof _Image
 *
 * The properties of _Image are not completely independent. Some logic
 * in this method should lead to a consistent object state.
 *
 * - with inline_data=TRUE we shall have a valid pixbuf
 * - with inline_data=FALSE we should have a valid filename (when created
 *   from scratch we don't)
 *
 * Changing the properties tries to ensure consistency and might trigger
 * additional actions like saving a file or loading it from disk. Also
 * some dependent parameters might change in response.
 *
 * - switch on inline data => make inline: set pixbuf, keep filename
 * - switch off inline data => save pixbuf to filename, if new filename
 * - set a new, non empty filename => load the file
 * - set an empty filename => make inline
 * - set a new, non NULL pixbuf => use as image, inline
 */
static void
image_set_props(Image *image, GPtrArray *props)
{
  struct stat st;
  time_t mtime = 0;
  char *old_file = image->file ? g_strdup(image->file) : NULL;
  const GdkPixbuf *old_pixbuf = dia_image_pixbuf (image->image);
  gboolean was_inline = image->inline_data;

  object_set_props_from_offsets(&image->element.object, image_offsets, props);

  /* use old value on error */
  if (!image->file || g_stat (image->file, &st) != 0)
    mtime = image->mtime;
  else
    mtime = st.st_mtime;

  if (   (!was_inline && image->inline_data && old_pixbuf)
      || (   (!image->file || strlen(image->file) == 0)
	  && old_pixbuf)
      || (image->pixbuf && (image->pixbuf != old_pixbuf))) { /* switch on inline */
    if (!image->pixbuf || old_pixbuf == image->pixbuf) { /* just reference the old image */
      image->pixbuf = g_object_ref ((GdkPixbuf *)old_pixbuf);
      image->inline_data = TRUE;
    } else if (old_pixbuf != image->pixbuf && image->pixbuf) { /* substitute the image and pixbuf */
      DiaImage *old_image = image->image;
      image->image = dia_image_new_from_pixbuf (image->pixbuf);
      image->pixbuf = g_object_ref ((GdkPixbuf *)dia_image_pixbuf (image->image));
      if (old_image)
        g_object_unref (old_image);
      image->inline_data = TRUE;
    } else {
      image->inline_data = FALSE;
      g_warning ("Not setting inline_data");
    }
  } else if (was_inline && !image->inline_data) { /* switch off inline */
    if (old_file && image->file && strcmp (old_file, image->file) != 0) {
       /* export inline data, if saving fails we keep it inline */
       image->inline_data = !dia_image_save (image->image, image->file);
    } else if (!image->file) {
       message_warning (_("Can't save image without filename"));
       image->inline_data = TRUE; /* keep inline */
    }
    if (!image->inline_data) {
      /* drop the pixbuf reference */
      if (image->pixbuf)
        g_object_unref (image->pixbuf);
      image->pixbuf = NULL;
    }
  } else if (   image->file && strlen(image->file) > 0
	     && (   (!old_file || (strcmp (old_file, image->file) != 0))
		 || (image->mtime != mtime))) { /* load new file */
    Element *elem = &image->element;
    DiaImage *img = NULL;

    if ((img = dia_image_load(image->file)) != NULL)
      image->image = img;
    else if (!image->pixbuf) /* dont overwrite inlined */
      image->image = dia_image_get_broken();
    elem->height = (elem->width*(float)dia_image_height(image->image))/
      (float)dia_image_width(image->image);
    /* release image->pixbuf? */
  } else if ((!image->file || strlen(image->file)==0) && !image->pixbuf) {
    /* make it a "broken image" without complaints */
    dia_log_message ("_Image::set_props() seting empty.");
  } else {
    if (   (image->inline_data && image->pixbuf != NULL) /* don't need a filename */
        || (!image->inline_data && image->file)) /* don't need a pixbuf */
      dia_log_message ("_Image::set_props() ok.");
    else
      g_warning ("_Image::set_props() not handled file='%s', pixbuf=%p, inline=%s",
	         image->file ? image->file : "",
	         image->pixbuf, image->inline_data ? "TRUE" : "FALSE");
  }
  g_free(old_file);
  /* remember modification time */
  image->mtime = mtime;

  image_update_data(image);
}

static real
image_distance_from(Image *image, Point *point)
{
  Element *elem = &image->element;
  Rectangle rect;
  real bw = image->draw_border ? image->border_width : 0;

  rect.left = elem->corner.x - bw;
  rect.right = elem->corner.x + elem->width + bw;
  rect.top = elem->corner.y - bw;
  rect.bottom = elem->corner.y + elem->height + bw;
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
    renderer_ops->set_linestyle(renderer, image->line_style, image->dashlength);
    renderer_ops->set_linejoin(renderer, LINEJOIN_MITER);
    
    renderer_ops->draw_rect(renderer, 
			     &ul_corner,
			     &lr_corner,
			     NULL,
			     &image->border_color);
  }
  /* Draw the image */
  if (image->image) {
    renderer_ops->draw_image(renderer, &elem->corner, elem->width,
			      elem->height, image->image);
  } else {
    DiaImage *broken = dia_image_get_broken();
    renderer_ops->draw_image(renderer, &elem->corner, elem->width,
			      elem->height, broken);
    dia_image_unref(broken);
  }
}

static void
image_update_data(Image *image)
{
  Element *elem = &image->element;
  ElementBBExtras *extra = &elem->extra_spacing;
  DiaObject *obj = &elem->object;

  if (image->keep_aspect && image->image) {
    /* maybe the image got changes since */
    real aspect_org = (float)dia_image_width(image->image)
                    / (float)dia_image_height(image->image);
    real aspect_now = elem->width / elem->height;

    if (fabs (aspect_now - aspect_org) > 1e-4) {
      elem->height = elem->width /aspect_org;
    }
  }

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
  image->connections[8].pos.x = elem->corner.x + elem->width / 2.0;
  image->connections[8].pos.y = elem->corner.y + elem->height / 2.0;
  
  /* the image border is drawn vompletely outside of the image, so no /2.0 on border width */
  extra->border_trans = (image->draw_border ? image->border_width : 0.0);
  element_update_boundingbox(elem);
  
  obj->position = elem->corner;
  image->connections[8].directions = DIR_ALL;
  element_update_handles(elem);
}


static DiaObject *
image_create(Point *startpoint,
	     void *user_data,
	     Handle **handle1,
	     Handle **handle2)
{
  Image *image;
  Element *elem;
  DiaObject *obj;
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
  
  element_init(elem, 8, NUM_CONNECTIONS);

  for (i=0; i<NUM_CONNECTIONS; i++) {
    obj->connections[i] = &image->connections[i];
    image->connections[i].object = obj;
    image->connections[i].connected = NULL;
  }
  image->connections[8].flags = CP_FLAGS_MAIN;

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
image_destroy(Image *image)
{
  if (image->file != NULL)
    g_free(image->file);

  if (image->image != NULL)
    dia_image_unref(image->image);

  if (image->pixbuf != NULL)
    g_object_unref(image->pixbuf);

  element_destroy(&image->element);
}

static DiaObject *
image_copy(Image *image)
{
  int i;
  Image *newimage;
  Element *elem, *newelem;
  DiaObject *newobj;
  
  elem = &image->element;
  
  newimage = g_malloc0(sizeof(Image));
  newelem = &newimage->element;
  newobj = &newelem->object;

  element_copy(elem, newelem);

  newimage->border_width = image->border_width;
  newimage->border_color = image->border_color;
  newimage->line_style = image->line_style;
  
  for (i=0;i<NUM_CONNECTIONS;i++) {
    newobj->connections[i] = &newimage->connections[i];
    newimage->connections[i].object = newobj;
    newimage->connections[i].connected = NULL;
    newimage->connections[i].pos = image->connections[i].pos;
    newimage->connections[i].flags = image->connections[i].flags;
  }

  newimage->file = g_strdup(image->file);
  if (image->image)
    dia_image_add_ref(image->image);
  newimage->image = image->image;

  /* not sure if we only want a reference, but it would be safe when 
   * someday editing doe not work inplace, but creates new pixbufs 
   * for every single undoable step */
  newimage->inline_data = image->inline_data;
  if (image->pixbuf)
    newimage->pixbuf = g_object_ref ((GdkPixbuf *)dia_image_pixbuf(newimage->image));
  else
    newimage->pixbuf = image->pixbuf; /* Just say NULL */

  newimage->mtime = image->mtime;
  newimage->draw_border = image->draw_border;
  newimage->keep_aspect = image->keep_aspect;

  return &newimage->element.object;
}

/* Gets the directory path of a filename.
   Uses current working directory if filename is a relative pathname.
   Examples:
     /some/dir/file.gif => /some/dir
     dir/file.gif => /cwd/dir
   
*/
static char *
get_directory(const char *filename) 
{
  char *cwd;
  char *directory;
  char *dirname;
  
  if (filename==NULL)
    return NULL;

  dirname = g_path_get_dirname(filename);
  if (g_path_is_absolute(dirname)) {
      directory = g_build_path(G_DIR_SEPARATOR_S, dirname, NULL);
  } else {
      cwd = g_get_current_dir();
      directory = g_build_path(G_DIR_SEPARATOR_S, cwd, dirname, NULL);
      g_free(cwd);
  }
  g_free(dirname);

  return directory;
}

static void
image_save(Image *image, ObjectNode obj_node, DiaContext *ctx)
{
  char *diafile_dir;
  
  element_save(&image->element, obj_node, ctx);

  if (image->border_width != 0.1)
    data_add_real(new_attribute(obj_node, "border_width"),
		  image->border_width, ctx);

  if (!color_equals(&image->border_color, &color_black))
    data_add_color(new_attribute(obj_node, "border_color"),
		   &image->border_color, ctx);
  
  if (image->line_style != LINESTYLE_SOLID)
    data_add_enum(new_attribute(obj_node, "line_style"),
		  image->line_style, ctx);

  if (image->line_style != LINESTYLE_SOLID &&
      image->dashlength != DEFAULT_LINESTYLE_DASHLEN)
    data_add_real(new_attribute(obj_node, "dashlength"),
		  image->dashlength, ctx);
  
  data_add_boolean(new_attribute(obj_node, "draw_border"), image->draw_border, ctx);
  data_add_boolean(new_attribute(obj_node, "keep_aspect"), image->keep_aspect, ctx);

  if (image->file != NULL) {
    if (g_path_is_absolute(image->file)) { /* Absolute pathname */
      diafile_dir = get_directory(dia_context_get_filename (ctx));

      if (diafile_dir && strncmp(diafile_dir, image->file, strlen(diafile_dir))==0) {
	/* The image pathname has the dia file pathname in the begining */
	/* Save the relative path: */
	data_add_filename(new_attribute(obj_node, "file"),
			  image->file + strlen(diafile_dir) + 1, ctx);
      } else {
	/* Save the absolute path: */
	data_add_filename(new_attribute(obj_node, "file"), image->file, ctx);
      }
      g_free(diafile_dir);
    } else {
      /* Relative path. Must be an erronuous filename...
	 Just save the filename. */
      data_add_filename(new_attribute(obj_node, "file"), image->file, ctx);
    }
  }
  /* only save image_data inline if told to do so */
  if (image->inline_data) {
    GdkPixbuf *pixbuf;
    data_add_boolean (new_attribute(obj_node, "inline_data"),
		      image->inline_data, ctx);

    /* just to be sure to get the currently visible */
    pixbuf = (GdkPixbuf *)dia_image_pixbuf (image->image);
    if (pixbuf != image->pixbuf && image->pixbuf != NULL)
      message_warning (_("Inconsistent pixbuf during image save."));
    if (pixbuf)
      data_add_pixbuf (new_attribute(obj_node, "pixbuf"), pixbuf, ctx);
  }
}

static DiaObject *
image_load(ObjectNode obj_node, int version, DiaContext *ctx)
{
  Image *image;
  Element *elem;
  DiaObject *obj;
  int i;
  AttributeNode attr;
  char *diafile_dir;
  
  image = g_malloc0(sizeof(Image));
  elem = &image->element;
  obj = &elem->object;
  
  obj->type = &image_type;
  obj->ops = &image_ops;

  element_load(elem, obj_node, ctx);
  
  image->border_width = 0.1;
  attr = object_find_attribute(obj_node, "border_width");
  if (attr != NULL)
    image->border_width =  data_real(attribute_first_data(attr), ctx);

  image->border_color = color_black;
  attr = object_find_attribute(obj_node, "border_color");
  if (attr != NULL)
    data_color(attribute_first_data(attr), &image->border_color, ctx);
  
  image->line_style = LINESTYLE_SOLID;
  attr = object_find_attribute(obj_node, "line_style");
  if (attr != NULL)
    image->line_style =  data_enum(attribute_first_data(attr), ctx);

  image->dashlength = DEFAULT_LINESTYLE_DASHLEN;
  attr = object_find_attribute(obj_node, "dashlength");
  if (attr != NULL)
    image->dashlength = data_real(attribute_first_data(attr), ctx);

  image->draw_border = TRUE;
  attr = object_find_attribute(obj_node, "draw_border");
  if (attr != NULL)
    image->draw_border =  data_boolean(attribute_first_data(attr), ctx);

  image->keep_aspect = TRUE;
  attr = object_find_attribute(obj_node, "keep_aspect");
  if (attr != NULL)
    image->keep_aspect =  data_boolean(attribute_first_data(attr), ctx);

  attr = object_find_attribute(obj_node, "file");
  if (attr != NULL) {
    image->file =  data_filename(attribute_first_data(attr), ctx);
  } else {
    image->file = g_strdup("");
  }

  element_init(elem, 8, NUM_CONNECTIONS);

  for (i=0;i<NUM_CONNECTIONS;i++) {
    obj->connections[i] = &image->connections[i];
    image->connections[i].object = obj;
    image->connections[i].connected = NULL;
  }
  image->connections[8].flags = CP_FLAGS_MAIN;

  image->image = NULL;
  
  if (strcmp(image->file, "")!=0) {
    diafile_dir = get_directory(dia_context_get_filename(ctx));

    if (g_path_is_absolute(image->file)) { /* Absolute pathname */
      image->image = dia_image_load(image->file);
      if (image->image == NULL) {
	/* Not found as abs path, try in same dir as diagram. */
	char *temp_string;
	const char *image_file_name = image->file;
	const char *psep;

	psep = strrchr(image->file, G_DIR_SEPARATOR);
	/* try the other G_OS as well */
	if (!psep)
	  psep =  strrchr(image->file, G_DIR_SEPARATOR == '/' ? '\\' : '/');
	if (psep)
	  image_file_name = psep + 1;

	temp_string = g_build_filename(diafile_dir, image_file_name, NULL);

	image->image = dia_image_load(temp_string);

	if (image->image != NULL) {
	  /* Found file in same dir as diagram. */
	  message_warning(_("The image file '%s' was not found in the specified directory.\n"
			  "Using the file '%s' instead.\n"), image->file, temp_string);
	  g_free(image->file);
	  image->file = temp_string;
	} else {
	  g_free(temp_string);
	  
	  image->image = dia_image_load((char *)image_file_name);
	  if (image->image != NULL) {
	    char *tmp;
	    /* Found file in current dir. */
	    message_warning(_("The image file '%s' was not found in the specified directory.\n"
			    "Using the file '%s' instead.\n"), image->file, image_file_name);
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

      temp_string = g_build_filename (diafile_dir, image->file, NULL);

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
  /* if we don't have an image yet try to recover it from inlined data */
  if (!image->image) {
    attr = object_find_attribute(obj_node, "pixbuf");
    if (attr != NULL) {
      GdkPixbuf *pixbuf = data_pixbuf (attribute_first_data(attr), ctx);

      if (pixbuf) {
	image->image = dia_image_new_from_pixbuf (pixbuf);
	image->inline_data = TRUE; /* avoid loosing it */
	/* FIXME: should we reset the filename? */
	g_object_unref (pixbuf);
      }
    }
  } else {
    attr = object_find_attribute(obj_node, "inline_data");
    if (attr != NULL)
      image->inline_data = data_boolean(attribute_first_data(attr), ctx);
    /* Should be set pixbuf, too? Or leave it till the first get. */
  }

  /* update mtime */
  {
    struct stat st;
    if (g_stat (image->file, &st) != 0)
      st.st_mtime = 0;

    image->mtime = st.st_mtime;
  }
  image_update_data(image);

  return &image->element.object;
}
