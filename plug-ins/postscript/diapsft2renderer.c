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

#include <config.h>

#include <string.h>

#include "font.h"

#include "diapsft2renderer.h"
#include "textline.h"

#undef PANGO_DISABLE_DEPRECATED /* pango_ft2_font_get_face */
#include <pango/pango.h>
#include <pango/pangoft2.h>
#include <pango/pango-engine.h>
/* I'd really rather avoid this */
#include <ft2build.h>
#include FT_GLYPH_H
#include FT_OUTLINE_H

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
static int paps_move_to( const FT_Vector* to,
			 void *user_data);
static int paps_line_to( const FT_Vector*  to,
			 void *user_data);
static int paps_conic_to( const FT_Vector*  control,
			  const FT_Vector*  to,
			  void *user_data);
static int paps_cubic_to( const FT_Vector*  control1,
			  const FT_Vector*  control2,
			  const FT_Vector*  to,
			  void *user_data);

static void dia_ps_ft2_renderer_class_init (DiaPsFt2RendererClass *klass);

static void
draw_text_line(DiaRenderer *self,
	       TextLine *text_line,
	       Point *pos, Alignment alignment, Color *color);

static gpointer parent_class = NULL;

static void
set_font(DiaRenderer *self, DiaFont *font, real height)
{
  DiaPsFt2Renderer *renderer = DIA_PS_FT2_RENDERER(self);

  if (renderer->current_font != font) {
    if (renderer->current_font != NULL) {
      dia_font_unref(renderer->current_font);
    }
    renderer->current_font = font;
    /* Dammit!  We have a random factor once again! 
     * And not only here but also in dia_font_scaled_build_layout() below ...
     */
    renderer->current_height = height/**4.3*/;
    dia_font_ref(font);
  }
  pango_context_set_font_description(dia_font_get_context(), 
				     dia_font_get_description(font));
}

/* ********************************************************* */
/*		   String rendering using PangoFt2           */
/* ********************************************************* */
/* Such a big mark really is a sign that this should go in its own file:) */
/* And so it did. */

/*======================================================================
  outline traversing functions.
  ----------------------------------------------------------------------*/
static int paps_move_to( const FT_Vector* to,
			 void *user_data)
{
  OutlineInfo *outline_info = (OutlineInfo*)user_data;
  fprintf(outline_info->OUT, "%d %d moveto\n",
	  (int)to->x ,
	  (int)to->y );
  return 0;
}

static int paps_line_to( const FT_Vector*  to,
			 void *user_data)
{
  OutlineInfo *outline_info = (OutlineInfo*)user_data;
  fprintf(outline_info->OUT, "%d %d lineto\n",
	  (int)to->x ,
	  (int)to->y );
  return 0;
}

static int paps_conic_to( const FT_Vector*  control,
			  const FT_Vector*  to,
			  void *user_data)
{
  OutlineInfo *outline_info = (OutlineInfo*)user_data;
  fprintf(outline_info->OUT, "%d %d %d %d conicto\n",
	  (int)control->x  ,
	  (int)control->y  ,
	  (int)to->x   ,
	  (int)to->y  );
  return 0;
}

static int paps_cubic_to( const FT_Vector*  control1,
			  const FT_Vector*  control2,
			  const FT_Vector*  to,
			  void *user_data)
{
  OutlineInfo *outline_info = (OutlineInfo*)user_data;
  fprintf(outline_info->OUT,
	  "%d %d %d %d %d %d curveto\n",
	  (int)control1->x , 
	  (int)control1->y ,
	  (int)control2->x ,
	  (int)control2->y ,
	  (int)to->x ,
	  (int)to->y );
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

  /* First calculate number of runs in text */
  runs_list = pango_line->runs;
  while(runs_list)
  {
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
    FT_Face ft_face;
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
  gchar px_buf[G_ASCII_DTOSTR_BUF_SIZE];
  gchar py_buf[G_ASCII_DTOSTR_BUF_SIZE];
  gchar d1_buf[G_ASCII_DTOSTR_BUF_SIZE];
  gchar d2_buf[G_ASCII_DTOSTR_BUF_SIZE];

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
	  "gsave %s %s translate %s %s scale\n",
	  g_ascii_formatd(px_buf, sizeof(px_buf), "%f", pos_x),
	  g_ascii_formatd(py_buf, sizeof(py_buf), "%f", pos_y),
	  g_ascii_formatd(d1_buf, sizeof(d1_buf), "%f", 2.54/72.0),
	  g_ascii_formatd(d2_buf, sizeof(d2_buf), "%f", -2.54/72.0) );
  fprintf(renderer->file, "start_ol\n");

  if ((error=FT_Load_Glyph(face, glyph_index, load_flags))) {
    fprintf(stderr, "Can't load glyph: %d\n", error);
    return;
  }
  if ((error=FT_Get_Glyph (face->glyph, &glyph))) {
    fprintf(stderr, "Can't get glyph: %d\n", error);
    FT_Done_Glyph (glyph);
    return;
  }
  if (face->glyph->format == FT_GLYPH_FORMAT_OUTLINE)
    FT_Outline_Decompose (&(((FT_OutlineGlyph)glyph)->outline),
                          &outlinefunc, &outline_info);
  fprintf(renderer->file, "end_ol grestore \n");
  
  FT_Done_Glyph (glyph);
}

static void
draw_string(DiaRenderer *self,
	    const char *text,
	    Point *pos, Alignment alignment,
	    Color *color)
{
  DiaPsFt2Renderer *renderer = DIA_PS_FT2_RENDERER(self);
  TextLine *text_line = text_line_new(text, renderer->current_font,
				      renderer->current_height);
  draw_text_line(self, text_line, pos, alignment, color);
  text_line_destroy(text_line);
}

static void
draw_text_line(DiaRenderer *self,
	       TextLine *text_line,
	       Point *pos, Alignment alignment, Color *color)
{
  DiaPsFt2Renderer *renderer = DIA_PS_FT2_RENDERER(self);
  PangoLayout *layout;
  int line, linecount;
  double xpos = pos->x, ypos = pos->y;
  char *text = text_line_get_string(text_line);
  /* TODO: we could probably pass the alignment down to the PS file? */
  xpos -= text_line_get_alignment_adjustment (text_line, alignment);

/* Using the global PangoContext does not allow to have renderer specific 
 * different ones. Or it implies the push/pop _context mess. Anyway just 
 * get rid of warnings for now. But the local code may be resurreted 
 * sooner or later...                                               --hb
 */
#define USE_GLOBAL_CONTEXT
#ifndef USE_GLOBAL_CONTEXT
  PangoAttrList* list;
  PangoAttribute* attr;
  guint length;
#endif

  if ((!text)||(text == (const char *)(1))) return;

  lazy_setcolor(DIA_PS_RENDERER(renderer),color);

#define ANNOYING_SCALE_FACTOR 5.9

  /* Make sure the letters aren't too wide. */
#ifdef USE_GLOBAL_CONTEXT
  layout = dia_font_build_layout(text, text_line_get_font(text_line),
				 text_line_get_height(text_line)*ANNOYING_SCALE_FACTOR);
#else
  /* approximately what would be required but w/o dia_font_get_context() */
  dia_font_set_height(text_line_get_font(text_line),
		      text_line_get_height(text_line));
  layout = pango_layout_new(dia_font_get_context());

  length = text ? strlen(text) : 0;
  pango_layout_set_text(layout,text,length);
        
  list = pango_attr_list_new();

  attr = pango_attr_font_desc_new(dia_font_get_description(text_line_get_font(text_line)));
  attr->start_index = 0;
  attr->end_index = length;
  pango_attr_list_insert(list,attr);
    
  pango_layout_set_attributes(layout,list);
  pango_attr_list_unref(list);

  pango_layout_set_indent(layout,0);
  pango_layout_set_justify(layout,FALSE);
#endif

  pango_layout_set_alignment(layout, PANGO_ALIGN_LEFT);
    
  linecount = pango_layout_get_line_count(layout);
  for (line = 0; line < linecount; line++) {
    PangoLayoutLine *layoutline = pango_layout_get_line(layout, line);

    /* Not sure scale is the right one here.  */
    text_line_adjust_layout_line(text_line, layoutline, ANNOYING_SCALE_FACTOR);

    postscript_draw_contour(DIA_PS_RENDERER(renderer),
			    DPI, /* dpi_x */
			    layoutline, xpos, ypos);
    ypos += 10;/* Some line height thing??? */
  }
}

static void
dump_fonts (DiaPsRenderer *renderer)
{
  postscript_contour_headers(renderer->file, DPI, DPI);
}

GType
dia_ps_ft2_renderer_get_type (void)
{
  static GType object_type = 0;

  if (!object_type)
    {
      static const GTypeInfo object_info =
      {
        sizeof (DiaPsFt2RendererClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) dia_ps_ft2_renderer_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (DiaPsFt2Renderer),
        0,              /* n_preallocs */
	NULL            /* init */
      };

      object_type = g_type_register_static (DIA_TYPE_PS_RENDERER,
                                            "DiaPsFt2Renderer",
                                            &object_info, 0);
    }
  
  return object_type;
}

static void
dia_ps_ft2_renderer_finalize (GObject *object)
{
  DiaPsFt2Renderer *renderer = DIA_PS_FT2_RENDERER(object);
  if (renderer->current_font != NULL) {
    dia_font_unref(renderer->current_font);
    renderer->current_font = NULL;
  }
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
dia_ps_ft2_renderer_class_init (DiaPsFt2RendererClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  DiaRendererClass *renderer_class = DIA_RENDERER_CLASS (klass);
  DiaPsRendererClass *ps_renderer_class = DIA_PS_RENDERER_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize = dia_ps_ft2_renderer_finalize;

  renderer_class->set_font     = set_font;
  renderer_class->draw_string  = draw_string;

  renderer_class->draw_text_line = draw_text_line;

  /* ps specific */
  ps_renderer_class->dump_fonts   = dump_fonts;
}
