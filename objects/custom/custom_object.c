/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * Flowchart toolbox -- objects for drawing flowcharts.
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
#include <assert.h>
#include <gtk/gtk.h>
#include <math.h>

#include "config.h"
#include "intl.h"
#include "shape_info.h"
#include "object.h"
#include "element.h"
#include "connectionpoint.h"
#include "render.h"
#include "attributes.h"
#include "text.h"
#include "widgets.h"
#include "message.h"
#include "sheet.h"

#include "pixmaps/custom.xpm"

#define DEFAULT_WIDTH 2.0
#define DEFAULT_HEIGHT 1.0
#define DEFAULT_BORDER 0.25

typedef struct _Custom Custom;
typedef struct _CustomPropertiesDialog CustomPropertiesDialog;
typedef struct _CustomDefaultsDialog CustomDefaultsDialog;
typedef struct _CustomState CustomState;

struct _CustomState {
  ObjectState obj_state;
  
  real border_width;
  Color border_color;
  Color inner_color;
  gboolean show_background;
  LineStyle line_style;

  real padding;
  TextAttributes text_attrib;
};

struct _Custom {
  Element element;

  ShapeInfo *info;
  /* transformation coords */
  real xscale, yscale;
  real xoffs,  yoffs;

  ConnectionPoint *connections;
  real border_width;
  Color border_color;
  Color inner_color;
  gboolean show_background;
  LineStyle line_style;

  Text *text;
  real padding;
};

typedef struct _CustomProperties {
  Color *fg_color;
  Color *bg_color;
  gboolean show_background;
  real border_width;
  LineStyle line_style;

  real padding;
  Font *font;
  real font_size;
  Alignment alignment;
  Color *font_color;
} CustomProperties;

struct _CustomPropertiesDialog {
  GtkWidget *vbox;

  GtkSpinButton *border_width;
  DiaColorSelector *fg_color;
  DiaColorSelector *bg_color;
  GtkToggleButton *show_background;
  DiaLineStyleSelector *line_style;

  GtkWidget *text_vbox;
  GtkSpinButton *padding;
  DiaAlignmentSelector *alignment;
  DiaFontSelector *font;
  GtkSpinButton *font_size;
  DiaColorSelector *font_color;

  Custom *custom;
};

struct _CustomDefaultsDialog {
  GtkWidget *vbox;

  GtkToggleButton *show_background;
  DiaLineStyleSelector *line_style;

  GtkSpinButton *padding;
  DiaAlignmentSelector *alignment;
  DiaFontSelector *font;
  GtkSpinButton *font_size;
};


static CustomPropertiesDialog *custom_properties_dialog;
static CustomDefaultsDialog *custom_defaults_dialog;
static CustomProperties default_properties;

static real custom_distance_from(Custom *custom, Point *point);
static void custom_select(Custom *custom, Point *clicked_point,
		       Renderer *interactive_renderer);
static void custom_move_handle(Custom *custom, Handle *handle,
			    Point *to, HandleMoveReason reason, 
			    ModifierKeys modifiers);
static void custom_move(Custom *custom, Point *to);
static void custom_draw(Custom *custom, Renderer *renderer);
static void custom_update_data(Custom *custom);
static Object *custom_create(Point *startpoint,
			  void *user_data,
			  Handle **handle1,
			  Handle **handle2);
static void custom_destroy(Custom *custom);
static Object *custom_copy(Custom *custom);
static GtkWidget *custom_get_properties(Custom *custom);
static ObjectChange *custom_apply_properties(Custom *custom);

static CustomState *custom_get_state(Custom *custom);
static void custom_set_state(Custom *custom, CustomState *state);

static void custom_save(Custom *custom, ObjectNode obj_node, const char *filename);
static Object *custom_load(ObjectNode obj_node, int version, const char *filename);
static GtkWidget *custom_get_defaults();
static void custom_apply_defaults();

static ObjectTypeOps custom_type_ops =
{
  (CreateFunc) custom_create,
  (LoadFunc)   custom_load,
  (SaveFunc)   custom_save,
  (GetDefaultsFunc)   custom_get_defaults,
  (ApplyDefaultsFunc) custom_apply_defaults
};

static ObjectType custom_type =
{
  "Flowchart - Custom",  /* name */
  0,                 /* version */
  (char **) custom_xpm, /* pixmap */

  &custom_type_ops      /* ops */
};

static SheetObject custom_sheetobj =
{
  "Flowchart - Custom",
  N_("A custom with text inside."),
  (char **)custom_xpm,
  NULL
};

static ObjectOps custom_ops = {
  (DestroyFunc)         custom_destroy,
  (DrawFunc)            custom_draw,
  (DistanceFunc)        custom_distance_from,
  (SelectFunc)          custom_select,
  (CopyFunc)            custom_copy,
  (MoveFunc)            custom_move,
  (MoveHandleFunc)      custom_move_handle,
  (GetPropertiesFunc)   custom_get_properties,
  (ApplyPropertiesFunc) custom_apply_properties,
  (ObjectMenuFunc)      NULL
};

static ObjectChange *
custom_apply_properties(Custom *custom)
{
  ObjectState *old_state;
  Font *font;
  Color col;
  Alignment align;

  if (custom != custom_properties_dialog->custom) {
    message_warning("Custom dialog problem:  %p != %p\n", custom,
		    custom_properties_dialog->custom);
    custom = custom_properties_dialog->custom;
  }

  old_state = (ObjectState *)custom_get_state(custom);
  
  custom->border_width = gtk_spin_button_get_value_as_float(custom_properties_dialog->border_width);
  dia_color_selector_get_color(custom_properties_dialog->fg_color, &custom->border_color);
  dia_color_selector_get_color(custom_properties_dialog->bg_color, &custom->inner_color);
  custom->show_background = gtk_toggle_button_get_active(custom_properties_dialog->show_background);
  dia_line_style_selector_get_linestyle(custom_properties_dialog->line_style, &custom->line_style, NULL);

  if (custom->info->has_text) {
    custom->padding = gtk_spin_button_get_value_as_float(custom_properties_dialog->padding);
    align = dia_alignment_selector_get_alignment(custom_properties_dialog->alignment);
    text_set_alignment(custom->text, align);
    font = dia_font_selector_get_font(custom_properties_dialog->font);
    text_set_font(custom->text, font);
    text_set_height(custom->text, gtk_spin_button_get_value_as_float(
					custom_properties_dialog->font_size));
    dia_color_selector_get_color(custom_properties_dialog->font_color, &col);
    text_set_color(custom->text, &col);
  }
  
  custom_update_data(custom);
  return new_object_state_change((Object *)custom, old_state, 
				 (GetStateFunc)custom_get_state,
				 (SetStateFunc)custom_set_state);
}

static GtkWidget *
custom_get_properties(Custom *custom)
{
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *color;
  GtkWidget *checkcustom;
  GtkWidget *linestyle;
  GtkWidget *border_width;
  GtkWidget *padding;
  GtkWidget *alignment;
  GtkWidget *font;
  GtkWidget *font_size;
  GtkWidget *font_color;
  GtkAdjustment *adj;

  if (custom_properties_dialog == NULL) {
    custom_properties_dialog = g_new(CustomPropertiesDialog, 1);

    vbox = gtk_vbox_new(FALSE, 5);
    custom_properties_dialog->vbox = vbox;

    hbox = gtk_hbox_new(FALSE, 5);
    label = gtk_label_new(_("Border width:"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    gtk_widget_show (label);
    adj = (GtkAdjustment *) gtk_adjustment_new(0.1, 0.00, 10.0, 0.01, 0.0, 0.0);
    border_width = gtk_spin_button_new(adj, 1.0, 2);
    gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(border_width), TRUE);
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(border_width), TRUE);
    custom_properties_dialog->border_width = GTK_SPIN_BUTTON(border_width);
    gtk_box_pack_start(GTK_BOX (hbox), border_width, TRUE, TRUE, 0);
    gtk_widget_show (border_width);
    gtk_widget_show(hbox);
    gtk_box_pack_start (GTK_BOX(vbox), hbox, TRUE, TRUE, 0);


    hbox = gtk_hbox_new(FALSE, 5);
    label = gtk_label_new(_("Foreground color:"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    gtk_widget_show (label);
    color = dia_color_selector_new();
    custom_properties_dialog->fg_color = DIACOLORSELECTOR(color);
    gtk_box_pack_start (GTK_BOX (hbox), color, TRUE, TRUE, 0);
    gtk_widget_show (color);
    gtk_widget_show(hbox);
    gtk_box_pack_start (GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

    hbox = gtk_hbox_new(FALSE, 5);
    label = gtk_label_new(_("Background color:"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    gtk_widget_show (label);
    color = dia_color_selector_new();
    custom_properties_dialog->bg_color = DIACOLORSELECTOR(color);
    gtk_box_pack_start (GTK_BOX (hbox), color, TRUE, TRUE, 0);
    gtk_widget_show (color);
    gtk_widget_show(hbox);
    gtk_box_pack_start (GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

    hbox = gtk_hbox_new(FALSE, 5);
    checkcustom = gtk_check_button_new_with_label(_("Draw background"));
    custom_properties_dialog->show_background = GTK_TOGGLE_BUTTON( checkcustom );
    gtk_widget_show(checkcustom);
    gtk_widget_show(hbox);
    gtk_box_pack_start (GTK_BOX (hbox), checkcustom, TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);

    hbox = gtk_hbox_new(FALSE, 5);
    label = gtk_label_new(_("Line style:"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    gtk_widget_show (label);
    linestyle = dia_line_style_selector_new();
    custom_properties_dialog->line_style = DIALINESTYLESELECTOR(linestyle);
    gtk_box_pack_start (GTK_BOX (hbox), linestyle, TRUE, TRUE, 0);
    gtk_widget_show (linestyle);
    gtk_widget_show(hbox);
    gtk_box_pack_start (GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

    vbox = gtk_vbox_new(FALSE, 5);
    gtk_widget_show(vbox);
    custom_properties_dialog->text_vbox = vbox;
    gtk_box_pack_start(GTK_BOX(custom_properties_dialog->vbox), vbox,
		       TRUE, TRUE, 0);

    hbox = gtk_hbox_new(FALSE, 5);
    label = gtk_label_new(_("Text padding:"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    gtk_widget_show (label);
    adj = (GtkAdjustment *) gtk_adjustment_new(0.1, 0.0, 10.0, 0.1, 0.0, 0.0);
    padding = gtk_spin_button_new(adj, 1.0, 2);
    gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(padding), TRUE);
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(padding), TRUE);
    custom_properties_dialog->padding = GTK_SPIN_BUTTON(padding);
    gtk_box_pack_start(GTK_BOX (hbox), padding, TRUE, TRUE, 0);
    gtk_widget_show (padding);
    gtk_widget_show(hbox);
    gtk_box_pack_start (GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

    hbox = gtk_hbox_new(FALSE, 5);
    label = gtk_label_new(_("Alignment:"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    gtk_widget_show (label);
    alignment = dia_alignment_selector_new();
    custom_properties_dialog->alignment = DIAALIGNMENTSELECTOR(alignment);
    gtk_box_pack_start (GTK_BOX (hbox), alignment, TRUE, TRUE, 0);
    gtk_widget_show (alignment);
    gtk_widget_show(hbox);
    gtk_box_pack_start (GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

    hbox = gtk_hbox_new(FALSE, 5);
    label = gtk_label_new(_("Font:"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    gtk_widget_show (label);
    font = dia_font_selector_new();
    custom_properties_dialog->font = DIAFONTSELECTOR(font);
    gtk_box_pack_start (GTK_BOX (hbox), font, TRUE, TRUE, 0);
    gtk_widget_show (font);
    gtk_widget_show(hbox);
    gtk_box_pack_start (GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

    hbox = gtk_hbox_new(FALSE, 5);
    label = gtk_label_new(_("Font size:"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    gtk_widget_show (label);
    adj = (GtkAdjustment *) gtk_adjustment_new(0.1, 0.1, 10.0, 0.1, 0.0, 0.0);
    font_size = gtk_spin_button_new(adj, 1.0, 2);
    gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(font_size), TRUE);
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(font_size), TRUE);
    custom_properties_dialog->font_size = GTK_SPIN_BUTTON(font_size);
    gtk_box_pack_start(GTK_BOX (hbox), font_size, TRUE, TRUE, 0);
    gtk_widget_show (font_size);
    gtk_widget_show(hbox);
    gtk_box_pack_start (GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

    hbox = gtk_hbox_new(FALSE, 5);
    label = gtk_label_new(_("Font color:"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    gtk_widget_show (label);
    color = dia_color_selector_new();
    custom_properties_dialog->font_color = DIACOLORSELECTOR(color);
    gtk_box_pack_start (GTK_BOX (hbox), color, TRUE, TRUE, 0);
    gtk_widget_show (color);
    gtk_widget_show(hbox);
    gtk_box_pack_start (GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

    gtk_widget_show (vbox);
  }

  custom_properties_dialog->custom = custom;
    
  gtk_spin_button_set_value(custom_properties_dialog->border_width,
			    custom->border_width);
  dia_color_selector_set_color(custom_properties_dialog->fg_color,
			       &custom->border_color);
  dia_color_selector_set_color(custom_properties_dialog->bg_color,
			       &custom->inner_color);
  gtk_toggle_button_set_active(custom_properties_dialog->show_background, 
			       custom->show_background);
  dia_line_style_selector_set_linestyle(custom_properties_dialog->line_style,
					custom->line_style, 1.0);

  if (custom->info->has_text) {
    gtk_spin_button_set_value(custom_properties_dialog->padding,
			      custom->padding);
    dia_alignment_selector_set_alignment(custom_properties_dialog->alignment, custom->text->alignment);
    dia_font_selector_set_font(custom_properties_dialog->font, custom->text->font);
    gtk_spin_button_set_value(custom_properties_dialog->font_size,
			      custom->text->height);
    dia_color_selector_set_color(custom_properties_dialog->font_color,
				 &custom->text->color);
    gtk_widget_show(custom_properties_dialog->text_vbox);
  } else
    gtk_widget_hide(custom_properties_dialog->text_vbox);

  return custom_properties_dialog->vbox;
}
static void
custom_apply_defaults()
{
  dia_line_style_selector_get_linestyle(custom_defaults_dialog->line_style,
					&default_properties.line_style, NULL);
  default_properties.show_background = gtk_toggle_button_get_active(custom_defaults_dialog->show_background);

  default_properties.padding = gtk_spin_button_get_value_as_float(custom_defaults_dialog->padding);
  default_properties.alignment = dia_alignment_selector_get_alignment(custom_defaults_dialog->alignment);
  default_properties.font = dia_font_selector_get_font(custom_defaults_dialog->font);
  default_properties.font_size = gtk_spin_button_get_value_as_float(custom_defaults_dialog->font_size);
}

static void
init_default_values() {
  static int defaults_initialized = 0;

  if (!defaults_initialized) {
    default_properties.show_background = 1;
    default_properties.padding = 0.5 * M_SQRT1_2;
    default_properties.alignment = ALIGN_CENTER;
    default_properties.font = font_getfont("Courier");
    default_properties.font_size = 0.8;
    defaults_initialized = 1;
  }
}

static GtkWidget *
custom_get_defaults()
{
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *checkcustom;
  GtkWidget *linestyle;
  GtkWidget *padding;
  GtkWidget *alignment;
  GtkWidget *font;
  GtkWidget *font_size;
  GtkAdjustment *adj;

  if (custom_defaults_dialog == NULL) {
  
    init_default_values();

    custom_defaults_dialog = g_new(CustomDefaultsDialog, 1);

    vbox = gtk_vbox_new(FALSE, 5);
    custom_defaults_dialog->vbox = vbox;

    hbox = gtk_hbox_new(FALSE, 5);
    checkcustom = gtk_check_button_new_with_label(_("Draw background"));
    custom_defaults_dialog->show_background = GTK_TOGGLE_BUTTON( checkcustom );
    gtk_widget_show(checkcustom);
    gtk_widget_show(hbox);
    gtk_box_pack_start (GTK_BOX (hbox), checkcustom, TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);

    hbox = gtk_hbox_new(FALSE, 5);
    label = gtk_label_new(_("Line style:"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    gtk_widget_show (label);
    linestyle = dia_line_style_selector_new();
    custom_defaults_dialog->line_style = DIALINESTYLESELECTOR(linestyle);
    gtk_box_pack_start (GTK_BOX (hbox), linestyle, TRUE, TRUE, 0);
    gtk_widget_show (linestyle);
    gtk_widget_show(hbox);
    gtk_box_pack_start (GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

    hbox = gtk_hbox_new(FALSE, 5);
    label = gtk_label_new(_("Text padding:"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    gtk_widget_show (label);
    adj = (GtkAdjustment *) gtk_adjustment_new(0.1, 0.0, 10.0, 0.1, 0.0, 0.0);
    padding = gtk_spin_button_new(adj, 1.0, 2);
    gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(padding), TRUE);
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(padding), TRUE);
    custom_defaults_dialog->padding = GTK_SPIN_BUTTON(padding);
    gtk_box_pack_start(GTK_BOX (hbox), padding, TRUE, TRUE, 0);
    gtk_widget_show (padding);
    gtk_widget_show(hbox);
    gtk_box_pack_start (GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

    hbox = gtk_hbox_new(FALSE, 5);
    label = gtk_label_new(_("Alignment:"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    gtk_widget_show (label);
    alignment = dia_alignment_selector_new();
    custom_defaults_dialog->alignment = DIAALIGNMENTSELECTOR(alignment);
    gtk_box_pack_start (GTK_BOX (hbox), alignment, TRUE, TRUE, 0);
    gtk_widget_show (alignment);
    gtk_widget_show(hbox);
    gtk_box_pack_start (GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

    hbox = gtk_hbox_new(FALSE, 5);
    label = gtk_label_new(_("Font:"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    gtk_widget_show (label);
    font = dia_font_selector_new();
    custom_defaults_dialog->font = DIAFONTSELECTOR(font);
    gtk_box_pack_start (GTK_BOX (hbox), font, TRUE, TRUE, 0);
    gtk_widget_show (font);
    gtk_widget_show(hbox);
    gtk_box_pack_start (GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

    hbox = gtk_hbox_new(FALSE, 5);
    label = gtk_label_new(_("Font size:"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    gtk_widget_show (label);
    adj = (GtkAdjustment *) gtk_adjustment_new(0.1, 0.1, 10.0, 0.1, 0.0, 0.0);
    font_size = gtk_spin_button_new(adj, 1.0, 2);
    gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(font_size), TRUE);
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(font_size), TRUE);
    custom_defaults_dialog->font_size = GTK_SPIN_BUTTON(font_size);
    gtk_box_pack_start(GTK_BOX (hbox), font_size, TRUE, TRUE, 0);
    gtk_widget_show (font_size);
    gtk_widget_show(hbox);
    gtk_box_pack_start (GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

    gtk_widget_show (vbox);
    gtk_widget_show (vbox);
  }

  gtk_toggle_button_set_active(custom_defaults_dialog->show_background, 
			       default_properties.show_background);
  dia_line_style_selector_set_linestyle(custom_defaults_dialog->line_style,
					default_properties.line_style, 1.0);

  gtk_spin_button_set_value(custom_defaults_dialog->padding,
			    default_properties.padding);
  dia_alignment_selector_set_alignment(custom_defaults_dialog->alignment,
				       default_properties.alignment);
  dia_font_selector_set_font(custom_defaults_dialog->font,
			     default_properties.font);
  gtk_spin_button_set_value(custom_defaults_dialog->font_size,
			    default_properties.font_size);

  return custom_defaults_dialog->vbox;
}

static void
transform_coord(Custom *custom, const Point *p1, Point *out)
{
  out->x = p1->x * custom->xscale + custom->xoffs;
  out->y = p1->y * custom->yscale + custom->yoffs;
}

static void
transform_rect(Custom *custom, const Rectangle *r1, Rectangle *out)
{
  out->left   = r1->left   * custom->xscale + custom->xoffs;
  out->right  = r1->right  * custom->xscale + custom->xoffs;
  out->top    = r1->top    * custom->yscale + custom->yoffs;
  out->bottom = r1->bottom * custom->yscale + custom->yoffs;
}

static real
custom_distance_from(Custom *custom, Point *point)
{
  Element *elem = &custom->element;
  Rectangle rect;
  Point p;
  real dist1, dist2;

  rect.left = elem->corner.x - custom->border_width/2;
  rect.right = elem->corner.x + elem->width + custom->border_width/2;
  rect.top = elem->corner.y - custom->border_width/2;
  rect.bottom = elem->corner.y + elem->height + custom->border_width/2;

  dist1 = distance_rectangle_point(&rect, point);
  if (custom->info->has_text) {
    dist2 = text_distance_from(custom->text, point);
    if (dist2 < dist1)
      return dist2;
  }
  return dist1;
}

static void
custom_select(Custom *custom, Point *clicked_point,
	   Renderer *interactive_renderer)
{
  if (custom->info->has_text) {
    text_set_cursor(custom->text, clicked_point, interactive_renderer);
    text_grab_focus(custom->text, (Object *)custom);
  }

  element_update_handles(&custom->element);
}

static void
custom_move_handle(Custom *custom, Handle *handle,
		Point *to, HandleMoveReason reason, ModifierKeys modifiers)
{
  assert(custom!=NULL);
  assert(handle!=NULL);
  assert(to!=NULL);

  element_move_handle(&custom->element, handle->id, to, reason);

  custom_update_data(custom);
}

static void
custom_move(Custom *custom, Point *to)
{
  custom->element.corner = *to;
  
  custom_update_data(custom);
}

static void
custom_draw(Custom *custom, Renderer *renderer)
{
  static GArray *arr = NULL;
  Point p1, p2;
  int i;
  GList *tmp;
  Element *elem;
  
  assert(custom != NULL);
  assert(renderer != NULL);

  if (!arr)
    arr = g_array_new(FALSE, FALSE, sizeof(Point));

  elem = &custom->element;

  renderer->ops->set_fillstyle(renderer, FILLSTYLE_SOLID);
  renderer->ops->set_linewidth(renderer, custom->border_width);
  renderer->ops->set_linestyle(renderer, custom->line_style);
  renderer->ops->set_linejoin(renderer, LINEJOIN_MITER);

  for (tmp = custom->info->display_list; tmp; tmp = tmp->next) {
    GraphicElement *el = tmp->data;
    switch (el->type) {
    case GE_LINE:
      transform_coord(custom, &el->d.line.p1, &p1);
      transform_coord(custom, &el->d.line.p2, &p2);
      renderer->ops->draw_line(renderer, &p1, &p2, &custom->border_color);
      break;
    case GE_POLYLINE:
      g_array_set_size(arr, el->d.polyline.npoints);
      for (i = 0; i < el->d.polyline.npoints; i++)
	transform_coord(custom, &el->d.polyline.points[i],
			&g_array_index(arr, Point, i));
      renderer->ops->draw_polyline(renderer,
				   (Point *)arr->data, el->d.polyline.npoints,
				   &custom->border_color);
      break;
    case GE_POLYGON:
      g_array_set_size(arr, el->d.polygon.npoints);
      for (i = 0; i < el->d.polygon.npoints; i++)
	transform_coord(custom, &el->d.polygon.points[i],
			&g_array_index(arr, Point, i));
      if (custom->show_background) 
	renderer->ops->fill_polygon(renderer,
				    (Point *)arr->data, el->d.polygon.npoints,
				    &custom->inner_color);
      renderer->ops->draw_polygon(renderer,
				  (Point *)arr->data, el->d.polygon.npoints,
				  &custom->border_color);
      break;
    case GE_RECT:
      transform_coord(custom, &el->d.rect.corner1, &p1);
      transform_coord(custom, &el->d.rect.corner2, &p2);
      if (custom->show_background)
	renderer->ops->fill_rect(renderer, &p1, &p2, &custom->inner_color);
      renderer->ops->draw_rect(renderer, &p1, &p2, &custom->border_color);
      break;
    case GE_ELLIPSE:
      transform_coord(custom, &el->d.ellipse.center, &p1);
      if (custom->show_background)
	renderer->ops->fill_ellipse(renderer, &p1,
				    el->d.ellipse.width * custom->xscale,
				    el->d.ellipse.height * custom->yscale,
				    &custom->inner_color); 
      renderer->ops->draw_ellipse(renderer, &p1,
				  el->d.ellipse.width * custom->xscale,
				  el->d.ellipse.height * custom->yscale,
				  &custom->border_color);
      break;
    }
  }

  if (custom->info->has_text) {
    Rectangle tb;
    
    /*if (renderer->is_interactive) {
      transform_rect(custom, &custom->info->text_bounds, &tb);
      p1.x = tb.left;  p1.y = tb.top;
      p2.x = tb.right; p2.y = tb.bottom;
      renderer->ops->draw_rect(renderer, &p1, &p2, &custom->border_color);
      }*/
    text_draw(custom->text, renderer);
  }
}

static CustomState *
custom_get_state(Custom *custom)
{
  CustomState *state = g_new(CustomState, 1);

  state->obj_state.free = NULL;
  
  state->border_width = custom->border_width;
  state->border_color = custom->border_color;
  state->inner_color = custom->inner_color;
  state->show_background = custom->show_background;
  state->line_style = custom->line_style;
  state->padding = custom->padding;
  if (custom->info->has_text)
    text_get_attributes(custom->text, &state->text_attrib);

  return state;
}

static void
custom_set_state(Custom *custom, CustomState *state)
{
  custom->border_width = state->border_width;
  custom->border_color = state->border_color;
  custom->inner_color = state->inner_color;
  custom->show_background = state->show_background;
  custom->line_style = state->line_style;
  custom->padding = state->padding;
  if (custom->info->has_text)
    text_set_attributes(custom->text, &state->text_attrib);

  g_free(state);
  
  custom_update_data(custom);
}


static void
custom_update_data(Custom *custom)
{
  Element *elem = &custom->element;
  ShapeInfo *info = custom->info;
  Object *obj = (Object *) custom;
  Point p;
  Rectangle tb;
  int i;

  /* update the translation coefficients first ... */
  custom->xscale = elem->width / (info->shape_bounds.right -
				  info->shape_bounds.left);
  custom->yscale = elem->height / (info->shape_bounds.bottom -
				   info->shape_bounds.top);

  /* resize shape if text does not fit inside text_bounds */
  if (info->has_text) {
    real width, height;
    real xscale = 0.0, yscale = 0.0;

    transform_rect(custom, &info->text_bounds, &tb);

    width = custom->text->max_width + 2*custom->padding+custom->border_width;
    height = custom->text->height * custom->text->numlines +
      2 * custom->padding + custom->border_width;

    xscale = width / (tb.right - tb.left);
    yscale = height / (tb.bottom - tb.top);

    xscale = MAX(xscale, yscale);
    if (xscale > 1.0) {
      elem->width  *= xscale;
      elem->height *= xscale;
    }
  }
  /* update transformation coefficients after the possible resize ... */
  custom->xscale = elem->width / (info->shape_bounds.right -
				  info->shape_bounds.left);
  custom->yscale = elem->height / (info->shape_bounds.bottom -
				   info->shape_bounds.top);
  custom->xoffs = elem->corner.x - custom->xscale * info->shape_bounds.left;
  custom->yoffs = elem->corner.y - custom->yscale * info->shape_bounds.top;

  /* reposition the text element to the new text bounding box ... */
  if (info->has_text) {
    transform_rect(custom, &info->text_bounds, &tb);
    switch (custom->text->alignment) {
    case ALIGN_LEFT:
      p.x = tb.left;
      break;
    case ALIGN_RIGHT:
      p.x = tb.right;
      break;
    case ALIGN_CENTER:
      p.x = (tb.left + tb.right) / 2;
      break;
    }
    /* align the text to be close to the shape ... */
    if ((tb.bottom+tb.top)/2 > elem->corner.y + elem->height)
      p.y = tb.top +
	font_ascent(custom->text->font, custom->text->height);
    else if ((tb.bottom+tb.top)/2 < elem->corner.y)
      p.y = tb.bottom + custom->text->height * (custom->text->numlines - 1);
    else
      p.y = (tb.top + tb.bottom -
	     custom->text->height * custom->text->numlines) / 2 +
	font_ascent(custom->text->font, custom->text->height);
    text_set_position(custom->text, &p);
  }

  for (i = 0; i < info->nconnections; i++)
    transform_coord(custom, &info->connections[i],&custom->connections[i].pos);

  element_update_boundingbox(elem);
  /* fix boundingbox for line_width: */
  obj->bounding_box.top -= custom->border_width/2;
  obj->bounding_box.left -= custom->border_width/2;
  obj->bounding_box.bottom += custom->border_width/2;
  obj->bounding_box.right += custom->border_width/2;

  /* extend bouding box to include text bounds ... */
  if (info->has_text) {
    transform_rect(custom, &info->text_bounds, &tb);
    if (obj->bounding_box.top > tb.top)
      obj->bounding_box.top = tb.top;
    if (obj->bounding_box.bottom < tb.bottom)
      obj->bounding_box.bottom = tb.bottom;
    if (obj->bounding_box.left > tb.left)
      obj->bounding_box.left = tb.left;
    if (obj->bounding_box.right < tb.right)
      obj->bounding_box.right = tb.right;
  }
  obj->position = elem->corner;
  
  element_update_handles(elem);
}

static Object *
custom_create(Point *startpoint,
	   void *user_data,
	   Handle **handle1,
	   Handle **handle2)
{
  Custom *custom;
  Element *elem;
  Object *obj;
  ShapeInfo *info = (ShapeInfo *)user_data;
  Point p;
  Font *font;
  int i;

  init_default_values();

  custom = g_malloc(sizeof(Custom));
  elem = &custom->element;
  obj = (Object *) custom;
  
  obj->type = info->object_type;

  obj->ops = &custom_ops;

  elem->corner = *startpoint;
  elem->width = DEFAULT_WIDTH;
  elem->height = DEFAULT_WIDTH;

  custom->info = info;

  custom->border_width =  attributes_get_default_linewidth();
  custom->border_color = attributes_get_foreground();
  custom->inner_color = attributes_get_background();
  custom->show_background = default_properties.show_background;
  custom->line_style = default_properties.line_style;

  custom->padding = default_properties.padding;
  
  if (info->has_text) {
    p = *startpoint;
    p.x += elem->width / 2.0;
    p.y += elem->height / 2.0 + default_properties.font_size / 2;
    custom->text = new_text("", default_properties.font,
			    default_properties.font_size, &p,
			    &custom->border_color,
			    default_properties.alignment);
  }
  element_init(elem, 8, info->nconnections);

  custom->connections = g_new(ConnectionPoint, info->nconnections);
  for (i = 0; i < info->nconnections; i++) {
    obj->connections[i] = &custom->connections[i];
    custom->connections[i].object = obj;
    custom->connections[i].connected = NULL;
  }

  custom_update_data(custom);

  *handle1 = NULL;
  *handle2 = obj->handles[7];  
  return (Object *)custom;
}

static void
custom_destroy(Custom *custom)
{
  if (custom->info->has_text)
    text_destroy(custom->text);

  g_free(custom->connections);

  element_destroy(&custom->element);
}

static Object *
custom_copy(Custom *custom)
{
  int i;
  Custom *newcustom;
  Element *elem, *newelem;
  Object *newobj;
  
  elem = &custom->element;
  
  newcustom = g_malloc(sizeof(Custom));
  newelem = &newcustom->element;
  newobj = (Object *) newcustom;

  element_copy(elem, newelem);

  newcustom->info = custom->info;

  newcustom->border_width = custom->border_width;
  newcustom->border_color = custom->border_color;
  newcustom->inner_color = custom->inner_color;
  newcustom->show_background = custom->show_background;
  newcustom->line_style = custom->line_style;
  newcustom->padding = custom->padding;

  if (custom->info->has_text)
    newcustom->text = text_copy(custom->text);
  
  newcustom->connections = g_new(ConnectionPoint, custom->info->nconnections);

  for (i = 0; i < custom->info->nconnections; i++) {
    newobj->connections[i] = &newcustom->connections[i];
    newcustom->connections[i].object = newobj;
    newcustom->connections[i].connected = NULL;
    newcustom->connections[i].pos = custom->connections[i].pos;
    newcustom->connections[i].last_pos = custom->connections[i].last_pos;
  }

  return (Object *)newcustom;
}

static void
custom_save(Custom *custom, ObjectNode obj_node, const char *filename)
{
  element_save(&custom->element, obj_node);

  if (custom->border_width != 0.1)
    data_add_real(new_attribute(obj_node, "border_width"),
		  custom->border_width);
  
  if (!color_equals(&custom->border_color, &color_black))
    data_add_color(new_attribute(obj_node, "border_color"),
		   &custom->border_color);
  
  if (!color_equals(&custom->inner_color, &color_white))
    data_add_color(new_attribute(obj_node, "inner_color"),
		   &custom->inner_color);
  
  data_add_boolean(new_attribute(obj_node, "show_background"), custom->show_background);

  if (custom->line_style != LINESTYLE_SOLID)
    data_add_enum(new_attribute(obj_node, "line_style"),
		  custom->line_style);

  if (custom->info->has_text)
    data_add_text(new_attribute(obj_node, "text"), custom->text);
}

static Object *
custom_load(ObjectNode obj_node, int version, const char *filename)
{
  Custom *custom;
  Element *elem;
  Object *obj;
  int i;
  AttributeNode attr;

  custom = g_malloc(sizeof(Custom));
  elem = &custom->element;
  obj = (Object *) custom;
  
  /* find out what type of object this is ... */
  custom->info = shape_info_get(obj_node);
  
  obj->type = custom->info->object_type;;
  obj->ops = &custom_ops;

  element_load(elem, obj_node);

  custom->border_width = 0.1;
  attr = object_find_attribute(obj_node, "border_width");
  if (attr != NULL)
    custom->border_width =  data_real( attribute_first_data(attr) );

  custom->border_color = color_black;
  attr = object_find_attribute(obj_node, "border_color");
  if (attr != NULL)
    data_color(attribute_first_data(attr), &custom->border_color);
  
  custom->inner_color = color_white;
  attr = object_find_attribute(obj_node, "inner_color");
  if (attr != NULL)
    data_color(attribute_first_data(attr), &custom->inner_color);
  
  custom->show_background = TRUE;
  attr = object_find_attribute(obj_node, "show_background");
  if (attr != NULL)
    custom->show_background = data_boolean( attribute_first_data(attr) );

  custom->line_style = LINESTYLE_SOLID;
  attr = object_find_attribute(obj_node, "line_style");
  if (attr != NULL)
    custom->line_style =  data_enum( attribute_first_data(attr) );

  if (custom->info->has_text) {
    custom->text = NULL;
    attr = object_find_attribute(obj_node, "text");
    if (attr != NULL)
      custom->text = data_text(attribute_first_data(attr));
  }

  element_init(elem, 8, custom->info->nconnections);

  custom->connections = g_new(ConnectionPoint, custom->info->nconnections);
  for (i = 0; i < custom->info->nconnections; i++) {
    obj->connections[i] = &custom->connections[i];
    custom->connections[i].object = obj;
    custom->connections[i].connected = NULL;
  }

  custom_update_data(custom);

  return (Object *)custom;
}


void
custom_object_new(ShapeInfo *info, ObjectType **otype, SheetObject **sheetobj)
{
  ObjectType *obj = g_new(ObjectType, 1);
  SheetObject *sheet = g_new(SheetObject, 1);

  *obj = custom_type;
  *sheet = custom_sheetobj;

  obj->name = info->name;
  sheet->object_type = info->name;
  sheet->description = info->description;
  sheet->user_data = info;

  info->object_type = obj;

  *otype = obj;
  *sheetobj = sheet;
}
