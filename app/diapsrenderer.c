/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * diapsrenderer.c -- implements the base class for Postscript rendering
 *   It is mostly refactoring of render_eps.c (some stuff not from the 
 *   latest version but from 1.24) before PS rendering became multi-pass
 *   and text rendering became (necessary) complicated.
 * Refatoring: Copyright (C) 2002 Hans Breuer
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

#include <config.h>

#include <string.h>
#include <time.h>

#include "diapsrenderer.h"
#include "message.h"
#include "dia_image.h"
#include "font.h"

#ifdef HAVE_FREETYPE

#include <pango/pango.h>
#include <pango/pangoft2.h>
/* I'd really rather avoid this */
#include <freetype/ftglyph.h>

#define DPI 300

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
void postscript_draw_contour(DiaPsRenderer *renderer,
			     int dpi_x,
			     PangoLayoutLine *pango_line,
			     double x_pos,
			     double y_pos
			     );
void draw_bezier_outline(DiaPsRenderer *renderer,
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

void
lazy_setcolor(DiaPsRenderer *renderer,
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
begin_render(DiaRenderer *self)
{
  DiaPsRenderer *renderer = DIA_PS_RENDERER(self);
  time_t time_now;

  g_assert (renderer->file != NULL);

  time_now  = time(NULL);

  if (renderer->is_eps)
    fprintf(renderer->file,
            "%%!PS-Adobe-2.0 EPSF-2.0\n");
  else
    fprintf(renderer->file,
            "%%!PS-Adobe-2.0\n");
  fprintf(renderer->file,
          "%%%%Title: %s\n"
          "%%%%Creator: Dia v%s\n"
          "%%%%CreationDate: %s"
          "%%%%For: %s\n"
          "%%%%Orientation: %s\n",
          renderer->title ? renderer->title : "(NULL)" ,
          VERSION,
          ctime(&time_now),
          g_get_user_name(),
          renderer->is_portrait ? "Portrait" : "Landscape");

  if (renderer->is_eps)
    fprintf(renderer->file,
            "%%%%Magnification: 1.0000\n"
	  "%%%%BoundingBox: 0 0 %d %d\n",
            (int) ceil(  (renderer->extent.right - renderer->extent.left) 
                       * renderer->scale),
            (int) ceil(  (renderer->extent.bottom - renderer->extent.top) 
                       * renderer->scale) );

  else
    fprintf(renderer->file,
	  "%%%%DocumentPaperSizes: %s\n",
            renderer->paper ? renderer->paper : "(NULL)");

  fprintf(renderer->file,
          "%%%%BeginSetup\n");
  fprintf(renderer->file,
          "%%%%EndSetup\n"
          "%%%%EndComments\n");

  DIA_PS_RENDERER_GET_CLASS(self)->begin_prolog(renderer);
  /* get our font definitions */
  DIA_PS_RENDERER_GET_CLASS(self)->dump_fonts(renderer);
  /* done it */
  DIA_PS_RENDERER_GET_CLASS(self)->end_prolog(renderer);
}

static void
end_render(DiaRenderer *self)
{
  DiaPsRenderer *renderer = DIA_PS_RENDERER(self);

  if (renderer->is_eps)
    fprintf(renderer->file, "showpage\n");
}

static void
set_linewidth(DiaRenderer *self, real linewidth)
{  /* 0 == hairline **/
  DiaPsRenderer *renderer = DIA_PS_RENDERER(self);

  /* Adobe's advice:  Set to very small but fixed size, to avoid changes
   * due to different resolution output. */
  /* .01 cm becomes <5 dots on 1200 DPI */
  if (linewidth == 0.0) linewidth=.01;
  fprintf(renderer->file, "%f slw\n", (double) linewidth);
}

static void
set_linecaps(DiaRenderer *self, LineCaps mode)
{
  DiaPsRenderer *renderer = DIA_PS_RENDERER(self);
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
set_linejoin(DiaRenderer *self, LineJoin mode)
{
  DiaPsRenderer *renderer = DIA_PS_RENDERER(self);
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
set_linestyle(DiaRenderer *self, LineStyle mode)
{
  DiaPsRenderer *renderer = DIA_PS_RENDERER(self);
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
set_dashlength(DiaRenderer *self, real length)
{  /* dot = 20% of len */
  DiaPsRenderer *renderer = DIA_PS_RENDERER(self);

  if (length<0.001)
    length = 0.001;
  
  renderer->dash_length = length;
  renderer->dot_length = length*0.2;
  
  set_linestyle(self, renderer->saved_line_style);
}

static void
set_fillstyle(DiaRenderer *self, FillStyle mode)
{
  DiaPsRenderer *renderer = DIA_PS_RENDERER(self);

  switch(mode) {
  case FILLSTYLE_SOLID:
    break;
  default:
    message_error("%s : Unsupported fill mode specified!\n",
                  G_OBJECT_CLASS_NAME (G_OBJECT_GET_CLASS (self)));
  }
}

static void
set_font(DiaRenderer *self, DiaFont *font, real height)
{
  DiaPsRenderer *renderer = DIA_PS_RENDERER(self);

#ifdef HAVE_FREETYPE
  renderer->current_font = font;
  /* Where did this 1.8 come from? */
  /* Used to be just as arbitrarily 1.6, but experimentation shows 1.8
     is better.  This holds only when using Freetype, in other cases all
     bets are really off, especially since some fonts may not be found.
  */
  /* 28.346 = 72.0 / 2.54 */
  renderer->current_height = height*DPI/2.54; /* Account for default zoom? */
  pango_context_set_font_description(dia_font_get_context(), font->pfd);
#else
  fprintf(renderer->file, "/%s-latin1 ff %f scf sf\n",
          dia_font_get_psfontname(font), (double)height);
#endif
}

static void
draw_line(DiaRenderer *self, 
	  Point *start, Point *end, 
	  Color *line_color)
{
  DiaPsRenderer *renderer = DIA_PS_RENDERER(self);

  lazy_setcolor(renderer,line_color);

  fprintf(renderer->file, "n %f %f m %f %f l s\n",
	  start->x, start->y, end->x, end->y);
}

static void
draw_polyline(DiaRenderer *self, 
	      Point *points, int num_points, 
	      Color *line_color)
{
  DiaPsRenderer *renderer = DIA_PS_RENDERER(self);
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
draw_polygon(DiaRenderer *self, 
	      Point *points, int num_points, 
	      Color *line_color)
{
  DiaPsRenderer *renderer = DIA_PS_RENDERER(self);
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
fill_polygon(DiaRenderer *self, 
	      Point *points, int num_points, 
	      Color *fill_color)
{
  DiaPsRenderer *renderer = DIA_PS_RENDERER(self);
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
draw_rect(DiaRenderer *self, 
	  Point *ul_corner, Point *lr_corner,
	  Color *color)
{
  DiaPsRenderer *renderer = DIA_PS_RENDERER(self);

  lazy_setcolor(renderer,color);
  
  fprintf(renderer->file, "n %f %f m %f %f l %f %f l %f %f l cp s\n",
	  (double) ul_corner->x, (double) ul_corner->y,
	  (double) ul_corner->x, (double) lr_corner->y,
	  (double) lr_corner->x, (double) lr_corner->y,
	  (double) lr_corner->x, (double) ul_corner->y );
}

static void
fill_rect(DiaRenderer *self, 
	  Point *ul_corner, Point *lr_corner,
	  Color *color)
{
  DiaPsRenderer *renderer = DIA_PS_RENDERER(self);

  lazy_setcolor(renderer,color);

  fprintf(renderer->file, "n %f %f m %f %f l %f %f l %f %f l f\n",
	  (double) ul_corner->x, (double) ul_corner->y,
	  (double) ul_corner->x, (double) lr_corner->y,
	  (double) lr_corner->x, (double) lr_corner->y,
	  (double) lr_corner->x, (double) ul_corner->y );
}

static void
draw_arc(DiaRenderer *self, 
	 Point *center,
	 real width, real height,
	 real angle1, real angle2,
	 Color *color)
{
  DiaPsRenderer *renderer = DIA_PS_RENDERER(self);

  lazy_setcolor(renderer,color);

  fprintf(renderer->file, "n %f %f %f %f %f %f ellipse s\n",
	  (double) center->x, (double) center->y,
	  (double) width/2.0, (double) height/2.0,
	  (double) 360.0 - angle2, (double) 360.0 - angle1 );
}

static void
fill_arc(DiaRenderer *self, 
	 Point *center,
	 real width, real height,
	 real angle1, real angle2,
	 Color *color)
{
  DiaPsRenderer *renderer = DIA_PS_RENDERER(self);

  lazy_setcolor(renderer,color);

  fprintf(renderer->file, "n %f %f m %f %f %f %f %f %f ellipse f\n",
	  (double) center->x, (double) center->y,
	  (double) center->x, (double) center->y,
	  (double) width/2.0, (double) height/2.0,
	  (double) 360.0 - angle2, (double) 360.0 - angle1 );
}

static void
draw_ellipse(DiaRenderer *self, 
	     Point *center,
	     real width, real height,
	     Color *color)
{
  DiaPsRenderer *renderer = DIA_PS_RENDERER(self);

  lazy_setcolor(renderer,color);

  fprintf(renderer->file, "n %f %f %f %f 0 360 ellipse cp s\n",
	  (double) center->x, (double) center->y,
	  (double) width/2.0, (double) height/2.0 );
}

static void
fill_ellipse(DiaRenderer *self, 
	     Point *center,
	     real width, real height,
	     Color *color)
{
  DiaPsRenderer *renderer = DIA_PS_RENDERER(self);

  lazy_setcolor(renderer,color);

  fprintf(renderer->file, "n %f %f %f %f 0 360 ellipse f\n",
	  (double) center->x, (double) center->y,
	  (double) width/2.0, (double) height/2.0 );
}

static void
draw_bezier(DiaRenderer *self, 
	    BezPoint *points,
	    int numpoints, /* numpoints = 4+3*n, n=>0 */
	    Color *color)
{
  DiaPsRenderer *renderer = DIA_PS_RENDERER(self);
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
fill_bezier(DiaRenderer *self, 
	    BezPoint *points, /* Last point must be same as first point */
	    int numpoints,
	    Color *color)
{
  DiaPsRenderer *renderer = DIA_PS_RENDERER(self);
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
void postscript_draw_contour(DiaPsRenderer *renderer,
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

void draw_bezier_outline(DiaPsRenderer *renderer,
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
#endif

static void
draw_string(DiaRenderer *self,
	    const char *text,
	    Point *pos, Alignment alignment,
	    Color *color)
{
#ifdef HAVE_FREETYPE
  DiaPsRenderer *renderer = DIA_PS_RENDERER(self);
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
  layout = pango_layout_new(dia_font_get_context());

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
#else
  DiaPsRenderer *renderer = DIA_PS_RENDERER(self);
  char *buffer;
  const char *str;
  int len;

  if (1 > strlen(text))
    return;

  lazy_setcolor(renderer,color);

  /* TODO: Use latin-1 encoding */

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
#endif
}

static void
draw_image(DiaRenderer *self,
	   Point *point,
	   real width, real height,
	   DiaImage image)
{
  DiaPsRenderer *renderer = DIA_PS_RENDERER(self);
  int img_width, img_height, img_rowstride;
  int v;
  int                 x, y;
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

  /* color output only */
  fprintf(renderer->file, "/pix %i string def\n", img_width * 3);
  fprintf(renderer->file, "%i %i 8\n", img_width, img_height);
  fprintf(renderer->file, "%f %f tr\n", point->x, point->y);
  fprintf(renderer->file, "%f %f sc\n", width, height);
  fprintf(renderer->file, "[%i 0 0 %i 0 0]\n", img_width, img_height);

  fprintf(renderer->file, "{currentfile pix readhexstring pop}\n");
  fprintf(renderer->file, "false 3 colorimage\n");
  fprintf(renderer->file, "\n");

  if (mask_data) {
    for (y = 0; y < img_width; y++) {
      for (x = 0; x < img_height; x++) {
        int i = y*img_height+x;
        fprintf(renderer->file, "%02x", 255-(mask_data[i]*(255-rgb_data[i*3])/255));
        fprintf(renderer->file, "%02x", 255-(mask_data[i]*(255-rgb_data[i*3+1])/255));
        fprintf(renderer->file, "%02x", 255-(mask_data[i]*(255-rgb_data[i*3+2])/255));
      }
      fprintf(renderer->file, "\n");
    }
  } else {
    guint8 *ptr = rgb_data;
    for (y = 0; y < img_width; y++) {
      for (x = 0; x < img_height; x++) {
        fprintf(renderer->file, "%02x", (int)(*ptr++));
        fprintf(renderer->file, "%02x", (int)(*ptr++));
        fprintf(renderer->file, "%02x", (int)(*ptr++));
      }
      fprintf(renderer->file, "\n");
    }
  }
  fprintf(renderer->file, "gr\n");
  fprintf(renderer->file, "\n");
   
  g_free(rgb_data);
  g_free(mask_data);
}

static void
begin_prolog (DiaPsRenderer *renderer)
{
  g_assert(renderer->file != NULL);

  fprintf(renderer->file, "%%%%BeginProlog\n");
  fprintf(renderer->file,
	  "[ /.notdef /.notdef /.notdef /.notdef /.notdef /.notdef /.notdef /.notdef /.notdef /.notdef\n"
	  "/.notdef /.notdef /.notdef /.notdef /.notdef /.notdef /.notdef /.notdef /.notdef /.notdef\n"
	  "/.notdef /.notdef /.notdef /.notdef /.notdef /.notdef /.notdef /.notdef /.notdef /.notdef\n"
	  "/.notdef /.notdef /space /exclam /quotedbl /numbersign /dollar /percent /ampersand /quoteright\n"
	  "/parenleft /parenright /asterisk /plus /comma /hyphen /period /slash /zero /one\n"
	  "/two /three /four /five /six /seven /eight /nine /colon /semicolon\n"
	  "/less /equal /greater /question /at /A /B /C /D /E\n"
	  "/F /G /H /I /J /K /L /M /N /O\n"
	  "/P /Q /R /S /T /U /V /W /X /Y\n"
	  "/Z /bracketleft /backslash /bracketright /asciicircum /underscore /quoteleft /a /b /c\n"
	  "/d /e /f /g /h /i /j /k /l /m\n"
	  "/n /o /p /q /r /s /t /u /v /w\n"
	  "/x /y /z /braceleft /bar /braceright /asciitilde /.notdef /.notdef /.notdef\n"
	  "/.notdef /.notdef /.notdef /.notdef /.notdef /.notdef /.notdef /.notdef /.notdef /.notdef\n"
	  "/.notdef /.notdef /.notdef /.notdef /.notdef /.notdef /.notdef /.notdef /.notdef /.notdef\n"
	  "/.notdef /.notdef /.notdef /.notdef /.notdef /.notdef /.notdef /.notdef /.notdef /.notdef\n"
	  "/space /exclamdown /cent /sterling /currency /yen /brokenbar /section /dieresis /copyright\n"
	  "/ordfeminine /guillemotleft /logicalnot /hyphen /registered /macron /degree /plusminus /twosuperior /threesuperior\n"
	  "/acute /mu /paragraph /periodcentered /cedilla /onesuperior /ordmasculine /guillemotright /onequarter /onehalf\n"
	  "/threequarters /questiondown /Agrave /Aacute /Acircumflex /Atilde /Adieresis /Aring /AE /Ccedilla\n"
	  "/Egrave /Eacute /Ecircumflex /Edieresis /Igrave /Iacute /Icircumflex /Idieresis /Eth /Ntilde\n"
	  "/Ograve /Oacute /Ocircumflex /Otilde /Odieresis /multiply /Oslash /Ugrave /Uacute /Ucircumflex\n"
	  "/Udieresis /Yacute /Thorn /germandbls /agrave /aacute /acircumflex /atilde /adieresis /aring\n"
	  "/ae /ccedilla /egrave /eacute /ecircumflex /edieresis /igrave /iacute /icircumflex /idieresis\n"
	  "/eth /ntilde /ograve /oacute /ocircumflex /otilde /odieresis /divide /oslash /ugrave\n"
	  "/uacute /ucircumflex /udieresis /yacute /thorn /ydieresis] /isolatin1encoding exch def\n");

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
	  "} bind def\n");

#ifdef HAVE_FREETYPE
  postscript_contour_headers(renderer->file, DPI, DPI);
#endif
}

#ifndef HAVE_FREETYPE
/* helper function */
static void 
print_reencode_font(FILE *file, char *fontname)
{
  /* Don't reencode the Symbol font, as it doesn't work in latin1 encoding.
   * Instead, just define Symbol-latin1 to be the same as Symbol. */
  if (!strcmp(fontname, "Symbol"))
    fprintf(file,
	    "/%s-latin1\n"
	    "    /%s findfont\n"
	    "definefont pop\n", fontname, fontname);
  else
    fprintf(file,
	    "/%s-latin1\n"
	    "    /%s findfont\n"
	    "    dup length dict begin\n"
	    "	{1 index /FID ne {def} {pop pop} ifelse} forall\n"
	    "	/Encoding isolatin1encoding def\n"
	    "    currentdict end\n"
	    "definefont pop\n", fontname, fontname);
}
#endif

static void
dump_fonts (DiaPsRenderer *renderer)
{
#ifndef HAVE_FREETYPE
  print_reencode_font(renderer->file, "Times-Roman");
  print_reencode_font(renderer->file, "Times-Italic");
  print_reencode_font(renderer->file, "Times-Bold");
  print_reencode_font(renderer->file, "Times-BoldItalic");
  print_reencode_font(renderer->file, "AvantGarde-Book");
  print_reencode_font(renderer->file, "AvantGarde-BookOblique");
  print_reencode_font(renderer->file, "AvantGarde-Demi");
  print_reencode_font(renderer->file, "AvantGarde-DemiOblique");
  print_reencode_font(renderer->file, "Bookman-Light");
  print_reencode_font(renderer->file, "Bookman-LightItalic");
  print_reencode_font(renderer->file, "Bookman-Demi");
  print_reencode_font(renderer->file, "Bookman-DemiItalic");
  print_reencode_font(renderer->file, "Courier");
  print_reencode_font(renderer->file, "Courier-Oblique");
  print_reencode_font(renderer->file, "Courier-Bold");
  print_reencode_font(renderer->file, "Courier-BoldOblique");
  print_reencode_font(renderer->file, "Helvetica");
  print_reencode_font(renderer->file, "Helvetica-Oblique");
  print_reencode_font(renderer->file, "Helvetica-Bold");
  print_reencode_font(renderer->file, "Helvetica-BoldOblique");
  print_reencode_font(renderer->file, "Helvetica-Narrow");
  print_reencode_font(renderer->file, "Helvetica-Narrow-Oblique");
  print_reencode_font(renderer->file, "Helvetica-Narrow-Bold");
  print_reencode_font(renderer->file, "Helvetica-Narrow-BoldOblique");
  print_reencode_font(renderer->file, "NewCenturySchoolbook-Roman");
  print_reencode_font(renderer->file, "NewCenturySchoolbook-Italic");
  print_reencode_font(renderer->file, "NewCenturySchoolbook-Bold");
  print_reencode_font(renderer->file, "NewCenturySchoolbook-BoldItalic");
  print_reencode_font(renderer->file, "Palatino-Roman");
  print_reencode_font(renderer->file, "Palatino-Italic");
  print_reencode_font(renderer->file, "Palatino-Bold");
  print_reencode_font(renderer->file, "Palatino-BoldItalic");
  print_reencode_font(renderer->file, "Symbol");
  print_reencode_font(renderer->file, "ZapfChancery-MediumItalic");
  print_reencode_font(renderer->file, "ZapfDingbats");
#endif
}

static void
end_prolog (DiaPsRenderer *renderer)
{
  if (renderer->is_eps) {
    fprintf(renderer->file,
            "%f %f scale\n", renderer->scale, -renderer->scale);
    fprintf(renderer->file,
            "%f %f translate\n", -renderer->extent.left, -renderer->extent.bottom);
  } else {
    /* done by BoundingBox above */
  }

  fprintf(renderer->file,
	  "%%%%EndProlog\n\n\n");
}

/* constructor */
static void
ps_renderer_init (GTypeInstance *instance, gpointer g_class)
{
  DiaPsRenderer *renderer = DIA_PS_RENDERER (instance);

  renderer->file = NULL;

  renderer->lcolor.red = -1.0;
  
  renderer->dash_length = 1.0;
  renderer->dot_length = 0.2;
  renderer->saved_line_style = LINESTYLE_SOLID;
  renderer->is_portrait = TRUE;

  renderer->scale = 28.346;
}

/* GObject stuff */
static void dia_ps_renderer_class_init (DiaPsRendererClass *klass);

static gpointer parent_class = NULL;

GType
dia_ps_renderer_get_type (void)
{
  static GType object_type = 0;

  if (!object_type)
    {
      static const GTypeInfo object_info =
      {
        sizeof (DiaPsRendererClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) dia_ps_renderer_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (DiaPsRenderer),
        0,              /* n_preallocs */
        ps_renderer_init       /* init */
      };

      object_type = g_type_register_static (DIA_TYPE_RENDERER,
                                            "DiaPsRenderer",
                                            &object_info, 0);
    }
  
  return object_type;
}

static void
dia_ps_renderer_finalize (GObject *object)
{
  DiaPsRenderer *renderer = DIA_PS_RENDERER (object);

  g_free(renderer->title);
  /*  fclose(renderer->file);*/

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
dia_ps_renderer_class_init (DiaPsRendererClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  DiaRendererClass *renderer_class = DIA_RENDERER_CLASS (klass);
  DiaPsRendererClass *ps_renderer_class = DIA_PS_RENDERER_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize = dia_ps_renderer_finalize;

  renderer_class->begin_render = begin_render;
  renderer_class->end_render   = end_render;

  renderer_class->set_linewidth  = set_linewidth;
  renderer_class->set_linecaps   = set_linecaps;
  renderer_class->set_linejoin   = set_linejoin;
  renderer_class->set_linestyle  = set_linestyle;
  renderer_class->set_dashlength = set_dashlength;
  renderer_class->set_fillstyle  = set_fillstyle;
  renderer_class->set_font       = set_font;

  renderer_class->draw_line    = draw_line;
  renderer_class->fill_polygon = fill_polygon;
  renderer_class->draw_arc     = draw_arc;
  renderer_class->fill_arc     = fill_arc;
  renderer_class->draw_ellipse = draw_ellipse;
  renderer_class->fill_ellipse = fill_ellipse;
  renderer_class->draw_string  = draw_string;
  renderer_class->draw_image   = draw_image;

  /* medium level functions */
  renderer_class->draw_bezier  = draw_bezier;
  renderer_class->fill_bezier  = fill_bezier;
  renderer_class->draw_rect = draw_rect;
  renderer_class->fill_rect = fill_rect;
  renderer_class->draw_polyline  = draw_polyline;
  renderer_class->draw_polygon   = draw_polygon;

  /* ps specific */
  ps_renderer_class->begin_prolog = begin_prolog;
  ps_renderer_class->dump_fonts = dump_fonts;
  ps_renderer_class->end_prolog = end_prolog;
}

