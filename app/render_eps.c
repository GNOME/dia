/* xxxxxx -- an diagram creation/manipulation program
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
 */
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "config.h"
#include "render_eps.h"
#include "message.h"

static void set_linewidth(RendererEPS *renderer, real linewidth);
static void set_linecaps(RendererEPS *renderer, LineCaps mode);
static void set_linejoin(RendererEPS *renderer, LineJoin mode);
static void set_linestyle(RendererEPS *renderer, LineStyle mode);
static void set_dashlength(RendererEPS *renderer, real length);
static void set_fillstyle(RendererEPS *renderer, FillStyle mode);
static void set_font(RendererEPS *renderer, Font *font, real height);
static void draw_line(RendererEPS *renderer, 
		      Point *start, Point *end, 
		      Color *line_color);
static void draw_polyline(RendererEPS *renderer, 
			  Point *points, int num_points, 
			  Color *line_color);
static void draw_polygon(RendererEPS *renderer, 
			 Point *points, int num_points, 
			 Color *line_color);
static void fill_polygon(RendererEPS *renderer, 
			 Point *points, int num_points, 
			 Color *line_color);
static void draw_rect(RendererEPS *renderer, 
		      Point *ul_corner, Point *lr_corner,
		      Color *color);
static void fill_rect(RendererEPS *renderer, 
		      Point *ul_corner, Point *lr_corner,
		      Color *color);
static void draw_arc(RendererEPS *renderer, 
		     Point *center,
		     real width, real height,
		     real angle1, real angle2,
		     Color *color);
static void fill_arc(RendererEPS *renderer, 
		     Point *center,
		     real width, real height,
		     real angle1, real angle2,
		     Color *color);
static void draw_ellipse(RendererEPS *renderer, 
			 Point *center,
			 real width, real height,
			 Color *color);
static void fill_ellipse(RendererEPS *renderer, 
			 Point *center,
			 real width, real height,
			 Color *color);
static void draw_bezier(RendererEPS *renderer, 
			Point *points,
			int numpoints, /* numpoints = 4+3*n, n=>0 */
			Color *color);
static void fill_bezier(RendererEPS *renderer, 
			Point *points, /* Last point must be same as first point */
			int numpoints, /* numpoints = 4+3*n, n=>0 */
			Color *color);
static void draw_string(RendererEPS *renderer,
			const char *text,
			Point *pos, Alignment alignment,
			Color *color);
static void draw_image(RendererEPS *renderer,
		       Point *point,
		       real width, real height,
		       void *not_decided_yet);

static RenderOps EpsRenderOps = {
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

RendererEPS *
new_eps_renderer(Diagram *dia, char *filename)
{
  RendererEPS *renderer;
  FILE *file;
  time_t time_now;
  double scale;
  Rectangle *extent;
  char *name;
 
  file = fopen(filename, "w");

  if (file==NULL) {
    message_error("Couldn't open: '%s' for writing.\n", filename);
    return NULL;
  }

  renderer = g_new(RendererEPS, 1);
  renderer->renderer.ops = &EpsRenderOps;
  renderer->renderer.is_interactive = 0;
  renderer->renderer.interactive_ops = NULL;

  renderer->file = file;

  renderer->dash_length = 10.0;
  renderer->dash_length = 1.0;
  renderer->saved_line_style = LINESTYLE_SOLID;
  
  time_now  = time(NULL);
  extent = &dia->extents;
  
  scale = 28.346;
  
  name = getlogin();
  if (name==NULL)
    name = "a user";
  
  fprintf(file,
	  "%%!PS-Adobe-2.0 EPSF-2.0\n"
	  "%%%%Title: %s\n"
	  "%%%%Creator: Dia v%s\n"
	  "%%%%CreationDate: %s"
	  "%%%%For: %s\n"
	  "%%%%Orientation: Portrait\n"
	  "%%%%BoundingBox: 0 0 %f %f\n" 
	  "%%%%Pages: 0\n"
	  "%%%%BeginSetup\n"
	  "%%%%EndSetup\n"
	  "%%%%Magnification: 1.0000\n"
	  "%%%%EndComments\n",
	  dia->filename,
	  VERSION,
	  ctime(&time_now),
	  name,
	  (extent->right - extent->left)*scale,
	  (extent->bottom - extent->top)*scale);

  fprintf(file,
	  "/cp {closepath} bind def\n"
	  "/c {curveto} bind def\n"
	  "/f {fill} bind def\n"
	  "/a {arc} bind def\n"
	  "/ef {eofill} bind def\n"
	  "/ex {exch} bind def\n"
	  "/gr {grestore} bind def\n"
	  "/gs {gsave} bind def\n"
	  "/sa {save} bind def\n"
	  "/rs {restore} bind def\n"
	  "/l {lineto} bind def\n"
	  "/m {moveto} bind def\n"
	  "/rm {rmoveto} bind def\n"
	  "/n {newpath} bind def\n"
	  "/s {stroke} bind def\n"
	  "/sh {show} bind def\n"
	  "/slc {setlinecap} bind def\n"
	  "/slj {setlinejoin} bind def\n"
	  "/slw {setlinewidth} bind def\n"
	  "/srgb {setrgbcolor} bind def\n"
	  "/rot {rotate} bind def\n"
	  "/sc {scale} bind def\n"
	  "/sd {setdash} bind def\n"
	  "/ff {findfont} bind def\n"
	  "/sf {setfont} bind def\n"
	  "/scf {scalefont} bind def\n"
	  "/sw {stringwidth pop} bind def\n"
	  "/tr {translate} bind def\n"

	  "\n/ellipsedict 8 dict def\n"
	  "ellipsedict /mtrx matrix put\n"
	  "/ellipse\n"
	  "{ ellipsedict begin\n"
          "   /endangle exch def\n"
          "   /startangle exch def\n"
          "   /yrad exch def\n"
          "   /xrad exch def\n"
          "   /y exch def\n"
          "   /x exch def"
	  "   /savematrix mtrx currentmatrix def\n"
          "   x y tr xrad yrad sc\n"
          "   0 0 1 startangle endangle arc\n"
          "   savematrix setmatrix\n"
          "   end\n"
	  "} def\n\n"

	  "%f %f scale\n"
	  "%f %f translate\n"
	  "%%%%EndProlog\n\n\n",
	  scale, -scale,
	  -extent->left, -extent->bottom );
  
  
  return renderer;
}

void
close_eps_renderer(RendererEPS *renderer)
{
  fclose(renderer->file);
}


static void
set_linewidth(RendererEPS *renderer, real linewidth)
{  /* 0 == hairline **/

  fprintf(renderer->file, "%f slw\n", (double) linewidth);
}

static void
set_linecaps(RendererEPS *renderer, LineCaps mode)
{
  int ps_mode;
  
  switch(mode) {
  case LINECAPS_BUTT:
    ps_mode = 0;
    break;
  case LINECAPS_ROUND:
    ps_mode = 1;
    break;
  case LINECAPS_PROJECTING:
    ps_mode = 2;
    break;
  }

  fprintf(renderer->file, "%d slc\n", ps_mode);
}

static void
set_linejoin(RendererEPS *renderer, LineJoin mode)
{
  int ps_mode;
  
  switch(mode) {
  case LINEJOIN_MITER:
    ps_mode = 0;
    break;
  case LINEJOIN_ROUND:
    ps_mode = 1;
    break;
  case LINEJOIN_BEVEL:
    ps_mode = 2;
    break;
  }

  fprintf(renderer->file, "%d slj\n", ps_mode);
}

static void
set_linestyle(RendererEPS *renderer, LineStyle mode)
{
  double hole_width;

  renderer->saved_line_style = mode;
  
  switch(mode) {
  case LINESTYLE_SOLID:
    fprintf(renderer->file, "[] 0 sd\n");
    break;
  case LINESTYLE_DASHED:
    fprintf(renderer->file, "[%f] 0 sd\n", (double) renderer->dash_length);
    break;
  case LINESTYLE_DASH_DOT:
    hole_width = (renderer->dash_length - renderer->dot_length) / 2.0;
    fprintf(renderer->file, "[%f %f %f %f] 0 sd\n",
	    (double) renderer->dash_length,
	    hole_width,
	    (double) renderer->dot_length,
	    hole_width );
    break;
  case LINESTYLE_DASH_DOT_DOT:
    hole_width = (renderer->dash_length - 2*renderer->dot_length) / 3.0;
    fprintf(renderer->file, "[%f %f %f %f %f %f] 0 sd\n",
	    (double) renderer->dash_length,
	    hole_width,
	    (double) renderer->dot_length,
	    hole_width,
	    (double) renderer->dot_length,
	    hole_width );
    break;
  }
}

static void
set_dashlength(RendererEPS *renderer, real length)
{  /* dot = 10% of len */
  
  renderer->dash_length = length;
  renderer->dot_length = length*0.1;
  
  set_linestyle(renderer, renderer->saved_line_style);
}

static void
set_fillstyle(RendererEPS *renderer, FillStyle mode)
{
  switch(mode) {
  case FILLSTYLE_SOLID:
    break;
  default:
    message_error("eps_renderer: Unsupported fill mode specified!\n");
  }
}

static void
set_font(RendererEPS *renderer, Font *font, real height)
{

  fprintf(renderer->file, "/%s ff %f scf sf\n",
	  font_get_psfontname(font), (double)height);
}

static void
draw_line(RendererEPS *renderer, 
	  Point *start, Point *end, 
	  Color *line_color)
{
  fprintf(renderer->file, "%f %f %f srgb\n",
	  (double) line_color->red, (double) line_color->green, (double) line_color->blue);

  fprintf(renderer->file, "n %f %f m %f %f l s\n",
	  start->x, start->y, end->x, end->y);
}

static void
draw_polyline(RendererEPS *renderer, 
	      Point *points, int num_points, 
	      Color *line_color)
{
  int i;
  
  fprintf(renderer->file, "%f %f %f srgb\n",
	  (double) line_color->red, (double) line_color->green, (double) line_color->blue);
  
  fprintf(renderer->file, "n %f %f m ",
	  points[0].x, points[0].y);

  for (i=1;i<num_points;i++) {
    fprintf(renderer->file, "%f %f l ",
	  points[i].x, points[i].y);
  }

  fprintf(renderer->file, "s\n");
}

static void
draw_polygon(RendererEPS *renderer, 
	      Point *points, int num_points, 
	      Color *line_color)
{
  int i;
  
  fprintf(renderer->file, "%f %f %f srgb\n",
	  (double) line_color->red, (double) line_color->green, (double) line_color->blue);
  
  fprintf(renderer->file, "n %f %f m ",
	  points[0].x, points[0].y);

  for (i=1;i<num_points;i++) {
    fprintf(renderer->file, "%f %f l ",
	  points[i].x, points[i].y);
  }

  fprintf(renderer->file, "cp s\n");
}

static void
fill_polygon(RendererEPS *renderer, 
	      Point *points, int num_points, 
	      Color *line_color)
{
  int i;
  
  fprintf(renderer->file, "%f %f %f srgb\n",
	  (double) line_color->red, (double) line_color->green, (double) line_color->blue);
  
  fprintf(renderer->file, "n %f %f m ",
	  points[0].x, points[0].y);

  for (i=1;i<num_points;i++) {
    fprintf(renderer->file, "%f %f l ",
	  points[i].x, points[i].y);
  }

  fprintf(renderer->file, "f\n");
}

static void
draw_rect(RendererEPS *renderer, 
	  Point *ul_corner, Point *lr_corner,
	  Color *color)
{
  fprintf(renderer->file, "%f %f %f srgb\n",
	  (double) color->red, (double) color->green, (double) color->blue);
  
  fprintf(renderer->file, "n %f %f m %f %f l %f %f l %f %f l cp s\n",
	  (double) ul_corner->x, (double) ul_corner->y,
	  (double) ul_corner->x, (double) lr_corner->y,
	  (double) lr_corner->x, (double) lr_corner->y,
	  (double) lr_corner->x, (double) ul_corner->y );

}

static void
fill_rect(RendererEPS *renderer, 
	  Point *ul_corner, Point *lr_corner,
	  Color *color)
{
  fprintf(renderer->file, "%f %f %f srgb\n",
	  (double) color->red, (double) color->green, (double) color->blue);

  fprintf(renderer->file, "n %f %f m %f %f l %f %f l %f %f l f\n",
	  (double) ul_corner->x, (double) ul_corner->y,
	  (double) ul_corner->x, (double) lr_corner->y,
	  (double) lr_corner->x, (double) lr_corner->y,
	  (double) lr_corner->x, (double) ul_corner->y );
}

static void
draw_arc(RendererEPS *renderer, 
	 Point *center,
	 real width, real height,
	 real angle1, real angle2,
	 Color *color)
{
  fprintf(renderer->file, "%f %f %f srgb\n",
	  (double) color->red, (double) color->green, (double) color->blue);

  fprintf(renderer->file, "n %f %f %f %f %f %f ellipse s\n",
	  (double) center->x, (double) center->y,
	  (double) width/2.0, (double) height/2.0,
	  (double) 360.0 - angle2, (double) 360.0 - angle1 );
}

static void
fill_arc(RendererEPS *renderer, 
	 Point *center,
	 real width, real height,
	 real angle1, real angle2,
	 Color *color)
{
  fprintf(renderer->file, "%f %f %f srgb\n",
	  (double) color->red, (double) color->green, (double) color->blue);

  fprintf(renderer->file, "n %f %f m %f %f %f %f %f %f ellipse f\n",
	  (double) center->x, (double) center->y,
	  (double) center->x, (double) center->y,
	  (double) width/2.0, (double) height/2.0,
	  (double) 360.0 - angle2, (double) 360.0 - angle1 );
}

static void
draw_ellipse(RendererEPS *renderer, 
	     Point *center,
	     real width, real height,
	     Color *color)
{
  fprintf(renderer->file, "%f %f %f srgb\n",
	  (double) color->red, (double) color->green, (double) color->blue);

  fprintf(renderer->file, "n %f %f %f %f 0 360 ellipse cp s\n",
	  (double) center->x, (double) center->y,
	  (double) width/2.0, (double) height/2.0 );
}

static void
fill_ellipse(RendererEPS *renderer, 
	     Point *center,
	     real width, real height,
	     Color *color)
{
  fprintf(renderer->file, "%f %f %f srgb\n",
	  (double) color->red, (double) color->green, (double) color->blue);

  fprintf(renderer->file, "n %f %f %f %f 0 360 ellipse f\n",
	  (double) center->x, (double) center->y,
	  (double) width/2.0, (double) height/2.0 );
}

static void
draw_bezier(RendererEPS *renderer, 
	    Point *points,
	    int numpoints, /* numpoints = 4+3*n, n=>0 */
	    Color *color)
{
  int i;

  fprintf(renderer->file, "%f %f %f srgb\n",
	  (double) color->red, (double) color->green, (double) color->blue);

  fprintf(renderer->file, "n %f %f m",
	  (double) points[0].x, (double) points[0].y);
  
  i = 1;
  while (i<=numpoints-3) {
  fprintf(renderer->file, " %f %f %f %f %f %f c",
	  (double) points[i].x, (double) points[i].y,
	  (double) points[i+1].x, (double) points[i+1].y,
	  (double) points[i+2].x, (double) points[i+2].y );
    i += 3;
  }

  fprintf(renderer->file, " s\n");
}

static void
fill_bezier(RendererEPS *renderer, 
	    Point *points, /* Last point must be same as first point */
	    int numpoints, /* numpoints = 4+3*n, n=>0 */
	    Color *color)
{
  int i;

  fprintf(renderer->file, "%f %f %f srgb\n",
	  (double) color->red, (double) color->green, (double) color->blue);

  fprintf(renderer->file, "n %f %f m",
	  (double) points[0].x, (double) points[0].y);
  
  i = 1;
  while (i<=numpoints-3) {
  fprintf(renderer->file, " %f %f %f %f %f %f c",
	  (double) points[i].x, (double) points[i].y,
	  (double) points[i+1].x, (double) points[i+1].y,
	  (double) points[i+2].x, (double) points[i+2].y );
    i += 3;
  }

  fprintf(renderer->file, " f\n");
}

static void
draw_string(RendererEPS *renderer,
	    const char *text,
	    Point *pos, Alignment alignment,
	    Color *color)
{
  char *buffer;
  const char *str;
  int len;

  /* TODO: Use latin-1 encoding */

  fprintf(renderer->file, "%f %f %f srgb\n",
	  (double) color->red, (double) color->green, (double) color->blue);


  /* Escape all '(' and ')':  */
  buffer = g_malloc(2*strlen(text)+1);
  *buffer = 0;
  str = text;
  while (*str != 0) {
    len = strcspn(str,"()\\");
    strncat(buffer, str, len);
    str += len;
    if (*str != 0) {
      strcat(buffer,"\\");
      strncat(buffer, str, 1);
      str++;
    }
  }
  fprintf(renderer->file, "(%s) ", buffer);
  g_free(buffer);
  
  switch (alignment) {
  case ALIGN_LEFT:
    fprintf(renderer->file, "%f %f m", pos->x, pos->y);
    break;
  case ALIGN_CENTER:
    fprintf(renderer->file, "dup sw 2 div %f ex sub %f m",
	    pos->x, pos->y);
    break;
  case ALIGN_RIGHT:
    fprintf(renderer->file, "dup sw %f ex sub %f m",
	    pos->x, pos->y);
    break;
  }
  
  fprintf(renderer->file, " gs 1 -1 sc sh gr\n");
}

static void
draw_image(RendererEPS *renderer,
	   Point *point,
	   real width, real height,
	   void *not_decided_yet)
{
  message_error("eps_renderer: Images are not supported yet!\n");
}

