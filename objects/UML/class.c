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

#include "intl.h"
#include "diarenderer.h"
#include "attributes.h"
#include "properties.h"
#include "diamenu.h"

#include "class.h"

#include "pixmaps/umlclass.xpm"

#define UMLCLASS_BORDER 0.1
#define UMLCLASS_UNDERLINEWIDTH 0.05

static real umlclass_distance_from(UMLClass *umlclass, Point *point);
static void umlclass_select(UMLClass *umlclass, Point *clicked_point,
			    DiaRenderer *interactive_renderer);
static ObjectChange* umlclass_move_handle(UMLClass *umlclass, Handle *handle,
				 Point *to, ConnectionPoint *cp,
                                          HandleMoveReason reason, ModifierKeys modifiers);
static ObjectChange* umlclass_move(UMLClass *umlclass, Point *to);
static void umlclass_draw(UMLClass *umlclass, DiaRenderer *renderer);
static DiaObject *umlclass_create(Point *startpoint,
			       void *user_data,
			       Handle **handle1,
			       Handle **handle2);
static void umlclass_destroy(UMLClass *umlclass);
static DiaObject *umlclass_copy(UMLClass *umlclass);

static void umlclass_save(UMLClass *umlclass, ObjectNode obj_node,
			  const char *filename);
static DiaObject *umlclass_load(ObjectNode obj_node, int version,
			     const char *filename);

static DiaMenu * umlclass_object_menu(DiaObject *obj, Point *p);
static ObjectChange *umlclass_show_comments_callback(DiaObject *obj, Point *pos, gpointer data);

static PropDescription *umlclass_describe_props(UMLClass *umlclass);
static void umlclass_get_props(UMLClass *umlclass, GPtrArray *props);
static void umlclass_set_props(UMLClass *umlclass, GPtrArray *props);

static void fill_in_fontdata(UMLClass *umlclass);

static ObjectTypeOps umlclass_type_ops =
{
  (CreateFunc) umlclass_create,
  (LoadFunc)   umlclass_load,
  (SaveFunc)   umlclass_save
};

DiaObjectType umlclass_type =
{
  "UML - Class",   /* name */
  0,                      /* version */
  (char **) umlclass_xpm,  /* pixmap */
  
  &umlclass_type_ops       /* ops */
};

static ObjectOps umlclass_ops = {
  (DestroyFunc)         umlclass_destroy,
  (DrawFunc)            umlclass_draw,
  (DistanceFunc)        umlclass_distance_from,
  (SelectFunc)          umlclass_select,
  (CopyFunc)            umlclass_copy,
  (MoveFunc)            umlclass_move,
  (MoveHandleFunc)      umlclass_move_handle,
  (GetPropertiesFunc)   umlclass_get_properties,
  (ApplyPropertiesFunc) umlclass_apply_properties,
  (ObjectMenuFunc)      umlclass_object_menu,
  (DescribePropsFunc)   umlclass_describe_props,
  (GetPropsFunc)        umlclass_get_props,
  (SetPropsFunc)        umlclass_set_props
};

static PropDescription umlclass_props[] = {
  ELEMENT_COMMON_PROPERTIES,
  PROP_STD_TEXT_COLOUR_OPTIONAL,
  PROP_STD_LINE_COLOUR_OPTIONAL,
  PROP_STD_FILL_COLOUR_OPTIONAL,

  PROP_STD_NOTEBOOK_BEGIN,
  PROP_NOTEBOOK_PAGE("class", PROP_FLAG_DONT_MERGE, N_("Class")),
  { "name", PROP_TYPE_STRING, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
  N_("Name"), NULL, NULL },
  { "stereotype", PROP_TYPE_STRING, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
  N_("Stereotype"), NULL, NULL },
  { "comment", PROP_TYPE_STRING, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
  N_("Comment"), NULL, NULL },
  { "abstract", PROP_TYPE_BOOL, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
  N_("Abstract"), NULL, NULL },
  { "template", PROP_TYPE_BOOL, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
  N_("Template"), NULL, NULL },

  { "suppress_attributes", PROP_TYPE_BOOL, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
  N_("Suppress Attributes"), NULL, NULL },
  { "suppress_operations", PROP_TYPE_BOOL, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
  N_("Suppress Operations"), NULL, NULL },
  { "visible_attributes", PROP_TYPE_BOOL, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
  N_("Visible Attributes"), NULL, NULL },
  { "visible_operations", PROP_TYPE_BOOL, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
  N_("Visible Operations"), NULL, NULL },
  { "visible_comments", PROP_TYPE_BOOL, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
  N_("Visible Comments"), NULL, NULL },
  { "wrap_operations", PROP_TYPE_BOOL, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
  N_("Wrap Operations"), NULL, NULL },
  { "wrap_after_char", PROP_TYPE_INT, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
  N_("Wrap after char"), NULL, NULL },

  /* all this just to make the defaults selectable ... */
  PROP_NOTEBOOK_PAGE("font", PROP_FLAG_DONT_MERGE, N_("Font")),
  PROP_STD_MULTICOL_BEGIN,
  PROP_MULTICOL_COLUMN("font"),
  /* FIXME: apparently multicol does not work correctly, this should be FIRST column */
  { "normal_font", PROP_TYPE_FONT, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
  N_("Normal"), NULL, NULL },
  { "polymorphic_font", PROP_TYPE_FONT, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
  N_("Polymorphic"), NULL, NULL },
  { "abstract_font", PROP_TYPE_FONT, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
  N_("Abstract"), NULL, NULL },
  { "classname_font", PROP_TYPE_FONT, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
  N_("Classname"), NULL, NULL },
  { "abstract_classname_font", PROP_TYPE_FONT, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
  N_("Abstract Classname"), NULL, NULL },
  { "comment_font", PROP_TYPE_FONT, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
  N_("Comment"), NULL, NULL },

  PROP_MULTICOL_COLUMN("height"),
  { "normal_font_height", PROP_TYPE_REAL, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
  N_(" "), NULL, NULL },
  { "polymorphic_font_height", PROP_TYPE_REAL, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
  N_(" "), NULL, NULL },
  { "abstract_font_height", PROP_TYPE_REAL, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
  N_(" "), NULL, NULL },
  { "classname_font_height", PROP_TYPE_REAL, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
  N_(" "), NULL, NULL },
  { "abstract_classname_font_height", PROP_TYPE_REAL, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
  N_(" "), NULL, NULL },
  { "comment_font_height", PROP_TYPE_REAL, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
  N_(" "), NULL, NULL },
  PROP_STD_MULTICOL_END,
  PROP_STD_NOTEBOOK_END,

  { "attributes", PROP_TYPE_STRINGLIST, PROP_FLAG_DONT_SAVE,
  N_("Attributes"), NULL, NULL }, 
  { "operations", PROP_TYPE_STRINGLIST, PROP_FLAG_DONT_SAVE,
  N_("Operations"), NULL, NULL }, 

  /* formal_params XXX */

  PROP_DESC_END
};

static PropDescription *
umlclass_describe_props(UMLClass *umlclass)
{
  if (umlclass_props[0].quark == 0)
    prop_desc_list_calculate_quarks(umlclass_props);
  return umlclass_props;
}

static PropOffset umlclass_offsets[] = {
  ELEMENT_COMMON_PROPERTIES_OFFSETS,

  { "line_colour", PROP_TYPE_COLOUR, offsetof(UMLClass, line_color) },
  { "fill_colour", PROP_TYPE_COLOUR, offsetof(UMLClass, fill_color) },
  { "name", PROP_TYPE_STRING, offsetof(UMLClass, name) },
  { "stereotype", PROP_TYPE_STRING, offsetof(UMLClass, stereotype) },
  { "comment", PROP_TYPE_STRING, offsetof(UMLClass, comment) },
  { "abstract", PROP_TYPE_BOOL, offsetof(UMLClass, abstract) },
  { "template", PROP_TYPE_BOOL, offsetof(UMLClass, template) },
  { "suppress_attributes", PROP_TYPE_BOOL, offsetof(UMLClass , suppress_attributes) },
  { "visible_attributes", PROP_TYPE_BOOL, offsetof(UMLClass , visible_attributes) },
  { "visible_comments", PROP_TYPE_BOOL, offsetof(UMLClass , visible_comments) },
  { "suppress_operations", PROP_TYPE_BOOL, offsetof(UMLClass , suppress_operations) },
  { "visible_operations", PROP_TYPE_BOOL, offsetof(UMLClass , visible_operations) },
  { "visible_comments", PROP_TYPE_BOOL, offsetof(UMLClass , visible_comments) },
  { "wrap_operations", PROP_TYPE_BOOL, offsetof(UMLClass , wrap_operations) },
  { "wrap_after_char", PROP_TYPE_INT, offsetof(UMLClass , wrap_after_char) },
  
  /* all this just to make the defaults selectable ... */
  PROP_OFFSET_STD_MULTICOL_BEGIN,
  PROP_OFFSET_MULTICOL_COLUMN("font"),
  { "normal_font", PROP_TYPE_FONT, offsetof(UMLClass, normal_font) },
  { "abstract_font", PROP_TYPE_FONT, offsetof(UMLClass, abstract_font) },
  { "polymorphic_font", PROP_TYPE_FONT, offsetof(UMLClass, polymorphic_font) },
  { "classname_font", PROP_TYPE_FONT, offsetof(UMLClass, classname_font) },
  { "abstract_classname_font", PROP_TYPE_FONT, offsetof(UMLClass, abstract_classname_font) },
  { "comment_font", PROP_TYPE_FONT, offsetof(UMLClass, comment_font) },

  PROP_OFFSET_MULTICOL_COLUMN("height"),
  { "normal_font_height", PROP_TYPE_REAL, offsetof(UMLClass, font_height) },
  { "abstract_font_height", PROP_TYPE_REAL, offsetof(UMLClass, abstract_font_height) },
  { "polymorphic_font_height", PROP_TYPE_REAL, offsetof(UMLClass, polymorphic_font_height) },
  { "classname_font_height", PROP_TYPE_REAL, offsetof(UMLClass, classname_font_height) },
  { "abstract_classname_font_height", PROP_TYPE_REAL, offsetof(UMLClass, abstract_classname_font_height) },
  { "comment_font_height", PROP_TYPE_REAL, offsetof(UMLClass, comment_font_height) },
  PROP_OFFSET_STD_MULTICOL_END,

  { "operations", PROP_TYPE_STRINGLIST, offsetof(UMLClass , operations_strings) },
  { "attributes", PROP_TYPE_STRINGLIST, offsetof(UMLClass , attributes_strings) },

  { NULL, 0, 0 },
};

static void
umlclass_get_props(UMLClass * umlclass, GPtrArray *props)
{
  object_get_props_from_offsets(&umlclass->element.object, 
                                umlclass_offsets, props);
}

static DiaMenuItem umlclass_menu_items[] = {
        { N_("Show Comments"), umlclass_show_comments_callback, NULL, 
          DIAMENU_ACTIVE|DIAMENU_TOGGLE },
        NULL
};

static DiaMenu umlclass_menu = {
        N_("Class"),
        sizeof(umlclass_menu_items)/sizeof(DiaMenuItem),
        umlclass_menu_items,
        NULL
};

DiaMenu *
umlclass_object_menu(DiaObject *obj, Point *p)
{
        umlclass_menu_items[0].active = DIAMENU_ACTIVE|DIAMENU_TOGGLE|
                (((UMLClass *)obj)->visible_comments?DIAMENU_TOGGLE_ON:0);

        return &umlclass_menu;
}

ObjectChange *umlclass_show_comments_callback(DiaObject *obj, Point *pos, gpointer data)
{
  ObjectChange *change = new_object_state_change(obj, NULL, NULL, NULL );

  ((UMLClass *)obj)->visible_comments = !((UMLClass *)obj)->visible_comments;
  umlclass_calculate_data((UMLClass *)obj);
  umlclass_update_data((UMLClass *)obj);
  return change;
}

static void
umlclass_set_props(UMLClass *umlclass, GPtrArray *props)
{
  object_set_props_from_offsets(&umlclass->element.object, umlclass_offsets,
                                props);
	
  /* Update data: */
  umlclass_calculate_data(umlclass);
  umlclass_update_data(umlclass);
}

static real
umlclass_distance_from(UMLClass *umlclass, Point *point)
{
  DiaObject *obj = &umlclass->element.object;
  return distance_rectangle_point(&obj->bounding_box, point);
}

static void
umlclass_select(UMLClass *umlclass, Point *clicked_point,
	       DiaRenderer *interactive_renderer)
{
  element_update_handles(&umlclass->element);
}

static ObjectChange*
umlclass_move_handle(UMLClass *umlclass, Handle *handle,
		     Point *to, ConnectionPoint *cp,
                     HandleMoveReason reason, ModifierKeys modifiers)
{
  assert(umlclass!=NULL);
  assert(handle!=NULL);
  assert(to!=NULL);

  assert(handle->id < UMLCLASS_CONNECTIONPOINTS);

  return NULL;
}

static ObjectChange*
umlclass_move(UMLClass *umlclass, Point *to)
{
  umlclass->element.corner = *to;
  umlclass_update_data(umlclass);

  return NULL;
}

static void
umlclass_draw(UMLClass *umlclass, DiaRenderer *renderer)
{
            /* Most of this crap could be rendered much more efficiently
               (and probably much cleaner as well) using marked-up
               Pango layout text.

            Breaking this function up into smaller, manageable and indented
            pieces would not hurt either -- cc */

  DiaRendererClass *renderer_ops = DIA_RENDERER_GET_CLASS (renderer);
  Element *elem;
  real x,y;
  Point p, p1, p2, p3;
  DiaFont *font;
  real font_height;
  int i;
  GList *list;
  
  assert(umlclass != NULL);
  assert(renderer != NULL);

  elem = &umlclass->element;

  x = elem->corner.x;
  y = elem->corner.y;

  renderer_ops->set_fillstyle(renderer, FILLSTYLE_SOLID);
  renderer_ops->set_linewidth(renderer, UMLCLASS_BORDER);
  renderer_ops->set_linestyle(renderer, LINESTYLE_SOLID);

  p1.x = x;
  p2.x = x + elem->width;
  p1.y = y;
  y += umlclass->namebox_height;
  p2.y = y;
  
  renderer_ops->fill_rect(renderer, 
                           &p1, &p2,
                           &umlclass->fill_color);
  renderer_ops->draw_rect(renderer, 
                           &p1, &p2,
                           &umlclass->line_color);

  /* stereotype: */
  p.x = x + elem->width / 2.0;
  p.y = p1.y;

  if (umlclass->stereotype != NULL && umlclass->stereotype[0] != '\0') {
    p.y += 0.1 + dia_font_ascent(umlclass->stereotype_string,
                                 umlclass->normal_font,
                                 umlclass->font_height);
    
    renderer_ops->set_font(renderer,
                            umlclass->normal_font, umlclass->font_height);
    renderer_ops->draw_string(renderer,
                               umlclass->stereotype_string,
                               &p, ALIGN_CENTER, 
                               &umlclass->text_color);
  }

  if (umlclass->name != NULL) {
  /* name: */
    if (umlclass->abstract) {
      font = umlclass->abstract_classname_font;
      font_height = umlclass->abstract_classname_font_height;
    } else {
      font = umlclass->classname_font;
      font_height = umlclass->classname_font_height;
    }
    p.y += font_height;

    renderer_ops->set_font(renderer, font, font_height);
    renderer_ops->draw_string(renderer,
                              umlclass->name,
                              &p, ALIGN_CENTER, 
                              &umlclass->text_color);
  }

  /* comment */
  if (umlclass->visible_comments && umlclass->comment != NULL && umlclass->comment[0] != '\0')
  {
    font = umlclass->comment_font;
    font_height = umlclass->comment_font_height;

    renderer_ops->set_font(renderer, font, font_height);
    p.y += font_height;
       
    renderer_ops->draw_string(renderer,
                               umlclass->comment,
                               &p, ALIGN_CENTER,
                               &umlclass->text_color);
  }
            
  if (umlclass->visible_attributes) {
    p1.x = x;
    p1.y = y;
    y += umlclass->attributesbox_height;
    p2.y = y;
  
    renderer_ops->fill_rect(renderer, 
			     &p1, &p2,
			     &umlclass->fill_color);
    renderer_ops->draw_rect(renderer, 
			     &p1, &p2,
			     &umlclass->line_color);

    if (!umlclass->suppress_attributes) {
      p.x = x + UMLCLASS_BORDER/2.0 + 0.1;
      p.y = p1.y + 0.1;

      i = 0;
      list = umlclass->attributes;
      while (list != NULL) {
         UMLAttribute *attr = (UMLAttribute *)list->data;
         gchar *attstr = g_list_nth(umlclass->attributes_strings, i)->data;
         real ascent;
         
         if (attr->abstract) {
           font = umlclass->abstract_font;
           font_height = umlclass->abstract_font_height;
         } else {
           font = umlclass->normal_font;
           font_height = umlclass->font_height;
         }
         ascent = dia_font_ascent(attstr,
                                  font,font_height);     
         p.y += ascent;
         
         renderer_ops->set_font (renderer, font, font_height);
         renderer_ops->draw_string(renderer,
                                    attstr,
                                    &p, ALIGN_LEFT, 
                                    &umlclass->text_color);


         if (attr->class_scope) {
           p1 = p; 
           p1.y += font_height * 0.1;
           p3 = p1;
           p3.x += dia_font_string_width(attstr,
                                         font, font_height);
           renderer_ops->set_linewidth(renderer, UMLCLASS_UNDERLINEWIDTH);
           renderer_ops->draw_line(renderer,
                                    &p1, &p3, &umlclass->line_color);
           renderer_ops->set_linewidth(renderer, UMLCLASS_BORDER);
         }    
         
         p.y += font_height - ascent;
         
         if (umlclass->visible_comments && attr->comment != NULL && attr->comment[0] != '\0')
         {
           p1 = p;
           
           font = umlclass->comment_font;
           font_height = umlclass->comment_font_height;
           
           p1.y += ascent;

           renderer_ops->set_font(renderer, font, font_height);
                       
           renderer_ops->draw_string(renderer,
                                      attr->comment,
                                      &p1, ALIGN_LEFT,
                                      &umlclass->text_color);
           p.y += font_height;

         }

         list = g_list_next(list);
         i++;
      }
    }
  }


  if (umlclass->visible_operations) {
    p1.x = x;
    p1.y = y;
    y += umlclass->operationsbox_height;
    p2.y = y;
    
    renderer_ops->fill_rect(renderer, 
                             &p1, &p2,
                             &umlclass->fill_color);
    renderer_ops->draw_rect(renderer, 
                             &p1, &p2,
                             &umlclass->line_color);
    if (!umlclass->suppress_operations) {
      GList *wrapsublist = NULL;
      gchar *part_opstr;
      int wrap_pos, last_wrap_pos, ident, wrapping_needed;

      p.x = x + UMLCLASS_BORDER/2.0 + 0.1;
      p.y = p1.y + 0.1;

      part_opstr = g_alloca (umlclass->max_wrapped_line_width);

      i = 0;
      list = umlclass->operations;
      while (list != NULL) {
        UMLOperation *op = (UMLOperation *)list->data;
        gchar* opstr;
        real ascent;
        /* Must add a new font for virtual yet not abstract methods.
           Bold Italic for abstract? */
        if (op->inheritance_type == UML_ABSTRACT) {
          font = umlclass->abstract_font;
          font_height = umlclass->abstract_font_height;
        } else if (op->inheritance_type == UML_POLYMORPHIC) {
          font = umlclass->polymorphic_font;
          font_height = umlclass->polymorphic_font_height;
        } else {
          font = umlclass->normal_font;
          font_height = umlclass->font_height;
        }

        wrapping_needed = 0;
        opstr = (gchar*) g_list_nth(umlclass->operations_strings, i)->data;
        if( umlclass->wrap_operations ) {
          wrapsublist = (GList*)g_list_nth( umlclass->operations_wrappos, i)->data;
          wrapping_needed = GPOINTER_TO_INT( wrapsublist->data );
        }

        ascent = dia_font_ascent(opstr, font, font_height);
        renderer_ops->set_font(renderer, font, font_height);
        
        if( umlclass->wrap_operations && wrapping_needed) {
          
          wrapsublist = g_list_next( wrapsublist);
          ident = GPOINTER_TO_INT( wrapsublist->data);
          wrapsublist = g_list_next( wrapsublist);
          wrap_pos = last_wrap_pos = 0;
          
          while( wrapsublist != NULL) {
            
            wrap_pos = GPOINTER_TO_INT( wrapsublist->data);
            
            if( last_wrap_pos == 0) {
              strncpy( part_opstr, opstr, wrap_pos);  
              memset( part_opstr+wrap_pos, '\0', 1);
            } else {  
              memset( part_opstr, ' ', ident);
              memset( part_opstr+ident, '\0', 1);
              strncat( part_opstr, opstr+last_wrap_pos, wrap_pos-last_wrap_pos);                
            }
            
            p.y += ascent;
  
            renderer_ops->draw_string(renderer,
                                      part_opstr,
                                      &p, ALIGN_LEFT, 
                                      &umlclass->text_color);
          
            last_wrap_pos = wrap_pos;
            wrapsublist = g_list_next( wrapsublist);
          }
        } else {

          p.y += ascent;

        renderer_ops->draw_string(renderer,
                                   opstr,
                                   &p, ALIGN_LEFT, 
                                   &umlclass->text_color);
        }

        if (op->class_scope) {
          p1 = p; 
          p1.y += font_height * 0.1;
          p3 = p1;
          p3.x += dia_font_string_width(opstr,
                                        font, font_height);
          renderer_ops->set_linewidth(renderer, UMLCLASS_UNDERLINEWIDTH);
          renderer_ops->draw_line(renderer, &p1, &p3,
                                   &umlclass->line_color);
          renderer_ops->set_linewidth(renderer, UMLCLASS_BORDER);
        }

        p.y += font_height - ascent;
        

        if (umlclass->visible_comments && op->comment != NULL && op->comment[0] != '\0')
        {
          p1 = p;
          p1.y += ascent;
          
          font = umlclass->comment_font;
          font_height = umlclass->comment_font_height;
          
          renderer_ops->set_font(renderer, font, font_height);
          
          renderer_ops->draw_string(renderer,
                                     op->comment,
                                     &p1, ALIGN_LEFT, 
                                     &umlclass->text_color);
          
          p.y += font_height;
        }


        list = g_list_next(list);
        i++;
      }
    }
  }

  if (umlclass->template) {
    x = elem->corner.x + elem->width - 2.3; 
    y = elem->corner.y - umlclass->templates_height + 0.3; 

    p1.x = x;
    p1.y = y;
    p2.x = x + umlclass->templates_width;
    p2.y = y + umlclass->templates_height;
    
    renderer_ops->fill_rect(renderer, 
                             &p1, &p2,
                             &umlclass->fill_color);

    renderer_ops->set_linestyle(renderer, LINESTYLE_DASHED);
    renderer_ops->set_dashlength(renderer, 0.3);
    
    renderer_ops->draw_rect(renderer, 
                             &p1, &p2,
                             &umlclass->line_color);

    p.x = x + 0.3;

    renderer_ops->set_font(renderer, umlclass->normal_font,
                            umlclass->font_height);
    i = 0;
    list = umlclass->formal_params;
    while (list != NULL) {
      /*UMLFormalParameter *param = (UMLFormalParameter *)list->data;*/
      p.y = y + 0.1 + (umlclass->font_height * i) +
              dia_font_ascent(umlclass->templates_strings[i],
                              umlclass->normal_font, umlclass->font_height);

      renderer_ops->draw_string(renderer,
                                 umlclass->templates_strings[i],
                                 &p, ALIGN_LEFT, 
                                 &umlclass->text_color);      
      
      list = g_list_next(list);
      i++;
    }
  }
}

void
umlclass_update_data(UMLClass *umlclass)
{
  Element *elem = &umlclass->element;
  DiaObject *obj = &elem->object;
  real x,y;
  GList *list;
  int i;
  int pointswide;
  int lowerleftcorner;
  real pointspacing;

  x = elem->corner.x;
  y = elem->corner.y;
  
  /* Update connections: */
  umlclass->connections[0].pos = elem->corner;
  umlclass->connections[0].directions = DIR_NORTH|DIR_WEST;

  /* there are four corner points and two side points, thus all
   * remaining points are on the top/bottom width
   */
  pointswide = (UMLCLASS_CONNECTIONPOINTS - 6) / 2;
  pointspacing = elem->width / (pointswide + 1.0);

  /* across the top connection points */
  for (i=1;i<=pointswide;i++) {
    umlclass->connections[i].pos.x = x + (pointspacing * i);
    umlclass->connections[i].pos.y = y;
    umlclass->connections[i].directions = DIR_NORTH;
  }

  i = (UMLCLASS_CONNECTIONPOINTS / 2) - 2;
  umlclass->connections[i].pos.x = x + elem->width;
  umlclass->connections[i].pos.y = y;
  umlclass->connections[i].directions = DIR_NORTH|DIR_EAST;

  i = (UMLCLASS_CONNECTIONPOINTS / 2) - 1;
  umlclass->connections[i].pos.x = x;
  umlclass->connections[i].pos.y = y + umlclass->namebox_height / 2.0;
  umlclass->connections[i].directions = DIR_WEST;

  i = (UMLCLASS_CONNECTIONPOINTS / 2);
  umlclass->connections[i].pos.x = x + elem->width;
  umlclass->connections[i].pos.y = y + umlclass->namebox_height / 2.0;
  umlclass->connections[i].directions = DIR_EAST;

  i = (UMLCLASS_CONNECTIONPOINTS / 2) + 1;
  umlclass->connections[i].pos.x = x;
  umlclass->connections[i].pos.y = y + elem->height;
  umlclass->connections[i].directions = DIR_WEST|DIR_SOUTH;

  /* across the bottom connection points */
  lowerleftcorner = (UMLCLASS_CONNECTIONPOINTS / 2) + 1;
  for (i=1;i<=pointswide;i++) {
    umlclass->connections[lowerleftcorner + i].pos.x = x + (pointspacing * i);
    umlclass->connections[lowerleftcorner + i].pos.y = y + elem->height;
    umlclass->connections[lowerleftcorner + i].directions = DIR_SOUTH;
  }

  /* bottom-right corner */
  i = (UMLCLASS_CONNECTIONPOINTS) - 1;
  umlclass->connections[i].pos.x = x + elem->width;
  umlclass->connections[i].pos.y = y + elem->height;
  umlclass->connections[i].directions = DIR_EAST|DIR_SOUTH;

  y += umlclass->namebox_height + 0.1 + umlclass->font_height/2;

  list = umlclass->attributes;
  while (list != NULL) {
    UMLAttribute *attr = (UMLAttribute *)list->data;

    attr->left_connection->pos.x = x;
    attr->left_connection->pos.y = y;
    attr->left_connection->directions = DIR_WEST;
    attr->right_connection->pos.x = x + elem->width;
    attr->right_connection->pos.y = y;
    attr->right_connection->directions = DIR_EAST;

    y += umlclass->font_height;
    if (umlclass->visible_comments && attr->comment != NULL && attr->comment[0] != '\0')
      y += umlclass->comment_font_height;

    list = g_list_next(list);
  }

  y = elem->corner.y + umlclass->namebox_height +
    umlclass->attributesbox_height + 0.1 + umlclass->font_height/2;
    
  list = umlclass->operations;
  while (list != NULL) {
    UMLOperation *op = (UMLOperation *)list->data;

    op->left_connection->pos.x = x;
    op->left_connection->pos.y = y;
    op->left_connection->directions = DIR_WEST;
    op->right_connection->pos.x = x + elem->width;
    op->right_connection->pos.y = y;
    op->right_connection->directions = DIR_EAST;

    y += umlclass->font_height;
    if (umlclass->visible_comments && op->comment != NULL && op->comment[0] != '\0')
      y += umlclass->comment_font_height;

    list = g_list_next(list);
  }
  
  element_update_boundingbox(elem);

  if (umlclass->template) {
    /* fix boundingumlclass for templates: */
    obj->bounding_box.top -= (umlclass->templates_height - 0.3) ;
    obj->bounding_box.right += (umlclass->templates_width - 2.3);
  }
  
  obj->position = elem->corner;

  element_update_handles(elem);
}

void
umlclass_calculate_data(UMLClass *umlclass)
{
  int i;
  GList *list;
  real maxwidth = 0.0;
  real width;
  int pos_next_comma, pos_brace, wrap_pos, last_wrap_pos, ident, offset, maxlinewidth, length;
  GList *sublist, *wrapsublist;
  

  if (umlclass->destroyed) return;
  
/*  umlclass->font_ascent =
          dia_font_ascent(umlclass->normal_font, umlclass->font_height);

          umlclass->abstract_font_ascent =
          dia_font_ascent(umlclass->abstract_font,
          umlclass->abstract_font_height);
*/
  /* name box: */

  if (umlclass->name != NULL && umlclass->name[0] != '\0') {
    if (umlclass->abstract) { 
      maxwidth = dia_font_string_width(umlclass->name,
                                       umlclass->abstract_classname_font,
                                       umlclass->abstract_classname_font_height);
    } else { 
      maxwidth = dia_font_string_width(umlclass->name,
                                       umlclass->classname_font,
                                       umlclass->classname_font_height);
    }
  }

  umlclass->namebox_height = umlclass->classname_font_height + 4*0.1;
  if (umlclass->stereotype_string != NULL) {
    g_free(umlclass->stereotype_string);
  }
  if (umlclass->stereotype != NULL && umlclass->stereotype[0] != '\0') {
    umlclass->namebox_height += umlclass->font_height;
    umlclass->stereotype_string = g_strconcat (
			UML_STEREOTYPE_START,
			umlclass->stereotype,
			UML_STEREOTYPE_END,
			NULL);

    width = dia_font_string_width (umlclass->stereotype_string,
                                   umlclass->normal_font,
                                   umlclass->font_height);
    maxwidth = MAX(width, maxwidth);
  } else {
    umlclass->stereotype_string = NULL;
  }

  if (umlclass->visible_comments && umlclass->comment != NULL && umlclass->comment[0] != '\0')
  {
       umlclass->namebox_height += umlclass->comment_font_height;
       width = dia_font_string_width (umlclass->comment,
                                      umlclass->comment_font,
                                      umlclass->comment_font_height);
       maxwidth = MAX(width, maxwidth);
  }

  /* attributes box: */
  if (umlclass->attributes_strings != NULL) {
    g_list_foreach(umlclass->attributes_strings, (GFunc)g_free, NULL);
    g_list_free(umlclass->attributes_strings);
  }
  umlclass->attributesbox_height = 2*0.1;

  umlclass->attributes_strings = NULL;
  if (g_list_length(umlclass->attributes) != 0) {
    i = 0;
    list = umlclass->attributes;
    while (list != NULL) {
      UMLAttribute *attr = (UMLAttribute *) list->data;
      gchar *attstr = uml_get_attribute_string(attr);

      umlclass->attributes_strings =
        g_list_append(umlclass->attributes_strings, attstr);

      if (attr->abstract) {
        width = dia_font_string_width(attstr,
                                      umlclass->abstract_font,
                                      umlclass->abstract_font_height);
        umlclass->attributesbox_height += umlclass->abstract_font_height;
      }
      else {
        width = dia_font_string_width(attstr,
                                      umlclass->normal_font,
                                      umlclass->font_height);
        umlclass->attributesbox_height += umlclass->font_height;
      }
      maxwidth = MAX(width, maxwidth);

      if (umlclass->visible_comments && attr->comment != NULL && attr->comment[0] != '\0') {
        width = dia_font_string_width(attr->comment,
                                      umlclass->comment_font,
                                      umlclass->comment_font_height);
        
        umlclass->attributesbox_height += umlclass->comment_font_height;

        maxwidth = MAX(width, maxwidth);
      }
      
      i++;
      list = g_list_next(list);
    }
  }

  if ((umlclass->attributesbox_height<0.4) ||
      umlclass->suppress_attributes )
          umlclass->attributesbox_height = 0.4;

  /* operations box: */
  umlclass->operationsbox_height = 2*0.1;
  /* neither leak previously calculated strings ... */
  if (umlclass->operations_strings != NULL) {
    g_list_foreach(umlclass->operations_strings, (GFunc)g_free, NULL);
    g_list_free(umlclass->operations_strings);
    umlclass->operations_strings = NULL;
  }
  /* ... nor their wrappings */
  if (umlclass->operations_wrappos != NULL) {
    g_list_foreach(umlclass->operations_wrappos, (GFunc)g_list_free, NULL);
    g_list_free(umlclass->operations_wrappos);
    umlclass->operations_wrappos = NULL;
  }

  if (0 != g_list_length(umlclass->operations)) {
    i = 0;
    list = umlclass->operations;
    while (list != NULL) {
      UMLOperation *op = (UMLOperation *) list->data;
      gchar *opstr = uml_get_operation_string(op);

      umlclass->operations_strings = 
        g_list_append(umlclass->operations_strings, opstr);
      
      length = 0;
      if( umlclass->wrap_operations ) {
      
        length = strlen( (const gchar*)opstr);
        
        sublist = NULL;
        if( length > umlclass->wrap_after_char) {
          gchar *part_opstr;

          sublist = g_list_append( sublist, GINT_TO_POINTER( 1));
          
          /* count maximal line width to create a secure buffer (part_opstr)
             and build the sublist with the wrapping data for the current operation, which will be used by umlclass_draw(), too. 
             The content of the sublist is:
             1st element: (bool) wrapping needed or not, 2nd: indentation in chars, 3rd-last: absolute wrapping positions */
          pos_next_comma = pos_brace = wrap_pos = offset = maxlinewidth = umlclass->max_wrapped_line_width = 0;
          while( wrap_pos + offset < length) {
            
            do {
              pos_next_comma = strcspn( (const gchar*)opstr + wrap_pos + offset, ",");
              wrap_pos += pos_next_comma + 1;
            } 
            while( wrap_pos < umlclass->wrap_after_char - pos_brace && wrap_pos + offset < length);
            
            if( offset == 0) {
              pos_brace = strcspn( opstr, "(");
              sublist = g_list_append( sublist, GINT_TO_POINTER( pos_brace+1));
            }
            sublist = g_list_append( sublist, GINT_TO_POINTER( wrap_pos + offset));
            
            maxlinewidth = MAX(maxlinewidth, wrap_pos);
 
            offset += wrap_pos;
            wrap_pos = 0;
          }   
          umlclass->max_wrapped_line_width = MAX( umlclass->max_wrapped_line_width, maxlinewidth+1);
          
          part_opstr = g_alloca(umlclass->max_wrapped_line_width);
          pos_next_comma = pos_brace = wrap_pos = offset = 0;
          
          wrapsublist = g_list_next( sublist);
          ident = GPOINTER_TO_INT( wrapsublist->data);
          wrapsublist = g_list_next( wrapsublist);
          wrap_pos = last_wrap_pos = 0;
          
          while( wrapsublist != NULL) {
            
            wrap_pos = GPOINTER_TO_INT( wrapsublist->data);
            
            if( last_wrap_pos == 0) {
              strncpy( part_opstr, opstr, wrap_pos);  
              memset( part_opstr+wrap_pos, '\0', 1);
            } else {  
              memset( part_opstr, ' ', ident);
              memset( part_opstr+ident, '\0', 1);
              strncat( part_opstr, opstr+last_wrap_pos, wrap_pos-last_wrap_pos);                
            }
            
            if (op->inheritance_type == UML_ABSTRACT) {
              width = dia_font_string_width(part_opstr,
                                            umlclass->abstract_font,
                                            umlclass->abstract_font_height);
              umlclass->operationsbox_height += umlclass->abstract_font_height;
            } else if (op->inheritance_type == UML_POLYMORPHIC) {
              width = dia_font_string_width(part_opstr,
                                            umlclass->polymorphic_font,
                                            umlclass->polymorphic_font_height);
              umlclass->operationsbox_height += umlclass->polymorphic_font_height;        
            } else {
              width = dia_font_string_width(part_opstr,
                                            umlclass->normal_font,
                                            umlclass->font_height);
              umlclass->operationsbox_height += umlclass->font_height;
            }
            
            maxwidth = MAX(width, maxwidth);
            last_wrap_pos = wrap_pos;
            wrapsublist = g_list_next( wrapsublist);
          }
  
        } else {
          
          sublist = g_list_append( sublist, GINT_TO_POINTER( 0));
        }
        umlclass->operations_wrappos = g_list_append( umlclass->operations_wrappos, sublist);
      } 
      
      if( !umlclass->wrap_operations || !(length > umlclass->wrap_after_char)) {

        if (op->inheritance_type == UML_ABSTRACT) {
          width = dia_font_string_width(opstr,
                                        umlclass->abstract_font,
                                        umlclass->abstract_font_height);
          umlclass->operationsbox_height += umlclass->abstract_font_height;
        } else if (op->inheritance_type == UML_POLYMORPHIC) {
          width = dia_font_string_width(opstr,
                                        umlclass->polymorphic_font,
                                        umlclass->polymorphic_font_height);
          umlclass->operationsbox_height += umlclass->polymorphic_font_height;        
        } else {
          width = dia_font_string_width(opstr,
                                        umlclass->normal_font,
                                        umlclass->font_height);
          umlclass->operationsbox_height += umlclass->font_height;
        }
          
        maxwidth = MAX(width, maxwidth);
      
      }
                  
      if (umlclass->visible_comments && op->comment != NULL && op->comment[0] != '\0') {
        width = dia_font_string_width(op->comment,
                                      umlclass->comment_font,
                                      umlclass->comment_font_height);

        umlclass->operationsbox_height += umlclass->comment_font_height;
        maxwidth = MAX(width, maxwidth);
      }

      i++;
      list = g_list_next(list);
    }
  }

  umlclass->element.width = maxwidth + 2*0.3;

  if ((umlclass->operationsbox_height<0.4) ||
      umlclass->suppress_operations )
          umlclass->operationsbox_height = 0.4;
  
  umlclass->element.height = umlclass->namebox_height;
  if (umlclass->visible_attributes)
    umlclass->element.height += umlclass->attributesbox_height;
  if (umlclass->visible_operations)
    umlclass->element.height += umlclass->operationsbox_height;

  /* templates box: */
  if (umlclass->templates_strings != NULL) {
    for (i=0;i<umlclass->num_templates;i++) {
      g_free(umlclass->templates_strings[i]);
    }
    g_free(umlclass->templates_strings);
  }
  umlclass->num_templates = g_list_length(umlclass->formal_params);

  umlclass->templates_height =
          umlclass->font_height * umlclass->num_templates + 2*0.1;
  umlclass->templates_height = MAX(umlclass->templates_height, 1.0);


  umlclass->templates_strings = NULL;
  maxwidth = 2.3;
  if (umlclass->num_templates != 0) {
    umlclass->templates_strings =
      g_malloc (sizeof (gchar *) * umlclass->num_templates);
    i = 0;
    list = umlclass->formal_params;
    while (list != NULL) {
      UMLFormalParameter *param;

      param = (UMLFormalParameter *) list->data;
      umlclass->templates_strings[i] = uml_get_formalparameter_string(param);
      
      width = dia_font_string_width(umlclass->templates_strings[i],
                                    umlclass->normal_font,
                                    umlclass->font_height);
      maxwidth = MAX(width, maxwidth);

      i++;
      list = g_list_next(list);
    }
  }
  umlclass->templates_width = maxwidth + 2*0.2;
}

static void
fill_in_fontdata(UMLClass *umlclass)
{
   if (umlclass->normal_font == NULL) {
     umlclass->font_height = 0.8;
     umlclass->normal_font = dia_font_new_from_style(DIA_FONT_MONOSPACE, 0.8);
   }
   if (umlclass->abstract_font == NULL) {
     umlclass->abstract_font_height = 0.8;
     umlclass->abstract_font = 
       dia_font_new_from_style(DIA_FONT_MONOSPACE | DIA_FONT_ITALIC | DIA_FONT_BOLD, 0.8);
   }
   if (umlclass->polymorphic_font == NULL) {
     umlclass->polymorphic_font_height = 0.8;
     umlclass->polymorphic_font = 
       dia_font_new_from_style(DIA_FONT_MONOSPACE | DIA_FONT_ITALIC, 0.8);
   }
   if (umlclass->classname_font == NULL) {
     umlclass->classname_font_height = 1.0;
     umlclass->classname_font = 
       dia_font_new_from_style(DIA_FONT_SANS | DIA_FONT_BOLD, 1.0);
   }
   if (umlclass->abstract_classname_font == NULL) {
     umlclass->abstract_classname_font_height = 1.0;
     umlclass->abstract_classname_font = 
       dia_font_new_from_style(DIA_FONT_SANS | DIA_FONT_BOLD | DIA_FONT_ITALIC, 1.0);
   }
   if (umlclass->comment_font == NULL) {
     umlclass->comment_font_height = 1.0;
     umlclass->comment_font = dia_font_new_from_style(DIA_FONT_SANS | DIA_FONT_ITALIC, 1.0);
   }
}

static DiaObject *
umlclass_create(Point *startpoint,
	       void *user_data,
  	       Handle **handle1,
	       Handle **handle2)
{
  UMLClass *umlclass;
  Element *elem;
  DiaObject *obj;
  int i;

  umlclass = g_malloc0(sizeof(UMLClass));
  elem = &umlclass->element;
  obj = &elem->object;
  
  obj->type = &umlclass_type;

  obj->ops = &umlclass_ops;

  elem->corner = *startpoint;

  element_init(elem, 8, UMLCLASS_CONNECTIONPOINTS); /* No attribs or ops => 0 extra connectionpoints. */

  umlclass->properties_dialog = NULL;
  fill_in_fontdata(umlclass);

  umlclass->name = g_strdup (_("Class"));
  umlclass->stereotype = NULL;
  umlclass->comment = NULL;

  umlclass->abstract = FALSE;

  umlclass->suppress_attributes = FALSE;
  umlclass->suppress_operations = FALSE;

  umlclass->visible_attributes = TRUE;
  umlclass->visible_operations = TRUE;
  umlclass->visible_comments = FALSE;

  umlclass->wrap_operations = TRUE;
  umlclass->wrap_after_char = UMLCLASS_WRAP_AFTER_CHAR;

  umlclass->attributes = NULL;

  umlclass->operations = NULL;
  
  umlclass->template = (GPOINTER_TO_INT(user_data)==1);
  umlclass->formal_params = NULL;
  
  umlclass->stereotype_string = NULL;
  umlclass->attributes_strings = NULL;
  umlclass->operations_strings = NULL;
  umlclass->operations_wrappos = NULL;
  umlclass->templates_strings = NULL;
  
  umlclass->text_color = color_black;
  umlclass->line_color = attributes_get_foreground();
  umlclass->fill_color = attributes_get_background();

  umlclass_calculate_data(umlclass);
  
  for (i=0;i<UMLCLASS_CONNECTIONPOINTS;i++) {
    obj->connections[i] = &umlclass->connections[i];
    umlclass->connections[i].object = obj;
    umlclass->connections[i].connected = NULL;
  }
  elem->extra_spacing.border_trans = UMLCLASS_BORDER/2.0;
  umlclass_update_data(umlclass);

  for (i=0;i<8;i++) {
    obj->handles[i]->type = HANDLE_NON_MOVABLE;
  }

  *handle1 = NULL;
  *handle2 = NULL;
  return &umlclass->element.object;
}

static void
umlclass_destroy(UMLClass *umlclass)
{
  int i;
  GList *list;
  UMLAttribute *attr;
  UMLOperation *op;
  UMLFormalParameter *param;

  umlclass->destroyed = TRUE;

  dia_font_unref(umlclass->normal_font);
  dia_font_unref(umlclass->abstract_font);
  dia_font_unref(umlclass->polymorphic_font);
  dia_font_unref(umlclass->classname_font);
  dia_font_unref(umlclass->abstract_classname_font);
  dia_font_unref(umlclass->comment_font);

  element_destroy(&umlclass->element);  
  
  g_free(umlclass->name);
  if (umlclass->stereotype != NULL)
    g_free(umlclass->stereotype);
  
  if (umlclass->comment != NULL)
    g_free(umlclass->comment);

  list = umlclass->attributes;
  while (list != NULL) {
    attr = (UMLAttribute *)list->data;
    g_free(attr->left_connection);
    g_free(attr->right_connection);
    uml_attribute_destroy(attr);
    list = g_list_next(list);
  }
  g_list_free(umlclass->attributes);
  
  list = umlclass->operations;
  while (list != NULL) {
    op = (UMLOperation *)list->data;
    g_free(op->left_connection);
    g_free(op->right_connection);
    uml_operation_destroy(op);
    list = g_list_next(list);
  }
  g_list_free(umlclass->operations);

  list = umlclass->formal_params;
  while (list != NULL) {
    param = (UMLFormalParameter *)list->data;
    uml_formalparameter_destroy(param);
    list = g_list_next(list);
  }
  g_list_free(umlclass->formal_params);

  if (umlclass->stereotype_string != NULL) {
    g_free(umlclass->stereotype_string);
  }

  if (umlclass->attributes_strings != NULL) {
    g_list_foreach(umlclass->attributes_strings, (GFunc)g_free, NULL);
    g_list_free(umlclass->attributes_strings);
    umlclass->attributes_strings = NULL;
  }

  if (umlclass->operations_strings != NULL) {
    g_list_foreach(umlclass->operations_strings, (GFunc)g_free, NULL);
    g_list_free(umlclass->operations_strings);
    umlclass->operations_strings = NULL;
  }

  if (umlclass->operations_wrappos != NULL) {
    g_list_foreach(umlclass->operations_wrappos, (GFunc)g_list_free, NULL);
    g_list_free(umlclass->operations_wrappos);
    umlclass->operations_wrappos = NULL;
  }

  if (umlclass->templates_strings != NULL) {
    for (i=0;i<umlclass->num_templates;i++) {
      g_free(umlclass->templates_strings[i]);
    }
    g_free(umlclass->templates_strings);
  }

  if (umlclass->properties_dialog != NULL) {
    g_list_free(umlclass->properties_dialog->deleted_connections);
    gtk_widget_destroy(umlclass->properties_dialog->dialog);
    g_free(umlclass->properties_dialog);
  }
}

static DiaObject *
umlclass_copy(UMLClass *umlclass)
{
  int i;
  UMLClass *newumlclass;
  Element *elem, *newelem;
  DiaObject *newobj;
  GList *list;
  UMLAttribute *attr;
  UMLAttribute *newattr;
  UMLOperation *op;
  UMLOperation *newop;
  UMLFormalParameter *param;
  
  elem = &umlclass->element;
  
  newumlclass = g_malloc0(sizeof(UMLClass));
  newelem = &newumlclass->element;
  newobj = &newelem->object;

  element_copy(elem, newelem);

  newumlclass->font_height = umlclass->font_height;
  newumlclass->abstract_font_height = umlclass->abstract_font_height;
  newumlclass->polymorphic_font_height = umlclass->polymorphic_font_height;
  newumlclass->classname_font_height = umlclass->classname_font_height;
  newumlclass->abstract_classname_font_height =
          umlclass->abstract_classname_font_height;
  newumlclass->comment_font_height =
          umlclass->comment_font_height;

  newumlclass->normal_font =
          dia_font_ref(umlclass->normal_font);
  newumlclass->abstract_font =
          dia_font_ref(umlclass->abstract_font);
  newumlclass->polymorphic_font =
          dia_font_ref(umlclass->polymorphic_font);
  newumlclass->classname_font =
          dia_font_ref(umlclass->classname_font);
  newumlclass->abstract_classname_font =
          dia_font_ref(umlclass->abstract_classname_font);
  newumlclass->comment_font =
          dia_font_ref(umlclass->comment_font);

  newumlclass->name = g_strdup(umlclass->name);
  if (umlclass->stereotype != NULL && umlclass->stereotype[0] != '\0')
    newumlclass->stereotype = g_strdup(umlclass->stereotype);
  else
    newumlclass->stereotype = NULL;

  if (umlclass->comment != NULL)
    newumlclass->comment = g_strdup(umlclass->comment);
  else
    newumlclass->comment = NULL;

  newumlclass->abstract = umlclass->abstract;
  newumlclass->suppress_attributes = umlclass->suppress_attributes;
  newumlclass->suppress_operations = umlclass->suppress_operations;
  newumlclass->visible_attributes = umlclass->visible_attributes;
  newumlclass->visible_operations = umlclass->visible_operations;
  newumlclass->visible_comments = umlclass->visible_comments;
  newumlclass->wrap_operations = umlclass->wrap_operations;
  newumlclass->wrap_after_char = umlclass->wrap_after_char;
  newumlclass->text_color = umlclass->text_color;
  newumlclass->line_color = umlclass->line_color;
  newumlclass->fill_color = umlclass->fill_color;

  newumlclass->attributes = NULL;
  list = umlclass->attributes;
  while (list != NULL) {
    attr = (UMLAttribute *)list->data;
    newattr = uml_attribute_copy(attr);
    
    newattr->left_connection = g_new0(ConnectionPoint,1);
    *newattr->left_connection = *attr->left_connection;
    newattr->left_connection->object = newobj;
    newattr->left_connection->connected = NULL;
    
    newattr->right_connection = g_new0(ConnectionPoint,1);
    *newattr->right_connection = *attr->right_connection;
    newattr->right_connection->object = newobj;
    newattr->right_connection->connected = NULL;
    
    newumlclass->attributes = g_list_prepend(newumlclass->attributes,
					     newattr);
    list = g_list_next(list);
  }

  newumlclass->operations = NULL;
  list = umlclass->operations;
  while (list != NULL) {
    op = (UMLOperation *)list->data;
    newop = uml_operation_copy(op);
    newop->left_connection = g_new0(ConnectionPoint,1);
    *newop->left_connection = *op->left_connection;
    newop->left_connection->object = newobj;
    newop->left_connection->connected = NULL;

    newop->right_connection = g_new0(ConnectionPoint,1);
    *newop->right_connection = *op->right_connection;
    newop->right_connection->object = newobj;
    newop->right_connection->connected = NULL;
    
    newumlclass->operations = g_list_prepend(newumlclass->operations,
					     newop);
    list = g_list_next(list);
  }

  newumlclass->template = umlclass->template;
  
  newumlclass->formal_params = NULL;
  list = umlclass->formal_params;
  while (list != NULL) {
    param = (UMLFormalParameter *)list->data;
    newumlclass->formal_params =
      g_list_prepend(newumlclass->formal_params,
		     uml_formalparameter_copy(param));
    list = g_list_next(list);
  }

  newumlclass->properties_dialog = NULL;
     
  newumlclass->stereotype_string = NULL;
  newumlclass->attributes_strings = NULL;
  newumlclass->operations_strings = NULL;
  newumlclass->operations_wrappos = NULL;
  newumlclass->templates_strings = NULL;

  for (i=0;i<UMLCLASS_CONNECTIONPOINTS;i++) {
    newobj->connections[i] = &newumlclass->connections[i];
    newumlclass->connections[i].object = newobj;
    newumlclass->connections[i].connected = NULL;
    newumlclass->connections[i].pos = umlclass->connections[i].pos;
    newumlclass->connections[i].last_pos = umlclass->connections[i].last_pos;
  }

  umlclass_calculate_data(newumlclass);

  i = UMLCLASS_CONNECTIONPOINTS;
  if ( (newumlclass->visible_attributes) &&
       (!newumlclass->suppress_attributes)) {
    list = newumlclass->attributes;
    while (list != NULL) {
      attr = (UMLAttribute *)list->data;
      newobj->connections[i++] = attr->left_connection;
      newobj->connections[i++] = attr->right_connection;
      
      list = g_list_next(list);
    }
  }
  
  if ( (newumlclass->visible_operations) &&
       (!newumlclass->suppress_operations)) {
    list = newumlclass->operations;
    while (list != NULL) {
      op = (UMLOperation *)list->data;
      newobj->connections[i++] = op->left_connection;
      newobj->connections[i++] = op->right_connection;
      
      list = g_list_next(list);
    }
  }

  umlclass_update_data(newumlclass);
  
  return &newumlclass->element.object;
}


static void
umlclass_save(UMLClass *umlclass, ObjectNode obj_node,
	      const char *filename)
{
  UMLAttribute *attr;
  UMLOperation *op;
  UMLFormalParameter *formal_param;
  GList *list;
  AttributeNode attr_node;
  
  element_save(&umlclass->element, obj_node);

  /* Class info: */
  data_add_string(new_attribute(obj_node, "name"),
		  umlclass->name);
  data_add_string(new_attribute(obj_node, "stereotype"),
		  umlclass->stereotype);
  data_add_string(new_attribute(obj_node, "comment"),
                  umlclass->comment);
  data_add_boolean(new_attribute(obj_node, "abstract"),
		   umlclass->abstract);
  data_add_boolean(new_attribute(obj_node, "suppress_attributes"),
		   umlclass->suppress_attributes);
  data_add_boolean(new_attribute(obj_node, "suppress_operations"),
		   umlclass->suppress_operations);
  data_add_boolean(new_attribute(obj_node, "visible_attributes"),
		   umlclass->visible_attributes);
  data_add_boolean(new_attribute(obj_node, "visible_operations"),
		   umlclass->visible_operations);
  data_add_boolean(new_attribute(obj_node, "visible_comments"),
		   umlclass->visible_comments);
  data_add_boolean(new_attribute(obj_node, "wrap_operations"),
		   umlclass->wrap_operations);
  data_add_int(new_attribute(obj_node, "wrap_after_char"),
		   umlclass->wrap_after_char);
  data_add_color(new_attribute(obj_node, "line_color"), 
		   &umlclass->line_color);
  data_add_color(new_attribute(obj_node, "fill_color"), 
		   &umlclass->fill_color);
  data_add_color(new_attribute(obj_node, "text_color"), 
		   &umlclass->text_color);
  data_add_font (new_attribute (obj_node, "normal_font"),
                 umlclass->normal_font);
  data_add_font (new_attribute (obj_node, "abstract_font"),
                 umlclass->abstract_font);
  data_add_font (new_attribute (obj_node, "polymorphic_font"),
                 umlclass->polymorphic_font);
  data_add_font (new_attribute (obj_node, "classname_font"),
                 umlclass->classname_font);
  data_add_font (new_attribute (obj_node, "abstract_classname_font"),
                 umlclass->abstract_classname_font);
  data_add_font (new_attribute (obj_node, "comment_font"),
                 umlclass->comment_font);
  data_add_real (new_attribute (obj_node, "font_height"),
                 umlclass->font_height);
  data_add_real (new_attribute (obj_node, "polymorphic_font_height"),
                 umlclass->polymorphic_font_height);
  data_add_real (new_attribute (obj_node, "abstract_font_height"),
                 umlclass->abstract_font_height);
  data_add_real (new_attribute (obj_node, "classname_font_height"),
                 umlclass->classname_font_height);
  data_add_real (new_attribute (obj_node, "abstract_classname_font_height"),
                 umlclass->abstract_classname_font_height);
  data_add_real (new_attribute (obj_node, "comment_font_height"),
                 umlclass->comment_font_height);

  /* Attribute info: */
  attr_node = new_attribute(obj_node, "attributes");
  list = umlclass->attributes;
  while (list != NULL) {
    attr = (UMLAttribute *) list->data;
    uml_attribute_write(attr_node, attr);
    list = g_list_next(list);
  }
  
  /* Operations info: */
  attr_node = new_attribute(obj_node, "operations");
  list = umlclass->operations;
  while (list != NULL) {
    op = (UMLOperation *) list->data;
    uml_operation_write(attr_node, op);
    list = g_list_next(list);
  }

  /* Template info: */
  data_add_boolean(new_attribute(obj_node, "template"),
		   umlclass->template);
  
  attr_node = new_attribute(obj_node, "templates");
  list = umlclass->formal_params;
  while (list != NULL) {
    formal_param = (UMLFormalParameter *) list->data;
    uml_formalparameter_write(attr_node, formal_param);
    list = g_list_next(list);
  }
}

static DiaObject *umlclass_load(ObjectNode obj_node, int version,
			     const char *filename)
{
  UMLClass *umlclass;
  Element *elem;
  DiaObject *obj;
  UMLAttribute *attr;
  UMLOperation *op;
  UMLFormalParameter *formal_param;
  AttributeNode attr_node;
  DataNode composite;
  int i;
  int num, num_attr, num_ops;
  GList *list;
  

  umlclass = g_malloc0(sizeof(UMLClass));
  elem = &umlclass->element;
  obj = &elem->object;
  
  obj->type = &umlclass_type;
  obj->ops = &umlclass_ops;

  element_load(elem, obj_node);

  /* kind of dirty, object_load_props() may leave us in an inconsistent state --hb */
  fill_in_fontdata(umlclass);
  
  object_load_props(obj,obj_node);
  /* a bunch of properties still need their own special handling */

  /* Class info: */
  umlclass->name = NULL;
  attr_node = object_find_attribute(obj_node, "name");
  if (attr_node != NULL)
    umlclass->name = data_string(attribute_first_data(attr_node));
  
  umlclass->stereotype = NULL;
  attr_node = object_find_attribute(obj_node, "stereotype");
  if (attr_node != NULL)
    umlclass->stereotype = data_string(attribute_first_data(attr_node));
  
  umlclass->comment = NULL;
  attr_node = object_find_attribute(obj_node, "comment");
  if (attr_node != NULL)
    umlclass->comment = data_string(attribute_first_data(attr_node));

  umlclass->abstract = FALSE;
  attr_node = object_find_attribute(obj_node, "abstract");
  if (attr_node != NULL)
    umlclass->abstract = data_boolean(attribute_first_data(attr_node));
  
  umlclass->suppress_attributes = FALSE;
  attr_node = object_find_attribute(obj_node, "suppress_attributes");
  if (attr_node != NULL)
    umlclass->suppress_attributes = data_boolean(attribute_first_data(attr_node));
  
  umlclass->suppress_operations = FALSE;
  attr_node = object_find_attribute(obj_node, "suppress_operations");
  if (attr_node != NULL)
    umlclass->suppress_operations = data_boolean(attribute_first_data(attr_node));
  
  umlclass->visible_attributes = FALSE;
  attr_node = object_find_attribute(obj_node, "visible_attributes");
  if (attr_node != NULL)
    umlclass->visible_attributes = data_boolean(attribute_first_data(attr_node));
  
  umlclass->visible_operations = FALSE;
  attr_node = object_find_attribute(obj_node, "visible_operations");
  if (attr_node != NULL)
    umlclass->visible_operations = data_boolean(attribute_first_data(attr_node));
  
  umlclass->visible_comments = FALSE;
  attr_node = object_find_attribute(obj_node, "visible_comments");
  if (attr_node != NULL)
    umlclass->visible_comments = data_boolean(attribute_first_data(attr_node));

  /* new since 0.94, don't wrap by default to keep old diagrams intact */
  umlclass->wrap_operations = FALSE;
  attr_node = object_find_attribute(obj_node, "wrap_operations");
  if (attr_node != NULL)
    umlclass->wrap_operations = data_boolean(attribute_first_data(attr_node));
  
  umlclass->wrap_after_char = UMLCLASS_WRAP_AFTER_CHAR;
  attr_node = object_find_attribute(obj_node, "wrap_after_char");
  if (attr_node != NULL)
    umlclass->wrap_after_char = data_int(attribute_first_data(attr_node));

  umlclass->line_color = color_black;
  /* support the old name ... */
  attr_node = object_find_attribute(obj_node, "foreground_color");
  if(attr_node != NULL)
    data_color(attribute_first_data(attr_node), &umlclass->line_color); 
  umlclass->text_color = umlclass->line_color;
  /* ... but prefer the new one */
  attr_node = object_find_attribute(obj_node, "line_color");
  if(attr_node != NULL)
    data_color(attribute_first_data(attr_node), &umlclass->line_color); 
  
  umlclass->fill_color = color_white;
  /* support the old name ... */
  attr_node = object_find_attribute(obj_node, "background_color");
  if(attr_node != NULL)
    data_color(attribute_first_data(attr_node), &umlclass->fill_color); 
  /* ... but prefer the new one */
  attr_node = object_find_attribute(obj_node, "fill_color");
  if(attr_node != NULL)
    data_color(attribute_first_data(attr_node), &umlclass->fill_color); 

  attr_node = object_find_attribute(obj_node, "text_color");
  if(attr_node != NULL)
    data_color(attribute_first_data(attr_node), &umlclass->text_color); 

  attr_node = object_find_attribute(obj_node, "normal_font");
  if (attr_node != NULL) {
    dia_font_unref(umlclass->normal_font);
    umlclass->normal_font = data_font(attribute_first_data(attr_node));
  }
  attr_node = object_find_attribute(obj_node, "abstract_font");
  if (attr_node != NULL) {
    dia_font_unref(umlclass->abstract_font);
    umlclass->abstract_font = data_font(attribute_first_data(attr_node));
  }
  attr_node = object_find_attribute(obj_node, "polymorphic_font");
  if (attr_node != NULL) {
    dia_font_unref(umlclass->polymorphic_font);
    umlclass->polymorphic_font = data_font(attribute_first_data(attr_node));
  }
  attr_node = object_find_attribute(obj_node, "classname_font");
  if (attr_node != NULL) {
    dia_font_unref(umlclass->classname_font);
    umlclass->classname_font = data_font(attribute_first_data(attr_node));
  }
  attr_node = object_find_attribute(obj_node, "abstract_classname_font");
  if (attr_node != NULL) {
    dia_font_unref(umlclass->abstract_classname_font);
    umlclass->abstract_classname_font = data_font(attribute_first_data(attr_node));
  }
  attr_node = object_find_attribute(obj_node, "comment_font");
  if (attr_node != NULL) {
    dia_font_unref(umlclass->comment_font);
    umlclass->comment_font = data_font(attribute_first_data(attr_node));
  }

  attr_node = object_find_attribute(obj_node, "font_height");
  if (attr_node != NULL) {
    umlclass->font_height = data_real(attribute_first_data(attr_node));
  }

  attr_node = object_find_attribute(obj_node, "polymorphic_font_height");
  if (attr_node != NULL) {
    umlclass->polymorphic_font_height = data_real(attribute_first_data(attr_node));
  }

  attr_node = object_find_attribute(obj_node, "abstract_font_height");
  if (attr_node != NULL) {
    umlclass->abstract_font_height = data_real(attribute_first_data(attr_node));
  }

  attr_node = object_find_attribute(obj_node, "classname_font_height");
  if (attr_node != NULL) {
    umlclass->classname_font_height = data_real(attribute_first_data(attr_node));
  }

  attr_node = object_find_attribute(obj_node, "abstract_classname_font_height");
  if (attr_node != NULL) {
    umlclass->abstract_classname_font_height = data_real(attribute_first_data(attr_node));
  }

  attr_node = object_find_attribute(obj_node, "comment_font_height");
  if (attr_node != NULL) {
    umlclass->comment_font_height = data_real(attribute_first_data(attr_node));
  }

 /* Attribute info: */
  attr_node = object_find_attribute(obj_node, "attributes");
  num_attr = num = attribute_num_data(attr_node);
  composite = attribute_first_data(attr_node);
  umlclass->attributes = NULL;
  for (i=0;i<num;i++) {
    attr = uml_attribute_read(composite);

    attr->left_connection = g_new0(ConnectionPoint,1);
    attr->left_connection->object = obj;
    attr->left_connection->connected = NULL;

    attr->right_connection = g_new0(ConnectionPoint,1);
    attr->right_connection->object = obj;
    attr->right_connection->connected = NULL;

    umlclass->attributes = g_list_append(umlclass->attributes, attr);
    composite = data_next(composite);
  }

  /* Operations info: */
  attr_node = object_find_attribute(obj_node, "operations");
  num_ops = num = attribute_num_data(attr_node);
  composite = attribute_first_data(attr_node);
  umlclass->operations = NULL;
  for (i=0;i<num;i++) {
    op = uml_operation_read(composite);

    op->left_connection = g_new0(ConnectionPoint,1);
    op->left_connection->object = obj;
    op->left_connection->connected = NULL;

    op->right_connection = g_new0(ConnectionPoint,1);
    op->right_connection->object = obj;
    op->right_connection->connected = NULL;
    
    umlclass->operations = g_list_append(umlclass->operations, op);
    composite = data_next(composite);
  }

  /* Template info: */
  umlclass->template = FALSE;
  attr_node = object_find_attribute(obj_node, "template");
  if (attr_node != NULL)
    umlclass->template = data_boolean(attribute_first_data(attr_node));

  attr_node = object_find_attribute(obj_node, "templates");
  num = attribute_num_data(attr_node);
  composite = attribute_first_data(attr_node);
  umlclass->formal_params = NULL;
  for (i=0;i<num;i++) {
    formal_param = uml_formalparameter_read(composite);
    umlclass->formal_params = g_list_append(umlclass->formal_params, formal_param);
    composite = data_next(composite);
  }

  if ( (!umlclass->visible_attributes) ||
       (umlclass->suppress_attributes))
    num_attr = 0;
  
  if ( (!umlclass->visible_operations) ||
       (umlclass->suppress_operations))
    num_ops = 0;
  
  element_init(elem, 8, UMLCLASS_CONNECTIONPOINTS + num_attr*2 + num_ops*2);

  umlclass->properties_dialog = NULL;
  fill_in_fontdata(umlclass);
  
  umlclass->stereotype_string = NULL;
  umlclass->attributes_strings = NULL;
  umlclass->operations_strings = NULL;
  umlclass->operations_wrappos = NULL;
  umlclass->templates_strings = NULL;

  umlclass_calculate_data(umlclass);
  
  for (i=0;i<UMLCLASS_CONNECTIONPOINTS;i++) {
    obj->connections[i] = &umlclass->connections[i];
    umlclass->connections[i].object = obj;
    umlclass->connections[i].connected = NULL;
  }

  i = UMLCLASS_CONNECTIONPOINTS;

  if ( (umlclass->visible_attributes) &&
       (!umlclass->suppress_attributes)) {
    list = umlclass->attributes;
    while (list != NULL) {
      attr = (UMLAttribute *)list->data;
      obj->connections[i++] = attr->left_connection;
      obj->connections[i++] = attr->right_connection;
      
      list = g_list_next(list);
    }
  }
  
  if ( (umlclass->visible_operations) &&
       (!umlclass->suppress_operations)) {
    list = umlclass->operations;
    while (list != NULL) {
      op = (UMLOperation *)list->data;
      obj->connections[i++] = op->left_connection;
      obj->connections[i++] = op->right_connection;
      
      list = g_list_next(list);
    }
  }
  elem->extra_spacing.border_trans = UMLCLASS_BORDER/2.0;
  umlclass_update_data(umlclass);

  
  for (i=0;i<8;i++) {
    obj->handles[i]->type = HANDLE_NON_MOVABLE;
  }

  return &umlclass->element.object;
}
