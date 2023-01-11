/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * wpg.c -- WordPerfect Graphics export plugin for dia
 * Copyright (C) 2000, 2014  Hans Breuer <Hans@Breuer.Org>
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

/*
 * ToDo:
 * - if helper points - like arc centers - are not in Dia's extent box
 *   their clipping produces unpredictable results
 * - the font setting needs improvement (maybe on Dia's side)
 */

#include "config.h"

#include <glib/gi18n-lib.h>

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <errno.h>

#include <glib.h>
#include <glib/gstdio.h>

#include "geometry.h"
#include "dia_image.h"
#include "diarenderer.h"
#include "filter.h"
#include "plug-ins.h"
#include "font.h"

/* Noise reduction for
 * return value of 'fwrite', declared with attribute warn_unused_result
 * discussion: http://gcc.gnu.org/bugzilla/show_bug.cgi?id=25509
 * root cause: http://sourceware.org/bugzilla/show_bug.cgi?id=11959
 */
#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wunused-result"
#endif

/* format specific */
#include "wpg_defs.h"

/* #define DEBUG_WPG */

/*
 * helper macros
 */
#define SC(a)  ((a) * renderer->Scale)
#define SCX(a) (((a) + renderer->XOffset) * renderer->Scale)
#define SCY(a) (((a) + renderer->YOffset) * renderer->Scale)

/* --- the renderer --- */
G_BEGIN_DECLS

#define WPG_TYPE_RENDERER           (wpg_renderer_get_type ())
#define WPG_RENDERER(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), WPG_TYPE_RENDERER, WpgRenderer))
#define WPG_RENDERER_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST ((klass), WPG_TYPE_RENDERER, WpgRendererClass))
#define WPG_IS_RENDERER(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), WPG_TYPE_RENDERER))
#define WPG_RENDERER_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), WPG_TYPE_RENDERER, WpgRendererClass))


enum {
  PROP_0,
  PROP_FONT,
  PROP_FONT_HEIGHT,
  LAST_PROP
};


GType wpg_renderer_get_type (void) G_GNUC_CONST;

typedef struct _WpgRenderer WpgRenderer;
typedef struct _WpgRendererClass WpgRendererClass;

struct _WpgRenderer
{
  DiaRenderer parent_instance;

  FILE *file;

  real Scale;   /* to WPU == 1/1200 inch */
  real XOffset, YOffset; /* in dia units */

  WPGStartData Box;
  WPGFillAttr  FillAttr;
  WPGLineAttr  LineAttr;
  WPGTextStyle TextStyle;


  DiaContext* ctx;

  DiaFont *font;
  double font_height;
};

struct _WpgRendererClass
{
  DiaRendererClass parent_class;
};

G_END_DECLS

#if (G_BYTE_ORDER == G_LITTLE_ENDIAN)
/* shortcut if testing of indirection isn't needed anymore */
#define fwrite_le(a,b,c,d) fwrite(a,b,c,d)
#else
static size_t
fwrite_le(void* buf, size_t size, size_t count, FILE* f)
{
  size_t n = 0;
  guint i;

  g_assert((1 == size) || (2 == size) || (4 == size));

  if (4 == size)
  {
    gint32 i32;

    for (i = 0; i < count; i++)
    {
      i32 = GINT32_TO_LE(((gint32*)buf)[i]);
      n += fwrite(&i32, sizeof(gint32), 1, f);
    }
  }
  else if (2 == size)
  {
    gint16 i16;

    for (i = 0; i < count; i++)
    {
      i16 = GINT16_TO_LE(((gint16*)buf)[i]);
      n += fwrite(&i16, sizeof(gint16), 1, f);
    }
  }
  else
    n = fwrite(buf, size, count, f);

  return n;
}
#endif

/*
 * color cube len, WPG is limited to 256 colors. To get simple
 * color reduction algorithms only 216 = 6^3 of them are used.
 */
#define CC_LEN 6

/*
 * "To avoid problems with WPC products the first 16 colors of
 *  the color map should never be changed".
 * Define WPG_NUM_DEF_COLORS to 0 ignore this statement (It
 * appears to work more reliable; tested with WinWord 97 and
 * Designer 4.1 - the latter does not respect the color maps
 * start color).
 */
#define WPG_NUM_DEF_COLORS 0 /* 16 */

/*
 * helper functions
 */
static guint8
LookupColor(WpgRenderer *renderer, Color* colour)
{
  /* find nearest color in the renderers color map */
  /* the following hack does only work with a correctly
     sorted color map */
  unsigned long i = (int)floor(colour->red * (CC_LEN - 1))
                  + (int)floor(colour->green * (CC_LEN - 1)) * CC_LEN
                  + (int)floor(colour->blue * (CC_LEN - 1)) * CC_LEN  * CC_LEN;
  DIAG_NOTE(g_message("LookupColor %d.", i));
  if (i >= CC_LEN * CC_LEN * CC_LEN)
    return CC_LEN * CC_LEN * CC_LEN - 1 + WPG_NUM_DEF_COLORS;
  else
    return i + WPG_NUM_DEF_COLORS;
}

static void
WriteRecHead(WpgRenderer *renderer, WPG_Type Type, guint32 Size)
{
  if (Size < 255)
  {
    WPGHead8 rh;
    rh.Type = (guint8)Type;
    rh.Size = (guint8)Size;
    fwrite(&rh, sizeof(guint8), 2, renderer->file);
  }
  else if (Size < 32768)
  {
    WPGHead16 rh;
    rh.Type  = (guint8)Type;
    rh.Dummy = 0xFF;
    rh.Size  = (guint16)Size;
    fwrite(&rh, sizeof(guint8), 2, renderer->file);
    fwrite_le(&(rh.Size), sizeof(guint16), 1, renderer->file);
  }
  else
  {
    WPGHead32 rh;
    rh.Type  = (guint8)Type;
    rh.Dummy = 0xFF;
    rh.Size  = Size;

    /* To avoid problems with structure packing this struct
     * is written in parts ...
     */
    fwrite(&rh, sizeof(guint8), 2, renderer->file);
    fwrite_le(&rh.Size, sizeof(guint32), 1, renderer->file);
  }
}

static void
WriteLineAttr(WpgRenderer *renderer, Color* colour)
{
  g_assert(4 == sizeof(WPGLineAttr));

  WriteRecHead(renderer, WPG_LINEATTR, sizeof(WPGLineAttr));
  renderer->LineAttr.Color = LookupColor(renderer, colour);
  fwrite(&renderer->LineAttr, sizeof(guint8), 2, renderer->file);
  fwrite_le(&renderer->LineAttr.Width, sizeof(guint16), 1, renderer->file);
}

static void
WriteFillAttr(WpgRenderer *renderer, Color* colour, gboolean bFill)
{
  g_assert(2 == sizeof(WPGFillAttr));

  WriteRecHead(renderer, WPG_FILLATTR, sizeof(WPGFillAttr));
  if (bFill)
  {
    int v = (int)(colour->alpha * 9);
    /*
     * Map transparency to fill style
     *   0 Hollow
     *   1 Solid
     *  11 Dots density 1 (least dense)
     *  16 Dots density 7 (densest)
     */
    renderer->FillAttr.Type = v > 8 ? 1 : v < 1 ? 0 : (10 + v);
    renderer->FillAttr.Color = LookupColor(renderer, colour);
    fwrite(&renderer->FillAttr, sizeof(WPGFillAttr), 1, renderer->file);
  }
  else
  {
    WPGFillAttr fa;
    fa.Color = LookupColor(renderer, colour);
    fa.Type = WPG_FA_HOLLOW;
    fwrite(&fa, sizeof(WPGFillAttr), 1, renderer->file);
  }
}

/*
 * render functions
 */
static void
begin_render(DiaRenderer *self, const DiaRectangle *update)
{
  WpgRenderer *renderer = WPG_RENDERER (self);
#if 0
  const WPGFileHead wpgFileHead = { "\377WPC", 16,
                                   1, 22,
                                   1, 0, /* Version */
                                   0, 0};
#else
  /* static conversion to little endian */
  const char wpgFileHead[16] = {255, 'W', 'P', 'C', 16, 0, 0, 0,
                                1, 22, 1, 0, 0, 0, 0, 0};
#endif

  gint16 i;
  guint8* pPal;
  Color color = { 1.0, 1.0, 1.0, 1.0 };

  DIAG_NOTE(g_message("begin_render"));

  fwrite(&wpgFileHead, 1, 16, renderer->file);

  /* bounding box */
  WriteRecHead(renderer, WPG_START, sizeof(WPGStartData));
#if 0
  fwrite(&renderer->Box, sizeof(WPGStartData), 1, renderer->file);
#else
  g_assert(6 == sizeof(WPGStartData));
  fwrite(&renderer->Box, sizeof(guint8), 2, renderer->file);
  fwrite_le(&renderer->Box.Width, sizeof(guint16), 2, renderer->file);
#endif
  /* initialize a well known colormap, see LookupColor */
  pPal = g_new(guint8, CC_LEN*CC_LEN*CC_LEN*3);
  for (i = 0; i < CC_LEN * CC_LEN * CC_LEN; i++)
  {
    pPal[3*i  ] = ((i % CC_LEN) * 255) / (CC_LEN - 1);   /* red */
    pPal[3*i+1] = (((i / CC_LEN) % CC_LEN) * 255) / (CC_LEN - 1); /* green */
    pPal[3*i+2] = ((i / (CC_LEN * CC_LEN)) * 255) / (CC_LEN - 1); /* blue varies least */
    /*
    g_print("%d\t%d\t%d\n", pPal[3*i  ], pPal[3*i+1], pPal[3*i+2]);
     */
  }

  WriteRecHead(renderer, WPG_COLORMAP, CC_LEN*CC_LEN*CC_LEN*3 + 2*sizeof(guint16));
  i = WPG_NUM_DEF_COLORS; /* start color */
  fwrite_le(&i, sizeof(gint16), 1, renderer->file);
  i = CC_LEN*CC_LEN*CC_LEN;
  fwrite_le(&i, sizeof(gint16), 1, renderer->file);
  fwrite(pPal, 1, CC_LEN*CC_LEN*CC_LEN*3, renderer->file);

  /* FIXME: following 3 lines needed to make filling work !? */
  renderer->FillAttr.Type = WPG_FA_SOLID;
  WriteFillAttr(renderer, &color, TRUE);
  WriteFillAttr(renderer, &color, FALSE);

  g_clear_pointer (&pPal, g_free);
}

static void
end_render(DiaRenderer *self)
{
  WpgRenderer *renderer = WPG_RENDERER (self);

  DIAG_NOTE(g_message( "end_render"));

  WriteRecHead(renderer, WPG_END, 0); /* no data following */
  fclose(renderer->file);
  renderer->file = NULL;
}

static void
set_linewidth(DiaRenderer *self, real linewidth)
{
  WpgRenderer *renderer = WPG_RENDERER (self);

  DIAG_NOTE(g_message("set_linewidth %f", linewidth));

  renderer->LineAttr.Width = SC(linewidth);
}


static void
set_linecaps (DiaRenderer *self, DiaLineCaps mode)
{
  DIAG_NOTE (g_message("set_linecaps %d", mode));

  switch (mode) {
    case DIA_LINE_CAPS_DEFAULT:
    case DIA_LINE_CAPS_BUTT:
      break;
    case DIA_LINE_CAPS_ROUND:
      break;
    case DIA_LINE_CAPS_PROJECTING:
      break;
    default:
      g_warning("WpgRenderer : Unsupported line-caps mode specified!\n");
  }
}


static void
set_linejoin (DiaRenderer *self, DiaLineJoin mode)
{
  DIAG_NOTE (g_message ("set_join %d", mode));

  switch (mode) {
    case DIA_LINE_JOIN_DEFAULT:
    case DIA_LINE_JOIN_MITER:
      break;
    case DIA_LINE_JOIN_ROUND:
      break;
    case DIA_LINE_JOIN_BEVEL:
      break;
    default:
      g_warning ("WpgRenderer : Unsupported line-join mode specified!\n");
  }
}


static void
set_linestyle (DiaRenderer *self, DiaLineStyle mode, double dash_length)
{
  WpgRenderer *renderer = WPG_RENDERER (self);

  DIAG_NOTE (g_message ("set_linestyle %d, %g", mode, dash_length));

  /* line type */
  switch (mode) {
    case DIA_LINE_STYLE_DEFAULT:
    case DIA_LINE_STYLE_SOLID:
      renderer->LineAttr.Type = WPG_LA_SOLID;
      break;
    case DIA_LINE_STYLE_DASHED:
      if (dash_length < 0.5)
        renderer->LineAttr.Type = WPG_LA_SHORTDASH;
      else
        renderer->LineAttr.Type = WPG_LA_MEDIUMDASH;
      break;
    case DIA_LINE_STYLE_DASH_DOT:
      renderer->LineAttr.Type = WPG_LA_DASHDOT;
      break;
    case DIA_LINE_STYLE_DASH_DOT_DOT:
      renderer->LineAttr.Type = WPG_LA_DASHDOTDOT;
      break;
    case DIA_LINE_STYLE_DOTTED:
      renderer->LineAttr.Type = WPG_LA_DOTS;
      break;
    default:
      g_warning ("WpgRenderer : Unsupported fill mode specified!\n");
  }
}


static void
set_fillstyle (DiaRenderer *self, DiaFillStyle mode)
{
  WpgRenderer *renderer = WPG_RENDERER (self);

  DIAG_NOTE (g_message ("set_fillstyle %d", mode));

  switch(mode) {
    case DIA_FILL_STYLE_SOLID:
      renderer->FillAttr.Type = WPG_FA_SOLID;
      break;
    default:
      g_warning ("WpgRenderer : Unsupported fill mode specified!\n");
  }
}


static void
wpg_renderer_set_font (DiaRenderer *self, DiaFont *font, double height)
{
  WpgRenderer *renderer = WPG_RENDERER (self);

  /* FIXME PANGO: this is a little broken. Use better matching. */

  const char *family_name;
  DIAG_NOTE(g_message("set_font %f %s", height, font->name));
  renderer->TextStyle.Height = SC (height);

  g_print ("f: %p h: %f\n", font, height);

  g_set_object (&renderer->font, font);

  family_name = dia_font_get_family (font);

  if ((strstr (family_name, "courier")) || (strstr (family_name, "monospace"))) {
    renderer->TextStyle.Font = 0x0DF0;
  } else if ((strstr (family_name, "times")) || (strstr (family_name, "serif"))) {
    renderer->TextStyle.Font = 0x1950;
  } else {
    renderer->TextStyle.Font = 0x1150; /* Helv */
  }
}


/* Need to translate coord system:
 *
 *   Dia x,y -> Wpg x,-y
 *
 * doing it before scaling.
 */
static void
draw_line(DiaRenderer *self,
          Point *start, Point *end,
          Color *line_colour)
{
  WpgRenderer *renderer = WPG_RENDERER (self);
  gint16 pData[4];

  DIAG_NOTE(g_message("draw_line %f,%f -> %f, %f",
            start->x, start->y, end->x, end->y));

  WriteLineAttr(renderer, line_colour);
  WriteRecHead(renderer, WPG_LINE, 4 * sizeof(gint16));

  /* point data */
  pData[0] = SCX(start->x);
  pData[1] = SCY(-start->y);
  pData[2] = SCX(end->x);
  pData[3] = SCY(-end->y);

  fwrite_le(pData, sizeof(gint16), 4, renderer->file);
}

static void
draw_polyline(DiaRenderer *self,
              Point *points, int num_points,
              Color *line_colour)
{
  WpgRenderer *renderer = WPG_RENDERER (self);
  int i;
  gint16* pData;

  DIAG_NOTE(g_message("draw_polyline n:%d %f,%f ...",
            num_points, points->x, points->y));

  g_return_if_fail(1 < num_points);

  WriteLineAttr(renderer, line_colour);
  WriteRecHead(renderer, WPG_POLYLINE, num_points * 2 * sizeof(gint16) + sizeof(gint16));

  pData = g_new(gint16, num_points * 2);

  /* number of points */
  pData[0] = num_points;
  fwrite_le(pData, sizeof(gint16), 1, renderer->file);

  /* point data */
  for (i = 0; i < num_points; i++)
  {
    pData[2*i]   = SCX(points[i].x);
    pData[2*i+1] = SCY(-points[i].y);
  }

  fwrite_le(pData, sizeof(gint16), num_points*2, renderer->file);

  g_clear_pointer (&pData, g_free);
}

static void
draw_polygon(DiaRenderer *self,
             Point *points, int num_points,
             Color *fill, Color *stroke)
{
  WpgRenderer *renderer = WPG_RENDERER (self);
  gint16* pData;
  int i;
  WPG_LineAttr lt = renderer->LineAttr.Type;

  DIAG_NOTE(g_message("draw_polygon n:%d %f,%f ...",
            num_points, points->x, points->y));

  if (!stroke)
    renderer->LineAttr.Type = WPG_LA_NONE;
  WriteLineAttr(renderer, stroke ? stroke : fill);
  if (fill)
    WriteFillAttr(renderer, fill, TRUE);
  else
    WriteFillAttr(renderer, stroke, FALSE);
  WriteRecHead(renderer, WPG_POLYGON, (num_points * 2 + 1) * sizeof(gint16));

  pData = g_new(gint16, num_points * 2);

  /* number of vertices */
  pData[0] = num_points;
  fwrite_le(pData, sizeof(gint16), 1, renderer->file);

  /* point data */
  for (i = 0; i < num_points; i++)
  {
    pData[2*i]   = SCX(points[i].x);
    pData[2*i+1] = SCY(-points[i].y);
  }

  fwrite_le(pData, sizeof(gint16), num_points*2, renderer->file);
  /* restore state */
  if (!stroke)
    renderer->LineAttr.Type = lt;
  /* switch off fill */
  WriteFillAttr(renderer, fill ? fill : stroke, FALSE);

  g_clear_pointer (&pData, g_free);
}

static void
draw_rect(DiaRenderer *self,
          Point *ul_corner, Point *lr_corner,
          Color *fill, Color *stroke)
{
  WpgRenderer *renderer = WPG_RENDERER (self);
  gint16* pData;
  WPG_LineAttr lt = renderer->LineAttr.Type;

  DIAG_NOTE(g_message("draw_rect %f,%f -> %f,%f",
            ul_corner->x, ul_corner->y, lr_corner->x, lr_corner->y));

  g_return_if_fail (fill || stroke);

  if (!stroke)
    renderer->LineAttr.Type = WPG_LA_NONE;
  WriteLineAttr(renderer, stroke ? stroke : fill);
  if (fill)
    WriteFillAttr(renderer, fill, TRUE);
  else
    WriteFillAttr(renderer, stroke, FALSE);
  WriteRecHead(renderer, WPG_RECTANGLE, 4*sizeof(gint16));

  pData = g_new(gint16, 4);
  pData[0] = SCX(ul_corner->x); /* lower left corner ! */
  pData[1] = SCY(-lr_corner->y);
  pData[2] = SC(lr_corner->x - ul_corner->x); /* width */
  pData[3] = SC(lr_corner->y - ul_corner->y); /* height */

  fwrite_le(pData, sizeof(gint16), 4, renderer->file);
  if (!stroke)
    renderer->LineAttr.Type = lt;
  /* switch off fill */
  WriteFillAttr(renderer, fill ? fill : stroke, FALSE);
  g_clear_pointer (&pData, g_free);
}

static void
draw_arc(DiaRenderer *self,
	 Point *center,
	 real width, real height,
	 real angle1, real angle2,
	 Color *colour)
{
  WpgRenderer *renderer = WPG_RENDERER (self);
  WPGEllipse ell;
  gboolean counter_clockwise = angle2 > angle1;

  DIAG_NOTE(g_message("draw_arc %fx%f <%f,<%f",
            width, height, angle1, angle2));

  ell.x = SCX(center->x);
  ell.y = SCY(-center->y);
  ell.RotAngle = 0;
  ell.rx = SC(width / 2.0);
  ell.ry = SC(height / 2.0);

  /* ensure range of 0..360 */
  while (angle1 < 0.0) angle1 += 360.0;
  while (angle2 < 0.0) angle2 += 360.0;
  /* make it counter-clockwise */
  if (counter_clockwise) {
    ell.StartAngle = angle1;
    ell.EndAngle   = angle2;
  } else {
    ell.StartAngle = angle2;
    ell.EndAngle   = angle1;
  }
  ell.Flags = 0;

  WriteLineAttr(renderer, colour);
  WriteRecHead(renderer, WPG_ELLIPSE, sizeof(WPGEllipse));

  g_assert(16 == sizeof(WPGEllipse));
  fwrite_le(&ell, sizeof(guint16), sizeof(WPGEllipse) / sizeof(guint16), renderer->file);
}

static void
fill_arc(DiaRenderer *self,
         Point *center,
         real width, real height,
         real angle1, real angle2,
         Color *colour)
{
  WpgRenderer *renderer = WPG_RENDERER (self);
  WPGEllipse ell;
  gboolean counter_clockwise = angle2 > angle1;

  DIAG_NOTE(g_message("fill_arc %fx%f <%f,<%f",
            width, height, angle1, angle2));

  ell.x = SCX(center->x);
  ell.y = SCY(-center->y);
  ell.RotAngle = 0;
  ell.rx = SC(width / 2.0);
  ell.ry = SC(height / 2.0);

  /* ensure range of 0..360 */
  while (angle1 < 0.0) angle1 += 360.0;
  while (angle2 < 0.0) angle2 += 360.0;
  /* make it counter-clockwise */
  if (counter_clockwise) {
    ell.StartAngle = angle1;
    ell.EndAngle   = angle2;
  } else {
    ell.StartAngle = angle2;
    ell.EndAngle   = angle1;
  }
  ell.Flags = 0; /* 0: connect to center; 1: connect start and end */

  WriteFillAttr(renderer, colour, TRUE);
  WriteRecHead(renderer, WPG_ELLIPSE, sizeof(WPGEllipse));

  g_assert(16 == sizeof(WPGEllipse));
  fwrite_le(&ell, sizeof(guint16), sizeof(WPGEllipse) / sizeof(guint16), renderer->file);
  WriteFillAttr(renderer, colour, FALSE);
}

static void
draw_ellipse(DiaRenderer *self,
             Point *center,
             real width, real height,
             Color *fill, Color *stroke)
{
  WpgRenderer *renderer = WPG_RENDERER (self);
  WPGEllipse ell;

  DIAG_NOTE(g_message("draw_ellipse %fx%f center @ %f,%f",
            width, height, center->x, center->y));

  ell.x = SCX(center->x);
  ell.y = SCY(-center->y);
  ell.RotAngle = 0;
  ell.rx = SC(width / 2.0);
  ell.ry = SC(height / 2.0);

  ell.StartAngle = 0;
  ell.EndAngle   = 360;
  ell.Flags = 0;

  if (stroke)
    WriteLineAttr(renderer, stroke);
  if (fill)
    WriteFillAttr(renderer, fill, TRUE);
  WriteRecHead(renderer, WPG_ELLIPSE, sizeof(WPGEllipse));

  g_assert(16 == sizeof(WPGEllipse));
  fwrite_le(&ell, sizeof(guint16), sizeof(WPGEllipse) / sizeof(guint16), renderer->file);
  if (fill)
    WriteFillAttr(renderer, fill, FALSE);
}

static void
draw_bezier(DiaRenderer *self,
            BezPoint *points,
            int numpoints,
            Color *colour)
{
  WpgRenderer *renderer = WPG_RENDERER (self);
  WPGPoint* pts;
  guint16 data[2];
  int i;

  DIAG_NOTE(g_message("draw_bezier n:%d %fx%f ...",
            numpoints, points->p1.x, points->p1.y));

  WriteLineAttr(renderer, colour);
  WriteRecHead(renderer, WPG_POLYCURVE, (3 * numpoints - 2) * sizeof(WPGPoint) + 3 * sizeof(guint16));

  pts = g_new(WPGPoint, 3 * numpoints - 2);

  /* ??? size of equivalent data in pre5.1 files ? DWORD */
  memset (data, 0, sizeof(guint16) * 2);
  fwrite_le(data, sizeof(guint16), 2, renderer->file);
  /* num points */
  data[0] = 3 * numpoints - 2;
  fwrite_le(data, sizeof(guint16), 1, renderer->file);

  /* WPG's Poly Curve is a cubic bezier compatible with Dia's bezier.
   * http://www.fileformat.info/format/wpg/egff.htm
   * could lead to the assumption of only quadratic bezier support,
   * but that's not the case.
   */
  pts[0].x = SCX( points[0].p1.x);
  pts[0].y = SCY(-points[0].p1.y);
  for (i = 1; i < numpoints; i++) {
    switch (points[i].type) {
      case BEZ_MOVE_TO:
      case BEZ_LINE_TO:
        /* just the point */
        pts[i*3-2].x = pts[i*3-1].x = pts[i*3].x = SCX( points[i].p1.x);
        pts[i*3-2].y = pts[i*3-1].y = pts[i*3].y = SCY(-points[i].p1.y);
        break;
      case BEZ_CURVE_TO:
        /* first control point */
        pts[i*3-2].x = SCX( points[i].p1.x);
        pts[i*3-2].y = SCY(-points[i].p1.y);
        /* second control point */
        pts[i*3-1].x = SCX( points[i].p2.x);
        pts[i*3-1].y = SCY(-points[i].p2.y);
        /* this segments end point */
        pts[i*3].x = SCX( points[i].p3.x);
        pts[i*3].y = SCY(-points[i].p3.y);
        break;
      default:
        g_warning ("Unknown type %i", points[i].type);
        break;
    }
  }

  fwrite_le (pts, sizeof (gint16), 2 * (numpoints*3-2), renderer->file);
  g_clear_pointer (&pts, g_free);
}

static gpointer parent_class = NULL;

static void
draw_beziergon (DiaRenderer *self,
		BezPoint *points,
		int numpoints,
		Color *fill,
		Color *stroke)
{
  DIAG_NOTE(g_message("draw_beziezgon %d points", numpoints));

#if 1
  /* Use fallback from base class until another program can
   * actually correctly show the filled Polycurve created
   * by Dia (our own import can).
   */
  if (fill)
    DIA_RENDERER_CLASS(parent_class)->draw_beziergon (self, points, numpoints, fill, NULL);
#else
  if (fill) {
    WpgRenderer *renderer = WPG_RENDERER (self);

    WriteFillAttr(renderer, fill, TRUE);
    draw_bezier (self, points, numpoints, fill);
    WriteFillAttr(renderer, fill, FALSE);
  }
#endif
  if (stroke) /* XXX: still not closing the path */
    draw_bezier (self, points, numpoints, stroke);
}


static void
draw_string (DiaRenderer  *self,
             const char   *text,
             Point        *pos,
             DiaAlignment  alignment,
             Color        *colour)
{
  WpgRenderer *renderer = WPG_RENDERER (self);
  gint16 len;
  WPGPoint pt;

  len = strlen(text);

  DIAG_NOTE(g_message("draw_string(%d) %f,%f %s",
            len, pos->x, pos->y, text));

  if (len < 1) return; /* shouldn't this be handled by Dia's core ? */

  renderer->TextStyle.YAlign = 3; /* bottom ??? */

  switch (alignment) {
    case DIA_ALIGN_LEFT:
      renderer->TextStyle.XAlign = 0;
      break;
    case DIA_ALIGN_CENTRE:
      renderer->TextStyle.XAlign = 1;
      break;
    case DIA_ALIGN_RIGHT:
      renderer->TextStyle.XAlign = 2;
      break;
    default:
      g_warning ("Unknown alignment %i", alignment);
      break;
  }

  renderer->TextStyle.Color = LookupColor(renderer, colour);
  renderer->TextStyle.Angle = 0;

  renderer->TextStyle.Width = renderer->TextStyle.Height * 0.6; /* ??? */

  WriteRecHead(renderer, WPG_TEXTSTYLE, sizeof(WPGTextStyle));

#if 0
  fwrite(&renderer->TextStyle, sizeof(WPGTextStyle), 1, renderer->file);
#else
  g_assert(22 == sizeof(WPGTextStyle));
  fwrite_le(&renderer->TextStyle.Width, sizeof(guint16), 1, renderer->file);
  fwrite_le(&renderer->TextStyle.Height, sizeof(guint16), 1, renderer->file);
  fwrite(&renderer->TextStyle.Reserved, 1, sizeof(renderer->TextStyle.Reserved), renderer->file);
  fwrite_le(&renderer->TextStyle.Font, sizeof(guint16), 1, renderer->file);
  fwrite(&renderer->TextStyle.Reserved2, sizeof(guint8), 1, renderer->file);
  fwrite(&renderer->TextStyle.XAlign, 1, 1, renderer->file);
  fwrite(&renderer->TextStyle.YAlign, 1, 1, renderer->file);
  fwrite(&renderer->TextStyle.Color, 1, 1, renderer->file);
  fwrite_le(&renderer->TextStyle.Angle, sizeof(guint16), 1, renderer->file);
#endif
  pt.x = SCX(pos->x);
  pt.y = SCY(-pos->y);

  WriteRecHead(renderer, WPG_TEXT, 3*sizeof(gint16) + len);
  fwrite_le(&len, sizeof(gint16), 1, renderer->file);
  fwrite_le(&pt.x,  sizeof(guint16), 1, renderer->file);
  fwrite_le(&pt.y,  sizeof(guint16), 1, renderer->file);
  fwrite(text, 1, len, renderer->file);
}

static void
draw_image(DiaRenderer *self,
           Point *point,
           real width, real height,
           DiaImage *image)
{
  WpgRenderer *renderer = WPG_RENDERER (self);
  WPGBitmap2 bmp;
  guint8 * pDiaImg = NULL, * pOut = NULL, * pIn = NULL, * p = NULL;
  guint8 b_1 = 0, b = 0, cnt;
  int x, y, stride;

  bmp.Angle  = 0;
  bmp.Left   = SCX(point->x);
  bmp.Top    = SCY(-point->y);
  bmp.Right  = SCX(point->x + width);
  bmp.Bottom = SCY(-point->y - height);

  bmp.Width  = dia_image_width(image);
  bmp.Height = dia_image_height(image);
  bmp.Depth  = 8; /* maximum allowed */

  bmp.Xdpi = 72; /* ??? */
  bmp.Ydpi = 72;

  DIAG_NOTE(g_message("draw_image %fx%f [%d,%d] @%f,%f",
            width, height, bmp.Width, bmp.Height, point->x, point->y));

  pDiaImg = dia_image_rgb_data(image);
  if (!pDiaImg) {
    dia_context_add_message(renderer->ctx, _("Not enough memory for image drawing."));
    return;
  }
  stride = dia_image_rowstride(image);
  pOut = g_new(guint8, bmp.Width * bmp.Height * 2); /* space for worst case RLE */
  p = pOut;

  for (y = 0; y < bmp.Height; y++)
  {
    /* from top to bottom line */
    pIn = pDiaImg + stride * y;
    cnt = 0; /* reset with every line */
    for (x = 0; x < bmp.Width; x ++)
    {
      /* simple color reduction to default palette */
      b = (((CC_LEN - 1) * pIn[0]) / 255) /* red */
        + (((CC_LEN - 1) * pIn[1]) / 255) * CC_LEN /* green */
        + (((CC_LEN - 1) * pIn[2]) / 255) * CC_LEN * CC_LEN /* blue */
        + WPG_NUM_DEF_COLORS;

      pIn += 3; /* increase read pointer */

      if (cnt > 0)
      {
        if ((b == b_1) && (cnt < 127))
          cnt++; /* increase counter*/
        else
        {
          *p++ = (cnt | 0x80); /* write run length and value */
          *p++ = b_1;
          cnt = 1; /* reset */
          b_1 = b;
        }
      }
      else
      {
        b_1 = b;
        cnt = 1;
      }
    }
    *p++ = (cnt | 0x80); /* write last value(s) */
    *p++ = b;
  }
  DIAG_NOTE(g_message( "Width x Height: %d RLE: %d",
	      bmp.Width * bmp.Height, p - pOut));

  if ((p - pOut) > 32767) {
    dia_context_add_message(renderer->ctx, "Bitmap size exceeds blocksize. Ignored.");
  }
  else {
    WriteRecHead(renderer, WPG_BITMAP2, sizeof(WPGBitmap2) + (p - pOut));
#if 0
    fwrite(&bmp, sizeof(WPGBitmap2), 1, renderer->file);
#else
    g_assert(20 == sizeof(WPGBitmap2));
    fwrite(&bmp, sizeof(guint16), sizeof(WPGBitmap2) / sizeof(guint16), renderer->file);
#endif
    fwrite(pOut, sizeof(guint8), p - pOut, renderer->file);
  }
#if 0
  /* RLE diagnose */
  {
    FILE* f;
    int i, j;
    f = g_fopen("c:\\temp\\wpg_bmp.raw", "wb");
    j = p - pOut;
    for (i = 0; i < j; i+=2)
    {
      for (x = 0; x < (pOut[i] & 0x7F); x++)
        fwrite(&pOut[i+1], 1, 1, f);
    }
    fclose(f);
  }
#endif
  g_clear_pointer (&pDiaImg, g_free);
  g_clear_pointer (&pOut, g_free);
}

/* gobject boiler plate */
static void wpg_renderer_class_init (WpgRendererClass *klass);

GType
wpg_renderer_get_type (void)
{
  static GType object_type = 0;

  if (!object_type)
    {
      static const GTypeInfo object_info =
      {
        sizeof (WpgRendererClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) wpg_renderer_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (WpgRenderer),
        0,              /* n_preallocs */
	NULL            /* init */
      };

      object_type = g_type_register_static (DIA_TYPE_RENDERER,
                                            "WpgRenderer",
                                            &object_info, 0);
    }

  return object_type;
}


static void
wpg_renderer_set_property (GObject      *object,
                           guint         property_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
  WpgRenderer *self = WPG_RENDERER (object);

  switch (property_id) {
    case PROP_FONT:
      wpg_renderer_set_font (DIA_RENDERER (self),
                             DIA_FONT (g_value_get_object (value)),
                             self->font_height);
      break;
    case PROP_FONT_HEIGHT:
      wpg_renderer_set_font (DIA_RENDERER (self),
                             self->font,
                             g_value_get_double (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}


static void
wpg_renderer_get_property (GObject    *object,
                           guint       property_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
  WpgRenderer *self = WPG_RENDERER (object);

  switch (property_id) {
    case PROP_FONT:
      g_value_set_object (value, self->font);
      break;
    case PROP_FONT_HEIGHT:
      g_value_set_double (value, self->font_height);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
wpg_renderer_finalize (GObject *object)
{
  WpgRenderer *wpg_renderer = WPG_RENDERER (object);

  g_clear_object (&wpg_renderer->font);

  if (wpg_renderer->file)
    fclose(wpg_renderer->file);
  wpg_renderer->file = NULL;

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
wpg_renderer_class_init (WpgRendererClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  DiaRendererClass *renderer_class = DIA_RENDERER_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->set_property = wpg_renderer_set_property;
  object_class->get_property = wpg_renderer_get_property;
  object_class->finalize = wpg_renderer_finalize;

  /* renderer members */
  renderer_class->begin_render = begin_render;
  renderer_class->end_render   = end_render;

  renderer_class->set_linewidth  = set_linewidth;
  renderer_class->set_linecaps   = set_linecaps;
  renderer_class->set_linejoin   = set_linejoin;
  renderer_class->set_linestyle  = set_linestyle;
  renderer_class->set_fillstyle  = set_fillstyle;

  renderer_class->draw_line    = draw_line;
  renderer_class->draw_polygon = draw_polygon;
  renderer_class->draw_arc     = draw_arc;
  renderer_class->fill_arc     = fill_arc;
  renderer_class->draw_ellipse = draw_ellipse;

  renderer_class->draw_string  = draw_string;
  renderer_class->draw_image   = draw_image;

  /* medium level functions */
  renderer_class->draw_rect      = draw_rect;
  renderer_class->draw_polyline  = draw_polyline;

  renderer_class->draw_bezier   = draw_bezier;
  renderer_class->draw_beziergon = draw_beziergon;

  g_object_class_override_property (object_class, PROP_FONT, "font");
  g_object_class_override_property (object_class, PROP_FONT_HEIGHT, "font-height");
}

/* dia export funtion */
static gboolean
export_data(DiagramData *data, DiaContext *ctx,
	    const gchar *filename, const gchar *diafilename,
	    void* user_data)
{
  WpgRenderer *renderer;
  FILE *file;
  DiaRectangle *extent;
  real width, height;

  file = g_fopen(filename, "wb"); /* "wb" for binary! */

  if (file == NULL) {
    dia_context_add_message_with_errno (ctx, errno, _("Can't open output file %s"),
					dia_context_get_filename(ctx));
    return FALSE;
  }

  renderer = g_object_new (WPG_TYPE_RENDERER, NULL);

  renderer->file = file;
  renderer->ctx = ctx;

  extent = &data->extents;

  /* use extents */
  DIAG_NOTE(g_message("export_data extents %f,%f -> %f,%f",
            extent->left, extent->top, extent->right, extent->bottom));

  width  = extent->right - extent->left;
  height = extent->bottom - extent->top;
#if 0
  /* extend to use full range */
  renderer->Scale = 0.001;
  if (width > height)
    while (renderer->Scale * width < 3276.7) renderer->Scale *= 10.0;
  else
    while (renderer->Scale * height < 3276.7) renderer->Scale *= 10.0;
#else
  /* scale from Dia's cm to WPU (1/1200 inch) */
  renderer->Scale = WPU_PER_DCM;
  /* avoid int16 overflow */
  if (width > height)
    while (renderer->Scale * width > 32767) renderer->Scale /= 10.0;
  else
    while (renderer->Scale * height > 32767) renderer->Scale /= 10.0;
  renderer->XOffset = - extent->left;
  renderer->YOffset =   extent->bottom;
#endif
  renderer->Box.Width  = width * renderer->Scale;
  renderer->Box.Height = height * renderer->Scale;
  renderer->Box.Flag   = 0;
  renderer->Box.Version = 0;

  data_render(data, DIA_RENDERER(renderer), NULL, NULL, NULL);

  g_clear_object (&renderer);

  return TRUE;
}



static const gchar *extensions[] = { "wpg", NULL };
static DiaExportFilter my_export_filter = {
    N_("WordPerfect Graphics"),
    extensions,
    export_data
};

DiaImportFilter my_import_filter = {
    N_("WordPerfect Graphics"),
    extensions,
    import_data
};

/* --- dia plug-in interface --- */
static gboolean
_plugin_can_unload (PluginInfo *info)
{
  return TRUE;
}

static void
_plugin_unload (PluginInfo *info)
{
  filter_unregister_export(&my_export_filter);
  filter_unregister_import(&my_import_filter);
}

DIA_PLUGIN_CHECK_INIT

PluginInitResult
dia_plugin_init(PluginInfo *info)
{
  if (!dia_plugin_info_init(info, "WPG",
                            _("WordPerfect Graphics export filter"),
                            _plugin_can_unload,
                            _plugin_unload))
    return DIA_PLUGIN_INIT_ERROR;

  filter_register_export(&my_export_filter);
  filter_register_import(&my_import_filter);

  return DIA_PLUGIN_INIT_OK;
}
