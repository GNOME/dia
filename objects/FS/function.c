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
  TextAttributes attrs;

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

#define FUNCTION_FONTHEIGHT 0.6
#define FUNCTION_BORDERWIDTH_SCALE 6.0
#define FUNCTION_MARGIN_SCALE 3.0
#define FUNCTION_MARGIN_X 2.4
#define FUNCTION_MARGIN_Y 2.4
#define FUNCTION_DASHLENGTH_SCALE 2.0

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
function_get_props(Function * function, GPtrArray *props);
static void
function_set_props(Function * function, GPtrArray *props);

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
  OBJECT_COMMON_PROPERTIES_OFFSETS,
  { "wish function", PROP_TYPE_BOOL, offsetof(Function, is_wish) },
  { "user function", PROP_TYPE_BOOL, offsetof(Function, is_user) },
  { "text", PROP_TYPE_TEXT, offsetof (Function, text) },
  { "text_font", PROP_TYPE_FONT, offsetof (Function, attrs.font) },
  { "text_height", PROP_TYPE_REAL, offsetof (Function, attrs.height) },
  { "text_colour", PROP_TYPE_COLOUR, offsetof (Function, attrs.color) },
  { NULL, 0, 0}
};

static void
function_get_props(Function * function, GPtrArray *props)
{
  text_get_attributes (function->text, &function->attrs);
  object_get_props_from_offsets(&function->element.object, 
                                function_offsets, props);
}

static void
function_set_props(Function *function, GPtrArray *props)
{
  object_set_props_from_offsets(&function->element.object, 
                                function_offsets, props);
  apply_textattr_properties (props, function->text, "text", &function->attrs);
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
  real font_height ;
  
  assert(pkg != NULL);
  assert(pkg->text != NULL);
  assert(renderer != NULL);

  elem = &pkg->element;

  x = elem->corner.x;
  y = elem->corner.y;
  w = elem->width;
  h = elem->height;

  font_height = pkg->text->height ;

  renderer->ops->set_fillstyle(renderer, FILLSTYLE_SOLID);
  renderer->ops->set_linewidth(renderer, font_height / FUNCTION_BORDERWIDTH_SCALE );
  renderer->ops->set_linestyle(renderer, pkg->is_wish ? LINESTYLE_DASHED : LINESTYLE_SOLID);
  if ( pkg->is_wish )
    renderer->ops->set_dashlength( renderer, font_height / FUNCTION_DASHLENGTH_SCALE ) ;


  p1.x = x; p1.y = y;
  p2.x = x+w; p2.y = y+h;

  if (pkg->is_user) {
    renderer->ops->fill_rect(renderer, 
			     &p1, &p2,
			     &color_white); 
    renderer->ops->draw_rect(renderer, 
			     &p1, &p2,
			     &color_black);
    p1.x += font_height / FUNCTION_MARGIN_SCALE;
    p1.y += font_height / FUNCTION_MARGIN_SCALE;
    p2.y -= font_height / FUNCTION_MARGIN_SCALE;
    p2.x -= font_height / FUNCTION_MARGIN_SCALE;
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
  DiaFont *font;
  Point p1;
  real h, w = 0, font_height;
  
  font = pkg->text->font ;
  font_height = pkg->text->height ;
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
  DiaFont *font;
  int i;
  
  pkg = g_malloc0(sizeof(Function));
  elem = &pkg->element;
  obj = &elem->object;
  
  obj->type = &function_type;

  obj->ops = &function_ops;

  elem->corner = *startpoint;

  /* choose default font name for your locale. see also font_data structure
     in lib/font.c. */
  font = font_getfont (_("Helvetica"));
  
  pkg->is_wish = FALSE;
  pkg->is_user = FALSE;

  /* The text position is recalculated later */
  p.x = 0.0;
  p.y = 0.0;
  pkg->text = new_text("", font, FUNCTION_FONTHEIGHT, &p, &color_black, ALIGN_CENTER);
  
  element_init(elem, 8, 8);
  
  for (i=0;i<8;i++) {
    obj->connections[i] = &pkg->connections[i];
    pkg->connections[i].object = obj;
    pkg->connections[i].connected = NULL;
  }
  pkg->element.extra_spacing.border_trans = FUNCTION_FONTHEIGHT / FUNCTION_BORDERWIDTH_SCALE/2.0;
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

  newpkg->element.extra_spacing.border_trans = pkg->element.extra_spacing.border_trans ;

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
  pkg->element.extra_spacing.border_trans = pkg->text ? pkg->text->height : FUNCTION_FONTHEIGHT / FUNCTION_BORDERWIDTH_SCALE/2.0;
  function_update_data(pkg);

  for (i=0;i<8;i++) {
    obj->handles[i]->type = HANDLE_NON_MOVABLE;
  }

  return &pkg->element.object;
}

static ObjectChange *
function_insert_word( Function* func, const char* word, gboolean newline )
{
  ObjectChange* change = function_create_change( func, TEXT_EDIT ) ;
  char* old_chars = text_get_string_copy( func->text ) ;
  char* new_chars = g_malloc( strlen( old_chars) + strlen( word )
		  	+ ( newline ? 2 : 1) ) ;
  sprintf( new_chars, newline ? "%s\n%s" : "%s%s", old_chars, word ) ;
  text_set_string( func->text, new_chars ) ;
  free( new_chars ) ;
  free( old_chars ) ;
  function_update_data( func ) ;
  text_set_cursor_at_end( func->text ) ;

  return change;
}

static ObjectChange *
function_insert_verb( Object* obj, Point* clicked, gpointer data)
{
  return function_insert_word( (Function*)obj, (const char*) data, FALSE ) ;
}

static ObjectChange *
function_insert_noun( Object* obj, Point* clicked, gpointer data)
{
  return function_insert_word( (Function*)obj, (const char*) data, TRUE ) ;
}

static ObjectChange *
function_toggle_user_function( Object* obj, Point* clicked, gpointer data)
{
  Function* func = (Function*)obj ;
  ObjectChange* change = function_create_change( func, USER_FUNC ) ;
  func->is_user = !func->is_user ;
  function_update_data( func ) ;

  return change;
}

static ObjectChange *
function_toggle_wish_function( Object* obj, Point* clicked, gpointer data)
{
  Function* func = (Function*)obj ;
  ObjectChange* change = function_create_change( func, WISH_FUNC ) ;
  func->is_wish = !func->is_wish ;
  function_update_data( func ) ;

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

struct _IndentedMenus fmenu[] = {
  {N_("Verb"),			0, NULL },
   {N_("Channel"),		1, NULL },
    {N_("Channel"),		2, function_insert_verb },
    {N_("Import"),		2, NULL },
     {N_("Import"),		3, function_insert_verb },
     {N_("Input"),		3, function_insert_verb },
     {N_("Receive"),		3, function_insert_verb },
     {N_("Allow"),		3, function_insert_verb },
     {N_("Form Entrance"),	3, function_insert_verb },
     {N_("Capture"),		3, function_insert_verb },
    {N_("Export"),		2, NULL },
     {N_("Export"),		3, function_insert_verb },
     {N_("Discharge"),		3, function_insert_verb },
     {N_("Eject"),		3, function_insert_verb },
     {N_("Dispose"),		3, function_insert_verb },
     {N_("Remove"),		3, function_insert_verb },
    {N_("Transfer"),		2, NULL },
     {N_("Transfer"),		3, function_insert_verb },
     {N_("Transport"),		3, NULL },
      {N_("Transport"),		4, function_insert_verb },
      {N_("Lift"),		4, function_insert_verb },
      {N_("Move"),		4, function_insert_verb },
      {N_("Channel"),		4, function_insert_verb },
     {N_("Transmit"),		3, NULL },
      {N_("Transmit"),		4, function_insert_verb },
      {N_("Conduct"),		4, function_insert_verb },
      {N_("Transfer"),		4, function_insert_verb },
      {N_("Convey"),		4, function_insert_verb },
    {N_("Guide"),		2, NULL },
     {N_("Guide"),		3, NULL },
      {N_("Guide"),		4, function_insert_verb },
      {N_("Direct"),		4, function_insert_verb },
      {N_("Straighten"),	4, function_insert_verb },
      {N_("Steer"),		4, function_insert_verb },
     {N_("Translate"),		3, function_insert_verb },
     {N_("Rotate"),		3, NULL },
      {N_("Rotate"),		4, function_insert_verb },
      {N_("Turn"),		4, function_insert_verb },
      {N_("Spin"),		4, function_insert_verb },
     {N_("Allow DOF"),		3, NULL },
      {N_("Allow DOF"),		4, function_insert_verb },
      {N_("Constrain"),		4, function_insert_verb },
      {N_("Unlock"),		4, function_insert_verb },
   {N_("Support"),		1, NULL },
    {N_("Support"),		2, function_insert_verb },
    {N_("Stop"),		2, NULL },
     {N_("Stop"),		3, function_insert_verb },
     {N_("Insulate"),		3, function_insert_verb },
     {N_("Protect"),		3, function_insert_verb },
     {N_("Prevent"),		3, function_insert_verb },
     {N_("Shield"),		3, function_insert_verb },
     {N_("Inhibit"),		3, function_insert_verb },
    {N_("Stabilize"),		2, NULL },
     {N_("Stabilize"),		3, function_insert_verb },
     {N_("Steady"),		3, function_insert_verb },
    {N_("Secure"),		2, NULL },
     {N_("Secure"),		3, function_insert_verb },
     {N_("Attach"),		3, function_insert_verb },
     {N_("Mount"),		3, function_insert_verb },
     {N_("Lock"),		3, function_insert_verb },
     {N_("Fasten"),		3, function_insert_verb },
     {N_("Hold"),		3, function_insert_verb },
    {N_("Position"),		2, NULL },
     {N_("Position"),		3, function_insert_verb },
     {N_("Orient"),		3, function_insert_verb },
     {N_("Align"),		3, function_insert_verb },
     {N_("Locate"),		3, function_insert_verb },
   {N_("Connect"),		1, NULL },
    {N_("Connect"),		2, function_insert_verb },
    {N_("Couple"),		2, NULL },
     {N_("Couple"),		3, function_insert_verb },
     {N_("Join"),		3, function_insert_verb },
     {N_("Assemble"),		3, function_insert_verb },
     {N_("Attach"),		3, function_insert_verb },
    {N_("Mix"),			2, NULL },
     {N_("Mix"),		3, function_insert_verb },
     {N_("Combine"),		3, function_insert_verb },
     {N_("Blend"),		3, function_insert_verb },
     {N_("Add"),		3, function_insert_verb },
     {N_("Pack"),		3, function_insert_verb },
     {N_("Coalesce"),		3, function_insert_verb },
   {N_("Branch"),		1, NULL },
    {N_("Branch"),		2, function_insert_verb },
    {N_("Separate"),		2, NULL },
     {N_("Separate"),		3, NULL },
      {N_("Separate"),		4, function_insert_verb },
      {N_("Switch"),		4, function_insert_verb },
      {N_("Divide"),		4, function_insert_verb },
      {N_("Release"),		4, function_insert_verb },
      {N_("Detach"),		4, function_insert_verb },
      {N_("Disconnect"),	4, function_insert_verb },
     {N_("Remove"),		3, NULL },
      {N_("Remove"),		4, function_insert_verb },
      {N_("Cut"),		4, function_insert_verb },
      {N_("Polish"),		4, function_insert_verb },
      {N_("Sand"),		4, function_insert_verb },
      {N_("Drill"),		4, function_insert_verb },
      {N_("Lathe"),		4, function_insert_verb },
    {N_("Refine"),		2, NULL },
     {N_("Refine"),		3, function_insert_verb },
     {N_("Purify"),		3, function_insert_verb },
     {N_("Strain"),		3, function_insert_verb },
     {N_("Filter"),		3, function_insert_verb },
     {N_("Percolate"),		3, function_insert_verb },
     {N_("Clear"),		3, function_insert_verb },
    {N_("Distribute"),		2, NULL },
     {N_("Distribute"),		3, function_insert_verb },
     {N_("Diverge"),		3, function_insert_verb },
     {N_("Scatter"),		3, function_insert_verb },
     {N_("Disperse"),		3, function_insert_verb },
     {N_("Diffuse"),		3, function_insert_verb },
     {N_("Empty"),		3, function_insert_verb },
    {N_("Dissipate"),		2, NULL },
     {N_("Dissipate"),		3, function_insert_verb },
     {N_("Absorb"),		3, function_insert_verb },
     {N_("Dampen"),		3, function_insert_verb },
     {N_("Dispel"),		3, function_insert_verb },
     {N_("Diffuse"),		3, function_insert_verb },
     {N_("Resist"),		3, function_insert_verb },
   {N_("Provision"),		1, NULL },
    {N_("Provision"),		2, function_insert_verb },
    {N_("Store"),		2, NULL },
     {N_("Store"),		3, function_insert_verb },
     {N_("Contain"),		3, function_insert_verb },
     {N_("Collect"),		3, function_insert_verb },
     {N_("Reserve"),		3, function_insert_verb },
     {N_("Capture"),		3, function_insert_verb },
    {N_("Supply"),		2, NULL },
     {N_("Supply"),		3, function_insert_verb },
     {N_("Fill"),		3, function_insert_verb },
     {N_("Provide"),		3, function_insert_verb },
     {N_("Replenish"),		3, function_insert_verb },
     {N_("Expose"),		3, function_insert_verb },
    {N_("Extract"),		2, function_insert_verb },
   {N_("Control Magnitude"),	1, NULL },
    {N_("Control Magnitude"),	2, function_insert_verb },
    {N_("Actuate"),		2, NULL },
     {N_("Actuate"),		3, function_insert_verb },
     {N_("Start"),		3, function_insert_verb },
     {N_("Initiate"),		3, function_insert_verb },
    {N_("Regulate"),		2, NULL },
     {N_("Regulate"),		3, function_insert_verb },
     {N_("Control"),		3, function_insert_verb },
     {N_("Allow"),		3, function_insert_verb },
     {N_("Prevent"),		3, function_insert_verb },
     {N_("Enable"),		3, function_insert_verb },
     {N_("Disable"),		3, function_insert_verb },
     {N_("Limit"),		3, function_insert_verb },
     {N_("Interrupt"),		3, function_insert_verb },
    {N_("Change"),		2, NULL },
     {N_("Change"),		3, function_insert_verb },
     {N_("Increase"),		3, function_insert_verb },
     {N_("Decrease"),		3, function_insert_verb },
     {N_("Amplify"),		3, function_insert_verb },
     {N_("Reduce"),		3, function_insert_verb },
     {N_("Magnify"),		3, function_insert_verb },
     {N_("Normalize"),		3, function_insert_verb },
     {N_("Multiply"),		3, function_insert_verb },
     {N_("Scale"),		3, function_insert_verb },
     {N_("Rectify"),		3, function_insert_verb },
     {N_("Adjust"),		3, function_insert_verb },
    {N_("Form"),		2, NULL },
     {N_("Form"),		3, function_insert_verb },
     {N_("Compact"),		3, function_insert_verb },
     {N_("Crush"),		3, function_insert_verb },
     {N_("Shape"),		3, function_insert_verb },
     {N_("Compress"),		3, function_insert_verb },
     {N_("Pierce"),		3, function_insert_verb },
   {N_("Convert"),		1, NULL },
    {N_("Convert"),		2, function_insert_verb },
    {N_("Transform"),		2, function_insert_verb },
    {N_("Liquefy"),		2, function_insert_verb },
    {N_("Solidify"),		2, function_insert_verb },
    {N_("Evaporate"),		2, function_insert_verb },
    {N_("Sublimate"),		2, function_insert_verb },
    {N_("Condense"),		2, function_insert_verb },
    {N_("Integrate"),		2, function_insert_verb },
    {N_("Differentiate"),	2, function_insert_verb },
    {N_("Process"),		2, function_insert_verb },
   {N_("Signal"),		1, NULL },
    {N_("Signal"),		2, function_insert_verb },
    {N_("Sense"),		2, NULL },
     {N_("Sense"),		3, function_insert_verb },
     {N_("Perceive"),		3, function_insert_verb },
     {N_("Recognize"),		3, function_insert_verb },
     {N_("Discern"),		3, function_insert_verb },
     {N_("Check"),		3, function_insert_verb },
     {N_("Locate"),		3, function_insert_verb },
     {N_("Verify"),		3, function_insert_verb },
    {N_("Indicate"),		2, NULL },
     {N_("Indicate"),		3, function_insert_verb },
     {N_("Mark"),		3, function_insert_verb },
    {N_("Display"),		2, function_insert_verb },
    {N_("Measure"),		2, NULL },
     {N_("Measure"),		3, function_insert_verb },
     {N_("Calculate"),		3, function_insert_verb },
    {N_("Represent"),		2, function_insert_verb },
  {N_("Noun"),			0, NULL },
    {N_("Material"),		1, NULL },
     {N_("Solid"),		2, function_insert_noun },
     {N_("Liquid"),		2, function_insert_noun },
     {N_("Gas"),		2, function_insert_noun },
     {N_("Human"),		2, NULL },
      {N_("Human"),		3, function_insert_noun },
      {N_("Hand"),		3, function_insert_noun },
      {N_("Foot"),		3, function_insert_noun },
      {N_("Head"),		3, function_insert_noun },
      {N_("Finger"),		3, function_insert_noun },
      {N_("Toe"),		3, function_insert_noun },
     {N_("Biological"),		2, function_insert_noun },
    {N_("Energy"),		1, NULL },
     {N_("Mechanical"),		2, NULL },
      {N_("Mech. Energy"),	3, function_insert_noun },
      {N_("Translation"),	3, function_insert_noun },
      {N_("Force"),		3, function_insert_noun },
      {N_("Rotation"),		3, function_insert_noun },
      {N_("Torque"),		3, function_insert_noun },
      {N_("Random Motion"),	3, function_insert_noun },
      {N_("Vibration"),		3, function_insert_noun },
      {N_("Rotational Energy"),	3, function_insert_noun },
      {N_("Translational Energy"),3, function_insert_noun },
     {N_("Electrical"),		2, NULL },
      {N_("Electricity"),	3, function_insert_noun },
      {N_("Voltage"),		3, function_insert_noun },
      {N_("Current"),		3, function_insert_noun },
     {N_("Hydraulic"),		2, function_insert_noun },
      {N_("Pressure"),		3, function_insert_noun },
      {N_("Volumetric Flow"),	3, function_insert_noun },
     {N_("Thermal"),		2, NULL },
      {N_("Heat"),		3, function_insert_noun },
      {N_("Conduction"),	3, function_insert_noun },
      {N_("Convection"),	3, function_insert_noun },
      {N_("Radiation"),		3, function_insert_noun },
     {N_("Pneumatic"),		2, function_insert_noun },
     {N_("Chemical"),		2, function_insert_noun },
     {N_("Radioactive"),	2, NULL },
      {N_("Radiation"),		3, function_insert_noun },
      {N_("Microwaves"),	3, function_insert_noun },
      {N_("Radio waves"),	3, function_insert_noun },
      {N_("X-Rays"),		3, function_insert_noun },
      {N_("Gamma Rays"),	3, function_insert_noun },
     {N_("Acoustic Energy"),	2, function_insert_noun },
     {N_("Optical Energy"),	2, function_insert_noun },
     {N_("Solar Energy"),	2, function_insert_noun },
     {N_("Magnetic Energy"),	2, function_insert_noun },
     {N_("Human"),		2, NULL },
      {N_("Human Motion"),	3, function_insert_noun },
      {N_("Human Force"),	3, function_insert_noun },
    {N_("Signal"),		1, NULL },
     {N_("Signal"),		2, function_insert_noun },
     {N_("Status"),		2, function_insert_noun },
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

static DiaMenu*
function_get_object_menu( Function* func, Point* clickedpoint )
{
  if ( ! function_menu ) {
  int i ;
    int curDepth = 0 ;
    DiaMenu* curMenu[ FS_SUBMENU_MAXINDENT ] ;
    int curitem[ FS_SUBMENU_MAXINDENT ] ;

    curitem[0] = 0 ;
    curMenu[0] = malloc( sizeof( DiaMenu ) ) ;
    curMenu[0]->title = "Function" ;
    curMenu[0]->num_items = function_count_submenu_items( &(fmenu[0]) ) ;
    curMenu[0]->items = malloc( curMenu[0]->num_items * sizeof(DiaMenuItem) );
    curMenu[0]->app_data = NULL ;
    for (i = 0 ; fmenu[i].depth >= 0; i++) {
      if ( fmenu[i].depth > curDepth ) {
	curDepth++ ;
	curMenu[curDepth] = malloc( sizeof( DiaMenu ) ) ;
	curMenu[curDepth]->title = NULL ;
	curMenu[curDepth]->app_data = NULL ;
	curMenu[curDepth]->num_items = function_count_submenu_items(&fmenu[i]);
	curMenu[curDepth]->items = malloc( curMenu[curDepth]->num_items *
				  sizeof(DiaMenuItem) ) ;
	/* Point this menu's parent to this new structure */
	curMenu[curDepth-1]->items[curitem[curDepth-1]-1].callback = NULL ;
	curMenu[curDepth-1]->items[curitem[curDepth-1]-1].callback_data =
		curMenu[curDepth] ;
	curitem[ curDepth ] = 0 ;
      } else if ( fmenu[i].depth < curDepth ) {
	curDepth=fmenu[i].depth ;
    }

      curMenu[curDepth]->items[curitem[curDepth]].text = fmenu[i].name ;
      curMenu[curDepth]->items[curitem[curDepth]].callback = fmenu[i].func ;
      curMenu[curDepth]->items[curitem[curDepth]].callback_data = fmenu[i].name;
      curMenu[curDepth]->items[curitem[curDepth]].active = 1 ;
      curitem[curDepth]++ ;
  }
    function_menu = curMenu[0] ;
  }
  return function_menu ;
}


