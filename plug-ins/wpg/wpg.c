/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * wpg.c -- HPGL export plugin for dia
 * Copyright (C) 2000, Hans Breuer, <Hans@Breuer.Org>
 *   based on dummy.c 
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
 * - bezier curves are not correctly converted to WPG's poly curve
 */

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <glib.h>

#include "config.h"
#include "intl.h"
#include "message.h"
#include "geometry.h"
#include "render.h"
#include "filter.h"
#include "plug-ins.h"

/* format specific */
#include "wpg_defs.h"

/*
 * helper macros
 */
#define SC(a) ((a) * renderer->Scale)

/* --- the renderer --- */
#define MY_RENDERER_NAME "WPG"

typedef struct _MyRenderer MyRenderer;
struct _MyRenderer {
  Renderer renderer;

  FILE *file;

  real Scale;

  real dash_length;

  WPGStartData Box;
  WPGFillAttr  FillAttr;
  WPGLineAttr  LineAttr;
  WPGTextStyle TextStyle;
};

/* include function declares and render object "vtable" */
#include "../renderer.inc"

#define DIAG_NOTE /* my_log */
void
my_log(MyRenderer* renderer, char* format, ...)
{
    gchar *string;
    va_list args;
  
    g_return_if_fail (format != NULL);
  
    va_start (args, format);
    string = g_strdup_vprintf (format, args);
    va_end (args);

    g_print(string);

    g_free(string);
}

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
guint8
LookupColor(MyRenderer *renderer, Color* colour)
{
  /* find nearest color in the renderers color map */
  /* the following hack does only work with a correctly 
     sorted color map */
  unsigned long i = (int)floor(colour->red * (CC_LEN - 1))
                  + (int)floor(colour->green * (CC_LEN - 1)) * CC_LEN
                  + (int)floor(colour->blue * (CC_LEN - 1)) * CC_LEN  * CC_LEN;
  DIAG_NOTE(renderer, "LookupColor %d.\n", i);
  if (i >= CC_LEN * CC_LEN * CC_LEN)
    return CC_LEN * CC_LEN * CC_LEN - 1 + WPG_NUM_DEF_COLORS;
  else 
    return i + WPG_NUM_DEF_COLORS;
}

void
WriteRecHead(MyRenderer *renderer, WPG_Type Type, guint32 Size)
{
  if (Size < 255)
  {
    WPGHead8 rh;
    rh.Type = (guint8)Type;
    rh.Size = (guint8)Size;
    fwrite(&rh, sizeof(rh), 1, renderer->file);
  }
  else if (Size < 32768)
  {
    WPGHead16 rh;
    rh.Type  = (guint8)Type;
    rh.Dummy = 0xFF;
    rh.Size  = (guint16)Size;
    fwrite(&rh, sizeof(rh), 1, renderer->file);
  }
  else
  {
    WPGHead32 rh;
    rh.Type  = (guint8)Type;
    rh.Dummy = 0xFF;
    rh.Size  = Size;
    fwrite(&rh, sizeof(rh), 1, renderer->file);
  }
}

void
WriteLineAttr(MyRenderer *renderer, Color* colour)
{
  WriteRecHead(renderer, WPG_LINEATTR, sizeof(WPGLineAttr));
  renderer->LineAttr.Color = LookupColor(renderer, colour);
  fwrite(&renderer->LineAttr, sizeof(WPGLineAttr), 1, renderer->file);
}

void
WriteFillAttr(MyRenderer *renderer, Color* colour, gboolean bFill)
{
  WriteRecHead(renderer, WPG_FILLATTR, sizeof(WPGFillAttr));
  if (bFill)
  {
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
begin_render(MyRenderer *renderer, DiagramData *data)
{
  const char WPGHead[16] = {255, 'W', 'P', 'C', 16, 0, 0, 0, 
                              1, 22, 1, 0, 0, 0, 0, 0};
  gint16 i;
  guint8* pPal;
  Color color = {1, 1, 1};

  DIAG_NOTE(renderer, "begin_render\n");

  fwrite(WPGHead, 1, sizeof(WPGHead), renderer->file);

  /* bounding box */
  WriteRecHead(renderer, WPG_START, sizeof(WPGStartData));
  fwrite(&renderer->Box, sizeof(WPGStartData), 1, renderer->file);

  /* initialize a well known colormap, see LookupColor */
  pPal = g_new(gint8, CC_LEN*CC_LEN*CC_LEN*3);
  for (i = 0; i < CC_LEN * CC_LEN * CC_LEN; i++)
  {
    pPal[3*i  ] = ((i % CC_LEN) * 255) / (CC_LEN - 1);   /* red */
    pPal[3*i+1] = (((i / CC_LEN) % CC_LEN) * 255) / (CC_LEN - 1); /* green */
    pPal[3*i+2] = ((i / (CC_LEN * CC_LEN)) * 255) / (CC_LEN - 1); /* blue varies least */
    /*
    g_print("%d\t%d\t%d\n", pPal[3*i  ], pPal[3*i+1], pPal[3*i+2]);
     */
  } 

  WriteRecHead(renderer, WPG_COLORMAP, CC_LEN*CC_LEN*CC_LEN*3+1*sizeof(guint16));
  i = WPG_NUM_DEF_COLORS; /* start color */
  fwrite(&i, sizeof(gint16), 1, renderer->file);
  i = CC_LEN*CC_LEN*CC_LEN;
  fwrite(&i, sizeof(gint16), 1, renderer->file);
  fwrite(pPal, 1, CC_LEN*CC_LEN*CC_LEN*3, renderer->file);

  /* FIXME: following 3 lines needed to make filling work !? */
  renderer->FillAttr.Type = WPG_FA_SOLID;
  WriteFillAttr(renderer, &color, TRUE);
  WriteFillAttr(renderer, &color, FALSE);

  g_free(pPal);
}

static void
end_render(MyRenderer *renderer)
{
  DIAG_NOTE(renderer, "end_render\n");

  WriteRecHead(renderer, WPG_END, 0); /* no data following */
  fclose(renderer->file);
}

static void
set_linewidth(MyRenderer *renderer, real linewidth)
{  
  DIAG_NOTE(renderer, "set_linewidth %f\n", linewidth);

  renderer->LineAttr.Width = SC(linewidth);
}

static void
set_linecaps(MyRenderer *renderer, LineCaps mode)
{
  DIAG_NOTE(renderer, "set_linecaps %d\n", mode);

  switch(mode) {
  case LINECAPS_BUTT:
    break;
  case LINECAPS_ROUND:
    break;
  case LINECAPS_PROJECTING:
    break;
  default:
    message_error(MY_RENDERER_NAME ": Unsupported fill mode specified!\n");
  }
}

static void
set_linejoin(MyRenderer *renderer, LineJoin mode)
{
  DIAG_NOTE(renderer, "set_join %d\n", mode);

  switch(mode) {
  case LINEJOIN_MITER:
    break;
  case LINEJOIN_ROUND:
    break;
  case LINEJOIN_BEVEL:
    break;
  default:
    message_error(MY_RENDERER_NAME ": Unsupported fill mode specified!\n");
  }
}

static void
set_linestyle(MyRenderer *renderer, LineStyle mode)
{
  DIAG_NOTE(renderer, "set_linestyle %d\n", mode);

  /* line type */
  switch (mode) {
  case LINESTYLE_SOLID:
    renderer->LineAttr.Type = WPG_LA_SOLID;
    break;
  case LINESTYLE_DASHED:
    if (renderer->dash_length < 0.5)
      renderer->LineAttr.Type = WPG_LA_SHORTDASH;
    else
      renderer->LineAttr.Type = WPG_LA_MEDIUMDASH;
    break;
  case LINESTYLE_DASH_DOT:
    renderer->LineAttr.Type = WPG_LA_DASHDOT;
    break;
  case LINESTYLE_DASH_DOT_DOT:
    renderer->LineAttr.Type = WPG_LA_DASHDOTDOT;
    break;
  case LINESTYLE_DOTTED:
    renderer->LineAttr.Type = WPG_LA_DOTS;
    break;
  default:
    message_error(MY_RENDERER_NAME ": Unsupported fill mode specified!\n");
  }
}

static void
set_dashlength(MyRenderer *renderer, real length)
{  
  DIAG_NOTE(renderer, "set_dashlength %f\n", length);

  /* dot = 20% of len */
  renderer->dash_length = length;
}

static void
set_fillstyle(MyRenderer *renderer, FillStyle mode)
{
  DIAG_NOTE(renderer, "set_fillstyle %d\n", mode);

  switch(mode) {
  case FILLSTYLE_SOLID:
    renderer->FillAttr.Type = WPG_FA_SOLID;
    break;
  default:
    message_error(MY_RENDERER_NAME ": Unsupported fill mode specified!\n");
  }
}

static void
set_font(MyRenderer *renderer, Font *font, real height)
{
  DIAG_NOTE(renderer, "set_font %f %s\n", height, font->name);
  renderer->TextStyle.Height = SC(height);

  if (strstr(font->name, "Courier"))
    renderer->TextStyle.Font = 0x0DF0;
  else if (strstr(font->name, "Times"))
    renderer->TextStyle.Font = 0x1950;
  else
    renderer->TextStyle.Font = 0x1150; /* Helv */
}

/* Need to translate coord system:
 * 
 *   Dia x,y -> Wpg x,-y
 *
 * doing it before scaling.
 */
static void
draw_line(MyRenderer *renderer, 
          Point *start, Point *end, 
          Color *line_colour)
{
  gint16 pData[4];

  DIAG_NOTE(renderer, "draw_line %f,%f -> %f, %f\n", 
            start->x, start->y, end->x, end->y);

  WriteLineAttr(renderer, line_colour);
  WriteRecHead(renderer, WPG_LINE, 4 * sizeof(gint16));

  /* point data */
  pData[0] = SC(start->x);
  pData[1] = SC(-start->y);
  pData[2] = SC(end->x);
  pData[3] = SC(-end->y);

  fwrite(pData, sizeof(gint16), 4, renderer->file);
}

static void
draw_polyline(MyRenderer *renderer, 
              Point *points, int num_points, 
              Color *line_colour)
{
  int i;
  gint16* pData;

  DIAG_NOTE(renderer, "draw_polyline n:%d %f,%f ...\n", 
            num_points, points->x, points->y);

  g_return_if_fail(1 < num_points);

  WriteLineAttr(renderer, line_colour);
  WriteRecHead(renderer, WPG_POLYLINE, num_points * 2 * sizeof(gint16) + sizeof(gint16));

  pData = g_new(gint16, num_points * 2);

  /* number of points */
  pData[0] = num_points;
  fwrite(pData, sizeof(gint16), 1, renderer->file);

  /* point data */
  for (i = 0; i < num_points; i++)
  {
    pData[2*i]   = SC(points[i].x);
    pData[2*i+1] = SC(-points[i].y);
  }

  fwrite(pData, sizeof(gint16), num_points*2, renderer->file);

  g_free(pData);
}

static void
draw_polygon(MyRenderer *renderer, 
             Point *points, int num_points, 
             Color *line_colour)
{
  gint16* pData;
  int i;

  DIAG_NOTE(renderer, "draw_polygon n:%d %f,%f ...\n", 
            num_points, points->x, points->y);

  WriteLineAttr(renderer, line_colour);
  WriteRecHead(renderer, WPG_POLYGON, (num_points * 2 + 1) * sizeof(gint16));

  pData = g_new(gint16, num_points * 2);

  /* number of vertices */
  pData[0] = num_points;
  fwrite(pData, sizeof(gint16), 1, renderer->file);

  /* point data */
  for (i = 0; i < num_points; i++)
  {
    pData[2*i]   = SC(points[i].x);
    pData[2*i+1] = SC(-points[i].y);
  }

  fwrite(pData, sizeof(gint16), num_points*2, renderer->file);

  g_free(pData);
}

static void
fill_polygon(MyRenderer *renderer, 
             Point *points, int num_points, 
             Color *colour)
{
  DIAG_NOTE(renderer, "fill_polygon n:%d %f,%f ...\n", 
            num_points, points->x, points->y);

  WriteFillAttr(renderer, colour, TRUE);
  draw_polygon(renderer,points,num_points,colour);
  WriteFillAttr(renderer, colour, FALSE);
}

static void
draw_rect(MyRenderer *renderer, 
          Point *ul_corner, Point *lr_corner,
          Color *colour)
{
  gint16* pData;
  DIAG_NOTE(renderer, "draw_rect %f,%f -> %f,%f\n", 
            ul_corner->x, ul_corner->y, lr_corner->x, lr_corner->y);

  WriteLineAttr(renderer, colour);
  WriteRecHead(renderer, WPG_RECTANGLE, 4*sizeof(gint16));

  pData = g_new(gint16, 4);
  pData[0] = SC(ul_corner->x); /* lower left corner ! */
  pData[1] = SC(-lr_corner->y);
  pData[2] = SC(lr_corner->x - ul_corner->x); /* width */
  pData[3] = SC(lr_corner->y - ul_corner->y); /* height */

  fwrite(pData, sizeof(gint16), 4, renderer->file);

  g_free(pData);
}

static void
fill_rect(MyRenderer *renderer, 
          Point *ul_corner, Point *lr_corner,
          Color *colour)
{
  DIAG_NOTE(renderer, "fill_rect %f,%f -> %f,%f\n", 
            ul_corner->x, ul_corner->y, lr_corner->x, lr_corner->y);

  WriteFillAttr(renderer, colour, TRUE);
  draw_rect(renderer, ul_corner, lr_corner, colour);
  WriteFillAttr(renderer, colour, FALSE);
}

static void
draw_arc(MyRenderer *renderer, 
	 Point *center,
	 real width, real height,
	 real angle1, real angle2,
	 Color *colour)
{
  WPGEllipse ell;

  DIAG_NOTE(renderer, "draw_arc %fx%f <%f,<%f\n", 
            width, height, angle1, angle2);

  ell.x = SC(center->x);
  ell.y = SC(-center->y);
  ell.RotAngle = 0;
  ell.rx = SC(width / 2.0);
  ell.ry = SC(height / 2.0);

  ell.StartAngle = angle1;
  ell.EndAngle   = angle2;
  ell.Flags = 0;

  WriteLineAttr(renderer, colour);
  WriteRecHead(renderer, WPG_ELLIPSE, sizeof(WPGEllipse));
  fwrite(&ell, sizeof(WPGEllipse), 1, renderer->file);
}

static void
fill_arc(MyRenderer *renderer, 
         Point *center,
         real width, real height,
         real angle1, real angle2,
         Color *colour)
{
  WPGEllipse ell;

  DIAG_NOTE(renderer, "fill_arc %fx%f <%f,<%f\n", 
            width, height, angle1, angle2);

  ell.x = SC(center->x);
  ell.y = SC(-center->y);
  ell.RotAngle = 0;
  ell.rx = SC(width / 2.0);
  ell.ry = SC(height / 2.0);

  ell.StartAngle = angle1;
  ell.EndAngle   = angle2;
  ell.Flags = 0; /* 0: connect to center; 1: connect start and end */

  WriteLineAttr(renderer, colour);
  WriteFillAttr(renderer, colour, TRUE);
  WriteRecHead(renderer, WPG_ELLIPSE, sizeof(WPGEllipse));
  fwrite(&ell, sizeof(WPGEllipse), 1, renderer->file);
  WriteFillAttr(renderer, colour, FALSE);
}

static void
draw_ellipse(MyRenderer *renderer, 
             Point *center,
             real width, real height,
             Color *colour)
{
  WPGEllipse ell;

  DIAG_NOTE(renderer, "draw_ellipse %fx%f center @ %f,%f\n", 
            width, height, center->x, center->y);

  ell.x = SC(center->x);
  ell.y = SC(-center->y);
  ell.RotAngle = 0;
  ell.rx = SC(width / 2.0);
  ell.ry = SC(height / 2.0);

  ell.StartAngle = 0;
  ell.EndAngle   = 360;
  ell.Flags = 0;

  WriteLineAttr(renderer, colour);
  WriteRecHead(renderer, WPG_ELLIPSE, sizeof(WPGEllipse));
  fwrite(&ell, sizeof(WPGEllipse), 1, renderer->file);
}

static void
fill_ellipse(MyRenderer *renderer, 
             Point *center,
             real width, real height,
             Color *colour)
{
  DIAG_NOTE(renderer, "fill_ellipse %fx%f center @ %f,%f\n", 
            width, height, center->x, center->y);

  WriteFillAttr(renderer, colour, TRUE);
  draw_ellipse(renderer,center,width,height,colour);
  WriteFillAttr(renderer, colour, FALSE);
}

static void
draw_bezier(MyRenderer *renderer, 
            BezPoint *points,
            int numpoints,
            Color *colour)
{
  gint16* pData;
  int i;

  DIAG_NOTE(renderer, "draw_bezier n:%d %fx%f ...\n", 
            numpoints, points->p1.x, points->p1.y);

  WriteLineAttr(renderer, colour);
  WriteRecHead(renderer, WPG_POLYCURVE, (numpoints * 2) * 2 * sizeof(gint16) + 3 * sizeof(gint16));

  pData = g_new(gint16, numpoints * 6);

  /* ??? size of equivalent data in pre5.1 files ? */
#if 1
  memset(pData, 0, sizeof(gint16)*2);
#else
  /* try first point instead */
  pData[0] = SC(points[0].p1.x);
  pData[1] = SC(-points[0].p1.y);
#endif
  fwrite(pData, sizeof(gint16), 2, renderer->file);

  pData[0] = numpoints * 2;
  fwrite(pData, sizeof(gint16), 1, renderer->file);

  /* WPG's Poly Curve is not quite the same as DIA's Bezier.
   * There is only one Control Point per Point and I can't 
   * get it right, but at least they are read without error.
   */
  for (i = 0; i < numpoints; i++)
  {
    switch (points[i].type)
    {
    case BEZ_MOVE_TO:
    case BEZ_LINE_TO:
      pData[4*i  ] =
      pData[4*i+2] = SC(points[i].p1.x);

      pData[4*i+1] =
      pData[4*i+3] = SC(-points[i].p1.y);
      break;
    case BEZ_CURVE_TO:
      /* real point */
      pData[4*i  ] = SC( points[i].p3.x);
      pData[4*i+1] = SC(-points[i].p3.y);
      /* first control point ? */
      pData[4*i+2] = SC( points[i].p1.x);
      pData[4*i+3] = SC(-points[i].p1.y);
      /* second control point ? */
      pData[4*i+2] = SC( points[i].p2.x);
      pData[4*i+3] = SC(-points[i].p2.y);
      break;
    }
  }

  fwrite(pData, sizeof(gint16), numpoints*4, renderer->file);
  g_free(pData);
}

static void
fill_bezier(MyRenderer *renderer, 
            BezPoint *points, /* Last point must be same as first point */
            int numpoints,
            Color *colour)
{
  DIAG_NOTE(renderer, "fill_bezier n:%d %fx%f ...\n", 
            numpoints, points->p1.x, points->p1.y);

}

static void
draw_string(MyRenderer *renderer,
            const char *text,
            Point *pos, Alignment alignment,
            Color *colour)
{
  gint16 len;
  WPGPoint pt;

  len = strlen(text);

  DIAG_NOTE(renderer, "draw_string(%d) %f,%f %s\n", 
            len, pos->x, pos->y, text);

  if (len < 1) return;

  renderer->TextStyle.YAlign = 3; /* bottom ??? */

  switch (alignment) {
  case ALIGN_LEFT:
    renderer->TextStyle.XAlign = 0;
    break;
  case ALIGN_CENTER:
    renderer->TextStyle.XAlign = 1;
    break;
  case ALIGN_RIGHT:
    renderer->TextStyle.XAlign = 2;
    break;
  }

  renderer->TextStyle.Color = LookupColor(renderer, colour);
  renderer->TextStyle.Angle = 0;

  renderer->TextStyle.Width = renderer->TextStyle.Height * 0.6; /* ??? */

  WriteRecHead(renderer, WPG_TEXTSTYLE, sizeof(WPGTextStyle));
  fwrite(&renderer->TextStyle, sizeof(WPGTextStyle), 1, renderer->file);

  pt.x = SC(pos->x);
  pt.y = SC(-pos->y);

  WriteRecHead(renderer, WPG_TEXT, 3*sizeof(gint16) + len);
  fwrite(&len, sizeof(gint16), 1, renderer->file);
  fwrite(&pt,  sizeof(WPGPoint), 1, renderer->file);
  fwrite(text, 1, len, renderer->file);
}

static void
draw_image(MyRenderer *renderer,
           Point *point,
           real width, real height,
           DiaImage image)
{
  WPGBitmap2 bmp;
  guint8* pOut = NULL, * pIn = NULL, * p = NULL;
  guint8 b_1, b, cnt;
  int x, y;

  DIAG_NOTE(renderer, "draw_image %fx%f @%f,%f\n", 
            width, height, point->x, point->y);

  bmp.Angle  = 0;
  bmp.Left   = SC(point->x);
  bmp.Top    = SC(-point->y - height);
  bmp.Right  = SC(point->x + width);
  bmp.Bottom = SC(-point->y);

  bmp.Width  = dia_image_width(image);
  bmp.Height = dia_image_height(image);
  bmp.Depth  = 8; /* maximum allowed */

  bmp.Xdpi = 72; /* ??? */
  bmp.Ydpi = 72;

  pIn = dia_image_rgb_data(image);
  pOut = g_new(guint8, bmp.Width * bmp.Height * 2); /* space for worst case RLE */
  p = pOut;

  pIn += (3* bmp.Width * (bmp.Height - 1)); /* last line */ 
  for (y = 0; y < bmp.Height; y++)
  {
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
    pIn -= (3 * bmp.Width * 2); /* start of previous line */
  }
  DIAG_NOTE(renderer, "Width x Height: %d RLE: %d\n", bmp.Width * bmp.Height, p - pOut);
  WriteRecHead(renderer, WPG_BITMAP2, sizeof(WPGBitmap2) + (p - pOut));
  fwrite(&bmp, sizeof(WPGBitmap2), 1, renderer->file);
  fwrite(pOut, sizeof(guint8), p - pOut, renderer->file);

#if 0
  /* RLE diagnose */
  {
    FILE* f;
    int i, j;
    f = fopen("c:\\temp\\wpg_bmp.raw", "wb");
    j = p - pOut;
    for (i = 0; i < j; i+=2)
    {
      for (x = 0; x < (pOut[i] & 0x7F); x++) 
        fwrite(&pOut[i+1], 1, 1, f);
    }
    fclose(f);
  }
#endif
  g_free(pOut);
}

static void
export_data(DiagramData *data, const gchar *filename, const gchar *diafilename)
{
  MyRenderer *renderer;
  FILE *file;
  Rectangle *extent;
  gint len;
  real width, height;

  file = fopen(filename, "wb"); /* "wb" for binary! */

  if (file == NULL) {
    message_error(_("Couldn't open: '%s' for writing.\n"), filename);
    return;
  }

  renderer = g_new0(MyRenderer, 1);
  renderer->renderer.ops = &MyRenderOps;
  renderer->renderer.is_interactive = 0;
  renderer->renderer.interactive_ops = NULL;

  renderer->file = file;

  extent = &data->extents;

  /* use extents */
  DIAG_NOTE(renderer, "export_data extents %f,%f -> %f,%f\n", 
            extent->left, extent->top, extent->right, extent->bottom);

  width  = extent->right - extent->left;
  height = extent->bottom - extent->top;
  renderer->Scale = 0.001;
  if (width > height)
    while (renderer->Scale * width < 3276.7) renderer->Scale *= 10.0;
  else
    while (renderer->Scale * height < 3276.7) renderer->Scale *= 10.0;

  renderer->Box.Width  = width * renderer->Scale;
  renderer->Box.Height = height * renderer->Scale;
  renderer->Box.Flag   = 0;
  renderer->Box.Version = 0;

  data_render(data, (Renderer *)renderer, NULL, NULL, NULL);

  g_free(renderer);
}

static const gchar *extensions[] = { "wpg", NULL };
static DiaExportFilter my_export_filter = {
    N_(MY_RENDERER_NAME),
    extensions,
    export_data
};


/* --- dia plug-in interface --- */

DIA_PLUGIN_CHECK_INIT

PluginInitResult
dia_plugin_init(PluginInfo *info)
{
    if (!dia_plugin_info_init(info, "WPG",
			      _("WordPerfect Graphics export filter"),
			      NULL, NULL))
	return DIA_PLUGIN_INIT_ERROR;

    filter_register_export(&my_export_filter);

    return DIA_PLUGIN_INIT_OK;
}
