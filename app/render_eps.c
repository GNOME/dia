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
#include "diapsrenderer.h"

#ifdef EPS_RENDERER_USING_DIA_RENDERER

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

static void begin_render(DiaRenderer *renderer);
static void end_render(DiaRenderer *renderer);
static void set_linewidth(DiaRenderer *renderer, real linewidth);
static void set_linecaps(DiaRenderer *renderer, LineCaps mode);
static void set_linejoin(DiaRenderer *renderer, LineJoin mode);
static void set_linestyle(DiaRenderer *renderer, LineStyle mode);
static void set_dashlength(DiaRenderer *renderer, real length);
static void set_fillstyle(DiaRenderer *renderer, FillStyle mode);
static void set_font(DiaRenderer *renderer, DiaFont *font, real height);
static void draw_line(DiaRenderer *renderer, 
		      Point *start, Point *end, 
		      Color *line_color);
static void draw_polyline(DiaRenderer *renderer, 
			  Point *points, int num_points, 
			  Color *line_color);
static void draw_polygon(DiaRenderer *renderer, 
			 Point *points, int num_points, 
			 Color *line_color);
static void fill_polygon(DiaRenderer *renderer, 
			 Point *points, int num_points, 
			 Color *line_color);
static void draw_rect(DiaRenderer *renderer, 
		      Point *ul_corner, Point *lr_corner,
		      Color *color);
static void fill_rect(DiaRenderer *renderer, 
		      Point *ul_corner, Point *lr_corner,
		      Color *color);
static void draw_arc(DiaRenderer *renderer, 
		     Point *center,
		     real width, real height,
		     real angle1, real angle2,
		     Color *color);
static void fill_arc(DiaRenderer *renderer, 
		     Point *center,
		     real width, real height,
		     real angle1, real angle2,
		     Color *color);
static void draw_ellipse(DiaRenderer *renderer, 
			 Point *center,
			 real width, real height,
			 Color *color);
static void fill_ellipse(DiaRenderer *renderer, 
			 Point *center,
			 real width, real height,
			 Color *color);
static void draw_bezier(DiaRenderer *renderer, 
			BezPoint *points,
			int numpoints,
			Color *color);
static void fill_bezier(DiaRenderer *renderer, 
			BezPoint *points, /* Last point must be same as first point */
			int numpoints,
			Color *color);
static void draw_string(DiaRenderer *renderer,
			const char *text,
			Point *pos, Alignment alignment,
			Color *color);
static void draw_image(DiaRenderer *renderer,
		       Point *point,
		       real width, real height,
		       DiaImage image);

/* These functions are used to create the prolog.  Currently takes care
 * of defining necessary fonts.
 */
static void begin_prolog(DiaRenderer *renderer);
static void end_prolog(DiaRenderer *renderer);
static void prolog_define_font(DiaRenderer *renderer,
			       DiaFont *font, real height);
static void prolog_check_string(DiaRenderer *renderer,
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
void postscript_draw_contour(DiaRenderer *renderer,
			     int dpi_x,
			     PangoLayoutLine *pango_line,
			     double x_pos,
			     double y_pos
			     );
void draw_bezier_outline(DiaRenderer *renderer,
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
/*  DiaFont *font = (DiaFont *)value;
  gchar *fontname = (gchar *)key;
  FILE *file = (FILE *)data;
*/
    
}

/* This hashtable keeps track of the fonts used in the diagram.
 * Indexed by psfontname, elements are DiaFonts.
 */
static GHashTable *font_table;

void
begin_prolog(DiaRenderer *renderer)
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
end_prolog(DiaRenderer *renderer)
{}

static void
prolog_define_font(DiaRenderer *renderer, DiaFont *font, real height)
{
        /* FIXME: check the lifetime of the return value of
         dia_font_get_psfontname(). If needed, copy it (and
        g_free() it when font_table is deleted) */
  /*  g_hash_table_insert(font_table, dia_font_get_psfontname(font), font);*/
}

static void
prolog_check_string(DiaRenderer *renderer,
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

/* This function switches the DiaRenderer into render mode */
void
eps_renderer_prolog_done(DiaRenderer *renderer) {
  g_hash_table_foreach(font_table, print_define_font, renderer->file);

  fprintf(renderer->file, 
	  "%%%%EndProlog\n\n"
	  "%%%%BeginSetup\n"
	  "%%%%EndSetup\n");
  renderer->renderer.ops = EpsRenderOps;
}

static void
eps_renderer_set_scale(DiagramData *data, DiaRenderer *renderer)
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

static DiaRenderer *
create_common_renderer(DiagramData *data, FILE *file)
{
  DiaRenderer *renderer;
  double scale;
  PangoFontDescription *font_description;

  if (EpsPrologOps == NULL)
    init_eps_renderer();

  renderer = g_new(DiaRenderer, 1);
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

static DiaRenderer *
create_eps_renderer(DiagramData *data, const char *filename,
		    const char *diafilename)
{
  DiaRenderer *renderer;
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

DiaRenderer *
new_eps_renderer(Diagram *dia, gchar *filename)
{
  return create_eps_renderer(dia->data, filename, dia->filename);
}

static void
begin_render(DiaRenderer *renderer)
{
}

static void
end_render(DiaRenderer *renderer)
{
  if (!renderer->is_ps) {
    fprintf(renderer->file, "showpage\n");
    fclose(renderer->file);
  }
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
void postscript_draw_contour(DiaRenderer *renderer,
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

void draw_bezier_outline(DiaRenderer *renderer,
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
set_font(DiaRenderer *renderer, DiaFont *font, real height)
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
draw_string(DiaRenderer *renderer,
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
#endif

/* --- export filter interface --- */
static void
export_eps(DiagramData *data, const gchar *filename, 
           const gchar *diafilename, void* user_data)
{
  DiaRenderer *renderer;
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
#else
static void
export_eps(DiagramData *data, const gchar *filename, 
           const gchar *diafilename, void* user_data)
{
  DiaPsRenderer *renderer;

  renderer = g_object_new (DIA_TYPE_PS_RENDERER, NULL);
  renderer->file =  fopen(filename, "w");
  renderer->scale = 28.346 * data->paper.scaling;
  renderer->extent = data->extents;
  renderer->is_eps = TRUE;

  if (renderer->file) {
    renderer->title = g_strdup (diafilename);

    data_render(data, DIA_RENDERER(renderer), NULL, NULL, NULL);
  }
  g_object_unref (renderer);
}

DiaRenderer *
new_psprint_renderer(Diagram *dia, FILE *file)
{
  DiaPsRenderer *renderer;

  renderer = g_object_new (DIA_TYPE_PS_RENDERER, NULL);
  renderer->file = file;
  renderer->is_eps = FALSE;

  return DIA_RENDERER(renderer);
}

#endif

static const gchar *extensions[] = { "eps", "epsi", NULL };
DiaExportFilter eps_export_filter = {
  N_("Encapsulated Postscript"),
  extensions,
  export_eps
};

