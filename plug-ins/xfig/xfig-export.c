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
#include "render.h"
#include "filter.h"
#include "object.h"
#include "properties.h"
#include "../app/group.h"

#include "xfig.h"

#define WARNING_OUT_OF_COLORS 0
#define MAX_WARNING 1

typedef struct _Rendererfig Rendererfig;
struct _Rendererfig {
  Renderer renderer;

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

  Color user_colors[512];
  int max_user_color;

  gchar *warnings[MAX_WARNING];
};

static void begin_render(Rendererfig *renderer, DiagramData *data);
static void end_render(Rendererfig *renderer);
static void set_linewidth(Rendererfig *renderer, real linewidth);
static void set_linecaps(Rendererfig *renderer, LineCaps mode);
static void set_linejoin(Rendererfig *renderer, LineJoin mode);
static void set_linestyle(Rendererfig *renderer, LineStyle mode);
static void set_dashlength(Rendererfig *renderer, real length);
static void set_fillstyle(Rendererfig *renderer, FillStyle mode);
static void set_font(Rendererfig *renderer, DiaFont *font, real height);
static void draw_line(Rendererfig *renderer, 
		      Point *start, Point *end, 
		      Color *color);
static void draw_polyline(Rendererfig *renderer, 
			  Point *points, int num_points, 
			  Color *color);
static void draw_polygon(Rendererfig *renderer, 
			 Point *points, int num_points, 
			 Color *color);
static void fill_polygon(Rendererfig *renderer, 
			 Point *points, int num_points, 
			 Color *color);
static void draw_rect(Rendererfig *renderer, 
		      Point *ul_corner, Point *lr_corner,
		      Color *colour);
static void fill_rect(Rendererfig *renderer, 
		      Point *ul_corner, Point *lr_corner,
		      Color *colour);
static void draw_arc(Rendererfig *renderer, 
		     Point *center,
		     real width, real height,
		     real angle1, real angle2,
		     Color *colour);
static void fill_arc(Rendererfig *renderer, 
		     Point *center,
		     real width, real height,
		     real angle1, real angle2,
		     Color *colour);
static void draw_ellipse(Rendererfig *renderer, 
			 Point *center,
			 real width, real height,
			 Color *colour);
static void fill_ellipse(Rendererfig *renderer, 
			 Point *center,
			 real width, real height,
			 Color *colour);
static void draw_bezier(Rendererfig *renderer, 
			BezPoint *points,
			int numpoints,
			Color *colour);
static void fill_bezier(Rendererfig *renderer, 
			BezPoint *points, /* Last point must be same as first point */
			int numpoints,
			Color *colour);
static void draw_string(Rendererfig *renderer,
			const char *text,
			Point *pos, Alignment alignment,
			Color *colour);
static void draw_image(Rendererfig *renderer,
		       Point *point,
		       real width, real height,
		       DiaImage image);

static void color_begin_render(Rendererfig *renderer, DiagramData *data);
static void color_draw_line(Rendererfig *renderer, 
		      Point *start, Point *end, 
		      Color *color);
static void color_draw_polyline(Rendererfig *renderer, 
			  Point *points, int num_points, 
			  Color *color);
static void color_draw_polygon(Rendererfig *renderer, 
			 Point *points, int num_points, 
			 Color *color);
static void color_fill_polygon(Rendererfig *renderer, 
			 Point *points, int num_points, 
			 Color *color);
static void color_draw_rect(Rendererfig *renderer, 
		      Point *ul_corner, Point *lr_corner,
		      Color *colour);
static void color_fill_rect(Rendererfig *renderer, 
		      Point *ul_corner, Point *lr_corner,
		      Color *colour);
static void color_draw_arc(Rendererfig *renderer, 
		     Point *center,
		     real width, real height,
		     real angle1, real angle2,
		     Color *colour);
static void color_fill_arc(Rendererfig *renderer, 
		     Point *center,
		     real width, real height,
		     real angle1, real angle2,
		     Color *colour);
static void color_draw_ellipse(Rendererfig *renderer, 
			 Point *center,
			 real width, real height,
			 Color *colour);
static void color_fill_ellipse(Rendererfig *renderer, 
			 Point *center,
			 real width, real height,
			 Color *colour);
static void color_draw_bezier(Rendererfig *renderer, 
			BezPoint *points,
			int numpoints,
			Color *colour);
static void color_fill_bezier(Rendererfig *renderer, 
			BezPoint *points, /* Last point must be same as first point */
			int numpoints,
			Color *colour);
static void color_draw_string(Rendererfig *renderer,
			const char *text,
			Point *pos, Alignment alignment,
			Color *colour);
static void color_draw_image(Rendererfig *renderer,
		       Point *point,
		       real width, real height,
		       DiaImage image);

static RenderOps figRenderColorOps = {
  (BeginRenderFunc) color_begin_render,
  (EndRenderFunc) end_render,

  (SetLineWidthFunc) set_linewidth,
  (SetLineCapsFunc) set_linecaps,
  (SetLineJoinFunc) set_linejoin,
  (SetLineStyleFunc) set_linestyle,
  (SetDashLengthFunc) set_dashlength,
  (SetFillStyleFunc) set_fillstyle,
  (SetFontFunc) set_font,
  
  (DrawLineFunc) color_draw_line,
  (DrawPolyLineFunc) color_draw_polyline,
  
  (DrawPolygonFunc) color_draw_polygon,
  (FillPolygonFunc) color_fill_polygon,

  (DrawRectangleFunc) color_draw_rect,
  (FillRectangleFunc) color_fill_rect,

  (DrawArcFunc) color_draw_arc,
  (FillArcFunc) color_fill_arc,

  (DrawEllipseFunc) color_draw_ellipse,
  (FillEllipseFunc) color_fill_ellipse,

  (DrawBezierFunc) color_draw_bezier,
  (FillBezierFunc) color_fill_bezier,

  (DrawStringFunc) color_draw_string,

  (DrawImageFunc) color_draw_image,
};

static RenderOps figRenderOps = {
  (BeginRenderFunc) begin_render,
  (EndRenderFunc) end_render,

  (SetLineWidthFunc) set_linewidth,
  (SetLineCapsFunc) set_linecaps,
  (SetLineJoinFunc) set_linejoin,
  (SetLineStyleFunc) set_linestyle,
  (SetDashLengthFunc) set_dashlength,
  (SetFillStyleFunc) set_fillstyle,
  (SetFontFunc) set_font,
  
  (DrawLineFunc) draw_line,
  (DrawPolyLineFunc) draw_polyline,
  
  (DrawPolygonFunc) draw_polygon,
  (FillPolygonFunc) fill_polygon,

  (DrawRectangleFunc) draw_rect,
  (FillRectangleFunc) fill_rect,

  (DrawArcFunc) draw_arc,
  (FillArcFunc) fill_arc,

  (DrawEllipseFunc) draw_ellipse,
  (FillEllipseFunc) fill_ellipse,

  (DrawBezierFunc) draw_bezier,
  (FillBezierFunc) fill_bezier,

  (DrawStringFunc) draw_string,

  (DrawImageFunc) draw_image,
};


static void figWarn(Rendererfig *renderer, int warning) {
  if (renderer->warnings[warning]) {
    message_warning(renderer->warnings[warning]);
    renderer->warnings[warning] = NULL;
  }
}

static int figLineStyle(Rendererfig *renderer) {
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

static int figLineWidth(Rendererfig *renderer) {
  return (int)((renderer->linewidth / 2.54) * 80.0);
}

/* Must be called before outputting anything that uses this color,
   in order to output a color pseudo object */
static void figCheckColor(Rendererfig *renderer, Color *color) {
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
  fprintf(renderer->file, "0 %d #%02x%02x%02x\n", renderer->max_user_color+32,
	  (int)(color->red*255), (int)(color->green*255), (int)(color->blue*255));
  renderer->max_user_color++;
}
static int figColor(Rendererfig *renderer, Color *color) {
  int i;

  for (i = 0; i < FIG_MAX_DEFAULT_COLORS; i++) {
    if (color_equals(color, &fig_default_colors[i])) return i;
  }
  for (i = 0; i < renderer->max_user_color; i++) {
    if (color_equals(color, &renderer->user_colors[i])) return i+32;
  }
  return 0;
}
static int figDepth(Rendererfig *renderer) {
  return renderer->depth;
}
static real figDashLength(Rendererfig *renderer) {
  return (renderer->dashlength / 2.54) * 80.0;
}
static int figJoinStyle(Rendererfig *renderer) {
  return renderer->joinmode;
}
static int figCapsStyle(Rendererfig *renderer) {
  return renderer->capsmode;
}

static int figCoord(Rendererfig *renderer, real coord) {
  return (int)((coord/2.54)*1200.0);
}

static real figFloatCoord(Rendererfig *renderer, real coord) {
  return (coord/2.54)*1200.0;
}

static int figAlignment(Rendererfig *renderer, int alignment) {
  return alignment;
}

static int figFont(Rendererfig *renderer) {
  int i;

  for (i = 0; fig_fonts[i] != NULL; i++) {
    if (!strcmp(renderer->font->name, fig_fonts[i]))
      return i;
  }

  return -1;
}

static real figFontSize(Rendererfig *renderer) {
  return (renderer->fontheight/2.54)*72.27;
}

static guchar *figText(Rendererfig *renderer, const guchar *text) {
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

static void color_begin_render(Rendererfig *renderer, DiagramData *data) {
  /* Set up warnings */
  renderer->warnings[WARNING_OUT_OF_COLORS] = 
    _("No more user-definable colors - using black");
  renderer->max_user_color = 0;
}

static void begin_render(Rendererfig *renderer, DiagramData *data) {
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

static void end_render(Rendererfig *renderer) {
}

static void set_linewidth(Rendererfig *renderer, real linewidth) {
  renderer->linewidth = linewidth;
}
static void set_linecaps(Rendererfig *renderer, LineCaps mode) {
  renderer->capsmode = mode;
}
static void set_linejoin(Rendererfig *renderer, LineJoin mode) {
  renderer->joinmode = mode;
}
static void set_linestyle(Rendererfig *renderer, LineStyle mode) {
  renderer->stylemode = mode;
}
static void set_dashlength(Rendererfig *renderer, real length) {
  renderer->dashlength = length;
}
static void set_fillstyle(Rendererfig *renderer, FillStyle mode) {
  renderer->fillmode = mode;
}
static void set_font(Rendererfig *renderer, DiaFont *font, real height) {
  renderer->font = font;
  renderer->fontheight = height;
}
/** Color pass functions */
static void color_draw_line(Rendererfig *renderer, 
		      Point *start, Point *end, 
		      Color *color) {
  figCheckColor(renderer, color);
}
static void color_draw_polyline(Rendererfig *renderer, 
			  Point *points, int num_points, 
			  Color *color) {
  figCheckColor(renderer, color);
}

static void color_draw_polygon(Rendererfig *renderer, 
                         Point *points, int num_points, 
                         Color *color) {
  figCheckColor(renderer, color);
}
static void color_fill_polygon(Rendererfig *renderer, 
                         Point *points, int num_points, 
                         Color *color) {
  figCheckColor(renderer, color);
}

static void color_draw_rect(Rendererfig *renderer, 
                      Point *ul_corner, Point *lr_corner,
                      Color *color) {
  figCheckColor(renderer, color);
}
static void color_fill_rect(Rendererfig *renderer, 
                      Point *ul_corner, Point *lr_corner,
                      Color *color) {
  figCheckColor(renderer, color);
}
static void color_draw_arc(Rendererfig *renderer, 
                     Point *center,
                     real width, real height,
                     real angle1, real angle2,
                     Color *color) {
  figCheckColor(renderer, color);
}
static void color_fill_arc(Rendererfig *renderer, 
		       Point *center,
		       real width, real height,
		       real angle1, real angle2,
		       Color *color) {
  figCheckColor(renderer, color);
}
static void color_draw_ellipse(Rendererfig *renderer, 
                         Point *center,
                         real width, real height,
                         Color *color) {
  figCheckColor(renderer, color);
}
static void color_fill_ellipse(Rendererfig *renderer, 
                         Point *center,
                         real width, real height,
                         Color *color) {
  figCheckColor(renderer, color);
}
static void color_draw_bezier(Rendererfig *renderer, 
                        BezPoint *points,
                        int numpoints,
                        Color *color) {
  figCheckColor(renderer, color);
}
static void color_fill_bezier(Rendererfig *renderer, 
			 BezPoint *points,
			 int numpoints,
			 Color *color) {
  figCheckColor(renderer, color);
}
static void color_draw_string(Rendererfig *renderer,
                        const char *text,
                        Point *pos, Alignment alignment,
                        Color *color) {
  figCheckColor(renderer, color);
}
static void color_draw_image(Rendererfig *renderer,
		       Point *point,
		       real width, real height,
		       DiaImage image) {
}

/** Real pass functions */
static void draw_line(Rendererfig *renderer, 
		      Point *start, Point *end, 
		      Color *color) {
  fprintf(renderer->file, "2 1 %d %d %d 0 %d 0 -1 %f %d %d 0 0 0 2\n",
	  figLineStyle(renderer), figLineWidth(renderer), 
	  figColor(renderer, color), figDepth(renderer),
	  figDashLength(renderer), figJoinStyle(renderer), 
	  figCapsStyle(renderer));
  fprintf(renderer->file, "\t%d %d %d %d\n",
	  figCoord(renderer, start->x), figCoord(renderer, start->y), 
	  figCoord(renderer, end->x), figCoord(renderer, end->y));
}

static void draw_polyline(Rendererfig *renderer, 
			  Point *points, int num_points, 
			  Color *color) {
  int i;

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
static void draw_polygon(Rendererfig *renderer, 
			 Point *points, int num_points, 
			 Color *color) {
  int i;

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
static void fill_polygon(Rendererfig *renderer, 
			 Point *points, int num_points, 
			 Color *color) {
  int i;

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

static void draw_rect(Rendererfig *renderer, 
		      Point *ul_corner, Point *lr_corner,
		      Color *color) {
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
static void fill_rect(Rendererfig *renderer, 
		      Point *ul_corner, Point *lr_corner,
		      Color *color) {
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
static void draw_arc(Rendererfig *renderer, 
		     Point *center,
		     real width, real height,
		     real angle1, real angle2,
		     Color *color) {
  Point first, second, last;

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
static void fill_arc(Rendererfig *renderer, 
		     Point *center,
		     real width, real height,
		     real angle1, real angle2,
		     Color *color) {
  Point first, second, last;

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
static void draw_ellipse(Rendererfig *renderer, 
			 Point *center,
			 real width, real height,
			 Color *color) {
  fprintf(renderer->file, "1 1 %d %d %d -1 %d 0 -1 %f 1 0.0 %d %d %d %d 0 0 0 0\n",
	  figLineStyle(renderer), 
	  figLineWidth(renderer), 
	  figColor(renderer, color),
	  figDepth(renderer),
	  figDashLength(renderer),
	  figCoord(renderer, center->x), figCoord(renderer, center->y),
	  figCoord(renderer, width/2), figCoord(renderer, height/2));

}
static void fill_ellipse(Rendererfig *renderer, 
			 Point *center,
			 real width, real height,
			 Color *color) {
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
static void draw_bezier(Rendererfig *renderer, 
			BezPoint *points,
			int numpoints,
			Color *color) {
  
}
static void fill_bezier(Rendererfig *renderer, 
			BezPoint *points, /* Last point must be same as first point */
			int numpoints,
			Color *colour) {
}
static void draw_string(Rendererfig *renderer,
			const char *text,
			Point *pos, Alignment alignment,
			Color *color) {
  guchar *figtext = figText(renderer, text);
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
static void draw_image(Rendererfig *renderer,
		       Point *point,
		       real width, real height,
		       DiaImage image) {
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
export_fig(DiagramData *data, const gchar *filename, 
           const gchar *diafilename, void* user_data)
{
  FILE *file;
  Rendererfig *renderer;
  int i;
  Layer *layer;

  file = fopen(filename, "w");

  if (file == NULL) {
    message_error(_("Couldn't open: '%s' for writing.\n"), filename);
    return;
  }

  renderer = g_new(Rendererfig, 1);
  renderer->renderer.is_interactive = 0;
  renderer->renderer.interactive_ops = NULL;

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

  renderer->renderer.ops = &figRenderColorOps;

  (((Renderer *)renderer)->ops->begin_render)((Renderer *)renderer);
  
  for (i=0; i<data->layers->len; i++) {
    layer = (Layer *) g_ptr_array_index(data->layers, i);
    layer_render(layer, (Renderer *)renderer, NULL, NULL, data, 0);
    renderer->depth++;
  }
  
  (((Renderer *)renderer)->ops->end_render)((Renderer *)renderer);

  renderer->renderer.ops = &figRenderOps;

  (((Renderer *)renderer)->ops->begin_render)((Renderer *)renderer);
  
  for (i=0; i<data->layers->len; i++) {
    layer = (Layer *) g_ptr_array_index(data->layers, i);
    layer_render(layer, (Renderer *)renderer, NULL, NULL, data, 0);
    renderer->depth++;
  }
  
  (((Renderer *)renderer)->ops->end_render)((Renderer *)renderer);

  g_free(renderer);

  fclose(file);
}

static const gchar *extensions[] = { "fig", NULL };
DiaExportFilter xfig_export_filter = {
  N_("XFig format"),
  extensions,
  export_fig
};
