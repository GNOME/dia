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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <gtk/gtk.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "intl.h"
#include "object.h"
#include "element.h"
#include "render.h"
#include "attributes.h"
#include "text.h"
#include "properties.h"

#include "pixmaps/function.xpm"

typedef struct _Function Function;
typedef struct _FunctionChange FunctionChange;

struct _Function {
  Element element;

  ConnectionPoint connections[8];
  
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

struct _FunctionChange {
  ObjectChange		obj_change ;
  enum FuncChangeType	change_type ;
  int			is_wish ;
  int			is_user ;
  char*			text ;
};

#define FUNCTION_BORDERWIDTH 0.1
#define FUNCTION_LINEWIDTH 0.05
#define FUNCTION_MARGIN_X 0.5
#define FUNCTION_MARGIN_Y 0.5
#define FUNCTION_MARGIN_M 0.4
#define FUNCTION_FONTHEIGHT 0.8

static real function_distance_from(Function *pkg, Point *point);
static void function_select(Function *pkg, Point *clicked_point,
			    Renderer *interactive_renderer);
static void function_move_handle(Function *pkg, Handle *handle,
				 Point *to, HandleMoveReason reason);
static void function_move(Function *pkg, Point *to);
static void function_draw(Function *pkg, Renderer *renderer);
static Object *function_create(Point *startpoint,
			       void *user_data,
			       Handle **handle1,
			       Handle **handle2);
static void function_destroy(Function *pkg);
static Object *function_copy(Function *pkg);
static void function_save(Function *pkg, ObjectNode obj_node,
			  const char *filename);
static Object *function_load(ObjectNode obj_node, int version,
			     const char *filename);
static void function_update_data(Function *pkg);
static DiaMenu *function_get_object_menu(Function *func, Point *clickedpoint) ;
static PropDescription *function_describe_props(Function *mes);
static void
function_get_props(Function * function, Property *props, guint nprops);
static void
function_set_props(Function * function, Property *props, guint nprops);

static ObjectTypeOps function_type_ops =
{
  (CreateFunc) function_create,
  (LoadFunc)   function_load,
  (SaveFunc)   function_save
};

ObjectType function_type =
{
  "FS - Function",   /* name */
  0,                      /* version */
  (char **) function_xpm,  /* pixmap */
  
  &function_type_ops       /* ops */
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
  (ApplyPropertiesFunc) object_apply_props_from_dialog,
  (ObjectMenuFunc)      function_get_object_menu,
  (DescribePropsFunc)   function_describe_props,
  (GetPropsFunc)        function_get_props,
  (SetPropsFunc)        function_set_props
};

static PropDescription function_props[] = {
  ELEMENT_COMMON_PROPERTIES,
  { "wish function", PROP_TYPE_BOOL, PROP_FLAG_VISIBLE,
    N_("Wish function"), NULL, NULL },
  { "user function", PROP_TYPE_BOOL, PROP_FLAG_VISIBLE,
    N_("User function"), NULL, NULL },
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
  OBJECT_COMMON_PROPERTIES_OFFSETS,
  { "wish function", PROP_TYPE_BOOL, offsetof(Function, is_wish) },
  { "user function", PROP_TYPE_BOOL, offsetof(Function, is_user) },
  { NULL, 0, 0}
};

static void
function_get_props(Function * function, Property *props, guint nprops)
{
  object_get_props_from_offsets(&function->element.object, 
                                function_offsets, props, nprops);
}

static void
function_set_props(Function *function, Property *props, guint nprops)
{
  object_set_props_from_offsets(&function->element.object, 
                                function_offsets, props, nprops);
  function_update_data(function);
}

static void
function_change_apply_revert( ObjectChange* objchg, Object* obj)
{
  int tmp ;
  char* ttxt ;
  FunctionChange* change = (FunctionChange*) objchg ;
  Function* fcn = (Function*) obj ;

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
     g_free( change->text ) ;
     change->text = ttxt ;
  }
}

static void
function_change_free( ObjectChange* objchg )
{
  FunctionChange* change = (FunctionChange*) objchg ;

  if ( change->change_type == TEXT_EDIT ) {
     g_free( change->text ) ;
  }
}

static ObjectChange*
function_create_change( Function* fcn, enum FuncChangeType change_type )
{
  FunctionChange* change = g_new0(FunctionChange,1) ;
  change->obj_change.apply = (ObjectChangeApplyFunc) function_change_apply_revert ;
  change->obj_change.revert = (ObjectChangeRevertFunc) function_change_apply_revert ;
  change->obj_change.free = (ObjectChangeFreeFunc) function_change_free ;
  change->change_type = change_type ;

  if ( change_type == WISH_FUNC || change_type == ALL )
     change->is_wish = fcn->is_wish ;

  if ( change_type == USER_FUNC || change_type == ALL )
     change->is_user = fcn->is_user ;

  if ( change_type == TEXT_EDIT || change_type == ALL )
     change->text = text_get_string_copy( fcn->text ) ;
  return (ObjectChange*) change ;
}
  
static real
function_distance_from(Function *pkg, Point *point)
{
  Object *obj = &pkg->element.object;
  return distance_rectangle_point(&obj->bounding_box, point);
}

static void
function_select(Function *pkg, Point *clicked_point,
		Renderer *interactive_renderer)
{
  text_set_cursor(pkg->text, clicked_point, interactive_renderer);
  text_grab_focus(pkg->text, &pkg->element.object);
  element_update_handles(&pkg->element);
}

static void
function_move_handle(Function *pkg, Handle *handle,
		     Point *to, HandleMoveReason reason)
{
  assert(pkg!=NULL);
  assert(handle!=NULL);
  assert(to!=NULL);

  assert(handle->id < 8);
}

static void
function_move(Function *pkg, Point *to)
{
  pkg->element.corner = *to;
  function_update_data(pkg);
}

static void
function_draw(Function *pkg, Renderer *renderer)
{
  Element *elem;
  real x, y, w, h;
  Point p1, p2;
  
  assert(pkg != NULL);
  assert(renderer != NULL);

  elem = &pkg->element;

  x = elem->corner.x;
  y = elem->corner.y;
  w = elem->width;
  h = elem->height;

  renderer->ops->set_fillstyle(renderer, FILLSTYLE_SOLID);
  renderer->ops->set_linewidth(renderer, FUNCTION_BORDERWIDTH );
  renderer->ops->set_linestyle(renderer, pkg->is_wish ? LINESTYLE_DASHED : LINESTYLE_SOLID);


  p1.x = x; p1.y = y;
  p2.x = x+w; p2.y = y+h;

  if (pkg->is_user) {
    renderer->ops->fill_rect(renderer, 
			     &p1, &p2,
			     &color_white); 
    renderer->ops->draw_rect(renderer, 
			     &p1, &p2,
			     &color_black);
    p1.x += FUNCTION_MARGIN_M;
    p1.y += FUNCTION_MARGIN_M;
    p2.y -= FUNCTION_MARGIN_M;
    p2.x -= FUNCTION_MARGIN_M;
    /* y += FUNCTION_MARGIN_M; */
  }
    
  renderer->ops->fill_rect(renderer, 
			   &p1, &p2,
			   &color_white);
  renderer->ops->draw_rect(renderer, 
			   &p1, &p2,
			   &color_black);

  
  text_draw(pkg->text, renderer);

}

static void
function_update_data(Function *pkg)
{
  Element *elem = &pkg->element;
  Object *obj = &elem->object;
  Font *font;
  Point p1;
  real h, w = 0;
  
  font = pkg->text->font;
  h = elem->corner.y + FUNCTION_MARGIN_Y;

  if (pkg->is_user) {
    h += 2*FUNCTION_MARGIN_M;
  }
    
  w = MAX(w, pkg->text->max_width);
  p1.y = h + pkg->text->ascent - ( pkg->is_user ? FUNCTION_MARGIN_M : 0 );  /* position of text */

  h += pkg->text->height*pkg->text->numlines;

  h += FUNCTION_MARGIN_Y;

  w += 2*FUNCTION_MARGIN_X; 

  p1.x = elem->corner.x + w/2.0 + ( pkg->is_user ? FUNCTION_MARGIN_M : 0 );
  text_set_position(pkg->text, &p1);
  
  if (pkg->is_user) {
    w += 2*FUNCTION_MARGIN_M;
  }
    
  elem->width = w;
  elem->height = h - elem->corner.y;

  /* Update connections: */
  pkg->connections[0].pos = elem->corner;
  pkg->connections[1].pos.x = elem->corner.x + elem->width / 2.0;
  pkg->connections[1].pos.y = elem->corner.y;
  pkg->connections[2].pos.x = elem->corner.x + elem->width;
  pkg->connections[2].pos.y = elem->corner.y;
  pkg->connections[3].pos.x = elem->corner.x;
  pkg->connections[3].pos.y = elem->corner.y + elem->height / 2.0;
  pkg->connections[4].pos.x = elem->corner.x + elem->width;
  pkg->connections[4].pos.y = elem->corner.y + elem->height / 2.0;
  pkg->connections[5].pos.x = elem->corner.x;
  pkg->connections[5].pos.y = elem->corner.y + elem->height;
  pkg->connections[6].pos.x = elem->corner.x + elem->width / 2.0;
  pkg->connections[6].pos.y = elem->corner.y + elem->height;
  pkg->connections[7].pos.x = elem->corner.x + elem->width;
  pkg->connections[7].pos.y = elem->corner.y + elem->height;
  
  element_update_boundingbox(elem);

  obj->position = elem->corner;

  element_update_handles(elem);
}

static Object *
function_create(Point *startpoint,
		void *user_data,
		Handle **handle1,
		Handle **handle2)
{
  Function *pkg;
  Element *elem;
  Object *obj;
  Point p;
  Font *font;
  int i;
  
  pkg = g_malloc0(sizeof(Function));
  elem = &pkg->element;
  obj = &elem->object;
  
  obj->type = &function_type;

  obj->ops = &function_ops;

  elem->corner = *startpoint;

  font = font_getfont("Helvetica");
  
  pkg->is_wish = FALSE;
  pkg->is_user = FALSE;

  /* The text position is recalculated later */
  p.x = 0.0;
  p.y = 0.0;
  pkg->text = new_text("", font, 0.8, &p, &color_black, ALIGN_CENTER);
  
  element_init(elem, 8, 8);
  
  for (i=0;i<8;i++) {
    obj->connections[i] = &pkg->connections[i];
    pkg->connections[i].object = obj;
    pkg->connections[i].connected = NULL;
  }
  pkg->element.extra_spacing.border_trans = FUNCTION_BORDERWIDTH/2.0;
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

static Object *
function_copy(Function *pkg)
{
  int i;
  Function *newpkg;
  Element *elem, *newelem;
  Object *newobj;
  
  elem = &pkg->element;
  
  newpkg = g_malloc0(sizeof(Function));
  newelem = &newpkg->element;
  newobj = &newelem->object;

  element_copy(elem, newelem);

  newpkg->text = text_copy(pkg->text);
  
  for (i=0;i<8;i++) {
    newobj->connections[i] = &newpkg->connections[i];
    newpkg->connections[i].object = newobj;
    newpkg->connections[i].connected = NULL;
    newpkg->connections[i].pos = pkg->connections[i].pos;
    newpkg->connections[i].last_pos = pkg->connections[i].last_pos;
  }
  newpkg->is_wish = pkg->is_wish ;
  newpkg->is_user = pkg->is_user ;

  newpkg->element.extra_spacing.border_trans = FUNCTION_BORDERWIDTH/2.0;

  function_update_data(newpkg);
  
  return &newpkg->element.object;
}


static void
function_save(Function *pkg, ObjectNode obj_node, const char *filename)
{
  element_save(&pkg->element, obj_node);

  data_add_text(new_attribute(obj_node, "text"),
		pkg->text);

  data_add_boolean(new_attribute(obj_node, "is_wish"),
		   pkg->is_wish);
  
  data_add_boolean(new_attribute(obj_node, "is_user"),
		   pkg->is_user);
}

static Object *
function_load(ObjectNode obj_node, int version, const char *filename)
{
  Function *pkg;
  AttributeNode attr;
  Element *elem;
  Object *obj;
  int i;
  
  pkg = g_malloc0(sizeof(Function));
  elem = &pkg->element;
  obj = &elem->object;
  
  obj->type = &function_type;
  obj->ops = &function_ops;

  element_load(elem, obj_node);
  
  pkg->text = NULL;
  attr = object_find_attribute(obj_node, "text");
  if (attr != NULL)
    pkg->text = data_text(attribute_first_data(attr));

  attr = object_find_attribute(obj_node, "is_wish");
  if (attr != NULL)
    pkg->is_wish = data_boolean(attribute_first_data(attr));
  else
    pkg->is_wish = FALSE;

  attr = object_find_attribute(obj_node, "is_user");
  if (attr != NULL)
    pkg->is_user = data_boolean(attribute_first_data(attr));
  else
    pkg->is_user = FALSE;

  element_init(elem, 8, 8);

  for (i=0;i<8;i++) {
    obj->connections[i] = &pkg->connections[i];
    pkg->connections[i].object = obj;
    pkg->connections[i].connected = NULL;
  }
  pkg->element.extra_spacing.border_trans = FUNCTION_BORDERWIDTH/2.0;
  function_update_data(pkg);

  for (i=0;i<8;i++) {
    obj->handles[i]->type = HANDLE_NON_MOVABLE;
  }

  return &pkg->element.object;
}

static ObjectChange *
function_insert_word( Object* obj, Point* clicked, gpointer data)
{
  Function* func = (Function*)obj ;
  ObjectChange* change = function_create_change( func, TEXT_EDIT ) ;
  char* word = (char*) data ;
  char* old_chars = text_get_string_copy( func->text ) ;
  char* new_chars = malloc( strlen( old_chars) + strlen( word ) + 1 ) ;
  sprintf( new_chars, "%s%s", old_chars, word ) ;
  text_set_string( func->text, new_chars ) ;
  free( new_chars ) ;
  free( old_chars ) ;
  function_update_data( func ) ;
  text_set_cursor_at_end( func->text ) ;

  return change;
}

struct _IndentedWords {
  char* word ;
  int	spaces ;
} ;

struct _IndentedWords verbs[] = {
  {N_("Channel"),		0 },
  {N_("   Import"),		3 },
  {N_("   Export"),		3 },
  {N_("   Transfer"),		3 },
  {N_("      Transport"),	6 },
  {N_("      Transmit"),	6 },
  {N_("   Guide"),		3 },
  {N_("      Translate"),	6 },
  {N_("      Rotate"),		6 },
  {N_("      Allow DOF"),	6 },
  {N_("Support"),		0 },
  {N_("   Stop"),		3 },
  {N_("   Stabilize"),		3 },
  {N_("   Secure"),		3 },
  {N_("   Position"),		3 },
  {N_("Connect"),		0 },
  {N_("   Couple"),		3 },
  {N_("   Mix"),		3 },
  {N_("Branch"),		0 },
  {N_("   Separate"),		3 },
  {N_("      Remove"),		6 },
  {N_("   Refine"),		3 },
  {N_("   Distribute"),		3 },
  {N_("   Dissipate"),		3 },
  {N_("Provision"),		0 },
  {N_("   Store"),		3 },
  {N_("   Supply"),		3 },
  {N_("   Extract"),		3 },
  {N_("Control Magnitude"),	0 },
  {N_("   Actuate"),		3 },
  {N_("   Regulate"),		3 },
  {N_("   Change"),		3 },
  {N_("   Form"),		3 },
  {N_("Convert"),		0 },
  {N_("Signal"),		0 },
  {N_("   Sense"),		3 },
  {N_("   Indicate"),		3 },
  {N_("   Display"),		3 },
  {N_("   Measure"),		3 }
} ;

static DiaMenuItem* function_menu_items = 0 ;

static DiaMenu function_menu = {
  "Function",
  sizeof(verbs)/sizeof(struct _IndentedWords),
  0,
  NULL
} ;

static DiaMenu*
function_get_object_menu( Function* func, Point* clickedpoint )
{
  int i ;
  if ( ! function_menu_items ) {
    function_menu_items = malloc( function_menu.num_items * 
				  sizeof(DiaMenuItem) ) ;
    for (i = 0 ; i< function_menu.num_items; i++) {
      function_menu_items[i].text = verbs[i].word ;
      function_menu_items[i].callback = function_insert_word ;
      function_menu_items[i].callback_data = (void*) (verbs[i].word + 
						      verbs[i].spaces ) ;
      function_menu_items[i].active = 1 ;
    }
    function_menu.items = function_menu_items ;
  }
  return &function_menu ;
}
