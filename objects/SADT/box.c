/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * SADT Activity/Data box -- objects for drawing SADT diagrams.
 * Copyright (C) 2000 Cyrille Chepelov
 * 
 * Forked from Flowchart toolbox -- objects for drawing flowcharts.
 * Copyright (C) 1999 James Henstridge.
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
#include <glib.h>

#include "intl.h"
#include "object.h"
#include "element.h"
#include "connectionpoint.h"
#include "render.h"
#include "attributes.h"
#include "text.h"
#include "widgets.h"
#include "message.h"
#include "connpoint_line.h"
#include "color.h"
#include "lazyprops.h"

#include "pixmaps/sadtbox.xpm"

#define DEFAULT_WIDTH 7.0
#define DEFAULT_HEIGHT 5.0
#define DEFAULT_BORDER 0.25
#define SADTBOX_LINE_WIDTH 0.10
#define SADTBOX_FG_COLOR color_black
#define SADTBOX_BG_COLOR color_white

typedef enum {
  ANCHOR_MIDDLE,
  ANCHOR_START,
  ANCHOR_END
} AnchorShape;

typedef struct _Box Box;
typedef struct _BoxPropertiesDialog BoxPropertiesDialog;
typedef struct _BoxDefaultsDialog BoxDefaultsDialog;
typedef struct _BoxState BoxState;
typedef struct _BoxDefaults BoxDefaults;

struct _BoxState {
  ObjectState obj_state;

  real padding;
  TextAttributes text_attrib;
  gchar *id;
};

struct _Box {
  Element element;

  ConnPointLine *north,*south,*east,*west;

  Text *text;
  gchar *id;
  real padding;
};

struct _BoxPropertiesDialog {
  AttributeDialog dialog;
  Box *parent;

  RealAttribute padding;
  TextFontAttribute text_font;
  TextFontHeightAttribute text_fontheight;
  TextColorAttribute text_color;
  StringAttribute id;
};

struct _BoxDefaults {
  Font *font;
  real font_size;
  Color fontcolor;
  real padding;
};

struct _BoxDefaultsDialog {
  AttributeDialog dialog;
  BoxDefaults *parent;

  RealAttribute padding;
  FontAttribute font;
  FontHeightAttribute font_size;
  ColorAttribute fontcolor;
};


static BoxPropertiesDialog *sadtbox_properties_dialog;
static BoxDefaultsDialog *sadtbox_defaults_dialog;
static BoxDefaults default_properties;

static real sadtbox_distance_from(Box *box, Point *point);
static void sadtbox_select(Box *box, Point *clicked_point,
		       Renderer *interactive_renderer);
static void sadtbox_move_handle(Box *box, Handle *handle,
			    Point *to, HandleMoveReason reason, 
			    ModifierKeys modifiers);
static void sadtbox_move(Box *box, Point *to);
static void sadtbox_draw(Box *box, Renderer *renderer);
static void sadtbox_update_data(Box *box, AnchorShape horix, AnchorShape vert);
static Object *sadtbox_create(Point *startpoint,
			  void *user_data,
			  Handle **handle1,
			  Handle **handle2);
static void sadtbox_destroy(Box *box);
static Object *sadtbox_copy(Box *box);
static PROPDLG_TYPE sadtbox_get_properties(Box *box);
static ObjectChange *sadtbox_apply_properties(Box *box);

static BoxState *sadtbox_get_state(Box *box);
static void sadtbox_set_state(Box *box, BoxState *state);

static void sadtbox_save(Box *box, ObjectNode obj_node, const char *filename);
static Object *sadtbox_load(ObjectNode obj_node, int version, const char *filename);
static PROPDLG_TYPE sadtbox_get_defaults(void);
static void sadtbox_apply_defaults(void);

static DiaMenu *sadtbox_get_object_menu(Box *box, Point *clickedpoint);

static ObjectTypeOps sadtbox_type_ops =
{
  (CreateFunc) sadtbox_create,
  (LoadFunc)   sadtbox_load,
  (SaveFunc)   sadtbox_save,
  (GetDefaultsFunc)   sadtbox_get_defaults,
  (ApplyDefaultsFunc) sadtbox_apply_defaults
};

ObjectType sadtbox_type =
{
  "SADT - box",  /* name */
  0,                 /* version */
  (char **) sadtbox_xpm, /* pixmap */

  &sadtbox_type_ops      /* ops */
};

static ObjectOps sadtbox_ops = {
  (DestroyFunc)         sadtbox_destroy,
  (DrawFunc)            sadtbox_draw,
  (DistanceFunc)        sadtbox_distance_from,
  (SelectFunc)          sadtbox_select,
  (CopyFunc)            sadtbox_copy,
  (MoveFunc)            sadtbox_move,
  (MoveHandleFunc)      sadtbox_move_handle,
  (GetPropertiesFunc)   sadtbox_get_properties,
  (ApplyPropertiesFunc) sadtbox_apply_properties,
  (ObjectMenuFunc)      sadtbox_get_object_menu
};

static ObjectChange *
sadtbox_apply_properties(Box *box)
{
  ObjectState *old_state;
  BoxPropertiesDialog *dlg = sadtbox_properties_dialog;

  PROPDLG_SANITY_CHECK(dlg,box);
  
  old_state = (ObjectState *)sadtbox_get_state(box);

  PROPDLG_APPLY_REAL(dlg, padding);
  PROPDLG_APPLY_TEXTFONT(dlg,text);
  PROPDLG_APPLY_TEXTFONTHEIGHT(dlg,text);
  PROPDLG_APPLY_TEXTCOLOR(dlg,text);
  PROPDLG_APPLY_STRING(dlg,id);

  sadtbox_update_data(box, ANCHOR_MIDDLE, ANCHOR_MIDDLE);
  return new_object_state_change(&box->element.object, 
                                 old_state, 
				 (GetStateFunc)sadtbox_get_state,
				 (SetStateFunc)sadtbox_set_state);
}

static PROPDLG_TYPE
sadtbox_get_properties(Box *box)
{
  BoxPropertiesDialog *dlg = sadtbox_properties_dialog;
  
  PROPDLG_CREATE(dlg,box);
  
  PROPDLG_SHOW_REAL(dlg,padding,_("Text padding:"),0.0,10.0,0.1);
  PROPDLG_SHOW_TEXTFONT(dlg,text,_("Font:"));
  PROPDLG_SHOW_TEXTFONTHEIGHT(dlg,text,_("Font size:"));
  PROPDLG_SHOW_TEXTCOLOR(dlg,text,_("Font color:"));
  PROPDLG_SHOW_STRING(dlg,id,_("Activity/Data identifier"));
  
  PROPDLG_READY(dlg);
  sadtbox_properties_dialog = dlg;

  PROPDLG_RETURN(dlg);
}
static void
sadtbox_apply_defaults(void)
{
  BoxDefaultsDialog *dlg = sadtbox_defaults_dialog;

  PROPDLG_APPLY_REAL(dlg,padding);
  PROPDLG_APPLY_FONT(dlg,font);
  PROPDLG_APPLY_FONTHEIGHT(dlg,font_size);
  PROPDLG_APPLY_COLOR(dlg,fontcolor);
}

static void
init_default_values(void) {
  static int defaults_initialized = 0;

  if (!defaults_initialized) {
    default_properties.padding = 0.5;
    default_properties.font = font_getfont("Helvetica-Bold");
    default_properties.font_size = 0.8;
    default_properties.fontcolor = color_black;
    defaults_initialized = 1;
  }
}

static GtkWidget *
sadtbox_get_defaults(void)
{
  BoxDefaultsDialog *dlg = sadtbox_defaults_dialog;

  init_default_values();
  PROPDLG_CREATE(dlg, &default_properties);
  PROPDLG_SHOW_REAL(dlg,padding,_("Text padding:"),0.0,10.0,0.1);
  PROPDLG_SHOW_FONT(dlg,font,_("Font:"));
  PROPDLG_SHOW_FONTHEIGHT(dlg,font_size,_("Font size:"));
  PROPDLG_SHOW_COLOR(dlg,fontcolor,_("Font color:"));
  PROPDLG_READY(dlg);
  sadtbox_defaults_dialog = dlg;

  PROPDLG_RETURN(dlg);
}

static real
sadtbox_distance_from(Box *box, Point *point)
{
  Element *elem = &box->element;
  Rectangle rect;

  rect.left = elem->corner.x - SADTBOX_LINE_WIDTH/2;
  rect.right = elem->corner.x + elem->width + SADTBOX_LINE_WIDTH/2;
  rect.top = elem->corner.y - SADTBOX_LINE_WIDTH/2;
  rect.bottom = elem->corner.y + elem->height + SADTBOX_LINE_WIDTH/2;
  return distance_rectangle_point(&rect, point);
}

static void
sadtbox_select(Box *box, Point *clicked_point,
	   Renderer *interactive_renderer)
{
  text_set_cursor(box->text, clicked_point, interactive_renderer);
  text_grab_focus(box->text, &box->element.object);
  element_update_handles(&box->element);
}

static void
sadtbox_move_handle(Box *box, Handle *handle,
		Point *to, HandleMoveReason reason, ModifierKeys modifiers)
{
  AnchorShape horiz = ANCHOR_MIDDLE, vert = ANCHOR_MIDDLE;

  assert(box!=NULL);
  assert(handle!=NULL);
  assert(to!=NULL);

  element_move_handle(&box->element, handle->id, to, reason);

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
  default:
    break;
  }
  sadtbox_update_data(box, horiz, vert);
}

static void
sadtbox_move(Box *box, Point *to)
{
  box->element.corner = *to;
  
  sadtbox_update_data(box, ANCHOR_MIDDLE, ANCHOR_MIDDLE);
}

static void
sadtbox_draw(Box *box, Renderer *renderer)
{
  Point lr_corner,pos;
  Element *elem;
  real idfontheight;

  assert(box != NULL);
  assert(renderer != NULL);

  elem = &box->element;

  lr_corner.x = elem->corner.x + elem->width;
  lr_corner.y = elem->corner.y + elem->height;

  renderer->ops->set_fillstyle(renderer, FILLSTYLE_SOLID);
  renderer->ops->fill_rect(renderer, 
			   &elem->corner,
			   &lr_corner, 
			   &SADTBOX_BG_COLOR);


  renderer->ops->set_linewidth(renderer, SADTBOX_LINE_WIDTH);
  renderer->ops->set_linestyle(renderer, LINESTYLE_SOLID);
  renderer->ops->set_linejoin(renderer, LINEJOIN_MITER);

  renderer->ops->draw_rect(renderer, 
			   &elem->corner,
			   &lr_corner, 
			   &SADTBOX_FG_COLOR);


  text_draw(box->text, renderer);

  idfontheight = .75 * box->text->height;
  renderer->ops->set_font(renderer, box->text->font, idfontheight);
  pos = lr_corner;
  pos.x -= .3 * idfontheight;
  pos.y -= .3 * idfontheight;
  renderer->ops->draw_string(renderer,
			     box->id,
			     &pos, ALIGN_RIGHT,
			     &box->text->color);
}

static void
sadtbox_free_state(ObjectState *objstate)
{
  BoxState *state = (BoxState *)objstate;
  g_free(state->id);
  /*  g_free(state); */ /* huh ?? NOT HERE !!! */
}

static BoxState *
sadtbox_get_state(Box *box)
{
  BoxState *state = g_new(BoxState, 1);

  state->obj_state.free = sadtbox_free_state;

  state->id = g_strdup(box->id);
  state->padding = box->padding;
  text_get_attributes(box->text, &state->text_attrib);

  return state;
}

static void
sadtbox_set_state(Box *box, BoxState *state)
{
  box->padding = state->padding;
  text_set_attributes(box->text, &state->text_attrib);
  if (box->id) g_free(box->id);
  box->id = state->id;
  g_free(state);
  
  sadtbox_update_data(box, ANCHOR_MIDDLE, ANCHOR_MIDDLE);
}


static void
sadtbox_update_data(Box *box, AnchorShape horiz, AnchorShape vert)
{
  Element *elem = &box->element;
  ElementBBExtras *extra = &elem->extra_spacing;
  Object *obj = &elem->object;
  Point center, bottom_right;
  Point p;
  real width, height;
  Point nw,ne,se,sw;

  /* save starting points */
  center = bottom_right = elem->corner;
  center.x += elem->width/2;
  bottom_right.x += elem->width;
  center.y += elem->height/2;
  bottom_right.y += elem->height;

  width = box->text->max_width + box->padding*2;
  height = box->text->height * box->text->numlines + box->padding*2;

  if (width > elem->width) elem->width = width;
  if (height > elem->height) elem->height = height;

  /* move shape if necessary ... */
  switch (horiz) {
  case ANCHOR_MIDDLE:
    elem->corner.x = center.x - elem->width/2; break;
  case ANCHOR_END:
    elem->corner.x = bottom_right.x - elem->width; break;
  default:
    break;
  }
  switch (vert) {
  case ANCHOR_MIDDLE:
    elem->corner.y = center.y - elem->height/2; break;
  case ANCHOR_END:
    elem->corner.y = bottom_right.y - elem->height; break;
  default:
    break;
  }

  p = elem->corner;
  p.x += elem->width / 2.0;
  p.y += elem->height / 2.0 - box->text->height * box->text->numlines / 2 +
    font_ascent(box->text->font, box->text->height);
  text_set_position(box->text, &p);

  extra->border_trans = SADTBOX_LINE_WIDTH / 2.0;
  element_update_boundingbox(elem);
  
  obj->position = elem->corner;
  
  element_update_handles(elem);

  /* Update connections: */
  nw = elem->corner;
  se = bottom_right;
  ne.x = se.x;
  ne.y = nw.y;
  sw.y = se.y;
  sw.x = nw.x;
  
  connpointline_update(box->north);
  connpointline_putonaline(box->north,&ne,&nw);
  connpointline_update(box->west);
  connpointline_putonaline(box->west,&nw,&sw);
  connpointline_update(box->south);
  connpointline_putonaline(box->south,&sw,&se);
  connpointline_update(box->east);
  connpointline_putonaline(box->east,&se,&ne);
}


static ConnPointLine *
sadtbox_get_clicked_border(Box *box, Point *clicked)
{
  ConnPointLine *cpl;
  real dist,dist2;

  cpl = box->north;
  dist = distance_line_point(&box->north->start,&box->north->end,0,clicked);

  dist2 = distance_line_point(&box->west->start,&box->west->end,0,clicked);
  if (dist2 < dist) {
    cpl = box->west;
    dist = dist2;
  }
  dist2 = distance_line_point(&box->south->start,&box->south->end,0,clicked);
  if (dist2 < dist) {
    cpl = box->south;
    dist = dist2;
  }
  dist2 = distance_line_point(&box->east->start,&box->east->end,0,clicked);
  if (dist2 < dist) {
    cpl = box->east;
    /*dist = dist2;*/
  }
  return cpl;
}

inline static ObjectChange *
sadtbox_create_change(Box *box, ObjectChange *inner, ConnPointLine *cpl) {
  return (ObjectChange *)inner;
}

static ObjectChange *
sadtbox_add_connpoint_callback(Object *obj, Point *clicked, gpointer data) 
{
  ObjectChange *change;
  ConnPointLine *cpl;
  Box *box = (Box *)obj;

  cpl = sadtbox_get_clicked_border(box,clicked);
  change = connpointline_add_point(cpl, clicked);
  sadtbox_update_data((Box *)obj,ANCHOR_MIDDLE, ANCHOR_MIDDLE);
  return sadtbox_create_change(box,change,cpl);
}

static ObjectChange *
sadtbox_remove_connpoint_callback(Object *obj, Point *clicked, gpointer data) 
{
  ObjectChange *change;
  ConnPointLine *cpl;
  Box *box = (Box *)obj;

  cpl = sadtbox_get_clicked_border(box,clicked);
  change = connpointline_remove_point(cpl, clicked);
  sadtbox_update_data((Box *)obj,ANCHOR_MIDDLE, ANCHOR_MIDDLE);
  return sadtbox_create_change(box,change,cpl);
}

static DiaMenuItem object_menu_items[] = {
  { N_("Add connection point"), sadtbox_add_connpoint_callback, NULL, 1 },
  { N_("Delete connection point"), sadtbox_remove_connpoint_callback, 
    NULL, 1 },
};

static DiaMenu object_menu = {
  N_("SADT box"),
  sizeof(object_menu_items)/sizeof(DiaMenuItem),
  object_menu_items,
  NULL
};

static DiaMenu *
sadtbox_get_object_menu(Box *box, Point *clickedpoint)
{
  ConnPointLine *cpl;

  cpl = sadtbox_get_clicked_border(box,clickedpoint);
  /* Set entries sensitive/selected etc here */
  object_menu_items[0].active = connpointline_can_add_point(cpl, clickedpoint);
  object_menu_items[1].active = connpointline_can_remove_point(cpl, clickedpoint);
  return &object_menu;
}


static Object *
sadtbox_create(Point *startpoint,
	   void *user_data,
	   Handle **handle1,
	   Handle **handle2)
{
  Box *box;
  Element *elem;
  Object *obj;
  Point p;

  init_default_values();

  box = g_malloc0(sizeof(Box));
  elem = &box->element;
  obj = &elem->object;
  
  obj->type = &sadtbox_type;

  obj->ops = &sadtbox_ops;

  elem->corner = *startpoint;
  elem->width = DEFAULT_WIDTH;
  elem->height = DEFAULT_HEIGHT;

  box->padding = default_properties.padding;
  
  p = *startpoint;
  p.x += elem->width / 2.0;
  p.y += elem->height / 2.0 + default_properties.font_size / 2;
  box->text = new_text("", default_properties.font,
		       default_properties.font_size, &p, 
		       &SADTBOX_FG_COLOR,
		       ALIGN_CENTER);
  box->id = g_strdup("A0"); /* should be made better. 
				   Automatic counting ? */

  element_init(elem, 8, 0);

  box->north = connpointline_create(obj,4);
  box->west = connpointline_create(obj,3); 
  box->south = connpointline_create(obj,1); 
  box->east = connpointline_create(obj,3);

  box->element.extra_spacing.border_trans = SADTBOX_LINE_WIDTH/2.0;
  sadtbox_update_data(box, ANCHOR_MIDDLE, ANCHOR_MIDDLE);

  *handle1 = NULL;
  *handle2 = obj->handles[7];  
  return &box->element.object;
}

static void
sadtbox_destroy(Box *box)
{
  text_destroy(box->text);
  
  connpointline_destroy(box->east);
  connpointline_destroy(box->south);
  connpointline_destroy(box->west);
  connpointline_destroy(box->north);
  
  g_free(box->id);

  element_destroy(&box->element);
}

static Object *
sadtbox_copy(Box *box)
{
  Box *newbox;
  Element *elem, *newelem;
  Object *newobj;
  int rcc;

  elem = &box->element;
  
  newbox = g_malloc(sizeof(Box));
  newelem = &newbox->element;
  newobj = &newelem->object;

  element_copy(elem, newelem);
  rcc = 0;
  newbox->north = connpointline_copy(newobj,box->north,&rcc);
  newbox->west = connpointline_copy(newobj,box->west,&rcc);
  newbox->south = connpointline_copy(newobj,box->south,&rcc);
  newbox->east = connpointline_copy(newobj,box->east,&rcc);
  g_assert(rcc == newobj->num_connections);
  
  newbox->id = g_strdup(box->id);
  newbox->padding = box->padding;
  newbox->text = text_copy(box->text);  

  return &newbox->element.object;
}


static void
sadtbox_save(Box *box, ObjectNode obj_node, const char *filename)
{
  element_save(&box->element, obj_node);

  save_real(obj_node,"padding",box->padding);
  save_text(obj_node,"text",box->text);
  save_int(obj_node,"cpl_north",box->north->num_connections);
  save_int(obj_node,"cpl_west",box->west->num_connections);
  save_int(obj_node,"cpl_south",box->south->num_connections);
  save_int(obj_node,"cpl_east",box->east->num_connections);
  save_string(obj_node,"id",box->id);
}

static Object *
sadtbox_load(ObjectNode obj_node, int version, const char *filename)
{
  Box *box;
  Element *elem;
  Object *obj;
  int realconncount;
  Point p = {0.0,0.0};

  box = g_malloc(sizeof(Box));
  elem = &box->element;
  obj = &elem->object;
  
  obj->type = &sadtbox_type;
  obj->ops = &sadtbox_ops;

  element_load(elem, obj_node);

  box->padding = load_real(obj_node,"padding",default_properties.padding);
  box->text = load_text(obj_node,"text",NULL);
  if (!box->text) 
    box->text = new_text("", default_properties.font,
			 default_properties.font_size, &p, 
			 &SADTBOX_FG_COLOR,
			 ALIGN_CENTER);

  element_init(elem, 8, 0);
  realconncount = 0;

  box->north = connpointline_load(obj,obj_node,"cpl_north",4,&realconncount);
  box->west = connpointline_load(obj,obj_node,"cpl_west",3,&realconncount);
  box->south = connpointline_load(obj,obj_node,"cpl_south",1,&realconncount);
  box->east = connpointline_load(obj,obj_node,"cpl_east",3,&realconncount);

  box->id = load_string(obj_node,"id",NULL);
  if (!box->id) box->id = g_strdup("A0");
  
  box->element.extra_spacing.border_trans = SADTBOX_LINE_WIDTH/2.0;
  sadtbox_update_data(box, ANCHOR_MIDDLE, ANCHOR_MIDDLE);

  return &box->element.object;
}


