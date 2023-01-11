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
#include <string.h>
#include <stdio.h>

#include "object.h"
#include "element.h"
#include "diarenderer.h"
#include "attributes.h"
#include "text.h"
#include "properties.h"

#include "uml.h"
#include "stereotype.h"

#include "pixmaps/object.xpm"

typedef struct _Objet Objet;

#define NUM_CONNECTIONS 9

struct _Objet {
  Element element;

  ConnectionPoint connections[NUM_CONNECTIONS];

  char *stereotype;
  Text *text;
  char *exstate;  /* used for explicit state */
  Text *attributes;

  TextAttributes text_attrs; /* for both text objects */
  real line_width;
  Color line_color;
  Color fill_color;

  Point ex_pos, st_pos;
  int is_active;
  int show_attributes;
  int is_multiple;

  char *attrib;

  char *st_stereotype;
};

#define OBJET_BORDERWIDTH 0.1
#define OBJET_ACTIVEBORDERWIDTH 0.2
#define OBJET_MARGIN_X(o) (o->text_attrs.height*(0.5/0.8))
#define OBJET_MARGIN_Y(o) (o->text_attrs.height*(0.5/0.8))
#define OBJET_MARGIN_M(o) (o->text_attrs.height*(0.4/0.8))
#define OBJET_FONTHEIGHT(o)  (o->text_attrs.height)

static real objet_distance_from(Objet *ob, Point *point);
static void objet_select(Objet *ob, Point *clicked_point,
			 DiaRenderer *interactive_renderer);
static DiaObjectChange* objet_move_handle(Objet *ob, Handle *handle,
				       Point *to, ConnectionPoint *cp,
				       HandleMoveReason reason, ModifierKeys modifiers);
static DiaObjectChange* objet_move(Objet *ob, Point *to);
static void objet_draw(Objet *ob, DiaRenderer *renderer);
static DiaObject *objet_create(Point *startpoint,
			    void *user_data,
			    Handle **handle1,
			    Handle **handle2);
static void objet_destroy(Objet *ob);
static DiaObject *objet_load(ObjectNode obj_node, int version,DiaContext *ctx);
static PropDescription *objet_describe_props(Objet *objet);
static void objet_get_props(Objet *objet, GPtrArray *props);
static void objet_set_props(Objet *objet, GPtrArray *props);
static void objet_update_data(Objet *ob);

static ObjectTypeOps objet_type_ops =
{
  (CreateFunc) objet_create,
  (LoadFunc)   objet_load,/*using_properties*/     /* load */
  (SaveFunc)   object_save_using_properties,      /* save */
  (GetDefaultsFunc)   NULL,
  (ApplyDefaultsFunc) NULL
};

/* Non-nice typo, needed for backwards compatibility. */
DiaObjectType objet_type =
{
  "UML - Objet",  /* name */
  0,              /* version */
  object_xpm,     /* pixmap */
  &objet_type_ops /* ops */
};

DiaObjectType umlobject_type =
{
  "UML - Object", /* name */
  0,              /* version */
  object_xpm,     /* pixmap */
  &objet_type_ops /* ops */
};

static ObjectOps objet_ops = {
  (DestroyFunc)         objet_destroy,
  (DrawFunc)            objet_draw,
  (DistanceFunc)        objet_distance_from,
  (SelectFunc)          objet_select,
  (CopyFunc)            object_copy_using_properties,
  (MoveFunc)            objet_move,
  (MoveHandleFunc)      objet_move_handle,
  (GetPropertiesFunc)   object_create_props_dialog,
  (ApplyPropertiesDialogFunc) object_apply_props_from_dialog,
  (ObjectMenuFunc)      NULL,
  (DescribePropsFunc)   objet_describe_props,
  (GetPropsFunc)        objet_get_props,
  (SetPropsFunc)        objet_set_props,
  (TextEditFunc) 0,
  (ApplyPropertiesListFunc) object_apply_props,
};


static PropDescription objet_props[] = {
  ELEMENT_COMMON_PROPERTIES,
  PROP_STD_NOTEBOOK_BEGIN,
  PROP_NOTEBOOK_PAGE("assoc", PROP_FLAG_DONT_MERGE, N_("General")),
  { "stereotype", PROP_TYPE_STRING, PROP_FLAG_VISIBLE,
    N_("Stereotype"), NULL, NULL },
  { "exstate", PROP_TYPE_STRING, PROP_FLAG_VISIBLE,
    N_("Explicit state"),NULL, NULL },
  { "attribstr", PROP_TYPE_MULTISTRING, PROP_FLAG_VISIBLE|PROP_FLAG_DONT_SAVE,
    N_("Attributes"),NULL, GINT_TO_POINTER(6) },
  { "attrib", PROP_TYPE_TEXT, 0, NULL,NULL, NULL },
  { "is_active", PROP_TYPE_BOOL, PROP_FLAG_VISIBLE,
    N_("Active object"),NULL,NULL},
  { "show_attribs", PROP_TYPE_BOOL, PROP_FLAG_VISIBLE,
    N_("Show attributes"),NULL,NULL},
  { "multiple", PROP_TYPE_BOOL, PROP_FLAG_VISIBLE,
    N_("Multiple instance"),NULL,NULL},

  PROP_NOTEBOOK_PAGE("style", PROP_FLAG_DONT_MERGE, N_("Style")),
  PROP_STD_TEXT_FONT_OPTIONS(PROP_FLAG_VISIBLE|PROP_FLAG_STANDARD|PROP_FLAG_OPTIONAL),
  PROP_STD_TEXT_HEIGHT_OPTIONS(PROP_FLAG_VISIBLE|PROP_FLAG_STANDARD|PROP_FLAG_OPTIONAL),
  /* can't use PROP_STD_TEXT_COLOUR_OPTIONAL cause it has PROP_FLAG_DONT_SAVE. It is designed to fill the Text object - not some subset */
  PROP_STD_TEXT_COLOUR_OPTIONS(PROP_FLAG_VISIBLE|PROP_FLAG_STANDARD|PROP_FLAG_OPTIONAL),
  { "text", PROP_TYPE_TEXT, 0, N_("Text"), NULL, NULL },
  PROP_STD_LINE_WIDTH_OPTIONAL,
  PROP_STD_LINE_COLOUR_OPTIONAL,
  PROP_STD_FILL_COLOUR_OPTIONAL,
  PROP_STD_NOTEBOOK_END,
  PROP_DESC_END
};

static PropDescription *
objet_describe_props(Objet *ob)
{
  if (objet_props[0].quark == 0) {
    prop_desc_list_calculate_quarks(objet_props);
  }
  return objet_props;
}

static PropOffset objet_offsets[] = {
  ELEMENT_COMMON_PROPERTIES_OFFSETS,
  { "name", PROP_TYPE_STRING, offsetof(Objet, exstate) },
  { "stereotype", PROP_TYPE_STRING, offsetof(Objet, stereotype) },
  { "text", PROP_TYPE_TEXT, offsetof(Objet, text) },
  { "exstate", PROP_TYPE_STRING, offsetof(Objet, exstate) },
  { "attrib", PROP_TYPE_TEXT, offsetof(Objet, attributes)},
  { "attribstr", PROP_TYPE_MULTISTRING, offsetof(Objet, attrib)},
  { "is_active", PROP_TYPE_BOOL, offsetof(Objet,is_active)},
  { "show_attribs", PROP_TYPE_BOOL, offsetof(Objet, show_attributes)},
  { "multiple", PROP_TYPE_BOOL, offsetof(Objet, is_multiple)},

  { PROP_STDNAME_TEXT_HEIGHT,PROP_STDTYPE_TEXT_HEIGHT,offsetof(Objet, text_attrs.height) },
  { "text_colour",PROP_TYPE_COLOUR,offsetof(Objet, text_attrs.color) },
  { "line_colour",PROP_TYPE_COLOUR,offsetof(Objet, line_color) },
  { "fill_colour",PROP_TYPE_COLOUR,offsetof(Objet, fill_color) },
  { PROP_STDNAME_LINE_WIDTH, PROP_STDTYPE_LINE_WIDTH, offsetof(Objet, line_width) },
  { "text_font",PROP_TYPE_FONT,offsetof(Objet, text_attrs.font) },
  { NULL, 0, 0 },
};


static void
objet_get_props (Objet * objet, GPtrArray *props)
{
  text_get_attributes (objet->text, &objet->text_attrs);
  /* the aligement is _not_ part of the deal */
  objet->text_attrs.alignment = DIA_ALIGN_CENTRE;
  g_clear_pointer (&objet->attrib, g_free);
  objet->attrib = text_get_string_copy (objet->attributes);

  object_get_props_from_offsets (&objet->element.object,
                                 objet_offsets,
                                 props);
}


static void
objet_set_props(Objet *objet, GPtrArray *props)
{
  object_set_props_from_offsets(&objet->element.object,
                                objet_offsets,props);
  apply_textstr_properties(props,objet->attributes,"attrib",objet->attrib);
  /* also update our text object with the new color (font + height) */
  /* the aligement is _not_ part of the deal */
  objet->text_attrs.alignment = DIA_ALIGN_CENTRE;
  apply_textattr_properties(props,objet->text,"text",&objet->text_attrs);
  objet->text_attrs.alignment = DIA_ALIGN_LEFT;
  apply_textattr_properties(props,objet->attributes,"attrib",&objet->text_attrs);
  objet->text_attrs.alignment = DIA_ALIGN_CENTRE;
  g_clear_pointer (&objet->st_stereotype, g_free);
  objet_update_data(objet);
}

static real
objet_distance_from(Objet *ob, Point *point)
{
  DiaObject *obj = &ob->element.object;
  return distance_rectangle_point(&obj->bounding_box, point);
}

static void
objet_select(Objet *ob, Point *clicked_point,
	       DiaRenderer *interactive_renderer)
{
  text_set_cursor(ob->text, clicked_point, interactive_renderer);
  text_grab_focus(ob->text, &ob->element.object);
  if (ob->show_attributes) /* second focus: allows to <tab> between them */
    text_grab_focus(ob->attributes, &ob->element.object);
  element_update_handles(&ob->element);
}


static DiaObjectChange *
objet_move_handle (Objet            *ob,
                   Handle           *handle,
                   Point            *to,
                   ConnectionPoint  *cp,
                   HandleMoveReason  reason,
                   ModifierKeys      modifiers)
{
  g_return_val_if_fail (ob != NULL, NULL);
  g_return_val_if_fail (handle != NULL, NULL);
  g_return_val_if_fail (to != NULL, NULL);

  g_return_val_if_fail (handle->id < 8, NULL);

  return NULL;
}


static DiaObjectChange*
objet_move(Objet *ob, Point *to)
{
  ob->element.corner = *to;
  objet_update_data(ob);

  return NULL;
}


static void
objet_draw (Objet *ob, DiaRenderer *renderer)
{
  Element *elem;
  double bw, x, y, w, h;
  Point p1, p2;
  int i;

  g_return_if_fail (ob != NULL);
  g_return_if_fail (renderer != NULL);

  elem = &ob->element;

  x = elem->corner.x;
  y = elem->corner.y;
  w = elem->width;
  h = elem->height;

  bw = (ob->is_active) ? OBJET_ACTIVEBORDERWIDTH: ob->line_width;

  dia_renderer_set_fillstyle (renderer, DIA_FILL_STYLE_SOLID);
  dia_renderer_set_linewidth (renderer, bw);
  dia_renderer_set_linestyle (renderer, DIA_LINE_STYLE_SOLID, 0.0);


  p1.x = x; p1.y = y;
  p2.x = x+w; p2.y = y+h;

  if (ob->is_multiple) {
    p1.x += OBJET_MARGIN_M (ob);
    p2.y -= OBJET_MARGIN_M (ob);
    dia_renderer_draw_rect (renderer,
                            &p1,
                            &p2,
                            &ob->fill_color,
                            &ob->line_color);
    p1.x -= OBJET_MARGIN_M (ob);
    p1.y += OBJET_MARGIN_M (ob);
    p2.x -= OBJET_MARGIN_M (ob);
    p2.y += OBJET_MARGIN_M (ob);
  }

  dia_renderer_draw_rect (renderer,
                          &p1,
                          &p2,
                          &ob->fill_color,
                          &ob->line_color);


  text_draw (ob->text, renderer);

  dia_renderer_set_font (renderer, ob->text->font, ob->text->height);

  if ((ob->st_stereotype != NULL) && (ob->st_stereotype[0] != '\0')) {
    dia_renderer_draw_string (renderer,
                              ob->st_stereotype,
                              &ob->st_pos,
                              DIA_ALIGN_CENTRE,
                              &ob->text_attrs.color);
  }

  if ((ob->exstate != NULL) && (ob->exstate[0] != '\0')) {
    dia_renderer_draw_string (renderer,
                              ob->exstate,
                              &ob->ex_pos,
                              DIA_ALIGN_CENTRE,
                              &ob->text_attrs.color);
  }

  /* Is there a better way to underline? */
  p1.x = x + (w - text_get_max_width (ob->text))/2;
  p1.y = ob->text->position.y + text_get_descent (ob->text);
  p2.x = p1.x + text_get_max_width (ob->text);
  p2.y = p1.y;

  dia_renderer_set_linewidth (renderer, ob->line_width/2);

  for (i=0; i<ob->text->numlines; i++) {
    p1.x = x + (w - text_get_line_width (ob->text, i))/2;
    p2.x = p1.x + text_get_line_width (ob->text, i);
    dia_renderer_draw_line (renderer,
                            &p1,
                            &p2,
                            &ob->text_attrs.color);
    p1.y = p2.y += ob->text->height;
  }

  if (ob->show_attributes) {
    p1.x = x; p2.x = x + w;
    p1.y = p2.y = ob->attributes->position.y - ob->attributes->ascent - OBJET_MARGIN_Y (ob);

    dia_renderer_set_linewidth (renderer, bw);
    dia_renderer_draw_line (renderer,
                            &p1,
                            &p2,
                            &ob->line_color);

    text_draw (ob->attributes, renderer);
  }
}

static void
objet_update_data(Objet *ob)
{
  Element *elem = &ob->element;
  DiaObject *obj = &elem->object;
  DiaFont *font;
  Point p1, p2;
  real h, w = 0;

  text_calc_boundingbox(ob->text, NULL);
  ob->stereotype = remove_stereotype_from_string(ob->stereotype);
  if (!ob->st_stereotype) {
    ob->st_stereotype =  string_to_stereotype(ob->stereotype);
  }

  font = ob->text->font;
  h = elem->corner.y + OBJET_MARGIN_Y(ob);

  if (ob->is_multiple) {
    h += OBJET_MARGIN_M(ob);
  }

  if ((ob->stereotype != NULL) && (ob->stereotype[0] != '\0')) {
      w = dia_font_string_width(ob->st_stereotype, font, OBJET_FONTHEIGHT(ob));
      h += OBJET_FONTHEIGHT(ob);
      ob->st_pos.y = h;
      h += OBJET_MARGIN_Y(ob)/2.0;
  }

  w = MAX(w, ob->text->max_width);
  p1.y = h + ob->text->ascent;  /* position of text */

  h += ob->text->height*ob->text->numlines;

  if ((ob->exstate != NULL) && (ob->exstate[0] != '\0')) {
      w = MAX(w, dia_font_string_width(ob->exstate, font, OBJET_FONTHEIGHT(ob)));
      h += OBJET_FONTHEIGHT(ob);
      ob->ex_pos.y = h;
  }

  h += OBJET_MARGIN_Y(ob);

  if (ob->show_attributes) {
      h += OBJET_MARGIN_Y(ob) + ob->attributes->ascent;
      p2.x = elem->corner.x + OBJET_MARGIN_X(ob);
      p2.y = h;
      text_set_position(ob->attributes, &p2);

      h += ob->attributes->height*ob->attributes->numlines;

      text_calc_boundingbox(ob->attributes, NULL);
      w = MAX(w, ob->attributes->max_width);
  }

  w += 2*OBJET_MARGIN_X(ob);

  p1.x = elem->corner.x + w/2.0;
  text_set_position(ob->text, &p1);

  ob->ex_pos.x = ob->st_pos.x = p1.x;


  if (ob->is_multiple) {
    w += OBJET_MARGIN_M(ob);
  }

  elem->width = w;
  elem->height = h - elem->corner.y;

  /* Update connections: */
  element_update_connections_rectangle(elem, ob->connections);

  element_update_boundingbox(elem);
  obj->position = elem->corner;
  element_update_handles(elem);
}

static DiaObject *
objet_create(Point *startpoint,
		    void *user_data,
		    Handle **handle1,
		    Handle **handle2)
{
  Objet *ob;
  Element *elem;
  DiaObject *obj;
  Point p;
  DiaFont *font;
  int i;

  ob = g_new0 (Objet, 1);
  elem = &ob->element;
  obj = &elem->object;

  obj->type = &umlobject_type;

  obj->ops = &objet_ops;

  elem->corner = *startpoint;

  ob->text_attrs.color = color_black;
  ob->line_width = attributes_get_default_linewidth();
  ob->line_color = attributes_get_foreground();
  ob->fill_color = attributes_get_background();

  font = dia_font_new_from_style(DIA_FONT_SANS, 0.8);

  ob->show_attributes = FALSE;
  ob->is_active = FALSE;
  ob->is_multiple = FALSE;

  ob->exstate = NULL;
  ob->stereotype = NULL;
  ob->st_stereotype = NULL;

  /* The text position is recalculated later */
  p.x = 0.0;
  p.y = 0.0;
  ob->attributes = new_text ("", font, 0.8, &p, &color_black, DIA_ALIGN_LEFT);
  ob->attrib = NULL;
  ob->text = new_text ("", font, 0.8, &p, &color_black, DIA_ALIGN_CENTRE);
  text_get_attributes(ob->text,&ob->text_attrs);

  g_clear_object (&font);

  element_init (elem, 8, NUM_CONNECTIONS);

  for (i=0;i<NUM_CONNECTIONS;i++) {
    obj->connections[i] = &ob->connections[i];
    ob->connections[i].object = obj;
    ob->connections[i].connected = NULL;
  }
  ob->connections[8].flags = CP_FLAGS_MAIN;
  elem->extra_spacing.border_trans = ob->line_width/2.0;
  objet_update_data(ob);

  for (i=0;i<8;i++) {
    obj->handles[i]->type = HANDLE_NON_MOVABLE;
  }

  *handle1 = NULL;
  *handle2 = NULL;

  return &ob->element.object;
}


static void
objet_destroy (Objet *ob)
{
  text_destroy (ob->text);
  text_destroy (ob->attributes);

  g_clear_pointer (&ob->stereotype, g_free);
  g_clear_pointer (&ob->st_stereotype, g_free);
  g_clear_pointer (&ob->exstate, g_free);
  g_clear_pointer (&ob->attrib, g_free);

  element_destroy (&ob->element);
}


static DiaObject *
objet_load(ObjectNode obj_node, int version,DiaContext *ctx)
{
  DiaObject * obj = object_load_using_properties(&objet_type,
                                                 obj_node,version,ctx);
  AttributeNode attr;
  /* For compatibility with previous dia files. If no line_width, use
   * OBJET_BORDERWIDTH, that was the previous line width.
   */
  attr = object_find_attribute(obj_node, PROP_STDNAME_LINE_WIDTH);
  if (attr == NULL)
    ((Objet*)obj)->line_width = OBJET_BORDERWIDTH;

  return obj;
}


