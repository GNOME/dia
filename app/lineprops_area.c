/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * lineprops_area.h -- Copyright (C) 1999 James Henstridge.
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

#include "intl.h"
#include "widgets.h"
#include "lineprops_area.h"
#include "interface.h"
#include "render_pixmap.h"

#include <gtk/gtk.h>

/* --------------- DiaArrowPreview -------------------------------- */
typedef struct _DiaArrowPreview DiaArrowPreview;
typedef struct _DiaArrowPreviewClass DiaArrowPreviewClass;

static GtkType dia_arrow_preview_get_type (void);
static GtkWidget *dia_arrow_preview_new (ArrowType atype, gboolean left);
static void dia_arrow_preview_set(DiaArrowPreview *arrow, 
                                  ArrowType atype, gboolean left);

#define DIA_ARROW_PREVIEW(obj) (GTK_CHECK_CAST((obj),dia_arrow_preview_get_type(), DiaArrowPreview))
#define DIA_ARROW_PREVIEW_CLASS(obj) (GTK_CHECK_CLASS_CAST((obj), dia_arrow_preview_get_type(), DiaArrowPreviewClass))

struct _DiaArrowPreview
{
  GtkMisc misc;
  ArrowType atype;
  gboolean left;
};
struct _DiaArrowPreviewClass
{
  GtkMiscClass parent_class;
};

static void dia_arrow_preview_class_init (DiaArrowPreviewClass  *klass);
static void dia_arrow_preview_init       (DiaArrowPreview       *arrow);
static gint dia_arrow_preview_expose     (GtkWidget      *widget,
					  GdkEventExpose *event);

static GtkType
dia_arrow_preview_get_type (void)
{
  static GtkType arrow_type = 0;

  if (!arrow_type) {
      static const GtkTypeInfo arrow_info = {
        "DiaArrowPreview",
        sizeof (DiaArrowPreview),
        sizeof (DiaArrowPreviewClass),
        (GtkClassInitFunc) dia_arrow_preview_class_init,
        (GtkObjectInitFunc) dia_arrow_preview_init,
        /* reserved_1 */ NULL,
        /* reserved_2 */ NULL,
        (GtkClassInitFunc) NULL,
      };
      arrow_type = gtk_type_unique (GTK_TYPE_MISC, &arrow_info);
    }
  return arrow_type;
}

static void
dia_arrow_preview_class_init (DiaArrowPreviewClass *class)
{
  GtkWidgetClass *widget_class;

  widget_class = (GtkWidgetClass *)class;
  widget_class->expose_event = dia_arrow_preview_expose;
}

static void
dia_arrow_preview_init (DiaArrowPreview *arrow)
{
  GTK_WIDGET_SET_FLAGS (arrow, GTK_NO_WINDOW);

  GTK_WIDGET (arrow)->requisition.width = 30 + GTK_MISC (arrow)->xpad * 2;
  GTK_WIDGET (arrow)->requisition.height = 20 + GTK_MISC (arrow)->ypad * 2;

  arrow->atype = ARROW_NONE;
  arrow->left = TRUE;
}

static GtkWidget *
dia_arrow_preview_new (ArrowType atype, gboolean left)
{
  DiaArrowPreview *arrow = gtk_type_new (dia_arrow_preview_get_type());

  arrow->atype = atype;
  arrow->left = left;
  return GTK_WIDGET(arrow);
}

static void
dia_arrow_preview_set(DiaArrowPreview *arrow, ArrowType atype, gboolean left)
{
  if (arrow->atype != atype || arrow->left != left) {
    arrow->atype = atype;
    arrow->left = left;
    if (GTK_WIDGET_DRAWABLE(arrow))
      gtk_widget_queue_clear(GTK_WIDGET(arrow));
  }
}

static gint
dia_arrow_preview_expose(GtkWidget *widget, GdkEventExpose *event)
{
  if (GTK_WIDGET_DRAWABLE(widget)) {
    Point from, to;
    DiaRenderer *renderer;
    DiaArrowPreview *arrow = DIA_ARROW_PREVIEW(widget);
    GtkMisc *misc = GTK_MISC(widget);
    gint width, height;
    gint x, y;
    GdkWindow *win;
    int linewidth = 2;
    DiaRendererClass *renderer_ops;

    width = widget->allocation.width - misc->xpad * 2;
    height = widget->allocation.height - misc->ypad * 2;
    x = (widget->allocation.x + misc->xpad);
    y = (widget->allocation.y + misc->ypad);

    win = widget->window;

    to.y = from.y = height/2;
    if (arrow->left) {
      from.x = width-linewidth;
      to.x = 0;
    } else {
      from.x = 0;
      to.x = width-linewidth;
    }
    renderer = new_pixmap_renderer(win, width, height);
    renderer_ops = DIA_RENDERER_GET_CLASS (renderer);
    renderer_pixmap_set_pixmap(renderer, win, x, y, width, height);
    renderer_ops->begin_render(renderer);
    renderer_ops->set_linewidth(renderer, linewidth);
    renderer_ops->draw_line(renderer, &to, &from, &color_black);
    arrow_draw (renderer, arrow->atype, 
                &to, &from, (real)height-linewidth, (real)height-linewidth,
                linewidth, &color_black, &color_white);
    renderer_ops->end_render(renderer);
    g_object_unref(renderer);
  }

  return TRUE;
}

/* --------------- DiaLinePreview -------------------------------- */
typedef struct _DiaLinePreview DiaLinePreview;
typedef struct _DiaLinePreviewClass DiaLinePreviewClass;

static GtkType dia_line_preview_get_type (void);
static void dia_line_preview_set(DiaLinePreview *line, LineStyle lstyle);
static GtkWidget *dia_line_preview_new (LineStyle lstyle);

#define DIA_LINE_PREVIEW(obj) (GTK_CHECK_CAST((obj),dia_line_preview_get_type(), DiaLinePreview))
#define DIA_LINE_PREVIEW_CLASS(obj) (GTK_CHECK_CLASS_CAST((obj), dia_line_preview_get_type(), DiaLinePreviewClass))

struct _DiaLinePreview
{
  GtkMisc misc;
  LineStyle lstyle;
};
struct _DiaLinePreviewClass
{
  GtkMiscClass parent_class;
};

static void dia_line_preview_class_init (DiaLinePreviewClass  *klass);
static void dia_line_preview_init       (DiaLinePreview       *arrow);
static gint dia_line_preview_expose     (GtkWidget      *widget,
					 GdkEventExpose *event);

static GtkType
dia_line_preview_get_type (void)
{
  static GtkType line_type = 0;

  if (!line_type) {
      static const GtkTypeInfo line_info = {
        "DiaLinePreview",
        sizeof (DiaLinePreview),
        sizeof (DiaLinePreviewClass),
        (GtkClassInitFunc) dia_line_preview_class_init,
        (GtkObjectInitFunc) dia_line_preview_init,
        /* reserved_1 */ NULL,
        /* reserved_2 */ NULL,
        (GtkClassInitFunc) NULL,
      };
      line_type = gtk_type_unique (GTK_TYPE_MISC, &line_info);
    }
  return line_type;
}

static void
dia_line_preview_class_init (DiaLinePreviewClass *class)
{
  GtkWidgetClass *widget_class;

  widget_class = (GtkWidgetClass *)class;
  widget_class->expose_event = dia_line_preview_expose;
}

static void
dia_line_preview_init (DiaLinePreview *line)
{
  GTK_WIDGET_SET_FLAGS (line, GTK_NO_WINDOW);

  GTK_WIDGET (line)->requisition.width = 40 + GTK_MISC (line)->xpad * 2;
  GTK_WIDGET (line)->requisition.height = 15 + GTK_MISC (line)->ypad * 2;

  
  line->lstyle = LINESTYLE_SOLID;
}

static GtkWidget *
dia_line_preview_new (LineStyle lstyle)
{
  DiaLinePreview *line = gtk_type_new (dia_line_preview_get_type());

  line->lstyle = lstyle;
  return GTK_WIDGET(line);
}

static void
dia_line_preview_set(DiaLinePreview *line, LineStyle lstyle)
{
  if (line->lstyle != lstyle) {
    line->lstyle = lstyle;
    if (GTK_WIDGET_DRAWABLE(line))
      gtk_widget_queue_clear(GTK_WIDGET(line));
  }
}

static gint
dia_line_preview_expose(GtkWidget *widget, GdkEventExpose *event)
{
  DiaLinePreview *line = DIA_LINE_PREVIEW(widget);
  GtkMisc *misc = GTK_MISC(widget);
  gint width, height;
  gint x, y;
  gint extent;
  GdkWindow *win;
  GdkGC *gc;
  GdkGCValues gcvalues;
  char dash_list[6];

  if (GTK_WIDGET_DRAWABLE(widget)) {
    width = widget->allocation.width - misc->xpad * 2;
    height = widget->allocation.height - misc->ypad * 2;
    extent = MIN(width, height);
    x = (widget->allocation.x + misc->xpad);
    y = (widget->allocation.y + misc->ypad);

    win = widget->window;
    gc = widget->style->fg_gc[widget->state];

    /* increase line width */
    gdk_gc_get_values(gc, &gcvalues);
    switch (line->lstyle) {
    case LINESTYLE_SOLID:
      gdk_gc_set_line_attributes(gc, 1, GDK_LINE_SOLID,
				 gcvalues.cap_style, gcvalues.join_style);
      break;
    case LINESTYLE_DASHED:
      gdk_gc_set_line_attributes(gc, 1, GDK_LINE_ON_OFF_DASH,
				 gcvalues.cap_style, gcvalues.join_style);
      dash_list[0] = 10;
      dash_list[1] = 10;
      gdk_gc_set_dashes(gc, 0, dash_list, 2);
      break;
    case LINESTYLE_DASH_DOT:
      gdk_gc_set_line_attributes(gc, 1, GDK_LINE_ON_OFF_DASH,
				 gcvalues.cap_style, gcvalues.join_style);
      dash_list[0] = 10;
      dash_list[1] = 4;
      dash_list[2] = 2;
      dash_list[3] = 4;
      gdk_gc_set_dashes(gc, 0, dash_list, 4);
      break;
    case LINESTYLE_DASH_DOT_DOT:
      gdk_gc_set_line_attributes(gc, 1, GDK_LINE_ON_OFF_DASH,
				 gcvalues.cap_style, gcvalues.join_style);
      dash_list[0] = 10;
      dash_list[1] = 2;
      dash_list[2] = 2;
      dash_list[3] = 2;
      dash_list[4] = 2;
      dash_list[5] = 2;
      gdk_gc_set_dashes(gc, 0, dash_list, 6);
      break;
    case LINESTYLE_DOTTED:
      gdk_gc_set_line_attributes(gc, 1, GDK_LINE_ON_OFF_DASH,
				 gcvalues.cap_style, gcvalues.join_style);
      dash_list[0] = 2;
      dash_list[1] = 2;
      gdk_gc_set_dashes(gc, 0, dash_list, 2);
      break;
    }
    gdk_draw_line(win, gc, x, y+height/2, x+width, y+height/2);
    gdk_gc_set_line_attributes(gc, gcvalues.line_width, gcvalues.line_style,
			       gcvalues.cap_style, gcvalues.join_style);
  }
  return TRUE;
}

/* ------- Code for DiaArrowChooser ----------------------- */
static GtkType dia_arrow_chooser_get_type (void);

static gint close_and_hide(GtkWidget *wid, GdkEventAny *event) {
  gtk_widget_hide(wid);
  return TRUE;
}
static const char *button_menu_key = "dia-button-menu";
static const char *menuitem_enum_key = "dia-menuitem-value";


#define DIA_ARROW_CHOOSER(obj) (GTK_CHECK_CAST((obj),dia_arrow_chooser_get_type(), DiaArrowChooser))
#define DIA_ARROW_CHOOSER_CLASS(obj) (GTK_CHECK_CLASS_CAST((obj), dia_arrow_chooser_get_type(), DiaArrowChooserClass))

typedef struct _DiaArrowChooser DiaArrowChooser;
typedef struct _DiaArrowChooserClass DiaArrowChooserClass;

struct _DiaArrowChooser
{
  GtkButton button;
  DiaArrowPreview *preview;
  Arrow arrow;
  gboolean left;

  DiaChangeArrowCallback callback;
  gpointer user_data;

  GtkWidget *dialog;
  DiaArrowSelector *selector;
};
struct _DiaArrowChooserClass
{
  GtkButtonClass parent_class;
};

static void dia_arrow_chooser_class_init (DiaArrowChooserClass  *klass);
static void dia_arrow_chooser_init       (DiaArrowChooser       *arrow);
static gint dia_arrow_chooser_event      (GtkWidget *widget,
					  GdkEvent *event);
static void dia_arrow_chooser_dialog_ok  (DiaArrowChooser *arrow);
static void dia_arrow_chooser_dialog_cancel (DiaArrowChooser *arrow);
static void dia_arrow_chooser_dialog_destroy (DiaArrowChooser *arrow);
static void dia_arrow_chooser_change_arrow_type (GtkMenuItem *mi,
						 DiaArrowChooser *arrow);

static GtkType
dia_arrow_chooser_get_type (void)
{
  static GtkType arrow_type = 0;

  if (!arrow_type) {
      static const GtkTypeInfo arrow_info = {
        "DiaArrowChooser",
        sizeof (DiaArrowChooser),
        sizeof (DiaArrowChooserClass),
        (GtkClassInitFunc) dia_arrow_chooser_class_init,
        (GtkObjectInitFunc) dia_arrow_chooser_init,
        /* reserved_1 */ NULL,
        /* reserved_2 */ NULL,
        (GtkClassInitFunc) NULL,
      };
      arrow_type = gtk_type_unique (GTK_TYPE_BUTTON, &arrow_info);
    }
  return arrow_type;
}

static void
dia_arrow_chooser_class_init (DiaArrowChooserClass *class)
{
  GtkWidgetClass *widget_class;

  widget_class = (GtkWidgetClass *)class;
  widget_class->event = dia_arrow_chooser_event;
}

static void
dia_arrow_chooser_init (DiaArrowChooser *arrow)
{
  GtkWidget *wid;
  
  arrow->left = FALSE;
  arrow->arrow.type = ARROW_NONE;
  arrow->arrow.length = DEFAULT_ARROW_LENGTH;
  arrow->arrow.width = DEFAULT_ARROW_WIDTH;

  wid = dia_arrow_preview_new(ARROW_NONE, arrow->left);
  gtk_container_add(GTK_CONTAINER(arrow), wid);
  gtk_widget_show(wid);
  arrow->preview = DIA_ARROW_PREVIEW(wid);

  arrow->dialog = NULL;
}

/* Creating the dialog separately so we can handle destroy */
void
dia_arrow_chooser_dialog_new(GtkWidget *widget, gpointer userdata)
{
  DiaArrowChooser *chooser = DIA_ARROW_CHOOSER(userdata);
  if (chooser->dialog == NULL) {
    GtkWidget *wid;
    chooser->dialog = wid = gtk_dialog_new();
    gtk_window_set_title(GTK_WINDOW(wid), _("Arrow Properties"));
    gtk_signal_connect(GTK_OBJECT(wid), "delete_event",
		       GTK_SIGNAL_FUNC(close_and_hide), NULL);
    gtk_signal_connect(GTK_OBJECT(wid), "destroy",
		       GTK_SIGNAL_FUNC(gtk_widget_destroyed),
		       &chooser->dialog);

    wid = dia_arrow_selector_new();
    gtk_container_set_border_width(GTK_CONTAINER(wid), 5);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(chooser->dialog)->vbox), wid,
		       TRUE, TRUE, 0);
    gtk_widget_show(wid);
    chooser->selector = DIAARROWSELECTOR(wid);
    dia_arrow_selector_set_arrow(chooser->selector, chooser->arrow);

    wid = gtk_button_new_with_label(_("OK"));
    GTK_WIDGET_SET_FLAGS(wid, GTK_CAN_DEFAULT);
    gtk_container_add(GTK_CONTAINER(GTK_DIALOG(chooser->dialog)->action_area),wid);
    gtk_widget_grab_default(wid);
    gtk_signal_connect_object(GTK_OBJECT(wid), "clicked",
			      GTK_SIGNAL_FUNC(dia_arrow_chooser_dialog_ok),
			      GTK_OBJECT(chooser));
    gtk_widget_show(wid);

    wid = gtk_button_new_with_label(_("Cancel"));
    GTK_WIDGET_SET_FLAGS(wid, GTK_CAN_DEFAULT);
    gtk_container_add(GTK_CONTAINER(GTK_DIALOG(chooser->dialog)->action_area),wid);
    gtk_signal_connect_object(GTK_OBJECT(wid), "clicked",
			      GTK_SIGNAL_FUNC(dia_arrow_chooser_dialog_cancel),
			      GTK_OBJECT(chooser));
    gtk_widget_show(wid);
  }
}

void
dia_arrow_chooser_dialog_show(GtkWidget *widget, gpointer userdata)
{
     DiaArrowChooser *chooser = DIA_ARROW_CHOOSER(userdata);
     dia_arrow_chooser_dialog_new(widget, chooser);
     dia_arrow_selector_set_arrow(chooser->selector, chooser->arrow);
     gtk_widget_show(chooser->dialog);
}

GtkWidget *
dia_arrow_chooser_new(gboolean left, DiaChangeArrowCallback callback,
		      gpointer user_data)
{
  DiaArrowChooser *chooser = gtk_type_new(dia_arrow_chooser_get_type());
  GtkWidget *menu, *mi, *ar;
  gint i;

  chooser->left = left;
  dia_arrow_preview_set(chooser->preview, chooser->preview->atype, left);
  chooser->callback = callback;
  chooser->user_data = user_data;

  dia_arrow_chooser_dialog_new(NULL, chooser);

  menu = gtk_menu_new();
  gtk_object_set_data_full(GTK_OBJECT(chooser), button_menu_key, menu,
			   (GtkDestroyNotify)gtk_widget_unref);
#ifndef G_OS_WIN32
  for (i = 0; arrow_types[i].name != NULL; i++) {
#else
  for (i = 0; i<= ARROW_BLANKED_CONCAVE; i++) {
#endif
    mi = gtk_menu_item_new();
#ifndef G_OS_WIN32
    gtk_object_set_data(GTK_OBJECT(mi), menuitem_enum_key,
			GINT_TO_POINTER(arrow_types[i].enum_value));
    gtk_tooltips_set_tip(tool_tips, mi, arrow_types[i].name, NULL);
    ar = dia_arrow_preview_new(arrow_types[i].enum_value, left);
#else
    gtk_object_set_data(GTK_OBJECT(mi), menuitem_enum_key, GINT_TO_POINTER(i)); 
    ar = dia_arrow_preview_new(i, left);                  
#endif
    ar = dia_arrow_preview_new(i, left);                                        
    gtk_container_add(GTK_CONTAINER(mi), ar);
    gtk_widget_show(ar);
    gtk_signal_connect(GTK_OBJECT(mi), "activate",
		       GTK_SIGNAL_FUNC(dia_arrow_chooser_change_arrow_type),
		       chooser);
    gtk_container_add(GTK_CONTAINER(menu), mi);
    gtk_widget_show(mi);
  }
  mi = gtk_menu_item_new_with_label(_("Details..."));
  gtk_signal_connect(GTK_OBJECT(mi), "activate",
		     GTK_SIGNAL_FUNC(dia_arrow_chooser_dialog_show),
		     GTK_OBJECT(chooser));
  gtk_container_add(GTK_CONTAINER(menu), mi);
  gtk_widget_show(mi);

  return GTK_WIDGET(chooser);
}

static gint
dia_arrow_chooser_event(GtkWidget *widget, GdkEvent *event)
{
  if (event->type == GDK_BUTTON_PRESS && event->button.button == 1) {
    GtkMenu *menu = gtk_object_get_data(GTK_OBJECT(widget), button_menu_key);
    gtk_menu_popup(menu, NULL, NULL, NULL, NULL,
		   event->button.button, event->button.time);
    return TRUE;
  }
  return FALSE;
}

static void
dia_arrow_chooser_dialog_ok  (DiaArrowChooser *arrow)
{
  Arrow new_arrow = dia_arrow_selector_get_arrow(arrow->selector);

  if (new_arrow.type   != arrow->arrow.type ||
      new_arrow.length != arrow->arrow.length ||
      new_arrow.width  != arrow->arrow.width) {
    arrow->arrow = new_arrow;
    dia_arrow_preview_set(arrow->preview, new_arrow.type, arrow->left);
    if (arrow->callback)
      (* arrow->callback)(arrow->arrow, arrow->user_data);
  }
  gtk_widget_hide(arrow->dialog);
}

static void
dia_arrow_chooser_dialog_cancel (DiaArrowChooser *arrow)
{
  if (arrow->dialog != NULL)
    dia_arrow_selector_set_arrow(arrow->selector, arrow->arrow);
  gtk_widget_hide(arrow->dialog);
}

static void
dia_arrow_chooser_dialog_destroy (DiaArrowChooser *chooser)
{
  /*  dia_arrow_selector_set_arrow(arrow->selector, arrow->arrow);*/
  chooser->dialog = NULL;
  chooser->selector = NULL;
}

static void
dia_arrow_chooser_change_arrow_type(GtkMenuItem *mi, DiaArrowChooser *arrow)
{
  ArrowType atype = GPOINTER_TO_INT(gtk_object_get_data(GTK_OBJECT(mi),
							menuitem_enum_key));

  if (arrow->arrow.type != atype) {
    dia_arrow_preview_set(arrow->preview, atype, arrow->left);
    arrow->arrow.type = atype;
    if (arrow->dialog != NULL)
      dia_arrow_selector_set_arrow(arrow->selector, arrow->arrow);
    if (arrow->callback)
      (* arrow->callback)(arrow->arrow, arrow->user_data);
  }
}


/* ------- Code for DiaLineChooser ---------------------- */
static GtkType dia_line_chooser_get_type (void);

#define DIA_LINE_CHOOSER(obj) (GTK_CHECK_CAST((obj),dia_line_chooser_get_type(), DiaLineChooser))
#define DIA_LINE_CHOOSER_CLASS(obj) (GTK_CHECK_CLASS_CAST((obj), dia_line_chooser_get_type(), DiaLineChooserClass))

typedef struct _DiaLineChooser DiaLineChooser;
typedef struct _DiaLineChooserClass DiaLineChooserClass;

struct _DiaLineChooser
{
  GtkButton button;
  DiaLinePreview *preview;
  LineStyle lstyle;
  real dash_length;

  DiaChangeLineCallback callback;
  gpointer user_data;

  GtkWidget *dialog;
  DiaLineStyleSelector *selector;
};
struct _DiaLineChooserClass
{
  GtkButtonClass parent_class;
};

static void dia_line_chooser_class_init (DiaLineChooserClass  *klass);
static void dia_line_chooser_init       (DiaLineChooser       *arrow);
static gint dia_line_chooser_event      (GtkWidget *widget,
					  GdkEvent *event);
static void dia_line_chooser_dialog_ok  (DiaLineChooser *lchooser);
static void dia_line_chooser_dialog_cancel (DiaLineChooser *lchooser);
static void dia_line_chooser_change_line_style (GtkMenuItem *mi,
						DiaLineChooser *lchooser);

static GtkType
dia_line_chooser_get_type (void)
{
  static GtkType arrow_type = 0;

  if (!arrow_type) {
      static const GtkTypeInfo arrow_info = {
        "DiaLineChooser",
        sizeof (DiaLineChooser),
        sizeof (DiaLineChooserClass),
        (GtkClassInitFunc) dia_line_chooser_class_init,
        (GtkObjectInitFunc) dia_line_chooser_init,
        /* reserved_1 */ NULL,
        /* reserved_2 */ NULL,
        (GtkClassInitFunc) NULL,
      };
      arrow_type = gtk_type_unique (GTK_TYPE_BUTTON, &arrow_info);
    }
  return arrow_type;
}

static void
dia_line_chooser_class_init (DiaLineChooserClass *class)
{
  GtkWidgetClass *widget_class;

  widget_class = (GtkWidgetClass *)class;
  widget_class->event = dia_line_chooser_event;
}

static void
dia_line_chooser_init (DiaLineChooser *lchooser)
{
  GtkWidget *wid;
  GtkWidget *menu, *mi, *ln;
  gint i;

  lchooser->lstyle = LINESTYLE_SOLID;
  lchooser->dash_length = DEFAULT_LINESTYLE_DASHLEN;

  wid = dia_line_preview_new(LINESTYLE_SOLID);
  gtk_container_add(GTK_CONTAINER(lchooser), wid);
  gtk_widget_show(wid);
  lchooser->preview = DIA_LINE_PREVIEW(wid);

  lchooser->dialog = wid = gtk_dialog_new();
  gtk_window_set_title(GTK_WINDOW(wid), _("Line Style Properties"));
  gtk_signal_connect(GTK_OBJECT(wid), "delete_event",
		     GTK_SIGNAL_FUNC(close_and_hide), NULL);

  wid = dia_line_style_selector_new();
  gtk_container_set_border_width(GTK_CONTAINER(wid), 5);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(lchooser->dialog)->vbox), wid,
		     TRUE, TRUE, 0);
  gtk_widget_show(wid);
  lchooser->selector = DIALINESTYLESELECTOR(wid);

  wid = gtk_button_new_with_label(_("OK"));
  GTK_WIDGET_SET_FLAGS(wid, GTK_CAN_DEFAULT);
  gtk_container_add(GTK_CONTAINER(GTK_DIALOG(lchooser->dialog)->action_area),
		    wid);
  gtk_widget_grab_default(wid);
  gtk_signal_connect_object(GTK_OBJECT(wid), "clicked",
			    GTK_SIGNAL_FUNC(dia_line_chooser_dialog_ok),
			    GTK_OBJECT(lchooser));
  gtk_widget_show(wid);

  wid = gtk_button_new_with_label(_("Cancel"));
  GTK_WIDGET_SET_FLAGS(wid, GTK_CAN_DEFAULT);
  gtk_container_add(GTK_CONTAINER(GTK_DIALOG(lchooser->dialog)->action_area),
		    wid);
  gtk_signal_connect_object(GTK_OBJECT(wid), "clicked",
			    GTK_SIGNAL_FUNC(dia_line_chooser_dialog_cancel),
			    GTK_OBJECT(lchooser));

  menu = gtk_menu_new();
  gtk_object_set_data_full(GTK_OBJECT(lchooser), button_menu_key, menu,
			   (GtkDestroyNotify)gtk_widget_unref);
  for (i = 0; i <= LINESTYLE_DOTTED; i++) {
    mi = gtk_menu_item_new();
    gtk_object_set_data(GTK_OBJECT(mi), menuitem_enum_key, GINT_TO_POINTER(i));
    ln = dia_line_preview_new(i);
    gtk_container_add(GTK_CONTAINER(mi), ln);
    gtk_widget_show(ln);
    gtk_signal_connect(GTK_OBJECT(mi), "activate",
		       GTK_SIGNAL_FUNC(dia_line_chooser_change_line_style),
		       lchooser);
    gtk_container_add(GTK_CONTAINER(menu), mi);
    gtk_widget_show(mi);
  }
  mi = gtk_menu_item_new_with_label(_("Details..."));
  gtk_signal_connect_object(GTK_OBJECT(mi), "activate",
			    GTK_SIGNAL_FUNC(gtk_widget_show),
			    GTK_OBJECT(lchooser->dialog));
  gtk_container_add(GTK_CONTAINER(menu), mi);
  gtk_widget_show(mi);

  gtk_widget_show(wid);
}

GtkWidget *
dia_line_chooser_new(DiaChangeLineCallback callback,
		     gpointer user_data)
{
  DiaLineChooser *chooser = gtk_type_new(dia_line_chooser_get_type());

  chooser->callback = callback;
  chooser->user_data = user_data;

  return GTK_WIDGET(chooser);
}

static gint
dia_line_chooser_event(GtkWidget *widget, GdkEvent *event)
{
  if (event->type == GDK_BUTTON_PRESS && event->button.button == 1) {
    GtkMenu *menu = gtk_object_get_data(GTK_OBJECT(widget), button_menu_key);
    gtk_menu_popup(menu, NULL, NULL, NULL, NULL,
		   event->button.button, event->button.time);
    return TRUE;
  }
  return FALSE;
}

static void
dia_line_chooser_dialog_ok (DiaLineChooser *lchooser)
{
  LineStyle new_style;
  real new_dash;

  dia_line_style_selector_get_linestyle(lchooser->selector,
					&new_style, &new_dash);
  if (new_style != lchooser->lstyle || new_dash != lchooser->dash_length) {
    lchooser->lstyle = new_style;
    lchooser->dash_length = new_dash;
    dia_line_preview_set(lchooser->preview, new_style);
    if (lchooser->callback)
      (* lchooser->callback)(new_style, new_dash, lchooser->user_data);
  }
  gtk_widget_hide(lchooser->dialog);
}
static void
dia_line_chooser_dialog_cancel (DiaLineChooser *lchooser)
{
  dia_line_style_selector_set_linestyle(lchooser->selector, lchooser->lstyle,
					lchooser->dash_length);
  gtk_widget_hide(lchooser->dialog);
}

static void
dia_line_chooser_change_line_style(GtkMenuItem *mi, DiaLineChooser *lchooser)
{
  LineStyle lstyle = GPOINTER_TO_INT(gtk_object_get_data(GTK_OBJECT(mi),
							 menuitem_enum_key));

  if (lchooser->lstyle != lstyle) {
    dia_line_preview_set(lchooser->preview, lstyle);
    lchooser->lstyle = lstyle;
    dia_line_style_selector_set_linestyle(lchooser->selector, lchooser->lstyle,
					  lchooser->dash_length);
    if (lchooser->callback)
      (* lchooser->callback)(lchooser->lstyle, lchooser->dash_length,
			     lchooser->user_data);
  }
}

