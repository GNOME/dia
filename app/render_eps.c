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
 */

/* The eps_dump_truetype_body function and much inspiration for font dumping
 * came from ttfps, which bears the following license notice:

 Copyright (c) 1997 by Juliusz Chroboczek 

 Copying
 *******

 This software is provided with no guarantee, not even of any kind.

 Feel free to do whatever you wish with it as long as you don't ask me
 to maintain it.

*/

/* The Document Structure Definitions used for the output is available at
 * http://www-cdf.fnal.gov/offline/PostScript/psstruct.ps
 * (Appendix G of the Red and White Book)
 */

/* Note: There is a use of setmatrix in ellipse, which is supposed to be
 * avoided.  Could it be?
 */

/* Note that the EPS renderer now has two phases:  One to collect font
 * info (and conceivably more, like color defs), and one to actually render.
 */

#include <config.h>

#include <string.h>
#include <time.h>
#include <math.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <locale.h>
#include <errno.h>

#include "intl.h"
#include "render_eps.h"
#include "message.h"
#include "diagramdata.h"
#include "font.h"

/* Using FT2 with Pango is currently broken on win32 
 * as a result the whole eps renderer vanishes
 */
#ifdef HAVE_FREETYPE

#include <pango/pango.h>
#include <pango/pangoft2.h>
/* I'd really rather avoid this */
#include <freetype/ftglyph.h>

#define DPI 300
#define TWIDDLE (2.54 * 1.1)
#endif

/*
If we can but get the TT font, OpenOffice has a converter at
http://gsl.openoffice.org/source/browse/gsl/psprint/source/fontsubset

For a chinese font with huge coverage, visit
http://www.founder.com.cn/fontweb/chanpinzl/CP_chaoda.htm
http://www.founder.com.cn/fontweb/chanpinzl/FA_chaoda.htm
 */

static void begin_render(RendererEPS *renderer);
static void end_render(RendererEPS *renderer);
static void set_linewidth(RendererEPS *renderer, real linewidth);
static void set_linecaps(RendererEPS *renderer, LineCaps mode);
static void set_linejoin(RendererEPS *renderer, LineJoin mode);
static void set_linestyle(RendererEPS *renderer, LineStyle mode);
static void set_dashlength(RendererEPS *renderer, real length);
static void set_fillstyle(RendererEPS *renderer, FillStyle mode);
static void set_font(RendererEPS *renderer, DiaFont *font, real height);
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
			BezPoint *points,
			int numpoints,
			Color *color);
static void fill_bezier(RendererEPS *renderer, 
			BezPoint *points, /* Last point must be same as first point */
			int numpoints,
			Color *color);
static void draw_string(RendererEPS *renderer,
			const char *text,
			Point *pos, Alignment alignment,
			Color *color);
static void draw_image(RendererEPS *renderer,
		       Point *point,
		       real width, real height,
		       DiaImage image);

/* These functions are used to create the prolog.  Currently takes care
 * of defining necessary fonts.
 */
static void begin_prolog(RendererEPS *renderer);
static void end_prolog(RendererEPS *renderer);
static void prolog_define_font(RendererEPS *renderer,
			       DiaFont *font, real height);
static void prolog_check_string(RendererEPS *renderer,
				const char *text,
				Point *pos, Alignment alignment,
				Color *color);

static void init_eps_renderer();

static void null_func() {}

#ifdef HAVE_FREETYPE
/* These routines stolen mercilessly from PAPS
 * http://imagic.weizmann.ac.il/~dov/freesw/paps
 */
/* Information passed in user data when drawing outlines */
typedef struct _OutlineInfo OutlineInfo;
struct _OutlineInfo {
  FILE *OUT;
  FT_Vector glyph_origin;
  int dpi;
};

void postscript_contour_headers(FILE *OUT, int dpi_x, int dpi_y);
void postscript_draw_contour(RendererEPS *renderer,
			     int dpi_x,
			     PangoLayoutLine *pango_line,
			     double x_pos,
			     double y_pos
			     );
void draw_bezier_outline(RendererEPS *renderer,
			 int dpi_x,
			 FT_Face face,
			 FT_UInt glyph_index,
			 double pos_x,
			 double pos_y
			 );
/* Countour traveling functions */
static int paps_move_to( FT_Vector* to,
			 void *user_data);
static int paps_line_to( FT_Vector*  to,
			 void *user_data);
static int paps_conic_to( FT_Vector*  control,
			  FT_Vector*  to,
			  void *user_data);
static int paps_cubic_to( FT_Vector*  control1,
			  FT_Vector*  control2,
			  FT_Vector*  to,
			  void *user_data);

#endif

static RenderOps *EpsRenderOps;
static RenderOps *EpsPrologOps;

static void
init_eps_renderer() {
  EpsRenderOps = create_renderops_table();

  EpsRenderOps->begin_render = (BeginRenderFunc) begin_render;
  EpsRenderOps->end_render = (EndRenderFunc) end_render;

  EpsRenderOps->set_linewidth = (SetLineWidthFunc) set_linewidth;
  EpsRenderOps->set_linecaps = (SetLineCapsFunc) set_linecaps;
  EpsRenderOps->set_linejoin = (SetLineJoinFunc) set_linejoin;
  EpsRenderOps->set_linestyle = (SetLineStyleFunc) set_linestyle;
  EpsRenderOps->set_dashlength = (SetDashLengthFunc) set_dashlength;
  EpsRenderOps->set_fillstyle = (SetFillStyleFunc) set_fillstyle;
  EpsRenderOps->set_font = (SetFontFunc) set_font;
  
  EpsRenderOps->draw_line = (DrawLineFunc) draw_line;
  EpsRenderOps->draw_polyline = (DrawPolyLineFunc) draw_polyline;
  
  EpsRenderOps->draw_polygon = (DrawPolygonFunc) draw_polygon;
  EpsRenderOps->fill_polygon = (FillPolygonFunc) fill_polygon;

  EpsRenderOps->draw_rect = (DrawRectangleFunc) draw_rect;
  EpsRenderOps->fill_rect = (FillRectangleFunc) fill_rect;

  EpsRenderOps->draw_arc = (DrawArcFunc) draw_arc;
  EpsRenderOps->fill_arc = (FillArcFunc) fill_arc;

  EpsRenderOps->draw_ellipse = (DrawEllipseFunc) draw_ellipse;
  EpsRenderOps->fill_ellipse = (FillEllipseFunc) fill_ellipse;

  EpsRenderOps->draw_bezier = (DrawBezierFunc) draw_bezier;
  EpsRenderOps->fill_bezier = (FillBezierFunc) fill_bezier;

  EpsRenderOps->draw_string = (DrawStringFunc) draw_string;

  EpsRenderOps->draw_image = (DrawImageFunc) draw_image;


  EpsPrologOps = create_renderops_table();

  EpsPrologOps->begin_render = (BeginRenderFunc) begin_prolog;
  EpsPrologOps->end_render = (EndRenderFunc) end_prolog;

  EpsPrologOps->set_linewidth = (SetLineWidthFunc) null_func;
  EpsPrologOps->set_linecaps = (SetLineCapsFunc) null_func;
  EpsPrologOps->set_linejoin = (SetLineJoinFunc) null_func;
  EpsPrologOps->set_linestyle = (SetLineStyleFunc) null_func;
  EpsPrologOps->set_dashlength = (SetDashLengthFunc) null_func;
  EpsPrologOps->set_fillstyle = (SetFillStyleFunc) null_func;
  EpsPrologOps->set_font = (SetFontFunc) prolog_define_font;
  
  EpsPrologOps->draw_line = (DrawLineFunc) null_func;
  EpsPrologOps->draw_polyline = (DrawPolyLineFunc) null_func;
  
  EpsPrologOps->draw_polygon = (DrawPolygonFunc) null_func;
  EpsPrologOps->fill_polygon = (FillPolygonFunc) null_func;

  EpsPrologOps->draw_rect = (DrawRectangleFunc) null_func;
  EpsPrologOps->fill_rect = (FillRectangleFunc) null_func;

  EpsPrologOps->draw_arc = (DrawArcFunc) null_func;
  EpsPrologOps->fill_arc = (FillArcFunc) null_func;

  EpsPrologOps->draw_ellipse = (DrawEllipseFunc) null_func;
  EpsPrologOps->fill_ellipse = (FillEllipseFunc) null_func;

  EpsPrologOps->draw_bezier = (DrawBezierFunc) null_func;
  EpsPrologOps->fill_bezier = (FillBezierFunc) null_func;

  EpsPrologOps->draw_string = (DrawStringFunc) prolog_check_string;

  EpsPrologOps->draw_image = (DrawImageFunc) null_func;
  
};


static void print_define_font(gpointer key, gpointer value,
			      gpointer data)
{
  DiaFont *font = (DiaFont *)value;
  gchar *fontname = (gchar *)key;
  FILE *file = (FILE *)data;
 
}

/* This hashtable keeps track of the fonts used in the diagram.
 * Indexed by psfontname, elements are DiaFonts.
 */
static GHashTable *font_table;

void
begin_prolog(RendererEPS *renderer)
{
  font_table = g_hash_table_new(g_str_hash, g_str_equal);
  fprintf(renderer->file, "%%%%BeginProlog\n");

  fprintf(renderer->file,
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

	  /*
	    "/colortogray {\n"
	    "/rgbdata exch store\n"
	    "rgbdata length 3 idiv\n"
	    "/npixls exch store\n"
	    "/rgbindx 0 store\n"
	    "0 1 npixls 1 sub {\n"
	    "grays exch\n"
	    "rgbdata rgbindx       get 20 mul\n"
	    "rgbdata rgbindx 1 add get 32 mul\n"
	    "rgbdata rgbindx 2 add get 12 mul\n"
	    "add add 64 idiv\n"
	    "put\n"
	    "/rgbindx rgbindx 3 add store\n"
	    "} for\n"
	    "grays 0 npixls getinterval\n"
	    "} bind def\n"
	  */
	  "/mergeprocs {\n"
	  "dup length\n"
	  "3 -1 roll\n"
	  "dup\n"
	  "length\n"
	  "dup\n"
	  "5 1 roll\n"
	  "3 -1 roll\n"
	  "add\n"
	  "array cvx\n"
	  "dup\n"
	  "3 -1 roll\n"
	  "0 exch\n"
	  "putinterval\n"
	  "dup\n"
	  "4 2 roll\n"
	  "putinterval\n"
	  "} bind def\n"
	  /*
	    "/colorimage {\n"
	    "pop pop\n"
	    "{colortogray} mergeprocs\n"
	    "image\n"
	    "} bind def\n\n"
	  */);
#ifdef HAVE_FREETYPE
  postscript_contour_headers(renderer->file, DPI, DPI /* dpi_x, dpi_y */);
#endif
}

static void
end_prolog(RendererEPS *renderer)
{}

static void
prolog_define_font(RendererEPS *renderer, DiaFont *font, real height)
{
        /* FIXME: check the lifetime of the return value of
         dia_font_get_psfontname(). If needed, copy it (and
        g_free() it when font_table is deleted) */
  /*  g_hash_table_insert(font_table, dia_font_get_psfontname(font), font);*/
}

static void
prolog_check_string(RendererEPS *renderer,
                    const char *text,
                    Point *pos, Alignment alignment,
                    Color *color)
{
  /*    const char *utf8_buffer;
    int utf8_len;

    if ((renderer->psu) && (text) && (text != (const char *)(1))) {

        utf8_buffer=text;
        utf8_len = strlen(utf8_buffer);

        if (utf8_len > 0) {
            psu_check_string_encodings(renderer->psu,utf8_buffer);
        }
    }
    */
  /* Not current doing pre-pass */
  /* In here we can grab all the chars needed, to allow incremental
   * font defs (or even just partial font defs).
   */
    
}

/* This function switches the RendererEPS into render mode */
void
eps_renderer_prolog_done(RendererEPS *renderer) {
  g_hash_table_foreach(font_table, print_define_font, renderer->file);

  fprintf(renderer->file, 
	  "%%%%EndProlog\n\n"
	  "%%%%BeginSetup\n"
	  "%%%%EndSetup\n");
  renderer->renderer.ops = EpsRenderOps;
}

static void
eps_renderer_set_scale(DiagramData *data, RendererEPS *renderer)
{
  double scale;
  Rectangle *extent;
  extent = &data->extents;
  
  scale = 28.346 * data->paper.scaling;

  fprintf(renderer->file,
	  "%% Paper scaling factor: %f\n"
	  "%f %f scale\n"
	  "%f %f translate\n\n",
	  data->paper.scaling,
	  scale, -scale,
	  -extent->left, -extent->bottom );
}

  /*** PROLOG STUFF ENDS HERE ***/

static RendererEPS *
create_common_renderer(DiagramData *data, FILE *file)
{
  RendererEPS *renderer;
  double scale;
  PangoFontDescription *font_description;

  if (EpsPrologOps == NULL)
    init_eps_renderer();

  renderer = g_new(RendererEPS, 1);
  renderer->renderer.ops = EpsPrologOps;
  renderer->renderer.is_interactive = 0;
  renderer->renderer.interactive_ops = NULL;

  renderer->pagenum = 1;
  renderer->file = file;
  renderer->lcolor.red = -1.0;
  
  renderer->dash_length = 1.0;
  renderer->dot_length = 0.2;
  renderer->saved_line_style = LINESTYLE_SOLID;
  
#ifdef HAVE_FREETYPE
  renderer->context = pango_ft2_get_context (DPI, DPI/*dpi_x, dpi_y*/);
  
  /* Setup pango */
  pango_context_set_language (renderer->context, pango_language_from_string ("en_US"));
  pango_context_set_base_dir (renderer->context, PANGO_DIRECTION_LTR);

  font_description = pango_font_description_new ();
  pango_font_description_set_family (font_description, g_strdup("sans"));
  pango_font_description_set_style (font_description, PANGO_STYLE_NORMAL);
  pango_font_description_set_variant (font_description, PANGO_VARIANT_NORMAL);
  pango_font_description_set_weight (font_description, PANGO_WEIGHT_NORMAL);
  pango_font_description_set_stretch (font_description, PANGO_STRETCH_NORMAL);
  pango_font_description_set_size (font_description, 12*PANGO_SCALE);

  pango_context_set_font_description (renderer->context, font_description);
#endif

  scale = 28.346 * data->paper.scaling;
  renderer->scale = scale;

  return renderer;
}

static RendererEPS *
create_eps_renderer(DiagramData *data, const char *filename,
		    const char *diafilename)
{
  RendererEPS *renderer;
  FILE *file;
  Rectangle *extent;
  char *name;
  time_t time_now;

  file = fopen(filename, "wb");

  if (file==NULL) {
    message_error(_("Couldn't open: '%s' for writing.\n"), filename);
    return NULL;
  }

  renderer = create_common_renderer(data, file);
  renderer->is_ps = FALSE;

  extent = &data->extents;

  name = g_get_user_name();
  if (name==NULL)
    name = "a user";

  time_now = time(0);
  
  fprintf(file,
	  "%%!PS-Adobe-2.0 EPSF-2.0\n"
	  "%%%%Title: %s\n"
	  "%%%%Creator: Dia v%s\n"
	  "%%%%CreationDate: %s"
	  "%%%%For: %s\n"
	  "%%%%Magnification: 1.0000\n"
	  "%%%%Orientation: Portrait\n"
	  "%%%%BoundingBox: 0 0 %d %d\n" 
	  "%%%%Pages: 1\n"
	  "%%%%EndComments\n",
	  g_basename(diafilename),
	  VERSION,
	  ctime(&time_now),
	  name,
	  (int) ceil((extent->right - extent->left)*renderer->scale),
	  (int) ceil((extent->bottom - extent->top)*renderer->scale) );

  return renderer;
}

RendererEPS *
new_eps_renderer(Diagram *dia, gchar *filename)
{
  return create_eps_renderer(dia->data, filename, dia->filename);
}

void
destroy_eps_renderer(RendererEPS *renderer)
{
  g_free(renderer);
}


RendererEPS *
new_psprint_renderer(Diagram *dia, FILE *file)
{
  RendererEPS *renderer;
  gchar *name;
  time_t time_now;

  renderer = create_common_renderer(dia->data, file);
  renderer->is_ps = TRUE;

  name = g_get_user_name();
  if (name==NULL)
    name = "a user";
  
  time_now = time(0);

  fprintf(file,
	  "%%!PS-Adobe-2.0\n"
	  "%%%%Title: %s\n"
	  "%%%%Creator: Dia v%s\n"
	  "%%%%CreationDate: %s"
	  "%%%%For: %s\n"
	  "%%%%DocumentPaperSizes: %s\n"
	  "%%%%Orientation: %s\n"
	  "%%%%EndComments\n",
	  dia->filename,
	  VERSION,
	  ctime(&time_now),
	  name,
	  dia->data->paper.name,
	  dia->data->paper.is_portrait ? "Portrait" : "Landscape");
  
  return renderer;
}

static void
begin_render(RendererEPS *renderer)
{
}

static void
end_render(RendererEPS *renderer)
{
  if (!renderer->is_ps) {
    fprintf(renderer->file, "showpage\n");
    fclose(renderer->file);
  }
}

static void
lazy_setcolor(RendererEPS *renderer,
              Color *color)
{
  if (!color_equals(color, &(renderer->lcolor))) {
    renderer->lcolor = *color;
    fprintf(renderer->file, "%f %f %f srgb\n",
            (double) color->red,
            (double) color->green,
            (double) color->blue);    
  }
}


static void
set_linewidth(RendererEPS *renderer, real linewidth)
{  /* 0 == hairline **/
  if (linewidth == 0.0) linewidth=.1; /* Adobe's advice */
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
  default:
    ps_mode = 0;
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
  default:
    ps_mode = 0;
  }

  fprintf(renderer->file, "%d slj\n", ps_mode);
}

static void
set_linestyle(RendererEPS *renderer, LineStyle mode)
{
  real hole_width;

  renderer->saved_line_style = mode;
  
  switch(mode) {
  case LINESTYLE_SOLID:
    fprintf(renderer->file, "[] 0 sd\n");
    break;
  case LINESTYLE_DASHED:
    fprintf(renderer->file, "[%f] 0 sd\n", renderer->dash_length);
    break;
  case LINESTYLE_DASH_DOT:
    hole_width = (renderer->dash_length - renderer->dot_length) / 2.0;
    fprintf(renderer->file, "[%f %f %f %f] 0 sd\n",
	    renderer->dash_length,
	    hole_width,
	    renderer->dot_length,
	    hole_width );
    break;
  case LINESTYLE_DASH_DOT_DOT:
    hole_width = (renderer->dash_length - 2.0*renderer->dot_length) / 3.0;
    fprintf(renderer->file, "[%f %f %f %f %f %f] 0 sd\n",
	    renderer->dash_length,
	    hole_width,
	    renderer->dot_length,
	    hole_width,
	    renderer->dot_length,
	    hole_width );
    break;
  case LINESTYLE_DOTTED:
    fprintf(renderer->file, "[%f] 0 sd\n", renderer->dot_length);
    break;
  }
}

static void
set_dashlength(RendererEPS *renderer, real length)
{  /* dot = 20% of len */
  if (length<0.001)
    length = 0.001;
  
  renderer->dash_length = length;
  renderer->dot_length = length*0.2;
  
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
draw_line(RendererEPS *renderer, 
	  Point *start, Point *end, 
	  Color *line_color)
{
  lazy_setcolor(renderer,line_color);

  fprintf(renderer->file, "n %f %f m %f %f l s\n",
	  start->x, start->y, end->x, end->y);
}

static void
draw_polyline(RendererEPS *renderer, 
	      Point *points, int num_points, 
	      Color *line_color)
{
  int i;

  lazy_setcolor(renderer,line_color);
  
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
  
  lazy_setcolor(renderer,line_color);
  
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
	      Color *fill_color)
{
  int i;

  lazy_setcolor(renderer,fill_color);
  
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
  lazy_setcolor(renderer,color);
  
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
  lazy_setcolor(renderer,color);

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
  lazy_setcolor(renderer,color);

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
  lazy_setcolor(renderer,color);

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
  lazy_setcolor(renderer,color);

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
  lazy_setcolor(renderer,color);

  fprintf(renderer->file, "n %f %f %f %f 0 360 ellipse f\n",
	  (double) center->x, (double) center->y,
	  (double) width/2.0, (double) height/2.0 );
}

static void
draw_bezier(RendererEPS *renderer, 
	    BezPoint *points,
	    int numpoints, /* numpoints = 4+3*n, n=>0 */
	    Color *color)
{
  int i;

  lazy_setcolor(renderer,color);

  if (points[0].type != BEZ_MOVE_TO)
    g_warning("first BezPoint must be a BEZ_MOVE_TO");

  fprintf(renderer->file, "n %f %f m",
	  (double) points[0].p1.x, (double) points[0].p1.y);
  
  for (i = 1; i < numpoints; i++)
    switch (points[i].type) {
    case BEZ_MOVE_TO:
      g_warning("only first BezPoint can be a BEZ_MOVE_TO");
      break;
    case BEZ_LINE_TO:
      fprintf(renderer->file, " %f %f l",
	      (double) points[i].p1.x, (double) points[i].p1.y);
      break;
    case BEZ_CURVE_TO:
      fprintf(renderer->file, " %f %f %f %f %f %f c",
	      (double) points[i].p1.x, (double) points[i].p1.y,
	      (double) points[i].p2.x, (double) points[i].p2.y,
	      (double) points[i].p3.x, (double) points[i].p3.y );
      break;
    }

  fprintf(renderer->file, " s\n");
}

static void
fill_bezier(RendererEPS *renderer, 
	    BezPoint *points, /* Last point must be same as first point */
	    int numpoints,
	    Color *color)
{
  int i;

  lazy_setcolor(renderer,color);

  if (points[0].type != BEZ_MOVE_TO)
    g_warning("first BezPoint must be a BEZ_MOVE_TO");

  fprintf(renderer->file, "n %f %f m",
	  (double) points[0].p1.x, (double) points[0].p1.y);
  
  for (i = 1; i < numpoints; i++)
    switch (points[i].type) {
    case BEZ_MOVE_TO:
      g_warning("only first BezPoint can be a BEZ_MOVE_TO");
      break;
    case BEZ_LINE_TO:
      fprintf(renderer->file, " %f %f l",
	      (double) points[i].p1.x, (double) points[i].p1.y);
      break;
    case BEZ_CURVE_TO:
      fprintf(renderer->file, " %f %f %f %f %f %f c",
	      (double) points[i].p1.x, (double) points[i].p1.y,
	      (double) points[i].p2.x, (double) points[i].p2.y,
	      (double) points[i].p3.x, (double) points[i].p3.y );
      break;
    }

  fprintf(renderer->file, " f\n");
}

#ifdef HAVE_FREETYPE
/* ********************************************************* */
/*		   String rendering using PangoFt2           */
/* ********************************************************* */
/* Such a big mark really is a sign that this should go in its own file:) */

/*======================================================================
  outline traversing functions.
  ----------------------------------------------------------------------*/
static int paps_move_to( FT_Vector* to,
			 void *user_data)
{
  OutlineInfo *outline_info = (OutlineInfo*)user_data;
  fprintf(outline_info->OUT, "%d %d moveto\n",
	  to->x ,
	  to->y );
  return 0;
}

static int paps_line_to( FT_Vector*  to,
			 void *user_data)
{
  OutlineInfo *outline_info = (OutlineInfo*)user_data;
  fprintf(outline_info->OUT, "%d %d lineto\n",
	  to->x ,
	  to->y );
  return 0;
}

static int paps_conic_to( FT_Vector*  control,
			  FT_Vector*  to,
			  void *user_data)
{
  OutlineInfo *outline_info = (OutlineInfo*)user_data;
  fprintf(outline_info->OUT, "%d %d %d %d conicto\n",
	  control->x  ,
	  control->y  ,
	  to->x   ,
	  to->y  );
  return 0;
}

static int paps_cubic_to( FT_Vector*  control1,
			  FT_Vector*  control2,
			  FT_Vector*  to,
			  void *user_data)
{
  OutlineInfo *outline_info = (OutlineInfo*)user_data;
  fprintf(outline_info->OUT,
	  "%d %d %d %d %d %d curveto\n",
	  control1->x , 
	  control1->y ,
	  control2->x ,
	  control2->y ,
	  to->x ,
	  to->y );
  return 0;
}

/* These must go in the prologue section */
void postscript_contour_headers(FILE *OUT, int dpi_x, int dpi_y)
{
  /* /dpi_x needed for /conicto */
  fprintf(OUT,
	  "/dpi_x %d def\n"
	  "/dpi_y %d def\n", dpi_x, dpi_y);
  /* Outline support */
  fprintf(OUT,
	  "/conicto {\n"
	  "    /to_y exch def\n"
	  "    /to_x exch def\n"
	  "    /conic_cntrl_y exch def\n"
	  "    /conic_cntrl_x exch def\n"
	  "    currentpoint\n"
	  "    /p0_y exch def\n"
	  "    /p0_x exch def\n"
	  "    /p1_x p0_x conic_cntrl_x p0_x sub 2 3 div mul add def\n"
	  "    /p1_y p0_y conic_cntrl_y p0_y sub 2 3 div mul add def\n"
	  "    /p2_x p1_x to_x p0_x sub 1 3 div mul add def\n"
	  "    /p2_y p1_y to_y p0_y sub 1 3 div mul add def\n"
	  "    p1_x p1_y p2_x p2_y to_x to_y curveto\n"
	  "} bind def\n"
	  "/start_ol { gsave 1.1 dpi_x div dup scale} bind def\n"
	  "/end_ol { closepath fill grestore } bind def\n"
	  );
}
				

/* postscript_draw_contour() dumps out the information of a line. It shows how
   to access the ft font information out of the pango font info.
 */
void postscript_draw_contour(RendererEPS *renderer,
			     int dpi_x,
			     PangoLayoutLine *pango_line,
			     double line_start_pos_x,
			     double line_start_pos_y)
{
  GSList *runs_list;
  int num_runs = 0;
  PangoRectangle ink_rect, logical_rect;
  int byte_width;
  FT_Bitmap bitmap;
  guchar *buf;

  /* First calculate number of runs in text */
  runs_list = pango_line->runs;
  while(runs_list)
    {
      PangoLayoutRun *run = runs_list->data;
      num_runs++;
      runs_list = runs_list->next;
    }
  /* Loop over the runs and output font info */
  runs_list = pango_line->runs;
  while(runs_list)
    {
      PangoLayoutRun *run = runs_list->data;
      PangoItem *item = run->item;
      PangoGlyphString *glyphs = run->glyphs;
      PangoAnalysis *analysis = &item->analysis;
      PangoFont *font = analysis->font;
      /*
	PangoFont *font = pango_context_load_font(renderer->context,
	renderer->current_font->pfd);
      */
      FT_Face ft_face;
      int bidi_level;
      int num_glyphs;
      int glyph_idx;

      if (font == NULL) {
	fprintf(stderr, "No font found\n");
	continue;
      } 
      ft_face = pango_ft2_font_get_face(font);
      if (ft_face == NULL) {
	fprintf(stderr, "Failed to get face for font %s\n",
		pango_font_description_to_string(pango_font_describe(font)));
	continue;
      }

      /*
      printf("Got face %s (PS %s) for font %s (diafont %s)\n",
	     ft_face->family_name,
	     FT_Get_Postscript_Name(ft_face),
	     pango_font_description_to_string(pango_font_describe(font)),
	     dia_font_get_family(renderer->current_font));
      */

      bidi_level = item->analysis.level;
      num_glyphs = glyphs->num_glyphs;
      
      for (glyph_idx=0; glyph_idx<num_glyphs; glyph_idx++)
	{
	  PangoGlyphGeometry geometry = glyphs->glyphs[glyph_idx].geometry;
	  double pos_x;
	  double pos_y;
	  double scale = 2.54/PANGO_SCALE/dpi_x;/*72.0 / PANGO_SCALE  / dpi_x;*/

	  pos_x = line_start_pos_x + 1.0* geometry.x_offset * scale;
	  pos_y = line_start_pos_y - 1.0*geometry.y_offset * scale;

	  line_start_pos_x += 1.0 * geometry.width * scale;

	  /*
	  printf("Drawing glyph %d: index %d at %f, %f (w %d)\n", glyph_idx, 
		 glyphs->glyphs[glyph_idx].glyph, pos_x, pos_y,
		 geometry.width);
	  */	  
	  draw_bezier_outline(renderer,
			      dpi_x,
			      ft_face,
			      (FT_UInt)(glyphs->glyphs[glyph_idx].glyph),
			      pos_x, pos_y
			      );
	}
      
      runs_list = runs_list->next;
    }
  
}

void draw_bezier_outline(RendererEPS *renderer,
			 int dpi_x,
			 FT_Face face,
			 FT_UInt glyph_index,
			 double pos_x,
			 double pos_y
			 )
{
  FT_Int load_flags = FT_LOAD_DEFAULT|FT_LOAD_NO_BITMAP;
  FT_Glyph glyph;
  FT_Error error;

  /* Need to transform */

  /* Output outline */
  FT_Outline_Funcs outlinefunc = 
  {
    paps_move_to,
    paps_line_to,
    paps_conic_to,
    paps_cubic_to
  };
  OutlineInfo outline_info;

  outline_info.glyph_origin.x = pos_x;
  outline_info.glyph_origin.y = pos_y;
  outline_info.dpi = dpi_x;
  outline_info.OUT = renderer->file;

  fprintf(renderer->file, 
	  "gsave %f %f translate %f %f scale 0 0 0 setrgbcolor\n",
	  pos_x, pos_y, 2.54/72.0, -2.54/72.0);
  fprintf(renderer->file, "start_ol\n");

  if ((error=FT_Load_Glyph(face, glyph_index, load_flags))) {
    fprintf(stderr, "Can't load glyph: %d\n", error);
    return;
  }
  FT_Get_Glyph (face->glyph, &glyph);
  FT_Outline_Decompose (&(((FT_OutlineGlyph)glyph)->outline),
                        &outlinefunc, &outline_info);
  fprintf(renderer->file, "end_ol grestore \n");
  
  FT_Done_Glyph (glyph);
}


static void
set_font(RendererEPS *renderer, DiaFont *font, real height)
{
  renderer->current_font = font;
  /* Where did this 1.8 come from? */
  /* Used to be just as arbitrarily 1.6, but experimentation shows 1.8
     is better.  This holds only when using Freetype, in other cases all
     bets are really off, especially since some fonts may not be found.
  */
  /* 28.346 = 72.0 / 2.54 */
  renderer->current_height = height*1.8;
  pango_context_set_font_description(renderer->context, font->pfd);
}

static void
draw_string(RendererEPS *renderer,
	    const gchar *text,
	    Point *pos, Alignment alignment,
	    Color *color)
{
  PangoLayout *layout;
  int width;
  int line, linecount;
  double xpos = pos->x, ypos = pos->y;
  PangoAttrList* list;
  PangoAttribute* attr;
  guint length;

  if ((!text)||(text == (const char *)(1))) return;

  lazy_setcolor(renderer,color);

  dia_font_set_height(renderer->current_font, renderer->current_height);
  layout = pango_layout_new(renderer->context);

  length = text ? strlen(text) : 0;
  pango_layout_set_text(layout,text,length);
        
  list = pango_attr_list_new();

  attr = pango_attr_font_desc_new(renderer->current_font->pfd);
  attr->start_index = 0;
  attr->end_index = length;
  pango_attr_list_insert(list,attr); /* eats attr */
    
  pango_layout_set_attributes(layout,list);
  pango_attr_list_unref(list);

  pango_layout_set_indent(layout,0);
  pango_layout_set_justify(layout,FALSE);

  switch (alignment) {
  case ALIGN_LEFT:
    pango_layout_set_alignment(layout,PANGO_ALIGN_LEFT);
    break;
  case ALIGN_CENTER:
    pango_layout_set_alignment(layout,PANGO_ALIGN_CENTER);
    break;
  case ALIGN_RIGHT:
    pango_layout_set_alignment(layout,PANGO_ALIGN_RIGHT);
    break;
  }
    
  pango_layout_get_size(layout, &width, NULL);
  linecount = pango_layout_get_line_count(layout);
  for (line = 0; line < linecount; line++) {
    PangoLayoutLine *layoutline = pango_layout_get_line(layout, line);
    real width, xoff = 0.0;
    PangoRectangle rectangle;

    pango_layout_line_get_extents(layoutline, &rectangle, NULL);
    width = rectangle.width;

    switch (alignment) {
    case ALIGN_LEFT:
      xoff = 0.0;
      break;
    case ALIGN_CENTER:
      xoff = width/2.0;
      break;
    case ALIGN_RIGHT:
      xoff = width;
      break;
    }
    xoff *= 2.54/PANGO_SCALE/DPI /* dpi_x */;

    postscript_draw_contour(renderer,
			    DPI, /* dpi_x */
			    layoutline,
			    xpos-xoff, ypos);
    /* xpos should be adjusted for align and/or RTL */
    ypos += 10;/* Some line height thing??? */
  }
}
#else
static void
set_font(RendererEPS *renderer, DiaFont *font, real height)
{
  message_error("Can't get Postscript names for Pango fonts.  Sorry, no text in PS files\n");
}

static void
draw_string(RendererEPS *renderer,
	    const gchar *text,
	    Point *pos, Alignment alignment,
	    Color *color)
{
  message_error("Can't get Postscript names for Pango fonts.  Sorry, no text in PS files\n");
}
#endif

/* ****** */
/* IMAGES */
/* ****** */

#define RLE 0
#if RLE
/* RLE-encodes as defined in the Red Book */
enum color_channel {
  BLACK, RED, GREEN, BLUE
};

#define CHANNEL_ADD(chan) ((chan)?(chan)-1:0)
#define CHANNEL_SKIP(chan) ((chan)?3:1)

/* Find the max number of chars < 128 with no triples */
static int
find_max_non_replicated(char *src, int srclen, enum color_channel chan)
{
  int items = 0;

  while (items*CHANNEL_SKIP(chan) < srclen && 
	 items < 128) {
    if (items*CHANNEL_SKIP(chan) == srclen-1) return items+1;
    if (src[items*CHANNEL_SKIP(chan)+CHANNEL_ADD(chan)] ==
	src[(items+1)*CHANNEL_SKIP(chan)+CHANNEL_ADD(chan)]) {
      /* Two of the same */
      if (items*CHANNEL_SKIP(chan) == srclen-2) return MIN(items+2,128);
      if (src[(items+1)*CHANNEL_SKIP(chan)+CHANNEL_ADD(chan)] ==
	  src[(items+2)*CHANNEL_SKIP(chan)+CHANNEL_ADD(chan)]) {
	return items;
      } else {
	items++;
      }
    }
    items ++;
  }
  return MIN(items, 128);
}

/* Find the max number of chars < 128 that are the same */
static int 
find_max_replicated(char *src, int srclen, enum color_channel chan)
{
  int items = 0;

  while (items*CHANNEL_SKIP(chan) < srclen && 
	 items < 128) {
    if (items*CHANNEL_SKIP(chan) == srclen-1) return items+1;
    if (src[items*CHANNEL_SKIP(chan)+CHANNEL_ADD(chan)] !=
	src[(items+1)*CHANNEL_SKIP(chan)+CHANNEL_ADD(chan)]) {
      /* Two different */
      return items+1;
    }
    items++;
  }
  return MIN(items+1, 128);
}

/* Return a 128-terminated char array with the RLE-encoding of src */
static guchar *
RLE_encode(guchar *src, int srclen, enum color_channel chan, int *len) {
  char *output;
  gint output_alloc, output_pos;
  gint src_pos;
  
  output_alloc = 256;
  output = (char *)g_malloc(output_alloc);
  output_pos = src_pos = 0;

  while (src_pos < srclen) {
    /* Find the size of the next block to encode */
    int max_same = find_max_replicated(src+src_pos, srclen-src_pos, chan);
    if (max_same > 2) {
      /* This is a good replication block */
      output[output_pos] = 257-max_same;
      output[output_pos+1] = src[src_pos+CHANNEL_ADD(chan)];
      output_pos += 2;
      src_pos += max_same*CHANNEL_SKIP(chan);
    } else {
      /* Put out a block of non-replicated bytes */
      int max_non_repl = find_max_non_replicated(src+src_pos, srclen-src_pos, chan);
      int i;

      output[output_pos] = max_non_repl-1;
      for (i = 0; i < max_non_repl; i++) {
	output[output_pos+i+1] = src[src_pos+i*CHANNEL_SKIP(chan)+CHANNEL_ADD(chan)];
      }
      output_pos += max_non_repl+1;
      src_pos += max_non_repl*CHANNEL_SKIP(chan);
    }

    if (output_alloc < output_pos + 128) {
      output_alloc *= 2;
      output = (char *)g_realloc(output, output_alloc);
    }
  }
  output[output_pos++] = 0x80;
  *len = output_pos;
  return output;
}

static guchar *
ASCII85_encode(guchar *src, int srclen) {
  int blocks = (srclen+3)/4;
  guchar *output = (char *)g_malloc(blocks*5+3);
  int output_pos = 0, i;
  unsigned long val;

  for (i = 0; i < blocks-1; i++) {
    val = (src[i*4]<<24)+(src[i*4+1]<<16)+(src[i*4+2]<<8)+src[i*4+3];
    if (val == 0) {
      output[output_pos++] = 'z';
    } else {
      output[output_pos+4] = (val%85)+33;
      val /= 85;
      output[output_pos+3] = (val%85)+33;
      val /= 85;
      output[output_pos+2] = (val%85)+33;
      val /= 85;
      output[output_pos+1] = (val%85)+33;
      val /= 85;
      output[output_pos+0] = (val%85)+33;
      output_pos += 5;
    }
  }

  /* Special treatment for last block */
  val = (src[i*4]<<24);
  if (srclen%4 != 1) {
    val += (src[i*4+1]<<16);
    if (srclen%4 != 2) {
      val += (src[i*4+2]<<8);
      if (srclen%4 != 3) {
	val += src[i*4+3];
      }
    }
  }
  output[output_pos+4] = (val%85)+33;
  val /= 85;
  output[output_pos+3] = (val%85)+33;
  val /= 85;
  output[output_pos+2] = (val%85)+33;
  val /= 85;
  output[output_pos+1] = (val%85)+33;
  val /= 85;
  output[output_pos+0] = (val%85)+33;
  output_pos += 5;

  output[output_pos] = '~';
  output[output_pos+1] = '>';
  output[output_pos+2] = 0;
  return output;
}

/* For testing porpoises only */
static guchar *
extract_channel(guchar *src, int srclen, enum color_channel chan, int *len)
{
  guchar *output = (guchar *)g_malloc(srclen/3);
  int i;

  for (i = 0; i < srclen/3; i++) {
    output[i] = src[i*3+CHANNEL_ADD(chan)];
  }
  return output;
}

#endif

static void
draw_image(RendererEPS *renderer,
	   Point *point,
	   real width, real height,
	   DiaImage image)
{
  int img_width, img_height, img_rowstride;
  int v;
  int                 x, y;
  unsigned char      *ptr;
  real ratio;
  guint8 *rgb_data;
  guint8 *mask_data;

  img_width = dia_image_width(image);
  img_rowstride = dia_image_rowstride(image);
  img_height = dia_image_height(image);

  rgb_data = dia_image_rgb_data(image);
  
  mask_data = dia_image_mask_data(image);

  ratio = height/width;

  fprintf(renderer->file, "gs\n");
  if (1) { /* Color output - experimental */
    guchar *rle;
    guchar *ascii;
    int len;

    fprintf(renderer->file, "/pix %i string def\n", img_width * 3);
    fprintf(renderer->file, "%i %i 8\n", img_width, img_height);
    fprintf(renderer->file, "%f %f tr\n", point->x, point->y);
    fprintf(renderer->file, "%f %f sc\n", width, height);
    fprintf(renderer->file, "[%i 0 0 %i 0 0]\n", img_width, img_height);

    /*
    fprintf(renderer->file, "/grays %i string def\n", img_width);
    fprintf(renderer->file, "/npixls 0 def\n");
    fprintf(renderer->file, "/rgbindx 0 def\n");
    */
#undef ASCII85
#undef FILTERS
#ifdef FILTERS
#if RLE
    fprintf(renderer->file, "currentfile\n");
#ifdef ASCII85
    fprintf(renderer->file, "/ASCII85Decode filter\n");
#endif
    fprintf(renderer->file, "0 /RunLengthDecode filter\n");
    fprintf(renderer->file, "currentfile\n");
#ifdef ASCII85
    fprintf(renderer->file, "/ASCII85Decode filter\n");
#endif
    fprintf(renderer->file, "0 /RunLengthDecode filter\n");
    fprintf(renderer->file, "currentfile\n");
#ifdef ASCII85
    fprintf(renderer->file, "/ASCII85Decode filter\n");
#endif
    fprintf(renderer->file, "0 /RunLengthDecode filter\n");
    fprintf(renderer->file, "true 3 colorimage\n");
    rle = RLE_encode(rgb_data, img_width*img_height*3, RED, &len);
#ifdef ASCII85
    ascii = ASCII85_encode(rle, len);
    fprintf(renderer->file, "%s\n", ascii);
    g_free(ascii);
#else
    fwrite(rle, len, 1, renderer->file);
#endif
    g_free(rle);
    rle = RLE_encode(rgb_data, img_width*img_height*3, GREEN, &len);
#ifdef ASCII85
    ascii = ASCII85_encode(rle, len);
    fprintf(renderer->file, "%s\n", ascii);
    g_free(ascii);
#else
    fwrite(rle, len, 1, renderer->file);
#endif
    g_free(rle);
    rle = RLE_encode(rgb_data, img_width*img_height*3, BLUE, &len);
#ifdef ASCII85
    ascii = ASCII85_encode(rle, len);
    fprintf(renderer->file, "%s\n", ascii);
    g_free(ascii);
#else
    fwrite(rle, len, 1, renderer->file);
#endif
    g_free(rle);
#else /* No filters */
    fprintf(renderer->file, "currentfile\n");
#ifdef ASCII85
    fprintf(renderer->file, "/ASCII85Decode filter\n");
#endif
    fprintf(renderer->file, "currentfile\n");
#ifdef ASCII85
    fprintf(renderer->file, "/ASCII85Decode filter\n");
#endif
    fprintf(renderer->file, "currentfile\n");
#ifdef ASCII85
    fprintf(renderer->file, "/ASCII85Decode filter\n");
#endif
    fprintf(renderer->file, "true 3 colorimage\n");
    rle = extract_channel(rgb_data, img_width*img_height*3, RED, &len);
#ifdef ASCII85
    ascii = ASCII85_encode(rle, len);
    fprintf(renderer->file, "%s\n", ascii);
    g_free(ascii);
#else
    fwrite(rle, len, 1, renderer->file);
#endif
    g_free(rle);
    rle = extract_channel(rgb_data, img_width*img_height*3, GREEN, &len);
#ifdef ASCII85
    ascii = ASCII85_encode(rle, len);
    fprintf(renderer->file, "%s\n", ascii);
    g_free(ascii);
#else
    fwrite(rle, len, 1, renderer->file);
#endif
    g_free(rle);
    rle = extract_channel(rgb_data, img_width*img_height*3, BLUE, &len);
#ifdef ASCII85
    ascii = ASCII85_encode(rle, len);
    fprintf(renderer->file, "%s\n", ascii);
    g_free(ascii);
#else
    fwrite(rle, len, 1, renderer->file);
#endif
    g_free(rle);
#endif

#else
    fprintf(renderer->file, "{currentfile pix readhexstring pop}\n");
    fprintf(renderer->file, "false 3 colorimage\n");
    fprintf(renderer->file, "\n");
    
#define ALPHA_TO_WHITE
    ptr = rgb_data;
    for (y = 0; y < img_height; y++) {
      for (x = 0; x < img_width; x++) {
#ifdef ALPHA_TO_WHITE
	if (mask_data) {
	  fprintf(renderer->file, "%02x", 255-(mask_data[y*img_rowstride+x]*(255-*ptr)/255));
	  ptr++;
	  fprintf(renderer->file, "%02x", 255-(mask_data[y*img_rowstride+x]*(255-*ptr)/255));
	  ptr++;
	  fprintf(renderer->file, "%02x", 255-(mask_data[y*img_rowstride+x]*(255-*ptr)/255));
	  ptr++;
	} else {
	  fprintf(renderer->file, "%02x", (int)(*ptr++));
	  fprintf(renderer->file, "%02x", (int)(*ptr++));
	  fprintf(renderer->file, "%02x", (int)(*ptr++));
	}
#else
	if (!mask_data || mask_data[y*img_rowstride+x] > 127) {
	  fprintf(renderer->file, "%02x", (int)(*ptr++));
	  fprintf(renderer->file, "%02x", (int)(*ptr++));
	  fprintf(renderer->file, "%02x", (int)(*ptr++));
	} else {
	  fprintf(renderer->file, "FFFFFF");
	  ptr+=3;
	}
#endif
      }
      fprintf(renderer->file, "\n");
      ptr += img_rowstride-img_width*3;
    }
#endif
  } else { /* Grayscale */
    fprintf(renderer->file, "/pix %i string def\n", img_width);
    fprintf(renderer->file, "/grays %i string def\n", img_width);
    fprintf(renderer->file, "/npixls 0 def\n");
    fprintf(renderer->file, "/rgbindx 0 def\n");
    fprintf(renderer->file, "%f %f tr\n", point->x, point->y);
    fprintf(renderer->file, "%f %f sc\n", width, height);
    fprintf(renderer->file, "%i %i 8\n", img_width, img_height);
    fprintf(renderer->file, "[%i 0 0 %i 0 0]\n", img_width, img_height);
    fprintf(renderer->file, "{currentfile pix readhexstring pop}\n");
    fprintf(renderer->file, "image\n");
    fprintf(renderer->file, "\n");
    ptr = rgb_data;
    for (y = 0; y < img_height; y++) {
      for (x = 0; x < img_width; x++) {
	v = (int)(*ptr++);
	v += (int)(*ptr++);
	v += (int)(*ptr++);
	v /= 3;
	fprintf(renderer->file, "%02x", v);
      }
      fprintf(renderer->file, "\n");
    }
  }
  /*  fprintf(renderer->file, "%f %f scale\n", 1.0, 1.0/ratio);*/
  fprintf(renderer->file, "gr\n");
  fprintf(renderer->file, "\n");
  
  g_free(rgb_data);
  g_free(mask_data);
}

/* --- export filter interface --- */
static void
export_eps(DiagramData *data, const gchar *filename, 
           const gchar *diafilename, void* user_data)
{
  RendererEPS *renderer;
  char *old_locale;

  old_locale = setlocale(LC_NUMERIC, "C");
  if ((renderer = create_eps_renderer(data, filename, diafilename))) {
    data_render(data, (Renderer *)renderer, NULL, NULL, NULL);
    eps_renderer_prolog_done(renderer);
    eps_renderer_set_scale(data, renderer);
    data_render(data, (Renderer *)renderer, NULL, NULL, NULL);
    destroy_eps_renderer(renderer);
  }
  setlocale(LC_NUMERIC, old_locale);
}

static const gchar *extensions[] = { "eps", "epsi", NULL };
DiaExportFilter eps_export_filter = {
  N_("Encapsulated Postscript"),
  extensions,
  export_eps
};

