/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998, 1999 Alexander Larsson
 *
 * Custom Objects -- objects defined in XML rather than C.
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

/* used when resizing to decide which side of the shape to expand/shrink */
typedef enum {
  ANCHOR_MIDDLE,
  ANCHOR_START,
  ANCHOR_END
} AnchorShape;

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
  real dashlength;

  gboolean flip_h, flip_v;

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
  real dashlength;

  gboolean flip_h, flip_v;

  Text *text;
  real padding;
};

typedef struct _CustomProperties {
  Color *fg_color;
  Color *bg_color;
  gboolean show_background;
  real border_width;

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
static void custom_update_data(Custom *custom, AnchorShape h, AnchorShape v);
static Object *custom_create(Point *startpoint,
			  void *user_data,
			  Handle **handle1,
			  Handle **handle2);
static void custom_destroy(Custom *custom);
static Object *custom_copy(Custom *custom);
static GtkWidget *custom_get_properties(Custom *custom);
static ObjectChange *custom_apply_properties(Custom *custom);
static DiaMenu *custom_get_object_menu(Custom *custom, Point *clickedpoint);

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
  "Custom - Generic",  /* name */
  0,                 /* version */
  (char **) custom_xpm, /* pixmap */

  &custom_type_ops      /* ops */
};

static SheetObject custom_sheetobj =
{
  "Custom - Generic",
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
  (ObjectMenuFunc)      custom_get_object_menu
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
  dia_line_style_selector_get_linestyle(custom_properties_dialog->line_style, &custom->line_style, &custom->dashlength);

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
  
  custom_update_data(custom, ANCHOR_MIDDLE, ANCHOR_MIDDLE);
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
  GtkAdjustment *adj;

  if (custom_properties_dialog == NULL) {
    custom_properties_dialog = g_new(CustomPropertiesDialog, 1);

    vbox = gtk_vbox_new(FALSE, 5);
    gtk_object_ref(GTK_OBJECT(vbox));
    gtk_object_sink(GTK_OBJECT(vbox));
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
					custom->line_style,custom->dashlength);

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
  real coord;
  out->left   = r1->left   * custom->xscale + custom->xoffs;
  out->right  = r1->right  * custom->xscale + custom->xoffs;
  out->top    = r1->top    * custom->yscale + custom->yoffs;
  out->bottom = r1->bottom * custom->yscale + custom->yoffs;

  if (out->left > out->right) {
    coord = out->left;
    out->left = out->right;
    out->right = coord;
  }
  if (out->top > out->bottom) {
    coord = out->top;
    out->top = out->bottom;
    out->bottom = coord;
  }
}

static real
custom_distance_from(Custom *custom, Point *point)
{
  Element *elem = &custom->element;
  Rectangle rect;
  /*Point p;*/
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
  AnchorShape horiz = ANCHOR_MIDDLE, vert = ANCHOR_MIDDLE;

  assert(custom!=NULL);
  assert(handle!=NULL);
  assert(to!=NULL);

  element_move_handle(&custom->element, handle->id, to, reason);

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
  custom_update_data(custom, horiz, vert);
}

static void
custom_move(Custom *custom, Point *to)
{
  custom->element.corner = *to;
  
  custom_update_data(custom, ANCHOR_MIDDLE, ANCHOR_MIDDLE);
}

static void
get_colour(Custom *custom, Color *colour, gint32 c)
{
  switch (c) {
  case COLOUR_NONE:
    break;
  case COLOUR_FOREGROUND:
    *colour = custom->border_color;
    break;
  case COLOUR_BACKGROUND:
    *colour = custom->inner_color;
    break;
  case COLOUR_TEXT:
    *colour = custom->text->color;
    break;
  default:
    colour->red   = ((c & 0xff0000) >> 16) / 255.0;
    colour->green = ((c & 0x00ff00) >> 8) / 255.0;
    colour->blue  =  (c & 0x0000ff) / 255.0;
    break;
  }
}

static void
custom_draw(Custom *custom, Renderer *renderer)
{
  static GArray *arr = NULL, *barr = NULL;
  Point p1, p2;
  real coord;
  int i;
  GList *tmp;
  Element *elem;
  real cur_line = 1.0, cur_dash = 1.0;
  LineCaps cur_caps = LINECAPS_BUTT;
  LineJoin cur_join = LINEJOIN_MITER;
  LineStyle cur_style = custom->line_style;
  
  assert(custom != NULL);
  assert(renderer != NULL);

  if (!arr)
    arr = g_array_new(FALSE, FALSE, sizeof(Point));
  if (!barr)
    barr = g_array_new(FALSE, FALSE, sizeof(BezPoint));

  elem = &custom->element;

  renderer->ops->set_fillstyle(renderer, FILLSTYLE_SOLID);
  renderer->ops->set_linewidth(renderer, custom->border_width);
  renderer->ops->set_linestyle(renderer, cur_style);
  renderer->ops->set_dashlength(renderer, custom->dashlength);
  renderer->ops->set_linecaps(renderer, cur_caps);
  renderer->ops->set_linejoin(renderer, cur_join);

  for (tmp = custom->info->display_list; tmp; tmp = tmp->next) {
    GraphicElement *el = tmp->data;
    Color fg, bg;

    if (el->any.s.line_width != cur_line) {
      cur_line = el->any.s.line_width;
      renderer->ops->set_linewidth(renderer,
				   custom->border_width*cur_line);
    }
    if ((el->any.s.linecap == LINECAPS_DEFAULT && cur_caps != LINECAPS_BUTT) ||
	el->any.s.linecap != cur_caps) {
      cur_caps = (el->any.s.linecap!=LINECAPS_DEFAULT) ?
	el->any.s.linecap : LINECAPS_BUTT;
      renderer->ops->set_linecaps(renderer, cur_caps);
    }
    if ((el->any.s.linejoin==LINEJOIN_DEFAULT && cur_join!=LINEJOIN_MITER) ||
	el->any.s.linejoin != cur_join) {
      cur_join = (el->any.s.linejoin!=LINEJOIN_DEFAULT) ?
	el->any.s.linejoin : LINEJOIN_MITER;
      renderer->ops->set_linejoin(renderer, cur_join);
    }
    if ((el->any.s.linestyle == LINESTYLE_DEFAULT &&
	 cur_style != custom->line_style) ||
	el->any.s.linestyle != cur_style) {
      cur_style = (el->any.s.linestyle!=LINESTYLE_DEFAULT) ?
	el->any.s.linestyle : custom->line_style;
      renderer->ops->set_linestyle(renderer, cur_style);
    }
    if (el->any.s.dashlength != cur_dash) {
      cur_dash = el->any.s.dashlength;
      renderer->ops->set_dashlength(renderer,
				    custom->dashlength*cur_dash);
    }
      
    cur_line = el->any.s.line_width;
    get_colour(custom, &fg, el->any.s.stroke);
    get_colour(custom, &bg, el->any.s.fill);
    switch (el->type) {
    case GE_LINE:
      transform_coord(custom, &el->line.p1, &p1);
      transform_coord(custom, &el->line.p2, &p2);
      if (el->any.s.stroke != COLOUR_NONE)
	renderer->ops->draw_line(renderer, &p1, &p2, &fg);
      break;
    case GE_POLYLINE:
      g_array_set_size(arr, el->polyline.npoints);
      for (i = 0; i < el->polyline.npoints; i++)
	transform_coord(custom, &el->polyline.points[i],
			&g_array_index(arr, Point, i));
      if (el->any.s.stroke != COLOUR_NONE)
	renderer->ops->draw_polyline(renderer,
				     (Point *)arr->data, el->polyline.npoints,
				     &fg);
      break;
    case GE_POLYGON:
      g_array_set_size(arr, el->polygon.npoints);
      for (i = 0; i < el->polygon.npoints; i++)
	transform_coord(custom, &el->polygon.points[i],
			&g_array_index(arr, Point, i));
      if (custom->show_background && el->any.s.fill != COLOUR_NONE) 
	renderer->ops->fill_polygon(renderer,
				    (Point *)arr->data, el->polygon.npoints,
				    &bg);
      if (el->any.s.stroke != COLOUR_NONE)
	renderer->ops->draw_polygon(renderer,
				    (Point *)arr->data, el->polygon.npoints,
				    &fg);
      break;
    case GE_RECT:
      transform_coord(custom, &el->rect.corner1, &p1);
      transform_coord(custom, &el->rect.corner2, &p2);
      if (p1.x > p2.x) {
	coord = p1.x;
	p1.x = p2.x;
	p2.x = coord;
      }
      if (p1.y > p2.y) {
	coord = p1.y;
	p1.y = p2.y;
	p2.y = coord;
      }
      if (custom->show_background && el->any.s.fill != COLOUR_NONE)
	renderer->ops->fill_rect(renderer, &p1, &p2, &bg);
      if (el->any.s.stroke != COLOUR_NONE)
	renderer->ops->draw_rect(renderer, &p1, &p2, &fg);
      break;
    case GE_ELLIPSE:
      transform_coord(custom, &el->ellipse.center, &p1);
      if (custom->show_background && el->any.s.fill != COLOUR_NONE)
	renderer->ops->fill_ellipse(renderer, &p1,
				    el->ellipse.width * custom->xscale,
				    el->ellipse.height * custom->yscale,
				    &bg);
      if (el->any.s.stroke != COLOUR_NONE)
	renderer->ops->draw_ellipse(renderer, &p1,
				    el->ellipse.width * custom->xscale,
				    el->ellipse.height * custom->yscale,
				    &fg);
      break;
    case GE_PATH:
      g_array_set_size(barr, el->path.npoints);
      for (i = 0; i < el->path.npoints; i++)
	switch (g_array_index(barr,BezPoint,i).type=el->path.points[i].type) {
	case BEZ_CURVE_TO:
	  transform_coord(custom, &el->path.points[i].p3,
			  &g_array_index(barr, BezPoint, i).p3);
	  transform_coord(custom, &el->path.points[i].p2,
			  &g_array_index(barr, BezPoint, i).p2);
	case BEZ_MOVE_TO:
	case BEZ_LINE_TO:
	  transform_coord(custom, &el->path.points[i].p1,
			  &g_array_index(barr, BezPoint, i).p1);
	}
      if (el->any.s.stroke != COLOUR_NONE)
	renderer->ops->draw_bezier(renderer, (BezPoint *)barr->data,
				   el->path.npoints, &fg);
      break;
    case GE_SHAPE:
      g_array_set_size(barr, el->path.npoints);
      for (i = 0; i < el->path.npoints; i++)
	switch (g_array_index(barr,BezPoint,i).type=el->path.points[i].type) {
	case BEZ_CURVE_TO:
	  transform_coord(custom, &el->path.points[i].p3,
			  &g_array_index(barr, BezPoint, i).p3);
	  transform_coord(custom, &el->path.points[i].p2,
			  &g_array_index(barr, BezPoint, i).p2);
	case BEZ_MOVE_TO:
	case BEZ_LINE_TO:
	  transform_coord(custom, &el->path.points[i].p1,
			  &g_array_index(barr, BezPoint, i).p1);
	}
      if (custom->show_background && el->any.s.fill != COLOUR_NONE)
	renderer->ops->fill_bezier(renderer, (BezPoint *)barr->data,
				   el->path.npoints, &bg);
      if (el->any.s.stroke != COLOUR_NONE)
	renderer->ops->draw_bezier(renderer, (BezPoint *)barr->data,
				   el->path.npoints, &fg);
      break;
    }
  }

  if (custom->info->has_text) {
    /*Rectangle tb;
    
      if (renderer->is_interactive) {
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
  state->dashlength = custom->dashlength;
  state->padding = custom->padding;
  state->flip_h = custom->flip_h;
  state->flip_v = custom->flip_v;
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
  custom->dashlength = state->dashlength;
  custom->padding = state->padding;
  custom->flip_h = state->flip_h;
  custom->flip_v = state->flip_v;
  if (custom->info->has_text)
    text_set_attributes(custom->text, &state->text_attrib);

  g_free(state);
  
  custom_update_data(custom, ANCHOR_MIDDLE, ANCHOR_MIDDLE);
}


static void
custom_update_data(Custom *custom, AnchorShape horiz, AnchorShape vert)
{
  Element *elem = &custom->element;
  ShapeInfo *info = custom->info;
  Object *obj = (Object *) custom;
  Point center, bottom_right;
  Point p;
  Rectangle tb;
  int i;

  /* save starting points */
  center = bottom_right = elem->corner;
  center.x += elem->width/2;
  bottom_right.x += elem->width;
  center.y += elem->height/2;
  bottom_right.y += elem->height;

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

    if (xscale > 1.0) {
      elem->width  *= xscale;
      custom->xscale *= xscale;
    }
    if (yscale > 1.0) {
      elem->height *= yscale;
      custom->yscale *= yscale;
    }
  }

  /* if aspect ratio should be fixed, make sure xscale == yscale */
  switch (info->aspect_type) {
  case SHAPE_ASPECT_FREE:
    break;
  case SHAPE_ASPECT_FIXED:
    if (custom->xscale != custom->yscale) {
      custom->xscale = custom->yscale = (custom->xscale + custom->yscale) / 2;
      elem->width = custom->xscale * (info->shape_bounds.right -
				      info->shape_bounds.left);
      elem->height = custom->yscale * (info->shape_bounds.bottom -
				       info->shape_bounds.top);
    }
    break;
  case SHAPE_ASPECT_RANGE:
    if (custom->xscale / custom->yscale < info->aspect_min) {
      custom->xscale = custom->yscale * info->aspect_min;
      elem->width = custom->xscale * (info->shape_bounds.right -
				      info->shape_bounds.left);
    }
    if (custom->xscale / custom->yscale > info->aspect_max) {
      custom->yscale = custom->xscale / info->aspect_max;
      elem->height = custom->yscale * (info->shape_bounds.bottom -
				       info->shape_bounds.top);
    }
    break;
  }

  /* move shape if necessary ... */
  switch (horiz) {
  case ANCHOR_START:
    break;
  case ANCHOR_MIDDLE:
    elem->corner.x = center.x - elem->width/2; break;
  case ANCHOR_END:
    elem->corner.x = bottom_right.x - elem->width; break;
  }
  switch (vert) {
  case ANCHOR_START:
    break;
  case ANCHOR_MIDDLE:
    elem->corner.y = center.y - elem->height/2; break;
  case ANCHOR_END:
    elem->corner.y = bottom_right.y - elem->height; break;
  }

  /* update transformation coefficients after the possible resize ... */
  custom->xscale = elem->width / (info->shape_bounds.right -
				  info->shape_bounds.left);
  custom->yscale = elem->height / (info->shape_bounds.bottom -
				   info->shape_bounds.top);
  custom->xoffs = elem->corner.x - custom->xscale * info->shape_bounds.left;
  custom->yoffs = elem->corner.y - custom->yscale * info->shape_bounds.top;

  /* flip image if required ... */
  if (custom->flip_h) {
    custom->xscale = -custom->xscale;
    custom->xoffs = elem->corner.x -custom->xscale * info->shape_bounds.right;
  }
  if (custom->flip_v) {
    custom->yscale = -custom->yscale;
    custom->yoffs = elem->corner.y -custom->yscale * info->shape_bounds.bottom;
  }
  
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
  int i;

  g_return_val_if_fail(info!=NULL,NULL);

  init_default_values();

  custom = g_new0(Custom, 1);
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
  attributes_get_default_line_style(&custom->line_style, &custom->dashlength);

  custom->padding = default_properties.padding;

  custom->flip_h = FALSE;
  custom->flip_v = FALSE;
  
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

  custom_update_data(custom, ANCHOR_MIDDLE, ANCHOR_MIDDLE);

  *handle1 = NULL;
  *handle2 = obj->handles[7];  
  return (Object *)custom;
}

static void
custom_destroy(Custom *custom)
{
  if (custom->info->has_text)
    text_destroy(custom->text);

  element_destroy(&custom->element);
  
  g_free(custom->connections);
}

static Object *
custom_copy(Custom *custom)
{
  int i;
  Custom *newcustom;
  Element *elem, *newelem;
  Object *newobj;
  
  elem = &custom->element;
  
  newcustom = g_new0(Custom, 1);
  newelem = &newcustom->element;
  newobj = (Object *) newcustom;

  element_copy(elem, newelem);

  newcustom->info = custom->info;

  newcustom->border_width = custom->border_width;
  newcustom->border_color = custom->border_color;
  newcustom->inner_color = custom->inner_color;
  newcustom->show_background = custom->show_background;
  newcustom->line_style = custom->line_style;
  newcustom->dashlength = custom->dashlength;
  newcustom->padding = custom->padding;

  newcustom->flip_h = custom->flip_h;
  newcustom->flip_v = custom->flip_v;

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

  if (custom->line_style != LINESTYLE_SOLID &&
      custom->dashlength != DEFAULT_LINESTYLE_DASHLEN)
    data_add_real(new_attribute(obj_node, "dashlength"),
                  custom->dashlength);

  data_add_boolean(new_attribute(obj_node, "flip_horizontal"), custom->flip_h);
  data_add_boolean(new_attribute(obj_node, "flip_vertical"), custom->flip_v);

  data_add_real(new_attribute(obj_node, "padding"), custom->padding);
  
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

  custom = g_new0(Custom, 1);
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

  custom->dashlength = DEFAULT_LINESTYLE_DASHLEN;
  attr = object_find_attribute(obj_node, "dashlength");
  if (attr != NULL)
    custom->dashlength = data_real(attribute_first_data(attr));

  custom->flip_h = FALSE;
  attr = object_find_attribute(obj_node, "flip_horizontal");
  if (attr != NULL)
    custom->flip_h = data_boolean(attribute_first_data(attr));
 
  custom->flip_v = FALSE;
  attr = object_find_attribute(obj_node, "flip_vertical");
  if (attr != NULL)
    custom->flip_v = data_boolean(attribute_first_data(attr));

  custom->padding = default_properties.padding;
  attr = object_find_attribute(obj_node, "padding");
  if (attr != NULL)
    custom->padding =  data_real( attribute_first_data(attr) );
  
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

  custom_update_data(custom, ANCHOR_MIDDLE, ANCHOR_MIDDLE);

  return (Object *)custom;
}

struct CustomObjectChange {
  ObjectChange objchange;

  enum { CHANGE_FLIPH, CHANGE_FLIPV} type;
  gboolean old_val;
};

static void
custom_change_apply(struct CustomObjectChange *change, Custom *custom)
{
  switch (change->type) {
  case CHANGE_FLIPH:
    custom->flip_h = !change->old_val;
    break;
  case CHANGE_FLIPV:
    custom->flip_v = !change->old_val;
    break;
  }
}
static void
custom_change_revert(struct CustomObjectChange *change, Custom *custom)
{
  switch (change->type) {
  case CHANGE_FLIPH:
    custom->flip_h = change->old_val;
    break;
  case CHANGE_FLIPV:
    custom->flip_v = change->old_val;
    break;
  }
}

static ObjectChange *
custom_flip_h_callback (Object *obj, Point *clicked, gpointer data)
{
  Custom *custom = (Custom *)obj;
  struct CustomObjectChange *change = g_new(struct CustomObjectChange, 1);

  change->objchange.apply = (ObjectChangeApplyFunc)custom_change_apply;
  change->objchange.revert = (ObjectChangeRevertFunc)custom_change_revert;
  change->objchange.free = NULL;
  change->type = CHANGE_FLIPH;
  change->old_val = custom->flip_h;

  custom->flip_h = !custom->flip_h;
  custom_update_data(custom, ANCHOR_MIDDLE, ANCHOR_MIDDLE);
  
  return &change->objchange;
}

static ObjectChange *
custom_flip_v_callback (Object *obj, Point *clicked, gpointer data)
{
  Custom *custom = (Custom *)obj;
  struct CustomObjectChange *change = g_new(struct CustomObjectChange, 1);

  change->objchange.apply = (ObjectChangeApplyFunc)custom_change_apply;
  change->objchange.revert = (ObjectChangeRevertFunc)custom_change_revert;
  change->objchange.free = NULL;
  change->type = CHANGE_FLIPV;
  change->old_val = custom->flip_v;

  custom->flip_v = !custom->flip_v;
  custom_update_data(custom, ANCHOR_MIDDLE, ANCHOR_MIDDLE);

  return &change->objchange;
}

static DiaMenuItem custom_menu_items[] = {
  { N_("Flip Horizontal"), custom_flip_h_callback, NULL, 1 },
  { N_("Flip Vertical"), custom_flip_v_callback, NULL, 1 },
};

static DiaMenu custom_menu = {
  "Custom",
  sizeof(custom_menu_items)/sizeof(DiaMenuItem),
  custom_menu_items,
  NULL
};

static DiaMenu *
custom_get_object_menu(Custom *custom, Point *clickedpoint)
{
  custom_menu.title = custom->info->name;
  return &custom_menu;
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

  if (info->icon) {
    sheet->pixmap = NULL;
    sheet->pixmap_file = info->icon;
  }

  info->object_type = obj;

  *otype = obj;
  *sheetobj = sheet;
}
