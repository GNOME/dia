/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998, 1999 Alexander Larsson
 *
 * Custom Objects -- objects defined in XML rather than C.
 * Copyright (C) 1999 James Henstridge.
 *
 * Non-uniform scaling/subshape support by Marcel Toele.
 * Modifications (C) 2007 Kern Automatiseringsdiensten BV.
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

#include <gmodule.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "shape_info.h"
#include "object.h"
#include "element.h"
#include "connectionpoint.h"
#include "diarenderer.h"
#include "attributes.h"
#include "text.h"
#include "sheet.h"
#include "properties.h"
#include "dia_image.h"
#include "custom_object.h"
#include "prefs.h"

#include "pixmaps/custom.xpm"

/* used when resizing to decide which side of the shape to expand/shrink */
typedef enum {
  ANCHOR_MIDDLE,
  ANCHOR_START,
  ANCHOR_END
} AnchorShape;

typedef struct _Custom Custom;

/*!
 * \brief Custom shape representation as DiaObject
 *
 * The custom shape object gets created with varying content given from the
 * shape file. It currently only supports two different sets of properties,
 * depending on the use of \code <textbox/> \endcode .
 *
 * \extends _Element
 * \ingroup ObjectCustom
 */
struct _Custom {
  Element element;

  /*! ShapeInfo giving the object it's drawing info */
  ShapeInfo *info;
  /*! transformation coords */
  double xscale, yscale;
  double xoffs,  yoffs;

  /*!
   * The sub-scale variables
   * The old_subscale is used for interactive
   * (shift-pressed) scaling
   * @{
   */
  double subscale;
  double old_subscale;
  /*! @} */
  /*!
   * \brief Pointer changing during the drawing of the display list
   * this is sort of a hack, passing a temporary value
   * using this field, but otherwise
   * sub-shapes are going to need major code refactoring:
   */
  GraphicElementSubShape* current_subshape;
  /*! Connection points need to be dynamically allocated */
  ConnectionPoint *connections;
  /*! width calculate from line_width */
  double border_width;
  Color border_color;
  Color inner_color;
  gboolean show_background;
  DiaLineStyle line_style;
  double dashlength;

  gboolean flip_h, flip_v;

  Text *text;
  double padding;

  DiaTextFitting text_fitting;
};


static double           custom_distance_from (Custom           *custom,
                                              Point            *point);
static void             custom_select        (Custom           *custom,
                                              Point            *clicked_point,
                                              DiaRenderer      *interactive_renderer);
static DiaObjectChange *custom_move_handle   (Custom           *custom,
                                              Handle           *handle,
                                              Point            *to,
                                              ConnectionPoint  *cp,
                                              HandleMoveReason  reason,
                                              ModifierKeys      modifiers);
static DiaObjectChange *custom_move          (Custom           *custom,
                                              Point            *to);
static void             custom_draw                (Custom           *custom,
                                                    DiaRenderer      *renderer);
static void             custom_draw_displaylist    (GList            *display_list,
                                                    Custom           *custom,
                                                    DiaRenderer      *renderer,
                                                    GArray           *arr,
                                                    GArray           *barr,
                                                    double           *cur_line,
                                                    double           *cur_dash,
                                                    DiaLineCaps      *cur_caps,
                                                    DiaLineJoin      *cur_join,
                                                    DiaLineStyle     *cur_style);
static void             custom_draw_element        (GraphicElement   *el,
                                                    Custom           *custom,
                                                    DiaRenderer      *renderer,
                                                    GArray           *arr,
                                                    GArray           *barr,
                                                    double           *cur_line,
                                                    double           *cur_dash,
                                                    DiaLineCaps      *cur_caps,
                                                    DiaLineJoin      *cur_join,
                                                    DiaLineStyle     *cur_style,
                                                    Color            *fg,
                                                    Color            *bg);
static void custom_update_data(Custom *custom, AnchorShape h, AnchorShape v);
static void custom_reposition_text(Custom *custom, GraphicElementText *text);
static DiaObject *custom_create(Point *startpoint,
				void *user_data,
				Handle **handle1,
				Handle **handle2);
static void custom_destroy(Custom *custom);
static DiaObject *custom_copy(Custom *custom);
static DiaMenu *custom_get_object_menu(Custom *custom, Point *clickedpoint);

static PropDescription *custom_describe_props(Custom *custom);
static void custom_get_props(Custom *custom, GPtrArray *props);
static void custom_set_props(Custom *custom, GPtrArray *props);

static DiaObject *custom_load_using_properties(ObjectNode obj_node, int version,DiaContext *ctx);

static ObjectTypeOps custom_type_ops =
  {
    (CreateFunc) custom_create,
    (LoadFunc)   custom_load_using_properties,
    (SaveFunc)   object_save_using_properties,
    (GetDefaultsFunc)   NULL,
    (ApplyDefaultsFunc) NULL
  };

/* This looks like it could be static, but it can't because we key
   on it to determine if an DiaObjectType is a custom/SVG shape */
G_MODULE_EXPORT
DiaObjectType custom_type =
  {
    "Custom - Generic",   /* name */
    /* version 0 had no configurable (text-)padding */
    1,                 /* version */
    custom_xpm,         /* pixmap */
    &custom_type_ops       /* ops */
  };

static ObjectOps custom_ops = {
  (DestroyFunc)         custom_destroy,
  (DrawFunc)            custom_draw,
  (DistanceFunc)        custom_distance_from,
  (SelectFunc)          custom_select,
  (CopyFunc)            custom_copy,
  (MoveFunc)            custom_move,
  (MoveHandleFunc)      custom_move_handle,
  (GetPropertiesFunc)   object_create_props_dialog,
  (ApplyPropertiesDialogFunc) object_apply_props_from_dialog,
  (ObjectMenuFunc)      custom_get_object_menu,
  (DescribePropsFunc)   custom_describe_props,
  (GetPropsFunc)        custom_get_props,
  (SetPropsFunc)        custom_set_props,
  (TextEditFunc) 0,
  (ApplyPropertiesListFunc) object_apply_props,
};

static PropDescription custom_props[] = {
  ELEMENT_COMMON_PROPERTIES,
  PROP_STD_LINE_WIDTH_OPTIONAL,
  PROP_STD_LINE_COLOUR_OPTIONAL,
  PROP_STD_FILL_COLOUR_OPTIONAL,
  PROP_STD_SHOW_BACKGROUND_OPTIONAL,
  PROP_STD_LINE_STYLE_OPTIONAL,
  { "flip_horizontal", PROP_TYPE_BOOL, PROP_FLAG_VISIBLE|PROP_FLAG_OPTIONAL,
    N_("Flip horizontal"), NULL, NULL },
  { "flip_vertical", PROP_TYPE_BOOL, PROP_FLAG_VISIBLE|PROP_FLAG_OPTIONAL,
    N_("Flip vertical"), NULL, NULL },

  { "subscale", PROP_TYPE_REAL, PROP_FLAG_OPTIONAL,
    N_("Scale of the sub-shapes"), NULL, NULL },
  PROP_DESC_END
};

static PropNumData text_padding_data = { 0.0, 10.0, 0.1 };

static PropDescription custom_props_text[] = {
  ELEMENT_COMMON_PROPERTIES,
  PROP_STD_LINE_WIDTH_OPTIONAL,
  PROP_STD_LINE_COLOUR_OPTIONAL,
  PROP_STD_FILL_COLOUR_OPTIONAL,
  PROP_STD_SHOW_BACKGROUND_OPTIONAL,
  PROP_STD_LINE_STYLE_OPTIONAL,
  { "padding", PROP_TYPE_REAL, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
    N_("Text padding"), NULL, &text_padding_data },
  PROP_STD_TEXT_ALIGNMENT,
  PROP_STD_TEXT_FONT,
  PROP_STD_TEXT_HEIGHT,
  PROP_STD_TEXT_COLOUR,
  /* BEWARE: the following makes the whole Text optional during load. Normally this
   * would leave the object in an inconsistent state but here we have a proper default
   * initialization even in the load case. See custom_load_using_properties()  --hb
   */
  { "text", PROP_TYPE_TEXT, PROP_FLAG_OPTIONAL,
    N_("Text"), NULL, NULL },
  PROP_STD_TEXT_FITTING,

  { "flip_horizontal", PROP_TYPE_BOOL, PROP_FLAG_VISIBLE|PROP_FLAG_OPTIONAL,
    N_("Flip horizontal"), NULL, NULL },
  { "flip_vertical", PROP_TYPE_BOOL, PROP_FLAG_VISIBLE|PROP_FLAG_OPTIONAL,
    N_("Flip vertical"), NULL, NULL },

  { "subscale", PROP_TYPE_REAL, PROP_FLAG_OPTIONAL,
    N_("Scale of the sub-shapes"), NULL, NULL },
  PROP_DESC_END
};

static PropOffset custom_offsets[] = {
  ELEMENT_COMMON_PROPERTIES_OFFSETS,
  { PROP_STDNAME_LINE_WIDTH, PROP_STDTYPE_LINE_WIDTH, offsetof(Custom, border_width) },
  { "line_colour", PROP_TYPE_COLOUR, offsetof(Custom, border_color) },
  { "fill_colour", PROP_TYPE_COLOUR, offsetof(Custom, inner_color) },
  { "show_background", PROP_TYPE_BOOL, offsetof(Custom, show_background) },
  { "line_style", PROP_TYPE_LINESTYLE,
    offsetof(Custom, line_style), offsetof(Custom, dashlength) },
  { "flip_horizontal", PROP_TYPE_BOOL, offsetof(Custom, flip_h) },
  { "flip_vertical", PROP_TYPE_BOOL, offsetof(Custom, flip_v) },
  { "subscale", PROP_TYPE_REAL, offsetof(Custom, subscale) },
  { NULL, 0, 0 }
};

static PropOffset custom_offsets_text[] = {
  ELEMENT_COMMON_PROPERTIES_OFFSETS,
  { PROP_STDNAME_LINE_WIDTH, PROP_STDTYPE_LINE_WIDTH, offsetof(Custom, border_width) },
  { "line_colour", PROP_TYPE_COLOUR, offsetof(Custom, border_color) },
  { "fill_colour", PROP_TYPE_COLOUR, offsetof(Custom, inner_color) },
  { "show_background", PROP_TYPE_BOOL, offsetof(Custom, show_background) },
  { "line_style", PROP_TYPE_LINESTYLE,
    offsetof(Custom, line_style), offsetof(Custom, dashlength) },
  { "flip_horizontal", PROP_TYPE_BOOL, offsetof(Custom, flip_h) },
  { "flip_vertical", PROP_TYPE_BOOL, offsetof(Custom, flip_v) },
  { "subscale", PROP_TYPE_REAL, offsetof(Custom, subscale) },
  { "padding", PROP_TYPE_REAL, offsetof(Custom, padding) },
  {"text",PROP_TYPE_TEXT,offsetof(Custom,text)},
  {"text_font",PROP_TYPE_FONT,offsetof(Custom,text),offsetof(Text,font)},
  {PROP_STDNAME_TEXT_HEIGHT, PROP_STDTYPE_TEXT_HEIGHT,offsetof(Custom,text),offsetof(Text,height)},
  {"text_colour",PROP_TYPE_COLOUR,offsetof(Custom,text),offsetof(Text,color)},
  {"text_alignment",PROP_TYPE_ENUM,offsetof(Custom,text),offsetof(Text,alignment)},
  {PROP_STDNAME_TEXT_FITTING,PROP_TYPE_ENUM,offsetof(Custom,text_fitting)},
  { NULL, 0, 0 }
};

void
custom_setup_properties (ShapeInfo *info, xmlNodePtr node)
{
  xmlChar *str;
  xmlNodePtr cur;
  int n_props, offs = 0;
  int i;

  /* count ext_attributes node items */
  if (node)
  {
    for (i = 0, cur = node->xmlChildrenNode; cur != NULL; cur = cur->next)
    {
      if ((!xmlIsBlankNode(cur)) && (cur->type == XML_ELEMENT_NODE))
	i++;
    }
    info->n_ext_attr = i;
  }

  /* create prop tables & initialize constant part */
  if (info->has_text)
  {
    n_props = sizeof (custom_props_text) / sizeof (PropDescription);
    info->props = g_new0 (PropDescription, n_props + info->n_ext_attr);
    memcpy (info->props, custom_props_text, sizeof (custom_props_text));
    info->prop_offsets = g_new0 (PropOffset, n_props + info->n_ext_attr);
    memcpy (info->prop_offsets, custom_offsets_text, sizeof (custom_offsets_text));
  }
  else
  {
    n_props = sizeof (custom_props) / sizeof (PropDescription);
    info->props = g_new0 (PropDescription, n_props + info->n_ext_attr);
    memcpy (info->props, custom_props, sizeof (custom_props));
    info->prop_offsets = g_new0 (PropOffset, n_props + info->n_ext_attr);
    memcpy (info->prop_offsets, custom_offsets, sizeof (custom_offsets));
  }

  if (node)
  {
    offs = sizeof (Custom);
    /* walk ext_attributes node ... */
    for (i = n_props-1, node = node->xmlChildrenNode; node != NULL; node = node->next)
    {
      if (xmlIsBlankNode(node))
	continue;
      if (node->type != XML_ELEMENT_NODE)
	continue;
      if (!xmlStrcmp(node->name, (const xmlChar *) "ext_attribute")) {
      char *pname, *ptype = 0;

      str = xmlGetProp(node, (const xmlChar *)"name");
      if (!str) {
        continue;
      }
      pname = g_strdup((char *) str);
      xmlFree(str);

      str = xmlGetProp(node, (const xmlChar *)"type");
      if (!str) {
        g_clear_pointer (&pname, g_free);
        continue;
      }
      ptype = g_strdup((char *) str);
      xmlFree(str);

      /* we got here, then fill an entry */
      info->props[i].name = g_strdup_printf ("custom:%s", pname);
      info->props[i].type = ptype;
      info->props[i].flags = PROP_FLAG_VISIBLE|PROP_FLAG_OPTIONAL;

        str = xmlGetProp (node, (const xmlChar *) "description");
        if (str) {
          g_clear_pointer (&pname, g_free);
          pname = g_strdup ((char *) str);
          xmlFree (str);
        }
        info->props[i++].description = pname;
      }
    }
  }

  prop_desc_list_calculate_quarks (info->props);

  /* 2nd pass after quarks & ops have been filled in */
  for (i = n_props-1; i < n_props-1+info->n_ext_attr; i++)
    if ((info->props[i].ops) && (info->props[i].ops->get_data_size))
    { /* if prop valid & supported */
      int size;
      info->prop_offsets[i].name = info->props[i].name;
      info->prop_offsets[i].type = info->props[i].type;
      info->prop_offsets[i].offset = offs;
      size = info->props[i].ops->get_data_size ();
      info->ext_attr_size += size;
      offs += size;
    }
    else
    {
      /* hope this is enough to have this prop ignored */
      info->props[i].flags = PROP_FLAG_DONT_SAVE | PROP_FLAG_OPTIONAL;
    }
}

static PropDescription *
custom_describe_props(Custom *custom)
{
  return custom->info->props;
}

static void
custom_get_props(Custom *custom, GPtrArray *props)
{
  object_get_props_from_offsets(&custom->element.object,
                                custom->info->prop_offsets,props);
}

static void
custom_set_props(Custom *custom, GPtrArray *props)
{
  object_set_props_from_offsets(&custom->element.object,
                                custom->info->prop_offsets,props);
  custom_update_data(custom, ANCHOR_MIDDLE, ANCHOR_MIDDLE);
}


static void
transform_subshape_coord (Custom                 *custom,
                          GraphicElementSubShape *subshape,
                          const Point            *p1,
                          Point                  *out)
{
  double scale;
  double width;
  double height;
  double xoffs;
  double yoffs;
  double cx;
  double cy;
  DiaRectangle orig_bounds, new_bounds;

  if (subshape->default_scale == 0.0) {
    ShapeInfo *info = custom->info;
    real svg_width = info->shape_bounds.right - info->shape_bounds.left;
    real svg_height = info->shape_bounds.bottom - info->shape_bounds.top;
    real h_scale = info->default_height / svg_height;
    real v_scale = info->default_width / svg_width;

    subshape->default_scale = (v_scale > h_scale ? h_scale : v_scale);
  }

  scale = custom->subscale * subshape->default_scale;

  xoffs = custom->xoffs;
  yoffs = custom->yoffs;

  /* step 1: calculate boundaries */
  orig_bounds = custom->info->shape_bounds;

  /* step 2: undo unkown/funky number magic when flip_h or flip_v is set */
  if(custom->flip_h) custom->xscale = -custom->xscale;
  if(custom->flip_v) custom->yscale = -custom->yscale;

  new_bounds.top = orig_bounds.top * custom->yscale;
  new_bounds.bottom = orig_bounds.bottom * custom->yscale;
  new_bounds.left = orig_bounds.left * custom->xscale;
  new_bounds.right = orig_bounds.right * custom->xscale;

  width = new_bounds.right - new_bounds.left;
  height = new_bounds.bottom - new_bounds.top;

  /* step #3: calculate the new center */
  if (subshape->h_anchor_method == OFFSET_METHOD_PROPORTIONAL) {
    /* handle proportional offset in x direction */
    cx = subshape->center.x * custom->xscale;
  } else {
    /* handle fixed offset in x direction */
    if (subshape->h_anchor_method < 0) {
      cx = new_bounds.right - ((orig_bounds.right-subshape->center.x) * scale);
    } else {
      cx = new_bounds.left + (subshape->center.x * scale);
    }
  }

  if (subshape->v_anchor_method == OFFSET_METHOD_PROPORTIONAL) {
    /* handle proportional offset in y direction */
    cy = subshape->center.y * custom->yscale;
  } else {
    /* handle fixed offset in y direction */
    if (subshape->v_anchor_method < 0) {
      cy = new_bounds.bottom - ((orig_bounds.bottom-subshape->center.y) * scale);
    } else {
      cy = new_bounds.top + (subshape->center.y * scale);
   }
  }

  /* step #4: calculate the new coordinate relative to the calculated center */
  out->x = cx - ((subshape->center.x - p1->x) * scale);
  out->y = cy - ((subshape->center.y - p1->y) * scale);

  /* step #5: handle flip_h/flip_v */
  if (custom->flip_h) {
    out->x = -out->x + width;

    xoffs -= (new_bounds.right - new_bounds.left);
    custom->xscale = -custom->xscale; /* undo the damage we've done above */
  }
  if (custom->flip_v) {
    out->y = -out->y + height;

    yoffs -= (new_bounds.bottom - new_bounds.top);
    custom->yscale = -custom->yscale; /* undo the damage we've done above */
  }

  /* step #6: finally, translate the coordinate to the correct offset */
  out->x += xoffs;
  out->y += yoffs;
}

static real
custom_transform_length(Custom *custom, real length)
{
  if (custom->current_subshape != NULL) {
    GraphicElementSubShape* subshape = custom->current_subshape;
    g_assert (custom->subscale > 0.0 && subshape->default_scale > 0.0);
    return custom->subscale * subshape->default_scale * length;
  } else {
    /* maybe we should consider the direction? */
    return sqrt (fabs(custom->xscale * custom->yscale)) * length;
  }
}

static void
transform_size(Custom *custom, real w1, real h1, real *w2, real *h2)
{
  if (custom->current_subshape != NULL) {
    GraphicElementSubShape* subshape = custom->current_subshape;
    g_assert (custom->subscale > 0.0 && subshape->default_scale > 0.0);
    if (w2)
      *w2 = w1 * custom->subscale * subshape->default_scale;
    if (h2)
      *h2 = h1 * custom->subscale * subshape->default_scale;
  } else {
    if (w2)
      *w2 = w1 * fabs(custom->xscale);
    if (h2)
      *h2 = h1 * fabs(custom->yscale);
  }
}

static void
transform_coord(Custom *custom, const Point *p1, Point *out)
{
  if (custom->current_subshape != NULL) {
    transform_subshape_coord(custom, custom->current_subshape, p1, out);
  } else {
    out->x = p1->x * custom->xscale + custom->xoffs;
    out->y = p1->y * custom->yscale + custom->yoffs;
  }
}

static void
transform_rect(Custom *custom, const DiaRectangle *r1, DiaRectangle *out)
{
  real coord;
  out->left   = r1->left   * custom->xscale + custom->xoffs;
  out->right  = r1->right  * custom->xscale + custom->xoffs;
  out->top    = r1->top    * custom->yscale + custom->yoffs;
  out->bottom = r1->bottom * custom->yscale + custom->yoffs;

  if (out->left > out->right) {
    coord = out->left;
    out->left = out->right;
    out->right = coord;
  }
  if (out->top > out->bottom) {
    coord = out->top;
    out->top = out->bottom;
    out->bottom = coord;
  }
}


static double
custom_distance_from (Custom *custom, Point *point)
{
  static GArray *arr = NULL, *barr = NULL;
  Point p1, p2;
  DiaRectangle rect;
  gint i;
  GList *tmp;
  real min_dist = G_MAXFLOAT, dist = G_MAXFLOAT;

  if (!arr) {
    arr = g_array_new (FALSE, FALSE, sizeof(Point));
  }
  if (!barr) {
    barr = g_array_new (FALSE, FALSE, sizeof(BezPoint));
  }

  for (tmp = custom->info->display_list; tmp != NULL; tmp = tmp->next) {
    GraphicElement *el = tmp->data;
    real line_width = el->any.s.line_width * custom->border_width;

    switch (el->type) {
      case GE_LINE:
        transform_coord (custom, &el->line.p1, &p1);
        transform_coord (custom, &el->line.p2, &p2);
        dist = distance_line_point (&p1, &p2, line_width, point);
        break;
      case GE_POLYLINE:
        transform_coord (custom, &el->polyline.points[0], &p1);
        dist = G_MAXFLOAT;
        for (i = 1; i < el->polyline.npoints; i++) {
          real seg_dist;

          transform_coord (custom, &el->polyline.points[i], &p2);
          seg_dist = distance_line_point (&p1, &p2, line_width, point);
          p1 = p2;
          dist = MIN (dist, seg_dist);
          if (dist == 0.0) {
            break;
          }
        }
        break;
      case GE_POLYGON:
        g_array_set_size (arr, el->polygon.npoints);
        for (i = 0; i < el->polygon.npoints; i++) {
          transform_coord (custom, &el->polygon.points[i],
                           &g_array_index (arr, Point, i));
        }
        dist = distance_polygon_point ((Point *)arr->data, el->polygon.npoints,
              line_width, point);
        break;
      case GE_RECT:
        transform_coord (custom, &el->rect.corner1, &p1);
        transform_coord (custom, &el->rect.corner2, &p2);
        if (p1.x < p2.x) {
          rect.left = p1.x - line_width/2;  rect.right = p2.x + line_width/2;
        } else {
          rect.left = p2.x - line_width/2;  rect.right = p1.x + line_width/2;
        }
        if (p1.y < p2.y) {
          rect.top = p1.y - line_width/2;  rect.bottom = p2.y + line_width/2;
        } else {
          rect.top = p2.y - line_width/2;  rect.bottom = p1.y + line_width/2;
        }
        dist = distance_rectangle_point (&rect, point);
        break;
      case GE_IMAGE:
        p2.x = el->image.topleft.x + el->image.width;
        p2.y = el->image.topleft.y + el->image.height;
        transform_coord (custom, &el->image.topleft, &p1);
        transform_coord (custom, &p2, &p2);

        rect.left   = p1.x;
        rect.top    = p1.y;
        rect.right  = p2.x;
        rect.bottom = p2.y;
        dist = distance_rectangle_point (&rect, point);
        break;
      case GE_TEXT:
        text_set_height (el->text.object,
                         custom_transform_length (custom, el->text.s.font_height));
        custom_reposition_text (custom, &el->text);
        dist = text_distance_from (el->text.object, point);
        text_set_position (el->text.object, &el->text.anchor);
        break;
      case GE_ELLIPSE:
        transform_coord (custom, &el->ellipse.center, &p1);
        dist = distance_ellipse_point (&p1,
                                       el->ellipse.width * fabs (custom->xscale),
                                       el->ellipse.height * fabs (custom->yscale),
                                       line_width, point);
        break;
      case GE_PATH:
        g_array_set_size (barr, el->path.npoints);
        for (i = 0; i < el->path.npoints; i++) {
          switch (g_array_index (barr,BezPoint,i).type = el->path.points[i].type) {
            case BEZ_CURVE_TO:
              transform_coord (custom,
                               &el->path.points[i].p3,
                               &g_array_index (barr, BezPoint, i).p3);
              transform_coord (custom,
                               &el->path.points[i].p2,
                               &g_array_index (barr, BezPoint, i).p2);
            case BEZ_MOVE_TO:
            case BEZ_LINE_TO:
              transform_coord (custom,
                               &el->path.points[i].p1,
                               &g_array_index (barr, BezPoint, i).p1);
              break;
            default:
              g_return_val_if_reached (G_MAXFLOAT);
          }
        }
        dist = distance_bez_line_point ((BezPoint *) barr->data,
                                        el->path.npoints,
                                        line_width,
                                        point);
        break;
      case GE_SHAPE:
        g_array_set_size (barr, el->path.npoints);
        for (i = 0; i < el->path.npoints; i++) {
          switch (g_array_index (barr, BezPoint , i).type = el->path.points[i].type) {
            case BEZ_CURVE_TO:
              transform_coord (custom,
                               &el->path.points[i].p3,
                               &g_array_index (barr, BezPoint, i).p3);
              transform_coord (custom,
                               &el->path.points[i].p2,
                               &g_array_index (barr, BezPoint, i).p2);
            case BEZ_MOVE_TO:
            case BEZ_LINE_TO:
              transform_coord (custom,
                               &el->path.points[i].p1,
                               &g_array_index (barr, BezPoint, i).p1);
              break;
            default:
              g_return_val_if_reached (G_MAXFLOAT);
          }
        }
        dist = distance_bez_shape_point ((BezPoint *) barr->data,
                                         el->path.npoints,
                                         line_width,
                                         point);
        break;
      case GE_SUBSHAPE:
        /* subshapes are supposed to be always in bounds, no need to calculate distance */
        break;
      default:
        g_return_val_if_reached (G_MAXFLOAT);
    }
    min_dist = MIN(min_dist, dist);
    if (min_dist == 0.0)
      break;
  }
  if (custom->info->has_text && min_dist != 0.0) {
    dist = text_distance_from(custom->text, point);
    min_dist = MIN(min_dist, dist);
  }
  return min_dist;
}

static void
custom_select(Custom *custom, Point *clicked_point,
	      DiaRenderer *interactive_renderer)
{
  if (custom->info->has_text) {
    text_set_cursor(custom->text, clicked_point, interactive_renderer);
    text_grab_focus(custom->text, &custom->element.object);
  }

  element_update_handles(&custom->element);
}


static void
custom_adjust_scale (Custom           *custom,
                     Handle           *handle,
                     Point            *to,
                     ConnectionPoint  *cp,
                     HandleMoveReason  reason,
                     ModifierKeys      modifiers)
{
  static int uniform_scale = FALSE;
  static Point orig_pos;

  float delta_max = 0.0f;

  switch (reason) {
    case HANDLE_MOVE_USER:
      if (!uniform_scale) {
        orig_pos.x = to->x;
        orig_pos.y = to->y;
      }

      if ((modifiers & MODIFIER_SHIFT) != 0) {
        if (!uniform_scale) /* transition */
          custom->old_subscale = MAX(custom->subscale, 0.0);
        uniform_scale = TRUE;
      } else {
        uniform_scale = FALSE;
      }

      delta_max = (to->x - orig_pos.x);

#ifdef USE_DELTA_MAX
    /* This may yield some awkard effects for new-comers.
     * Maybe we should make it a configurable option.
     */
    if (ABS(to->y - orig_pos.y) > ABS(delta_max))
    	delta_max = (to->y - orig_pos.y);
#endif

      if (uniform_scale)
        custom->subscale =
          custom->old_subscale + (SUBSCALE_ACCELERATION * delta_max);

      if( custom->subscale < SUBSCALE_MININUM_SCALE )
        custom->subscale = SUBSCALE_MININUM_SCALE;

      break;
    case HANDLE_MOVE_USER_FINAL:
      uniform_scale = FALSE;
      break;
    case HANDLE_MOVE_CONNECTED:
    case HANDLE_MOVE_CREATE:
    case HANDLE_MOVE_CREATE_FINAL:
    default:
      break;
  }
}


static DiaObjectChange *
custom_move_handle (Custom           *custom,
                    Handle           *handle,
                    Point            *to,
                    ConnectionPoint  *cp,
                    HandleMoveReason  reason,
                    ModifierKeys      modifiers)
{
  AnchorShape horiz = ANCHOR_MIDDLE, vert = ANCHOR_MIDDLE;
  Point corner;
  real width, height;

  g_return_val_if_fail (custom != NULL, NULL);
  g_return_val_if_fail (handle != NULL, NULL);
  g_return_val_if_fail (to != NULL, NULL);

  /* remember ... */
  corner = custom->element.corner;
  width = custom->element.width;
  height = custom->element.height;

  switch (reason) {
    case HANDLE_MOVE_USER:
    case HANDLE_MOVE_USER_FINAL:
    case HANDLE_MOVE_CONNECTED : /* silence gcc */
    case HANDLE_MOVE_CREATE : /* silence gcc */
    case HANDLE_MOVE_CREATE_FINAL : /* silence gcc */
      custom_adjust_scale (custom, handle, to, cp, reason, modifiers);
      break;
    default:
      g_return_val_if_reached (NULL);
  }

  element_move_handle(&custom->element, handle->id, to, cp, reason, modifiers);

  switch (handle->id) {
    case HANDLE_RESIZE_NW:
      horiz = ANCHOR_END; vert = ANCHOR_END; break;
    case HANDLE_RESIZE_N:
      vert = ANCHOR_END; break;
    case HANDLE_RESIZE_NE:
      horiz = ANCHOR_START; vert = ANCHOR_END; break;
    case HANDLE_RESIZE_E:
      horiz = ANCHOR_START; break;
    case HANDLE_RESIZE_SE:
      horiz = ANCHOR_START; vert = ANCHOR_START; break;
    case HANDLE_RESIZE_S:
      vert = ANCHOR_START; break;
    case HANDLE_RESIZE_SW:
      horiz = ANCHOR_END; vert = ANCHOR_START; break;
    case HANDLE_RESIZE_W:
      horiz = ANCHOR_END; break;
    case HANDLE_MOVE_STARTPOINT:
    case HANDLE_MOVE_ENDPOINT:
    case HANDLE_CUSTOM1:
    case HANDLE_CUSTOM2:
    case HANDLE_CUSTOM3:
    case HANDLE_CUSTOM4:
    case HANDLE_CUSTOM5:
    case HANDLE_CUSTOM6:
    case HANDLE_CUSTOM7:
    case HANDLE_CUSTOM8:
    case HANDLE_CUSTOM9:
    default:
      break;
  }
  custom_update_data(custom, horiz, vert);

  if (width != custom->element.width && height != custom->element.height)
    return element_change_new (&corner, width, height, &custom->element);

  return NULL;
}


static DiaObjectChange *
custom_move (Custom *custom, Point *to)
{
  custom->element.corner = *to;

  custom_update_data (custom, ANCHOR_MIDDLE, ANCHOR_MIDDLE);

  return NULL;
}


static void
get_colour (Custom *custom, Color *colour, gint32 c, double opacity)
{
  switch (c) {
    case DIA_SVG_COLOUR_NONE:
      break;
    case DIA_SVG_COLOUR_FOREGROUND:
      *colour = custom->border_color;
      break;
    case DIA_SVG_COLOUR_BACKGROUND:
      *colour = custom->inner_color;
      break;
    case DIA_SVG_COLOUR_TEXT:
      *colour = custom->text->color;
      break;
    default:
      colour->alpha = opacity;
      colour->red   = ((c & 0x00ff0000) >> 16) / 255.0;
      colour->green = ((c & 0x0000ff00) >> 8) / 255.0;
      colour->blue  =  (c & 0x000000ff) / 255.0;
      break;
  }
}


static void
custom_draw (Custom *custom, DiaRenderer *renderer)
{
  static GArray *arr = NULL, *barr = NULL;
  double cur_line = 1.0, cur_dash = 1.0;
  DiaLineCaps cur_caps = DIA_LINE_CAPS_BUTT;
  DiaLineJoin cur_join = DIA_LINE_JOIN_MITER;
  DiaLineStyle cur_style = custom->line_style;

  g_return_if_fail (custom != NULL);
  g_return_if_fail (renderer != NULL);

  if (!arr) {
    arr = g_array_new (FALSE, FALSE, sizeof(Point));
  }
  if (!barr) {
    barr = g_array_new (FALSE, FALSE, sizeof(BezPoint));
  }

  dia_renderer_set_fillstyle (renderer, DIA_FILL_STYLE_SOLID);
  dia_renderer_set_linewidth (renderer, custom->border_width);
  cur_line = custom->border_width;
  dia_renderer_set_linestyle (renderer, cur_style, custom->dashlength);
  dia_renderer_set_linecaps (renderer, cur_caps);
  dia_renderer_set_linejoin (renderer, cur_join);

  /*
   * Because we do not know if any of these values are reused in the loop, we pass
   * them all by reference.
   * If anyone does know this, please correct/simplify.
   */
  custom_draw_displaylist (custom->info->display_list,
                           custom,
                           renderer, arr, barr,
                           &cur_line,
                           &cur_dash,
                           &cur_caps,
                           &cur_join,
                           &cur_style);

  if (custom->info->has_text) {
    text_draw (custom->text, renderer);
  }
}


static void
custom_draw_displaylist (GList        *display_list,
                         Custom       *custom,
                         DiaRenderer  *renderer,
                         GArray       *arr,
                         GArray       *barr,
                         double       *cur_line,
                         double       *cur_dash,
                         DiaLineCaps  *cur_caps,
                         DiaLineJoin  *cur_join,
                         DiaLineStyle *cur_style)
{
  GList *tmp;

  for (tmp = display_list; tmp; tmp = tmp->next) {
    GraphicElement *el = tmp->data;
    Color fg, bg;

    /*
     * Because we do not know if any of these values are reused in the loop,
     * we pass them all by reference.
     * If anyone does know this, please correct/simplify.
     */
    custom_draw_element( el, custom, renderer, arr, barr, cur_line, cur_dash,
                         cur_caps, cur_join, cur_style, &fg, &bg );
  }
}


static void
custom_draw_element (GraphicElement *el,
                     Custom         *custom,
                     DiaRenderer    *renderer,
                     GArray         *arr,
                     GArray         *barr,
                     double         *cur_line,
                     double         *cur_dash,
                     DiaLineCaps    *cur_caps,
                     DiaLineJoin    *cur_join,
                     DiaLineStyle   *cur_style,
                     Color          *fg,
                     Color          *bg)
{
  Point p1, p2;
  double width, height;
  double coord;
  int i;
  double radius;

  /* Somehow - maybe due to scaling - the exact match does not always work. Instead of:
   *   (el->any.s.line_width != (*cur_line))
   * be more tolerant to avoid using completely wrong line width.
   */
  if (fabs (el->any.s.line_width - (*cur_line)) > 0.001) {
    (*cur_line) = el->any.s.line_width;
    dia_renderer_set_linewidth (renderer,
                                custom->border_width * (*cur_line));
  }
  if ((el->any.s.linecap == DIA_LINE_CAPS_DEFAULT && (*cur_caps) != DIA_LINE_CAPS_BUTT) ||
      el->any.s.linecap != (*cur_caps)) {
    (*cur_caps) = (el->any.s.linecap!=DIA_LINE_CAPS_DEFAULT) ?
                      el->any.s.linecap : DIA_LINE_CAPS_BUTT;
    dia_renderer_set_linecaps (renderer, (*cur_caps));
  }
  if ((el->any.s.linejoin == DIA_LINE_JOIN_DEFAULT && (*cur_join) != DIA_LINE_JOIN_MITER) ||
      el->any.s.linejoin != (*cur_join)) {
    (*cur_join) = (el->any.s.linejoin != DIA_LINE_JOIN_DEFAULT) ?
                      el->any.s.linejoin : DIA_LINE_JOIN_MITER;
    dia_renderer_set_linejoin (renderer, (*cur_join));
  }

  if ((el->any.s.linestyle == DIA_LINE_STYLE_DEFAULT &&
      (*cur_style) != custom->line_style) || el->any.s.linestyle != (*cur_style)) {
    (*cur_style) = (el->any.s.linestyle != DIA_LINE_STYLE_DEFAULT) ?
                      el->any.s.linestyle : custom->line_style;
    dia_renderer_set_linestyle (renderer, (*cur_style),
                                custom->dashlength*(*cur_dash));
  }

  if (el->any.s.dashlength != (*cur_dash)) {
    (*cur_dash) = el->any.s.dashlength;
    dia_renderer_set_linestyle (renderer,
                                (*cur_style),
                                custom->dashlength*(*cur_dash));
  }

  (*cur_line) = el->any.s.line_width;
  get_colour (custom, fg, el->any.s.stroke, el->any.s.stroke_opacity);
  get_colour (custom, bg, el->any.s.fill, el->any.s.fill_opacity);
  switch (el->type) {
    case GE_LINE:
      transform_coord (custom, &el->line.p1, &p1);
      transform_coord (custom, &el->line.p2, &p2);
      if (el->any.s.stroke != DIA_SVG_COLOUR_NONE)
        dia_renderer_draw_line (renderer, &p1, &p2, fg);
      break;
    case GE_POLYLINE:
      g_array_set_size (arr, el->polyline.npoints);
      for (i = 0; i < el->polyline.npoints; i++) {
        transform_coord (custom,
                         &el->polyline.points[i],
                         &g_array_index (arr, Point, i));
      }
      if (el->any.s.stroke != DIA_SVG_COLOUR_NONE) {
        dia_renderer_draw_polyline (renderer,
                                    (Point *) arr->data,
                                    el->polyline.npoints,
                                    fg);
      }
      break;
    case GE_POLYGON:
      g_array_set_size (arr, el->polygon.npoints);
      for (i = 0; i < el->polygon.npoints; i++) {
        transform_coord (custom,
                         &el->polygon.points[i],
                         &g_array_index (arr, Point, i));
      }
      dia_renderer_draw_polygon (renderer,
                                 (Point *)arr->data,
                                 el->polygon.npoints,
                                 (custom->show_background && el->any.s.fill != DIA_SVG_COLOUR_NONE) ? bg : NULL,
                                 (el->any.s.stroke != DIA_SVG_COLOUR_NONE) ? fg : NULL);
      break;
    case GE_RECT:
      transform_coord (custom, &el->rect.corner1, &p1);
      transform_coord (custom, &el->rect.corner2, &p2);
      radius = custom_transform_length (custom, el->rect.corner_radius);
      if (p1.x > p2.x) {
        coord = p1.x;
        p1.x = p2.x;
        p2.x = coord;
      }
      if (p1.y > p2.y) {
        coord = p1.y;
        p1.y = p2.y;
        p2.y = coord;
      }
      /* the renderer implementation will use simple rect with a small enough radius */
      dia_renderer_draw_rounded_rect (renderer,
                                      &p1,
                                      &p2,
                                      (custom->show_background && el->any.s.fill != DIA_SVG_COLOUR_NONE) ? bg : NULL,
                                      (el->any.s.stroke != DIA_SVG_COLOUR_NONE) ? fg : NULL,
                                      radius);
      break;
    case GE_TEXT:
      text_set_height (el->text.object,
                       custom_transform_length (custom, el->text.s.font_height));
      custom_reposition_text (custom, &el->text);
      get_colour (custom,
                  &el->text.object->color,
                  el->any.s.fill,
                  el->any.s.fill_opacity);
      text_draw (el->text.object, renderer);
      text_set_position (el->text.object, &el->text.anchor);
      break;
    case GE_ELLIPSE:
      transform_coord (custom, &el->ellipse.center, &p1);
      transform_size (custom,
                      el->ellipse.width,
                      el->ellipse.height,
                      &width,
                      &height);
      dia_renderer_draw_ellipse (renderer,
                                 &p1,
                                 width,
                                 height,
                                 (custom->show_background && el->any.s.fill != DIA_SVG_COLOUR_NONE) ? bg : NULL,
                                 (el->any.s.stroke != DIA_SVG_COLOUR_NONE) ? fg : NULL);
      break;
    case GE_IMAGE:
      transform_coord (custom, &el->image.topleft, &p1);
      /* to scale correctly also for sub-shape some extra hoops */
      transform_size (custom, el->image.width, el->image.height, &width, &height);
      dia_renderer_draw_image (renderer,
                               &p1,
                               width,
                               height,
                               el->image.image);
      break;
    case GE_PATH:
      g_array_set_size (barr, el->path.npoints);
      for (i = 0; i < el->path.npoints; i++)
        switch (g_array_index (barr,BezPoint,i).type=el->path.points[i].type) {
          case BEZ_CURVE_TO:
            transform_coord (custom,
                             &el->path.points[i].p3,
                             &g_array_index (barr, BezPoint, i).p3);
            transform_coord (custom, &el->path.points[i].p2,
                             &g_array_index (barr, BezPoint, i).p2);
          case BEZ_MOVE_TO:
          case BEZ_LINE_TO:
            transform_coord (custom, &el->path.points[i].p1,
                             &g_array_index (barr, BezPoint, i).p1);
            break;
          default:
            g_return_if_reached ();
      }
      if (el->any.s.stroke != DIA_SVG_COLOUR_NONE) {
        dia_renderer_draw_bezier (renderer,
                                  (BezPoint *) barr->data,
                                  el->path.npoints,
                                  fg);
      }
      break;
    case GE_SHAPE:
      g_array_set_size (barr, el->path.npoints);
      for (i = 0; i < el->path.npoints; i++)
        switch (g_array_index (barr,BezPoint,i).type=el->path.points[i].type) {
          case BEZ_CURVE_TO:
            transform_coord (custom,
                             &el->path.points[i].p3,
                             &g_array_index (barr, BezPoint, i).p3);
            transform_coord (custom,
                             &el->path.points[i].p2,
                             &g_array_index (barr, BezPoint, i).p2);
          case BEZ_MOVE_TO:
          case BEZ_LINE_TO:
            transform_coord (custom,
                             &el->path.points[i].p1,
                             &g_array_index (barr, BezPoint, i).p1);
            break;
          default:
            g_return_if_reached ();
      }
      if (custom->show_background && el->any.s.fill != DIA_SVG_COLOUR_NONE) {
        dia_renderer_draw_beziergon(renderer,
            (BezPoint *)barr->data, el->path.npoints,
            bg, (el->any.s.stroke != DIA_SVG_COLOUR_NONE) ? fg : NULL);
      } else if (el->any.s.stroke != DIA_SVG_COLOUR_NONE) {
        dia_renderer_draw_bezier (renderer,
                                  (BezPoint *) barr->data,
                                  el->path.npoints,
                                  fg);
      }
      break;
    case GE_SUBSHAPE:
      {
        GraphicElementSubShape* subshape = (GraphicElementSubShape*)el;

        custom->current_subshape = subshape;
        custom_draw_displaylist (subshape->display_list,
                                 custom,
                                 renderer,
                                 arr,
                                 barr,
                                 cur_line,
                                 cur_dash,
                                 cur_caps,
                                 cur_join,
                                 cur_style);
        custom->current_subshape = NULL;
      }
      break;
    default:
      g_return_if_reached ();
  }
}

static void
assert_boundaries(Custom *custom)
{
  Element *elem = &custom->element;
  ShapeInfo *info = custom->info;

  int i = 0;
  GList *tmp;

  real min_width = 0.0;
  real min_height = 0.0;
  real r = 0.0;

  /* undo unkown/funky number magic when flip_h or flip_v is set */
  if (custom->flip_h) custom->xscale = -custom->xscale;
  if (custom->flip_v) custom->yscale = -custom->yscale;

  /* calculate the minimum width and height required for all subshapes */
  for (tmp = info->subshapes; tmp != NULL; tmp = tmp->next, i++) {
    GraphicElementSubShape *subshape = tmp->data;

    real parent_shape_orig_width = info->shape_bounds.right - info->shape_bounds.left;
    real parent_shape_orig_height = info->shape_bounds.bottom - info->shape_bounds.top;

    real scale = custom->subscale * subshape->default_scale;
    real width = (2*subshape->half_width) * scale;
    real height = (2*subshape->half_height) * scale;

    if (subshape->h_anchor_method == OFFSET_METHOD_PROPORTIONAL) {
      /* handle proportional anchor in x direction */
      real prop_min_width = subshape->center.x / parent_shape_orig_width;
      real scaled_half_width = subshape->half_width * scale;

      if (prop_min_width > 0.5)
        prop_min_width = 1.0 - prop_min_width;

      r = prop_min_width * parent_shape_orig_width*custom->xscale;

      if (scaled_half_width > r)
        width = (scaled_half_width / prop_min_width)-0.01;

    } else {
      /* handle fixed anchor in x direction */
      if (subshape->h_anchor_method > 0)
        width = (subshape->center.x + subshape->half_width) * scale;
      else
        width = (parent_shape_orig_width - subshape->center.x + subshape->half_width) * scale;
    }

    if (subshape->v_anchor_method == OFFSET_METHOD_PROPORTIONAL) {
      /* handle proportional anchor in y direction */
      real prop_min_height = subshape->center.y / parent_shape_orig_height;
      real scaled_half_height = subshape->half_height * scale;

      if (prop_min_height > 0.5)
        prop_min_height = 1.0 - prop_min_height;

      r = prop_min_height * parent_shape_orig_height*custom->yscale;

      if (scaled_half_height > r)
        height = (scaled_half_height / prop_min_height)-0.01;

    } else {
      /* handle fixed anchor in y direction */
      if (subshape->v_anchor_method > 0)
        height = (subshape->center.y + subshape->half_height) * scale;
      else if(subshape->v_anchor_method < 0)
        height = (parent_shape_orig_height - subshape->center.y + subshape->half_height) * scale;
    }

    if (width > min_width)
      min_width = width;
    if (height > min_height)
      min_height = height;
  }

  if (elem->width < min_width)
    elem->width = min_width;
  if (elem->height < min_height)
    elem->height = min_height;

  /* redo unknown/funky number magic when flip_h or flip_v is set */
  if (custom->flip_h) custom->xscale = -custom->xscale;
  if (custom->flip_v) custom->yscale = -custom->yscale;
}

static void
custom_update_data(Custom *custom, AnchorShape horiz, AnchorShape vert)
{
  Element *elem = &custom->element;
  ShapeInfo *info = custom->info;
  DiaObject *obj = &elem->object;
  Point center, bottom_right;
  Point p;
  static GArray *arr = NULL, *barr = NULL;

  int i;
  GList *tmp;
  char *txs;

  /* save starting points */
  center = bottom_right = elem->corner;
  center.x += elem->width/2;
  bottom_right.x += elem->width;
  center.y += elem->height/2;
  bottom_right.y += elem->height;

  assert_boundaries(custom);

  /* update the translation coefficients first ... */
  custom->xscale = elem->width / (info->shape_bounds.right -
				  info->shape_bounds.left);
  custom->yscale = elem->height / (info->shape_bounds.bottom -
				   info->shape_bounds.top);

  /* resize shape if text does not fit inside text_bounds */
  if (   (info->has_text)
      && (custom->text_fitting != DIA_TEXT_FIT_NEVER)) {
    real text_width, text_height;
    real xscale = 0.0, yscale = 0.0;
    DiaRectangle tb;

    text_calc_boundingbox(custom->text, NULL);
    text_width =
      custom->text->max_width + 2*custom->padding+custom->border_width;
    text_height = custom->text->height * custom->text->numlines +
      2 * custom->padding + custom->border_width;

    transform_rect(custom, &info->text_bounds, &tb);
    xscale = text_width / (tb.right - tb.left);

    if (   custom->text_fitting == DIA_TEXT_FIT_ALWAYS
        || (   custom->text_fitting == DIA_TEXT_FIT_WHEN_NEEDED
	    && xscale > 1.00000001)) {
      elem->width  *= xscale;
      custom->xscale *= xscale;

      /* Maybe now we won't need to do Y scaling */
      transform_rect(custom, &info->text_bounds, &tb);
    }

    yscale = text_height / (tb.bottom - tb.top);
    if (   custom->text_fitting == DIA_TEXT_FIT_ALWAYS
        || (   custom->text_fitting == DIA_TEXT_FIT_WHEN_NEEDED
	    && yscale > 1.00000001)) {
      elem->height *= yscale;
      custom->yscale *= yscale;
    }
  }

  /* if aspect ratio should be fixed, make sure xscale == yscale */
  switch (info->aspect_type) {
    case SHAPE_ASPECT_FREE:
      break;
    case SHAPE_ASPECT_FIXED:
      if (custom->xscale != custom->yscale) {
        /* depending on the moving handle let one scale take precedence */
        if (ANCHOR_MIDDLE != horiz && ANCHOR_MIDDLE != vert)
          custom->xscale = custom->yscale = (custom->xscale + custom->yscale) / 2;
        else if (ANCHOR_MIDDLE == horiz)
          custom->xscale = custom->yscale;
        else
          custom->yscale = custom->xscale;

        elem->width = custom->xscale * (info->shape_bounds.right -
                info->shape_bounds.left);
        elem->height = custom->yscale * (info->shape_bounds.bottom -
                info->shape_bounds.top);
      }
      break;
    case SHAPE_ASPECT_RANGE:
      if (custom->xscale / custom->yscale < info->aspect_min) {
        custom->xscale = custom->yscale * info->aspect_min;
        elem->width = custom->xscale * (info->shape_bounds.right -
                info->shape_bounds.left);
      }
      if (custom->xscale / custom->yscale > info->aspect_max) {
        custom->yscale = custom->xscale / info->aspect_max;
        elem->height = custom->yscale * (info->shape_bounds.bottom -
                info->shape_bounds.top);
      }
      break;
    default:
      g_return_if_reached ();
  }

  /* move shape if necessary ... */
  switch (horiz) {
    case ANCHOR_START:
      break;
    case ANCHOR_MIDDLE:
      elem->corner.x = center.x - elem->width/2; break;
    case ANCHOR_END:
      elem->corner.x = bottom_right.x - elem->width; break;
    default:
      g_return_if_reached ();
  }

  switch (vert) {
    case ANCHOR_START:
      break;
    case ANCHOR_MIDDLE:
      elem->corner.y = center.y - elem->height/2; break;
    case ANCHOR_END:
      elem->corner.y = bottom_right.y - elem->height; break;
    default:
      g_return_if_reached ();
  }

  /* update transformation coefficients after the possible resize ... */
  custom->xscale = elem->width / (info->shape_bounds.right -
				  info->shape_bounds.left);
  custom->yscale = elem->height / (info->shape_bounds.bottom -
				   info->shape_bounds.top);
  custom->xoffs = elem->corner.x - custom->xscale * info->shape_bounds.left;
  custom->yoffs = elem->corner.y - custom->yscale * info->shape_bounds.top;

  /* flip image if required ... */
  if (custom->flip_h) {
    custom->xscale = -custom->xscale;
    custom->xoffs = elem->corner.x -custom->xscale * info->shape_bounds.right;
  }
  if (custom->flip_v) {
    custom->yscale = -custom->yscale;
    custom->yoffs = elem->corner.y -custom->yscale * info->shape_bounds.bottom;
  }

  /* reposition the text element to the new text bounding box ... */
  if (info->has_text) {
    DiaRectangle tb;
    transform_rect (custom, &info->text_bounds, &tb);
    switch (custom->text->alignment) {
      case DIA_ALIGN_LEFT:
        p.x = tb.left+custom->padding;
        break;
      case DIA_ALIGN_RIGHT:
        p.x = tb.right-custom->padding;
        break;
      case DIA_ALIGN_CENTRE:
        p.x = (tb.left + tb.right) / 2;
        break;
      default:
        g_return_if_reached ();
    }
    /* align the text to be close to the shape ... */
    txs = text_get_string_copy(custom->text);
    if ((tb.bottom+tb.top)/2 > elem->corner.y + elem->height)
      p.y = tb.top +
	dia_font_ascent(txs,custom->text->font, custom->text->height);
    else if ((tb.bottom+tb.top)/2 < elem->corner.y)
      p.y = tb.bottom + custom->text->height * (custom->text->numlines - 1);
    else
      p.y = (tb.top + tb.bottom -
	     custom->text->height * custom->text->numlines) / 2 +
	dia_font_ascent(txs,custom->text->font, custom->text->height);
    text_set_position(custom->text, &p);
    g_clear_pointer (&txs, g_free);
  }

  for (i = 0; i < info->nconnections; i++)
    transform_coord(custom, &info->connections[i], &custom->connections[i].pos);
  elem->extra_spacing.border_trans = 0; /*custom->border_width/2; */
  element_update_boundingbox(elem);

  /* Merge in the bounding box of every individual element */
  if (!arr)
    arr = g_array_new(FALSE, FALSE, sizeof(Point));
  if (!barr)
    barr = g_array_new(FALSE, FALSE, sizeof(BezPoint));

  for (tmp = custom->info->display_list; tmp != NULL; tmp = tmp->next) {
    GraphicElement *el = tmp->data;
    DiaRectangle rect;
    /* Line width handling/behavior for custom objects is special. Instead of
     * directly using the given width some ratio with the shape global custom
     * border_width gets calculated - to allow influencing all line width used
     * in a complex shape (and not break backward compatibility). With
    real lwfactor = custom->border_width / 2;
     * we get traces in the diagram at high zoom levels, so use something more
     * safe for the bounding box calculation
     */
    real lwfactor = el->any.s.line_width == custom->border_width
                  ? 1.0 : custom->border_width;

    switch(el->type) {
    case GE_SUBSHAPE :
      /* if the sub-shapes leave traces in the diagram here is the place to fix it --hb */
      continue;
      break;
    case GE_LINE: {
      LineBBExtras extra;
      Point p1,p2;
      extra.start_trans = extra.end_trans = el->line.s.line_width * lwfactor;
      extra.start_long = extra.end_long = 0;

      transform_coord(custom, &el->line.p1, &p1);
      transform_coord(custom, &el->line.p2, &p2);

      line_bbox(&p1,&p2,&extra,&rect);
      break;
    }
    case GE_POLYGON:
    case GE_POLYLINE: {
      PolyBBExtras extra;

      extra.start_trans = extra.end_trans = extra.middle_trans =
        el->polyline.s.line_width * lwfactor;
      extra.start_long = extra.end_long = 0;

      g_array_set_size(arr, el->polyline.npoints);
      for (i = 0; i < el->polyline.npoints; i++)
        transform_coord(custom, &el->polyline.points[i],
                        &g_array_index(arr, Point, i));

      polyline_bbox(&g_array_index(arr,Point,0),el->polyline.npoints,
                    &extra,el->type==GE_POLYGON,&rect);
      break;
    }
    case GE_SHAPE:
    case GE_PATH: {
      PolyBBExtras extra;
      extra.start_trans = extra.end_trans = extra.middle_trans =
        el->path.s.line_width * lwfactor;
      extra.start_long = extra.end_long = 0;

      g_array_set_size (barr, el->path.npoints);
      for (i = 0; i < el->path.npoints; i++) {
        switch (g_array_index (barr, BezPoint, i).type = el->path.points[i].type) {
          case BEZ_CURVE_TO:
            transform_coord (custom, &el->path.points[i].p3,
                             &g_array_index (barr, BezPoint, i).p3);
            transform_coord (custom, &el->path.points[i].p2,
                             &g_array_index (barr, BezPoint, i).p2);
          case BEZ_MOVE_TO:
          case BEZ_LINE_TO:
            transform_coord (custom, &el->path.points[i].p1,
                             &g_array_index (barr, BezPoint, i).p1);
            break;
          default:
            g_return_if_reached ();
        }
      }

      polybezier_bbox(&g_array_index(barr,BezPoint,0),el->path.npoints,
                      &extra,el->type==GE_SHAPE,&rect);
      break;
    }
    case GE_ELLIPSE: {
      ElementBBExtras extra;
      Point centre;
      extra.border_trans = el->ellipse.s.line_width * lwfactor;
      transform_coord(custom, &el->ellipse.center, &centre);

      ellipse_bbox(&centre,
                   el->ellipse.width * fabs(custom->xscale),
                   el->ellipse.height * fabs(custom->yscale),
                   &extra,&rect);
      break;
    }
    case GE_RECT: {
      ElementBBExtras extra;
      DiaRectangle rin,trin;
      rin.left = el->rect.corner1.x;
      rin.top = el->rect.corner1.y;
      rin.right = el->rect.corner2.x;
      rin.bottom = el->rect.corner2.y;
      transform_rect(custom, &rin, &trin);

      extra.border_trans = el->rect.s.line_width * lwfactor;
      rectangle_bbox(&trin,&extra,&rect);
      break;
    }
    case GE_IMAGE: {
      DiaRectangle bounds;

      bounds.left = bounds.right = el->image.topleft.x;
      bounds.top = bounds.bottom = el->image.topleft.y;
      bounds.right += el->image.width;
      bounds.bottom += el->image.height;

      transform_rect(custom, &bounds, &rect);
      break;
    }
    case GE_TEXT:
      text_set_height (el->text.object, custom_transform_length (custom, el->text.s.font_height));
      custom_reposition_text(custom, &el->text);
      text_calc_boundingbox(el->text.object,&rect);
      /* padding only to be applied on the users text box */
      /* but we need to restore the original position of the 'constant' object */
      text_set_position(el->text.object, &el->text.anchor);
      break;
    default :
      g_assert_not_reached();
      continue;
    }
    rectangle_union(&obj->bounding_box,&rect);
  }

  /* extend bounding box to include text bounds ... */
  if (info->has_text) {
    DiaRectangle tb;
    text_calc_boundingbox(custom->text, &tb);
    tb.left -= custom->padding;
    tb.top -= custom->padding;
    tb.right += custom->padding;
    tb.bottom += custom->padding;
    rectangle_union(&obj->bounding_box, &tb);
  }

  obj->position = elem->corner;

  element_update_handles(elem);
}


/* reposition the text element to the new text bounding box ... */
static void
custom_reposition_text (Custom *custom, GraphicElementText *text)
{
  Element *elem = &custom->element;
  Point p;
  DiaRectangle tb;

  transform_rect (custom, &text->text_bounds, &tb);
  switch (text->object->alignment) {
    case DIA_ALIGN_LEFT:
      p.x = tb.left;
      break;
    case DIA_ALIGN_RIGHT:
      p.x = tb.right;
      break;
    case DIA_ALIGN_CENTRE:
      p.x = (tb.left + tb.right) / 2;
      break;
    default:
      g_return_if_reached ();
  }
  /* align the text to be close to the shape ... */

  if ((tb.bottom+tb.top)/2 > elem->corner.y + elem->height)
    p.y = tb.top +
      dia_font_ascent(text->string,
		      text->object->font, text->object->height);
  else if ((tb.bottom+tb.top)/2 < elem->corner.y)
    p.y = tb.bottom + text->object->height * (text->object->numlines - 1);
  else
    p.y = (tb.top + tb.bottom -
	   text->object->height * text->object->numlines) / 2 +
      dia_font_ascent(text->string,
		      text->object->font, text->object->height);
  text_set_position(text->object, &p);
  return;
}

static DiaObject *
custom_create(Point *startpoint,
	      void *user_data,
	      Handle **handle1,
	      Handle **handle2)
{
  Custom *custom;
  Element *elem;
  DiaObject *obj;
  ShapeInfo *info = (ShapeInfo *)user_data;
  Point p;
  int i;
  DiaFont *font = NULL;
  real font_height;

  g_return_val_if_fail(info!=NULL,NULL);

  if (!info->loaded) /* called for it's side effect */
    shape_info_getbyname (info->name);

  custom = dia_new_with_extra (sizeof (Custom), 1, info->ext_attr_size);
  elem = &custom->element;
  obj = &elem->object;

  obj->type = info->object_type;

  obj->ops = &custom_ops;

  elem->corner = *startpoint;

  elem->width = shape_info_get_default_width(info);
  elem->height = shape_info_get_default_height(info);

  custom->info = info;

  custom->old_subscale = 1.0;
  custom->subscale = 1.0;
  custom->current_subshape = NULL;

  custom->border_width = attributes_get_default_linewidth();
  custom->border_color = attributes_get_foreground();
  custom->inner_color = attributes_get_background();
  custom->show_background = TRUE;
  attributes_get_default_line_style(&custom->line_style, &custom->dashlength);

  custom->padding = 0.1;

  custom->flip_h = FALSE;
  custom->flip_v = FALSE;

  if (info->has_text) {
    attributes_get_default_font(&font, &font_height);
    p = *startpoint;
    p.x += elem->width / 2.0;
    p.y += elem->height / 2.0 + font_height / 2;
    custom->text = new_text("", font, font_height, &p, &custom->border_color,
                            info->text_align);
    g_clear_object (&font);

    /* _no_ new default: does not shrink with textbox automatically. */
    custom->text_fitting = (info->resize_with_text ? DIA_TEXT_FIT_WHEN_NEEDED : DIA_TEXT_FIT_NEVER);
  }
  shape_info_realise(custom->info);
  element_init(elem, 8, info->nconnections);

  custom->connections = g_new0(ConnectionPoint, info->nconnections);
  for (i = 0; i < info->nconnections; i++) {
    obj->connections[i] = &custom->connections[i];
    custom->connections[i].object = obj;
    custom->connections[i].connected = NULL;
    custom->connections[i].flags = 0;
    if (i == info->main_cp) {
      custom->connections[i].flags = CP_FLAGS_MAIN;
      custom->connections[i].directions = DIR_ALL;
    } else {
      transform_coord(custom, &info->connections[i], &custom->connections[i].pos);

      /* Set the directions for the connection points as hints to the
         zig-zag line. A hint is only set if the connection point is on
         the boundary of the shape. */
      custom->connections[i].directions = 0;
      if(custom->info->connections[i].x == custom->info->shape_bounds.left)
        custom->connections[i].directions |= DIR_WEST;
      if(custom->info->connections[i].x == custom->info->shape_bounds.right)
        custom->connections[i].directions |= DIR_EAST;
      if(custom->info->connections[i].y == custom->info->shape_bounds.top)
        custom->connections[i].directions |= DIR_NORTH;
      if(custom->info->connections[i].y == custom->info->shape_bounds.bottom)
        custom->connections[i].directions |= DIR_SOUTH;
    }
  }

  custom_update_data(custom, ANCHOR_MIDDLE, ANCHOR_MIDDLE);

  *handle1 = NULL;
  *handle2 = obj->handles[7];
  return &custom->element.object;
}

static void
custom_destroy(Custom *custom)
{
  if (custom->info->has_text)
    text_destroy(custom->text);

  /*
   * custom_destroy is called per object. It _must not_ destroy class stuff
   * (ShapeInfo) cause it does not hold a reference to it. Fixes e.g.
   * bug #158288, #160550, ...
   * DONT TOUCH : custom->info->display_list
   */

  /* TODO: free allocated ext props (string, etc.) */

  element_destroy(&custom->element);

  g_clear_pointer (&custom->connections, g_free);
}

static DiaObject *
custom_copy(Custom *custom)
{
  int i;
  Custom *newcustom;
  Element *elem, *newelem;
  DiaObject *newobj;

  elem = &custom->element;
  /* can't use object_copy_using_properties() becauses there is no way
   * to pass in our creation data (info) ... */
  newcustom = dia_new_with_extra (sizeof (Custom), 1, custom->info->ext_attr_size);
  newelem = &newcustom->element;
  newobj = &newcustom->element.object;

  element_copy(elem, newelem);
  newcustom->info = custom->info;

  newcustom->padding = custom->padding;
  newcustom->current_subshape = NULL; /* it's temporary state, don't copy from wrong object */
  newcustom->old_subscale = custom->old_subscale;
  newcustom->subscale = custom->subscale;

  if (custom->info->has_text) {
    newcustom->text = text_copy(custom->text);
  }

  newcustom->connections = g_new0(ConnectionPoint, custom->info->nconnections);
  for (i = 0; i < custom->info->nconnections; i++) {
    newobj->connections[i] = &newcustom->connections[i];
    newcustom->connections[i].object = newobj;
    newcustom->connections[i].connected = NULL;
    newcustom->connections[i].pos = custom->connections[i].pos;
    newcustom->connections[i].directions = custom->connections[i].directions;
    newcustom->connections[i].flags = custom->connections[i].flags;
  }

  /* ... but everything mapped to property gets copied by StdProps method
   * including the extended attributes properties. A simple memcpy can't copy by refs */
  object_copy_props (newobj, &custom->element.object, FALSE);

  return &newcustom->element.object;
}

static DiaObject *
custom_load_using_properties(ObjectNode obj_node, int version,DiaContext *ctx)
{
  Custom *custom;
  DiaObject *obj;
  Point startpoint = {0.0,0.0};
  Handle *handle1,*handle2;

  obj = custom_type.ops->create(&startpoint, shape_info_get(obj_node), &handle1, &handle2);
  if (obj) {
    custom = (Custom*)obj;
    if (version < 1)
      custom->padding = 0.5 * M_SQRT1_2; /* old padding */
    /* old default: only grow from text box, no auto-shrink. */
    custom->text_fitting = (custom->info->resize_with_text ? DIA_TEXT_FIT_WHEN_NEEDED : DIA_TEXT_FIT_NEVER);
    object_load_props(obj,obj_node,ctx);

    custom_update_data(custom, ANCHOR_MIDDLE, ANCHOR_MIDDLE);
    custom->old_subscale = custom->subscale;
  }
  return obj;
}


struct _DiaCustomObjectChange {
  DiaObjectChange objchange;

  enum { CHANGE_FLIPH, CHANGE_FLIPV} type;
  gboolean old_val;
};


DIA_DEFINE_OBJECT_CHANGE (DiaCustomObjectChange, dia_custom_object_change)


static void
dia_custom_object_change_free (DiaObjectChange *self)
{

}


static void
dia_custom_object_change_apply (DiaObjectChange *self, DiaObject *obj)
{
  DiaCustomObjectChange *change = DIA_CUSTOM_OBJECT_CHANGE (self);
  Custom *custom = (Custom *) obj;

  switch (change->type) {
    case CHANGE_FLIPH:
      custom->flip_h = !change->old_val;
      break;
    case CHANGE_FLIPV:
      custom->flip_v = !change->old_val;
      break;
    default:
      g_return_if_reached ();
  }
}


static void
dia_custom_object_change_revert (DiaObjectChange *self, DiaObject *obj)
{
  DiaCustomObjectChange *change = DIA_CUSTOM_OBJECT_CHANGE (self);
  Custom *custom = (Custom *) obj;

  switch (change->type) {
    case CHANGE_FLIPH:
      custom->flip_h = change->old_val;
      break;
    case CHANGE_FLIPV:
      custom->flip_v = change->old_val;
      break;
    default:
      g_return_if_reached ();
  }
}


static DiaObjectChange *
custom_flip_h_callback (DiaObject *obj, Point *clicked, gpointer data)
{
  Custom *custom = (Custom *) obj;
  DiaCustomObjectChange *change;

  change = dia_object_change_new (DIA_TYPE_CUSTOM_OBJECT_CHANGE);

  change->type = CHANGE_FLIPH;
  change->old_val = custom->flip_h;

  custom->flip_h = !custom->flip_h;
  custom_update_data (custom, ANCHOR_MIDDLE, ANCHOR_MIDDLE);

  return DIA_OBJECT_CHANGE (change);
}


static DiaObjectChange *
custom_flip_v_callback (DiaObject *obj, Point *clicked, gpointer data)
{
  Custom *custom = (Custom *) obj;
  DiaCustomObjectChange *change;

  change = dia_object_change_new (DIA_TYPE_CUSTOM_OBJECT_CHANGE);

  change->type = CHANGE_FLIPV;
  change->old_val = custom->flip_v;

  custom->flip_v = !custom->flip_v;
  custom_update_data (custom, ANCHOR_MIDDLE, ANCHOR_MIDDLE);

  return DIA_OBJECT_CHANGE (change);
}


static DiaMenuItem custom_menu_items[] = {
  { N_("Flip Horizontal"), custom_flip_h_callback, NULL, 1 },
  { N_("Flip Vertical"), custom_flip_v_callback, NULL, 1 },
};

static DiaMenu custom_menu = {
  "Custom",
  sizeof(custom_menu_items)/sizeof(DiaMenuItem),
  custom_menu_items,
  NULL
};

static DiaMenu *
custom_get_object_menu(Custom *custom, Point *clickedpoint)
{
  if (custom_menu.title && custom->info->name &&
      (0 != strcmp(custom_menu.title,custom->info->name))) {
    if (custom_menu.app_data_free) custom_menu.app_data_free(&custom_menu);
  }
  custom_menu.title = custom->info->name;
  return &custom_menu;
}


void
custom_object_new (ShapeInfo *info, DiaObjectType **otype)
{
  DiaObjectType *obj = g_new0 (DiaObjectType, 1);

  *obj = custom_type;

  obj->name = info->name;
  obj->flags |= info->object_flags;
  obj->default_user_data = info;

  if (info->icon) {
    if (g_file_test (info->icon, G_FILE_TEST_EXISTS)) {
      obj->pixmap = NULL;
      obj->pixmap_file = info->icon;
    } else {
      g_warning (_("Cannot open icon file %s for object type '%s'."),
                 info->icon,
                 obj->name);
    }
  }

  info->object_type = obj;

  *otype = obj;
}
