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

#include <gtk/gtk.h>

/* --------------- DiaArrowPreview -------------------------------- */

#define DIA_ARROW_PREVIEW(obj) (GTK_CHECK_CAST((obj),dia_arrow_preview_get_type(), DiaArrowPreview))
#define DIA_ARROW_PREVIEW_CLASS(obj) (GTK_CHECK_CLASS_CAST((obj), dia_arrow_preview_get_type(), DiaArrowPreviewClass))

typedef struct _DiaArrowPreview DiaArrowPreview;
typedef struct _DiaArrowPreviewClass DiaArrowPreviewClass;

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

GtkType
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

GtkWidget *
dia_arrow_preview_new (ArrowType atype, gboolean left)
{
  DiaArrowPreview *arrow = gtk_type_new (dia_arrow_preview_get_type());

  arrow->atype = atype;
  arrow->left = left;
  return GTK_WIDGET(arrow);
}

void
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
  DiaArrowPreview *arrow = DIA_ARROW_PREVIEW(widget);
  GtkMisc *misc = GTK_MISC(widget);
  gint width, height;
  gint x, y;
  gint extent;
  GdkWindow *win;
  GdkGC *gc;
  GdkGCValues gcvalues;
  GdkPoint pts[5];

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
    gdk_gc_set_line_attributes(gc, 3, gcvalues.line_style,
			       gcvalues.cap_style, gcvalues.join_style);
    switch (arrow->atype) {
    case ARROW_NONE:
      if (arrow->left)
	gdk_draw_line(win, gc, x+5,y+height/2, x+width,y+height/2);
      else
	gdk_draw_line(win, gc, x,y+height/2, x+width-5,y+height/2);
      break;
    case ARROW_LINES:
      if (arrow->left) {
	gdk_draw_line(win, gc, x+5,y+height/2, x+width,y+height/2);
	gdk_draw_line(win, gc, x+5,y+height/2, x+5+height/2,y+5);
	gdk_draw_line(win, gc, x+5,y+height/2, x+5+height/2,y+height-5);
      } else {
	gdk_draw_line(win, gc, x,y+height/2, x+width-5,y+height/2);
	gdk_draw_line(win, gc, x+width-5,y+height/2, x+width-5-height/2,y+5);
	gdk_draw_line(win, gc, x+width-5,y+height/2, x+width-5-height/2,y+height-5);
      }
      break;
    case ARROW_HOLLOW_TRIANGLE:
      if (arrow->left) {
	pts[0].x = x+5;          pts[0].y = y+height/2;
	pts[1].x = x+5+height/2; pts[1].y = y+5;
	pts[2].x = x+5+height/2; pts[2].y = y+height-5;
	pts[3].x = x+5;          pts[3].y = y+height/2;
	gdk_draw_polygon(win, gc, FALSE, pts, 4);
	gdk_draw_line(win, gc, x+5+height/2, y+height/2, x+width, y+height/2);
      } else {
	pts[0].x = x+width-5;          pts[0].y = y+height/2;
	pts[1].x = x+width-5-height/2; pts[1].y = y+5;
	pts[2].x = x+width-5-height/2; pts[2].y = y+height-5;
	pts[3].x = x+width-5;          pts[3].y = y+height/2;
	gdk_draw_polygon(win, gc, FALSE, pts, 4);
	gdk_draw_line(win, gc, x, y+height/2, x+width-5-height/2, y+height/2);
      }
      break;
    case ARROW_UNFILLED_TRIANGLE:
      if (arrow->left) {
	pts[0].x = x+5;          pts[0].y = y+height/2;
	pts[1].x = x+5+height/2; pts[1].y = y+5;
	pts[2].x = x+5+height/2; pts[2].y = y+height-5;
	pts[3].x = x+5;          pts[3].y = y+height/2;
	gdk_draw_polygon(win, gc, FALSE, pts, 4);
        gdk_draw_line(win, gc, x+5, y+height/2, x+width, y+height/2);
      } else {
	pts[0].x = x+width-5;          pts[0].y = y+height/2;
	pts[1].x = x+width-5-height/2; pts[1].y = y+5;
	pts[2].x = x+width-5-height/2; pts[2].y = y+height-5;
	pts[3].x = x+width-5;          pts[3].y = y+height/2;
	gdk_draw_polygon(win, gc, FALSE, pts, 4);
        gdk_draw_line(win, gc, x, y+height/2, x+width-5, y+height/2);
      }
      break;
    case ARROW_FILLED_TRIANGLE:
      if (arrow->left) {
	pts[0].x = x+5;          pts[0].y = y+height/2;
	pts[1].x = x+5+height/2; pts[1].y = y+5;
	pts[2].x = x+5+height/2; pts[2].y = y+height-5;
	pts[3].x = x+5;          pts[3].y = y+height/2;
	gdk_draw_polygon(win, gc, TRUE, pts, 4);
	gdk_draw_polygon(win, gc, FALSE, pts, 4);
	gdk_draw_line(win, gc, x+5+height/2, y+height/2, x+width, y+height/2);
      } else {
	pts[0].x = x+width-5;          pts[0].y = y+height/2;
	pts[1].x = x+width-5-height/2; pts[1].y = y+5;
	pts[2].x = x+width-5-height/2; pts[2].y = y+height-5;
	pts[3].x = x+width-5;          pts[3].y = y+height/2;
	gdk_draw_polygon(win, gc, TRUE, pts, 4);
	gdk_draw_polygon(win, gc, FALSE, pts, 4);
	gdk_draw_line(win, gc, x, y+height/2, x+width-5-height/2, y+height/2);
      }
      break;
    case ARROW_HOLLOW_DIAMOND:
      if (arrow->left) {
	pts[0].x = x+5;            pts[0].y = y+height/2;
	pts[1].x = x+5+height/2;   pts[1].y = y+5;
	pts[2].x = x+5+height*3/4; pts[2].y = y+height/2;
	pts[3].x = x+5+height/2;   pts[3].y = y+height-5;
	pts[4].x = x+5;            pts[4].y = y+height/2;
	gdk_draw_polygon(win, gc, FALSE, pts, 5);
	gdk_draw_line(win, gc, x+5+height*3/4,y+height/2, x+width, y+height/2);
      } else {
	pts[0].x = x+width-5;            pts[0].y = y+height/2;
	pts[1].x = x+width-5-height/2;   pts[1].y = y+5;
	pts[2].x = x+width-5-height*3/4; pts[2].y = y+height/2;
	pts[3].x = x+width-5-height/2;   pts[3].y = y+height-5;
	pts[4].x = x+width-5;            pts[4].y = y+height/2;
	gdk_draw_polygon(win, gc, FALSE, pts, 5);
	gdk_draw_line(win, gc, x, y+height/2, x+width-5-height*3/4,y+height/2);
      }
      break;
    case ARROW_FILLED_DIAMOND:
      if (arrow->left) {
	pts[0].x = x+5;            pts[0].y = y+height/2;
	pts[1].x = x+5+height/2;   pts[1].y = y+5;
	pts[2].x = x+5+height*3/4; pts[2].y = y+height/2;
	pts[3].x = x+5+height/2;   pts[3].y = y+height-5;
	pts[4].x = x+5;            pts[4].y = y+height/2;
	gdk_draw_polygon(win, gc, TRUE, pts, 5);
	gdk_draw_polygon(win, gc, FALSE, pts, 5);
	gdk_draw_line(win, gc, x+5+height*3/4,y+height/2, x+width, y+height/2);
      } else {
	pts[0].x = x+width-5;            pts[0].y = y+height/2;
	pts[1].x = x+width-5-height/2;   pts[1].y = y+5;
	pts[2].x = x+width-5-height*3/4; pts[2].y = y+height/2;
	pts[3].x = x+width-5-height/2;   pts[3].y = y+height-5;
	pts[4].x = x+width-5;            pts[4].y = y+height/2;
	gdk_draw_polygon(win, gc, TRUE, pts, 5);
	gdk_draw_polygon(win, gc, FALSE, pts, 5);
	gdk_draw_line(win, gc, x, y+height/2, x+width-5-height*3/4,y+height/2);
      }
      break;
    case ARROW_SLASHED_CROSS:
      if (arrow->left) {
        gdk_draw_line(win, gc, x+5,y+height/2, x+width,y+height/2); /*line*/
        gdk_draw_line(win, gc, x+5,y+height-5, x+height-5,y+5);   /*slash*/
        gdk_draw_line(win, gc, x+height/2,y+height-5, x+height/2,y+5); 
	/*cross */
      } else {
        gdk_draw_line(win, gc, x,y+height/2, x+width-5,y+height/2); /*line*/
        gdk_draw_line(win, gc, x+width-height/2-5,y+height-5, x+width-5,y+5);  
	/*slash*/
        gdk_draw_line(win, gc, x+width-height/2,y+height-5, x+width-height/2,
		      y+5);  /*cross*/
      }
      break;
    case ARROW_HALF_HEAD:
      if (arrow->left) {
	gdk_draw_line(win, gc, x+5,y+height/2, x+width,y+height/2);
	gdk_draw_line(win, gc, x+5,y+height/2, x+5+height/2,y+5);
      } else {
	gdk_draw_line(win, gc, x,y+height/2, x+width-5,y+height/2);
	gdk_draw_line(win, gc, x+width-5,y+height/2, x+width-5-height/2,
		      y+height-5);
      }
      break;
    case ARROW_FILLED_ELLIPSE:
      if (arrow->left) {
	gdk_draw_line(win,gc,x+5+8,y+height/2,x+width,y+height/2);
	gdk_draw_arc(win,gc,TRUE,x+5,y+height/2-6,12,12,0,64*360);
      } else {
	gdk_draw_line(win,gc,x,y+height/2,x+width-5-8,y+height/2);
	gdk_draw_arc(win,gc,TRUE,x+width-5-12,y+height/2-6,12,12,0,64*360);
      }
      break;
    case ARROW_HOLLOW_ELLIPSE:
      if (arrow->left) {
	gdk_draw_line(win,gc,x+5+8,y+height/2,x+width,y+height/2);
	gdk_draw_arc(win,gc,FALSE,x+5,y+height/2-4,8,8,0,64*360);
      } else {
	gdk_draw_line(win,gc,x,y+height/2,x+width-5-8,y+height/2);
	gdk_draw_arc(win,gc,FALSE,x+width-5-8,y+height/2-4,8,8,0,64*360);
      }
      break;
    case ARROW_DOUBLE_HOLLOW_TRIANGLE:
      if(arrow->left) {
      	pts[0].x = x+5;          pts[0].y = y+height/2;
	pts[1].x = x+5+height/2; pts[1].y = y+5;
	pts[2].x = x+5+height/2; pts[2].y = y+height-5;
	pts[3].x = x+5;          pts[3].y = y+height/2;
	gdk_draw_polygon(win, gc, FALSE, pts, 4);
     	pts[0].x = x+5+height/2; pts[0].y = y+height/2;
	pts[1].x = x+5+height;   pts[1].y = y+5;
	pts[2].x = x+5+height;   pts[2].y = y+height-5;
	pts[3].x = x+5+height/2; pts[3].y = y+height/2;
	gdk_draw_polygon(win, gc, FALSE, pts, 4);
	gdk_draw_line(win, gc, x+5+height, y+height/2, x+width, y+height/2);
      } else {
	pts[0].x = x+width-5;          pts[0].y = y+height/2;
	pts[1].x = x+width-5-height/2; pts[1].y = y+5;
	pts[2].x = x+width-5-height/2; pts[2].y = y+height-5;
	pts[3].x = x+width-5;          pts[3].y = y+height/2;
	gdk_draw_polygon(win, gc, FALSE, pts, 4);
	pts[0].x = x+width-5-height/2; pts[0].y = y+height/2;
	pts[1].x = x+width-5-height;   pts[1].y = y+5;
	pts[2].x = x+width-5-height;   pts[2].y = y+height-5;
	pts[3].x = x+width-5-height/2; pts[3].y = y+height/2;
	gdk_draw_polygon(win, gc, FALSE, pts, 4);
	gdk_draw_line(win, gc, x, y+height/2, x+width-5-height, y+height/2);
      }
      break;
    case ARROW_DOUBLE_FILLED_TRIANGLE:
      if(arrow->left) {
      	pts[0].x = x+5;          pts[0].y = y+height/2;
	pts[1].x = x+5+height/2; pts[1].y = y+5;
	pts[2].x = x+5+height/2; pts[2].y = y+height-5;
	pts[3].x = x+5;          pts[3].y = y+height/2;
	gdk_draw_polygon(win, gc, TRUE, pts, 4);
	gdk_draw_polygon(win, gc, FALSE, pts, 4);
      	pts[0].x = x+5+height/2; pts[0].y = y+height/2;
	pts[1].x = x+5+height; pts[1].y = y+5;
	pts[2].x = x+5+height; pts[2].y = y+height-5;
	pts[3].x = x+5+height/2; pts[3].y = y+height/2;
	gdk_draw_polygon(win, gc, TRUE, pts, 4);
	gdk_draw_polygon(win, gc, FALSE, pts, 4);
	gdk_draw_line(win, gc, x+5+height, y+height/2, x+width, y+height/2);
      } else {
	pts[0].x = x+width-5;          pts[0].y = y+height/2;
	pts[1].x = x+width-5-height/2; pts[1].y = y+5;
	pts[2].x = x+width-5-height/2; pts[2].y = y+height-5;
	pts[3].x = x+width-5;          pts[3].y = y+height/2;
	gdk_draw_polygon(win, gc, TRUE, pts, 4);
	gdk_draw_polygon(win, gc, FALSE, pts, 4);
	pts[0].x = x+width-5-height/2; pts[0].y = y+height/2;
	pts[1].x = x+width-5-height; pts[1].y = y+5;
	pts[2].x = x+width-5-height; pts[2].y = y+height-5;
	pts[3].x = x+width-5-height/2; pts[3].y = y+height/2;
	gdk_draw_polygon(win, gc, TRUE, pts, 4);
	gdk_draw_polygon(win, gc, FALSE, pts, 4);
	gdk_draw_line(win, gc, x, y+height/2, x+width-5-height, y+height/2);
      }
      break;    
    case ARROW_FILLED_DOT:
      if (arrow->left) {
	gdk_draw_line(win,gc,x+5+6,y+height/2,x+width,y+height/2);
        gdk_draw_line(win,gc,x+5+6,y,x+5+6,y+height);
	gdk_draw_arc(win,gc,TRUE,x+5,y+height/2-6,12,12,0,64*360);
      } else {
	gdk_draw_line(win,gc,x,y+height/2,x+width-5-6,y+height/2);
        gdk_draw_line(win,gc,x+width-5-6,y,x+width-5-6,y+height);
	gdk_draw_arc(win,gc,TRUE,x+width-5-12,y+height/2-6,12,12,0,64*360);
      }        
      break;
    case ARROW_BLANKED_DOT:
      if (arrow->left) {
	gdk_draw_line(win,gc,x+5+6,y+height/2,x+width,y+height/2);
        gdk_draw_line(win,gc,x+5+6,y,x+5+6,y+height);
	gdk_draw_arc(win,gc,FALSE,x+5,y+height/2-6,12,12,0,64*360);
      } else {
	gdk_draw_line(win,gc,x,y+height/2,x+width-5-6,y+height/2);
        gdk_draw_line(win,gc,x+width-5-6,y,x+width-5-6,y+height);
	gdk_draw_arc(win,gc,FALSE,x+width-5-12,y+height/2-6,12,12,0,64*360);
      }        
      break;
    case ARROW_DIMENSION_ORIGIN:
      if (arrow->left) {
	gdk_draw_line(win,gc,x+5+12,y+height/2,x+width,y+height/2);
        gdk_draw_line(win,gc,x+5+6,y,x+5+6,y+height);
	gdk_draw_arc(win,gc,FALSE,x+5,y+height/2-6,12,12,0,64*360);
      } else {
	gdk_draw_line(win,gc,x,y+height/2,x+width-5-12,y+height/2);
        gdk_draw_line(win,gc,x+width-5-6,y,x+width-5-6,y+height);
	gdk_draw_arc(win,gc,FALSE,x+width-5-12,y+height/2-6,12,12,0,64*360);
      }        
      break;
    case ARROW_FILLED_BOX:
      if (arrow->left) {
      	pts[0].x = x+5;          pts[0].y = y+height/2-6;
      	pts[1].x = x+5;          pts[1].y = y+height/2+6;
      	pts[2].x = x+5+12;       pts[2].y = y+height/2+6;
      	pts[3].x = x+5+12;       pts[3].y = y+height/2-6;        
	gdk_draw_line(win,gc,x+5+12,y+height/2,x+width,y+height/2);
        gdk_draw_line(win,gc,x+5+6,y,x+5+6,y+height);        
	gdk_draw_polygon(win, gc, TRUE, pts, 4);
      } else {
      	pts[0].x = x+width-5;          pts[0].y = y+height/2-6;
      	pts[1].x = x+width-5;          pts[1].y = y+height/2+6;
      	pts[2].x = x+width-5-12;       pts[2].y = y+height/2+6;
      	pts[3].x = x+width-5-12;       pts[3].y = y+height/2-6;        
	gdk_draw_line(win,gc,x,y+height/2,x+width-5-12,y+height/2);
        gdk_draw_line(win,gc,x+width-5-6,y,x+width-5-6,y+height);
	gdk_draw_polygon(win, gc, TRUE, pts, 4);
      }        
      break;
    case ARROW_BLANKED_BOX:
      if (arrow->left) {
      	pts[0].x = x+5;          pts[0].y = y+height/2-6;
      	pts[1].x = x+5;          pts[1].y = y+height/2+6;
      	pts[2].x = x+5+12;       pts[2].y = y+height/2+6;
      	pts[3].x = x+5+12;       pts[3].y = y+height/2-6;        
	gdk_draw_line(win,gc,x+5+12,y+height/2,x+width,y+height/2);
        gdk_draw_line(win,gc,x+5+6,y,x+5+6,y+height);        
	gdk_draw_polygon(win, gc, FALSE, pts, 4);
      } else {
      	pts[0].x = x+width-5;          pts[0].y = y+height/2-6;
      	pts[1].x = x+width-5;          pts[1].y = y+height/2+6;
      	pts[2].x = x+width-5-12;       pts[2].y = y+height/2+6;
      	pts[3].x = x+width-5-12;       pts[3].y = y+height/2-6;        
	gdk_draw_line(win,gc,x,y+height/2,x+width-5-12,y+height/2);
        gdk_draw_line(win,gc,x+width-5-6,y,x+width-5-6,y+height);
	gdk_draw_polygon(win, gc, FALSE, pts, 4);
      }        
      break;
    case ARROW_SLASH_ARROW:
      if (arrow->left) {
        gdk_draw_line(win, gc, x+height/2,y+height/2, x+width,y+height/2); /*line*/
        gdk_draw_line(win, gc, x+5,y+height-5, x+height-5,y+5);   /*slash*/
        gdk_draw_line(win, gc, x+height/2,y+height-4, x+height/2,y+4); 
	/*cross */
      } else {
        gdk_draw_line(win, gc, x,y+height/2, x+width-height/2,y+height/2); /*line*/
        gdk_draw_line(win, gc, x+width-height/2-5,y+height-5, x+width-5,y+5);  
	/*slash*/
        gdk_draw_line(win, gc, x+width-height/2,y+height-4, x+width-height/2,
		      y+4);  /*cross*/
      }
      break;
    case ARROW_INTEGRAL_SYMBOL:
      if (arrow->left) {
        gdk_draw_line(win, gc, x+height/2,y+height/2, x+width,y+height/2); /*line*/
	gdk_draw_arc(win,gc,FALSE,x+height/2-12,y+height/2-9,
                     12,12,64*270,64*60);
        gdk_draw_arc(win,gc,FALSE,x+height/2+1,y+height/2-5,
                     12,12,64*90,64*60);
        gdk_draw_line(win, gc, x+height/2,y+height-4, x+height/2,y+4); 
      } else {
        gdk_draw_line(win, gc, x,y+height/2, x+width-height/2,y+height/2); /*line*/
	gdk_draw_arc(win,gc,FALSE,x+width-height/2-13,y+height/2-8,
                     12,12,64*270,64*60);
        gdk_draw_arc(win,gc,FALSE,x+width-height/2-0,y+height/2-5,
                     12,12,64*90,64*60);
        gdk_draw_line(win, gc, x+width-height/2,y+height-4, x+width-height/2,
		      y+4);  /*cross*/
      }
      break;
    }    
    gdk_gc_set_line_attributes(gc, gcvalues.line_width, gcvalues.line_style,
			       gcvalues.cap_style, gcvalues.join_style);
  }
  return TRUE;
}


/* --------------- DiaLinePreview -------------------------------- */

#define DIA_LINE_PREVIEW(obj) (GTK_CHECK_CAST((obj),dia_line_preview_get_type(), DiaLinePreview))
#define DIA_LINE_PREVIEW_CLASS(obj) (GTK_CHECK_CLASS_CAST((obj), dia_line_preview_get_type(), DiaLinePreviewClass))

typedef struct _DiaLinePreview DiaLinePreview;
typedef struct _DiaLinePreviewClass DiaLinePreviewClass;

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

GtkType
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
  GTK_WIDGET (line)->requisition.height = 20 + GTK_MISC (line)->ypad * 2;

  
  line->lstyle = LINESTYLE_SOLID;
}

GtkWidget *
dia_line_preview_new (LineStyle lstyle)
{
  DiaLinePreview *line = gtk_type_new (dia_line_preview_get_type());

  line->lstyle = lstyle;
  return GTK_WIDGET(line);
}

void
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
      gdk_gc_set_line_attributes(gc, 3, GDK_LINE_SOLID,
				 gcvalues.cap_style, gcvalues.join_style);
      break;
    case LINESTYLE_DASHED:
      gdk_gc_set_line_attributes(gc, 3, GDK_LINE_ON_OFF_DASH,
				 gcvalues.cap_style, gcvalues.join_style);
      dash_list[0] = 10;
      dash_list[1] = 10;
      gdk_gc_set_dashes(gc, 0, dash_list, 2);
      break;
    case LINESTYLE_DASH_DOT:
      gdk_gc_set_line_attributes(gc, 3, GDK_LINE_ON_OFF_DASH,
				 gcvalues.cap_style, gcvalues.join_style);
      dash_list[0] = 10;
      dash_list[1] = 4;
      dash_list[2] = 2;
      dash_list[3] = 4;
      gdk_gc_set_dashes(gc, 0, dash_list, 4);
      break;
    case LINESTYLE_DASH_DOT_DOT:
      gdk_gc_set_line_attributes(gc, 3, GDK_LINE_ON_OFF_DASH,
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
      gdk_gc_set_line_attributes(gc, 3, GDK_LINE_ON_OFF_DASH,
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
gint close_and_hide(GtkWidget *wid, GdkEventAny *event) {
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
static void dia_arrow_chooser_change_arrow_type (GtkMenuItem *mi,
						 DiaArrowChooser *arrow);

GtkType
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

  arrow->dialog = wid = gtk_dialog_new();
  gtk_window_set_title(GTK_WINDOW(wid), _("Arrow Properties"));
  gtk_signal_connect(GTK_OBJECT(wid), "delete_event",
		     GTK_SIGNAL_FUNC(close_and_hide), NULL);

  wid = dia_arrow_selector_new();
  gtk_container_set_border_width(GTK_CONTAINER(wid), 5);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(arrow->dialog)->vbox), wid,
		     TRUE, TRUE, 0);
  gtk_widget_show(wid);
  arrow->selector = DIAARROWSELECTOR(wid);

  wid = gtk_button_new_with_label(_("OK"));
  GTK_WIDGET_SET_FLAGS(wid, GTK_CAN_DEFAULT);
  gtk_container_add(GTK_CONTAINER(GTK_DIALOG(arrow->dialog)->action_area),wid);
  gtk_widget_grab_default(wid);
  gtk_signal_connect_object(GTK_OBJECT(wid), "clicked",
			    GTK_SIGNAL_FUNC(dia_arrow_chooser_dialog_ok),
			    GTK_OBJECT(arrow));
  gtk_widget_show(wid);

  wid = gtk_button_new_with_label(_("Cancel"));
  GTK_WIDGET_SET_FLAGS(wid, GTK_CAN_DEFAULT);
  gtk_container_add(GTK_CONTAINER(GTK_DIALOG(arrow->dialog)->action_area),wid);
  gtk_signal_connect_object(GTK_OBJECT(wid), "clicked",
			    GTK_SIGNAL_FUNC(dia_arrow_chooser_dialog_cancel),
			    GTK_OBJECT(arrow));
  gtk_widget_show(wid);
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

  menu = gtk_menu_new();
  gtk_object_set_data_full(GTK_OBJECT(chooser), button_menu_key, menu,
			   (GtkDestroyNotify)gtk_widget_unref);
  for (i = 0; i <= ARROW_INTEGRAL_SYMBOL; i++) {
    mi = gtk_menu_item_new();
    gtk_object_set_data(GTK_OBJECT(mi), menuitem_enum_key, GINT_TO_POINTER(i));
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
  gtk_signal_connect_object(GTK_OBJECT(mi), "activate",
			    GTK_SIGNAL_FUNC(gtk_widget_show),
			    GTK_OBJECT(chooser->dialog));
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
  dia_arrow_selector_set_arrow(arrow->selector, arrow->arrow);
  gtk_widget_hide(arrow->dialog);
}

static void
dia_arrow_chooser_change_arrow_type(GtkMenuItem *mi, DiaArrowChooser *arrow)
{
  ArrowType atype = GPOINTER_TO_INT(gtk_object_get_data(GTK_OBJECT(mi),
							menuitem_enum_key));

  if (arrow->arrow.type != atype) {
    dia_arrow_preview_set(arrow->preview, atype, arrow->left);
    arrow->arrow.type = atype;
    dia_arrow_selector_set_arrow(arrow->selector, arrow->arrow);
    if (arrow->callback)
      (* arrow->callback)(arrow->arrow, arrow->user_data);
  }
}


/* ------- Code for DiaLineChooser ---------------------- */
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

GtkType
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

