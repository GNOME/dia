#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
/* Information used here is taken from the FIG Format 3.2 specification
   <URL:http://www.xfig.org/userman/fig-format.html>
   Some items left unspecified in the specifications are taken from the
   XFig source v. 3.2.3c
 */

#include <string.h>
#include <math.h>
#include <glib.h>
#include <stdlib.h>
#include <errno.h>
#include <locale.h>

#include "intl.h"
#include "message.h"
#include "geometry.h"
#include "diarenderer.h"
#include "filter.h"
#include "object.h"
#include "properties.h"
#include "dia_image.h"
#include "group.h"

#include "xfig.h"

#define WARNING_OUT_OF_COLORS 0
#define MAX_WARNING 1

#define XFIG_TYPE_RENDERER           (xfig_renderer_get_type ())
#define XFIG_RENDERER(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), XFIG_TYPE_RENDERER, XfigRenderer))
#define XFIG_RENDERER_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST ((klass), XFIG_TYPE_RENDERER, XfigRendererClass))
#define XFIG_IS_RENDERER(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), XFIG_TYPE_RENDERER))
#define XFIG_RENDERER_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), XFIG_TYPE_RENDERER, XfigRendererClass))

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

static void begin_render(DiaRenderer *self);
static void end_render(DiaRenderer *renderer);
static void set_linewidth(DiaRenderer *self, real linewidth);
static void set_linecaps(DiaRenderer *self, LineCaps mode);
static void set_linejoin(DiaRenderer *self, LineJoin mode);
static void set_linestyle(DiaRenderer *self, LineStyle mode);
static void set_dashlength(DiaRenderer *self, real length);
static void set_fillstyle(DiaRenderer *self, FillStyle mode);
static void set_font(DiaRenderer *self, DiaFont *font, real height);
static void draw_line(DiaRenderer *self, 
		      Point *start, Point *end, 
		      Color *color);
static void draw_polyline(DiaRenderer *self, 
			  Point *points, int num_points, 
			  Color *color);
static void draw_polygon(DiaRenderer *self, 
			 Point *points, int num_points, 
			 Color *color);
static void fill_polygon(DiaRenderer *self, 
			 Point *points, int num_points, 
			 Color *color);
static void draw_rect(DiaRenderer *self, 
		      Point *ul_corner, Point *lr_corner,
		      Color *colour);
static void fill_rect(DiaRenderer *self, 
		      Point *ul_corner, Point *lr_corner,
		      Color *colour);
static void draw_arc(DiaRenderer *self, 
		     Point *center,
		     real width, real height,
		     real angle1, real angle2,
		     Color *colour);
static void fill_arc(DiaRenderer *self, 
		     Point *center,
		     real width, real height,
		     real angle1, real angle2,
		     Color *colour);
static void draw_ellipse(DiaRenderer *self, 
			 Point *center,
			 real width, real height,
			 Color *colour);
static void fill_ellipse(DiaRenderer *self, 
			 Point *center,
			 real width, real height,
			 Color *colour);
static void draw_bezier(DiaRenderer *self, 
			BezPoint *points,
			int numpoints,
			Color *colour);
static void fill_bezier(DiaRenderer *self, 
			BezPoint *points, /* Last point must be same as first point */
			int numpoints,
			Color *colour);
static void draw_string(DiaRenderer *self,
			const char *text,
			Point *pos, Alignment alignment,
			Color *colour);
static void draw_image(DiaRenderer *self,
		       Point *point,
		       real width, real height,
		       DiaImage image);
static void draw_object(DiaRenderer *self,
			Object *object);

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
  XfigRenderer *xfig_renderer = XFIG_RENDERER (object);

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
  renderer_class->set_dashlength = set_dashlength;
  renderer_class->set_fillstyle = set_fillstyle;
  renderer_class->set_font = set_font;
  
  renderer_class->draw_line = draw_line;
  renderer_class->draw_polyline = draw_polyline;
  
  renderer_class->draw_polygon = draw_polygon;
  renderer_class->fill_polygon = fill_polygon;

  renderer_class->draw_rect = draw_rect;
  renderer_class->fill_rect = fill_rect;

  renderer_class->draw_arc = draw_arc;
  renderer_class->fill_arc = fill_arc;

  renderer_class->draw_ellipse = draw_ellipse;
  renderer_class->fill_ellipse = fill_ellipse;

  renderer_class->draw_bezier = draw_bezier;
  renderer_class->fill_bezier = fill_bezier;

  renderer_class->draw_string = draw_string;

  renderer_class->draw_image = draw_image;

  renderer_class->draw_object = draw_object;
}

/* Helper functions */
static void 
figWarn(XfigRenderer *renderer, int warning) 
{
  if (renderer->warnings[warning]) {
    message_warning(renderer->warnings[warning]);
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
  return (int)((renderer->linewidth / 2.54) * 80.0);
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

static int 
figDepth(XfigRenderer *renderer) 
{
  return renderer->depth;
}

static real 
figDashLength(XfigRenderer *renderer) 
{
  return (renderer->dashlength / 2.54) * 80.0;
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
figCoord(XfigRenderer *renderer, real coord) 
{
  return (int)((coord/2.54)*1200.0);
}

static real 
figFloatCoord(XfigRenderer *renderer, real coord) 
{
  return (coord/2.54)*1200.0;
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
  int len = strlen(text);
  int newlen = len;
  guchar *returntext;
  for (i = 0; i < len; i++) {
    if (text[i] > 127) {
      newlen += 3;
    }
  }
  returntext = g_malloc(sizeof(char)*(newlen+1));
  j = 0;
  for (i = 0; i < len; i++, j++) {
    if (text[i] > 127) {
      sprintf(&returntext[j], "\\%03o", text[i]);
      j+=3;
    } else {
      returntext[j] = text[i];
    }
  }
  returntext[j] = 0;
  return returntext;
}

static void 
begin_render(DiaRenderer *self) 
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
set_linestyle(DiaRenderer *self, LineStyle mode) 
{
  XfigRenderer *renderer = XFIG_RENDERER(self);

  renderer->stylemode = mode;
}

static void 
set_dashlength(DiaRenderer *self, real length) 
{
  XfigRenderer *renderer = XFIG_RENDERER(self);

  renderer->dashlength = length;
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

  if (renderer->color_pass) {
    figCheckColor(renderer, color);
    return;
  }

  fprintf(renderer->file, "2 1 %d %d %d 0 %d 0 -1 %f %d %d 0 0 0 2\n",
	  figLineStyle(renderer), figLineWidth(renderer), 
	  figColor(renderer, color), figDepth(renderer),
	  figDashLength(renderer), figJoinStyle(renderer), 
	  figCapsStyle(renderer));
  fprintf(renderer->file, "\t%d %d %d %d\n",
	  figCoord(renderer, start->x), figCoord(renderer, start->y), 
	  figCoord(renderer, end->x), figCoord(renderer, end->y));
}

static void 
draw_polyline(DiaRenderer *self, 
              Point *points, int num_points, 
              Color *color) 
{
  int i;
  XfigRenderer *renderer = XFIG_RENDERER(self);

  if (renderer->color_pass) {
    figCheckColor(renderer, color);
    return;
  }

  fprintf(renderer->file, "2 1 %d %d %d 0 %d 0 -1 %f %d %d 0 0 0 %d\n",
	  figLineStyle(renderer), figLineWidth(renderer), 
	  figColor(renderer, color), figDepth(renderer),
	  figDashLength(renderer), figJoinStyle(renderer),
	  figCapsStyle(renderer), num_points);
  fprintf(renderer->file, "\t");
  for (i = 0; i < num_points; i++) {
    fprintf(renderer->file, "%d %d ",
	    figCoord(renderer, points[i].x), figCoord(renderer, points[i].y));
  }
  fprintf(renderer->file, "\n");
}

static void 
draw_polygon(DiaRenderer *self, 
             Point *points, int num_points, 
             Color *color) 
{
  int i;
  XfigRenderer *renderer = XFIG_RENDERER(self);

  if (renderer->color_pass) {
    figCheckColor(renderer, color);
    return;
  }

  fprintf(renderer->file, "2 3 %d %d %d 0 %d 0 -1 %f %d %d 0 0 0 %d\n",
	  figLineStyle(renderer), figLineWidth(renderer), 
	  figColor(renderer, color), figDepth(renderer),
	  figDashLength(renderer), figJoinStyle(renderer),
	  figCapsStyle(renderer), num_points+1);
  fprintf(renderer->file, "\t");
  for (i = 0; i < num_points; i++) {
    fprintf(renderer->file, "%d %d ",
	    figCoord(renderer, points[i].x), figCoord(renderer, points[i].y));
  }
  fprintf(renderer->file, "%d %d\n",
	  figCoord(renderer, points[0].x), figCoord(renderer, points[0].y));
}

static void 
fill_polygon(DiaRenderer *self, 
             Point *points, int num_points, 
             Color *color) 
{
  int i;
  XfigRenderer *renderer = XFIG_RENDERER(self);

  if (renderer->color_pass) {
    figCheckColor(renderer, color);
    return;
  }

  fprintf(renderer->file, "2 3 %d 0 %d %d %d 0 20 %f %d %d 0 0 0 %d\n",
	  figLineStyle(renderer), 
	  figColor(renderer, color), figColor(renderer, color),
	  figDepth(renderer),
	  figDashLength(renderer), figJoinStyle(renderer),
	  figCapsStyle(renderer), num_points+1);
  fprintf(renderer->file, "\t");
  for (i = 0; i < num_points; i++) {
    fprintf(renderer->file, "%d %d ",
	    figCoord(renderer, points[i].x), figCoord(renderer, points[i].y));
  }
  fprintf(renderer->file, "%d %d\n",
	  figCoord(renderer, points[0].x), figCoord(renderer, points[0].y));
}

static void 
draw_rect(DiaRenderer *self, 
          Point *ul_corner, Point *lr_corner,
          Color *color) 
{
  XfigRenderer *renderer = XFIG_RENDERER(self);

  if (renderer->color_pass) {
    figCheckColor(renderer, color);
    return;
  }

  fprintf(renderer->file, "2 3 %d %d %d 0 %d 0 -1 %f %d %d 0 0 0 5\n",
	  figLineStyle(renderer), figLineWidth(renderer), 
	  figColor(renderer, color), figDepth(renderer),
	  figDashLength(renderer), figJoinStyle(renderer), 
	  figCapsStyle(renderer));
  fprintf(renderer->file, "\t%d %d %d %d %d %d %d %d %d %d\n",
	  figCoord(renderer, ul_corner->x), figCoord(renderer, ul_corner->y), 
	  figCoord(renderer, lr_corner->x), figCoord(renderer, ul_corner->y), 
	  figCoord(renderer, lr_corner->x), figCoord(renderer, lr_corner->y), 
	  figCoord(renderer, ul_corner->x), figCoord(renderer, lr_corner->y), 
	  figCoord(renderer, ul_corner->x), figCoord(renderer, ul_corner->y));
}

static void 
fill_rect(DiaRenderer *self, 
          Point *ul_corner, Point *lr_corner,
          Color *color) 
{
  XfigRenderer *renderer = XFIG_RENDERER(self);

  if (renderer->color_pass) {
    figCheckColor(renderer, color);
    return;
  }

  fprintf(renderer->file, "2 3 %d %d %d %d %d -1 20 %f %d %d 0 0 0 5\n",
	  figLineStyle(renderer), figLineWidth(renderer), 
	  figColor(renderer, color), figColor(renderer, color),
	  figDepth(renderer),
	  figDashLength(renderer), figJoinStyle(renderer), 
	  figCapsStyle(renderer));
  fprintf(renderer->file, "\t%d %d %d %d %d %d %d %d %d %d\n",
	  figCoord(renderer, ul_corner->x), figCoord(renderer, ul_corner->y), 
	  figCoord(renderer, lr_corner->x), figCoord(renderer, ul_corner->y), 
	  figCoord(renderer, lr_corner->x), figCoord(renderer, lr_corner->y), 
	  figCoord(renderer, ul_corner->x), figCoord(renderer, lr_corner->y), 
	  figCoord(renderer, ul_corner->x), figCoord(renderer, ul_corner->y));
}

static void 
draw_arc(DiaRenderer *self, 
         Point *center,
         real width, real height,
         real angle1, real angle2,
         Color *color) 
{
  Point first, second, last;
  XfigRenderer *renderer = XFIG_RENDERER(self);

  if (renderer->color_pass) {
    figCheckColor(renderer, color);
    return;
  }

  first = *center;
  first.x += (width/2.0)*cos(angle1);
  first.y -= (height/2.0)*sin(angle1);

  second = *center;
  second.x += (width/2.0)*cos(angle1+(angle2-angle1)/2.0);
  second.y -= (height/2.0)*sin(angle1+(angle2-angle1)/2.0);

  last = *center;
  last.x += (width/2.0)*cos(angle2);
  last.y -= (height/2.0)*sin(angle2);

  fprintf(renderer->file, "5 1 %d %d %d %d %d 0 -1 %f %d 1 0 0 %f %f %d %d %d %d %d %d\n",
	  figLineStyle(renderer), figLineWidth(renderer), 
	  figColor(renderer, color), figColor(renderer, color),
	  figDepth(renderer),
	  figDashLength(renderer),
	  figCapsStyle(renderer),
	  figFloatCoord(renderer, center->x), 
	  figFloatCoord(renderer, center->y),
	  figCoord(renderer, first.x), figCoord(renderer, first.y), 
	  figCoord(renderer, second.x), figCoord(renderer, second.y), 
	  figCoord(renderer, last.x), figCoord(renderer, last.y));

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

  if (renderer->color_pass) {
    figCheckColor(renderer, color);
    return;
  }

  first = *center;
  first.x += (width/2.0)*cos(angle1);
  first.y -= (height/2.0)*sin(angle1);

  second = *center;
  second.x += (width/2.0)*cos(angle1+(angle2-angle1)/2.0);
  second.y -= (height/2.0)*sin(angle1+(angle2-angle1)/2.0);

  last = *center;
  last.x += (width/2.0)*cos(angle2);
  last.y -= (height/2.0)*sin(angle2);

  fprintf(renderer->file, "5 1 %d %d %d %d %d 20 0 %f %d 1 0 0 %f %f %d %d %d %d %d %d\n",
	  figLineStyle(renderer), figLineWidth(renderer), 
	  figColor(renderer, color), figColor(renderer, color),
	  figDepth(renderer),
	  figDashLength(renderer),
	  figCapsStyle(renderer),
	  figFloatCoord(renderer, center->x), 
	  figFloatCoord(renderer, center->y),
	  figCoord(renderer, first.x), figCoord(renderer, first.y), 
	  figCoord(renderer, second.x), figCoord(renderer, second.y), 
	  figCoord(renderer, last.x), figCoord(renderer, last.y));

}

static void 
draw_ellipse(DiaRenderer *self, 
             Point *center,
             real width, real height,
             Color *color) 
{
  XfigRenderer *renderer = XFIG_RENDERER(self);

  if (renderer->color_pass) {
    figCheckColor(renderer, color);
    return;
  }

  fprintf(renderer->file, "1 1 %d %d %d -1 %d 0 -1 %f 1 0.0 %d %d %d %d 0 0 0 0\n",
	  figLineStyle(renderer), 
	  figLineWidth(renderer), 
	  figColor(renderer, color),
	  figDepth(renderer),
	  figDashLength(renderer),
	  figCoord(renderer, center->x), figCoord(renderer, center->y),
	  figCoord(renderer, width/2), figCoord(renderer, height/2));

}

static void 
fill_ellipse(DiaRenderer *self, 
             Point *center,
             real width, real height,
             Color *color) 
{
  XfigRenderer *renderer = XFIG_RENDERER(self);

  if (renderer->color_pass) {
    figCheckColor(renderer, color);
    return;
  }

  fprintf(renderer->file, "1 1 %d %d %d %d %d 0 20 %f 1 0.0 %d %d %d %d 0 0 0 0\n",
	  figLineStyle(renderer), 
	  figLineWidth(renderer), 
	  figColor(renderer, color),
	  figColor(renderer, color),
	  figDepth(renderer),
	  figDashLength(renderer),
	  figCoord(renderer, center->x), figCoord(renderer, center->y),
	  figCoord(renderer, width/2), figCoord(renderer, height/2));
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
fill_bezier(DiaRenderer *self, 
            BezPoint *points, /* Last point must be same as first point */
            int numpoints,
            Color *color) 
{
  XfigRenderer *renderer = XFIG_RENDERER(self);

  if (renderer->color_pass) {
    figCheckColor(renderer, color);
    return;
  }

  DIA_RENDERER_CLASS(parent_class)->fill_bezier(self, points, numpoints, color);
}

static void 
draw_string(DiaRenderer *self,
            const char *text,
            Point *pos, Alignment alignment,
            Color *color) 
{
  guchar *figtext = NULL;
  XfigRenderer *renderer = XFIG_RENDERER(self);

  if (renderer->color_pass) {
    figCheckColor(renderer, color);
    return;
  }

  figtext = figText(renderer, text);
  fprintf(renderer->file, "4 %d %d %d 0 %d %f 0.0 4 0.0 0.0 %d %d %s\\001\n",
	  figAlignment(renderer, alignment),
	  figColor(renderer, color), 
	  figDepth(renderer),
	  figFont(renderer),
	  figFontSize(renderer),
	  figCoord(renderer, pos->x),
	  figCoord(renderer, pos->y),
	  figtext);
  g_free(figtext);
}

static void 
draw_image(DiaRenderer *self,
           Point *point,
           real width, real height,
           DiaImage image) 
{
  XfigRenderer *renderer = XFIG_RENDERER(self);

  if (renderer->color_pass)
    return;

  fprintf(renderer->file, "2 5 %d 0 -1 0 %d 0 -1 %f %d %d 0 0 0 5\n",
	  figLineStyle(renderer), figDepth(renderer),
	  figDashLength(renderer), figJoinStyle(renderer), 
	  figCapsStyle(renderer));
  fprintf(renderer->file, "\t0 %s\n", dia_image_filename(image));
  fprintf(renderer->file, "\t%d %d %d %d %d %d %d %d %d %d\n",
	  figCoord(renderer, point->x), figCoord(renderer, point->y), 
	  figCoord(renderer, point->x+width), figCoord(renderer, point->y), 
	  figCoord(renderer, point->x+width), figCoord(renderer, point->y+height), 
	  figCoord(renderer, point->x), figCoord(renderer, point->y+height), 
	  figCoord(renderer, point->x), figCoord(renderer, point->y));
}

static void 
draw_object(DiaRenderer *self,
            Object *object) 
{
  XfigRenderer *renderer = XFIG_RENDERER(self);

  if (!renderer->color_pass)
    fprintf(renderer->file, "6 0 0 0 0\n");
  object->ops->draw(object, DIA_RENDERER(renderer));
  if (!renderer->color_pass)
    fprintf(renderer->file, "-6\n");
}

static void
export_fig(DiagramData *data, const gchar *filename, 
           const gchar *diafilename, void* user_data)
{
  FILE *file;
  XfigRenderer *renderer;
  int i;
  Layer *layer;

  file = fopen(filename, "w");

  if (file == NULL) {
    message_error(_("Can't open output file %s: %s\n"), filename, strerror(errno));
    return;
  }

  renderer = g_object_new(XFIG_TYPE_RENDERER, NULL);

  renderer->file = file;

  fprintf(file, "#FIG 3.2\n");
  fprintf(file, (data->paper.is_portrait?"Portrait\n":"Landscape\n"));
  fprintf(file, "Center\n");
  fprintf(file, "Metric\n");
  fprintf(file, "%s\n", data->paper.name);
  fprintf(file, "%f\n", data->paper.scaling*100.0);
  fprintf(file, "Single\n"); /* Could we do layers this way? */
  fprintf(file, "-2\n");
  fprintf(file, "1200 2\n");

  renderer->color_pass = TRUE;

  DIA_RENDERER_GET_CLASS(renderer)->begin_render(DIA_RENDERER(renderer));
  
  for (i=0; i<data->layers->len; i++) {
    layer = (Layer *) g_ptr_array_index(data->layers, i);
    layer_render(layer, DIA_RENDERER(renderer), NULL, NULL, data, 0);
    renderer->depth++;
  }
  
  DIA_RENDERER_GET_CLASS(renderer)->end_render(DIA_RENDERER(renderer));

  renderer->color_pass = FALSE;

  DIA_RENDERER_GET_CLASS(renderer)->begin_render(DIA_RENDERER(renderer));
  
  for (i=0; i<data->layers->len; i++) {
    layer = (Layer *) g_ptr_array_index(data->layers, i);
    layer_render(layer, DIA_RENDERER(renderer), NULL, NULL, data, 0);
    renderer->depth++;
  }
  
  DIA_RENDERER_GET_CLASS(renderer)->end_render(DIA_RENDERER(renderer));

  g_object_unref(renderer);

  fclose(file);
}

static const gchar *extensions[] = { "fig", NULL };
DiaExportFilter xfig_export_filter = {
  N_("XFig format"),
  extensions,
  export_fig
};
