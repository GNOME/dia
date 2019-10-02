/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * win32print.c - output file directly to a windoze printer
 *                Don't try it with a *non* PostScript printer !
 *
 * Copyright (C) 2001 Hans Breuer
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

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winspool.h>

#include <stdlib.h>
#include <stdio.h>

#include <fcntl.h>
#include <io.h>

#include <glib.h>

#include "win32print.h"

static void
PrintError (const char* s, DWORD err)
{
  if (0 != err)
  {
    char* lpBuffer;
    /* get the Windows message */
    FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER
                   | FORMAT_MESSAGE_FROM_SYSTEM
                   | FORMAT_MESSAGE_IGNORE_INSERTS,
                   NULL,
                   err,
                   MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                   (char*)&lpBuffer,
                   0,NULL);
    g_printerr ("%s : %s", s, lpBuffer);
    LocalFree (lpBuffer);
  }
}

const char*
win32_printer_default (void)
{
  static char sName[1024];
  GetProfileString("windows", "device", "", sName, sizeof(sName));
  if (strchr (sName, ','))
    *strchr (sName, ',') = 0;
  else if ( strlen (sName) < 1)
    strcpy (sName, "unknown");

  return sName;
}

static HANDLE hPrinter = NULL;
static ADDJOB_INFO_1* pJob = NULL;
static HANDLE hFile = INVALID_HANDLE_VALUE;

FILE*
win32_printer_open (char* sName)
{
  BYTE data [_MAX_PATH*2];
  DWORD dwID = 0;
  DWORD dwSizeRequired=0;
  int err = 0;

  if (!OpenPrinter (sName,
                    &hPrinter,
                    NULL))
  {
    g_printerr ("Failed to open printer : %s\n", sName);
    return NULL;
  }

  if (!AddJob (hPrinter, 1, data, sizeof(data), &dwSizeRequired))
  {
    PrintError ("Failed to add job", GetLastError());
  }
  else
  {
    pJob = (ADDJOB_INFO_1*)data;
    hFile = CreateFile (pJob->Path,
                        GENERIC_WRITE,
                        0,      /* no share */
                        NULL,   /* default security */
                        OPEN_ALWAYS,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL); /* template */
    if (INVALID_HANDLE_VALUE == hFile)
    {
      PrintError ("Failed to CreateFile", GetLastError());
    }
    else
    {
       int hnd = _open_osfhandle ((long)hFile, _O_APPEND);
	 if (-1 != hnd)
         return _fdopen (hnd, "wb");
       else
         PrintError ("Failed to _open_osfhandle", 0);
    }
  }

  return NULL;
}

int
win32_printer_close (FILE* f)
{
  int ret = 0;

  fflush (f);
  if (!pJob || !ScheduleJob (hPrinter, pJob->JobId))
  {
    PrintError ("Failed to schedule job", GetLastError());
    ret = GetLastError();
  }

  if (f)
    fclose (f);
  CloseHandle (hFile);
  ClosePrinter (hPrinter);
  return ret;
}
