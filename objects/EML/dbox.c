#include "dbox.h"

static EMLBox* emlbox_new(real , gchar *, gint , real , real , real,
                          ConnectionPoint *, ConnectionPoint *);
static EMLBox* listbox_new(real , gchar *, gint , real , real , real,
                           ConnectionPoint *, ConnectionPoint *);
static EMLBox* textbox_new(real , gchar *, gint , real , real , real,
                           ConnectionPoint *, ConnectionPoint *);
static real listbox_calc_connections(EMLBox *, Point *, GList **, real);
static real textbox_calc_connections(EMLBox *, Point *, GList **, real);
static real listbox_draw(EMLBox *, Renderer *, Point *, real);
static real textbox_draw(EMLBox *, Renderer *, Point *, real);
static void listbox_calc_geometry(EMLBox *, real *, real *);
static void textbox_calc_geometry(EMLBox *, real *, real *);
static void listbox_destroy(EMLBox *);
static void textbox_destroy(EMLBox *);
static void listbox_add_el(EMLBox *, gpointer);
static void textbox_add_el(EMLBox *, gpointer);

EMLBoxT EMLListBox = {
  &listbox_new,
  &listbox_destroy,
  &listbox_add_el,
  &listbox_calc_geometry,
  &listbox_calc_connections,
  &listbox_draw
};

EMLBoxT EMLTextBox = {
  &textbox_new,
  &textbox_destroy,
  &textbox_add_el,
  &textbox_calc_geometry,
  &textbox_calc_connections,
  &textbox_draw
};

static EMLBox*
emlbox_new(real font_height,
           gchar *font_name,
           gint text_alignment,
           real border_width,
           real separator_width,
           real separator_style,
           ConnectionPoint *left_connection,
           ConnectionPoint *right_connection)
{
  EMLBox *box = g_new(EMLBox, 1);
  box->font_height       = font_height;
  box->text_alignment    = text_alignment;
  box->border_width      = border_width;
  box->separator_width   = separator_width;
  box->separator_style   = separator_style;
  box->left_connection   = left_connection;
  box->right_connection  = right_connection;
  box->font              = 
    (font_name == NULL) ? NULL : font_getfont(font_name);
  box->els               = NULL;
  return box;
}

static EMLBox*
listbox_new(real font_height,
            gchar *font_name,
            gint text_alignment,
            real border_width,
            real separator_width,
            real separator_style,
            ConnectionPoint *left_connection,
            ConnectionPoint *right_connection)
{
  EMLBox *box;

  box = emlbox_new(font_height, font_name, text_alignment,
                   border_width, separator_width, separator_style,
                   left_connection, right_connection);

  box->ops = &EMLListBox;

  return box;
}

static EMLBox*
textbox_new(real font_height,
            gchar *font_name,
            gint text_alignment,
            real border_width,
            real separator_width,
            real separator_style,
            ConnectionPoint *left_connection,
            ConnectionPoint *right_connection)
{
  EMLBox *box;

  box = emlbox_new(font_height, font_name, text_alignment,
                   border_width, separator_width, separator_style,
                   left_connection, right_connection);

  box->ops = &EMLTextBox;

  return box;
}

void
emlbox_destroy(EMLBox *box)
{
  box->ops->destroy(box);
}


static void 
free_foreach(gpointer data, gpointer user_fun)
{
  if (user_fun != NULL) {
    ((void (*) (EMLBox*)) user_fun) ((EMLBox*) data);
  }
  else {
    g_free(data);
  }
}

static
void
listbox_destroy(EMLBox *box)
{
  g_list_foreach(box->els, free_foreach, emlbox_destroy);
  g_list_free(box->els);
  g_free(box);
}

static
void
textbox_destroy(EMLBox *box)
{
  g_list_foreach(box->els, free_foreach, NULL);
  g_list_free(box->els);
  g_free(box);
}

void
emlbox_add_el(EMLBox *box, gpointer data)
{
  box->ops->add_el(box, data);
}

static
void
listbox_add_el(EMLBox *box, gpointer data)
{
    box->els = g_list_append(box->els, data);
}

static
void
textbox_add_el(EMLBox *box, gpointer text)
{
  g_assert(box->els == NULL);
  box->els = g_list_append(box->els, (gpointer) text);
}

void
emlbox_calc_geometry(EMLBox *box, real *boxwidth, real *boxheight)
{
  box->ops->calc_geometry(box, boxwidth, boxheight);
}

static void
listbox_calc_geometry(EMLBox *box, real *boxwidth, real *boxheight)
{
  GList *list;
  real width, height;

  list = box->els;
  while (list != NULL) {

    width = height = 0;
    emlbox_calc_geometry((EMLBox*) list->data, &width, &height);

    *boxwidth = MAX(*boxwidth, width);
    *boxheight += height;

    list = g_list_next(list);
    if (list != NULL)
      *boxheight += box->separator_width + 0.1;
  }

}

static void
textbox_calc_geometry(EMLBox *box, real *boxwidth, real *boxheight)
{
  GList *list;
  gchar *text;

  list = box->els;
  
  if (list == NULL) {
    *boxwidth = *boxheight = 0;
  }
  else {
    text = (gchar*) list->data;
    *boxwidth = font_string_width(text, box->font, box->font_height);
    *boxheight = box->font_height + text_ascent(box->font, box->font_height);
  }

}

real
emlbox_calc_connections(EMLBox *box, Point *corner, GList **connections,
                        real width)
{
  return box->ops->calc_connections(box, corner, connections, width);
}

static
real
listbox_calc_connections(EMLBox *box, Point *corner,
                         GList **connections, real width)
{
  GList *list;
  EMLBox *el_box;
  Point pos;
  //ConnectionPoint *connection;
  real x, y;

  
  x = pos.x = corner->x;
  y = pos.y = corner->y;

  list = box->els;
  while (list != NULL) {

    el_box = (EMLBox*) list->data;
    pos.y +=
      el_box->ops->calc_connections(el_box, &pos, connections, width); 
    list = g_list_next(list);
    if (list != NULL)
      pos.y += box->separator_width + 0.1;
  }

  return (pos.y - corner->y);
}

static
real
textbox_calc_connections(EMLBox *box, Point *corner,
                         GList **connections, real width)
{
  real ascent;

  g_assert(box->left_connection != NULL);
  g_assert(box->right_connection != NULL);

  ascent = text_ascent(box->font, box->font_height);

  box->left_connection->pos.x = corner->x;
  box->left_connection->pos.y =
    corner->y + (ascent + box->font_height) / 2.0;
  *connections = g_list_append(*connections, box->left_connection);

  box->right_connection->pos.x = corner->x + width;
  box->right_connection->pos.y =
    corner->y + (ascent + box->font_height) / 2.0;
  *connections = g_list_append(*connections, box->right_connection);

  return (box->els == NULL) ? 0 : (ascent + box->font_height);
}

real
emlbox_draw(EMLBox *box, Renderer *renderer, Element *el)
{

  Point left, right;

  left.x = el->corner.x;
  left.y = el->corner.y;
  right.x = left.x + el->width;
  right.y = left.y + el->height;

  renderer->ops->set_fillstyle(renderer, FILLSTYLE_SOLID);
  renderer->ops->set_linewidth(renderer, box->border_width);
  renderer->ops->set_linestyle(renderer, LINESTYLE_SOLID);

  renderer->ops->fill_rect(renderer, &left, &right, &color_white);
  renderer->ops->draw_rect(renderer, &left, &right, &color_black);

  return box->ops->draw(box, renderer, &el->corner, el->width);
}

static
real
listbox_draw(EMLBox *box, Renderer *renderer, Point *corner , real width)
{
  GList *list;
  EMLBox *el_box;
  Point pos, left, right;
  
  pos.x = corner->x;
  pos.y = corner->y;

  list = box->els;
  while (list != NULL) {

    el_box = (EMLBox*) list->data;
    pos.y +=
      el_box->ops->draw(el_box, renderer, &pos, width); 
    list = g_list_next(list);

    if (list != NULL) {

        left.y = right.y = pos.y + 0.1;
        left.x = pos.x;
        right.x = left.x + width;

        if (box->separator_width != 0.0) {
            renderer->ops->set_linewidth(renderer, box->separator_width);
            renderer->ops->set_linestyle(renderer, box->separator_style);
            renderer->ops->draw_line(renderer, &left, &right,  &color_black);
        }

        pos.y += box->separator_width + 0.1;
    }
  }
  return (pos.y - corner->y);
}

static
real
textbox_draw(EMLBox *box, Renderer *renderer, Point *corner , real boxwidth)
{
  Point left;
  gchar *text;
  real ascent;

  ascent = text_ascent(box->font, box->font_height);

  if (box->els != NULL) {

    text = (gchar*) box->els->data;

    switch (box->text_alignment) {
    case ALIGN_CENTER:
        left.x = corner->x + boxwidth / 2.0 + 0.1;
        break;
    case ALIGN_RIGHT:
        left.x = corner->x + boxwidth - 0.2;
        break;
    case ALIGN_LEFT:
    default:
        left.x = corner->x + 0.2;
    }
    
    left.y = corner->y + ascent + box->font_height / 2.0;

    renderer->ops->set_font(renderer, box->font, box->font_height);
    renderer->ops->draw_string(renderer, text, &left, box->text_alignment, 
                               &color_black);

    return (box->font_height + ascent);
    
  }

  return 0;
}
