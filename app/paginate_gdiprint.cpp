/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998, 1999 Alexander Larsson
 *
 * paginate_gdiprint.c -- pagination code for the win32 GDI. Rendering
 *   is done by the WMF plug-in.
 *
 * based on :
 *   paginate_gnomeprint.[ch] -- pagination code for the gnome-print backend
 *   Copyright (C) 1999 James Henstridge
 *
 * the rest is : 
 *   Copyright 2001 Hans Breuer <Hans@Breuer.Org>
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

#include <glib.h>
#include <math.h>

extern "C" {

#include "paginate_gdiprint.h"

#include "diagram.h"
#include "diagramdata.h"

#include "render.h"
#include "filter.h"

#include "message.h"

}

namespace W32 {
#include <windows.h>
}

#define ERROR_RETURN(err) \
{ \
  char *msg = g_win32_error_message(err); \
  message_error(msg); \
  g_free(msg); \
  return; \
}

static guint
print_page(DiagramData *data, DiaExportFilter* pExp, W32::HANDLE hDC,
           Rectangle *bounds, gint xpos, gint ypos)
{
  guint nobjs = 0;
  DiagramData page_data = *data; /* ugliness! */
  page_data.extents = *bounds; 

  /* transform coordinate system */
  if (data->paper.is_portrait) {
  } else {
  }

  /* set up clip mask ? */

  /* render the region */
  W32::StartPage(hDC);
  pExp->export(&page_data, "", "", hDC);
  W32::EndPage(hDC);

  return nobjs;
}

static void
paginate_gdiprint(Diagram *dia, DiaExportFilter* pExp, W32::HANDLE hDC)
{
  Rectangle *extents;
  gdouble width, height;
  gdouble x, y, initx, inity;
  gint xpos, ypos;
  guint nobjs = 0;

  /* the usable area of the page */
  width = dia->data->paper.width;
  height = dia->data->paper.height;

  /* get extents, and make them multiples of width / height */
  extents = &dia->data->extents;
  initx = extents->left;
  inity = extents->top;
  /* make page boundaries align with origin */
  if (!dia->data->paper.fitto) {
    initx = floor(initx / width)  * width;
    inity = floor(inity / height) * height;
  }

  /* iterate through all the pages in the diagram */
  for (y = inity, ypos = 0; y < extents->bottom; y += height, ypos++)
    for (x = inity, xpos = 0; x < extents->right; x += width, xpos++) {
      Rectangle page_bounds;

      page_bounds.left = x;
      page_bounds.right = x + width;
      page_bounds.top = y;
      page_bounds.bottom = y + height;

      nobjs += print_page(dia->data, pExp, hDC, &page_bounds, xpos, ypos);
    }

}

/* to remember changes made in the print dialog, finally leaked. */
static W32::HGLOBAL hDevMode = NULL;
static W32::HGLOBAL hDevNames = NULL;

extern "C"
void
diagram_print_gdi(Diagram *dia)
{
  W32::PRINTDLG printDlg;
  W32::DOCINFO  docInfo;
  DiaExportFilter* pExp = NULL;
  int i;

  pExp = filter_guess_export_filter("dummy.wmf");

  if (!pExp) {
    message_error("Can't print without the WMF plugin installed");
    return;
  }

  memset (&printDlg, 0, sizeof (W32::PRINTDLG));
  printDlg.hDevMode  = hDevMode;
  printDlg.hDevNames = hDevNames;

  printDlg.Flags = 0;
  printDlg.Flags |= PD_RETURNDC | PD_NOSELECTION;
  printDlg.nMinPage = printDlg.nMaxPage = 0;
  printDlg.nCopies = 1;
  printDlg.lStructSize = sizeof (W32::PRINTDLG);

  if (!W32::PrintDlg (&printDlg)) {
    W32::DWORD dwError = W32::CommDlgExtendedError ();
    if (dwError)
      message_error("Print Dialog failed with error %d", dwError);
    return;
  }

  /* remember settings made in dialog */
  hDevMode  = printDlg.hDevMode;
  hDevNames = printDlg.hDevNames;

  /* check capabilities ? */
  /* W32::GetDeviceCaps(print_dlg.hDC, RASTERCAPS) */

  docInfo.cbSize = sizeof(W32::DOCINFO);
  docInfo.lpszDocName = dia->filename;
  if (printDlg.Flags & PD_PRINTTOFILE)
    docInfo.lpszOutput = "FILE:";
  else
    docInfo.lpszOutput = NULL; /* use hDC */
  docInfo.fwType = 0;
  docInfo.lpszDatatype = NULL;

  /* finally print */
  if (0 >= W32::StartDoc(printDlg.hDC, &docInfo))
    ERROR_RETURN(W32::GetLastError());
  for (i = 0; i < printDlg.nCopies; i++)
    paginate_gdiprint(dia, pExp, printDlg.hDC);

  /* clean up */
  if (0 >= W32::EndDoc(printDlg.hDC))
    ERROR_RETURN(W32::GetLastError());
}
