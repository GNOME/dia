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

/* DO NOT USE THIS OBJECT AS A BASIS FOR A NEW OBJECT. */

#include "config.h"

#include <glib/gi18n-lib.h>

#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "object.h"
#include "element.h"
#include "diarenderer.h"
#include "attributes.h"
#include "text.h"
#include "properties.h"

#include "pixmaps/function.xpm"

typedef struct _Function Function;
typedef struct _FunctionChange FunctionChange;

#define NUM_CONNECTIONS 9

struct _Function {
  Element element;

  ConnectionPoint connections[NUM_CONNECTIONS];

  Text *text;

  int is_wish;
  int is_user;
};

enum FuncChangeType {
  WISH_FUNC,
  USER_FUNC,
  TEXT_EDIT,
  ALL
};


#define DIA_FS_TYPE_FUNCTION_OBJECT_CHANGE dia_fs_function_object_change_get_type ()
G_DECLARE_FINAL_TYPE (DiaFSFunctionObjectChange,
                      dia_fs_function_object_change,
                      DIA_FS, FUNCTION_OBJECT_CHANGE,
                      DiaObjectChange)


struct _DiaFSFunctionObjectChange {
  DiaObjectChange      obj_change;
  enum FuncChangeType  change_type;
  int                  is_wish;
  int                  is_user;
  char                *text;
};


DIA_DEFINE_OBJECT_CHANGE (DiaFSFunctionObjectChange, dia_fs_function_object_change)


#define FUNCTION_FONTHEIGHT 0.8
#define FUNCTION_BORDERWIDTH_SCALE 6.0
#define FUNCTION_MARGIN_SCALE 3.0
#define FUNCTION_MARGIN_X 2.4
#define FUNCTION_MARGIN_Y 2.4
#define FUNCTION_DASHLENGTH_SCALE 2.0

static double           function_distance_from   (Function         *pkg,
                                                  Point            *point);
static void             function_select          (Function         *pkg,
                                                  Point            *clicked_point,
                                                  DiaRenderer      *interactive_renderer);
static DiaObjectChange* function_move_handle     (Function         *pkg,
                                                  Handle           *handle,
                                                  Point            *to,
                                                  ConnectionPoint  *cp,
                                                  HandleMoveReason  reason,
                                                  ModifierKeys      modifiers);
static DiaObjectChange* function_move            (Function         *pkg,
                                                  Point            *to);
static void function_draw(Function *pkg, DiaRenderer *renderer);
static DiaObject *function_create(Point *startpoint,
			       void *user_data,
			       Handle **handle1,
			       Handle **handle2);
static void function_destroy(Function *pkg);
static DiaObject *function_copy(Function *pkg);
static void function_save(Function *pkg, ObjectNode obj_node,
			  DiaContext *ctx);
static DiaObject *function_load(ObjectNode obj_node, int version, DiaContext *ctx);
static void function_update_data(Function *pkg);
static DiaMenu *function_get_object_menu(Function *func, Point *clickedpoint) ;
static PropDescription *function_describe_props(Function *mes);
static void
function_get_props(Function * function, GPtrArray *props);
static void
function_set_props(Function * function, GPtrArray *props);

static ObjectTypeOps function_type_ops =
{
  (CreateFunc) function_create,
  (LoadFunc)   function_load,
  (SaveFunc)   function_save
};

DiaObjectType function_type =
{
  "FS - Function",  /* name */
  0,             /* version */
  function_xpm,   /* pixmap */
  &function_type_ops /* ops */
};

static ObjectOps function_ops = {
  (DestroyFunc)         function_destroy,
  (DrawFunc)            function_draw,
  (DistanceFunc)        function_distance_from,
  (SelectFunc)          function_select,
  (CopyFunc)            function_copy,
  (MoveFunc)            function_move,
  (MoveHandleFunc)      function_move_handle,
  (GetPropertiesFunc)   object_create_props_dialog,
  (ApplyPropertiesDialogFunc) object_apply_props_from_dialog,
  (ObjectMenuFunc)      function_get_object_menu,
  (DescribePropsFunc)   function_describe_props,
  (GetPropsFunc)        function_get_props,
  (SetPropsFunc)        function_set_props,
  (TextEditFunc) 0,
  (ApplyPropertiesListFunc) object_apply_props,
};

static PropDescription function_props[] = {
  ELEMENT_COMMON_PROPERTIES,
  { "wish function", PROP_TYPE_BOOL, PROP_FLAG_VISIBLE,
    N_("Wish function"), NULL, NULL },
  { "user function", PROP_TYPE_BOOL, PROP_FLAG_VISIBLE,
    N_("User function"), NULL, NULL },
  { "text", PROP_TYPE_TEXT, 0, NULL, NULL },
  PROP_STD_TEXT_FONT,
  PROP_STD_TEXT_HEIGHT,
  PROP_STD_TEXT_COLOUR,
  PROP_DESC_END
};

static PropDescription *
function_describe_props(Function *mes)
{
  if (function_props[0].quark == 0)
    prop_desc_list_calculate_quarks(function_props);
  return function_props;

}

static PropOffset function_offsets[] = {
  ELEMENT_COMMON_PROPERTIES_OFFSETS,
  { "wish function", PROP_TYPE_BOOL, offsetof(Function, is_wish) },
  { "user function", PROP_TYPE_BOOL, offsetof(Function, is_user) },
  { "text", PROP_TYPE_TEXT, offsetof (Function, text) },
  { "text_font", PROP_TYPE_FONT, offsetof(Function,text),offsetof(Text,font) },
  { PROP_STDNAME_TEXT_HEIGHT, PROP_STDTYPE_TEXT_HEIGHT, offsetof(Function,text),offsetof(Text,height) },
  { "text_colour", PROP_TYPE_COLOUR, offsetof(Function,text),offsetof(Text,color) },
  { NULL, 0, 0}
};

static void
function_get_props(Function * function, GPtrArray *props)
{
  object_get_props_from_offsets(&function->element.object,
                                function_offsets, props);
}

static void
function_set_props(Function *function, GPtrArray *props)
{
  object_set_props_from_offsets(&function->element.object,
                                function_offsets, props);
  function_update_data(function);
}


static void
function_change_apply_revert (DiaFSFunctionObjectChange *change, DiaObject* obj)
{
  int tmp;
  char *ttxt;
  Function *fcn = (Function *) obj;

  if ( change->change_type == WISH_FUNC || change->change_type == ALL ) {
     tmp = fcn->is_wish ;
     fcn->is_wish = change->is_wish ;
     change->is_wish = tmp ;
  }
  if ( change->change_type == USER_FUNC || change->change_type == ALL ) {
     tmp = fcn->is_user ;
     fcn->is_user = change->is_user ;
     change->is_user = tmp ;
  }
  if ( change->change_type == TEXT_EDIT || change->change_type == ALL ) {
     ttxt = text_get_string_copy( fcn->text ) ;
     text_set_string( fcn->text, change->text ) ;
    g_clear_pointer (&change->text, g_free);
    change->text = ttxt;
  }
}


static void
dia_fs_function_object_change_apply (DiaObjectChange *self, DiaObject *obj)
{
  function_change_apply_revert (DIA_FS_FUNCTION_OBJECT_CHANGE (self), obj);
}


static void
dia_fs_function_object_change_revert (DiaObjectChange *self, DiaObject *obj)
{
  function_change_apply_revert (DIA_FS_FUNCTION_OBJECT_CHANGE (self), obj);
}


static void
dia_fs_function_object_change_free (DiaObjectChange *self)
{
  DiaFSFunctionObjectChange *change = DIA_FS_FUNCTION_OBJECT_CHANGE (self);

  if (change->change_type == TEXT_EDIT) {
    g_clear_pointer (&change->text, g_free);
  }
}


static DiaObjectChange *
function_create_change (Function *fcn, enum FuncChangeType change_type)
{
  DiaFSFunctionObjectChange *change;

  change = dia_object_change_new (DIA_FS_TYPE_FUNCTION_OBJECT_CHANGE);

  change->change_type = change_type ;

  if (change_type == WISH_FUNC || change_type == ALL) {
    change->is_wish = fcn->is_wish;
  }

  if (change_type == USER_FUNC || change_type == ALL) {
    change->is_user = fcn->is_user;
  }

  if (change_type == TEXT_EDIT || change_type == ALL) {
    change->text = text_get_string_copy (fcn->text);
  }

  return DIA_OBJECT_CHANGE (change);
}


static double
function_distance_from(Function *pkg, Point *point)
{
  DiaObject *obj = &pkg->element.object;

  return distance_rectangle_point (&obj->bounding_box, point);
}


static void
function_select(Function *pkg, Point *clicked_point,
		DiaRenderer *interactive_renderer)
{
  text_set_cursor(pkg->text, clicked_point, interactive_renderer);
  text_grab_focus(pkg->text, &pkg->element.object);
  element_update_handles(&pkg->element);
}


static DiaObjectChange *
function_move_handle (Function         *pkg,
                      Handle           *handle,
                      Point            *to,
                      ConnectionPoint  *cp,
                      HandleMoveReason  reason,
                      ModifierKeys      modifiers)
{
  g_return_val_if_fail (pkg != NULL, NULL);
  g_return_val_if_fail (handle != NULL, NULL);
  g_return_val_if_fail (to != NULL, NULL);

  g_return_val_if_fail (handle->id < 8, NULL);

  return NULL;
}


static DiaObjectChange*
function_move (Function *pkg, Point *to)
{
  pkg->element.corner = *to;
  function_update_data (pkg);

  return NULL;
}


static void
function_draw (Function *pkg, DiaRenderer *renderer)
{
  Element *elem;
  real x, y, w, h;
  Point p1, p2;
  real font_height ;

  g_return_if_fail (pkg != NULL);
  g_return_if_fail (pkg->text != NULL);
  g_return_if_fail (renderer != NULL);

  elem = &pkg->element;

  x = elem->corner.x;
  y = elem->corner.y;
  w = elem->width;
  h = elem->height;

  font_height = pkg->text->height ;

  dia_renderer_set_fillstyle (renderer, DIA_FILL_STYLE_SOLID);
  dia_renderer_set_linewidth (renderer,
                              font_height / FUNCTION_BORDERWIDTH_SCALE);
  if (pkg->is_wish) {
    dia_renderer_set_linestyle (renderer, DIA_LINE_STYLE_DASHED,
                                font_height / FUNCTION_DASHLENGTH_SCALE);
  } else {
    dia_renderer_set_linestyle (renderer, DIA_LINE_STYLE_SOLID, 0.0);
  }

  p1.x = x; p1.y = y;
  p2.x = x+w; p2.y = y+h;

  if (pkg->is_user) {
    dia_renderer_draw_rect (renderer,
                            &p1,
                            &p2,
                            &color_white,
                            &color_black);
    p1.x += font_height / FUNCTION_MARGIN_SCALE;
    p1.y += font_height / FUNCTION_MARGIN_SCALE;
    p2.y -= font_height / FUNCTION_MARGIN_SCALE;
    p2.x -= font_height / FUNCTION_MARGIN_SCALE;
    /* y += FUNCTION_MARGIN_M; */
  }
  dia_renderer_draw_rect (renderer,
                          &p1,
                          &p2,
                          &color_white,
                          &color_black);


  text_draw (pkg->text, renderer);
}

static void
function_update_data(Function *pkg)
{
  Element *elem = &pkg->element;
  DiaObject *obj = &elem->object;
  Point p1;
  real h, w = 0, font_height;

  text_calc_boundingbox(pkg->text, NULL) ;
  font_height = pkg->text->height ;
  pkg->element.extra_spacing.border_trans = (font_height / FUNCTION_BORDERWIDTH_SCALE) / 2.0;
  h = elem->corner.y + font_height/FUNCTION_MARGIN_Y;

  if (pkg->is_user) {
    h += 2*font_height/FUNCTION_MARGIN_SCALE;
  }

  w = MAX(w, pkg->text->max_width);
  p1.y = h + pkg->text->ascent - ( pkg->is_user ? font_height/FUNCTION_MARGIN_SCALE : 0 );  /* position of text */

  h += pkg->text->height*pkg->text->numlines;

  h += font_height/FUNCTION_MARGIN_Y;

  w += 2*font_height/FUNCTION_MARGIN_X;

  p1.x = elem->corner.x + w/2.0 + ( pkg->is_user ? font_height/FUNCTION_MARGIN_SCALE : 0 );
  text_set_position(pkg->text, &p1);

  if (pkg->is_user) {
    w += 2*font_height/FUNCTION_MARGIN_SCALE;
  }

  elem->width = w;
  elem->height = h - elem->corner.y;

  /* Update connections: */
  connpoint_update(&pkg->connections[0],
		   elem->corner.x,
		   elem->corner.y,
		   DIR_NORTHWEST);
  connpoint_update(&pkg->connections[1],
		   elem->corner.x + elem->width / 2.0,
		   elem->corner.y,
		   DIR_NORTH);
  connpoint_update(&pkg->connections[2],
		   elem->corner.x + elem->width,
		   elem->corner.y,
		   DIR_NORTHEAST);
  connpoint_update(&pkg->connections[3],
		   elem->corner.x,
		   elem->corner.y + elem->height / 2.0,
		   DIR_WEST);
  connpoint_update(&pkg->connections[4],
		   elem->corner.x + elem->width,
		   elem->corner.y + elem->height / 2.0,
		   DIR_EAST);
  connpoint_update(&pkg->connections[5],
		   elem->corner.x,
		   elem->corner.y + elem->height,
		   DIR_SOUTHWEST);
  connpoint_update(&pkg->connections[6],
		   elem->corner.x + elem->width / 2.0,
		   elem->corner.y + elem->height,
		   DIR_SOUTH);
  connpoint_update(&pkg->connections[7],
		   elem->corner.x + elem->width,
		   elem->corner.y + elem->height,
		   DIR_SOUTHEAST);
  connpoint_update(&pkg->connections[8],
		   elem->corner.x + elem->width / 2.0,
		   elem->corner.y + elem->height / 2.0,
		   DIR_SOUTHEAST);

  element_update_boundingbox(elem);

  obj->position = elem->corner;

  element_update_handles(elem);
}

static DiaObject *
function_create(Point *startpoint,
		void *user_data,
		Handle **handle1,
		Handle **handle2)
{
  Function *pkg;
  Element *elem;
  DiaObject *obj;
  Point p;
  DiaFont *font;
  int i;

  pkg = g_new0 (Function, 1);
  elem = &pkg->element;
  obj = &elem->object;

  obj->type = &function_type;

  obj->ops = &function_ops;

  elem->corner = *startpoint;

  font = dia_font_new_from_style (DIA_FONT_SANS, FUNCTION_FONTHEIGHT);

  pkg->is_wish = FALSE;
  pkg->is_user = FALSE;

  /* The text position is recalculated later */
  p.x = 0.0;
  p.y = 0.0;
  pkg->text = new_text ("",
                        font,
                        FUNCTION_FONTHEIGHT,
                        &p,
                        &color_black,
                        DIA_ALIGN_CENTRE);
  g_clear_object (&font);

  element_init (elem, 8, NUM_CONNECTIONS);

  for (i = 0; i < NUM_CONNECTIONS; i++) {
    obj->connections[i] = &pkg->connections[i];
    pkg->connections[i].object = obj;
    pkg->connections[i].connected = NULL;
  }
  pkg->connections[8].flags = CP_FLAGS_MAIN;

  function_update_data(pkg);

  for (i=0;i<8;i++) {
    obj->handles[i]->type = HANDLE_NON_MOVABLE;
  }


  *handle1 = NULL;
  *handle2 = NULL;

  return &pkg->element.object;
}

static void
function_destroy(Function *pkg)
{
  text_destroy(pkg->text);

  element_destroy(&pkg->element);
}

static DiaObject *
function_copy(Function *pkg)
{
  int i;
  Function *newpkg;
  Element *elem, *newelem;
  DiaObject *newobj;

  elem = &pkg->element;

  newpkg = g_new0 (Function, 1);
  newelem = &newpkg->element;
  newobj = &newelem->object;

  element_copy(elem, newelem);

  newpkg->text = text_copy(pkg->text);

  for (i=0;i<NUM_CONNECTIONS;i++) {
    newobj->connections[i] = &newpkg->connections[i];
    newpkg->connections[i].object = newobj;
    newpkg->connections[i].connected = NULL;
    newpkg->connections[i].pos = pkg->connections[i].pos;
    newpkg->connections[i].flags = pkg->connections[i].flags;
  }
  newpkg->is_wish = pkg->is_wish ;
  newpkg->is_user = pkg->is_user ;

  newpkg->element.extra_spacing.border_trans = pkg->element.extra_spacing.border_trans ;

  function_update_data(newpkg);

  return &newpkg->element.object;
}


static void
function_save(Function *pkg, ObjectNode obj_node, DiaContext *ctx)
{
  element_save(&pkg->element, obj_node, ctx);

  data_add_text(new_attribute(obj_node, "text"),
		pkg->text, ctx);

  data_add_boolean(new_attribute(obj_node, "is_wish"),
		   pkg->is_wish, ctx);

  data_add_boolean(new_attribute(obj_node, "is_user"),
		   pkg->is_user, ctx);
}

static DiaObject *
function_load(ObjectNode obj_node, int version, DiaContext *ctx)
{
  Function *pkg;
  AttributeNode attr;
  Element *elem;
  DiaObject *obj;
  int i;

  pkg = g_new0 (Function, 1);
  elem = &pkg->element;
  obj = &elem->object;

  obj->type = &function_type;
  obj->ops = &function_ops;

  element_load(elem, obj_node, ctx);

  pkg->text = NULL;
  attr = object_find_attribute(obj_node, "text");
  if (attr != NULL)
    pkg->text = data_text(attribute_first_data(attr), ctx);
  else { /* paranoid */
    DiaFont *font = dia_font_new_from_style (DIA_FONT_SANS,
                                             FUNCTION_FONTHEIGHT);
    pkg->text = new_text ("",
                          font,
                          FUNCTION_FONTHEIGHT,
                          &obj->position,
                          &color_black,
                          DIA_ALIGN_CENTRE);
    g_clear_object (&font);
  }

  attr = object_find_attribute(obj_node, "is_wish");
  if (attr != NULL)
    pkg->is_wish = data_boolean(attribute_first_data(attr), ctx);
  else
    pkg->is_wish = FALSE;

  attr = object_find_attribute(obj_node, "is_user");
  if (attr != NULL)
    pkg->is_user = data_boolean(attribute_first_data(attr), ctx);
  else
    pkg->is_user = FALSE;

  element_init(elem, 8, NUM_CONNECTIONS);

  for (i=0;i<NUM_CONNECTIONS;i++) {
    obj->connections[i] = &pkg->connections[i];
    pkg->connections[i].object = obj;
    pkg->connections[i].connected = NULL;
  }
  pkg->connections[8].flags = CP_FLAGS_MAIN;

  function_update_data(pkg);

  for (i=0;i<8;i++) {
    obj->handles[i]->type = HANDLE_NON_MOVABLE;
  }

  return &pkg->element.object;
}


static DiaObjectChange *
function_insert_word (Function *func, const char *word, gboolean newline)
{
  DiaObjectChange* change = function_create_change (func, TEXT_EDIT);
  char *old_chars = text_get_string_copy (func->text);
  char *new_chars = g_new0 (char,
                            strlen (old_chars) + strlen (word) + (newline ? 2 : 1));
  sprintf (new_chars, newline ? "%s\n%s" : "%s%s", old_chars, word);
  text_set_string (func->text, new_chars);
  g_clear_pointer (&new_chars, g_free);
  g_clear_pointer (&old_chars, g_free);
  function_update_data (func);
  text_set_cursor_at_end (func->text);

  return change;
}


static DiaObjectChange *
function_insert_verb (DiaObject *obj, Point *clicked, gpointer data)
{
  return function_insert_word ((Function*) obj, (const char*) data, FALSE);
}


static DiaObjectChange *
function_insert_noun (DiaObject *obj, Point *clicked, gpointer data)
{
  return function_insert_word ((Function*)obj, (const char*) data, TRUE);
}


static DiaObjectChange *
function_toggle_user_function (DiaObject *obj, Point *clicked, gpointer data)
{
  Function *func = (Function *) obj;
  DiaObjectChange *change = function_create_change (func, USER_FUNC);
  func->is_user = !func->is_user;
  function_update_data (func);

  return change;
}


static DiaObjectChange *
function_toggle_wish_function (DiaObject *obj, Point *clicked, gpointer data)
{
  Function* func = (Function*) obj;
  DiaObjectChange* change = function_create_change (func, WISH_FUNC);
  func->is_wish = !func->is_wish;
  function_update_data (func);

  return change;
}


struct _IndentedMenus {
  char*			name ;
  int			depth ;
  DiaMenuCallback	func ;
} ;

/* Maximum number of submenus in the hierarchy. This is used to generate
 * the object menu.
 */
#define FS_SUBMENU_MAXINDENT 5

#if THIS_C_COMPILER_ALSO_UNDERSTANDS_EMACS_LISP
   ; Elisp functions to generate comments:)
   (setq q nil)
   (defun q-pop () (setq q (cdr q)))
   (defun q-push ()
     (re-search-forward "(\"\\([^\"]*\\)\")" (+ (point) 30))
     (setq word (buffer-substring (match-beginning 1) (match-end 1)))
     (setq q (cons word q)))
   (defun q-write ()
     (let ((words (reverse q)))
       (beginning-of-line)
       (insert "/* Translators: Menu item " (car words))
       (mapcar (lambda (w) (insert "/" w)) (cdr words))
       (insert " *" "/\n")))
#endif

struct _IndentedMenus fmenu[] = {
/* Translators: Menu item Verb */
  {N_("Verb"),			0, NULL },
/* Translators: Menu item Verb/Channel */
   {N_("Channel"),		1, NULL },
/* Translators: Menu item Verb/Channel/Channel */
    {N_("Channel"),		2, function_insert_verb },
/* Translators: Menu item Verb/Channel/Import */
    {N_("Import"),		2, NULL },
/* Translators: Menu item Verb/Channel/Import/Import */
     {N_("Import"),		3, function_insert_verb },
/* Translators: Menu item Verb/Channel/Import/Input */
     {N_("Input"),		3, function_insert_verb },
/* Translators: Menu item Verb/Channel/Import/Receive */
     {N_("Receive"),		3, function_insert_verb },
/* Translators: Menu item Verb/Channel/Import/Allow */
     {N_("Allow"),		3, function_insert_verb },
/* Translators: Menu item Verb/Channel/Import/Form Entrance */
     {N_("Form Entrance"),	3, function_insert_verb },
/* Translators: Menu item Verb/Channel/Import/Capture */
     {N_("Capture"),		3, function_insert_verb },
/* Translators: Menu item Verb/Channel/Export */
    {N_("Export"),		2, NULL },
/* Translators: Menu item Verb/Channel/Export/Export */
     {N_("Export"),		3, function_insert_verb },
/* Translators: Menu item Verb/Channel/Export/Discharge */
     {N_("Discharge"),		3, function_insert_verb },
/* Translators: Menu item Verb/Channel/Export/Eject */
     {N_("Eject"),		3, function_insert_verb },
/* Translators: Menu item Verb/Channel/Export/Dispose */
     {N_("Dispose"),		3, function_insert_verb },
/* Translators: Menu item Verb/Channel/Export/Remove */
     {N_("Remove"),		3, function_insert_verb },
/* Translators: Menu item Verb/Channel/Transfer */
    {N_("Transfer"),		2, NULL },
/* Translators: Menu item Verb/Channel/Transfer/Transfer */
     {N_("Transfer"),		3, function_insert_verb },
/* Translators: Menu item Verb/Channel/Transfer/Transport */
     {N_("Transport"),		3, NULL },
/* Translators: Menu item Verb/Channel/Transfer/Transport/Transport */
      {N_("Transport"),		4, function_insert_verb },
/* Translators: Menu item Verb/Channel/Transfer/Transport/Lift */
      {N_("Lift"),		4, function_insert_verb },
/* Translators: Menu item Verb/Channel/Transfer/Transport/Move */
      {N_("Move"),		4, function_insert_verb },
/* Translators: Menu item Verb/Channel/Transfer/Transport/Channel */
      {N_("Channel"),		4, function_insert_verb },
/* Translators: Menu item Verb/Channel/Transfer/Transmit */
     {N_("Transmit"),		3, NULL },
/* Translators: Menu item Verb/Channel/Transfer/Transmit/Transmit */
      {N_("Transmit"),		4, function_insert_verb },
/* Translators: Menu item Verb/Channel/Transfer/Transmit/Conduct */
      {N_("Conduct"),		4, function_insert_verb },
/* Translators: Menu item Verb/Channel/Transfer/Transmit/Transfer */
      {N_("Transfer"),		4, function_insert_verb },
/* Translators: Menu item Verb/Channel/Transfer/Transmit/Convey */
      {N_("Convey"),		4, function_insert_verb },
/* Translators: Menu item Verb/Channel/Guide */
    {N_("Guide"),		2, NULL },
/* Translators: Menu item Verb/Channel/Guide/Guide */
     {N_("Guide"),		3, NULL },
/* Translators: Menu item Verb/Channel/Guide/Guide/Guide */
      {N_("Guide"),		4, function_insert_verb },
/* Translators: Menu item Verb/Channel/Guide/Guide/Direct */
      {N_("Direct"),		4, function_insert_verb },
/* Translators: Menu item Verb/Channel/Guide/Guide/Straighten */
      {N_("Straighten"),	4, function_insert_verb },
/* Translators: Menu item Verb/Channel/Guide/Guide/Steer */
      {N_("Steer"),		4, function_insert_verb },
/* Translators: Menu item Verb/Channel/Guide/Translate */
     {N_("Translate"),		3, function_insert_verb },
/* Translators: Menu item Verb/Channel/Guide/Rotate */
     {N_("Rotate"),		3, NULL },
/* Translators: Menu item Verb/Channel/Guide/Rotate/Rotate */
      {N_("Rotate"),		4, function_insert_verb },
/* Translators: Menu item Verb/Channel/Guide/Rotate/Turn */
      {N_("Turn"),		4, function_insert_verb },
/* Translators: Menu item Verb/Channel/Guide/Rotate/Spin */
      {N_("Spin"),		4, function_insert_verb },
/* Translators: Menu item Verb/Channel/Guide/Allow DOF */
     {N_("Allow DOF"),		3, NULL },
/* Translators: Menu item Verb/Channel/Guide/Allow DOF/Allow DOF */
      {N_("Allow DOF"),		4, function_insert_verb },
/* Translators: Menu item Verb/Channel/Guide/Allow DOF/Constrain */
      {N_("Constrain"),		4, function_insert_verb },
/* Translators: Menu item Verb/Channel/Guide/Allow DOF/Unlock */
      {N_("Unlock"),		4, function_insert_verb },
/* Translators: Menu item Verb/Support */
   {N_("Support"),		1, NULL },
/* Translators: Menu item Verb/Support/Support */
    {N_("Support"),		2, function_insert_verb },
/* Translators: Menu item Verb/Support/Stop */
    {N_("Stop"),		2, NULL },
/* Translators: Menu item Verb/Support/Stop/Stop */
     {N_("Stop"),		3, function_insert_verb },
/* Translators: Menu item Verb/Support/Stop/Insulate */
     {N_("Insulate"),		3, function_insert_verb },
/* Translators: Menu item Verb/Support/Stop/Protect */
     {N_("Protect"),		3, function_insert_verb },
/* Translators: Menu item Verb/Support/Stop/Prevent */
     {N_("Prevent"),		3, function_insert_verb },
/* Translators: Menu item Verb/Support/Stop/Shield */
     {N_("Shield"),		3, function_insert_verb },
/* Translators: Menu item Verb/Support/Stop/Inhibit */
     {N_("Inhibit"),		3, function_insert_verb },
/* Translators: Menu item Verb/Support/Stabilize */
    {N_("Stabilize"),		2, NULL },
/* Translators: Menu item Verb/Support/Stabilize/Stabilize */
     {N_("Stabilize"),		3, function_insert_verb },
/* Translators: Menu item Verb/Support/Stabilize/Steady */
     {N_("Steady"),		3, function_insert_verb },
/* Translators: Menu item Verb/Support/Secure */
    {N_("Secure"),		2, NULL },
/* Translators: Menu item Verb/Support/Secure/Secure */
     {N_("Secure"),		3, function_insert_verb },
/* Translators: Menu item Verb/Support/Secure/Attach */
     {N_("Attach"),		3, function_insert_verb },
/* Translators: Menu item Verb/Support/Secure/Mount */
     {N_("Mount"),		3, function_insert_verb },
/* Translators: Menu item Verb/Support/Secure/Lock */
     {N_("Lock"),		3, function_insert_verb },
/* Translators: Menu item Verb/Support/Secure/Fasten */
     {N_("Fasten"),		3, function_insert_verb },
/* Translators: Menu item Verb/Support/Secure/Hold */
     {N_("Hold"),		3, function_insert_verb },
/* Translators: Menu item Verb/Support/Position */
    {N_("Position"),		2, NULL },
/* Translators: Menu item Verb/Support/Position/Position */
     {N_("Position"),		3, function_insert_verb },
/* Translators: Menu item Verb/Support/Position/Orient */
     {N_("Orient"),		3, function_insert_verb },
/* Translators: Menu item Verb/Support/Position/Align */
     {N_("Align"),		3, function_insert_verb },
/* Translators: Menu item Verb/Support/Position/Locate */
     {N_("Locate"),		3, function_insert_verb },
/* Translators: Menu item Verb/Connect */
   {N_("Connect"),		1, NULL },
/* Translators: Menu item Verb/Connect/Connect */
    {N_("Connect"),		2, function_insert_verb },
/* Translators: Menu item Verb/Connect/Couple */
    {N_("Couple"),		2, NULL },
/* Translators: Menu item Verb/Connect/Couple/Couple */
     {N_("Couple"),		3, function_insert_verb },
/* Translators: Menu item Verb/Connect/Couple/Join */
     {N_("Join"),		3, function_insert_verb },
/* Translators: Menu item Verb/Connect/Couple/Assemble */
     {N_("Assemble"),		3, function_insert_verb },
/* Translators: Menu item Verb/Connect/Couple/Attach */
     {N_("Attach"),		3, function_insert_verb },
/* Translators: Menu item Verb/Connect/Mix */
    {N_("Mix"),			2, NULL },
/* Translators: Menu item Verb/Connect/Mix/Mix */
     {N_("Mix"),		3, function_insert_verb },
/* Translators: Menu item Verb/Connect/Mix/Combine */
     {N_("Combine"),		3, function_insert_verb },
/* Translators: Menu item Verb/Connect/Mix/Blend */
     {N_("Blend"),		3, function_insert_verb },
/* Translators: Menu item Verb/Connect/Mix/Add */
     {N_("Add"),		3, function_insert_verb },
/* Translators: Menu item Verb/Connect/Mix/Pack */
     {N_("Pack"),		3, function_insert_verb },
/* Translators: Menu item Verb/Connect/Mix/Coalesce */
     {N_("Coalesce"),		3, function_insert_verb },
/* Translators: Menu item Verb/Branch */
   {N_("Branch"),		1, NULL },
/* Translators: Menu item Verb/Branch/Branch */
    {N_("Branch"),		2, function_insert_verb },
/* Translators: Menu item Verb/Branch/Separate */
    {N_("Separate"),		2, NULL },
/* Translators: Menu item Verb/Branch/Separate/Separate */
     {N_("Separate"),		3, NULL },
/* Translators: Menu item Verb/Branch/Separate/Separate/Separate */
      {N_("Separate"),		4, function_insert_verb },
/* Translators: Menu item Verb/Branch/Separate/Separate/Switch */
      {N_("Switch"),		4, function_insert_verb },
/* Translators: Menu item Verb/Branch/Separate/Separate/Divide */
      {N_("Divide"),		4, function_insert_verb },
/* Translators: Menu item Verb/Branch/Separate/Separate/Release */
      {N_("Release"),		4, function_insert_verb },
/* Translators: Menu item Verb/Branch/Separate/Separate/Detach */
      {N_("Detach"),		4, function_insert_verb },
/* Translators: Menu item Verb/Branch/Separate/Separate/Disconnect */
      {N_("Disconnect"),	4, function_insert_verb },
/* Translators: Menu item Verb/Branch/Separate/Remove */
     {N_("Remove"),		3, NULL },
/* Translators: Menu item Verb/Branch/Separate/Remove/Remove */
      {N_("Remove"),		4, function_insert_verb },
/* Translators: Menu item Verb/Branch/Separate/Remove/Cut */
      {N_("Cut"),		4, function_insert_verb },
/* Translators: Menu item Verb/Branch/Separate/Remove/Polish */
      {N_("Polish"),		4, function_insert_verb },
/* Translators: Menu item Verb/Branch/Separate/Remove/Sand */
      {N_("Sand"),		4, function_insert_verb },
/* Translators: Menu item Verb/Branch/Separate/Remove/Drill */
      {N_("Drill"),		4, function_insert_verb },
/* Translators: Menu item Verb/Branch/Separate/Remove/Lathe */
      {N_("Lathe"),		4, function_insert_verb },
/* Translators: Menu item Verb/Branch/Refine */
    {N_("Refine"),		2, NULL },
/* Translators: Menu item Verb/Branch/Refine/Refine */
     {N_("Refine"),		3, function_insert_verb },
/* Translators: Menu item Verb/Branch/Refine/Purify */
     {N_("Purify"),		3, function_insert_verb },
/* Translators: Menu item Verb/Branch/Refine/Strain */
     {N_("Strain"),		3, function_insert_verb },
/* Translators: Menu item Verb/Branch/Refine/Filter */
     {N_("Filter"),		3, function_insert_verb },
/* Translators: Menu item Verb/Branch/Refine/Percolate */
     {N_("Percolate"),		3, function_insert_verb },
/* Translators: Menu item Verb/Branch/Refine/Clear */
     {N_("Clear"),		3, function_insert_verb },
/* Translators: Menu item Verb/Branch/Distribute */
    {N_("Distribute"),		2, NULL },
/* Translators: Menu item Verb/Branch/Distribute/Distribute */
     {N_("Distribute"),		3, function_insert_verb },
/* Translators: Menu item Verb/Branch/Distribute/Diverge */
     {N_("Diverge"),		3, function_insert_verb },
/* Translators: Menu item Verb/Branch/Distribute/Scatter */
     {N_("Scatter"),		3, function_insert_verb },
/* Translators: Menu item Verb/Branch/Distribute/Disperse */
     {N_("Disperse"),		3, function_insert_verb },
/* Translators: Menu item Verb/Branch/Distribute/Diffuse */
     {N_("Diffuse"),		3, function_insert_verb },
/* Translators: Menu item Verb/Branch/Distribute/Empty */
     {N_("Empty"),		3, function_insert_verb },
/* Translators: Menu item Verb/Branch/Dissipate */
    {N_("Dissipate"),		2, NULL },
/* Translators: Menu item Verb/Branch/Dissipate/Dissipate */
     {N_("Dissipate"),		3, function_insert_verb },
/* Translators: Menu item Verb/Branch/Dissipate/Absorb */
     {N_("Absorb"),		3, function_insert_verb },
/* Translators: Menu item Verb/Branch/Dissipate/Dampen */
     {N_("Dampen"),		3, function_insert_verb },
/* Translators: Menu item Verb/Branch/Dissipate/Dispel */
     {N_("Dispel"),		3, function_insert_verb },
/* Translators: Menu item Verb/Branch/Dissipate/Diffuse */
     {N_("Diffuse"),		3, function_insert_verb },
/* Translators: Menu item Verb/Branch/Dissipate/Resist */
     {N_("Resist"),		3, function_insert_verb },
/* Translators: Menu item Verb/Provision */
   {N_("Provision"),		1, NULL },
/* Translators: Menu item Verb/Provision/Provision */
    {N_("Provision"),		2, function_insert_verb },
/* Translators: Menu item Verb/Provision/Store */
    {N_("Store"),		2, NULL },
/* Translators: Menu item Verb/Provision/Store/Store */
     {N_("Store"),		3, function_insert_verb },
/* Translators: Menu item Verb/Provision/Store/Contain */
     {N_("Contain"),		3, function_insert_verb },
/* Translators: Menu item Verb/Provision/Store/Collect */
     {N_("Collect"),		3, function_insert_verb },
/* Translators: Menu item Verb/Provision/Store/Reserve */
     {N_("Reserve"),		3, function_insert_verb },
/* Translators: Menu item Verb/Provision/Store/Capture */
     {N_("Capture"),		3, function_insert_verb },
/* Translators: Menu item Verb/Provision/Supply */
    {N_("Supply"),		2, NULL },
/* Translators: Menu item Verb/Provision/Supply/Supply */
     {N_("Supply"),		3, function_insert_verb },
/* Translators: Menu item Verb/Provision/Supply/Fill */
     {N_("Fill"),		3, function_insert_verb },
/* Translators: Menu item Verb/Provision/Supply/Provide */
     {N_("Provide"),		3, function_insert_verb },
/* Translators: Menu item Verb/Provision/Supply/Replenish */
     {N_("Replenish"),		3, function_insert_verb },
/* Translators: Menu item Verb/Provision/Supply/Expose */
     {N_("Expose"),		3, function_insert_verb },
/* Translators: Menu item Verb/Provision/Extract */
    {N_("Extract"),		2, function_insert_verb },
/* Translators: Menu item Verb/Control Magnitude */
   {N_("Control Magnitude"),	1, NULL },
/* Translators: Menu item Verb/Control Magnitude/Control Magnitude */
    {N_("Control Magnitude"),	2, function_insert_verb },
/* Translators: Menu item Verb/Control Magnitude/Actuate */
    {N_("Actuate"),		2, NULL },
/* Translators: Menu item Verb/Control Magnitude/Actuate/Actuate */
     {N_("Actuate"),		3, function_insert_verb },
/* Translators: Menu item Verb/Control Magnitude/Actuate/Start */
     {N_("Start"),		3, function_insert_verb },
/* Translators: Menu item Verb/Control Magnitude/Actuate/Initiate */
     {N_("Initiate"),		3, function_insert_verb },
/* Translators: Menu item Verb/Control Magnitude/Regulate */
    {N_("Regulate"),		2, NULL },
/* Translators: Menu item Verb/Control Magnitude/Regulate/Regulate */
     {N_("Regulate"),		3, function_insert_verb },
/* Translators: Menu item Verb/Control Magnitude/Regulate/Control */
     {N_("Control"),		3, function_insert_verb },
/* Translators: Menu item Verb/Control Magnitude/Regulate/Allow */
     {N_("Allow"),		3, function_insert_verb },
/* Translators: Menu item Verb/Control Magnitude/Regulate/Prevent */
     {N_("Prevent"),		3, function_insert_verb },
/* Translators: Menu item Verb/Control Magnitude/Regulate/Enable */
     {N_("Enable"),		3, function_insert_verb },
/* Translators: Menu item Verb/Control Magnitude/Regulate/Disable */
     {N_("Disable"),		3, function_insert_verb },
/* Translators: Menu item Verb/Control Magnitude/Regulate/Limit */
     {N_("Limit"),		3, function_insert_verb },
/* Translators: Menu item Verb/Control Magnitude/Regulate/Interrupt */
     {N_("Interrupt"),		3, function_insert_verb },
/* Translators: Menu item Verb/Control Magnitude/Change */
    {N_("Change"),		2, NULL },
/* Translators: Menu item Verb/Control Magnitude/Change/Change */
     {N_("Change"),		3, function_insert_verb },
/* Translators: Menu item Verb/Control Magnitude/Change/Increase */
     {N_("Increase"),		3, function_insert_verb },
/* Translators: Menu item Verb/Control Magnitude/Change/Decrease */
     {N_("Decrease"),		3, function_insert_verb },
/* Translators: Menu item Verb/Control Magnitude/Change/Amplify */
     {N_("Amplify"),		3, function_insert_verb },
/* Translators: Menu item Verb/Control Magnitude/Change/Reduce */
     {N_("Reduce"),		3, function_insert_verb },
/* Translators: Menu item Verb/Control Magnitude/Change/Magnify */
     {N_("Magnify"),		3, function_insert_verb },
/* Translators: Menu item Verb/Control Magnitude/Change/Normalize */
     {N_("Normalize"),		3, function_insert_verb },
/* Translators: Menu item Verb/Control Magnitude/Change/Multiply */
     {N_("Multiply"),		3, function_insert_verb },
/* Translators: Menu item Verb/Control Magnitude/Change/Scale */
     {N_("Scale"),		3, function_insert_verb },
/* Translators: Menu item Verb/Control Magnitude/Change/Rectify */
     {N_("Rectify"),		3, function_insert_verb },
/* Translators: Menu item Verb/Control Magnitude/Change/Adjust */
     {N_("Adjust"),		3, function_insert_verb },
/* Translators: Menu item Verb/Control Magnitude/Form */
    {N_("Form"),		2, NULL },
/* Translators: Menu item Verb/Control Magnitude/Form/Form */
     {N_("Form"),		3, function_insert_verb },
/* Translators: Menu item Verb/Control Magnitude/Form/Compact */
     {N_("Compact"),		3, function_insert_verb },
/* Translators: Menu item Verb/Control Magnitude/Form/Crush */
     {N_("Crush"),		3, function_insert_verb },
/* Translators: Menu item Verb/Control Magnitude/Form/Shape */
     {N_("Shape"),		3, function_insert_verb },
/* Translators: Menu item Verb/Control Magnitude/Form/Compress */
     {N_("Compress"),		3, function_insert_verb },
/* Translators: Menu item Verb/Control Magnitude/Form/Pierce */
     {N_("Pierce"),		3, function_insert_verb },
/* Translators: Menu item Verb/Convert */
   {N_("Convert"),		1, NULL },
/* Translators: Menu item Verb/Convert/Convert */
    {N_("Convert"),		2, function_insert_verb },
/* Translators: Menu item Verb/Convert/Transform */
    {N_("Transform"),		2, function_insert_verb },
/* Translators: Menu item Verb/Convert/Liquefy */
    {N_("Liquefy"),		2, function_insert_verb },
/* Translators: Menu item Verb/Convert/Solidify */
    {N_("Solidify"),		2, function_insert_verb },
/* Translators: Menu item Verb/Convert/Evaporate */
    {N_("Evaporate"),		2, function_insert_verb },
/* Translators: Menu item Verb/Convert/Sublimate */
    {N_("Sublimate"),		2, function_insert_verb },
/* Translators: Menu item Verb/Convert/Condense */
    {N_("Condense"),		2, function_insert_verb },
/* Translators: Menu item Verb/Convert/Integrate */
    {N_("Integrate"),		2, function_insert_verb },
/* Translators: Menu item Verb/Convert/Differentiate */
    {N_("Differentiate"),	2, function_insert_verb },
/* Translators: Menu item Verb/Convert/Process */
    {N_("Process"),		2, function_insert_verb },
/* Translators: Menu item Verb/Signal */
   {N_("Signal"),		1, NULL },
/* Translators: Menu item Verb/Signal/Signal */
    {N_("Signal"),		2, function_insert_verb },
/* Translators: Menu item Verb/Signal/Sense */
    {N_("Sense"),		2, NULL },
/* Translators: Menu item Verb/Signal/Sense/Sense */
     {N_("Sense"),		3, function_insert_verb },
/* Translators: Menu item Verb/Signal/Sense/Perceive */
     {N_("Perceive"),		3, function_insert_verb },
/* Translators: Menu item Verb/Signal/Sense/Recognize */
     {N_("Recognize"),		3, function_insert_verb },
/* Translators: Menu item Verb/Signal/Sense/Discern */
     {N_("Discern"),		3, function_insert_verb },
/* Translators: Menu item Verb/Signal/Sense/Check */
     {N_("Check"),		3, function_insert_verb },
/* Translators: Menu item Verb/Signal/Sense/Locate */
     {N_("Locate"),		3, function_insert_verb },
/* Translators: Menu item Verb/Signal/Sense/Verify */
     {N_("Verify"),		3, function_insert_verb },
/* Translators: Menu item Verb/Signal/Indicate */
    {N_("Indicate"),		2, NULL },
/* Translators: Menu item Verb/Signal/Indicate/Indicate */
     {N_("Indicate"),		3, function_insert_verb },
/* Translators: Menu item Verb/Signal/Indicate/Mark */
     {N_("Mark"),		3, function_insert_verb },
/* Translators: Menu item Verb/Signal/Display */
    {N_("Display"),		2, function_insert_verb },
/* Translators: Menu item Verb/Signal/Measure */
    {N_("Measure"),		2, NULL },
/* Translators: Menu item Verb/Signal/Measure/Measure */
     {N_("Measure"),		3, function_insert_verb },
/* Translators: Menu item Verb/Signal/Measure/Calculate */
     {N_("Calculate"),		3, function_insert_verb },
/* Translators: Menu item Verb/Signal/Represent */
    {N_("Represent"),		2, function_insert_verb },
/* Translators: Menu item Noun */
  {N_("Noun"),			0, NULL },
/* Translators: Menu item Noun/Material */
    {N_("Material"),		1, NULL },
/* Translators: Menu item Noun/Material/Solid */
     {N_("Solid"),		2, function_insert_noun },
/* Translators: Menu item Noun/Material/Liquid */
     {N_("Liquid"),		2, function_insert_noun },
/* Translators: Menu item Noun/Material/Gas */
     {N_("Gas"),		2, function_insert_noun },
/* Translators: Menu item Noun/Material/Human */
     {N_("Human"),		2, NULL },
/* Translators: Menu item Noun/Material/Human/Human */
      {N_("Human"),		3, function_insert_noun },
/* Translators: Menu item Noun/Material/Human/Hand */
      {N_("Hand"),		3, function_insert_noun },
/* Translators: Menu item Noun/Material/Human/Foot */
      {N_("Foot"),		3, function_insert_noun },
/* Translators: Menu item Noun/Material/Human/Head */
      {N_("Head"),		3, function_insert_noun },
/* Translators: Menu item Noun/Material/Human/Finger */
      {N_("Finger"),		3, function_insert_noun },
/* Translators: Menu item Noun/Material/Human/Toe */
      {N_("Toe"),		3, function_insert_noun },
/* Translators: Menu item Noun/Material/Biological */
     {N_("Biological"),		2, function_insert_noun },
/* Translators: Menu item Noun/Energy */
    {N_("Energy"),		1, NULL },
/* Translators: Menu item Noun/Energy/Mechanical */
     {N_("Mechanical"),		2, NULL },
/* Translators: Menu item Noun/Energy/Mechanical/Mech. Energy */
      {N_("Mech. Energy"),	3, function_insert_noun },
/* Translators: Menu item Noun/Energy/Mechanical/Translation */
      {N_("Translation"),	3, function_insert_noun },
/* Translators: Menu item Noun/Energy/Mechanical/Force */
      {N_("Force"),		3, function_insert_noun },
/* Translators: Menu item Noun/Energy/Mechanical/Rotation */
      {N_("Rotation"),		3, function_insert_noun },
/* Translators: Menu item Noun/Energy/Mechanical/Torque */
      {N_("Torque"),		3, function_insert_noun },
/* Translators: Menu item Noun/Energy/Mechanical/Random Motion */
      {N_("Random Motion"),	3, function_insert_noun },
/* Translators: Menu item Noun/Energy/Mechanical/Vibration */
      {N_("Vibration"),		3, function_insert_noun },
/* Translators: Menu item Noun/Energy/Mechanical/Rotational Energy */
      {N_("Rotational Energy"),	3, function_insert_noun },
/* Translators: Menu item Noun/Energy/Mechanical/Translational Energy */
      {N_("Translational Energy"),3, function_insert_noun },
/* Translators: Menu item Noun/Energy/Electricity */
     {N_("Electrical"),		2, NULL },
/* Translators: Menu item Noun/Energy/Electricity/Electricity */
      {N_("Electricity"),	3, function_insert_noun },
/* Translators: Menu item Noun/Energy/Electricity/Voltage */
      {N_("Voltage"),		3, function_insert_noun },
/* Translators: Menu item Noun/Energy/Electricity/Current */
      {N_("Current"),		3, function_insert_noun },
/* Translators: Menu item Noun/Energy/Hydraulic */
     {N_("Hydraulic"),		2, function_insert_noun },
/* Translators: Menu item Noun/Energy/Hydraulic/Pressure */
      {N_("Pressure"),		3, function_insert_noun },
/* Translators: Menu item Noun/Energy/Hydraulic/Volumetric Flow */
      {N_("Volumetric Flow"),	3, function_insert_noun },
/* Translators: Menu item Noun/Energy/Thermal */
     {N_("Thermal"),		2, NULL },
/* Translators: Menu item Noun/Energy/Thermal/Heat */
      {N_("Heat"),		3, function_insert_noun },
/* Translators: Menu item Noun/Energy/Thermal/Conduction */
      {N_("Conduction"),	3, function_insert_noun },
/* Translators: Menu item Noun/Energy/Thermal/Convection */
      {N_("Convection"),	3, function_insert_noun },
/* Translators: Menu item Noun/Energy/Thermal/Radiation */
      {N_("Radiation"),		3, function_insert_noun },
/* Translators: Menu item Noun/Energy/Pneumatic */
     {N_("Pneumatic"),		2, function_insert_noun },
/* Translators: Menu item Noun/Energy/Chemical */
     {N_("Chemical"),		2, function_insert_noun },
/* Translators: Menu item Noun/Energy/Radioactive */
     {N_("Radioactive"),	2, NULL },
/* Translators: Menu item Noun/Energy/Radioactive/Radiation */
      {N_("Radiation"),		3, function_insert_noun },
/* Translators: Menu item Noun/Energy/Radioactive/Microwaves */
      {N_("Microwaves"),	3, function_insert_noun },
/* Translators: Menu item Noun/Energy/Radioactive/Radio waves */
      {N_("Radio waves"),	3, function_insert_noun },
/* Translators: Menu item Noun/Energy/Radioactive/X-Rays */
      {N_("X-Rays"),		3, function_insert_noun },
/* Translators: Menu item Noun/Energy/Radioactive/Gamma Rays */
      {N_("Gamma Rays"),	3, function_insert_noun },
/* Translators: Menu item Noun/Energy/Acoustic Energy */
     {N_("Acoustic Energy"),	2, function_insert_noun },
/* Translators: Menu item Noun/Energy/Optical Energy */
     {N_("Optical Energy"),	2, function_insert_noun },
/* Translators: Menu item Noun/Energy/Solar Energy */
     {N_("Solar Energy"),	2, function_insert_noun },
/* Translators: Menu item Noun/Energy/Magnetic Energy */
     {N_("Magnetic Energy"),	2, function_insert_noun },
/* Translators: Menu item Noun/Energy/Human */
     {N_("Human"),		2, NULL },
/* Translators: Menu item Noun/Energy/Human/Human Motion */
      {N_("Human Motion"),	3, function_insert_noun },
/* Translators: Menu item Noun/Energy/Human/Human Force */
      {N_("Human Force"),	3, function_insert_noun },
/* Translators: Menu item Noun/Signal */
    {N_("Signal"),		1, NULL },
/* Translators: Menu item Noun/Signal/Signal */
     {N_("Signal"),		2, function_insert_noun },
/* Translators: Menu item Noun/Signal/Status */
     {N_("Status"),		2, function_insert_noun },
/* Translators: Menu item Noun/Signal/Control */
     {N_("Control"),		2, function_insert_noun },
  {NULL,			0, NULL },
  {N_("User/Device Fn"),	0, function_toggle_user_function },
  {N_("Wish Fn"),		0, function_toggle_wish_function },
  { NULL,			-1, NULL }
} ;

static DiaMenu* function_menu = 0 ;

static int
function_count_submenu_items( struct _IndentedMenus* itemPtr )
{
  int cnt = 0 ;
  int depth = itemPtr->depth ;
  while ( itemPtr->depth >= depth ) {
    if ( itemPtr->depth == depth )
      cnt++ ;
    itemPtr++ ;
  }
  return cnt ;
}


static DiaMenu *
function_get_object_menu (Function *func, Point *clickedpoint)
{
  if (!function_menu) {
    int curDepth = 0;
    DiaMenu *curMenu[FS_SUBMENU_MAXINDENT];
    int curitem[FS_SUBMENU_MAXINDENT];

    curitem[0] = 0;
    curMenu[0] = g_new0 (DiaMenu, 1);
    curMenu[0]->title = "Function";
    curMenu[0]->num_items = function_count_submenu_items (&(fmenu[0]));
    curMenu[0]->items = g_new0 (DiaMenuItem, curMenu[0]->num_items);
    curMenu[0]->app_data = NULL;

    for (int i = 0; fmenu[i].depth >= 0; i++) {
      if (fmenu[i].depth > curDepth) {
        curDepth++;
        curMenu[curDepth] = g_new0 (DiaMenu, 1);
        curMenu[curDepth]->title = NULL;
        curMenu[curDepth]->app_data = NULL;
        curMenu[curDepth]->num_items = function_count_submenu_items (&fmenu[i]);
        curMenu[curDepth]->items = g_new0 (DiaMenuItem, curMenu[curDepth]->num_items);
        /* Point this menu's parent to this new structure */
        curMenu[curDepth-1]->items[curitem[curDepth-1]-1].callback = NULL ;
        curMenu[curDepth-1]->items[curitem[curDepth-1]-1].callback_data =
          curMenu[curDepth];
        curitem[curDepth] = 0 ;
      } else if (fmenu[i].depth < curDepth) {
        curDepth = fmenu[i].depth;
      }

      curMenu[curDepth]->items[curitem[curDepth]].text = fmenu[i].name;
      curMenu[curDepth]->items[curitem[curDepth]].callback = fmenu[i].func;
      curMenu[curDepth]->items[curitem[curDepth]].callback_data = fmenu[i].name;
      curMenu[curDepth]->items[curitem[curDepth]].active = 1;
      curitem[curDepth]++;
    }

    function_menu = curMenu[0];
  }
  return function_menu;
}
