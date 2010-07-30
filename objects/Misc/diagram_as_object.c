/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * diagram_as_object.c -- embedding diagrams
 * Copyright (C) 2009  Hans Breuer, <Hans@Breuer.Org>
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

#include <time.h>
#ifdef HAVE_UTIME_H
#  include <utime.h>
#else
#  include <sys/utime.h>
#endif
#include <glib.h>
#ifdef G_OS_WIN32
#include <io.h> /* close */
#endif
#include <glib/gstdio.h>

#include "object.h"
#include "diarenderer.h"
#include "attributes.h"
#include "properties.h"
#include "boundingbox.h"
#include "element.h"
#include "diagramdata.h"
#include "filter.h"
#include "intl.h"

#include "filter.h"
#include "dia_image.h"

#include "pixmaps/diagram_as_element.xpm"

#define DEFAULT_WIDTH 2.0
#define DEFAULT_HEIGHT 2.0
#define NUM_CONNECTIONS 9
/* Object definition */
typedef struct _DiagramAsElement {
  Element element;

  ConnectionPoint connections[NUM_CONNECTIONS];

  Color border_color;
  real border_line_width;
  Color inner_color;
  gboolean show_background;

  gchar *filename;
  time_t mtime;
  DiagramData *data;

  /* for indirect rendering*/
  DiaImage *image;
  
  real scale;
} DiagramAsElement;

static DiaObject *
_dae_create (Point *startpoint,
	     void *user_data,
	     Handle **handle1,
	     Handle **handle2);
static DiaObject *
_dae_load (ObjectNode obj_node, int version, const char *filename);
static void
_dae_save (DiaObject *obj, ObjectNode obj_node, const char *filename);

static ObjectTypeOps _dae_type_ops =
{
  (CreateFunc) _dae_create,
  (LoadFunc)   _dae_load, /* can't use object_load_using_properties, signature mismatch */
  (SaveFunc)   _dae_save, /* overwrite for filename normalization */
  (GetDefaultsFunc)   NULL,
  (ApplyDefaultsFunc) NULL
};

DiaObjectType diagram_as_element_type =
{
  "Misc - Diagram",  /* name */
  0,                 /* version */
  (char **) diagram_as_element_xpm, /* pixmap */
  &_dae_type_ops,     /* ops */
  NULL,              /* pixmap_file */
  0                  /* default_uder_data */
};

static void _dae_update_data (DiagramAsElement *dae);

static PropDescription _dae_props[] = {
  ELEMENT_COMMON_PROPERTIES,
  PROP_STD_LINE_WIDTH,
  PROP_STD_LINE_COLOUR,
  PROP_STD_FILL_COLOUR,
  PROP_STD_SHOW_BACKGROUND,
  { "diagram_file", PROP_TYPE_FILE, PROP_FLAG_VISIBLE,
    N_("Diagram file"), NULL, NULL},
  PROP_DESC_END
};
static PropDescription *
_dae_describe_props(DiagramAsElement *dae)
{
  if (_dae_props[0].quark == 0)
    prop_desc_list_calculate_quarks(_dae_props);
  return _dae_props;
}
static PropOffset _dae_offsets[] = {
  ELEMENT_COMMON_PROPERTIES_OFFSETS,
  { PROP_STDNAME_LINE_WIDTH, PROP_STDTYPE_LINE_WIDTH, offsetof(DiagramAsElement, border_line_width) },
  { "line_colour", PROP_TYPE_COLOUR, offsetof(DiagramAsElement, border_color) },
  { "fill_colour", PROP_TYPE_COLOUR, offsetof(DiagramAsElement, inner_color) },
  { "show_background", PROP_TYPE_BOOL,offsetof(DiagramAsElement, show_background) },
  { "diagram_file", PROP_TYPE_FILE, offsetof(DiagramAsElement, filename) },
  { NULL }
};
static void
_dae_get_props(DiagramAsElement *dae, GPtrArray *props)
{  
  object_get_props_from_offsets(&dae->element.object, _dae_offsets, props);
}
static void
_dae_set_props(DiagramAsElement *dae, GPtrArray *props)
{
  object_set_props_from_offsets(&dae->element.object, _dae_offsets, props);
  _dae_update_data(dae);
}
static real
_dae_distance_from(DiagramAsElement *dae, Point *point)
{
  DiaObject *obj = &dae->element.object;
  return distance_rectangle_point(&obj->bounding_box, point);
}
static void
_dae_select(DiagramAsElement *dae, Point *clicked_point, DiaRenderer *interactive_renderer)
{
  element_update_handles(&dae->element);
}
static ObjectChange*
_dae_move_handle(DiagramAsElement *dae, Handle *handle,
		 Point *to, ConnectionPoint *cp, 
		 HandleMoveReason reason, ModifierKeys modifiers)
{
  Element *elem = &dae->element;
  real aspect = elem->width / elem->height;

  g_assert(handle!=NULL);
  g_assert(to!=NULL);

  element_move_handle_aspect(&dae->element, handle->id, to, /*cp, reason, modifiers,*/ aspect);
  _dae_update_data(dae);

  return NULL;
}
static ObjectChange*
_dae_move(DiagramAsElement *dae, Point *to)
{
  dae->element.corner = *to;
  _dae_update_data(dae);

  return NULL;
}
_dae_draw(DiagramAsElement *dae, DiaRenderer *renderer)
{
  DiaRendererClass *renderer_ops = DIA_RENDERER_GET_CLASS (renderer);
  Element *elem = &dae->element;

  if (!dae->data) {
    /* just draw the box */
    Point lower_right = {
      elem->corner.x + elem->width,
      elem->corner.y + elem->height 
    };

    renderer_ops->draw_rect(renderer,&elem->corner, &lower_right, 
                            &dae->border_color);

  } else {
    if (FALSE) {
      /* if the renderer supports transformations ... */
      /* temporary messing with it (not enough) */
      dae->data->paper.scaling *= dae->scale;
      data_render (dae->data, DIA_RENDERER (renderer), NULL, NULL, NULL);
      dae->data->paper.scaling /= dae->scale;
    } else {
      /* we have to render to an image and draw that */
      if (!dae->image) { /* lazy creation */
	gchar *imgfname = NULL;
	gint fd = g_file_open_tmp ("diagram-as-elementXXXXXX.png", &imgfname, NULL);
	if (fd != -1) {
          DiaExportFilter *ef = filter_get_by_name ("cairo-alpha-png");
	  if (!ef) /* prefer cairo with alpha, but don't require it */
	    ef = filter_guess_export_filter (imgfname);
	  close(fd);
	  if (ef) {
	    ef->export_func (dae->data, imgfname, dae->filename, ef->user_data);
	    /* TODO: change export_func to return success or GError* */
	    dae->image = dia_image_load (imgfname);
	  }
	  g_unlink (imgfname);
	  g_free (imgfname);
	}
      }
      if (dae->image)
	renderer_ops->draw_image (renderer, &elem->corner, elem->width, elem->height, dae->image);
    }
  }
}
static void
_dae_update_data(DiagramAsElement *dae)
{
  struct utimbuf utbuf;
  Element *elem = &dae->element;
  DiaObject *obj = &elem->object;
  
  if (   strlen(dae->filename)
#if GLIB_CHECK_VERSION(2,18,0)
      && g_utime(dae->filename, &utbuf) == 0
#else
      && utime(dae->filename, &utbuf) == 0
#endif
      && dae->mtime != utbuf.modtime) {
    DiaImportFilter *inf;

    if (dae->data)
      g_object_unref(dae->data);
    dae->data = g_object_new (DIA_TYPE_DIAGRAM_DATA, NULL);

    inf = filter_guess_import_filter(dae->filename);
    if (inf && inf->import_func(dae->filename, dae->data, inf->user_data)) {
      dae->scale = dae->element.width / (dae->data->extents.right - dae->data->extents.left);
      dae->element.height = (dae->data->extents.bottom - dae->data->extents.top) * dae->scale;
      dae->mtime = utbuf.modtime;
    }
    /* invalidate possibly cached image */
    if (dae->image) {
      g_object_unref (dae->image);
      dae->image = NULL;
    }
  }
  /* fixme - fit the scale to draw the diagram in elements size ?*/
  if (dae->scale)
    dae->scale = dae->element.width / (dae->data->extents.right - dae->data->extents.left);

  elem->extra_spacing.border_trans = dae->border_line_width/2.0;
  element_update_boundingbox(elem);
  element_update_handles(elem);

  /* adjust objects position, otherwise it'll jump on move */
  obj->position = elem->corner;
}
static void 
_dae_destroy(DiagramAsElement *dae) 
{
  if (dae->data)
    g_object_unref(dae->data);

  g_free(dae->filename);
  
  if (dae->image)
    g_object_unref (dae->image);

  element_destroy(&dae->element);
}

static ObjectOps _dae_ops = {
  (DestroyFunc)         _dae_destroy,
  (DrawFunc)            _dae_draw,
  (DistanceFunc)        _dae_distance_from,
  (SelectFunc)          _dae_select,
  (CopyFunc)            object_copy_using_properties,
  (MoveFunc)            _dae_move,
  (MoveHandleFunc)      _dae_move_handle,
  (GetPropertiesFunc)   object_create_props_dialog,
  (ApplyPropertiesDialogFunc) object_apply_props_from_dialog,
  (ObjectMenuFunc)      NULL,
  (DescribePropsFunc)   _dae_describe_props,
  (GetPropsFunc)        _dae_get_props,
  (SetPropsFunc)        _dae_set_props,
  (TextEditFunc) 0,
  (ApplyPropertiesListFunc) object_apply_props,
};

/*! factory function */
static DiaObject *
_dae_create (Point *startpoint,
	     void *user_data,
	     Handle **handle1,
	     Handle **handle2)
{
  DiagramAsElement *dae;
  Element *elem;
  DiaObject *obj;
  int i;

  dae = g_new0(DiagramAsElement, 1);

  obj = &dae->element.object;
  obj->type = &diagram_as_element_type;
  obj->ops = &_dae_ops;

  elem = &dae->element;
  elem->corner = *startpoint;
  elem->width = DEFAULT_WIDTH;
  elem->height = DEFAULT_HEIGHT;
  
  element_init(elem, 8, NUM_CONNECTIONS);

  for (i=0; i<NUM_CONNECTIONS; i++) {
    obj->connections[i] = &dae->connections[i];
    dae->connections[i].object = obj;
    dae->connections[i].connected = NULL;
  }
  dae->connections[8].flags = CP_FLAGS_MAIN;

  dae->filename = g_strdup("");

  _dae_update_data(dae);

  *handle1 = NULL;
  *handle2 = obj->handles[7];  
  return &dae->element.object;
}

static DiaObject *
_dae_load (ObjectNode obj_node, int version, const char *filename)
{
  DiaObject *obj;
  DiagramAsElement *dae;
 
  obj = object_load_using_properties (&diagram_as_element_type,
                                       obj_node, version, filename);
  /* filename de-normalization */
  dae = (DiagramAsElement*)obj;
  if (strlen(dae->filename) && !g_path_is_absolute (dae->filename)) {
    gchar *dirname = g_path_get_dirname (filename);
    gchar *filename = g_build_filename (dirname, dae->filename, NULL);
    g_free (dae->filename);
    dae->filename = filename;
    g_free (dirname);    
  }
  return obj;
}

static void
_dae_save (DiaObject *obj, ObjectNode obj_node, const char *filename)
{
  DiagramAsElement *dae;
  /* filename normalization */
  gchar *saved_path = NULL;

  dae = (DiagramAsElement*)obj;
  if (strlen(dae->filename) && g_path_is_absolute (dae->filename)) {
    gchar *dirname = g_path_get_dirname (filename);
    if (strstr (dae->filename, dirname) == dae->filename) {
      saved_path = dae->filename;
      dae->filename += (strlen (dirname) + 1);
    }
    g_free (dirname);
  }
  object_save_using_properties (obj, obj_node, diagram_as_element_type.version, filename);
  
  if (saved_path) {
    dae->filename = saved_path;
  }
}
