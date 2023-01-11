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

#include "config.h"

#include <glib/gi18n-lib.h>

#include <time.h>
#include <glib.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h> /* close */
#endif
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

  char *filename;
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
_dae_load (ObjectNode obj_node, int version, DiaContext *ctx);
static void
_dae_save (DiaObject *obj, ObjectNode obj_node, DiaContext *ctx);

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
  "Misc - Diagram",         /* name */
  0,                     /* version */
  diagram_as_element_xpm, /* pixmap */
  &_dae_type_ops,            /* ops */
  NULL,              /* pixmap_file */
  0            /* default_uder_data */
};

static void _dae_update_data (DiagramAsElement *dae);

static const char *_extensions[] = { "dia", NULL };

static PropDescription _dae_props[] = {
  ELEMENT_COMMON_PROPERTIES,
  PROP_STD_LINE_WIDTH,
  PROP_STD_LINE_COLOUR,
  PROP_STD_FILL_COLOUR,
  PROP_STD_SHOW_BACKGROUND,
  { "diagram_file", PROP_TYPE_FILE, PROP_FLAG_VISIBLE,
    N_("Diagram file"), NULL, /* extra_data */_extensions },
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
static DiaObjectChange*
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
static DiaObjectChange*
_dae_move(DiagramAsElement *dae, Point *to)
{
  dae->element.corner = *to;
  _dae_update_data(dae);

  return NULL;
}

static void
_dae_draw (DiagramAsElement *dae, DiaRenderer *renderer)
{
  Element *elem = &dae->element;

  if (!dae->data) {
    /* just draw the box */
    Point lower_right = {
      elem->corner.x + elem->width,
      elem->corner.y + elem->height
    };

    dia_renderer_draw_rect (renderer,
                            &elem->corner,
                            &lower_right,
                            NULL,
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
        char *imgfname = NULL;
        int fd = g_file_open_tmp ("diagram-as-elementXXXXXX.png", &imgfname, NULL);
        if (fd != -1) {
          DiaExportFilter *ef = filter_export_get_by_name ("cairo-alpha-png");
          if (!ef) { /* prefer cairo with alpha, but don't require it */
            ef = filter_guess_export_filter (imgfname);
          }
          close(fd);
          if (ef) {
            DiaContext *ctx = dia_context_new ("Diagram as Object");

            dia_context_set_filename (ctx, imgfname);
            if (ef->export_func (dae->data, ctx, imgfname, dae->filename, ef->user_data)) {
              DiaImage *tmp_image = dia_image_load (imgfname);

              /* some extra gymnastics to create an image w/o filename */
              if (tmp_image) {
                dae->image = dia_image_new_from_pixbuf ((GdkPixbuf *) dia_image_pixbuf (tmp_image));
                g_clear_object (&tmp_image);
              }
              /* FIXME: where to put the message in case of an error? */
              dia_context_release (ctx);
            }
          } /* found a filter */
          g_unlink (imgfname);
          g_clear_pointer (&imgfname, g_free);
        } /* temporary file created*/
      } /* only if we have no image yet */
      if (dae->image) {
        dia_renderer_draw_image (renderer,
                                 &elem->corner,
                                 elem->width,
                                 elem->height,
                                 dae->image);
      }
    }
  }
}


static void
_dae_update_data (DiagramAsElement *dae)
{
  GStatBuf statbuf;
  Element *elem = &dae->element;
  DiaObject *obj = &elem->object;
  static int working = 0;

  if (working > 2)
    return; /* protect against infinite recursion */
  ++working;

  if (   strlen (dae->filename)
      && g_stat (dae->filename, &statbuf) == 0
      && dae->mtime != statbuf.st_mtime) {
    DiaImportFilter *inf;

    g_clear_object (&dae->data);
    dae->data = g_object_new (DIA_TYPE_DIAGRAM_DATA, NULL);

    inf = filter_guess_import_filter(dae->filename);
    if (inf) {
      DiaContext *ctx = dia_context_new (diagram_as_element_type.name);

      dia_context_set_filename (ctx, dae->filename);
      if (inf->import_func(dae->filename, dae->data, ctx, inf->user_data)) {
        data_update_extents (dae->data); /* should already be called by importer? */
        dae->scale = dae->element.width / (dae->data->extents.right - dae->data->extents.left);
        dae->element.height = (dae->data->extents.bottom - dae->data->extents.top) * dae->scale;
        dae->mtime = statbuf.st_mtime;
      }
      /* FIXME: where to put the message in case of an error? */
      dia_context_release (ctx);
    }
    /* invalidate possibly cached image */
    g_clear_object (&dae->image);
  }
  /* fixme - fit the scale to draw the diagram in elements size ?*/
  if (dae->scale)
    dae->scale = dae->element.width / (dae->data->extents.right - dae->data->extents.left);

  elem->extra_spacing.border_trans = dae->border_line_width/2.0;
  element_update_boundingbox(elem);
  element_update_handles(elem);
  element_update_connections_rectangle(elem, dae->connections);

  /* adjust objects position, otherwise it'll jump on move */
  obj->position = elem->corner;

  --working;
}


static void
_dae_destroy(DiagramAsElement *dae)
{
  g_clear_object (&dae->data);

  g_clear_pointer (&dae->filename, g_free);

  g_clear_object (&dae->image);

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
_dae_load (ObjectNode obj_node, int version, DiaContext *ctx)
{
  DiaObject *obj;
  DiagramAsElement *dae;

  obj = object_load_using_properties (&diagram_as_element_type,
                                       obj_node, version, ctx);
  /* filename de-normalization */
  dae = (DiagramAsElement*)obj;
  if (strlen(dae->filename) && !g_path_is_absolute (dae->filename)) {
    char *dirname = g_path_get_dirname (dia_context_get_filename(ctx));
    char *fname = g_build_filename (dirname, dae->filename, NULL);
    g_clear_pointer (&dae->filename, g_free);
    dae->filename = fname;
    g_clear_pointer (&dirname, g_free);

    /* need to update again with new filenames */
    _dae_update_data(dae);
  }
  return obj;
}


static void
_dae_save (DiaObject *obj, ObjectNode obj_node, DiaContext *ctx)
{
  DiagramAsElement *dae;
  /* filename normalization */
  char *saved_path = NULL;

  dae = (DiagramAsElement*)obj;
  if (strlen(dae->filename) && g_path_is_absolute (dae->filename)) {
    char *dirname = g_path_get_dirname (dia_context_get_filename (ctx));
    if (strstr (dae->filename, dirname) == dae->filename) {
      saved_path = dae->filename;
      dae->filename += (strlen (dirname) + g_str_has_suffix (dirname, G_DIR_SEPARATOR_S) ? 0 : 1);
    }
    g_clear_pointer (&dirname, g_free);
  }
  object_save_using_properties (obj, obj_node, ctx);

  if (saved_path) {
    dae->filename = saved_path;
  }
}
