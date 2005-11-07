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
 *
 * File:    class.c
 *
 * Purpose: This file contains implementation of the "class" code. 
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <gtk/gtk.h>
#include <math.h>
#include <string.h>
#include <ctype.h>

#include "intl.h"
#include "diarenderer.h"
#include "attributes.h"
#include "properties.h"
#include "diamenu.h"

#include "class.h"

#include "pixmaps/umlclass.xpm"

#include "debug.h"

#define UMLCLASS_BORDER 0.1
#define UMLCLASS_UNDERLINEWIDTH 0.05

static real umlclass_distance_from(UMLClass *umlclass, Point *point);
static void umlclass_select(UMLClass *umlclass, Point *clicked_point,
			    DiaRenderer *interactive_renderer);
static ObjectChange* umlclass_move_handle(UMLClass *umlclass, Handle *handle,
				 Point *to, ConnectionPoint *cp, HandleMoveReason reason, ModifierKeys modifiers);
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
static int umlclass_num_dynamic_connectionpoints(UMLClass *class);

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
  (ApplyPropertiesFunc) umlclass_apply_props_from_dialog,
  (ObjectMenuFunc)      umlclass_object_menu,
  (DescribePropsFunc)   umlclass_describe_props,
  (GetPropsFunc)        umlclass_get_props,
  (SetPropsFunc)        umlclass_set_props
};

extern PropDescDArrayExtra umlattribute_extra;
extern PropDescDArrayExtra umloperation_extra;
extern PropDescDArrayExtra umlparameter_extra;
extern PropDescDArrayExtra umlformalparameter_extra;

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
  { "template", PROP_TYPE_BOOL, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL| PROP_FLAG_NO_DEFAULTS,
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
  { "Comment_line_length", PROP_TYPE_INT, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
  N_("Comment line length"), NULL, NULL},

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

  /* these are used during load, but currently not during save */
  { "attributes", PROP_TYPE_DARRAY, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
  N_("Attributes"), NULL, NULL /* umlattribute_extra */ }, 
  { "operations", PROP_TYPE_DARRAY, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
  N_("Operations"), NULL, NULL /* umloperations_extra */ }, 
  /* the naming is questionable, but kept for compatibility */
  { "templates", PROP_TYPE_DARRAY, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
  N_("Template Parameters"), NULL, NULL /* umlformalparameters_extra */ }, 

  PROP_DESC_END
};

static PropDescription *
umlclass_describe_props(UMLClass *umlclass)
{
 if (umlclass_props[0].quark == 0) {
    int i = 0;

    prop_desc_list_calculate_quarks(umlclass_props);
    while (umlclass_props[i].name != NULL) {
      /* can't do this static, at least not on win32 
       * due to relocation (initializer not a constant)
       */
      if (0 == strcmp(umlclass_props[i].name, "attributes"))
        umlclass_props[i].extra_data = &umlattribute_extra;
      else if (0 == strcmp(umlclass_props[i].name, "operations")) {
        PropDescription *records = umloperation_extra.common.record;
        int j = 0;

        umlclass_props[i].extra_data = &umloperation_extra;
	while (records[j].name != NULL) {
          if (0 == strcmp(records[j].name, "parameters"))
	    records[j].extra_data = &umlparameter_extra;
	  j++;
	}
      }
      else if (0 == strcmp(umlclass_props[i].name, "templates"))
        umlclass_props[i].extra_data = &umlformalparameter_extra;

      i++;
    }
  }
  return umlclass_props;
}

static PropOffset umlclass_offsets[] = {
  ELEMENT_COMMON_PROPERTIES_OFFSETS,

  { "text_colour", PROP_TYPE_COLOUR, offsetof(UMLClass, text_color) },
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
  { "Comment_line_length", PROP_TYPE_INT, offsetof(UMLClass, Comment_line_length) },
  
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

  { "operations", PROP_TYPE_DARRAY, offsetof(UMLClass , operations) },
  { "attributes", PROP_TYPE_DARRAY, offsetof(UMLClass , attributes) } ,
  { "templates",  PROP_TYPE_DARRAY, offsetof(UMLClass , formal_params) } ,

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
  /* now that operations/attributes can be set here as well we need to 
   * take for the number of connections update as well
   * Note that due to a hack in umlclass_load, this is called before
   * the normal connection points are set up.
   */
  DiaObject *obj = &umlclass->element.object;
  GList *list;
  int num;

  object_set_props_from_offsets(&umlclass->element.object, umlclass_offsets,
                                props);

  num = UMLCLASS_CONNECTIONPOINTS + umlclass_num_dynamic_connectionpoints(umlclass);
  
#ifdef UML_MAINPOINT
  obj->num_connections = num + 1;
#else
  obj->num_connections = num;
#endif

  obj->connections =  g_realloc(obj->connections, obj->num_connections*sizeof(ConnectionPoint *));
  
  /* Update data: */
  if (num > UMLCLASS_CONNECTIONPOINTS) {
    int i;
    /* this is just updating pointers to ConnectionPoint, the real connection handling is elsewhere.
     * Note: Can't optimize here on number change cause the ops/attribs may have changed regardless of that.
     */
    i = UMLCLASS_CONNECTIONPOINTS;
    list = (!umlclass->visible_attributes || umlclass->suppress_attributes) ? NULL : umlclass->attributes;
    while (list != NULL) {
      UMLAttribute *attr = (UMLAttribute *)list->data;

      printf("Setting obj conn %d to %p->left: %p\n", i, attr, attr->left_connection);
      obj->connections[i] = attr->left_connection;
      obj->connections[i]->object = obj;
      i++;
      printf("Setting obj conn %d to %p->right: %p\n", i, attr, attr->right_connection);
      obj->connections[i] = attr->right_connection;
      obj->connections[i]->object = obj;
      i++;
      list = g_list_next(list);
    }
    list = (!umlclass->visible_operations || umlclass->suppress_operations) ? NULL : umlclass->operations;
    while (list != NULL) {
      UMLOperation *op = (UMLOperation *)list->data;
      obj->connections[i] = op->left_connection;
      obj->connections[i]->object = obj;
      i++;
      obj->connections[i] = op->right_connection;
      obj->connections[i]->object = obj;
      i++;
      list = g_list_next(list);
    }
  }
#ifdef UML_MAINPOINT
  obj->connections[num] = &umlclass->connections[UMLCLASS_CONNECTIONPOINTS];
  obj->connections[num]->object = obj;
#endif

  umlclass_calculate_data(umlclass);
  umlclass_update_data(umlclass);
  /* Would like to sanity check here, but the call to object_load_props
   * in umlclass_load means we will be called with inconsistent data. */
  umlclass_sanity_check(umlclass, "After updating data");
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
uml_underline_text(DiaRenderer  *renderer, 
               Point         StartPoint,
               DiaFont      *font,
               real          font_height,
               gchar        *attstr,
               Color        *color, 
               real          line_width,
               real          underline_width)
{
  DiaRendererClass *renderer_ops = DIA_RENDERER_GET_CLASS (renderer);
  Point    UnderlineStartPoint;
  Point    UnderlineEndPoint;

  UnderlineStartPoint = StartPoint;
  UnderlineStartPoint.y += font_height * 0.1;
  UnderlineEndPoint = UnderlineStartPoint;
  UnderlineEndPoint.x += dia_font_string_width(attstr, font, font_height);
  renderer_ops->set_linewidth(renderer, underline_width);
  renderer_ops->draw_line(renderer, &UnderlineStartPoint, &UnderlineEndPoint, color);
  renderer_ops->set_linewidth(renderer, line_width);
}

/*
 ** uml_create_documentation_tag
 *
 *  FILENAME: \dia\objects\UML\class.c
 *
 *  PARAMETERS: 
 *    comment -  The comment to be wrapped to the line length limit
 *    WrapPoint - The maximum line length allowed for the line.
 *    NumberOfLines - The number of comment lines after the wrapping.
 *
 *  DESCRIPTION:
 *          This function takes a string of characters and creates a
 *          documentation tagged string which is also a wrapped string
 *          where no line is longer than the value of WrapPoint. 
 *  
 *          First a string is created containing only the text
 *          "{documentation = ". Then the contents of the comment string
 *          are added but wrapped. This is done by first looking for any
 *          New Line characters. If the line segment is longer than the
 *          WrapPoint would allow, the line is broken at either the
 *          first whitespace before the WrapPoint or if there are no
 *          whitespaces in the segment, at the WrapPoint.  This
 *          continues until the entire string has been processed and
 *          then the resulting new string is returned. No attempt is
 *          made to rejoin any of the segments, that is all New Lines
 *          are treated as hard newlines. No syllable matching is done
 *          either so breaks in words will sometimes not make real
 *          sense. 
 *  
 *          Finally, since this function returns newly created dynamic
 *          memory the caller must free the memory to prevent memory
 *          leaks.
 *
 *  RETURNS:
 *      A pointer to the string containing the line breakpoints for 
 *      wrapping.
 *
 *  NOTE:
 *      This function should most likely be move to a source file for
 *      handling global UML functionallity at some point.
 */
static gchar *
uml_create_documentation_tag(gchar * comment,gint WrapPoint, gint *NumberOfLines)
{
  gchar  *CommentTag           = "{documentation = ";
  gint   TagLength             = strlen(CommentTag);
  gchar  *WrappedComment       = g_malloc(TagLength+1);
  gint   LengthOfComment       = strlen(comment);
  gint   CommentIndex          = 0;
  gint   LengthOfWrappedComment= 0;
  gint   LineLen    = WrapPoint - TagLength;

  WrappedComment[0] = '\0';
  strcat(WrappedComment, CommentTag);
  LengthOfWrappedComment = strlen(WrappedComment);
  *NumberOfLines = 1;

  /* Remove leading whitespace */
  while( isspace(comment[CommentIndex])){
    CommentIndex++;
  }


  while( CommentIndex < LengthOfComment) /* more of the comment to go? */
  {
    gchar *Nl         = strchr(&comment[CommentIndex], '\n');
    gint   BytesToNextNewLine = 0;

    /* if this is the first line then we have to take into 
     * account the tag of the tagged value
     */

    LengthOfWrappedComment = strlen(WrappedComment);
    /*
    * First handle the next new lines
    */
    if ( Nl != NULL){
      BytesToNextNewLine = (Nl - &comment[CommentIndex]);
    }

    if ((Nl != NULL) && (BytesToNextNewLine < LineLen)){
      LineLen = BytesToNextNewLine;
    }
    else{
		if( (CommentIndex + LineLen) > LengthOfComment){
			LineLen = LengthOfComment-CommentIndex;
		}
      while (LineLen > 0){
        if ((LineLen == strlen(&comment[CommentIndex])) ||
          isspace(comment[CommentIndex+LineLen])){
          break;
        } else{
          LineLen--;
        }
      }
      if ((*NumberOfLines > 1) &&( LineLen == 0)){
          LineLen = WrapPoint;
      }
    }
  if (LineLen < 0){
	  LineLen = 0;
  }

    /* Grow the wrapped text to make room for the NL and the next chunk */
    WrappedComment = g_realloc(WrappedComment,LengthOfWrappedComment+LineLen+2);
    memset(&WrappedComment[LengthOfWrappedComment],0,LineLen+2);
    strncat(WrappedComment, &comment[CommentIndex], LineLen);

    CommentIndex  += LineLen;
    while( isspace(comment[CommentIndex])){
      CommentIndex++;
    }
    if (CommentIndex < LengthOfComment){
     /* if this is not the last line add a new-line*/
      strcat(WrappedComment,"\n");
      *NumberOfLines+=1;
    }
    LengthOfWrappedComment = strlen(WrappedComment);
    LineLen    = WrapPoint;
  }
  WrappedComment = g_realloc(WrappedComment,LengthOfWrappedComment+2);
  strcat(WrappedComment, "}");
  return WrappedComment;
}

/*
 ** uml_draw_comments
 *
 *  FILENAME: \dia\objects\UML\class.c
 *
 *  PARAMETERS:
 *    renderer    - The Renderer on which the comment is being drawn.
 *    *font       - The font to render the comment in. 
 *    font_height - The Y size of the font used to render the comment 
 *    *text_color - A pointer to the color to use to render the comment 
 *    *comment    - The comment string to render
 *    Comment_line_length-The maximum length of any one line in the comment
 *    *p          - The point at which the comment is to start.  
 *    alignment   - The method to use for alignment of the font.
 *
 *  DESCRIPTION:
 *    Draw the comment at the point, p, using the comment font from the
 *    class defined by umlclass. When complete update the point to reflect
 *    the size of data drawn.
 *    The comment will have been word wrapped using the function
 *    uml_create_documentation_tag, so it may have more than one line on the
 *    display.
 *
 *  RETURNS:  void, No useful information is returned.
 *
 */

static void
uml_draw_comments(DiaRenderer *renderer, 
                  DiaFont     *font,
                  real         font_height,
                  Color       *text_color,
                  gchar       *comment,
                  gint         Comment_line_length, 
                  Point       *p, 
                  gint         alignment)
{
  gint      NumberOfLines = 0;
  gint      Index;
  gchar     *CommentString = 0;
  gchar     *NewLineP= NULL;
  gchar     *RenderP;
  
  DiaRendererClass *renderer_ops = DIA_RENDERER_GET_CLASS (renderer);
  
  CommentString = 
        uml_create_documentation_tag(comment, Comment_line_length, &NumberOfLines);
  RenderP = CommentString;                                                       
  renderer_ops->set_font(renderer, font, font_height);
  for ( Index=0; Index < NumberOfLines; Index++)
  {
    p->y += font_height;                    /* Advance to the next line */
    NewLineP = strchr(RenderP, '\n');
    if ( NewLineP != NULL)
    {
      *NewLineP++ = '\0';
    }
    renderer_ops->draw_string(renderer, RenderP, p, alignment, text_color);
    RenderP = NewLineP;
    if ( NewLineP == NULL){
        break;
    }
  }
  g_free(CommentString);
}


/*
 ** umlclass_draw_namebox
 *
 *  FILENAME: \dia\objects\UML\class.c
 *
 *  PARAMETERS:
 *          umlclass  - The pointer to the class being drawn
 *          renderer  - The pointer to the rendering object used to draw
 *          elem      - The pointer to the element within the class to be drawn
 *
 *  DESCRIPTION:
 *
 *  RETURNS:
 *          The offset from the start of the class to the bottom of the namebox
 *
 */
static real
umlclass_draw_namebox(UMLClass *umlclass, DiaRenderer *renderer, Element *elem )
{
  DiaRendererClass *renderer_ops = DIA_RENDERER_GET_CLASS (renderer);
  real     font_height;
  DiaFont *font;
  Point   StartPoint;
  Point   LowerRightPoint;
  real    Yoffset;
  Color   *text_color = &umlclass->text_color;
  
  StartPoint.x = elem->corner.x;
  StartPoint.y = elem->corner.y;
  Yoffset = elem->corner.y + umlclass->namebox_height;
  
  LowerRightPoint = StartPoint;
  LowerRightPoint.x += elem->width;
  LowerRightPoint.y  = Yoffset;
  
  /*
   * First draw the outer box and fill color for the class name
   * object
   */
  renderer_ops->fill_rect(renderer, &StartPoint, &LowerRightPoint, &umlclass->fill_color);
  renderer_ops->draw_rect(renderer, &StartPoint, &LowerRightPoint, &umlclass->line_color);

  /* Start at the midpoint on the X axis */
  StartPoint.x += elem->width / 2.0;

  /* stereotype: */
  if (umlclass->stereotype != NULL && umlclass->stereotype[0] != '\0') {
    gchar *String = umlclass->stereotype_string;
    StartPoint.y += 0.1;
    StartPoint.y += dia_font_ascent(String, umlclass->normal_font, umlclass->font_height);
    renderer_ops->set_font(renderer, umlclass->normal_font, umlclass->font_height);
    renderer_ops->draw_string(renderer,  String, &StartPoint, ALIGN_CENTER, text_color);
  }

  /* name: */
  if (umlclass->name != NULL) {
    if (umlclass->abstract) {
      font = umlclass->abstract_classname_font;
      font_height = umlclass->abstract_classname_font_height;
    } else {
      font = umlclass->classname_font;
      font_height = umlclass->classname_font_height;
    }
    StartPoint.y += font_height;

    renderer_ops->set_font(renderer, font, font_height);
    renderer_ops->draw_string(renderer, umlclass->name, &StartPoint, ALIGN_CENTER, text_color);
  }

  /* comment */
  if (umlclass->visible_comments && umlclass->comment != NULL && umlclass->comment[0] != '\0'){
    uml_draw_comments(renderer, umlclass->comment_font ,umlclass->comment_font_height, 
                           &umlclass->text_color, umlclass->comment, 
                           umlclass->Comment_line_length, &StartPoint, ALIGN_CENTER);
  }
  return Yoffset;
}

/*
 ** umlclass_draw_attributebox
 *
 *  FILENAME: \dia\objects\UML\class.c
 *
 *  PARAMETERS:
 *          umlclass  - The pointer to the class being drawn
 *          renderer  - The pointer to the rendering object used to draw
 *          elem      - The pointer to the element within the class to be drawn
 *          Yoffset   - The Y offset from the start of the class at which to draw 
 *                      the attributebox 
 *
 *  DESCRIPTION:
 *
 *  RETURNS:
 *          The offset from the start of the class to the bottom of the attributebox
 */
static real
umlclass_draw_attributebox(UMLClass *umlclass, DiaRenderer *renderer, Element *elem, real Yoffset)
{
  DiaRendererClass *renderer_ops = DIA_RENDERER_GET_CLASS (renderer);
  real     font_height;
  Point    StartPoint;
  Point    LowerRight;
  DiaFont *font;
  Color   *fill_color = &umlclass->fill_color;
  Color   *line_color = &umlclass->line_color;
  Color   *text_color = &umlclass->text_color;
  GList   *list;

  StartPoint.x = elem->corner.x;
  StartPoint.y = Yoffset;
  Yoffset   += umlclass->attributesbox_height;

  LowerRight   = StartPoint;
  LowerRight.x += elem->width;
  LowerRight.y = Yoffset;

  renderer_ops->fill_rect(renderer, &StartPoint, &LowerRight, fill_color);
  renderer_ops->draw_rect(renderer, &StartPoint, &LowerRight, line_color);

  if (!umlclass->suppress_attributes) {
    gint i = 0;
    StartPoint.x += (UMLCLASS_BORDER/2.0 + 0.1);
    StartPoint.y += 0.1;

    list = umlclass->attributes;
    while (list != NULL)
    {
      UMLAttribute *attr   = (UMLAttribute *)list->data;
      gchar        *attstr = g_list_nth(umlclass->attributes_strings, i)->data;

      if (attr->abstract)  {
        font = umlclass->abstract_font;
        font_height = umlclass->abstract_font_height;
      }
      else  {
        font = umlclass->normal_font;
        font_height = umlclass->font_height;
      }
      StartPoint.y += font_height;
      renderer_ops->set_font (renderer, font, font_height);
      renderer_ops->draw_string(renderer, attstr, &StartPoint, ALIGN_LEFT, text_color);

      if (attr->class_scope) {
        uml_underline_text(renderer, StartPoint, font, font_height, attstr, line_color, 
                        UMLCLASS_BORDER, UMLCLASS_UNDERLINEWIDTH );
      }

      if (umlclass->visible_comments && attr->comment != NULL && attr->comment[0] != '\0') {
        uml_draw_comments(renderer, umlclass->comment_font ,umlclass->comment_font_height, 
                               &umlclass->text_color, attr->comment, 
                               umlclass->Comment_line_length, &StartPoint, ALIGN_LEFT);
        StartPoint.y += umlclass->comment_font_height/2;
      }
      list = g_list_next(list);
      i++;
    }
  }
  return Yoffset;
}


/*
 ** umlclass_draw_operationbox
 *
 *  FILENAME: \dia\objects\UML\class.c
 *
 *  PARAMETERS:
 *          umlclass  - The pointer to the class being drawn
 *          renderer  - The pointer to the rendering object used to draw
 *          elem      - The pointer to the element within the class to be drawn
 *          Yoffset   - The Y offset from the start of the class at which to draw 
 *                      the operationbox 
 *
 *  DESCRIPTION:
 *
 *  RETURNS:
 *          The offset from the start of the class to the bottom of the operationbox
 *
 */
static real
umlclass_draw_operationbox(UMLClass *umlclass, DiaRenderer *renderer, Element *elem, real Yoffset)
{
  DiaRendererClass *renderer_ops = DIA_RENDERER_GET_CLASS (renderer);
  real     font_height;
  Point    StartPoint;
  Point    LowerRight;
  DiaFont *font;
  GList   *list;
  Color   *fill_color = &umlclass->fill_color;
  Color   *line_color = &umlclass->line_color;
  Color   *text_color = &umlclass->text_color;


  StartPoint.x = elem->corner.x;
  StartPoint.y = Yoffset;
  Yoffset   += umlclass->operationsbox_height;

  LowerRight   = StartPoint;
  LowerRight.x += elem->width;
  LowerRight.y = Yoffset;

  renderer_ops->fill_rect(renderer, &StartPoint, &LowerRight, fill_color);
  renderer_ops->draw_rect(renderer, &StartPoint, &LowerRight, line_color);

  if (!umlclass->suppress_operations) {
    gint i = 0;
    GList *wrapsublist = NULL;
    gchar *part_opstr = NULL;
    int wrap_pos, last_wrap_pos, ident, wrapping_needed;
    int part_opstr_len = 0, part_opstr_need = 0;

    StartPoint.x += (UMLCLASS_BORDER/2.0 + 0.1);
    StartPoint.y += 0.1;

    list = umlclass->operations;
    while (list != NULL) {
      UMLOperation *op = (UMLOperation *)list->data;
      gchar* opstr;
      real ascent;

      switch (op->inheritance_type) {
      case UML_ABSTRACT:
        font = umlclass->abstract_font;
        font_height = umlclass->abstract_font_height;
        break;
      case UML_POLYMORPHIC:
        font = umlclass->polymorphic_font;
        font_height = umlclass->polymorphic_font_height;
        break;
      case UML_LEAF:
      default:
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

        while( wrapsublist != NULL)   {
          wrap_pos = GPOINTER_TO_INT( wrapsublist->data);

          if( last_wrap_pos == 0)  {
            part_opstr_need = wrap_pos + 1;
            if (part_opstr_len < part_opstr_need) {
              part_opstr_len = part_opstr_need;
              part_opstr = g_realloc (part_opstr, part_opstr_need);
            }
            strncpy( part_opstr, opstr, wrap_pos);
            memset( part_opstr+wrap_pos, '\0', 1);
          }
          else   {
            part_opstr_need = ident + wrap_pos - last_wrap_pos + 1;
            if (part_opstr_len < part_opstr_need) {
              part_opstr_len = part_opstr_need;
              part_opstr = g_realloc (part_opstr, part_opstr_need);
            }
            memset( part_opstr, ' ', ident);
            memset( part_opstr+ident, '\0', 1);
            strncat( part_opstr, opstr+last_wrap_pos, wrap_pos-last_wrap_pos);
          }

          StartPoint.y += ascent;
          renderer_ops->draw_string(renderer, part_opstr, &StartPoint, ALIGN_LEFT, text_color);
          last_wrap_pos = wrap_pos;
          wrapsublist = g_list_next( wrapsublist);
        }
      }
      else
      {
        StartPoint.y += ascent;
        renderer_ops->draw_string(renderer, opstr, &StartPoint, ALIGN_LEFT, text_color);
      }

      if (op->class_scope) {
        uml_underline_text(renderer, StartPoint, font, font_height, opstr, line_color, 
                        UMLCLASS_BORDER, UMLCLASS_UNDERLINEWIDTH );
      }

      StartPoint.y += font_height - ascent;

      if (umlclass->visible_comments && op->comment != NULL && op->comment[0] != '\0'){
        uml_draw_comments(renderer, umlclass->comment_font ,umlclass->comment_font_height, 
                               &umlclass->text_color, op->comment, 
                               umlclass->Comment_line_length, &StartPoint, ALIGN_LEFT);
      }

      list = g_list_next(list);
      i++;
    }
    if (part_opstr){
      g_free(part_opstr);
    }
  }
  return Yoffset;
}

/*
 ** umlclass_draw_template_parameters_box
 *
 *  FILENAME: \dia\objects\UML\class.c
 *
 *  PARAMETERS:
 *          umlclass  - The pointer to the class being drawn
 *          renderer  - The pointer to the rendering object used to draw
 *          elem      - The pointer to the element within the class to be drawn
 *
 *  DESCRIPTION:
 *           
 *          This function draws the template parameters box in the upper
 *          right hand corner of the class box for paramertize classes
 *          (aka template classes). It then fills in this box with the
 *          parameters for the class. 
 *  
 *          At this time there is no provision for adding comments or
 *          documentation to the display.
 *
 *  RETURNS:
 *          void.
 *
 */
static void
umlclass_draw_template_parameters_box(UMLClass *umlclass, DiaRenderer *renderer, Element *elem)
{
  DiaRendererClass *renderer_ops = DIA_RENDERER_GET_CLASS (renderer);
  Point UpperLeft;
  Point LowerRight;
  Point TextInsert;
  GList *list;
  gint   i;
  DiaFont   *font = umlclass->normal_font;
  real       font_height = umlclass->font_height;
  Color     *fill_color = &umlclass->fill_color;
  Color     *line_color = &umlclass->line_color;
  Color     *text_color = &umlclass->text_color;

  UpperLeft.x = elem->corner.x + elem->width - 2.3;
  UpperLeft.y =  elem->corner.y - umlclass->templates_height + 0.3;
  TextInsert = UpperLeft;
  LowerRight = UpperLeft;
  LowerRight.x += umlclass->templates_width;
  LowerRight.y += umlclass->templates_height;

  renderer_ops->fill_rect(renderer, &UpperLeft, &LowerRight, fill_color);
  renderer_ops->set_linestyle(renderer, LINESTYLE_DASHED);
  renderer_ops->set_dashlength(renderer, 0.3);
  renderer_ops->draw_rect(renderer, &UpperLeft, &LowerRight, line_color);

  TextInsert.x += 0.3;
  renderer_ops->set_font(renderer, font, font_height);
  i = 0;
  list = umlclass->formal_params;
  while (list != NULL)
  {
    gchar *ParameterString = umlclass->templates_strings[i];
    
    TextInsert.y +=(0.1 + dia_font_ascent(ParameterString, font, font_height));
    renderer_ops->draw_string(renderer, ParameterString, &TextInsert, ALIGN_LEFT, text_color);

    list = g_list_next(list);
    i++;
  }
}

/*
 ** umlclass_draw
 *
 *  FILENAME: \dia\objects\UML\class.c
 *
 *  PARAMETERS:
 *
 *  DESCRIPTION:
 *
 *  Important Note from earlier contributer:
 *            Most of this crap could be rendered much more efficiently 
 *            (and probably much cleaner as well) using marked-up 
 *            Pango layout text. 
 *  RETURNS:
 *
 */
static void
umlclass_draw(UMLClass *umlclass, DiaRenderer *renderer)
{
  DiaRendererClass *renderer_ops = DIA_RENDERER_GET_CLASS (renderer);
  real     y  = 0.0;
  Element *elem;
  
  assert(umlclass != NULL);
  assert(renderer != NULL);

  renderer_ops->set_fillstyle(renderer, FILLSTYLE_SOLID);
  renderer_ops->set_linewidth(renderer, UMLCLASS_BORDER);
  renderer_ops->set_linestyle(renderer, LINESTYLE_SOLID);
  
  elem = &umlclass->element;

  y = umlclass_draw_namebox(umlclass, renderer, elem);
  if (umlclass->visible_attributes) {
    y = umlclass_draw_attributebox(umlclass, renderer, elem, y);
  }
  if (umlclass->visible_operations) {
    y = umlclass_draw_operationbox(umlclass, renderer, elem, y);
  }
  if (umlclass->template) {
    umlclass_draw_template_parameters_box(umlclass, renderer, elem);
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

#ifdef UML_MAINPOINT
  /* Main point -- lives just after fixed connpoints in umlclass array */
  i = UMLCLASS_CONNECTIONPOINTS;
  umlclass->connections[i].pos.x = x + elem->width / 2;
  umlclass->connections[i].pos.y = y + elem->height / 2;
  umlclass->connections[i].directions = DIR_ALL;  
  umlclass->connections[i].flags = CP_FLAGS_MAIN;
#endif

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

  
  obj->position = elem->corner;

  element_update_handles(elem);

  umlclass_sanity_check(umlclass, "After updating data");
}


/*
 *
 * umlclass_calculate_name_data
 *
 *  FILENAME: \dia\objects\UML\class.c
 *
 *  PARAMETERS:
 *     umlclass  - the class being rendered
 *
 *  DESCRIPTION:
 *      This function calculates the height of the class bounding box for
 *      the name and returns the width of that box. The height is stored
 *      in the class structure.
 *      When calculating the comment, if any, the comment is word wrapped
 *      and the resulting number of lines is then used to calculate the
 *      height of the bounding box.
 *
 *  RETURNS:
 *
 */


static real
umlclass_calculate_name_data(UMLClass *umlclass)
{
  real   maxwidth = 0.0;
  real   width = 0.0;
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
    umlclass->stereotype_string = g_strconcat ( UML_STEREOTYPE_START,
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
    int NumberOfCommentLines = 0;
    gchar *wrapped_box = uml_create_documentation_tag(umlclass->comment,
                                                             umlclass->Comment_line_length, 
                                                             &NumberOfCommentLines);

    width = dia_font_string_width (wrapped_box, 
                                   umlclass->comment_font,
                                   umlclass->comment_font_height);

    g_free(wrapped_box);
    umlclass->namebox_height += umlclass->comment_font_height * NumberOfCommentLines;
    maxwidth = MAX(width, maxwidth);
  }
  return maxwidth;
}

/*
 ** umlclass_calculate_attribute_data
 *
 *  FILENAME: \dia\objects\UML\class.c
 *
 *  PARAMETERS:
 *      umlclass  - The class to be drawn.
 *
 *  DESCRIPTION:
 *      Calculate the bounding box for the attributes. Include the
 *      comments if enabled and present.
 *
 *  RETURNS:
 *    The real width of the attribute bounding box.
 *
 */

static real
umlclass_calculate_attribute_data(UMLClass *umlclass) 
{
  int    i;
  int    pos_next_comma;
  int    pos_brace;
  int    wrap_pos;
  int    last_wrap_pos;
  int    maxlinewidth;
  int    length;
  real   maxwidth = 0.0;
  real   width    = 0.0;
  GList *list;

  /* attributes box: */
  if (umlclass->attributes_strings != NULL)
  {
    g_list_foreach(umlclass->attributes_strings, (GFunc)g_free, NULL);
    g_list_free(umlclass->attributes_strings);
  }
  umlclass->attributesbox_height = 2*0.1;

  umlclass->attributes_strings = NULL;
  if (g_list_length(umlclass->attributes) != 0)
  {
    i = 0;
    list = umlclass->attributes;
    while (list != NULL)
    {
      UMLAttribute *attr   = (UMLAttribute *) list->data;
      gchar        *attstr = uml_get_attribute_string(attr);

      umlclass->attributes_strings =
        g_list_append(umlclass->attributes_strings, attstr);

      if (attr->abstract)
      {
        width = dia_font_string_width(attstr,
                                      umlclass->abstract_font,
                                      umlclass->abstract_font_height);
        umlclass->attributesbox_height += umlclass->abstract_font_height;
      }
      else
      {
        width = dia_font_string_width(attstr,
                                      umlclass->normal_font,
                                      umlclass->font_height);
        umlclass->attributesbox_height += umlclass->font_height;
      }
      maxwidth = MAX(width, maxwidth);

      if (umlclass->visible_comments && attr->comment != NULL && attr->comment[0] != '\0')
      {
        int NumberOfLines = 0;
        gchar *Wrapped = uml_create_documentation_tag(attr->comment,
                                                              umlclass->Comment_line_length, 
                                                              &NumberOfLines);

        width = dia_font_string_width(Wrapped,
                                      umlclass->comment_font,
                                      umlclass->comment_font_height);

        g_free(Wrapped);
        umlclass->attributesbox_height += (umlclass->comment_font_height * (NumberOfLines));
        umlclass->attributesbox_height += umlclass->comment_font_height/2;

        maxwidth = MAX(width, maxwidth);
      }

      i++;
      list = g_list_next(list);
    }
  }

  if ((umlclass->attributesbox_height<0.4)|| umlclass->suppress_attributes )
  {
    umlclass->attributesbox_height = 0.4;
  }
  return maxwidth;
}


/*
 ** umlclass_calculate_operation_data
 *
 *  FILENAME: \dia\objects\UML\class.c
 *
 *  PARAMETERS:
 *      umlclass  - The class to be drawn.
 *
 *  DESCRIPTION:
 *      Calculate the bounding box for the operation. Include the
 *      comments if enabled and present.
 *
 *  RETURNS:
 *    The real width of the attribute bounding box.
 *
  */
static real
umlclass_calculate_operation_data(UMLClass *umlclass)
{
  int    i;
  int    pos_next_comma;
  int    pos_brace;
  int    wrap_pos;
  int    last_wrap_pos;
  int    ident;
  int    offset;
  int    maxlinewidth;
  int    length;
  real   maxwidth = 0.0;
  real   width    = 0.0;
  GList *list;
  GList *sublist;
  GList *wrapsublist;

  /* operations box: */
  umlclass->operationsbox_height = 2*0.1;
  /* neither leak previously calculated strings ... */
  if (umlclass->operations_strings != NULL)
  {
    g_list_foreach(umlclass->operations_strings, (GFunc)g_free, NULL);
    g_list_free(umlclass->operations_strings);
    umlclass->operations_strings = NULL;
  }
  /* ... nor their wrappings */
  if (umlclass->operations_wrappos != NULL)
  {
    g_list_foreach(umlclass->operations_wrappos, (GFunc)g_list_free, NULL);
    g_list_free(umlclass->operations_wrappos);
    umlclass->operations_wrappos = NULL;
  }

  if (0 != g_list_length(umlclass->operations))
  {
    i = 0;
    list = umlclass->operations;
    while (list != NULL)
    {
      UMLOperation *op = (UMLOperation *) list->data;
      gchar *opstr = uml_get_operation_string(op);

      umlclass->operations_strings =
        g_list_append(umlclass->operations_strings, opstr);

      length = 0;
      if( umlclass->wrap_operations )
      {
        length = strlen( (const gchar*)opstr);
        sublist = NULL;
        if( length > umlclass->wrap_after_char)
        {
          gchar *part_opstr;
          sublist = g_list_append( sublist, GINT_TO_POINTER( 1));

          /* count maximal line width to create a secure buffer (part_opstr)
          and build the sublist with the wrapping data for the current operation, which will be used by umlclass_draw(), too. 
          The content of the sublist is:
          1st element: (bool) wrapping needed or not, 2nd: indentation in chars, 3rd-last: absolute wrapping positions */
          pos_next_comma = pos_brace = wrap_pos = offset = maxlinewidth = umlclass->max_wrapped_line_width = 0;
          while( wrap_pos + offset < length)
          {
            do
            {
              pos_next_comma = strcspn( (const gchar*)opstr + wrap_pos + offset, ",");
              wrap_pos += pos_next_comma + 1;
            } while( wrap_pos < umlclass->wrap_after_char - pos_brace && wrap_pos + offset < length);

            if( offset == 0){
              pos_brace = strcspn( opstr, "(");
              sublist = g_list_append( sublist, GINT_TO_POINTER( pos_brace+1));
            }
            sublist = g_list_append( sublist, GINT_TO_POINTER( wrap_pos + offset));

            maxlinewidth = MAX(maxlinewidth, wrap_pos);

            offset += wrap_pos;
            wrap_pos = 0;
          }
          umlclass->max_wrapped_line_width = MAX( umlclass->max_wrapped_line_width, maxlinewidth+1);

          wrapsublist = g_list_next( sublist);
          ident = GPOINTER_TO_INT( wrapsublist->data);
          part_opstr = g_alloca(umlclass->max_wrapped_line_width+ident+1);
          pos_next_comma = pos_brace = wrap_pos = offset = 0;

          wrapsublist = g_list_next( wrapsublist);
          wrap_pos = last_wrap_pos = 0;

          while( wrapsublist != NULL){
            DiaFont   *Font;
            real       FontHeight;
            wrap_pos = GPOINTER_TO_INT( wrapsublist->data);
            if( last_wrap_pos == 0){
              strncpy( part_opstr, opstr, wrap_pos);
              memset( part_opstr+wrap_pos, '\0', 1);
            }
            else
            {
              memset( part_opstr, ' ', ident);
              memset( part_opstr+ident, '\0', 1);
              strncat( part_opstr, opstr+last_wrap_pos, wrap_pos-last_wrap_pos);
            }

            switch(op->inheritance_type)
            {
            case UML_ABSTRACT:
              Font       =  umlclass->abstract_font;
              FontHeight =  umlclass->abstract_font_height;
              break;
            case UML_POLYMORPHIC:
              Font       =  umlclass->polymorphic_font;
              FontHeight =  umlclass->polymorphic_font_height;
              break;
            case UML_LEAF:
            default:
              Font       = umlclass->normal_font;
              FontHeight = umlclass->font_height;
            }
            width = dia_font_string_width(part_opstr,Font,FontHeight);
            umlclass->operationsbox_height += FontHeight;

            maxwidth = MAX(width, maxwidth);
            last_wrap_pos = wrap_pos;
            wrapsublist = g_list_next( wrapsublist);
          }
        }
        else
        {
          sublist = g_list_append( sublist, GINT_TO_POINTER( 0));
        }
        umlclass->operations_wrappos = g_list_append( umlclass->operations_wrappos, sublist);
      }

      if( !umlclass->wrap_operations || !(length > umlclass->wrap_after_char)) {
        DiaFont   *Font;
        real       FontHeight;

        switch(op->inheritance_type)
        {
        case UML_ABSTRACT:
          Font       =  umlclass->abstract_font;
          FontHeight =  umlclass->abstract_font_height;
          break;
        case UML_POLYMORPHIC:
          Font       =  umlclass->polymorphic_font;
          FontHeight =  umlclass->polymorphic_font_height;
          break;
        case UML_LEAF:
        default:
          Font       = umlclass->normal_font;
          FontHeight = umlclass->font_height;
        }
        width = dia_font_string_width(opstr,Font,FontHeight);
        umlclass->operationsbox_height += FontHeight;

        maxwidth = MAX(width, maxwidth);
      }

      if (umlclass->visible_comments && op->comment != NULL && op->comment[0] != '\0'){
        int NumberOfLines = 0;
        gchar *Wrapped = uml_create_documentation_tag(op->comment,
                                                      umlclass->Comment_line_length, 
                                                      &NumberOfLines);

        width = dia_font_string_width(Wrapped,
                                      umlclass->comment_font,
                                      umlclass->comment_font_height);

        g_free(Wrapped);
        umlclass->operationsbox_height += (umlclass->comment_font_height * (NumberOfLines+1));

        maxwidth = MAX(width, maxwidth);
      }

      i++;
      list = g_list_next(list);
    }
  }

  umlclass->element.width = maxwidth + 2*0.3;

  if ((umlclass->operationsbox_height<0.4) || umlclass->suppress_operations ) {
    umlclass->operationsbox_height = 0.4;
  }

  return maxwidth;
}


/*
 ** umlclass_calculate_data
 *
 *  FILENAME: \dia\objects\UML\class.c
 *
 *  PARAMETERS:
 *      umlclass  - The class to be drawn.
 *
 *  DESCRIPTION:
 *      This function calculates the bounding box of the class image to be
 *      displayed. It also calculates the three containing boxes. This is
 *      done by calculating the size of the text to be displayed within
 *      each of the contained bounding boxes, name, attributes and
 *      operations.
 *      Because the comments may require wrapping, each comment is wrapped
 *      and the resulting number of lines is used to calculate the size of
 *      the comment within the box.
 *      The various font settings with in the class properties contribute
 *      to the overall size of the resulting bounding box.
 *
 *  RETURNS:
 *
 */
void
umlclass_calculate_data(UMLClass *umlclass)
{
  int    i;
  int    pos_next_comma;
  int    pos_brace;
  int    wrap_pos;
  int    last_wrap_pos;
  int    ident;
  int    offset;
  int    maxlinewidth;
  int    length;
  real   maxwidth = 0.0;
  real   width;
  GList *list;
  GList *sublist;
  GList *wrapsublist;


  if (!umlclass->destroyed)
  {
    maxwidth = MAX(umlclass_calculate_name_data(umlclass),      maxwidth);
    maxwidth = MAX(umlclass_calculate_attribute_data(umlclass), maxwidth);
    maxwidth = MAX(umlclass_calculate_operation_data(umlclass), maxwidth);

    umlclass->element.height = umlclass->namebox_height;
    umlclass->element.width  = maxwidth+0.5;

    if (umlclass->visible_attributes){
      umlclass->element.height += umlclass->attributesbox_height;
    }
    if (umlclass->visible_operations){
      umlclass->element.height += umlclass->operationsbox_height;
    }
    /* templates box: */
    if (umlclass->templates_strings != NULL)
    {
      for (i=0;i<umlclass->num_templates;i++)
      {
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
    if (umlclass->num_templates != 0)
    {
      umlclass->templates_strings =
        g_malloc (sizeof (gchar *) * umlclass->num_templates);
      i = 0;
      list = umlclass->formal_params;
      while (list != NULL)
      {
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
     umlclass->comment_font_height = 0.7;
     umlclass->comment_font = dia_font_new_from_style(DIA_FONT_SANS | DIA_FONT_ITALIC, 0.7);
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

#ifdef UML_MAINPOINT
  element_init(elem, 8, UMLCLASS_CONNECTIONPOINTS + 1); /* No attribs or ops => 0 extra connectionpoints. */
#else
  element_init(elem, 8, UMLCLASS_CONNECTIONPOINTS); /* No attribs or ops => 0 extra connectionpoints. */
#endif

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
#ifdef UML_MAINPOINT
  /* Put mainpoint at the end, after conditional attr/oprn points,
   * but store it in the local connectionpoint array. */
  i += umlclass_num_dynamic_connectionpoints(umlclass);
  obj->connections[i] = &umlclass->connections[UMLCLASS_CONNECTIONPOINTS];
  umlclass->connections[UMLCLASS_CONNECTIONPOINTS].object = obj;
  umlclass->connections[UMLCLASS_CONNECTIONPOINTS].connected = NULL;
#endif

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

  umlclass_sanity_check(umlclass, "Destroying");

  umlclass->destroyed = TRUE;

  dia_font_unref(umlclass->normal_font);
  dia_font_unref(umlclass->abstract_font);
  dia_font_unref(umlclass->polymorphic_font);
  dia_font_unref(umlclass->classname_font);
  dia_font_unref(umlclass->abstract_classname_font);
  dia_font_unref(umlclass->comment_font);

  element_destroy(&umlclass->element);  
  
  g_free(umlclass->name);
  g_free(umlclass->stereotype);
  g_free(umlclass->comment);

  list = umlclass->attributes;
  while (list != NULL) {
    attr = (UMLAttribute *)list->data;
    printf("Destroying attr %p\n", attr);
    uml_attribute_destroy(attr);
    list = g_list_next(list);
  }
  printf("Freeing umlclass->attributes %p\n", umlclass->attributes);
  g_list_free(umlclass->attributes);
  
  list = umlclass->operations;
  while (list != NULL) {
    op = (UMLOperation *)list->data;
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
  newumlclass->Comment_line_length = umlclass->Comment_line_length;
  newumlclass->text_color = umlclass->text_color;
  newumlclass->line_color = umlclass->line_color;
  newumlclass->fill_color = umlclass->fill_color;

  newumlclass->attributes = NULL;
  list = umlclass->attributes;
  while (list != NULL) {
    attr = (UMLAttribute *)list->data;
    newattr = uml_attribute_copy(attr, newobj);
    
    newumlclass->attributes = g_list_prepend(newumlclass->attributes,
					     newattr);
    list = g_list_next(list);
  }

  newumlclass->operations = NULL;
  list = umlclass->operations;
  while (list != NULL) {
    op = (UMLOperation *)list->data;
    newop = uml_operation_copy(op);
    newop->left_connection->object = newobj;
    newop->left_connection->connected = NULL;

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
      printf("Setting copy conn %d of %p to left %p\n", i, newobj, attr->left_connection);
      newobj->connections[i++] = attr->left_connection;
      printf("Setting copy conn %d of %p to right %p\n", i, newobj, attr->right_connection);
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

#ifdef UML_MAINPOINT
  newobj->connections[i] = &newumlclass->connections[UMLCLASS_CONNECTIONPOINTS];
  newumlclass->connections[UMLCLASS_CONNECTIONPOINTS].object = newobj;
  newumlclass->connections[UMLCLASS_CONNECTIONPOINTS].connected = NULL;
  newumlclass->connections[UMLCLASS_CONNECTIONPOINTS].pos = 
    umlclass->connections[UMLCLASS_CONNECTIONPOINTS].pos;
  newumlclass->connections[UMLCLASS_CONNECTIONPOINTS].last_pos =
    umlclass->connections[UMLCLASS_CONNECTIONPOINTS].last_pos;
  newumlclass->connections[UMLCLASS_CONNECTIONPOINTS].flags = 
    umlclass->connections[UMLCLASS_CONNECTIONPOINTS].flags;
  i++;
#endif

  umlclass_update_data(newumlclass);

  umlclass_sanity_check(newumlclass, "Copied");
  
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
  
  umlclass_sanity_check(umlclass, "Saving");

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
  data_add_int(new_attribute(obj_node, "Comment_line_length"),
                   umlclass->Comment_line_length);
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
  data_add_real (new_attribute (obj_node, "normal_font_height"),
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
    printf("Writing attr %p\n", attr);
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
  AttributeNode attr_node;
  int i;
  GList *list;
  

  umlclass = g_malloc0(sizeof(UMLClass));
  elem = &umlclass->element;
  obj = &elem->object;
  
  obj->type = &umlclass_type;
  obj->ops = &umlclass_ops;

  element_load(elem, obj_node);

#ifdef UML_MAINPOINT
  element_init(elem, 8, UMLCLASS_CONNECTIONPOINTS + 1);
#else
  element_init(elem, 8, UMLCLASS_CONNECTIONPOINTS);
#endif

  umlclass->properties_dialog = NULL;

  for (i=0;i<UMLCLASS_CONNECTIONPOINTS;i++) {
    obj->connections[i] = &umlclass->connections[i];
    umlclass->connections[i].object = obj;
    umlclass->connections[i].connected = NULL;
  }

  fill_in_fontdata(umlclass);
  
  /* kind of dirty, object_load_props() may leave us in an inconsnected = NULL;
  }

  fill_in_fontdata(umlclass);
  
  /* kind of dirty, object_load_props() may leave us in an inconsistent state --hb */
  object_load_props(obj,obj_node);
  /* a bunch of properties still need their own special handling */

  /* Class info: */

  /* parameters loaded via StdProp dont belong here anymore. In case of strings they 
   * will produce leaks. Otherwise the are just wasteing time (at runtime and while 
   * reading the code). Except maybe for some compatibility stuff. 
   * Although that *could* probably done via StdProp too.                      --hb
   */

  /* new since 0.94, don't wrap by default to keep old diagrams intact */
  umlclass->wrap_operations = FALSE;
  attr_node = object_find_attribute(obj_node, "wrap_operations");
  if (attr_node != NULL)
    umlclass->wrap_operations = data_boolean(attribute_first_data(attr_node));
  
  umlclass->wrap_after_char = UMLCLASS_WRAP_AFTER_CHAR;
  attr_node = object_find_attribute(obj_node, "wrap_after_char");
  if (attr_node != NULL)
    umlclass->wrap_after_char = data_int(attribute_first_data(attr_node));

  umlclass->Comment_line_length = UMLCLASS_COMMENT_LINE_LENGTH;
  attr_node = object_find_attribute(obj_node,"Comment_line_length");
  if (attr_node != NULL)
  {
    umlclass->Comment_line_length = data_int(attribute_first_data(attr_node));
  }
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

  /* Attribute info: */
  list = umlclass->attributes;
  while (list) {
    UMLAttribute *attr = list->data;
    g_assert(attr);

    attr->left_connection->object = obj;
    attr->left_connection->connected = NULL;
    attr->right_connection->object = obj;
    attr->right_connection->connected = NULL;
    list = g_list_next(list);
  }

  /* Operations info: */
  list = umlclass->operations;
  while (list) {
    UMLOperation *op = (UMLOperation *)list->data;
    g_assert(op);

    op->left_connection->object = obj;
    op->left_connection->connected = NULL;

    op->right_connection->object = obj;
    op->right_connection->connected = NULL;
    list = g_list_next(list);
  }

  /* Template info: */
  umlclass->template = FALSE;
  attr_node = object_find_attribute(obj_node, "template");
  if (attr_node != NULL)
    umlclass->template = data_boolean(attribute_first_data(attr_node));
  
  fill_in_fontdata(umlclass);
  
  umlclass->stereotype_string = NULL;
  umlclass->attributes_strings = NULL;
  umlclass->operations_strings = NULL;
  umlclass->operations_wrappos = NULL;
  umlclass->templates_strings = NULL;

  umlclass_calculate_data(umlclass);

  elem->extra_spacing.border_trans = UMLCLASS_BORDER/2.0;
  umlclass_update_data(umlclass);
  
  for (i=0;i<8;i++) {
    obj->handles[i]->type = HANDLE_NON_MOVABLE;
  }

  umlclass_sanity_check(umlclass, "Loaded class");

  return &umlclass->element.object;
}

/** Returns the number of connection points used by the attributes and
 * connections in the current state of the object. 
 */
static int
umlclass_num_dynamic_connectionpoints(UMLClass *umlclass) {
  int num = 0;
  if ( (umlclass->visible_attributes) &&
       (!umlclass->suppress_attributes)) {
    GList *list = umlclass->attributes;
    num += 2 * g_list_length(list);
  }
  
  if ( (umlclass->visible_operations) &&
       (!umlclass->suppress_operations)) {
    GList *list = umlclass->operations;
    num += 2 * g_list_length(list);
  }
  return num;
}

void
umlclass_sanity_check(UMLClass *c, gchar *msg)
{
#ifdef UML_MAINPOINT
  int num_fixed_connections = UMLCLASS_CONNECTIONPOINTS + 1;
#else
  int num_fixed_connections = UMLCLASS_CONNECTIONPOINTS;
#endif
  DiaObject *obj = (DiaObject*)c;
  GList *attrs, *ops;
  int i;

  dia_object_sanity_check((DiaObject *)c, msg);

  /* Check that num_connections is correct */
  dia_assert_true(num_fixed_connections + umlclass_num_dynamic_connectionpoints(c)
		  == obj->num_connections,
		  "%s: Class %p has %d connections, but %d fixed and %d dynamic\n",
		  msg, c, obj->num_connections, num_fixed_connections,
		  umlclass_num_dynamic_connectionpoints(c));
  
  for (i = 0; i < UMLCLASS_CONNECTIONPOINTS; i++) {
    dia_assert_true(&c->connections[i] == obj->connections[i],
		    "%s: Class %p connection mismatch at %d: %p != %p\n",
		    msg, c, i, &c->connections[i], obj->connections[i]);
  }

#ifdef UML_MAINPOINT
  dia_assert_true(&c->connections[i] ==
		  obj->connections[i + umlclass_num_dynamic_connectionpoints(c)],
		  "%s: Class %p mainpoint mismatch: %p != %p (at %d)\n",
		  msg, c, i, &c->connections[i],
		  obj->connections[i + umlclass_num_dynamic_connectionpoints(c)],
		  i + umlclass_num_dynamic_connectionpoints(c));
#endif

  /* Check that attributes are set up right. */
  i = 0;
  for (attrs = c->attributes; attrs != NULL; attrs = g_list_next(attrs)) {
    UMLAttribute *attr = (UMLAttribute *)attrs->data;
    int conn_offset = UMLCLASS_CONNECTIONPOINTS + 2 * i;
    dia_assert_true(attr->name != NULL,
		    "%s: %p attr %d has null name\n",
		    msg, c, i);
    dia_assert_true(attr->type != NULL,
		    "%s: %p attr %d has null type\n",
		    msg, c, i);
    dia_assert_true(attr->comment != NULL,
		    "%s: %p attr %d has null comment\n",
		    msg, c, i);
    
    dia_assert_true(attr->left_connection != NULL,
		    "%s: %p attr %d has null left connection\n",
		    msg, c, i);
    dia_assert_true(attr->left_connection == obj->connections[conn_offset],
		    "%s: %p attr %d left conn %p doesn't match obj conn %d: %p\n",
		    msg, c, i, attr->left_connection,
		    conn_offset, obj->connections[conn_offset]);
    dia_assert_true(attr->right_connection == obj->connections[conn_offset + 1],
		    "%s: %p attr %d right conn %p doesn't match obj conn %d: %p\n",
		    msg, c, i, attr->right_connection,
		    conn_offset + 1, obj->connections[conn_offset + 1]);
    dia_assert_true(attr->right_connection != NULL,
		    "%s: %p attr %d has null right connection\n",
		    msg, c, i);
    
    i++;
  }
  /* Check that operations are set up right. */
}
