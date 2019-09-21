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
#include <string.h>

// it's so ugly --hb ;(
// include before, cause it has extern "C" already

#include "paginate_gdiprint.h"

#if 1
extern "C" {

#include "filter.h"

#include "message.h"

}
#endif

namespace W32 {
// can't
// #define WIN32_LEAN_AND_MEAN
// because we need stuff like PRINTDLG
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
           DiaRectangle *bounds, gint xpos, gint ypos, DiaContext *ctx)
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
  W32::StartPage((W32::HDC)hDC);
  pExp->export_func(&page_data, ctx, "", "", (W32::HDC)hDC);
  W32::EndPage((W32::HDC)hDC);

  return nobjs;
}

static void
paginate_gdiprint(DiagramData *data, DiaExportFilter* pExp, W32::HANDLE hDC, DiaContext *ctx)
{
  DiaRectangle *extents;
  gdouble width, height;
  gdouble x, y, initx, inity;
  gint xpos, ypos;
  guint nobjs = 0;

  /* the usable area of the page */
  width = data->paper.width;
  height = data->paper.height;

  /* get extents, and make them multiples of width / height */
  extents = &data->extents;
  initx = extents->left;
  inity = extents->top;
  /* make page boundaries align with origin */
  if (!data->paper.fitto) {
    initx = floor(initx / width)  * width;
    inity = floor(inity / height) * height;
  }

  /* iterate through all the pages in the diagram */
  for (y = inity, ypos = 0; y < extents->bottom; y += height, ypos++) {
    /* ensure we are not producing pages for epsilon */
    if ((extents->bottom - y) < 1e-6)
      break;
    for (x = initx, xpos = 0; x < extents->right; x += width, xpos++) {
      DiaRectangle page_bounds;

      if ((extents->right - x) < 1e-6)
	break;

      page_bounds.left = x;
      page_bounds.right = x + width;
      page_bounds.top = y;
      page_bounds.bottom = y + height;

      nobjs += print_page(data, pExp, hDC, &page_bounds, xpos, ypos, ctx);
    }
  }
}

/* to remember changes made in the print dialog, finally leaked. */
static W32::HGLOBAL hDevMode = NULL;
static W32::HGLOBAL hDevNames = NULL;

extern "C"
void
diagram_print_gdi(DiagramData *data, const gchar *filename, DiaContext *ctx)
{
  W32::PRINTDLG printDlg;
  W32::DOCINFO  docInfo;
  W32::DEVMODE* pDevMode;
  DiaExportFilter* pExp = NULL;
  int i;

  pExp = filter_export_get_by_name("wmf");

  if (!pExp) {
    dia_context_add_message (ctx, "Can't print without the WMF plugin installed");
    return;
  }

  memset (&printDlg, 0, sizeof (W32::PRINTDLG));
  printDlg.hDevMode  = hDevMode;
  printDlg.hDevNames = hDevNames;

  printDlg.Flags = 0;
  printDlg.nMinPage = printDlg.nMaxPage = 0;
  printDlg.nCopies = 1;
  printDlg.lStructSize = sizeof (W32::PRINTDLG);

  /* Uhmm, first call to initialize device settings ... */
  if (!printDlg.hDevMode) {
    printDlg.Flags = PD_RETURNDEFAULT;
    if (!W32::PrintDlg (&printDlg))
      g_warning ("Failed to get printer defaults.");
  }

  pDevMode = (W32::DEVMODE*) W32::GlobalLock (printDlg.hDevMode);
  if (pDevMode) {
    /* initialize with Dia default */
    pDevMode->dmFields |= DM_ORIENTATION;
    pDevMode->dmOrientation = data->paper.is_portrait ?
      DMORIENT_PORTRAIT : DMORIENT_LANDSCAPE;

    /* Maybe we could adjust the scaling here as well but are al of:
     *   dmPaperSize, dmPaperLength, dmPaperWidth, dmScale
     * initialized despite of the api documentation ?
     */
    g_print ("Paper size %d, length %d width %d scale %d\n",
             pDevMode->dmPaperSize,
             pDevMode->dmPaperLength, pDevMode->dmPaperWidth, pDevMode->dmScale);

    W32::GlobalUnlock (printDlg.hDevMode);
  }

  printDlg.Flags = PD_RETURNDC | PD_NOSELECTION;

  if (!W32::PrintDlg (&printDlg)) {
    W32::DWORD dwError = W32::CommDlgExtendedError ();
    if (dwError) {
	const gchar *emsg;
      /* ugly common dialog api does *not* use standard windoze error codes */
	if (dwError < PDERR_PRINTERCODES) emsg = "Common Dialog Error";
	else if (dwError == PDERR_LOADDRVFAILURE) emsg = "Failed to load driver";
	else if (dwError == PDERR_PRINTERNOTFOUND) emsg = "Printer Not Found";
	else if (dwError < CFERR_CHOOSEFONTCODES) emsg = "Printer Selection Error";
	else emsg = "Unexpected Error"; /* Other common dialogs errors */

      message_error("Print Dialog failed with error %d\n%s",
                    dwError, emsg);
    }
    return;
  }

  /* remember settings made in dialog */
  hDevMode  = printDlg.hDevMode;
  hDevNames = printDlg.hDevNames;

  /* check capabilities ? */
  /* W32::GetDeviceCaps(print_dlg.hDC, RASTERCAPS) */

  docInfo.cbSize = sizeof(W32::DOCINFO);
  docInfo.lpszDocName = filename;
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
    paginate_gdiprint(data, pExp, printDlg.hDC, ctx);

  /* clean up */
  if (0 >= W32::EndDoc(printDlg.hDC))
    ERROR_RETURN(W32::GetLastError());
}
