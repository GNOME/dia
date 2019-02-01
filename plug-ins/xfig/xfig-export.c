/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * xfig-export.c: xfig export filter for dia
 * Copyright (C) 2001 Lars Clausen
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
#include "config.h"

#include <stdio.h>
/* Information used here is taken from the FIG Format 3.2 specification
   <URL:http://www.xfig.org/userman/fig-format.html>
   Some items left unspecified in the specifications are taken from the
   XFig source v. 3.2.3c
 */

#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <errno.h>
#include <locale.h>

#include <glib.h>
#include <glib/gstdio.h>

#include "intl.h"
#include "message.h"
#include "geometry.h"
#include "diarenderer.h"
#include "filter.h"
#include "object.h"
#include "properties.h"
#include "dia_image.h"
#include "group.h"
#include "diatransformrenderer.h"

#include "xfig.h"

#define WARNING_OUT_OF_COLORS 0
#define MAX_WARNING 1

#define XFIG_TYPE_RENDERER           (xfig_renderer_get_type ())
#define XFIG_RENDERER(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), XFIG_TYPE_RENDERER, XfigRenderer))
#define XFIG_RENDERER_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST ((klass), XFIG_TYPE_RENDERER, XfigRendererClass))
#define XFIG_IS_RENDERER(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), XFIG_TYPE_RENDERER))
#define XFIG_RENDERER_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), XFIG_TYPE_RENDERER, XfigRendererClass))
#define DTOSTR_BUF_SIZE G_ASCII_DTOSTR_BUF_SIZE
#define xfig_dtostr(buf,d) \
	g_ascii_formatd(buf, sizeof(buf), "%f", d)

GType xfig_renderer_get_type (void) G_GNUC_CONST;

typedef struct _XfigRenderer XfigRenderer;
typedef struct _XfigRendererClass XfigRendererClass;

struct _XfigRendererClass
{
  DiaRendererClass parent_class;
};

struct _XfigRenderer
{
  DiaRenderer parent_instance;

  FILE *file;

  int depth;

  real linewidth;
  LineCaps capsmode;
  LineJoin joinmode;
  LineStyle stylemode;
  real dashlength;
  FillStyle fillmode;
  DiaFont *font;
  real fontheight;

  gboolean color_pass;
  Color user_colors[512];
  int max_user_color;

  gchar *warnings[MAX_WARNING];
};

/* check whether there exists an arrow head */
static int hasArrow(Arrow *arrow)
{
  return (!arrow || ARROW_NONE==arrow->type) ? 0 : 1;
}

static void begin_render(DiaRenderer *self, const Rectangle *update);
static void end_render(DiaRenderer *renderer);
static void set_linewidth(DiaRenderer *self, real linewidth);
static void set_linecaps(DiaRenderer *self, LineCaps mode);
static void set_linejoin(DiaRenderer *self, LineJoin mode);
static void set_linestyle(DiaRenderer *self, LineStyle mode, real dash_length);
static void set_fillstyle(DiaRenderer *self, FillStyle mode);
static void set_font(DiaRenderer *self, DiaFont *font, real height);
static void draw_line(DiaRenderer *self, 
		      Point *start, Point *end, 
		      Color *color);
static void draw_polyline(DiaRenderer *self, 
			  Point *points, int num_points, 
			  Color *color);
static void draw_line_with_arrows(DiaRenderer *self, 
				  Point *start, Point *end, 
				  real line_width,
				  Color *color,
				  Arrow *start_arrow,
				  Arrow *end_arrow);
static void draw_polyline_with_arrows(DiaRenderer *self, 
				      Point *points, int num_points, 
				      real line_width,
				      Color *color,
				      Arrow *start_arrow,
				      Arrow *end_arrow);
static void draw_polygon(DiaRenderer *self, 
			 Point *points, int num_points, 
			 Color *fill, Color *stroke);
static void draw_rect(DiaRenderer *self, 
		      Point *ul_corner, Point *lr_corner,
		      Color *fill, Color *stroke);
static void draw_arc(DiaRenderer *self, 
		     Point *center,
		     real width, real height,
		     real angle1, real angle2,
		     Color *colour);
static void draw_arc_with_arrows(DiaRenderer *self, 
				 Point *startpoint,
				 Point *endpoint,
				 Point *midpoint,
				 real line_width,
				 Color *colour,
				 Arrow *start_arrow,
				 Arrow *end_arrow);
static void fill_arc(DiaRenderer *self, 
		     Point *center,
		     real width, real height,
		     real angle1, real angle2,
		     Color *colour);
static void draw_ellipse(DiaRenderer *self, 
			 Point *center,
			 real width, real height,
			 Color *fill, Color *stroke);
static void draw_bezier(DiaRenderer *self, 
			BezPoint *points,
			int numpoints,
			Color *colour);
static void draw_bezier_with_arrows(DiaRenderer *self, 
				    BezPoint *points,
				    int numpoints,
				    real line_width,
				    Color *colour,
				    Arrow *start_arrow,
				    Arrow *end_arrow);
static void draw_beziergon(DiaRenderer *self, 
			   BezPoint *points, /* Last point must be same as first point */
			   int numpoints,
			   Color *fill,
			   Color *stroke);
static void draw_string(DiaRenderer *self,
			const char *text,
			Point *pos, Alignment alignment,
			Color *colour);
static void draw_image(DiaRenderer *self,
		       Point *point,
		       real width, real height,
		       DiaImage *image);
static void draw_object(DiaRenderer *self,
			DiaObject *object,
			DiaMatrix *matrix);

static void xfig_renderer_class_init (XfigRendererClass *klass);

static gpointer parent_class = NULL;

GType
xfig_renderer_get_type (void)
{
  static GType object_type = 0;

  if (!object_type)
    {
      static const GTypeInfo object_info =
      {
        sizeof (XfigRendererClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) xfig_renderer_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (XfigRenderer),
        0,              /* n_preallocs */
	NULL            /* init */
      };

      object_type = g_type_register_static (DIA_TYPE_RENDERER,
                                            "XfigRenderer",
                                            &object_info, 0);
    }
  
  return object_type;
}

static void
xfig_renderer_finalize (GObject *object)
{
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
xfig_renderer_class_init (XfigRendererClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  DiaRendererClass *renderer_class = DIA_RENDERER_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize = xfig_renderer_finalize;

  renderer_class->begin_render = begin_render;
  renderer_class->end_render = end_render;

  renderer_class->set_linewidth = set_linewidth;
  renderer_class->set_linecaps = set_linecaps;
  renderer_class->set_linejoin = set_linejoin;
  renderer_class->set_linestyle = set_linestyle;
  renderer_class->set_fillstyle = set_fillstyle;
  renderer_class->set_font = set_font;
  
  renderer_class->draw_line = draw_line;
  renderer_class->draw_polyline = draw_polyline;
  
  renderer_class->draw_polygon = draw_polygon;

  renderer_class->draw_arc = draw_arc;
  renderer_class->fill_arc = fill_arc;

  renderer_class->draw_ellipse = draw_ellipse;

  renderer_class->draw_rect = draw_rect;
  renderer_class->draw_bezier = draw_bezier;
  renderer_class->draw_beziergon = draw_beziergon;

  renderer_class->draw_string = draw_string;

  renderer_class->draw_image = draw_image;

  renderer_class->draw_line_with_arrows = draw_line_with_arrows;
  renderer_class->draw_polyline_with_arrows = draw_polyline_with_arrows;
  renderer_class->draw_arc_with_arrows = draw_arc_with_arrows;
  renderer_class->draw_bezier_with_arrows = draw_bezier_with_arrows;
  renderer_class->draw_object = draw_object;
}

/* Helper functions */
static void 
figWarn(XfigRenderer *renderer, int warning) 
{
  if (renderer->warnings[warning]) {
    message_warning("%s", renderer->warnings[warning]);
    renderer->warnings[warning] = NULL;
  }
}

static int 
figLineStyle(XfigRenderer *renderer) 
{
  switch (renderer->stylemode) {
  case LINESTYLE_SOLID:
    return 0;
  case LINESTYLE_DASHED:
    return 1;
  case LINESTYLE_DASH_DOT:
    return 3;
  case LINESTYLE_DASH_DOT_DOT:
    return 4;
  case LINESTYLE_DOTTED:
    return 2;
  default:
    return 0;
  }
}

static int 
figLineWidth(XfigRenderer *renderer) 
{
  int width = 0;
  /* Minimal line width in fig diagrams. */
  if (renderer->linewidth <= 0.03175) width = 1;
  else width = (int)((renderer->linewidth / 2.54) * 80.0);
  return width;
}

/* Must be called before outputting anything that uses this color,
   in order to output a color pseudo object.
   The color pseudo object even must be placed first in the xfig file. 
 */
static void 
figCheckColor(XfigRenderer *renderer, Color *color) 
{
  int i;

  for (i = 0; i < FIG_MAX_DEFAULT_COLORS; i++) {
    if (color_equals(color, &fig_default_colors[i])) return;
  }
  for (i = 0; i < renderer->max_user_color; i++) {
    if (color_equals(color, &renderer->user_colors[i])) return;
  }
  if (renderer->max_user_color == FIG_MAX_USER_COLORS) {
    figWarn(renderer, WARNING_OUT_OF_COLORS);
    return;
  }
  renderer->user_colors[renderer->max_user_color] = *color;
  fprintf(renderer->file, "0 %d #%02x%02x%02x\n", 
          renderer->max_user_color + FIG_MAX_DEFAULT_COLORS,
          (int)(color->red*255), (int)(color->green*255), (int)(color->blue*255));
  renderer->max_user_color++;
}

static int 
figColor(XfigRenderer *renderer, Color *color) 
{
  int i;

  for (i = 0; i < FIG_MAX_DEFAULT_COLORS; i++) {
    if (color_equals(color, &fig_default_colors[i])) 
      return i;
  }
  for (i = 0; i < renderer->max_user_color; i++) {
    if (color_equals(color, &renderer->user_colors[i])) 
      return i + FIG_MAX_DEFAULT_COLORS;
  }
  return 0;
}

static real 
figCoord(XfigRenderer *renderer, real coord) 
{
  return (coord/2.54)*1200.0;
}

static real
figAltCoord(XfigRenderer *renderer, real coord)
{
  return (coord/2.54)*80.0;
}

static int 
figDepth(XfigRenderer *renderer) 
{
  return renderer->depth;
}

static real 
figDashLength(XfigRenderer *renderer) 
{
  return figAltCoord(renderer, renderer->dashlength);
}

static int 
figJoinStyle(XfigRenderer *renderer)
{
  return renderer->joinmode;
}

static int 
figCapsStyle(XfigRenderer *renderer) 
{
  return renderer->capsmode;
}

static int 
figAlignment(XfigRenderer *renderer, int alignment) 
{
  return alignment;
}

static int 
figFont(XfigRenderer *renderer) 
{
  int i;
  const char *legacy_name;

  /* FIXME: this is broken */
  legacy_name = dia_font_get_legacy_name(renderer->font);
  for (i = 0; fig_fonts[i] != NULL; i++) {
    if (!strcmp(legacy_name, fig_fonts[i]))
      return i;
  }

  return -1;
}

static real 
figFontSize(XfigRenderer *renderer) 
{
  return (renderer->fontheight/2.54)*72.27;
}

static guchar *
figText(XfigRenderer *renderer, const guchar *text) 
{
  int i, j;
  int len = strlen((char *) text);
  int newlen = len;
  char *returntext;
  for (i = 0; i < len; i++) {
    if (text[i] > 127) {
      newlen += 3;
    } else if ('\\'==text[i]) {
      newlen += 1;
    }
  }
  returntext = g_malloc(sizeof(char)*(newlen+1));
  j = 0;
  for (i = 0; i < len; i++, j++) {
    if (text[i] > 127) {
      sprintf(&returntext[j], "\\%03o", text[i]);
      j+=3;
    } else if ('\\'==text[i]) { /* backslash must be quoted in xfig */
      returntext[j++] = '\\';
      returntext[j]   = '\\';
    } else {
      returntext[j] = text[i];
    }
  }
  returntext[j] = 0;
  return (guchar *)returntext;
}

static void
figArrow(XfigRenderer *renderer, Arrow *arrow, real line_width)
{
  int type, style;
  gchar lw_buf[DTOSTR_BUF_SIZE];
  gchar aw_buf[DTOSTR_BUF_SIZE];
  gchar al_buf[DTOSTR_BUF_SIZE];
  
  switch (arrow->type) {
  case ARROW_NONE:
    return;
  case ARROW_LINES:             /* {open arrow} */
    type = 0; style = 0; break;
  case ARROW_UNFILLED_TRIANGLE:
  case ARROW_HOLLOW_TRIANGLE:   /* {blanked arrow} */
    type = 1; style = 0; break;
  case ARROW_FILLED_TRIANGLE:   /* {filled arrow} */
    type = 1; style = 1; break;
  case ARROW_HOLLOW_DIAMOND:   
    type = 3; style = 0; break;
  case ARROW_FILLED_DIAMOND:
    type = 3; style = 1; break;
  default:
    message_warning(_("Fig format has no equivalent of arrow style %s; using simple arrow.\n"), 
		    arrow_get_name_from_type(arrow->type));
    /* Notice fallthrough */
  case ARROW_FILLED_CONCAVE:
    type = 2; style = 1; break;
  case ARROW_BLANKED_CONCAVE:
    type = 2; style = 0; break;
  }
  fprintf(renderer->file, "  %d %d %s %s %s\n",
	  type, style,
	  xfig_dtostr(lw_buf, figAltCoord(renderer, line_width)), 
	  xfig_dtostr(aw_buf, figCoord(renderer, arrow->width)),
	  xfig_dtostr(al_buf, figCoord(renderer, arrow->length)) );	  
}

static void 
begin_render(DiaRenderer *self, const Rectangle *update) 
{
  XfigRenderer *renderer = XFIG_RENDERER(self);

  if (renderer->color_pass) {
    /* Set up warnings */
    renderer->warnings[WARNING_OUT_OF_COLORS] = 
      _("No more user-definable colors - using black");
    renderer->max_user_color = 0;
  }

  renderer->depth = 0;

  renderer->linewidth = 0;
  renderer->capsmode = 0;
  renderer->joinmode = 0;
  renderer->stylemode = 0;
  renderer->dashlength = 0;
  renderer->fillmode = 0;
  renderer->font = NULL;
  renderer->fontheight = 1;

}

static void 
end_render(DiaRenderer *renderer) 
{
}

static void 
set_linewidth(DiaRenderer *self, real linewidth) 
{
  XfigRenderer *renderer = XFIG_RENDERER(self);

  renderer->linewidth = linewidth;
}

static void 
set_linecaps(DiaRenderer *self, LineCaps mode) 
{
  XfigRenderer *renderer = XFIG_RENDERER(self);

  renderer->capsmode = mode;
}

static void 
set_linejoin(DiaRenderer *self, LineJoin mode) 
{
  XfigRenderer *renderer = XFIG_RENDERER(self);

  renderer->joinmode = mode;
}

static void 
set_linestyle(DiaRenderer *self, LineStyle mode, real dash_length)
{
  XfigRenderer *renderer = XFIG_RENDERER(self);

  renderer->stylemode = mode;
  renderer->dashlength = dash_length;
}

static void 
set_fillstyle(DiaRenderer *self, FillStyle mode) 
{
  XfigRenderer *renderer = XFIG_RENDERER(self);

  renderer->fillmode = mode;
}
static void 
set_font(DiaRenderer *self, DiaFont *font, real height) 
{
  XfigRenderer *renderer = XFIG_RENDERER(self);

  renderer->font = font;
  renderer->fontheight = height;
}

static void 
draw_line(DiaRenderer *self, 
          Point *start, Point *end, 
          Color *color) 
{
  XfigRenderer *renderer = XFIG_RENDERER(self);
  gchar d_buf[DTOSTR_BUF_SIZE];

  if (renderer->color_pass) {
    figCheckColor(renderer, color);
    return;
  }

  fprintf(renderer->file, "2 1 %d %d %d 0 %d 0 -1 %s %d %d 0 0 0 2\n",
	  figLineStyle(renderer), figLineWidth(renderer), 
	  figColor(renderer, color), figDepth(renderer),
	  xfig_dtostr(d_buf, figDashLength(renderer)),
	  figJoinStyle(renderer), figCapsStyle(renderer) );
  fprintf(renderer->file, "\t%d %d %d %d\n",
	  (int)figCoord(renderer, start->x), (int)figCoord(renderer, start->y), 
	  (int)figCoord(renderer, end->x), (int)figCoord(renderer, end->y));
}

static void 
draw_line_with_arrows(DiaRenderer *self, 
		      Point *start, Point *end, 
		      real line_width,
		      Color *color,
		      Arrow *start_arrow,
		      Arrow *end_arrow) 
{
  XfigRenderer *renderer = XFIG_RENDERER(self);
  gchar d_buf[DTOSTR_BUF_SIZE];

  if (renderer->color_pass) {
    figCheckColor(renderer, color);
    return;
  }

  fprintf(renderer->file, "2 1 %d %d %d 0 %d 0 -1 %s %d %d 0 %d %d 2\n",
	  figLineStyle(renderer), figLineWidth(renderer), 
	  figColor(renderer, color), figDepth(renderer),
	  xfig_dtostr(d_buf, figDashLength(renderer)),
	  figJoinStyle(renderer), 
	  figCapsStyle(renderer),
	  hasArrow(end_arrow),
	  hasArrow(start_arrow));
  if (hasArrow(end_arrow)) {
    figArrow(renderer, end_arrow, line_width);
  }
  if (hasArrow(start_arrow)) {
    figArrow(renderer, start_arrow, line_width);
  }
  fprintf(renderer->file, "\t%d %d %d %d\n",
	  (int)figCoord(renderer, start->x), (int)figCoord(renderer, start->y), 
	  (int)figCoord(renderer, end->x), (int)figCoord(renderer, end->y));
}

static void 
draw_polyline(DiaRenderer *self, 
              Point *points, int num_points, 
              Color *color) 
{
  int i;
  XfigRenderer *renderer = XFIG_RENDERER(self);
  gchar d_buf[DTOSTR_BUF_SIZE];

  if (renderer->color_pass) {
    figCheckColor(renderer, color);
    return;
  }

  fprintf(renderer->file, "2 1 %d %d %d 0 %d 0 -1 %s %d %d 0 0 0 %d\n",
	  figLineStyle(renderer), figLineWidth(renderer), 
	  figColor(renderer, color), figDepth(renderer),
	  xfig_dtostr(d_buf, figDashLength(renderer)),
	  figJoinStyle(renderer),
	  figCapsStyle(renderer), num_points);
  fprintf(renderer->file, "\t");
  for (i = 0; i < num_points; i++) {
    fprintf(renderer->file, "%d %d ",
	    (int)figCoord(renderer, points[i].x), (int)figCoord(renderer, points[i].y));
  }
  fprintf(renderer->file, "\n");
}

static void 
draw_polyline_with_arrows(DiaRenderer *self, 
			  Point *points, int num_points, 
			  real line_width,
			  Color *color,
			  Arrow *start_arrow,
			  Arrow *end_arrow) 
{
  int i;
  XfigRenderer *renderer = XFIG_RENDERER(self);
  gchar d_buf[DTOSTR_BUF_SIZE];

  if (renderer->color_pass) {
    figCheckColor(renderer, color);
    return;
  }

  fprintf(renderer->file, "2 1 %d %d %d 0 %d 0 -1 %s %d %d 0 %d %d %d\n",
	  figLineStyle(renderer), figLineWidth(renderer), 
	  figColor(renderer, color), figDepth(renderer),
	  xfig_dtostr(d_buf, figDashLength(renderer)),
	  figJoinStyle(renderer),
	  figCapsStyle(renderer),
	  hasArrow(end_arrow),
	  hasArrow(start_arrow), num_points);
  if (hasArrow(end_arrow)) {
    figArrow(renderer, end_arrow, line_width);
  }
  if (hasArrow(start_arrow)) {
    figArrow(renderer, start_arrow, line_width);
  }
  fprintf(renderer->file, "\t");
  for (i = 0; i < num_points; i++) {
    fprintf(renderer->file, "%d %d ",
	    (int)figCoord(renderer, points[i].x), (int)figCoord(renderer, points[i].y));
  }
  fprintf(renderer->file, "\n");
}

static void
draw_polygon(DiaRenderer *self,
	     Point *points, int num_points,
	     Color *fill, Color *stroke)
{
  int i;
  XfigRenderer *renderer = XFIG_RENDERER(self);
  gchar d_buf[DTOSTR_BUF_SIZE];

  if (renderer->color_pass) {
    if (stroke)
      figCheckColor(renderer, stroke);
    if (fill)
      figCheckColor(renderer, fill);
    return;
  }

  fprintf(renderer->file, "2 3 %d %d %d %d %d 0 %d %s %d %d 0 0 0 %d\n",
	  figLineStyle(renderer), stroke ? figLineWidth(renderer) : 0,
	  stroke ? figColor(renderer, stroke) : 0, fill ? figColor(renderer, fill) : 0,
	  figDepth(renderer),
	  fill ? 20 : -1,
	  xfig_dtostr(d_buf, figDashLength(renderer)),
	  figJoinStyle(renderer),
	  figCapsStyle(renderer), num_points+1);
  fprintf(renderer->file, "\t");
  for (i = 0; i < num_points; i++) {
    fprintf(renderer->file, "%d %d ",
	    (int)figCoord(renderer, points[i].x), (int)figCoord(renderer, points[i].y));
  }
  fprintf(renderer->file, "%d %d\n",
	  (int)figCoord(renderer, points[0].x), (int)figCoord(renderer, points[0].y));
}

static void
draw_rect(DiaRenderer *self,
          Point *ul_corner, Point *lr_corner,
          Color *fill, Color *stroke)
{
  XfigRenderer *renderer = XFIG_RENDERER(self);
  gchar d_buf[DTOSTR_BUF_SIZE];

  if (renderer->color_pass) {
    if (stroke)
      figCheckColor(renderer, stroke);
    if (fill)
      figCheckColor(renderer, fill);
    return;
  }

  fprintf(renderer->file, "2 3 %d %d %d %d %d -1 %d %s %d %d 0 0 0 5\n",
	  figLineStyle(renderer), stroke ? figLineWidth(renderer) : 0,
	  stroke ? figColor(renderer, stroke) : 0, fill ? figColor(renderer, fill) : 0,
	  figDepth(renderer),
	  fill ? 20 : -1,
	  xfig_dtostr(d_buf, figDashLength(renderer)),
	  figJoinStyle(renderer), 
	  figCapsStyle(renderer));
  fprintf(renderer->file, "\t%d %d %d %d %d %d %d %d %d %d\n",
	  (int)figCoord(renderer, ul_corner->x), (int)figCoord(renderer, ul_corner->y), 
	  (int)figCoord(renderer, lr_corner->x), (int)figCoord(renderer, ul_corner->y), 
	  (int)figCoord(renderer, lr_corner->x), (int)figCoord(renderer, lr_corner->y), 
	  (int)figCoord(renderer, ul_corner->x), (int)figCoord(renderer, lr_corner->y), 
	  (int)figCoord(renderer, ul_corner->x), (int)figCoord(renderer, ul_corner->y));
}
static void 
draw_arc(DiaRenderer *self, 
         Point *center,
         real width, real height,
         real angle1, real angle2,
         Color *color) 
{
  Point first, second, last;
  int direction = angle2 > angle1 ? 1 : 0; /* Dia not always gives counterclockwise */
  XfigRenderer *renderer = XFIG_RENDERER(self);
  gchar dl_buf[DTOSTR_BUF_SIZE];
  gchar cx_buf[DTOSTR_BUF_SIZE];
  gchar cy_buf[DTOSTR_BUF_SIZE];

  if (renderer->color_pass) {
    figCheckColor(renderer, color);
    return;
  }
  fprintf (renderer->file, "#draw_arc center=(%g,%g) radius=%g angle1=%g° angle2=%g°\n", center->x, center->y, (width+height)/4.0, angle1, angle2);
  /* adjust to radians */
  angle1 *= (M_PI/180.0);
  angle2 *= (M_PI/180.0);

  first = *center;
  first.x += (width/2.0)*cos(angle1);
  first.y -= (height/2.0)*sin(angle1);

  second = *center;
  second.x += (width/2.0)*cos((angle1+angle2)/2.0);
  second.y -= (height/2.0)*sin((angle1+angle2)/2.0);

  last = *center;
  last.x += (width/2.0)*cos(angle2);
  last.y -= (height/2.0)*sin(angle2);

  fprintf(renderer->file, "5 1 %d %d %d %d %d 0 -1 %s %d %d 0 0 %s %s %d %d %d %d %d %d\n",
	  figLineStyle(renderer), figLineWidth(renderer), 
	  figColor(renderer, color), figColor(renderer, color),
	  figDepth(renderer),
	  xfig_dtostr(dl_buf, figDashLength(renderer)),
	  figCapsStyle(renderer),
	  direction,
	  xfig_dtostr(cx_buf, figCoord(renderer, center->x)), 
	  xfig_dtostr(cy_buf, figCoord(renderer, center->y)),
	  (int)figCoord(renderer, first.x), (int)figCoord(renderer, first.y), 
	  (int)figCoord(renderer, second.x), (int)figCoord(renderer, second.y), 
	  (int)figCoord(renderer, last.x), (int)figCoord(renderer, last.y));

}

/* once more copied from lib/diarenderer.c (see also objects/standard/arc.c */
static gboolean
is_right_hand (const Point *a, const Point *b, const Point *c)
{
  Point dot1, dot2;

  dot1 = *a;
  point_sub(&dot1, c);
  point_normalize(&dot1);
  dot2 = *b;
  point_sub(&dot2, c);
  point_normalize(&dot2);
  return point_cross(&dot1, &dot2) > 0;
}

static void 
draw_arc_with_arrows(DiaRenderer *self, 
		     Point *startpoint,
		     Point *endpoint,
		     Point *midpoint,
		     real line_width,
		     Color *color,
		     Arrow *start_arrow,
		     Arrow *end_arrow) 
{
  Point center;
  int direction = 0;
  real radius = -1.0;
  XfigRenderer *renderer = XFIG_RENDERER(self);
  gchar dl_buf[DTOSTR_BUF_SIZE];
  gchar cx_buf[DTOSTR_BUF_SIZE];
  gchar cy_buf[DTOSTR_BUF_SIZE];

  if (renderer->color_pass) {
    figCheckColor(renderer, color);
    return;
  }

  center.x = 0.0;
  center.y = 0.0;
  direction = is_right_hand (startpoint, midpoint, endpoint) ? 0 : 1;
  if (!three_point_circle (startpoint, midpoint, endpoint, &center, &radius))
    message_warning("xfig: arc conversion failed");

  fprintf (renderer->file, "#draw_arc_with_arrows center=(%g,%g) radius=%g\n", center.x, center.y, radius);

  fprintf(renderer->file, "5 1 %d %d %d %d %d 0 -1 %s %d %d %d %d %s %s %d %d %d %d %d %d\n",
	  figLineStyle(renderer), figLineWidth(renderer), 
	  figColor(renderer, color), figColor(renderer, color),
	  figDepth(renderer),
	  xfig_dtostr(dl_buf, figDashLength(renderer)),
	  figCapsStyle(renderer),
	  direction,
	  hasArrow(end_arrow),
	  hasArrow(start_arrow),
	  xfig_dtostr(cx_buf, figCoord(renderer, center.x)),
	  xfig_dtostr(cy_buf, figCoord(renderer, center.y)),
	  (int)figCoord(renderer, startpoint->x), (int)figCoord(renderer, startpoint->y), 
	  (int)figCoord(renderer, midpoint->x), (int)figCoord(renderer, midpoint->y), 
	  (int)figCoord(renderer, endpoint->x), (int)figCoord(renderer, endpoint->y));

  if (hasArrow(end_arrow)) {
    figArrow(renderer, end_arrow, line_width);
  }
  if (hasArrow(start_arrow)) {
    figArrow(renderer, start_arrow, line_width);
  }
}

static void 
fill_arc(DiaRenderer *self, 
         Point *center,
         real width, real height,
         real angle1, real angle2,
         Color *color) 
{
  Point first, second, last;
  XfigRenderer *renderer = XFIG_RENDERER(self);
  gchar dl_buf[DTOSTR_BUF_SIZE];
  gchar cx_buf[DTOSTR_BUF_SIZE];
  gchar cy_buf[DTOSTR_BUF_SIZE];

  if (renderer->color_pass) {
    figCheckColor(renderer, color);
    return;
  }

  fprintf (renderer->file, "#fill_arc center=(%g,%g) radius=%g angle1=%g° angle2=%g°\n", center->x, center->y, (width+height)/4.0, angle1, angle2);
  /* adjust to radians */
  angle1 *= (M_PI/180.0);
  angle2 *= (M_PI/180.0);

  first = *center;
  first.x += (width/2.0)*cos(angle1);
  first.y -= (height/2.0)*sin(angle1);

  second = *center;
  second.x += (width/2.0)*cos((angle1+angle2)/2.0);
  second.y -= (height/2.0)*sin((angle1+angle2)/2.0);

  last = *center;
  last.x += (width/2.0)*cos(angle2);
  last.y -= (height/2.0)*sin(angle2);

  fprintf(renderer->file, "5 2 %d %d %d %d %d 20 0 %s %d 1 0 0 %s %s %d %d %d %d %d %d\n",
	  figLineStyle(renderer), figLineWidth(renderer), 
	  figColor(renderer, color), figColor(renderer, color),
	  figDepth(renderer),
	  xfig_dtostr(dl_buf, figDashLength(renderer)),
	  figCapsStyle(renderer),
	  xfig_dtostr(cx_buf, figCoord(renderer, center->x)), 
	  xfig_dtostr(cy_buf, figCoord(renderer, center->y)),
	  (int)figCoord(renderer, first.x), (int)figCoord(renderer, first.y), 
	  (int)figCoord(renderer, second.x), (int)figCoord(renderer, second.y), 
	  (int)figCoord(renderer, last.x), (int)figCoord(renderer, last.y));

}

static void
draw_ellipse (DiaRenderer *self,
	      Point *center,
	      real width, real height,
	      Color *fill, Color *stroke)
{
  XfigRenderer *renderer = XFIG_RENDERER(self);
  gchar d_buf[DTOSTR_BUF_SIZE];

  if (renderer->color_pass) {
    if (stroke)
      figCheckColor(renderer, stroke);
    if (fill)
      figCheckColor(renderer, fill);
    return;
  }

  fprintf(renderer->file, "1 1 %d %d %d %d %d 0 %d %s 1 0.0 %d %d %d %d 0 0 0 0\n",
	  figLineStyle(renderer), 
	  stroke ? figLineWidth(renderer) : 0,
	  stroke ? figColor(renderer, stroke) : 0,
	  fill ? figColor(renderer, fill) : 0,
	  figDepth(renderer),
	  fill ? 20 : -1,
	  xfig_dtostr(d_buf, figDashLength(renderer)),
	  (int)figCoord(renderer, center->x), (int)figCoord(renderer, center->y),
	  (int)figCoord(renderer, width/2), (int)figCoord(renderer, height/2));
}

static void 
draw_bezier(DiaRenderer *self, 
            BezPoint *points,
            int numpoints,
            Color *color) 
{
  XfigRenderer *renderer = XFIG_RENDERER(self);

  if (renderer->color_pass) {
    figCheckColor(renderer, color);
    return;
  }

  DIA_RENDERER_CLASS(parent_class)->draw_bezier(self, points, numpoints, color);
}

static void 
draw_bezier_with_arrows(DiaRenderer *self, 
			BezPoint *points,
			int numpoints,
			real line_width,
			Color *color,
			Arrow *start_arrow,
			Arrow *end_arrow) 
{
  XfigRenderer *renderer = XFIG_RENDERER(self);

  if (renderer->color_pass) {
    figCheckColor(renderer, color);
    return;
  }

  DIA_RENDERER_CLASS(parent_class)->draw_bezier_with_arrows(self, points, numpoints, line_width, color, start_arrow, end_arrow);
}

static void 
draw_beziergon (DiaRenderer *self, 
		BezPoint *points,
		int numpoints,
		Color *fill,
		Color *stroke) 
{
  XfigRenderer *renderer = XFIG_RENDERER(self);

  if (renderer->color_pass) {
    if (fill)
      figCheckColor(renderer, fill);
    if (stroke)
      figCheckColor(renderer, stroke);
    return;
  }

  DIA_RENDERER_CLASS(parent_class)->draw_beziergon(self, points, numpoints, fill, stroke);
}

static void 
draw_string(DiaRenderer *self,
            const char *text,
            Point *pos, Alignment alignment,
            Color *color) 
{
  guchar *figtext = NULL;
  XfigRenderer *renderer = XFIG_RENDERER(self);
  gchar d_buf[DTOSTR_BUF_SIZE];

  if (renderer->color_pass) {
    figCheckColor(renderer, color);
    return;
  }

  figtext = figText(renderer, (unsigned char *) text);
  /* xfig texts are specials */
  fprintf(renderer->file, "4 %d %d %d 0 %d %s 0.0 6 0.0 0.0 %d %d %s\\001\n",
	  figAlignment(renderer, alignment),
	  figColor(renderer, color), 
	  figDepth(renderer),
	  figFont(renderer),
	  xfig_dtostr(d_buf, figFontSize(renderer)),
	  (int)figCoord(renderer, pos->x),
	  (int)figCoord(renderer, pos->y),
	  figtext);
  g_free(figtext);
}

static void 
draw_image(DiaRenderer *self,
           Point *point,
           real width, real height,
           DiaImage *image) 
{
  XfigRenderer *renderer = XFIG_RENDERER(self);
  gchar d_buf[DTOSTR_BUF_SIZE];

  if (renderer->color_pass)
    return;

  fprintf(renderer->file, "2 5 %d 0 -1 0 %d 0 -1 %s %d %d 0 0 0 5\n",
	  figLineStyle(renderer), figDepth(renderer),
	  xfig_dtostr(d_buf, figDashLength(renderer)),
	  figJoinStyle(renderer), 
	  figCapsStyle(renderer));
  fprintf(renderer->file, "\t0 %s\n", dia_image_filename(image));
  fprintf(renderer->file, "\t%d %d %d %d %d %d %d %d %d %d\n",
	  (int)figCoord(renderer, point->x), (int)figCoord(renderer, point->y), 
	  (int)figCoord(renderer, point->x+width), (int)figCoord(renderer, point->y), 
	  (int)figCoord(renderer, point->x+width), (int)figCoord(renderer, point->y+height), 
	  (int)figCoord(renderer, point->x), (int)figCoord(renderer, point->y+height), 
	  (int)figCoord(renderer, point->x), (int)figCoord(renderer, point->y));
}

static void 
draw_object(DiaRenderer *self,
            DiaObject   *object,
	    DiaMatrix   *matrix)
{
  XfigRenderer *renderer = XFIG_RENDERER(self);

  if (renderer->color_pass) {
    /* color pass does not need transformation */
    object->ops->draw(object, DIA_RENDERER(renderer));
  } else {
    fprintf(renderer->file, "6 0 0 0 0\n");
    if (matrix) {
      DiaRenderer *tr = dia_transform_renderer_new (self);
      DIA_RENDERER_GET_CLASS(tr)->draw_object (tr, object, matrix);
      g_object_unref (tr);
    } else {
      object->ops->draw(object, DIA_RENDERER(renderer));
    }
    fprintf(renderer->file, "-6\n");
  }
}

static gboolean
export_fig(DiagramData *data, DiaContext *ctx,
	   const gchar *filename, const gchar *diafilename,
	   void* user_data)
{
  FILE *file;
  XfigRenderer *renderer;
  int i;
  Layer *layer;
  gchar d_buf[DTOSTR_BUF_SIZE];

  file = g_fopen(filename, "w");

  if (file == NULL) {
    dia_context_add_message_with_errno (ctx, errno, _("Can't open output file %s"), 
					dia_context_get_filename(ctx));
    return FALSE;
  }

  renderer = g_object_new(XFIG_TYPE_RENDERER, NULL);

  renderer->file = file;

  fprintf(file, "#FIG 3.2\n");
  fprintf(file, (data->paper.is_portrait?"Portrait\n":"Landscape\n"));
  fprintf(file, "Center\n");
  fprintf(file, "Metric\n");
  fprintf(file, "%s\n", data->paper.name);
  fprintf(file, "%s\n", xfig_dtostr(d_buf, data->paper.scaling*100.0));
  fprintf(file, "Single\n"); /* Could we do layers this way? */
  fprintf(file, "-2\n");
  fprintf(file, "1200 2\n");

  renderer->color_pass = TRUE;

  DIA_RENDERER_GET_CLASS(renderer)->begin_render(DIA_RENDERER(renderer), NULL);
  
  for (i=0; i<data->layers->len; i++) {
    layer = (Layer *) g_ptr_array_index(data->layers, i);
    if (layer->visible) {
      layer_render(layer, DIA_RENDERER(renderer), NULL, NULL, data, 0);
      renderer->depth++;
    }
  }
  
  DIA_RENDERER_GET_CLASS(renderer)->end_render(DIA_RENDERER(renderer));

  renderer->color_pass = FALSE;

  DIA_RENDERER_GET_CLASS(renderer)->begin_render(DIA_RENDERER(renderer), NULL);
  
  for (i=0; i<data->layers->len; i++) {
    layer = (Layer *) g_ptr_array_index(data->layers, i);
    if (layer->visible) {
      layer_render(layer, DIA_RENDERER(renderer), NULL, NULL, data, 0);
      renderer->depth++;
    }
  }
  
  DIA_RENDERER_GET_CLASS(renderer)->end_render(DIA_RENDERER(renderer));

  g_object_unref(renderer);

  fclose(file);

  return TRUE;
}

static const gchar *extensions[] = { "fig", NULL };
DiaExportFilter xfig_export_filter = {
  N_("Xfig format"),
  extensions,
  export_fig
};
