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
#include <assert.h>
#include <gtk/gtk.h>
#include <math.h>
#include <string.h>

#include "config.h"
#include "intl.h"
#include "render.h"
#include "attributes.h"

#include "process.h"

#include "pixmaps/emlprocess.xpm"

#define EMLPROCESS_BORDER 0.1
#define EMLPROCESS_SECTIONWIDTH 0.05
#define EMLPROCESS_UNDERLINEWIDTH 0.0

static real emlprocess_distance_from(EMLProcess *emlprocess, Point *point);
static void emlprocess_select(EMLProcess *emlprocess, Point *clicked_point,
			    Renderer *interactive_renderer);
static ObjectChange* emlprocess_move_handle(EMLProcess *emlprocess, Handle *handle,
					    Point *to, HandleMoveReason reason, ModifierKeys modifiers);
static ObjectChange* emlprocess_move(EMLProcess *emlprocess, Point *to);
static void emlprocess_draw(EMLProcess *emlprocess, Renderer *renderer);
static DiaObject *emlprocess_create(Point *startpoint,
			       void *user_data,
			       Handle **handle1,
			       Handle **handle2);
static void emlprocess_destroy(EMLProcess *emlprocess);
static DiaObject *emlprocess_copy(EMLProcess *emlprocess);

static void emlprocess_save(EMLProcess *emlprocess, ObjectNode obj_node,
			  const char *filename);
static DiaObject *emlprocess_load(ObjectNode obj_node, int version,
			     const char *filename);

static ObjectTypeOps emlprocess_type_ops =
{
  (CreateFunc) emlprocess_create,
  (LoadFunc)   emlprocess_load,
  (SaveFunc)   emlprocess_save
};

DiaObjectType emlprocess_type =
{
  "EML - Process",           /* name */
  0,                         /* version */
  (char **) emlprocess_xpm,  /* pixmap */
  
  &emlprocess_type_ops       /* ops */
};

static ObjectOps emlprocess_ops = {
  (DestroyFunc)         emlprocess_destroy,
  (DrawFunc)            emlprocess_draw,
  (DistanceFunc)        emlprocess_distance_from,
  (SelectFunc)          emlprocess_select,
  (CopyFunc)            emlprocess_copy,
  (MoveFunc)            emlprocess_move,
  (MoveHandleFunc)      emlprocess_move_handle,
  (GetPropertiesFunc)   emlprocess_get_properties,
  (ApplyPropertiesFunc) emlprocess_apply_properties,
  (ObjectMenuFunc)      NULL
};


static real
emlprocess_distance_from(EMLProcess *emlprocess, Point *point)
{
  DiaObject *obj = &emlprocess->element.object;
  return distance_rectangle_point(&obj->bounding_box, point);
}

static void
emlprocess_select(EMLProcess *emlprocess, Point *clicked_point,
	       Renderer *interactive_renderer)
{
  element_update_handles(&emlprocess->element);
}

static ObjectChange*
emlprocess_move_handle(EMLProcess *emlprocess, Handle *handle,
		     Point *to, HandleMoveReason reason, ModifierKeys modifiers)
{
  assert(emlprocess!=NULL);
  assert(handle!=NULL);
  assert(to!=NULL);

  assert(handle->id < 8);

  return NULL;
}

static ObjectChange*
emlprocess_move(EMLProcess *emlprocess, Point *to)
{
  emlprocess->element.corner = *to;
  emlprocess_calculate_connections(emlprocess);
  emlprocess_update_data(emlprocess);

  return NULL;
}

static void
emlprocess_draw(EMLProcess *emlprocess, Renderer *renderer)
{
  EMLBox *namebox;
  Point point;
  assert(emlprocess != NULL);
  assert(renderer != NULL);

  namebox = emlprocess->name_box;
  g_free(namebox->els->data);
  namebox->els->data = g_strdup(emlprocess->name);
  point.x = namebox->right_connection->pos.x - 0.2;
  point.y = namebox->right_connection->pos.y
      + namebox->font_height / 2.0;

  emlbox_draw(emlprocess->box, renderer, &emlprocess->element);

  renderer->ops->set_font(renderer, namebox->font, namebox->font_height);
  renderer->ops->draw_string(renderer, emlprocess->refname, &point,
                             ALIGN_RIGHT, &color_black);
}

void
emlprocess_calculate_connections(EMLProcess *emlprocess)
{
  real height, width;
  ConnectionPoint *connection;
  real x, y;

  Element *elem = &emlprocess->element;

  g_list_free(emlprocess->box_connections);
  emlprocess->box_connections = NULL;

  connection = &emlprocess->proc_connections[4];
  x = elem->corner.x;
  y = elem->corner.y;

  width = elem->width;
  height = emlbox_calc_connections(emlprocess->box, &elem->corner,
                          &emlprocess->box_connections, width);

  emlprocess->box_connections =
    g_list_prepend(emlprocess->box_connections, connection);
  connection->pos.x = x;
  connection->pos.y = y;
  connection++;

  emlprocess->box_connections =
    g_list_prepend(emlprocess->box_connections, connection);
  connection->pos.x = x + width / 2.0;
  connection->pos.y = y;
  connection++;

  emlprocess->box_connections =
    g_list_prepend(emlprocess->box_connections, connection);
  connection->pos.x = x + width;
  connection->pos.y = y;
  connection++;

  emlprocess->box_connections =
    g_list_prepend(emlprocess->box_connections, connection);
  connection->pos.x = x;
  connection->pos.y = y + height;
  connection++;

  emlprocess->box_connections = g_list_prepend(emlprocess->box_connections, connection);
  connection->pos.x = x + width / 2.0;
  connection->pos.y = y + height;
  connection++;

  emlprocess->box_connections =
    g_list_prepend(emlprocess->box_connections, connection);
  connection->pos.x = x + width;
  connection->pos.y = y + height;
}

void
emlprocess_update_data(EMLProcess *emlprocess)
{
  Element *elem = &emlprocess->element;
  DiaObject *obj = (DiaObject *)emlprocess;

  element_update_boundingbox(elem);
  /* fix boundingemlprocess for line_width: */
  obj->bounding_box.top -= EMLPROCESS_BORDER/2.0;
  obj->bounding_box.left -= EMLPROCESS_BORDER/2.0;
  obj->bounding_box.bottom += EMLPROCESS_BORDER/2.0;
  obj->bounding_box.right += EMLPROCESS_BORDER/2.0;

  obj->position = elem->corner;

  element_update_handles(elem);

}

void
emlprocess_calculate_data(EMLProcess *emlprocess)
{
  emlprocess->element.width = 0;
  emlprocess->element.height = 0;

  emlbox_calc_geometry(emlprocess->box,
                       &emlprocess->element.width,
                       &emlprocess->element.height);

  emlprocess->element.width += 2 * 0.2;
}

static
GList*
parameters_get_relations(GList *params)
{
  EMLParameter *param;
  GList *retlist;
  GList *list;

  retlist = NULL;
  list = params;
  while (list != NULL) {
    param = (EMLParameter *) list->data;
    if (param->type == EML_RELATION)
      retlist = g_list_append(retlist, param);
    
    list = g_list_next(list);
  }

  return retlist;
}

static
EMLBox*
create_startup_box(EMLProcess *emlprocess)
{
  EMLBox *topbox;
  EMLBox *childbox;
  EMLBox *textbox;
  EMLParameter *param;
  gchar *text;
  GList *list;

  topbox = EMLListBox.new(0, NULL,
                          ALIGN_LEFT,
                          EMLPROCESS_BORDER,
                          EMLPROCESS_SECTIONWIDTH,
                          LINESTYLE_SOLID,
                          NULL, NULL);

  textbox = EMLTextBox.new(emlprocess->startupfun_font_height,
                           emlprocess->startupfun_font,
                           ALIGN_LEFT,
                           EMLPROCESS_BORDER,
                           EMLPROCESS_SECTIONWIDTH,
                           LINESTYLE_SOLID,
                           emlprocess->startupfun->left_connection,
                           emlprocess->startupfun->right_connection);

  if (strlen(emlprocess->startupfun->name) == 0
      && strlen(emlprocess->startupfun->module) == 0
      && emlprocess->startupfun->parameters == NULL)
      text = g_strdup(" ");
  else
      text = eml_get_function_string(emlprocess->startupfun);
  emlbox_add_el(textbox, text);
  emlbox_add_el(topbox, textbox);

  /* relations */
  list = parameters_get_relations(emlprocess->startupfun->parameters);
  if (list != NULL) {
    childbox = EMLListBox.new(0, NULL,
                               ALIGN_LEFT,
                               EMLPROCESS_BORDER,
                               EMLPROCESS_UNDERLINEWIDTH,
                               LINESTYLE_SOLID,
                               NULL, NULL);
    emlbox_add_el(topbox, childbox);

    while (list != NULL) {
      param = (EMLParameter*) list->data;
      if (param->type == EML_RELATION) {
        textbox = EMLTextBox.new(emlprocess->startupparams_height,
                                 emlprocess->startupparams_font,
                                 ALIGN_LEFT,
                                 EMLPROCESS_BORDER,
                                 EMLPROCESS_UNDERLINEWIDTH,
                                 LINESTYLE_SOLID,
                                 param->left_connection,
                                 param->right_connection);
        text = eml_get_parameter_string(param);
        emlbox_add_el(textbox, text);
        emlbox_add_el(childbox, textbox);
      
      }
      list = g_list_next(list);
    }
    g_list_free(list);
  }

  return topbox;
}

static
EMLBox*
create_interfaces_box(EMLProcess *emlprocess)
{
  EMLBox *topbox;
  EMLBox *childbox;
  EMLBox *childbox2;
  EMLBox *textbox;
  EMLInterface *iface;
  EMLFunction *fun;
  EMLParameter *param;
  GList *list, *child_list;

  list = emlprocess->interfaces;
  if (list == NULL)
    return NULL;

  topbox = EMLListBox.new(0, NULL,
                          ALIGN_LEFT,
                          EMLPROCESS_BORDER,
                          EMLPROCESS_SECTIONWIDTH,
                          LINESTYLE_SOLID,
                          NULL, NULL);

  while (list != NULL) {
    childbox = EMLListBox.new(0, NULL,
                              ALIGN_LEFT,
                              EMLPROCESS_BORDER,
                              EMLPROCESS_SECTIONWIDTH,
                              LINESTYLE_DOTTED,
                              NULL, NULL);
    emlbox_add_el(topbox, childbox);

    iface = (EMLInterface*) list->data;

    child_list = iface->functions;
    if (child_list != NULL) {
      childbox2 = EMLListBox.new(0, NULL,
                                 ALIGN_LEFT,
                                 EMLPROCESS_BORDER,
                                 EMLPROCESS_UNDERLINEWIDTH,
                                 LINESTYLE_SOLID,
                                 NULL, NULL);
      emlbox_add_el(childbox, childbox2);

      while (child_list != NULL) {
        fun = (EMLFunction*) child_list->data;
        textbox = EMLTextBox.new(emlprocess->interface_font_height,
                                 emlprocess->interface_font,
                                 ALIGN_LEFT,
                                 EMLPROCESS_BORDER,
                                 EMLPROCESS_UNDERLINEWIDTH,
                                 LINESTYLE_SOLID,
                                 fun->left_connection,
                                 fun->right_connection);
        emlbox_add_el(textbox, eml_get_iffunction_string(fun));
        emlbox_add_el(childbox2, textbox);
        child_list = g_list_next(child_list);
      }
    }

    child_list = iface->messages;
    if (child_list != NULL) {
      childbox2 = EMLListBox.new(0, NULL,
                                 ALIGN_LEFT,
                                 EMLPROCESS_BORDER,
                                 EMLPROCESS_UNDERLINEWIDTH,
                                 LINESTYLE_SOLID,
                                 NULL, NULL);
      emlbox_add_el(childbox, childbox2);

      while (child_list != NULL) {
        param = (EMLParameter*) child_list->data;
        textbox = EMLTextBox.new(emlprocess->interface_font_height,
                                 emlprocess->interface_font,
                                 ALIGN_LEFT,
                                 EMLPROCESS_BORDER,
                                 EMLPROCESS_UNDERLINEWIDTH,
                                 LINESTYLE_SOLID,
                                 param->left_connection,
                                 param->right_connection);

        emlbox_add_el(textbox, eml_get_ifmessage_string(param));
        emlbox_add_el(childbox2, textbox);
        child_list = g_list_next(child_list);
      }
    }
    list = g_list_next(list);
  }

  return topbox;
}

static
void
box_add_separator(EMLBox *box)
{
  emlbox_add_el(box,
                EMLListBox.new(0, NULL,
                               ALIGN_LEFT,
                               EMLPROCESS_BORDER,
                               EMLPROCESS_SECTIONWIDTH,
                               LINESTYLE_SOLID,
                               NULL, NULL));
}

void
emlprocess_create_box(EMLProcess *emlprocess)
{
  ConnectionPoint *proc_connections;
  EMLBox *topbox, *childbox, *textbox;
  
  proc_connections = emlprocess->proc_connections;

  topbox = EMLListBox.new(0, NULL,
                          ALIGN_LEFT,
                          EMLPROCESS_BORDER,
                          EMLPROCESS_SECTIONWIDTH,
                          LINESTYLE_SOLID,
                          NULL, NULL);

  /*name section*/
  textbox = EMLTextBox.new(emlprocess->name_font_height,
                           emlprocess->name_font,
                           ALIGN_LEFT,
                           EMLPROCESS_BORDER,
                           EMLPROCESS_SECTIONWIDTH,
                           LINESTYLE_SOLID,
                           &proc_connections[0],
                           &proc_connections[1]);
  emlprocess->name_box = textbox;
  emlbox_add_el(textbox, g_strjoin(" ", emlprocess->name, emlprocess->refname, NULL));
  emlbox_add_el(topbox, textbox);

  box_add_separator(topbox);

  childbox = create_startup_box(emlprocess);
  if (childbox != NULL)
    emlbox_add_el(topbox, childbox);

  box_add_separator(topbox);

  /* proclife section */
  textbox = EMLTextBox.new(emlprocess->proclife_font_height,
                           emlprocess->proclife_font,
                           ALIGN_CENTER,
                           EMLPROCESS_BORDER,
                           EMLPROCESS_SECTIONWIDTH,
                           LINESTYLE_SOLID,
                           &proc_connections[2],
                           &proc_connections[3]);
  emlbox_add_el(textbox, g_strdup(emlprocess->proclife));
  emlbox_add_el(topbox, textbox);

  box_add_separator(topbox);

  childbox = create_interfaces_box(emlprocess);
  if (childbox != NULL)
    emlbox_add_el(topbox, childbox);

  emlprocess->box = topbox;
}

void
emlprocess_destroy_box(EMLProcess *emlprocess)
{
  emlbox_destroy(emlprocess->box);
}

static
void
emlprocess_dialog_init(EMLProcess *emlprocess)
{
  emlprocess->dialog_state.name = NULL;
  emlprocess->dialog_state.refname = NULL;
  emlprocess->dialog_state.proclife = NULL;
  emlprocess->dialog_state.startupfun = NULL;
  emlprocess->dialog_state.interfaces = NULL;
}

static
void
fill_in_fontdata(EMLProcess *emlprocess)
{
  emlprocess->name_font_height = 0.8;
  emlprocess->startupfun_font_height = 0.8;
  emlprocess->startupparams_height = 0.8;
  emlprocess->proclife_font_height = 0.8;
  emlprocess->interface_font_height = 0.8;

  /* choose default font name for your locale. see also font_data structure
     in lib/font.c. if "Courier" works for you, it would be better.  */
  emlprocess->name_font       = g_strdup(_("Courier"));
  /* choose default font name for your locale. see also font_data structure
     in lib/font.c. */
  emlprocess->startupfun_font = g_strdup(_("Helvetica"));
  emlprocess->startupparams_font   = g_strdup(_("Helvetica"));
  emlprocess->proclife_font   = g_strdup(_("Helvetica"));
  emlprocess->interface_font  = g_strdup(_("Helvetica"));

}
 
static
DiaObject *
emlprocess_create(Point *startpoint,
	       void *user_data,
  	       Handle **handle1,
	       Handle **handle2)
{
  EMLProcess *emlprocess;
  Element *elem;
  DiaObject *obj;
  GList *list;
  int i;
  ConnectionPoint *connection;

  emlprocess = g_new0(EMLProcess, 1);
  elem = &emlprocess->element;
  obj = (DiaObject *) emlprocess;
  
  obj->type = &emlprocess_type;

  obj->ops = &emlprocess_ops;

  elem->corner = *startpoint;

  emlprocess->properties_dialog = NULL;
  emlprocess->box_connections = NULL;
  emlprocess->box = NULL;
  emlprocess->name = g_strdup("X");
  emlprocess->refname = g_strdup("process");
  emlprocess->proclife = g_strdup("inf");

  emlprocess->startupfun = eml_function_new();
  function_connections_new(emlprocess->startupfun);

  emlprocess->interfaces = NULL;

  fill_in_fontdata(emlprocess);

  emlprocess_dialog_init(emlprocess);

  emlprocess_create_box(emlprocess);
  emlprocess_calculate_data(emlprocess);
  emlprocess_calculate_connections(emlprocess);
  element_init(elem, 8, g_list_length(emlprocess->box_connections));

  list = emlprocess->box_connections;

  for (i=0;list != NULL;i++) {
    connection = (ConnectionPoint*) list->data;
    obj->connections[i] = connection;
    connection->object = obj;
    connection->connected = NULL;
    list = g_list_next(list);
  }

  emlprocess_update_data(emlprocess);

  for (i=0;i<8;i++) {
    obj->handles[i]->type = HANDLE_NON_MOVABLE;
  }

  *handle1 = NULL;
  *handle2 = NULL;
  return (DiaObject *)emlprocess;

}

static void
emlprocess_destroy(EMLProcess *emlprocess)
{
  element_destroy(&emlprocess->element);
  
  g_free(emlprocess->name);
  g_free(emlprocess->refname);
  g_free(emlprocess->proclife);
  function_connections_destroy(emlprocess->startupfun);
  eml_function_destroy(emlprocess->startupfun);
  g_list_foreach(emlprocess->interfaces, list_free_foreach,
                 emlprocess_interface_destroy);
  g_list_free(emlprocess->interfaces);

  g_list_free(emlprocess->box_connections);

  emlprocess_destroy_box(emlprocess);

  if (emlprocess->properties_dialog != NULL)
    emlprocess_dialog_destroy(emlprocess);

  g_free(emlprocess->name_font      );
  g_free(emlprocess->startupfun_font);
  g_free(emlprocess->startupparams_font  );
  g_free(emlprocess->proclife_font  );
  g_free(emlprocess->interface_font );

}

static DiaObject *
emlprocess_copy(EMLProcess *emlprocess)
{
  EMLProcess *newemlprocess;
  Element *elem, *newelem;
  DiaObject *newobj;
  GList *list;
  ConnectionPoint *connection;
  int i;

  elem = &emlprocess->element;
  
  newemlprocess = g_new0(EMLProcess, 1);
  newelem = &newemlprocess->element;
  newobj = (DiaObject *) newemlprocess;

  element_copy(elem, newelem);

  newemlprocess->properties_dialog = NULL;
  newemlprocess->box_connections = NULL;
  newemlprocess->box = NULL;
  newemlprocess->interfaces = NULL;

  newemlprocess->dialog_state.name = NULL;
  newemlprocess->dialog_state.refname = NULL;
  newemlprocess->dialog_state.proclife = NULL;
  newemlprocess->dialog_state.startupfun = NULL;
  newemlprocess->dialog_state.interfaces = NULL;

  newemlprocess->name = g_strdup(emlprocess->name);
  newemlprocess->refname = g_strdup(emlprocess->refname);
  newemlprocess->proclife = g_strdup(emlprocess->proclife);
  newemlprocess->startupfun = eml_function_copy(emlprocess->startupfun);
  newemlprocess->interfaces = list_map(emlprocess->interfaces,
                              (MapFun) emlprocess_interface_copy);

  function_connections_new(newemlprocess->startupfun);
  g_list_foreach(newemlprocess->interfaces,
                 list_foreach_fun, interface_connections_new);

  newemlprocess->name_font       =   g_strdup(emlprocess->name_font      );
  newemlprocess->startupfun_font =   g_strdup(emlprocess->startupfun_font);
  newemlprocess->startupparams_font   =
    g_strdup(emlprocess->startupparams_font  );
  newemlprocess->proclife_font   =   g_strdup(emlprocess->proclife_font  );
  newemlprocess->interface_font  =   g_strdup(emlprocess->interface_font );

  newemlprocess->name_font_height       =   emlprocess->name_font_height      ;
  newemlprocess->startupfun_font_height =   emlprocess->startupfun_font_height;
  newemlprocess->startupparams_height   =   emlprocess->startupparams_height  ;
  newemlprocess->proclife_font_height   =   emlprocess->proclife_font_height  ;
  newemlprocess->interface_font_height  =   emlprocess->interface_font_height ;

  emlprocess_dialog_init(newemlprocess);

  emlprocess_create_box(newemlprocess);
  emlprocess_calculate_data(newemlprocess);
  emlprocess_calculate_connections(newemlprocess);

  list = newemlprocess->box_connections;
  for (i = 0; list != NULL; i++) {
    connection = (ConnectionPoint*) list->data;
    newobj->connections[i] = connection;
    connection->connected = NULL;
    connection->object = newobj;
    connection->last_pos =
      ((ConnectionPoint*)
       g_list_nth_data(emlprocess->box_connections, i))->last_pos;
    list = g_list_next(list);
  }

  emlprocess_update_data(newemlprocess);

  return (DiaObject *)newemlprocess;

}


static void
emlprocess_save(EMLProcess *emlprocess, ObjectNode obj_node,
	      const char *filename)
{
  GList *list;
  AttributeNode attr_node;
  EMLInterface *iface;

  element_save(&emlprocess->element, obj_node);

  data_add_string(new_attribute(obj_node, "name"),
		  emlprocess->name);
  data_add_string(new_attribute(obj_node, "refname"),
		  emlprocess->refname);
  data_add_string(new_attribute(obj_node, "proclife"),
		  emlprocess->proclife);

  attr_node = new_attribute(obj_node, "startupfun");
  eml_function_write(attr_node, emlprocess->startupfun);

  attr_node = new_attribute(obj_node, "interfaces");

  list =   emlprocess->interfaces;
  while (list != NULL) {
    iface = (EMLInterface*) list->data;
    eml_interface_write(attr_node, iface);
    list = g_list_next(list);
  }

}

static DiaObject *emlprocess_load(ObjectNode obj_node, int version,
			     const char *filename)
{
  EMLProcess *emlprocess;
  Element *elem;
  DiaObject *obj;
  AttributeNode attr_node;
  DataNode composite;
  EMLInterface *iface;
  GList *list;
  ConnectionPoint *connection;
  int i;
  int num;
  
  emlprocess = g_new0(EMLProcess, 1);
  elem = &emlprocess->element;
  obj = (DiaObject *) emlprocess;
  
  obj->type = &emlprocess_type;
  obj->ops = &emlprocess_ops;

  element_load(elem, obj_node);
  
  fill_in_fontdata(emlprocess);

  emlprocess->name = NULL;
  attr_node = object_find_attribute(obj_node, "name");
  if (attr_node != NULL)
    emlprocess->name = data_string(attribute_first_data(attr_node));

  emlprocess->refname = NULL;
  attr_node = object_find_attribute(obj_node, "refname");
  if (attr_node != NULL)
    emlprocess->refname = data_string(attribute_first_data(attr_node));

  emlprocess->proclife = NULL;
  attr_node = object_find_attribute(obj_node, "proclife");
  if (attr_node != NULL)
    emlprocess->proclife = data_string(attribute_first_data(attr_node));

  attr_node = object_find_attribute(obj_node, "startupfun");
  composite = attribute_first_data(attr_node);
  emlprocess->startupfun = eml_function_read(composite);
  function_connections_new(emlprocess->startupfun);
  /* Interfaces: */
  attr_node = object_find_attribute(obj_node, "interfaces");
  num = attribute_num_data(attr_node);
  composite = attribute_first_data(attr_node);
  emlprocess->interfaces = NULL;
  for (i=0;i<num;i++) {
    iface = eml_interface_read(composite);
    interface_connections_new(iface);
    emlprocess->interfaces = g_list_append(emlprocess->interfaces, iface);
    composite = data_next(composite);
  }

  emlprocess->properties_dialog = NULL;
  emlprocess->box = NULL;
  emlprocess->box_connections = NULL;

  emlprocess_create_box(emlprocess);  
  emlprocess_calculate_connections(emlprocess);
  element_init(elem, 8, g_list_length(emlprocess->box_connections));

  list = emlprocess->box_connections;

  for (i=0;list != NULL;i++) {
    connection = (ConnectionPoint*) list->data;
    obj->connections[i] = connection;
    connection->object = obj;
    connection->connected = NULL;
    list = g_list_next(list);
  }

  emlprocess_update_data(emlprocess);

  for (i=0;i<8;i++) {
    obj->handles[i]->type = HANDLE_NON_MOVABLE;
  }

  emlprocess_dialog_init(emlprocess);

  return (DiaObject *)emlprocess;
}

