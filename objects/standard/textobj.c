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

#include "config.h"

#include <glib/gi18n-lib.h>

#include <math.h>

#include "object.h"
#include "connectionpoint.h"
#include "diarenderer.h"
#include "diainteractiverenderer.h"
#include "font.h"
#include "text.h"
#include "attributes.h"
#include "properties.h"
#include "diamenu.h"
#include "create.h"
#include "message.h" /* just dia_log_message */
#include "dia-object-change-list.h"

#define HANDLE_TEXT HANDLE_CUSTOM1


typedef enum _Valign Valign;
enum _Valign {
        VALIGN_TOP,
        VALIGN_BOTTOM,
        VALIGN_CENTER,
        VALIGN_FIRST_LINE
};
typedef struct _Textobj Textobj;
/*!
 * \brief Standard - Text
 * \extends _DiaObject
 * \ingroup StandardObjects
 */
struct _Textobj {
  DiaObject object;
  /*! just one handle to connect/move */
  Handle text_handle;
  /*! the real text object to be drawn */
  Text *text;
  /*! vertical alignment of the whole text block */
  Valign vert_align;
  /*! bounding box filling */
  Color fill_color;
  /*! background to be filled or transparent */
  gboolean show_background;
  /*! margin used for background drawing and connection point placement */
  real margin;
  /*! rotation around text handle position */
  real text_angle;
};

static struct _TextobjProperties {
  DiaAlignment alignment;
  Valign vert_align;
} default_properties = { DIA_ALIGN_LEFT, VALIGN_FIRST_LINE };

static real textobj_distance_from(Textobj *textobj, Point *point);
static void textobj_select(Textobj *textobj, Point *clicked_point,
			   DiaRenderer *interactive_renderer);
static DiaObjectChange *textobj_move_handle (Textobj          *textobj,
                                             Handle           *handle,
                                             Point            *to,
                                             ConnectionPoint  *cp,
                                             HandleMoveReason  reason,
                                             ModifierKeys      modifiers);
static DiaObjectChange *textobj_move        (Textobj          *textobj,
                                             Point            *to);
static void textobj_draw(Textobj *textobj, DiaRenderer *renderer);
static void textobj_update_data(Textobj *textobj);
static DiaObject *textobj_create(Point *startpoint,
			      void *user_data,
			      Handle **handle1,
			      Handle **handle2);
static DiaObject *textobj_copy(Textobj *textobj);
static void textobj_destroy(Textobj *textobj);

static void textobj_get_props(Textobj *textobj, GPtrArray *props);
static void textobj_set_props(Textobj *textobj, GPtrArray *props);

static void textobj_save(Textobj *textobj, ObjectNode obj_node,
			 DiaContext *ctx);
static DiaObject *textobj_load(ObjectNode obj_node, int version, DiaContext *ctx);
static DiaMenu *textobj_get_object_menu(Textobj *textobj, Point *clickedpoint);
static gboolean textobj_transform(Textobj *textobj, const DiaMatrix *m);

static void textobj_valign_point(Textobj *textobj, Point* p);

static ObjectTypeOps textobj_type_ops =
{
  (CreateFunc) textobj_create,
  (LoadFunc)   textobj_load,
  (SaveFunc)   textobj_save,
  (GetDefaultsFunc)   NULL,
  (ApplyDefaultsFunc) NULL
};

PropEnumData prop_text_vert_align_data[] = {
  { N_("Bottom"), VALIGN_BOTTOM },
  { N_("Top"), VALIGN_TOP },
  { N_("Center"), VALIGN_CENTER },
  { N_("First Line"), VALIGN_FIRST_LINE },
  { NULL, 0 }
};
static PropNumData text_margin_range = { 0.0, 10.0, 0.1 };
static PropNumData text_angle_range = { -180.0, 180.0, 5.0 };
static PropDescription textobj_props[] = {
  OBJECT_COMMON_PROPERTIES,
  PROP_STD_TEXT_ALIGNMENT,
  { "text_vert_alignment", PROP_TYPE_ENUM, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
    N_("Vertical text alignment"), NULL, prop_text_vert_align_data },
  PROP_STD_TEXT_FONT,
  PROP_STD_TEXT_HEIGHT,
  PROP_STD_TEXT_COLOUR,
  { "text_angle", PROP_TYPE_REAL, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
    N_("Text angle"), NULL, &text_angle_range },
  PROP_STD_SAVED_TEXT,
  PROP_STD_FILL_COLOUR_OPTIONAL,
  PROP_STD_SHOW_BACKGROUND_OPTIONAL,
  { "text_margin", PROP_TYPE_REAL,  PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
    N_("Text margin"), NULL, &text_margin_range },
  PROP_DESC_END
};

static PropOffset textobj_offsets[] = {
  OBJECT_COMMON_PROPERTIES_OFFSETS,
  {"text",PROP_TYPE_TEXT,offsetof(Textobj,text)},
  {"text_font",PROP_TYPE_FONT,offsetof(Textobj,text),offsetof(Text,font)},
  {PROP_STDNAME_TEXT_HEIGHT,PROP_STDTYPE_TEXT_HEIGHT,offsetof(Textobj,text),offsetof(Text,height)},
  {"text_colour",PROP_TYPE_COLOUR,offsetof(Textobj,text),offsetof(Text,color)},
  {"text_alignment",PROP_TYPE_ENUM,offsetof(Textobj,text),offsetof(Text,alignment)},
  {"text_angle",PROP_TYPE_REAL,offsetof(Textobj,text_angle)},
  {"text_vert_alignment",PROP_TYPE_ENUM,offsetof(Textobj,vert_align)},
  { "fill_colour", PROP_TYPE_COLOUR, offsetof(Textobj, fill_color) },
  { "show_background", PROP_TYPE_BOOL, offsetof(Textobj, show_background) },
  { "text_margin", PROP_TYPE_REAL, offsetof(Textobj, margin) },
  { NULL, 0, 0 }
};

/* Version history:
 * Version 1 added vertical alignment, and needed old objects to use the
 *     right alignment.
 */
DiaObjectType textobj_type =
{
  "Standard - Text",   /* name */
  1,                   /* version */
  (const char **) "res:/org/gnome/Dia/objects/standard/text.png",
  &textobj_type_ops,   /* ops */
  NULL,
  0,
  textobj_props,
  textobj_offsets
};

DiaObjectType *_textobj_type = (DiaObjectType *) &textobj_type;

static ObjectOps textobj_ops = {
  (DestroyFunc)         textobj_destroy,
  (DrawFunc)            textobj_draw,
  (DistanceFunc)        textobj_distance_from,
  (SelectFunc)          textobj_select,
  (CopyFunc)            textobj_copy,
  (MoveFunc)            textobj_move,
  (MoveHandleFunc)      textobj_move_handle,
  (GetPropertiesFunc)   object_create_props_dialog,
  (ApplyPropertiesDialogFunc) object_apply_props_from_dialog,
  (ObjectMenuFunc)      textobj_get_object_menu,
  (DescribePropsFunc)   object_describe_props,
  (GetPropsFunc)        textobj_get_props,
  (SetPropsFunc)        textobj_set_props,
  (TextEditFunc) 0,
  (ApplyPropertiesListFunc) object_apply_props,
  (TransformFunc)       textobj_transform,
};

static void
textobj_get_props(Textobj *textobj, GPtrArray *props)
{
  object_get_props_from_offsets(&textobj->object,textobj_offsets,props);
}

static void
textobj_set_props(Textobj *textobj, GPtrArray *props)
{
  object_set_props_from_offsets(&textobj->object,textobj_offsets,props);
  textobj_update_data(textobj);
}

static void
_textobj_get_poly (const Textobj *textobj, Point poly[4])
{
  Point ul, lr;
  Point pt = textobj->text_handle.pos;
  DiaRectangle box;
  DiaMatrix m = { 1, 0, 0, 1, pt.x, pt.y };
  DiaMatrix t = { 1, 0, 0, 1, -pt.x, -pt.y };
  int i;

  dia_matrix_set_angle_and_scales (&m, G_PI * textobj->text_angle / 180.0, 1.0, 1.0);
  dia_matrix_multiply (&m, &t, &m);

  text_calc_boundingbox (textobj->text, &box);
  ul.x = box.left - textobj->margin;
  ul.y = box.top - textobj->margin;
  lr.x = box.right + textobj->margin;
  lr.y = box.bottom + textobj->margin;

  poly[0].x = ul.x;
  poly[0].y = ul.y;
  poly[1].x = lr.x;
  poly[1].y = ul.y;
  poly[2].x = lr.x;
  poly[2].y = lr.y;
  poly[3].x = ul.x;
  poly[3].y = lr.y;

  for (i = 0; i < 4; ++i)
    transform_point (&poly[i], &m);
}

static real
textobj_distance_from(Textobj *textobj, Point *point)
{
  if (textobj->text_angle != 0) {
    Point poly[4];

    _textobj_get_poly (textobj, poly);
    return distance_polygon_point(poly, 4, 0.0, point);
  }
  if (textobj->show_background)
    return distance_rectangle_point(&textobj->object.bounding_box, point);
  return text_distance_from(textobj->text, point);
}

static void
textobj_select(Textobj *textobj, Point *clicked_point,
	       DiaRenderer *interactive_renderer)
{
  text_set_cursor(textobj->text, clicked_point, interactive_renderer);
  text_grab_focus(textobj->text, &textobj->object);
}


static DiaObjectChange *
textobj_move_handle (Textobj          *textobj,
                     Handle           *handle,
                     Point            *to,
                     ConnectionPoint  *cp,
                     HandleMoveReason  reason,
                     ModifierKeys      modifiers)
{
  g_return_val_if_fail (textobj != NULL, NULL);
  g_return_val_if_fail (handle != NULL, NULL);
  g_return_val_if_fail (to != NULL, NULL);

  if (handle->id == HANDLE_TEXT) {
    textobj_move(textobj, to);
  }

  return NULL;
}


static DiaObjectChange *
textobj_move (Textobj *textobj, Point *to)
{
  textobj->object.position = *to;

  textobj_update_data (textobj);

  return NULL;
}


static void
textobj_draw (Textobj *textobj, DiaRenderer *renderer)
{
  g_return_if_fail (textobj != NULL);
  g_return_if_fail (renderer != NULL);

  if (textobj->show_background) {
    DiaRectangle box;
    Point ul, lr;
    text_calc_boundingbox (textobj->text, &box);
    ul.x = box.left - textobj->margin;
    ul.y = box.top - textobj->margin;
    lr.x = box.right + textobj->margin;
    lr.y = box.bottom + textobj->margin;
    if (textobj->text_angle == 0) {
      dia_renderer_draw_rect (renderer, &ul, &lr, &textobj->fill_color, NULL);
    } else {
      Point poly[4];

      _textobj_get_poly (textobj, poly);
      dia_renderer_draw_polygon (renderer, poly, 4, &textobj->fill_color, NULL);
    }
  }
  if (textobj->text_angle == 0) {
    text_draw (textobj->text, renderer);
  } else {
    dia_renderer_draw_rotated_text (renderer,
                                    textobj->text,
                                    &textobj->text_handle.pos,
                                    textobj->text_angle);
    /* XXX: interactive case not working correctly */
    if (DIA_IS_INTERACTIVE_RENDERER (renderer) &&
        dia_object_is_selected (&textobj->object) &&
        textobj->text->focus.has_focus) {
      /* editing is not rotated */
      text_draw (textobj->text, renderer);
    }
  }
}


static void
textobj_valign_point (Textobj *textobj, Point* p)
{
  DiaRectangle *bb  = &(textobj->object.bounding_box);
  real offset;

  switch (textobj->vert_align){
    case VALIGN_BOTTOM:
      offset = bb->bottom - textobj->object.position.y;
      p->y -= offset;
      break;
    case VALIGN_TOP:
      offset = bb->top - textobj->object.position.y;
      p->y -= offset;
      break;
    case VALIGN_CENTER:
      offset = (bb->bottom + bb->top)/2 - textobj->object.position.y;
      p->y -= offset;
      break;
    case VALIGN_FIRST_LINE:
      break;
    default:
      g_return_if_reached ();
  }
}


static void
textobj_update_data (Textobj *textobj)
{
  Point to2;
  DiaObject *obj = &textobj->object;
  DiaRectangle tx_bb;

  text_set_position(textobj->text, &obj->position);
  text_calc_boundingbox(textobj->text, &obj->bounding_box);

  to2 = obj->position;
  textobj_valign_point(textobj, &to2);
  /* shift text position depending on text alignment */
  if (VALIGN_TOP == textobj->vert_align)
    to2.y += textobj->margin; /* down */
  else if (VALIGN_BOTTOM == textobj->vert_align)
    to2.y -= textobj->margin; /* up */

  if (DIA_ALIGN_LEFT == textobj->text->alignment) {
    to2.x += textobj->margin; /* right */
  } else if (DIA_ALIGN_RIGHT == textobj->text->alignment) {
    to2.x -= textobj->margin; /* left */
  }
  text_set_position (textobj->text, &to2);

  /* always use the unrotated box ... */
  text_calc_boundingbox(textobj->text, &tx_bb);
  /* grow the bounding box by 2x margin */
  obj->bounding_box.top    -= textobj->margin;
  obj->bounding_box.left   -= textobj->margin;
  obj->bounding_box.bottom += textobj->margin;
  obj->bounding_box.right  += textobj->margin;

  textobj->text_handle.pos = obj->position;
  if (textobj->text_angle == 0) {
    obj->bounding_box = tx_bb;
    g_return_if_fail (obj->enclosing_box != NULL);
    *obj->enclosing_box = tx_bb;
  } else {
    /* ... and grow it when necessary */
    Point poly[4];
    int i;

    _textobj_get_poly (textobj, poly);
    /* we don't want the joined box for boundingbox because
     * selection would become too greedy than.
     */
    obj->bounding_box.left = obj->bounding_box.right = poly[0].x;
    obj->bounding_box.top = obj->bounding_box.bottom = poly[0].y;
    for (i = 1; i < 4; ++i)
      rectangle_add_point (&obj->bounding_box, &poly[i]);
    g_return_if_fail (obj->enclosing_box != NULL);
    *obj->enclosing_box = obj->bounding_box;
    /* join for editing/selection bbox */
    rectangle_union (obj->enclosing_box, &tx_bb);
  }
}

static DiaObject *
textobj_create(Point *startpoint,
	       void *user_data,
	       Handle **handle1,
	       Handle **handle2)
{
  Textobj *textobj;
  DiaObject *obj;
  Color col;
  DiaFont *font = NULL;
  real font_height;

  textobj = g_new0 (Textobj, 1);
  obj = &textobj->object;
  obj->enclosing_box = g_new0 (DiaRectangle, 1);

  obj->type = &textobj_type;

  obj->ops = &textobj_ops;

  col = attributes_get_foreground();
  attributes_get_default_font(&font, &font_height);
  textobj->text = new_text("", font, font_height,
			   startpoint, &col, default_properties.alignment );
  /* need to initialize to object.position as well, it is used update data */
  obj->position = *startpoint;

  g_clear_object (&font);
  textobj->vert_align = default_properties.vert_align;

  /* default visibility must be off to keep compatibility */
  textobj->fill_color = attributes_get_background();
  textobj->show_background = FALSE;

  object_init(obj, 1, 0);

  obj->handles[0] = &textobj->text_handle;
  textobj->text_handle.id = HANDLE_TEXT;
  textobj->text_handle.type = HANDLE_MAJOR_CONTROL;
  textobj->text_handle.connect_type = HANDLE_CONNECTABLE;
  textobj->text_handle.connected_to = NULL;
  /* no margin till Dia 0.98 */
  textobj->margin = 0.0;

  textobj_update_data(textobj);

  *handle1 = NULL;
  *handle2 = obj->handles[0];
  return &textobj->object;
}

static DiaObject *
textobj_copy(Textobj *textobj)
{
  Textobj *copied = (Textobj *)object_copy_using_properties(&textobj->object);
  copied->object.enclosing_box = g_new (DiaRectangle, 1);
  *copied->object.enclosing_box = *textobj->object.enclosing_box;
  return &copied->object;
}

static void
textobj_destroy(Textobj *textobj)
{
  text_destroy(textobj->text);
  g_clear_pointer (&textobj->object.enclosing_box, g_free);
  object_destroy(&textobj->object);
}

static void
textobj_save(Textobj *textobj, ObjectNode obj_node, DiaContext *ctx)
{
  object_save(&textobj->object, obj_node, ctx);

  data_add_text(new_attribute(obj_node, "text"),
		textobj->text, ctx);
  data_add_enum(new_attribute(obj_node, "valign"),
		  textobj->vert_align, ctx);

  if (textobj->show_background) {
    data_add_color(new_attribute(obj_node, "fill_color"), &textobj->fill_color, ctx);
    data_add_boolean(new_attribute(obj_node, "show_background"), textobj->show_background, ctx);
  }
  if (textobj->margin > 0.0)
    data_add_real(new_attribute(obj_node, "margin"), textobj->margin, ctx);
  if (textobj->text_angle != 0.0)
    data_add_real(new_attribute(obj_node, "text_angle"), textobj->text_angle, ctx);
}

static DiaObject *
textobj_load(ObjectNode obj_node, int version, DiaContext *ctx)
{
  Textobj *textobj;
  DiaObject *obj;
  AttributeNode attr;
  Point startpoint = {0.0, 0.0};

  textobj = g_new0 (Textobj, 1);
  obj = &textobj->object;
  obj->enclosing_box = g_new0 (DiaRectangle, 1);

  obj->type = &textobj_type;
  obj->ops = &textobj_ops;

  object_load(obj, obj_node, ctx);

  attr = object_find_attribute (obj_node, "text");
  if (attr != NULL) {
    textobj->text = data_text (attribute_first_data (attr), ctx);
  } else {
    DiaFont *font = dia_font_new_from_style (DIA_FONT_MONOSPACE, 1.0);
    textobj->text = new_text ("",
                              font,
                              1.0,
                              &startpoint,
                              &color_black,
                              DIA_ALIGN_CENTRE);
    g_clear_object (&font);
  }

  attr = object_find_attribute(obj_node, "valign");
  if (attr != NULL)
    textobj->vert_align = data_enum(attribute_first_data(attr), ctx);
  else if (version == 0) {
    textobj->vert_align = VALIGN_FIRST_LINE;
  }
  attr = object_find_attribute(obj_node, "text_angle");
  if (attr != NULL)
    textobj->text_angle = data_real(attribute_first_data(attr), ctx);

  /* default visibility must be off to keep compatibility */
  textobj->fill_color = attributes_get_background();
  attr = object_find_attribute(obj_node, "fill_color");
  if (attr)
    data_color(attribute_first_data(attr), &textobj->fill_color, ctx);
  attr = object_find_attribute(obj_node, "show_background");
  if (attr)
    textobj->show_background = data_boolean(attribute_first_data(attr), ctx);
  else
    textobj->show_background = FALSE;
  attr = object_find_attribute(obj_node, "margin");
  if (attr)
    textobj->margin = data_real(attribute_first_data(attr), ctx);

  object_init(obj, 1, 0);

  obj->handles[0] = &textobj->text_handle;
  textobj->text_handle.id = HANDLE_TEXT;
  textobj->text_handle.type = HANDLE_MAJOR_CONTROL;
  textobj->text_handle.connect_type = HANDLE_CONNECTABLE;
  textobj->text_handle.connected_to = NULL;

  textobj_update_data(textobj);

  return &textobj->object;
}


static DiaObjectChange *
_textobj_convert_to_path_callback (DiaObject *obj, Point *clicked, gpointer data)
{
  Textobj *textobj = (Textobj *)obj;
  const Text *text = textobj->text;
  DiaObject *path = NULL;

  if (!text_is_empty(text)) /* still screwed with empty lines ;) */
    path = create_standard_path_from_text (text);

  if (path) {
    DiaObjectChange *change;
    Color bg = textobj->fill_color;

    /* FIXME: otherwise object_substitue() will tint the text with bg */
    textobj->fill_color = text->color;
    change = object_substitute (obj, path);
    /* restore */
    textobj->fill_color = bg;

    return change;
  }

  /* silently fail */
  return dia_object_change_list_new ();
}


static DiaMenuItem textobj_menu_items[] = {
  { N_("Convert to Path"), _textobj_convert_to_path_callback, NULL, DIAMENU_ACTIVE }
};


static DiaMenu textobj_menu = {
  "Text",
  sizeof(textobj_menu_items)/sizeof(DiaMenuItem),
  textobj_menu_items,
  NULL
};

static DiaMenu *
textobj_get_object_menu(Textobj *textobj, Point *clickedpoint)
{
  const Text *text = textobj->text;

  /* Set entries sensitive/selected etc here */
  textobj_menu_items[0].active = (text->numlines > 0) ? DIAMENU_ACTIVE : 0;

  return &textobj_menu;
}
static gboolean
textobj_transform(Textobj *textobj, const DiaMatrix *m)
{
  real a, sx, sy;

  g_return_val_if_fail(m != NULL, FALSE);

  if (!dia_matrix_get_angle_and_scales (m, &a, &sx, &sy)) {
    dia_log_message ("textobj_transform() can't convert given matrix");
    return FALSE;
  } else {
    /* XXX: what to do if width!=height */
    real height = text_get_height (textobj->text) * MIN(sx,sy);
    real angle = a*180/G_PI;
    Point p = textobj->object.position;

    /* rotation is invariant to the handle position */
    transform_point (&p, m);
    text_set_height (textobj->text, height);
    textobj->text_angle = angle;
    textobj->object.position = p;
 }

  textobj_update_data(textobj);
  return TRUE;
}
