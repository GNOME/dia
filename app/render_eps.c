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
#include "config.h"

#include <string.h>
#include <time.h>
#include <math.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <locale.h>

#include "intl.h"
#include "render_eps.h"
#include "message.h"
#include "diagramdata.h"
#include "charconv.h" 

#ifdef HAVE_UNICODE
#include <unicode.h>
#include "ps-utf8.h"
#endif


static void begin_render(RendererEPS *renderer, DiagramData *data);
static void end_render(RendererEPS *renderer);
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

static RenderOps EpsRenderOps = {
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


#ifdef HAVE_UNICODE
/* callback functions used by the PSUnicoder: */ 
static void eps_destroy_ps_font(gpointer usrdata, const gchar *fontname);
static void eps_build_ps_encoding(gpointer usrdata, 
                                  const gchar *name,
                                  const unicode_char_t table[PSEPAGE_SIZE]);
static void eps_build_ps_font(gpointer usrdata, 
                              const gchar *name,
                              const gchar *face,
                              const gchar *encoding_name);
static void eps_select_ps_font(gpointer usrdata, 
                               const gchar *fontname,
                               float size);
static void eps_show_string(gpointer usrdata, const gchar *string);
static void eps_get_string_width(gpointer usrdata, const gchar *string,
                                 gboolean first);

static PSUnicoderCallbacks eps_unicoder_callbacks = {
  eps_destroy_ps_font,
  eps_build_ps_encoding,
  eps_build_ps_font,
  eps_select_ps_font,
  eps_show_string,
  eps_get_string_width,
};

#else /* !HAVE_UNICODE */

static void print_reencode_font(FILE *file, char *fontname)
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
#endif /* HAVE_UNICODE */

static RendererEPS *
create_eps_renderer(DiagramData *data, const char *filename,
		    const char *diafilename)
{
  RendererEPS *renderer;
  FILE *file;
  time_t time_now;
  double scale;
  Rectangle *extent;
  char *name;
 
  file = fopen(filename, "wb");

  if (file==NULL) {
    message_error(_("Couldn't open: '%s' for writing.\n"), filename);
    return NULL;
  }

  renderer = g_new(RendererEPS, 1);
  renderer->renderer.ops = &EpsRenderOps;
  renderer->renderer.is_interactive = 0;
  renderer->renderer.interactive_ops = NULL;

  renderer->is_ps = 0;
  renderer->pagenum = 1;
  renderer->file = file;
  renderer->lcolor.red = -1.0;

  renderer->dash_length = 1.0;
  renderer->dot_length = 0.2;
  renderer->saved_line_style = LINESTYLE_SOLID;
  
  time_now  = time(NULL);
  extent = &data->extents;
  
  scale = 28.346 * data->paper.scaling;
  
  name = getlogin();
  if (name==NULL)
    name = "a user";
  
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
	  "%%%%BeginSetup\n"
	  "%%%%EndSetup\n"
	  "%%%%EndComments\n",
	  diafilename,
	  VERSION,
	  ctime(&time_now),
	  name,
	  (int) ceil((extent->right - extent->left)*scale),
	  (int) ceil((extent->bottom - extent->top)*scale) );

  fprintf(file, "%%%%BeginProlog\n");
#ifndef HAVE_UNICODE
  fprintf(file,
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

  print_reencode_font(file, "Times-Roman");
  print_reencode_font(file, "Times-Italic");
  print_reencode_font(file, "Times-Bold");
  print_reencode_font(file, "Times-BoldItalic");
  print_reencode_font(file, "AvantGarde-Book");
  print_reencode_font(file, "AvantGarde-BookOblique");
  print_reencode_font(file, "AvantGarde-Demi");
  print_reencode_font(file, "AvantGarde-DemiOblique");
  print_reencode_font(file, "Bookman-Light");
  print_reencode_font(file, "Bookman-LightItalic");
  print_reencode_font(file, "Bookman-Demi");
  print_reencode_font(file, "Bookman-DemiItalic");
  print_reencode_font(file, "Courier");
  print_reencode_font(file, "Courier-Oblique");
  print_reencode_font(file, "Courier-Bold");
  print_reencode_font(file, "Courier-BoldOblique");
  print_reencode_font(file, "Helvetica");
  print_reencode_font(file, "Helvetica-Oblique");
  print_reencode_font(file, "Helvetica-Bold");
  print_reencode_font(file, "Helvetica-BoldOblique");
  print_reencode_font(file, "Helvetica-Narrow");
  print_reencode_font(file, "Helvetica-Narrow-Oblique");
  print_reencode_font(file, "Helvetica-Narrow-Bold");
  print_reencode_font(file, "Helvetica-Narrow-BoldOblique");
  print_reencode_font(file, "NewCenturySchoolbook-Roman");
  print_reencode_font(file, "NewCenturySchoolbook-Italic");
  print_reencode_font(file, "NewCenturySchoolbook-Bold");
  print_reencode_font(file, "NewCenturySchoolbook-BoldItalic");
  print_reencode_font(file, "Palatino-Roman");
  print_reencode_font(file, "Palatino-Italic");
  print_reencode_font(file, "Palatino-Bold");
  print_reencode_font(file, "Palatino-BoldItalic");
  print_reencode_font(file, "Symbol");
  print_reencode_font(file, "ZapfChancery-MediumItalic");
  print_reencode_font(file, "ZapfDingbats");
#endif /* !HAVE_UNICODE */

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
	  */
	  "%f %f scale\n"
	  "%f %f translate\n"
	  "%%%%EndProlog\n\n\n",
	  scale, -scale,
	  -extent->left, -extent->bottom );
#ifdef HAVE_UNICODE
  renderer->psu = ps_unicoder_new(&eps_unicoder_callbacks,(gpointer)renderer);
#endif 

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
#ifdef HAVE_UNICODE
  ps_unicoder_destroy(renderer->psu);
#endif
  g_free(renderer);
}


RendererEPS *
new_psprint_renderer(Diagram *dia, FILE *file)
{
  RendererEPS *renderer;
  time_t time_now;
  double scale;
  Rectangle *extent;
  char *name;

  renderer = g_new(RendererEPS, 1);
  renderer->renderer.ops = &EpsRenderOps;
  renderer->renderer.is_interactive = 0;
  renderer->renderer.interactive_ops = NULL;

  renderer->is_ps = 1;
  renderer->pagenum = 1;
  renderer->file = file;
  renderer->lcolor.red = -1.0;
  
  renderer->dash_length = 1.0;
  renderer->dot_length = 0.2;
  renderer->saved_line_style = LINESTYLE_SOLID;
  
  time_now  = time(NULL);
  extent = &dia->data->extents;
  
  scale = 28.346;
  
  name = getlogin();
  if (name==NULL)
    name = "a user";
  
  fprintf(file,
	  "%%!PS-Adobe-2.0\n"
	  "%%%%Title: %s\n"
	  "%%%%Creator: Dia v%s\n"
	  "%%%%CreationDate: %s"
	  "%%%%For: %s\n"
	  "%%%%DocumentPaperSizes: %s\n"
	  "%%%%Orientation: %s\n"
	  "%%%%BeginSetup\n"
	  "%%%%EndSetup\n"
	  "%%%%EndComments\n",
	  dia->filename,
	  VERSION,
	  ctime(&time_now),
	  name,
	  dia->data->paper.name,
	  dia->data->paper.is_portrait ? "Portrait" : "Landscape");

  fprintf(file, "%%%%BeginProlog\n");

#ifndef HAVE_UNICODE
  fprintf(file,
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

  print_reencode_font(file, "Times-Roman");
  print_reencode_font(file, "Times-Italic");
  print_reencode_font(file, "Times-Bold");
  print_reencode_font(file, "Times-BoldItalic");
  print_reencode_font(file, "AvantGarde-Book");
  print_reencode_font(file, "AvantGarde-BookOblique");
  print_reencode_font(file, "AvantGarde-Demi");
  print_reencode_font(file, "AvantGarde-DemiOblique");
  print_reencode_font(file, "Bookman-Light");
  print_reencode_font(file, "Bookman-LightItalic");
  print_reencode_font(file, "Bookman-Demi");
  print_reencode_font(file, "Bookman-DemiItalic");
  print_reencode_font(file, "Courier");
  print_reencode_font(file, "Courier-Oblique");
  print_reencode_font(file, "Courier-Bold");
  print_reencode_font(file, "Courier-BoldOblique");
  print_reencode_font(file, "Helvetica");
  print_reencode_font(file, "Helvetica-Oblique");
  print_reencode_font(file, "Helvetica-Bold");
  print_reencode_font(file, "Helvetica-BoldOblique");
  print_reencode_font(file, "Helvetica-Narrow");
  print_reencode_font(file, "Helvetica-Narrow-Oblique");
  print_reencode_font(file, "Helvetica-Narrow-Bold");
  print_reencode_font(file, "Helvetica-Narrow-BoldOblique");
  print_reencode_font(file, "NewCenturySchoolbook-Roman");
  print_reencode_font(file, "NewCenturySchoolbook-Italic");
  print_reencode_font(file, "NewCenturySchoolbook-Bold");
  print_reencode_font(file, "NewCenturySchoolbook-BoldItalic");
  print_reencode_font(file, "Palatino-Roman");
  print_reencode_font(file, "Palatino-Italic");
  print_reencode_font(file, "Palatino-Bold");
  print_reencode_font(file, "Palatino-BoldItalic");
  print_reencode_font(file, "Symbol");
  print_reencode_font(file, "ZapfChancery-MediumItalic");
  print_reencode_font(file, "ZapfDingbats");
#endif /* !HAVE_UNICODE */
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
	  */
	  "%%%%EndProlog\n\n\n");
  
#ifdef HAVE_UNICODE
  renderer->psu = ps_unicoder_new(&eps_unicoder_callbacks,(gpointer)renderer);
#endif 
  return renderer;
}

static void
begin_render(RendererEPS *renderer, DiagramData *data)
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
  char *old_locale;

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

#ifdef HAVE_UNICODE

/* Note: we don't need to play with LC_NUMERIC locale settings in the 
   PSUnicoder callback functions ; the locale is set to "C" once, by 
   draw_string. */

static void 
eps_destroy_ps_font(gpointer usrdata, const gchar *fontname)
{
  RendererEPS *renderer = (RendererEPS *)usrdata;
  
  fprintf(renderer->file,"/%s undefinefont\n",fontname);
}

static void eps_build_ps_encoding(gpointer usrdata, 
                                  const gchar *name,
                                  const unicode_char_t table[PSEPAGE_SIZE])
{
  /* the table starts at PSEPAGE_BEGIN. Before that, we'll emit
     /xi (just as /notdef's) */
  RendererEPS *renderer = (RendererEPS *)usrdata;
  int i;
  
  fprintf(renderer->file, " [");
  for (i = 0; i<PSEPAGE_BEGIN; i++) {
    fprintf(renderer->file," /xi");
    if (!((i+1) % 16)) fprintf(renderer->file,"\n");
  }
  for (i=0; i<PSEPAGE_SIZE; i++) {
    fprintf(renderer->file," /%s",unicode_to_ps_name(table[i]));
    if (!((i+1) % 16)) fprintf(renderer->file,"\n");
  }
  fprintf(renderer->file,"] /%s exch def\n",name);
}

static void 
eps_build_ps_font(gpointer usrdata,
                  const gchar *name,
                  const gchar *face,
                  const gchar *encoding_name)
{
  RendererEPS *renderer = (RendererEPS *)usrdata;
  
  fprintf(renderer->file,
          "/%s\n"
          "  /%s findfont\n"
          "  dup length dict begin\n"
          "  {1 index /FID ne {def} {pop pop} ifelse} forall\n"
          "  /Encoding %s def\n"
          "  currentdict end\n"
          "definefont pop\n", name, face, encoding_name);
}

static void 
eps_select_ps_font(gpointer usrdata, const gchar *fontname, float size )
{
  RendererEPS *renderer = (RendererEPS *)usrdata;
  
  fprintf(renderer->file, "/%s ff %f scf sf\n", fontname, size);
}

static void eps_show_string(gpointer usrdata, const gchar *string)
{
  RendererEPS *renderer = (RendererEPS *)usrdata;
  /* string has nothing we would have to escape. */
  fprintf(renderer->file, "(%s)\n", string);  
}
                  
static void eps_get_string_width(gpointer usrdata, const gchar *string,
                                 gboolean first)
{
  RendererEPS *renderer = (RendererEPS *)usrdata;

  if (first) {
    fprintf(renderer->file, "(%s) sw\n", string);  
  } else {
    fprintf(renderer->file, "(%s) sw add\n", string);  
  }
}
                  

static void
set_font(RendererEPS *renderer, Font *font, real height)
{
  psu_set_font_face(renderer->psu,
                    font_get_psfontname(font), 
                    (float)height);
}

static void
draw_string(RendererEPS *renderer,
	    const char *text,
	    Point *pos, Alignment alignment,
	    Color *color)
{
  char *utf8_buffer;
  int utf8_len;

  if ((!text)||(text == (const char *)(1))) return;

  lazy_setcolor(renderer,color);

  /* <FIXME:> dia doesn't talk UTF-8 but the local charset (in fact, it 
     doesn't know what it talks, and assumes it's latin-1). The PSUnicoder
     talks UTF-8. We do a quick-and-dirty translation for now. */
  utf8_buffer = charconv_local8_to_utf8(text);
  /* </FIXME> */
  utf8_len = strlen(utf8_buffer); /* deliberate */

  if (utf8_len <= 0) {
    return; /* null string -> we won't display anything 
               and we will crash the Postscript stack. */
  }
  psu_check_string_encodings(renderer->psu,utf8_buffer);
  
  switch (alignment) {
  case ALIGN_LEFT:
    fprintf(renderer->file, "%f %f m ", pos->x, pos->y);
    break;
  case ALIGN_CENTER:
    psu_get_string_width(renderer->psu,utf8_buffer);
    if (!strlen(utf8_buffer)) {
      g_warning("null string.");
    }
    fprintf(renderer->file, "2 div %f ex sub %f m ",
	    pos->x, pos->y);
    break;
  case ALIGN_RIGHT:
    psu_get_string_width(renderer->psu,utf8_buffer);
    fprintf(renderer->file, "%f ex sub %f m ",
	    pos->x, pos->y);
    break;
  }

  psu_show_string(renderer->psu,utf8_buffer);
  
  g_free(utf8_buffer);
  
  if (utf8_len>0) {        /* always true, normally. */
    fprintf(renderer->file, " gs 1 -1 sc sh gr\n");
  }
}

#else /* !HAVE_UNICODE*/

static void
set_font(RendererEPS *renderer, Font *font, real height)
{
  fprintf(renderer->file, "/%s-latin1 ff %f scf sf\n",
	  font_get_psfontname(font), (double)height);
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

  lazy_setcolor(renderer,color);

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

#endif /* HAVE_UNICODE */

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
  return items+1;
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
      output[output_pos+1] = src[src_pos];
      output_pos += 2;
      src_pos += max_same;
    } else {
      /* Put out a block of non-replicated bytes */
      int max_non_repl = find_max_non_replicated(src+src_pos, srclen-src_pos, chan);
      int i;

      output[output_pos] = max_non_repl-1;
      for (i = 0; i < max_non_repl; i++) {
	output[output_pos+i+1] = src[src_pos+i*CHANNEL_SKIP(chan)+CHANNEL_ADD(chan)];
      }
      output_pos += max_non_repl+1;
      src_pos += max_non_repl;
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

  for (i = 0; i < blocks; i++) {
    if (i == blocks-1) {
      /* Special treatment for last block */
      long val;
      val = src[i*4]<<24;
      if (srclen%4 != 1) {
	val += src[i*4+1]<<16;
	if (srclen%4 != 2) {
	  val += src[i*4+2]<<8;
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
    } else {
      unsigned long val;
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
  }
  output[output_pos] = '~';
  output[output_pos+1] = '>';
  output[output_pos+2] = 0;
  return output;
}

#endif

static void
draw_image(RendererEPS *renderer,
	   Point *point,
	   real width, real height,
	   DiaImage image)
{
  int img_width, img_height;
  int v;
  int                 x, y;
  unsigned char      *ptr;
  real ratio;
  guint8 *rgb_data;
  guint8 *mask_data;

  img_width = dia_image_width(image);
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

#if RLE
    fprintf(renderer->file, "currentfile\n");
    fprintf(renderer->file, "/ASCII85Decode filter\n");
    fprintf(renderer->file, "0 /RunLengthDecode filter\n");
    fprintf(renderer->file, "currentfile\n");
    fprintf(renderer->file, "/ASCII85Decode filter\n");
    fprintf(renderer->file, "0 /RunLengthDecode filter\n");
    fprintf(renderer->file, "currentfile\n");
    fprintf(renderer->file, "/ASCII85Decode filter\n");
    fprintf(renderer->file, "0 /RunLengthDecode filter\n");
    fprintf(renderer->file, "true 3 colorimage\n");
    rle = RLE_encode(rgb_data, img_width*img_height*3, RED, &len);
    ascii = ASCII85_encode(rle, len);
    fprintf(renderer->file, "%s\n", ascii);
    g_free(rle);
    g_free(ascii);
    rle = RLE_encode(rgb_data, img_width*img_height*3, GREEN, &len);
    ascii = ASCII85_encode(rle, len);
    fprintf(renderer->file, "%s\n", ascii);
    g_free(rle);
    g_free(ascii);
    rle = RLE_encode(rgb_data, img_width*img_height*3, BLUE, &len);
    ascii = ASCII85_encode(rle, len);
    fprintf(renderer->file, "%s\n", ascii);
    g_free(rle);
    g_free(ascii);
#else
    fprintf(renderer->file, "{currentfile pix readhexstring pop}\n");
    fprintf(renderer->file, "false 3 colorimage\n");
    fprintf(renderer->file, "\n");
    
#define ALPHA_TO_WHITE
    ptr = rgb_data;
    for (y = 0; y < img_width; y++) {
      for (x = 0; x < img_height; x++) {
#ifdef ALPHA_TO_WHITE
	if (mask_data) {
	  fprintf(renderer->file, "%02x", 255-(mask_data[y*img_height+x]*(255-*ptr)/255));
	  ptr++;
	  fprintf(renderer->file, "%02x", 255-(mask_data[y*img_height+x]*(255-*ptr)/255));
	  ptr++;
	  fprintf(renderer->file, "%02x", 255-(mask_data[y*img_height+x]*(255-*ptr)/255));
	  ptr++;
	} else {
	  fprintf(renderer->file, "%02x", (int)(*ptr++));
	  fprintf(renderer->file, "%02x", (int)(*ptr++));
	  fprintf(renderer->file, "%02x", (int)(*ptr++));
	}
#else
	if (!mask_data || mask_data[y*img_height+x] > 127) {
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
