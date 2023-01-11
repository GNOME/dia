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
 * File:    table.c
 *
 * Purpose: This file contains implementation of the "table" code.
 */

/*
 * The code listed here is very similar to the one for the UML class
 * object ... at least in its core. Indeed, I've used it as a
 * template. -- pn
 */

#include "config.h"

#include <glib/gi18n-lib.h>

#include <string.h>
#include <ctype.h>

#include "database.h"
#include "propinternals.h"
#include "diarenderer.h"
#include "element.h"
#include "attributes.h"
#include "pixmaps/table.xpm"

#define TABLE_UNDERLINE_WIDTH 0.05
#define TABLE_COMMENT_MAXWIDTH 40
#define TABLE_ATTR_NAME_TYPE_GAP 0.5
#define TABLE_ATTR_NAME_OFFSET 0.3
#define TABLE_ATTR_COMMENT_OFFSET 0.25
#define TABLE_ATTR_INDIC_WIDTH 0.20
#define TABLE_ATTR_INDIC_LINE_WIDTH 0.01
#define TABLE_NORMAL_FONT_HEIGHT 0.8

/* ----------------------------------------------------------------------- */

static double           table_calculate_namebox_data    (Table             *);
static double           table_init_attributesbox_height (Table             *);
static DiaObject       *table_create                    (Point             *,
                                                         void              *,
                                                         Handle           **,
                                                         Handle           **);
static DiaObject       *table_load                      (ObjectNode         obj_node,
                                                         int                version,
                                                         DiaContext        *ctx);
static void             table_save                      (Table             *,
                                                         ObjectNode,
                                                         DiaContext        *ctx);
static void             table_destroy                   (Table             *);
static double           table_distance_from             (Table             *,
                                                         Point             *);
static void             table_select                    (Table             *,
                                                         Point             *,
                                                         DiaRenderer       *);
static DiaObject       *table_copy                      (Table             *table);
static DiaObjectChange *table_move                      (Table             *,
                                                         Point             *);
static DiaObjectChange *table_move_handle               (Table             *,
                                                         Handle            *,
                                                         Point             *,
                                                         ConnectionPoint   *,
                                                         HandleMoveReason,
                                                         ModifierKeys);
static PropDescription *table_describe_props            (Table             *);
static void             table_get_props                 (Table             *,
                                                         GPtrArray         *);
static void             table_set_props                 (Table             *,
                                                         GPtrArray         *);
static void             table_draw                      (Table             *,
                                                         DiaRenderer       *);
static double           table_draw_namebox              (Table             *,
                                                         DiaRenderer       *,
                                                         Element           *);
static double           table_draw_attributesbox        (Table             *,
                                                         DiaRenderer       *,
                                                         Element           *,
                                                         double);
static DiaMenu         *table_object_menu               (DiaObject         *,
                                                         Point             *);
static DiaObjectChange *table_show_comments_cb          (DiaObject         *,
                                                         Point             *,
                                                         gpointer);
static void             underline_table_attribute       (DiaRenderer       *,
                                                         Point,
                                                         TableAttribute    *,
                                                         Table             *);
static void             fill_diamond                    (DiaRenderer       *,
                                                         double,
                                                         double,
                                                         Point             *,
                                                         Color             *);
static void             table_init_fonts                (Table             *);

static TableAttribute *table_attribute_new (void);
static void table_attribute_free (TableAttribute *);
static TableAttribute *table_attribute_copy (TableAttribute *);
static void table_update_connectionpoints (Table *);
static void table_update_positions (Table *);
static void table_compute_width_height (Table *);
static TableState *table_state_new (Table *);
static DiaObjectChange *table_change_new                (Table             *,
                                                         TableState        *,
                                                         GList             *,
                                                         GList             *,
                                                         GList             *);
static void table_update_primary_key_font (Table *);

static char *create_documentation_tag (char     *comment,
                                       gboolean  tagging,
                                       int       WrapPoint,
                                       int      *NumberOfLines);
static void draw_comments (DiaRenderer *renderer,
                           DiaFont     *font,
                           double       font_height,
                           Color       *text_color,
                           char        *comment,
                           gboolean     comment_tagging,
                           int          Comment_line_length,
                           Point       *p,
                           int          alignment);

/* ----------------------------------------------------------------------- */

static ObjectTypeOps table_type_ops =
  {
    (CreateFunc) table_create,
    (LoadFunc) table_load,
    (SaveFunc) table_save
  };

DiaObjectType table_type =
  {
    "Database - Table", /* name */
    0,               /* version */
    table_xpm,        /* pixmap */
    &table_type_ops      /* ops */
  };

static ObjectOps table_ops =
  {
    (DestroyFunc)               table_destroy,
    (DrawFunc)                  table_draw,
    (DistanceFunc)              table_distance_from,
    (SelectFunc)                table_select,
    (CopyFunc)                  table_copy,
    (MoveFunc)                  table_move,
    (MoveHandleFunc)            table_move_handle,
    (GetPropertiesFunc) object_create_props_dialog,
    (ApplyPropertiesDialogFunc) object_apply_props_from_dialog,
    (ObjectMenuFunc)            table_object_menu,
    (DescribePropsFunc)         table_describe_props,
    (GetPropsFunc)              table_get_props,
    (SetPropsFunc)              table_set_props,
    (TextEditFunc) 0,
    (ApplyPropertiesListFunc) object_apply_props,
};

static PropDescription table_attribute_props[] =
  {
    { "name", PROP_TYPE_STRING, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
      N_("Name"), NULL, NULL },
    { "type", PROP_TYPE_STRING, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
      N_("Type"), NULL, NULL },
    { "comment", PROP_TYPE_STRING, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
      N_("Comment"), NULL, NULL },
    { "primary_key", PROP_TYPE_BOOL, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
      N_("Primary"), NULL, N_("Primary key") },
    { "nullable", PROP_TYPE_BOOL, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
      N_("Nullable"), NULL, NULL },
    { "unique", PROP_TYPE_BOOL, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
      N_("Unique"), NULL, NULL },
    { "default_value", PROP_TYPE_STRING, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
      N_("Default"), NULL, N_("Default value") },

    PROP_DESC_END
  };

static PropOffset table_attribute_offsets[] = {
  { "name", PROP_TYPE_STRING, offsetof(TableAttribute, name) },
  { "type", PROP_TYPE_STRING, offsetof(TableAttribute, type) },
  { "comment", PROP_TYPE_STRING, offsetof(TableAttribute, comment) },
  { "primary_key", PROP_TYPE_BOOL, offsetof(TableAttribute, primary_key) },
  { "nullable", PROP_TYPE_BOOL, offsetof(TableAttribute, nullable) },
  { "unique", PROP_TYPE_BOOL, offsetof(TableAttribute, unique) },
  { "default_value", PROP_TYPE_STRING, offsetof(TableAttribute, default_value) },

  { NULL, 0, 0 },
};

static PropDescDArrayExtra table_attribute_extra =
  {
    { table_attribute_props, table_attribute_offsets, "table_attribute" },
    (NewRecordFunc) table_attribute_new,
    (FreeRecordFunc) table_attribute_free
  };

static PropDescription table_props[] =
  {
    ELEMENT_COMMON_PROPERTIES,

    PROP_STD_NOTEBOOK_BEGIN,
    PROP_NOTEBOOK_PAGE("table", PROP_FLAG_DONT_MERGE, N_("Table")),
    { "name", PROP_TYPE_STRING, PROP_FLAG_VISIBLE,
      N_("Name"), NULL, NULL },
    { "comment", PROP_TYPE_STRING, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
      N_("Comment"), NULL, NULL },

    PROP_MULTICOL_BEGIN("visibilities"),
    PROP_MULTICOL_COLUMN("first"),
    { "visible_comment", PROP_TYPE_BOOL, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
      N_("Visible comments"), NULL, NULL },
    { "underline_primary_key", PROP_TYPE_BOOL, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
      N_("Underline primary keys"), NULL, NULL },
    PROP_MULTICOL_COLUMN("second"),
    { "tagging_comment", PROP_TYPE_BOOL, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
      N_("Comment tagging"), NULL, NULL },
    { "bold_primary_keys", PROP_TYPE_BOOL, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
      N_("Use bold font for primary keys"), NULL, NULL },
    PROP_MULTICOL_END("visibilities"),

    PROP_NOTEBOOK_PAGE("attribute", PROP_FLAG_DONT_MERGE | PROP_FLAG_NO_DEFAULTS, N_("Attributes")),
    { "attributes", PROP_TYPE_DARRAY, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL | PROP_FLAG_DONT_MERGE | PROP_FLAG_NO_DEFAULTS,
      "", NULL, &table_attribute_extra },

    PROP_NOTEBOOK_PAGE("style", PROP_FLAG_DONT_MERGE, N_("Style")),
    PROP_FRAME_BEGIN("fonts", 0, N_("Fonts")),
    PROP_MULTICOL_BEGIN("table"),
    PROP_MULTICOL_COLUMN("font"),
    { "normal_font", PROP_TYPE_FONT, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
      N_("Normal"), NULL, NULL },
    { "name_font", PROP_TYPE_FONT, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
      N_("Table name"), NULL, NULL },
    { "comment_font", PROP_TYPE_FONT, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
      N_("Comment"), NULL, NULL },
    PROP_MULTICOL_COLUMN("height"),
    { "normal_font_height", PROP_TYPE_REAL, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
      N_(" "), NULL, NULL },
    { "name_font_height", PROP_TYPE_REAL, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
      N_(" "), NULL, NULL },
    { "comment_font_height", PROP_TYPE_REAL, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
      N_(" "), NULL, NULL },
    PROP_MULTICOL_END("table"),
    PROP_FRAME_END("fonts", 0),

    PROP_STD_TEXT_COLOUR_OPTIONS(PROP_FLAG_VISIBLE | PROP_FLAG_STANDARD | PROP_FLAG_OPTIONAL),
    PROP_STD_LINE_COLOUR_OPTIONAL,
    PROP_STD_FILL_COLOUR_OPTIONAL,
    PROP_STD_LINE_WIDTH_OPTIONAL,

    PROP_STD_NOTEBOOK_END,

    PROP_DESC_END
  };

static PropOffset table_offsets[] = {
  ELEMENT_COMMON_PROPERTIES_OFFSETS,

  { "text_colour", PROP_TYPE_COLOUR, offsetof(Table, text_color) },
  { "line_colour", PROP_TYPE_COLOUR, offsetof(Table, line_color) },
  { "fill_colour", PROP_TYPE_COLOUR, offsetof(Table, fill_color) },
  { PROP_STDNAME_LINE_WIDTH, PROP_STDTYPE_LINE_WIDTH, offsetof(Table, border_width) },
  { "name", PROP_TYPE_STRING, offsetof(Table, name) },
  { "comment", PROP_TYPE_STRING, offsetof(Table, comment) },
  { "visible_comment", PROP_TYPE_BOOL, offsetof(Table, visible_comment) },
  { "tagging_comment", PROP_TYPE_BOOL, offsetof(Table, tagging_comment) },
  { "underline_primary_key", PROP_TYPE_BOOL, offsetof(Table, underline_primary_key) },
  { "bold_primary_keys", PROP_TYPE_BOOL, offsetof(Table, bold_primary_key) },

  PROP_OFFSET_MULTICOL_BEGIN("table"),
  PROP_OFFSET_MULTICOL_COLUMN("font"),
  { "normal_font", PROP_TYPE_FONT, offsetof(Table, normal_font) },
  { "name_font", PROP_TYPE_FONT, offsetof(Table, name_font) },
  { "comment_font", PROP_TYPE_FONT, offsetof(Table, comment_font) },

  PROP_OFFSET_MULTICOL_COLUMN("height"),
  { "normal_font_height", PROP_TYPE_REAL, offsetof(Table, normal_font_height) },
  { "name_font_height", PROP_TYPE_REAL, offsetof(Table, name_font_height) },
  { "comment_font_height", PROP_TYPE_REAL, offsetof(Table, comment_font_height) },
  PROP_OFFSET_MULTICOL_END("table"),

  { "attributes", PROP_TYPE_DARRAY, offsetof(Table, attributes) },

  { NULL, 0, 0 }
};

static DiaMenuItem table_menu_items[] = {
  { N_("Show comments"), table_show_comments_cb, NULL,
    DIAMENU_ACTIVE | DIAMENU_TOGGLE },
};

static DiaMenu table_menu = {
  N_("Table"),
  sizeof (table_menu_items)/sizeof (DiaMenuItem),
  table_menu_items,
  NULL
};

/* ----------------------------------------------------------------------- */

/**
 * Create a new TableAttribute
 */
static TableAttribute *
table_attribute_new (void)
{
  TableAttribute * attr;

  attr = g_new0 (TableAttribute, 1);
  if (attr != NULL)
    {
      attr->name = g_strdup ("");
      attr->type = g_strdup ("");
      attr->comment = g_strdup ("");
      attr->primary_key = FALSE;
      /* by default nullable */
      attr->nullable = TRUE;
      /* by default not unique */
      attr->unique = FALSE;

      /* empty default value by default */
      attr->default_value = g_strdup ("");

      attr->left_connection = NULL;
      attr->right_connection = NULL;
    }
  return attr;
}

static void
table_attribute_ensure_connection_points (TableAttribute * attr,
                                          DiaObject * obj)
{
  if (attr->left_connection == NULL)
    attr->left_connection = g_new0 (ConnectionPoint, 1);
  g_assert (attr->left_connection != NULL);
  attr->left_connection->object = obj;
  if (attr->right_connection == NULL)
    attr->right_connection = g_new0 (ConnectionPoint, 1);
  g_assert (attr->right_connection != NULL);
  attr->right_connection->object = obj;
}


/**
 * Free a TableAttribute and its allocated resources. Upon return of
 * this function the passed pointer will not be valid anymore.
 */
static void
table_attribute_free (TableAttribute * attr)
{
  g_clear_pointer (&attr->name, g_free);
  g_clear_pointer (&attr->type, g_free);
  g_clear_pointer (&attr->comment, g_free);

  /* do not free the connection points here as they may be shared */

  g_free (attr);
}


/**
 * Create a copy of the passed attribute. The returned copy of the
 * attribute needs to be freed using g_free when it is no longer needed.
 */
static TableAttribute *
table_attribute_copy (TableAttribute * orig)
{
  TableAttribute * copy;

  copy = g_new0 (TableAttribute, 1);
  copy->name = g_strdup (orig->name);
  copy->type = g_strdup (orig->type);
  copy->comment = g_strdup (orig->comment);
  copy->primary_key = orig->primary_key;
  copy->nullable = orig->nullable;
  copy->unique = orig->unique;
  copy->default_value = g_strdup (orig->default_value);

  return copy;
}

/* ----------------------------------------------------------------------- */

/**
 * Construct and initialize a new table object.
 */
static DiaObject *
table_create (Point * startpoint,
                void * user_data,
                Handle **handle1,
                Handle **handle2)
{
  Table *table;
  Element *elem;
  DiaObject *obj;
  gint i;

  table = g_new0 (Table, 1);
  elem = &table->element;
  obj = &elem->object;

  /* init data */
  table->name = g_strdup (_("Table"));
  table->comment = NULL;
  table->visible_comment = FALSE;
  table->tagging_comment = FALSE;
  table->underline_primary_key = TRUE;
  table->bold_primary_key = FALSE;
  table->attributes = NULL;

  /* init colors */
  table->text_color = attributes_get_foreground ();
  table->line_color = attributes_get_foreground ();
  table->fill_color = attributes_get_background ();

  table->border_width = attributes_get_default_linewidth ();

  /* init the fonts */
  table_init_fonts (table);

  /* now init inherited `element' */
  elem->corner = *startpoint;
  element_init (elem, 8, TABLE_CONNECTIONPOINTS);

  /* init the inherited `object' */
  obj->type = &table_type;
  obj->ops = &table_ops;

  /* init the connection points */
  for (i = 0; i < TABLE_CONNECTIONPOINTS; i++) {
    obj->connections[i] = &(table->connections[i]);
    table->connections[i].object = obj;
    table->connections[i].connected = NULL;
  }

  /* init the 8 handles placed around the table object */
  for (i = 0; i < 8; i++) {
    obj->handles[i]->type = HANDLE_NON_MOVABLE;
  }

  *handle1 = NULL;
  *handle2 = NULL;

  table_update_primary_key_font (table);
  table_compute_width_height (table);
  table_update_positions (table);

  return obj;
}

static DiaObject *
table_load (ObjectNode obj_node, int version, DiaContext *ctx)
{
  Table * table;
  Element * elem;
  DiaObject * obj;
  gint i;

  table = g_new0 (Table, 1);
  elem = &table->element;
  obj = &elem->object;

  obj->type = &table_type;
  obj->ops = &table_ops;

  element_load (elem, obj_node, ctx);
  element_init (elem, 8, TABLE_CONNECTIONPOINTS);

  object_load_props(obj,obj_node, ctx);

  /* fill in defaults if not given in the loaded file */
  if (object_find_attribute (obj_node, "line_colour") == NULL)
    table->line_color = attributes_get_foreground ();
  if (object_find_attribute (obj_node, "text_colour") == NULL)
    table->text_color = attributes_get_foreground ();
  if (object_find_attribute (obj_node, "fill_colour") == NULL)
    table->fill_color = attributes_get_background ();
  if (object_find_attribute (obj_node, PROP_STDNAME_LINE_WIDTH) == NULL)
    table->border_width = attributes_get_default_linewidth ();
  if (object_find_attribute (obj_node, "underline_primary_key") == NULL)
    table->underline_primary_key = TRUE;

  /* init the fonts */
  table_init_fonts (table);

  /* init the connection points */
  for (i = 0; i < TABLE_CONNECTIONPOINTS; i++) {
    obj->connections[i] = &(table->connections[i]);
    table->connections[i].object = obj;
    table->connections[i].connected = NULL;
  }

  /* init the 8 handles placed around the table object */
  for (i = 0; i < 8; i++) {
    obj->handles[i]->type = HANDLE_NON_MOVABLE;
  }

  table_update_primary_key_font (table);
  table_compute_width_height (table);
  table_update_positions (table);

  return &table->element.object;
}

static void
table_save (Table *table, ObjectNode obj_node, DiaContext *ctx)
{
  object_save_props (&table->element.object, obj_node, ctx);
}

static void
table_destroy (Table * table)
{
  GList * list;

  table->destroyed = TRUE;

  element_destroy (&table->element);

  g_clear_pointer (&table->name, g_free);
  g_clear_pointer (&table->comment, g_free);

  list = table->attributes;
  while (list != NULL) {
    TableAttribute * attr = (TableAttribute *) list->data;
    table_attribute_free (attr);

    list = g_list_next (list);
  }
  g_list_free (table->attributes);

  g_clear_object (&table->normal_font);
  g_clear_object (&table->primary_key_font);
  g_clear_object (&table->name_font);
  g_clear_object (&table->comment_font);
}

static void
table_draw (Table *table, DiaRenderer *renderer)
{
  real y = 0.0;
  Element * elem;

  dia_renderer_set_linewidth (renderer, table->border_width);
  dia_renderer_set_fillstyle (renderer, DIA_FILL_STYLE_SOLID);
  dia_renderer_set_linestyle (renderer, DIA_LINE_STYLE_SOLID, 0.0);

  elem = &table->element;

  y = table_draw_namebox (table, renderer, elem);
  table_draw_attributesbox (table, renderer, elem, y);
}

static real
table_draw_namebox (Table *table, DiaRenderer *renderer, Element *elem)
{
  Point startP;
  Point endP;

  /* upper left corner */
  startP.x = elem->corner.x;
  startP.y = elem->corner.y;
  /* lower right corner */
  endP.x = startP.x + elem->width;
  endP.y = startP.y + table->namebox_height;

  /* first draw the outer box and fill for the class name object */
  dia_renderer_draw_rect (renderer, &startP, &endP, &table->fill_color, &table->line_color);

  if (IS_NOT_EMPTY (table->name)) {
    startP.x += elem->width / 2.0;
    startP.y += table->name_font_height;
    dia_renderer_set_font (renderer,
                           table->name_font,
                           table->name_font_height);
    dia_renderer_draw_string (renderer,
                              table->name,
                              &startP,
                              DIA_ALIGN_CENTRE,
                              &table->text_color);
  }

  if (table->visible_comment && IS_NOT_EMPTY (table->comment)) {
    draw_comments (renderer,
                   table->comment_font,
                   table->comment_font_height,
                   &table->text_color,
                   table->comment,
                   table->tagging_comment,
                   TABLE_COMMENT_MAXWIDTH,
                   &startP,
                   DIA_ALIGN_CENTRE);
  }

  return endP.y;
}


/**
 *
 * pn: This function is "borrowed" from the source code for the UML
 * class object.
 *
 * Draw the comment at the point, p, using the comment font from the
 * class defined by umlclass. When complete update the point to reflect
 * the size of data drawn.
 * The comment will have been word wrapped using the function
 * create_documenattion_tag, so it may have more than one line on the
 * display.
 *
 * @param   renderer            The Renderer on which the comment is being drawn
 * @param   font                The font to render the comment in.
 * @param   font_height         The Y size of the font used to render the comment
 * @param   text_color          A pointer to the color to use to render the comment
 * @param   comment             The comment string to render
 * @param   comment_tagging     If the {documentation = } tag should be enforced
 * @param   Comment_line_length The maximum length of any one line in the comment
 * @param   p                   The point at which the comment is to start
 * @param   alignment           The method to use for alignment of the font
 * @see   uml_create_documentation
 */
static void
draw_comments (DiaRenderer *renderer,
               DiaFont     *font,
               real         font_height,
               Color       *text_color,
               char        *comment,
               gboolean     comment_tagging,
               gint         Comment_line_length,
               Point       *p,
               int          alignment)
{
  int   NumberOfLines = 0;
  int   Index;
  char *CommentString = 0;
  char *NewLineP= NULL;
  char *RenderP;

  CommentString =
    create_documentation_tag (comment, comment_tagging, Comment_line_length, &NumberOfLines);
  RenderP = CommentString;
  dia_renderer_set_font (renderer, font, font_height);
  for ( Index=0; Index < NumberOfLines; Index++) {
    p->y += font_height;                    /* Advance to the next line */
    NewLineP = strchr (RenderP, '\n');
    if ( NewLineP != NULL) {
      *NewLineP++ = '\0';
    }
    dia_renderer_draw_string (renderer, RenderP, p, alignment, text_color);
    RenderP = NewLineP;
    if ( NewLineP == NULL){
      break;
    }
  }
  g_clear_pointer (&CommentString, g_free);
}


static void
fill_diamond (DiaRenderer *renderer,
              real         half_height,
              real         width,
              Point       *lower_midpoint,
              Color       *color)
{
  Point poly[4];

  poly[0].x = lower_midpoint->x - width/2.0;
  poly[0].y = lower_midpoint->y;
  poly[1].x = lower_midpoint->x;
  poly[1].y = lower_midpoint->y + half_height;
  poly[2].x = lower_midpoint->x + width/2.0;
  poly[2].y = lower_midpoint->y;
  poly[3].x = poly[1].x;
  poly[3].y = lower_midpoint->y - half_height;

  dia_renderer_set_fillstyle (renderer, DIA_FILL_STYLE_SOLID);
  dia_renderer_set_linejoin (renderer, DIA_LINE_JOIN_MITER);

  dia_renderer_draw_polygon (renderer, poly, 4, color, NULL);
}

static real
table_draw_attributesbox (Table         *table,
                          DiaRenderer   *renderer,
                          Element       *elem,
                          real           Yoffset)
{
  Point startP, startTypeP;
  Point endP;
  Point indicP;
  GList * list;
  Color * text_color = &table->text_color;
  Color * fill_color = &table->fill_color;
  Color * line_color = &table->line_color;
  DiaFont * attr_font;
  real attr_font_height;
  real scale;

  startP.x = elem->corner.x;
  startP.y = Yoffset;

  endP.x = startP.x + elem->width;
  endP.y = startP.y + table->attributesbox_height;

  dia_renderer_draw_rect (renderer, &startP, &endP, fill_color, line_color);

  startP.x += TABLE_ATTR_NAME_OFFSET;
  startP.x += (table->border_width/2.0 + 0.1);

  list = table->attributes;
  /* adjust size of symbols with font height */
  scale = table->normal_font_height/TABLE_NORMAL_FONT_HEIGHT;
  while (list != NULL) {
    TableAttribute * attr = (TableAttribute *) list->data;

    if (attr->primary_key) {
      attr_font = table->primary_key_font;
      attr_font_height = table->primary_key_font_height;
    } else {
      attr_font = table->normal_font;
      attr_font_height = table->normal_font_height;
    }

    startP.y += attr_font_height;
    dia_renderer_set_font (renderer,
                           attr_font,
                           attr_font_height);

    dia_renderer_set_linewidth (renderer, TABLE_ATTR_INDIC_LINE_WIDTH);
    indicP = startP;
    indicP.x -= ((TABLE_ATTR_NAME_OFFSET/2.0)
                  + (TABLE_ATTR_INDIC_WIDTH*scale/4.0));
    indicP.y -= (attr_font_height/2.0);
    indicP.y += TABLE_ATTR_INDIC_WIDTH*scale/2.0;
    if (attr->primary_key) {
      fill_diamond (renderer,
                    0.75*TABLE_ATTR_INDIC_WIDTH*scale,
                    TABLE_ATTR_INDIC_WIDTH*scale,
                    &indicP,
                    &table->line_color);
    } else if (attr->nullable) {
      dia_renderer_draw_ellipse (renderer,
                                 &indicP,
                                 TABLE_ATTR_INDIC_WIDTH*scale,
                                 TABLE_ATTR_INDIC_WIDTH*scale,
                                 NULL,
                                 &table->line_color);
    } else {
      dia_renderer_draw_ellipse (renderer,
                                 &indicP,
                                 TABLE_ATTR_INDIC_WIDTH*scale,
                                 TABLE_ATTR_INDIC_WIDTH*scale,
                                 &table->line_color,
                                 NULL);
    }

    if (IS_NOT_EMPTY (attr->name)) {
      dia_renderer_draw_string (renderer,
                                attr->name,
                                &startP,
                                DIA_ALIGN_LEFT,
                                text_color);
    }

    if (IS_NOT_EMPTY (attr->type)) {
      startTypeP = startP;
      startTypeP.x += table->maxwidth_attr_name + TABLE_ATTR_NAME_TYPE_GAP;
      dia_renderer_draw_string (renderer,
                                attr->type,
                                &startTypeP,
                                DIA_ALIGN_LEFT,
                                text_color);
    }

    if (table->underline_primary_key && attr->primary_key)
      underline_table_attribute (renderer, startP, attr, table);

    if (table->visible_comment && IS_NOT_EMPTY (attr->comment)) {
      startP.x += TABLE_ATTR_COMMENT_OFFSET;
      draw_comments (renderer,
                     table->comment_font,
                     table->comment_font_height,
                     text_color,
                     attr->comment,
                     table->tagging_comment,
                     TABLE_COMMENT_MAXWIDTH,
                     &startP,
                     DIA_ALIGN_LEFT);
      startP.x -= TABLE_ATTR_COMMENT_OFFSET;
      startP.y += table->comment_font_height/2;
    }

    list = g_list_next (list);
  }

  return Yoffset;
}

static real
table_distance_from (Table * table, Point *point)
{
  const DiaRectangle * rect;
  DiaObject * obj;

  obj = &table->element.object;
  rect = dia_object_get_bounding_box (obj);
  return distance_rectangle_point (rect, point);
}

static void
table_select (Table * table, Point * clicked_point,
                DiaRenderer * interactive_renderer)
{
  element_update_handles (&table->element);
}

static DiaObject *
table_copy(Table * orig)
{
  Table * copy;
  Element * orig_elem, * copy_elem;
  DiaObject * copy_obj;
  GList * list;
  gint i;

  orig_elem = &orig->element;

  copy = g_new0 (Table, 1);
  copy_elem = &copy->element;
  copy_obj = &copy_elem->object;
  element_copy (orig_elem, copy_elem);

  for (i = 0; i < TABLE_CONNECTIONPOINTS; i++)
    {
      copy_obj->connections[i] = &copy->connections[i];
      copy->connections[i].object = copy_obj;
      copy->connections[i].connected = NULL;
      copy->connections[i].pos = orig->connections[i].pos;
    }

  copy->name = g_strdup (orig->name);
  copy->comment = g_strdup (orig->comment);
  copy->visible_comment = orig->visible_comment;
  copy->tagging_comment = orig->tagging_comment;
  copy->underline_primary_key = orig->underline_primary_key;
  copy->bold_primary_key = orig->bold_primary_key;
  list = orig->attributes;
  i = TABLE_CONNECTIONPOINTS;
  while (list != NULL)
    {
      TableAttribute * orig_attr = (TableAttribute *) list->data;
      TableAttribute * copy_attr = table_attribute_copy (orig_attr);
      table_attribute_ensure_connection_points (copy_attr, copy_obj);
      copy_obj->connections[i++] = copy_attr->left_connection;
      copy_obj->connections[i++] = copy_attr->right_connection;

      copy->attributes = g_list_append (copy->attributes, copy_attr);
      list = g_list_next (list);
    }
  copy->normal_font_height = orig->normal_font_height;
  copy->normal_font = g_object_ref (orig->normal_font);
  copy->name_font_height = orig->name_font_height;
  copy->name_font = g_object_ref (orig->name_font);
  copy->comment_font_height = orig->comment_font_height;
  copy->comment_font = g_object_ref (orig->comment_font);
  copy->text_color = orig->text_color;
  copy->line_color = orig->line_color;
  copy->fill_color = orig->fill_color;
  copy->border_width = orig->border_width;

  table_update_primary_key_font (copy);
  table_compute_width_height (copy);
  table_update_positions (copy);

  return &copy->element.object;
}


static DiaObjectChange *
table_move (Table *table, Point *to)
{
  table->element.corner = *to;
  table_update_positions (table);
  return NULL;
}


static DiaObjectChange *
table_move_handle (Table            *table,
                   Handle           *handle,
                   Point            *to,
                   ConnectionPoint  *cp,
                   HandleMoveReason  reason,
                   ModifierKeys      modifiers)
{
  /* ignore this event */
  return NULL;
}

static PropDescription *
table_describe_props (Table *table)
{
  if (table_props[0].quark == 0) {
    prop_desc_list_calculate_quarks (table_props);
  }

  return table_props;
}

static void
table_get_props (Table * table, GPtrArray *props)
{
  object_get_props_from_offsets (&table->element.object, table_offsets, props);
}

static void
table_set_props (Table *table, GPtrArray *props)
{
  object_set_props_from_offsets (&table->element.object, table_offsets, props);

  if (find_prop_by_name (props, "normal_font_height") != NULL)
    table->primary_key_font_height = table->normal_font_height;

  if (find_prop_by_name (props, "normal_font") != NULL)
    table_update_primary_key_font (table);

  /* the following routines depend on the fonts, and we can get called
   * here during load and the fonts are optional ...
   */
  if (table->normal_font && table->name_font && table->comment_font)
    {
      table_update_connectionpoints (table);
      table_compute_width_height (table);
      table_update_positions (table);
    }
}

/**
 * Init the height and width of the underlying element. This routine
 * uses `table_calculate_namebox_data' and
 * `table_init_attributesbox_height' which require the table's font
 * members to be initialized, so be sure to call this routine after the
 * font members were already initialized.
 */
static void
table_compute_width_height (Table * table)
{
  real width = 0.0;
  real maxwidth = 0.0;

  width = table_calculate_namebox_data (table);
  table->element.height = table->namebox_height;
  maxwidth = MAX(width, maxwidth);

  width = table_init_attributesbox_height (table);
  table->element.height += table->attributesbox_height;
  maxwidth = MAX(width, maxwidth);

  table->element.width = maxwidth + 0.5;
}

/**
 * Compute the height of the box surrounding the table's attributes,
 * store it in the passed table structure and return the width into
 * which all attributes of the table fit. While traversing the table's
 * attributes to compute the height, store also the width of the longest
 * attribute name in the passed table structure. This function makes use
 * of the fonts defined in the passed table structure, so be sure to
 * initialize them before calling this routine.
 */
static real
table_init_attributesbox_height (Table * table)
{
  real maxwidth_name = 0.0;
  real maxwidth_type = 0.0;
  real maxwidth_comment = 0.0;
  real width = 0.0;
  GList * list = table->attributes;
  DiaFont * comment_font = table->comment_font;
  real comment_font_height = table->comment_font_height;
  DiaFont * attr_font;
  real attr_font_height;

  /* height of an empty attributesbox */
  table->attributesbox_height = 2*0.1;

  while (list != NULL) {
    TableAttribute * attrib = (TableAttribute *) list->data;

    if (attrib->primary_key)
      {
        attr_font = table->primary_key_font;
        attr_font_height = table->primary_key_font_height;
      }
    else
      {
        attr_font = table->normal_font;
        attr_font_height = table->normal_font_height;
      }

    if (IS_NOT_EMPTY(attrib->name))
      {
        width = dia_font_string_width (attrib->name,
                                       attr_font, attr_font_height);
        maxwidth_name = MAX(maxwidth_name, width);
      }
    if (IS_NOT_EMPTY(attrib->type))
      {
        width = dia_font_string_width (attrib->type,
                                       attr_font, attr_font_height);
        maxwidth_type = MAX(maxwidth_type, width);
      }
    table->attributesbox_height += attr_font_height;

    if (table->visible_comment && IS_NOT_EMPTY(attrib->comment))
      {
        int num_of_lines = 0;
        char * cmt_str = create_documentation_tag (attrib->comment,
                                                   table->tagging_comment,
                                                   TABLE_COMMENT_MAXWIDTH,
                                                   &num_of_lines);
        width = dia_font_string_width (cmt_str,
                                       comment_font,
                                       comment_font_height);
        width += TABLE_ATTR_COMMENT_OFFSET;
        g_clear_pointer (&cmt_str, g_free);

        table->attributesbox_height += (comment_font_height * num_of_lines);
        table->attributesbox_height += comment_font_height / 2;
        maxwidth_comment = MAX(maxwidth_comment, width);
      }

    list = g_list_next (list);
  }
  table->maxwidth_attr_name = maxwidth_name;
  width = TABLE_ATTR_NAME_OFFSET
    + maxwidth_name
    + maxwidth_type
    + TABLE_ATTR_NAME_TYPE_GAP;
  return MAX(width, maxwidth_comment);
}

static void
underline_table_attribute (DiaRenderer     *renderer,
                           Point            StartPoint,
                           TableAttribute  *attr,
                           Table           *table)
{
  Point    UnderlineStartPoint;
  Point    UnderlineEndPoint;
  DiaFont * font;
  real font_height;

  if (attr->primary_key) {
    font = table->primary_key_font;
    font_height = table->primary_key_font_height;
  } else {
    font = table->normal_font;
    font_height = table->normal_font_height;
  }

  UnderlineStartPoint = StartPoint;
  UnderlineStartPoint.y += font_height * 0.1;
  UnderlineEndPoint = UnderlineStartPoint;
  UnderlineEndPoint.x += table->maxwidth_attr_name + TABLE_ATTR_NAME_TYPE_GAP;
  if (IS_NOT_EMPTY (attr->type)) {
    UnderlineEndPoint.x += dia_font_string_width (attr->type,
                                                  font,
                                                  font_height);
  }
  dia_renderer_set_linewidth (renderer, TABLE_UNDERLINE_WIDTH);
  dia_renderer_draw_line (renderer,
                          &UnderlineStartPoint,
                          &UnderlineEndPoint,
                          &table->text_color);
}

/**
 * pn: This function is "borrowed" from the source code for the UML
 * class object.
 *
 * Create a documentation tag from a comment.
 *
 * First a string is created containing only the text
 * "{documentation = ". Then the contents of the comment string
 * are added but wrapped. This is done by first looking for any
 * New Line characters. If the line segment is longer than the
 * WrapPoint would allow, the line is broken at either the
 * first whitespace before the WrapPoint or if there are no
 * whitespaces in the segment, at the WrapPoint.  This
 * continues until the entire string has been processed and
 * then the resulting new string is returned. No attempt is
 * made to rejoin any of the segments, that is all New Lines
 * are treated as hard newlines. No syllable matching is done
 * either so breaks in words will sometimes not make real
 * sense.
 * <p>
 * Finally, since this function returns newly created dynamic
 * memory the caller must free the memory to prevent memory
 * leaks.
 *
 * @param  comment       The comment to be wrapped to the line length limit
 * @param  WrapPoint     The maximum line length allowed for the line.
 * @param  NumberOfLines The number of comment lines after the wrapping.
 * @return               a pointer to the wrapped documentation
 *
 *  NOTE:
 *      This function should most likely be move to a source file for
 *      handling global UML functionallity at some point.
 */
static char *
create_documentation_tag (char     *comment,
                          gboolean  tagging,
                          int       WrapPoint,
                          int      *NumberOfLines)
{
  char *CommentTag = tagging ? "{documentation = " : "";
  int   TagLength  = strlen (CommentTag);
  /* Make sure that there is at least some value greater then zero for the WrapPoint to
   * support diagrams from earlier versions of Dia. So if the WrapPoint is zero then use
   * the taglength as the WrapPoint. If the Tag has been changed such that it has a length
   * of 0 then use 1.
   */
  int   WorkingWrapPoint = (TagLength<WrapPoint) ? WrapPoint : ((TagLength<=0)?1:TagLength);
  int   RawLength        = TagLength + strlen(comment) + (tagging?1:0);
  int   MaxCookedLength  = RawLength + RawLength/WorkingWrapPoint;
  char *WrappedComment   = g_new0 (char, MaxCookedLength + 1);
  int   AvailSpace       = WorkingWrapPoint - TagLength;
  char *Scan;
  char *BreakCandidate;
  gunichar ScanChar;
  gboolean AddNL            = FALSE;

  if (tagging)
    strcat(WrappedComment, CommentTag);
  *NumberOfLines = 1;

  while ( *comment ) {
    /* Skip spaces */
    while ( *comment && g_unichar_isspace(g_utf8_get_char(comment)) ) {
        comment = g_utf8_next_char(comment);
    }
    /* Copy chars */
    if ( *comment ){
      /* Scan to \n or avalable space exhausted */
      Scan = comment;
      BreakCandidate = NULL;
      while ( *Scan && *Scan != '\n' && AvailSpace > 0 ) {
        ScanChar = g_utf8_get_char(Scan);
        /* We known, that g_unichar_isspace() is not recommended for word breaking;
         * but Pango usage seems too complex.
         */
        if ( g_unichar_isspace(ScanChar) )
          BreakCandidate = Scan;
        AvailSpace--; /* not valid for nonspacing marks */
        Scan = g_utf8_next_char(Scan);
      }
      if ( AvailSpace==0 && BreakCandidate != NULL )
        Scan = BreakCandidate;
      if ( AddNL ){
        strcat(WrappedComment, "\n");
        *NumberOfLines+=1;
      }
      AddNL = TRUE;
      strncat(WrappedComment, comment, Scan-comment);
        AvailSpace = WorkingWrapPoint;
      comment = Scan;
    }
  }
  if (tagging)
    strcat(WrappedComment, "}");

  g_return_val_if_fail (strlen (WrappedComment) <= MaxCookedLength, NULL);

  return WrappedComment;
}


/**
 * Compute the dimension of the box surrounding the table's name and its
 * comment if any and if it is visible, store it (the height) in the
 * passed table structure and return the width of the namebox. This
 * function makes use of the fonts defined in the passed table
 * structure, so be sure to initialize them before calling this routine.
 */
static double
table_calculate_namebox_data (Table * table)
{
  double maxwidth = 0.0;

  if (IS_NOT_EMPTY (table->name)) {
    maxwidth = dia_font_string_width (table->name,
                                      table->name_font,
                                      table->name_font_height);
  }
  table->namebox_height = table->name_font_height + 2*0.1;

  if (table->visible_comment && IS_NOT_EMPTY (table->comment)) {
    double width;
    int numOfCommentLines = 0;
    char * wrapped_box = create_documentation_tag (table->comment,
                                                   table->tagging_comment,
                                                   TABLE_COMMENT_MAXWIDTH,
                                                   &numOfCommentLines);
    width = dia_font_string_width (wrapped_box,
                                   table->comment_font,
                                   table->comment_font_height);
    g_clear_pointer (&wrapped_box, g_free);
    table->namebox_height += table->comment_font_height * numOfCommentLines;
    maxwidth = MAX(width, maxwidth);
  }

  return maxwidth;
}


static void
table_update_positions (Table *table)
{
  ConnectionPoint * connections = table->connections;
  Element * elem = &table->element;
  GList * list;
  double x, y;
  real pointspacing;
  gint i;
  gint pointswide;
  gint southWestIndex;
  real attr_font_height;

  x = elem->corner.x;
  y = elem->corner.y;

  /* north-west */
  connpoint_update (&connections[0], x, y, DIR_NORTHWEST);

  /* dynamic number of connection points between north-west and north-east */
  pointswide = (TABLE_CONNECTIONPOINTS - 6) / 2;
  pointspacing = elem->width / (pointswide + 1.0);

  for (i = 1; i <= pointswide; i++) {
    connpoint_update (&connections[i], x + (pointspacing * i), y,
                      DIR_NORTH);
  }

  /* north-east */
  i = (TABLE_CONNECTIONPOINTS / 2) - 2;
  connpoint_update (&connections[i], x + elem->width, y,
                    DIR_NORTHEAST);

  /* west */
  i = (TABLE_CONNECTIONPOINTS / 2) - 1;
  connpoint_update (&connections[i], x, y + table->namebox_height / 2.0,
                    DIR_WEST);

  /* east */
  i = (TABLE_CONNECTIONPOINTS / 2);
  connpoint_update (&connections[i],
                    x + elem->width,
                    y + table->namebox_height / 2.0,
                    DIR_EAST);

  /* south west */
  southWestIndex = i = (TABLE_CONNECTIONPOINTS / 2) + 1;
  connpoint_update (&connections[i], x, y + elem->height,
                    DIR_SOUTHWEST);

  /* dynamic number of connection points between south-west and south-east */
  for (i = 1; i <= pointswide; i++) {
    connpoint_update (&connections[southWestIndex + i],
                      x + (pointspacing * i),
                      y + elem->height,
                      DIR_SOUTH);
  }

  /* south east */
  i = (TABLE_CONNECTIONPOINTS - 1);
  connpoint_update (&connections[i], x + elem->width, y + elem->height,
                    DIR_SOUTHEAST);

  y += table->namebox_height + 0.1 + table->normal_font_height/2;

  list = table->attributes;
  while (list != NULL)
    {
      TableAttribute * attr = (TableAttribute *) list->data;

      attr_font_height = (attr->primary_key == TRUE)
        ? table->primary_key_font_height
        : table->normal_font_height;

      if (attr->left_connection != NULL)
        connpoint_update (attr->left_connection, x, y, DIR_WEST);
      if (attr->right_connection != NULL)
        connpoint_update (attr->right_connection, x + elem->width, y, DIR_EAST);

      y += attr_font_height;

      if (table->visible_comment && IS_NOT_EMPTY (attr->comment)) {
        int num_of_lines = 0;
        char * str = create_documentation_tag (attr->comment,
                                               table->tagging_comment,
                                               TABLE_COMMENT_MAXWIDTH,
                                               &num_of_lines);
        y += table->comment_font_height * num_of_lines;
        y += table->comment_font_height / 2.0;
        g_clear_pointer (&str, g_free);
      }

      list = g_list_next (list);
    }

  element_update_boundingbox (elem);
  elem->object.position = elem->corner;
  element_update_handles (elem);
}

/**
 * Fix the object's connectionpoints array to be as large as neccessary to
 * hold the required number of connection points and (re)initialize it. This
 * routine reflects changes to the table->attributes list.
 */
void
table_update_connectionpoints (Table * table)
{
  DiaObject * obj;
  GList * list;
  gint index;
  gint num_connections, num_attrs;

  obj = &table->element.object;
  num_attrs = g_list_length (table->attributes);
  num_connections = TABLE_CONNECTIONPOINTS + 2*num_attrs;

  if (num_connections != obj->num_connections) {
    obj->num_connections = num_connections;
    obj->connections = g_renew (ConnectionPoint *,
                                obj->connections,
                                num_connections);
  }

  list = table->attributes;
  index = TABLE_CONNECTIONPOINTS;
  while (list != NULL)
    {
      TableAttribute * attr = (TableAttribute *) list->data;
      table_attribute_ensure_connection_points (attr, obj);
      obj->connections[index++] = attr->left_connection;
      obj->connections[index++] = attr->right_connection;
      list = g_list_next (list);
    }
}


static DiaMenu *
table_object_menu (DiaObject *obj, Point *p)
{
  table_menu_items[0].active = DIAMENU_ACTIVE|DIAMENU_TOGGLE|
    (((Table *)obj)->visible_comment ? DIAMENU_TOGGLE_ON : 0);

  return &table_menu;
}


static DiaObjectChange *
table_show_comments_cb (DiaObject *obj, Point *pos, gpointer data)
{
  TableState * state;
  Table * table = (Table *) obj;

  state = table_state_new (table);
  table->visible_comment = !table->visible_comment;
  table_compute_width_height (table);
  table_update_positions (table);

  return table_change_new (table, state, NULL, NULL, NULL);
}


/**
 * This routine updates the font for primary keys. It depends on
 * `normal_font' and `bold_primary_key' properties of the passed table.
 * This routine should be called when at least one of these properties
 * have been changed.
 */
static void
table_update_primary_key_font (Table * table)
{
  g_clear_object (&table->primary_key_font);

  if (!table->bold_primary_key
      || (DIA_FONT_STYLE_GET_WEIGHT (dia_font_get_style (table->normal_font))
          == DIA_FONT_BOLD)) {
    table->primary_key_font = g_object_ref (table->normal_font);
  } else {
    table->primary_key_font = dia_font_copy (table->normal_font);
    dia_font_set_weight (table->primary_key_font, DIA_FONT_BOLD);
  }

  table->primary_key_font_height = table->normal_font_height;
}

static void
table_init_fonts (Table * table)
{
  if (table->normal_font == NULL)
    {
      table->normal_font_height = TABLE_NORMAL_FONT_HEIGHT;
      table->normal_font = dia_font_new_from_style (DIA_FONT_MONOSPACE, TABLE_NORMAL_FONT_HEIGHT);
    }
  if (table->name_font == NULL)
    {
      table->name_font_height = 0.7;
      table->name_font =
        dia_font_new_from_style (DIA_FONT_SANS | DIA_FONT_BOLD, 0.7);
    }
  if (table->comment_font == NULL)
    {
      table->comment_font_height = 0.7;
      table->comment_font =
        dia_font_new_from_style (DIA_FONT_SANS | DIA_FONT_ITALIC, 0.7);
    }
}

/* TableState & TableChange functions ------------------------------------- */

TableState *
table_state_new (Table * table)
{
  TableState * state = g_new0 (TableState, 1);
  GList * list;

  state->name = g_strdup (table->name);
  state->comment = g_strdup (table->comment);
  state->visible_comment = table->visible_comment;
  state->tagging_comment = table->tagging_comment;
  state->underline_primary_key = table->underline_primary_key;
  state->bold_primary_key = table->bold_primary_key;
  state->border_width = table->border_width;

  list = table->attributes;
  while (list != NULL)
    {
      TableAttribute * attr = (TableAttribute *) list->data;
      TableAttribute * copy = table_attribute_copy (attr);

      copy->left_connection = attr->left_connection;
      copy->right_connection = attr->right_connection;

      state->attributes = g_list_append (state->attributes, copy);
      list = g_list_next (list);
    }

  return state;
}

/**
 * Set the values stored in state to the passed table and reinit the table
 * object. The table-state object will be free.
 */
static void
table_state_set (TableState * state, Table * table)
{
  table->name = state->name;
  table->comment = state->comment;
  table->visible_comment = state->visible_comment;
  table->tagging_comment = state->tagging_comment;
  table->border_width = state->border_width;
  table->underline_primary_key = state->underline_primary_key;
  table->bold_primary_key = state->bold_primary_key;
  table->border_width = state->border_width;
  table->attributes = state->attributes;

  g_free (state);

  table_update_connectionpoints (table);
  table_update_primary_key_font (table);
  table_compute_width_height (table);
  table_update_positions (table);
}

static void
table_state_free (TableState * state)
{
  GList * list;

  g_clear_pointer (&state->name, g_free);
  g_clear_pointer (&state->comment, g_free);

  list = state->attributes;
  while (list != NULL)
    {
      TableAttribute * attr = (TableAttribute *) list->data;
      table_attribute_free (attr);
      list = g_list_next (list);
    }
  g_list_free (state->attributes);

  g_free (state);
}


struct _DiaDBTableObjectChange {
  DiaObjectChange obj_change;

  Table *obj;

  GList *added_cp;
  GList *deleted_cp;
  GList *disconnected;

  int applied;

  TableState *saved_state;
};


DIA_DEFINE_OBJECT_CHANGE (DiaDBTableObjectChange, dia_db_table_object_change)


/**
 * Called to UNDO a change on the table object.
 */
static void
dia_db_table_object_change_revert (DiaObjectChange *self, DiaObject *obj)
{
  DiaDBTableObjectChange *change = DIA_DB_TABLE_OBJECT_CHANGE (self);
  TableState *old_state;
  GList *list;

  old_state = table_state_new (change->obj);

  table_state_set (change->saved_state, change->obj);

  list = change->disconnected;
  while (list) {
    Disconnect *dis = (Disconnect *)list->data;

    object_connect (dis->other_object, dis->other_handle, dis->cp);

    list = g_list_next (list);
  }

  change->saved_state = old_state;
  change->applied = FALSE;
}


static void
dia_db_table_object_change_free (DiaObjectChange *self)
{
  DiaDBTableObjectChange *change = DIA_DB_TABLE_OBJECT_CHANGE (self);
  GList * free_list, * lst;

  table_state_free (change->saved_state);

  free_list = (change->applied == TRUE)
    ? change->deleted_cp
    : change->added_cp;

  lst = free_list;
  while (lst) {
    ConnectionPoint * cp = (ConnectionPoint *) lst->data;
    g_assert (cp->connected == NULL);
    object_remove_connections_to (cp);
    g_clear_pointer (&cp, g_free);

    lst = g_list_next (lst);
  }
  g_list_free (free_list);
}


/**
 * Called to REDO a change on the table object.
 */
static void
dia_db_table_object_change_apply (DiaObjectChange *self, DiaObject *obj)
{
  DiaDBTableObjectChange *change = DIA_DB_TABLE_OBJECT_CHANGE (self);
  TableState *old_state;
  GList *lst;

  g_print ("apply (o: 0x%08x) (c: 0x%08x)\n",
           GPOINTER_TO_UINT (obj),
           GPOINTER_TO_UINT (change));

  /* first the get the current state for later use */
  old_state = table_state_new (change->obj);
  /* now apply the change */
  table_state_set (change->saved_state, change->obj);

  lst = change->disconnected;
  while (lst) {
    Disconnect * dis = (Disconnect *) lst->data;
    object_unconnect (dis->other_object, dis->other_handle);
    lst = g_list_next (lst);
  }

  change->saved_state = old_state;
  change->applied = TRUE;
}


static DiaObjectChange *
table_change_new (Table       *table,
                  TableState *saved_state,
                  GList      *added,
                  GList      *deleted,
                  GList      *disconnects)
{
  DiaDBTableObjectChange * change;

  change = dia_object_change_new (DIA_DB_TYPE_TABLE_OBJECT_CHANGE);

  change->obj = table;
  change->added_cp = added;
  change->deleted_cp = deleted;
  change->disconnected = disconnects;
  change->applied = TRUE;
  change->saved_state = saved_state;

  return DIA_OBJECT_CHANGE (change);
}
